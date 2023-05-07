#include "AIController.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <bitset>

using namespace std;
#define ERROR_DISTANCE 9999


AIController::AIController()
{
	m_pGameState = nullptr;
	m_bShouldFlap = false;

#if EXPORT
	std::ofstream o("export.csv");

	_currentGenerationNum = 0;
	o << ",";
	for (int i = 0; i < BIRD_COUNT; i++)
		o << std::to_string(i) << ",";
	o << std::endl;

	while (true)
	{
		std::ifstream f("generation_" + std::to_string(_currentGenerationNum) + ".json");
		if (!f.good())
			break;
		o << _currentGenerationNum << ",";
		_currentGeneration = json::parse(f);
		for (int birdNum = 0; birdNum < BIRD_COUNT; birdNum++)
		{
			if (!_currentGeneration[JSON_CHROMOSOME + std::to_string(birdNum)].contains(JSON_SCORE))
				break;
			o << _currentGeneration[JSON_CHROMOSOME + std::to_string(birdNum)][JSON_SCORE] << ",";
		}
		o << std::endl;
		_currentGenerationNum++;
	}

	o.close();
	exit(0);
#endif
}

void AIController::Init()
{
	// JSON Loading

#if REPLAY
	std::ifstream f("generation_" + std::to_string(REPLAY_GENERATION) + ".json");
	_currentGeneration = json::parse(f);

	_currentGenerationNum = REPLAY_GENERATION;

	std::cout << "Replaying " + std::to_string(_currentGenerationNum) + "\n" << std::endl;
#else
	_currentGenerationNum = -1;
	_currentChromosomeNum = -1;

	while (_currentChromosomeNum < 0)
	{
		std::ifstream f("generation_" + std::to_string(_currentGenerationNum + 1) + ".json");
		if (!f.good())
			break;
		_currentGenerationNum++;
		_currentGeneration = json::parse(f);
		for (int chromosome = 0; chromosome < BIRD_COUNT; chromosome++)
			if (!_currentGeneration[JSON_CHROMOSOME + std::to_string(chromosome)].contains(JSON_SCORE))
			{
				_currentChromosomeNum = chromosome;
				break;
			}
	}

	// No Generation found, so create one
	if (_currentGenerationNum < 0)
	{
		for (int chromosome = 0; chromosome < BIRD_COUNT; chromosome++)
		{
			// Create a Network
			json networkJSON;

			for (int layer = 0; layer < HIDDEN_LAYER_COUNT; layer++)
			{
				json layerJSON;

				for (int neuron = 0; neuron < NEURONS_PER_HIDDEN_LAYER; neuron++)
				{
					json neuronJSON;

					// Randomise Weights

					int weightCount = NEURONS_PER_HIDDEN_LAYER;
					// If this is the first hidden layer, the inputs are the input values
					if (layer == 0)
						weightCount = INPUT_COUNT;

					for (int i = 0; i < weightCount; i++)
						neuronJSON[JSON_WEIGHTS].push_back((static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (RANDOM_WIEGHT_MAX * 2))) - RANDOM_WIEGHT_MAX);

					// Randomise Bias
					neuronJSON[JSON_BIAS] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (RANDOM_BIAS_MAX * 2))) - RANDOM_BIAS_MAX;

					// Add Neuron to Layer JSON
					layerJSON[JSON_NEURON + std::to_string(neuron)] = neuronJSON;
				}

				// Add Layer to Network JSON
				networkJSON[JSON_LAYER + std::to_string(layer)] = layerJSON;
			}

			// ----- Output Neuron
			json outputJSON;

			// Randomise Weights
			for (int i = 0; i < NEURONS_PER_HIDDEN_LAYER; i++)
				outputJSON[JSON_WEIGHTS].push_back((static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (RANDOM_WIEGHT_MAX * 2))) - RANDOM_WIEGHT_MAX);

			// Randomise Bias
			outputJSON[JSON_BIAS] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (RANDOM_BIAS_MAX * 2))) - RANDOM_BIAS_MAX;

			// Add Output Neuron to Network JSON
			networkJSON[JSON_OUTPUT] = outputJSON;

			// ----- Add Network to Generation JSON
			_currentGeneration[JSON_CHROMOSOME + std::to_string(chromosome)] = networkJSON;
		}

		_currentGenerationNum = 0;
		_currentChromosomeNum = 0;

		SaveCurrentGeneration();
	}
	else if (_currentChromosomeNum < 0)
	{
		// Stopped after completing a generation but didn't create a new one
		CreateNewGeneration();
	}

	std::cout << "Starting at " + std::to_string(_currentGenerationNum) + "\n" << std::endl;
#endif

	for (int chromosome = 0; chromosome < BIRD_COUNT; chromosome++)
		_neuralNetworks.push_back(new NeuralNetwork(_currentGeneration[JSON_CHROMOSOME + std::to_string(chromosome)]));
}

AIController::~AIController()
{
#if !REPLAY
	CreateNewGeneration();
#endif

	for (int i = 0; i < _neuralNetworks.size(); i++)
		delete _neuralNetworks[i];
}

// update - the AI method which determines whether the bird should flap or not. 
// set m_bShouldFlap to true or false.
void AIController::update(Bird* bird)
{
	if (m_pGameState == nullptr || bird->IsDead())
		return;

	Pipe* pipe = m_pGameState->GetPipeContainer();
	Land* land = m_pGameState->GetLandContainer();
	//Bird* bird = m_pGameState->GetBird();

	// do some AI stuff, decide whether to flap
	float fDistanceToFloor = distanceToFloor(land, bird);
	float fDistanceToNearestPipe = distanceToNearestPipes(pipe, bird);

	std::vector<float> inputs;
	inputs.push_back(fDistanceToFloor);
	inputs.push_back(fDistanceToNearestPipe);
	inputs.push_back(444.0f);

	if (fDistanceToNearestPipe != ERROR_DISTANCE) {
		float fDistanceToCentreOfGap = distanceToCentreOfPipeGap(pipe, bird);

		inputs[2] = fDistanceToCentreOfGap;
	}

	m_bShouldFlap = _neuralNetworks[bird->GetID()]->Calculate(inputs, 0) > 0.0f;

	// this means the birdie always flaps. Should only be called when the bird should need to flap. 
	//m_bShouldFlap = true;

	return;
}

float AIController::distanceToFloor(Land* land, Bird* bird)
{
	// the land is always the same height so get the first sprite
	std::vector<sf::Sprite> landSprites = land->GetSprites();
	if (landSprites.size() > 0)
	{
		return landSprites.at(0).getPosition().y - bird->GetSprite().getPosition().y;
	}

	return ERROR_DISTANCE; // this is an error but also means 
}

float AIController::distanceToNearestPipes(Pipe* pipe, Bird* bird)
{
	float nearest1 = 999999;
	sf::Sprite* nearestSprite1 = nullptr;

	// get nearest pipes
	std::vector<sf::Sprite> pipeSprites = pipe->GetSprites();
	for (unsigned int i = 0; i < pipeSprites.size(); i++) {
		sf::Sprite s = pipeSprites.at(i);
		float fDistance = s.getPosition().x - bird->GetSprite().getPosition().x;
		if (fDistance > 0 && fDistance < nearest1) {
			nearestSprite1 = &(pipeSprites.at(i));
			nearest1 = fDistance;
		}
	}

	if (nearestSprite1 == nullptr)
		return ERROR_DISTANCE;

	return nearestSprite1->getPosition().x - bird->GetSprite().getPosition().x;
}

float AIController::distanceToCentreOfPipeGap(Pipe* pipe, Bird* bird)
{
	float nearest1 = 999999;
	float nearest2 = 999999;
	sf::Sprite* nearestSprite1 = nullptr;
	sf::Sprite* nearestSprite2 = nullptr;

	// get nearest pipes
	std::vector<sf::Sprite> pipeSprites = pipe->GetSprites();
	for (unsigned int i = 0; i < pipeSprites.size(); i++) {
		sf::Sprite s = pipeSprites.at(i);
		float fDistance = s.getPosition().x - bird->GetSprite().getPosition().x;
		if (fDistance > 0 && fDistance < nearest1) {
			nearestSprite1 = &(pipeSprites.at(i));
			nearest1 = fDistance;
		}
		else if (fDistance > 0 && fDistance < nearest2) {
			nearestSprite2 = &(pipeSprites.at(i));
			nearest2 = fDistance;
		}
	}

	if (nearestSprite1 == nullptr || nearestSprite2 == nullptr)
		return ERROR_DISTANCE;


	sf::Sprite* topSprite = nullptr;
	sf::Sprite* bottomSprite = nullptr;

	if (nearestSprite1->getPosition().y < nearestSprite2->getPosition().y) {
		topSprite = nearestSprite1;
		bottomSprite = nearestSprite2;
	}
	else {
		topSprite = nearestSprite2;
		bottomSprite = nearestSprite1;
	}

	float distance = ((bottomSprite->getGlobalBounds().top) - (topSprite->getGlobalBounds().height + topSprite->getGlobalBounds().top)) / 2;
	distance += (topSprite->getGlobalBounds().top + topSprite->getGlobalBounds().height);

	return distance - bird->GetSprite().getPosition().y;
}

// note when this is called, it resets the flap state (don't edit)
bool AIController::shouldFlap()
{
	bool output = m_bShouldFlap;
	m_bShouldFlap = false;

	return output;
}

void AIController::BirdDied(Bird* bird, int score)
{
	Log(std::to_string(bird->GetID()) + " died at " + std::to_string(score) + "\n");
	_currentGeneration[JSON_CHROMOSOME + std::to_string(bird->GetID())][JSON_SCORE] = score;
}

void AIController::CreateNewGeneration()
{
	// Generate a seed so that the results are repeatable
	unsigned int seed = unsigned int(time(NULL));
	if (_currentGeneration.contains("seed"))
		seed = _currentGeneration["seed"];
	else
		_currentGeneration["seed"] = seed;
	srand(seed);
	SaveCurrentGeneration();

	_currentChromosomeNum = 0;
	_currentGenerationNum++;

	// Parent genes for next generation
	json winners[PARENT_COUNT];
	std::string winningChromosomes[PARENT_COUNT];

	// Selection

	// Run tournaments until PARENT_COUNT have been chosen
	std::vector<int> temp;
	std::vector<std::vector<int>> tournament;

	for (int i = 0; i < PARENT_COUNT; i++)
	{
		std::vector<int> group;
		tournament.push_back(group);
	}

	// Fill temp with json indexes
	for (int i = 0; i < BIRD_COUNT; i++)
		temp.push_back(i);

	int currentGroup = 0;
	while (temp.size() > 0)
	{
		// Get a random index of temp
		int random = rand() % temp.size();
		// Add the json index at that temp index to the current group
		tournament[currentGroup].push_back(temp[random]);
		// Erase the temp index to prevent the json index from being chosen more than once
		temp.erase(temp.begin() + random);

		currentGroup = (currentGroup + 1) % PARENT_COUNT;
	}

	// Print out the groups
	for (int groupNum = 0; groupNum < PARENT_COUNT; groupNum++)
	{
		Log("Group " + std::to_string(groupNum) + ": ");
		for (int i = 0; i < tournament[groupNum].size(); i++)
		{
			Log(std::to_string(tournament[groupNum][i]) +
				" (" +
				std::to_string((int)_currentGeneration[JSON_CHROMOSOME + std::to_string(tournament[groupNum][i])][JSON_SCORE])
				+ ")");
			if (i < tournament[groupNum].size() - 1)
				Log(", ");
		}
		Log("\n");
	}

	Log("Parents are: ");
	for (int group = 0; group < PARENT_COUNT; group++)
	{
		int max = tournament[group][0];
		// Compare the genes
		// Start at 1 because 0 is already max by default
		for (int i = 1; i < tournament[group].size(); i++)
		{
			int current = tournament[group][i];
			if (_currentGeneration[JSON_CHROMOSOME + std::to_string(current)][JSON_SCORE]
		> _currentGeneration[JSON_CHROMOSOME + std::to_string(max)][JSON_SCORE])
				max = current;
		}

		winners[group] = _currentGeneration[JSON_CHROMOSOME + std::to_string(max)];
		Log(std::to_string(max) +
			"(" +
			std::to_string((int)_currentGeneration[JSON_CHROMOSOME + std::to_string(max)][JSON_SCORE]) +
			")");
		if (group < PARENT_COUNT - 1)
			Log(", ");
	}
	Log("\n");

	_currentGeneration.clear();


	// Encode
	for (int round = 0; round < PARENT_COUNT; round++)
	{
		json networkJSON = winners[round];

		// Iterate through each Layer
		for (int layer = 0; layer < HIDDEN_LAYER_COUNT; layer++)
		{
			json layerJSON = networkJSON[JSON_LAYER + std::to_string(layer)];

			// Encode each Neuron in this Layer
			for (int neuron = 0; neuron < NEURONS_PER_HIDDEN_LAYER; neuron++)
			{
				json neuronJSON = layerJSON[JSON_NEURON + std::to_string(neuron)];

				for (float weight : neuronJSON[JSON_WEIGHTS])
				{
					int recast = *(int*)&weight;
					winningChromosomes[round] += std::bitset<32>(recast).to_string();
				}

				float bias = neuronJSON[JSON_BIAS];
				int recast = *(int*)&bias;
				winningChromosomes[round] += std::bitset<32>(recast).to_string();
			}
		}

		// Encode this Layer's Output Neuron
		json outputJSON = networkJSON[JSON_OUTPUT];

		for (float weight : outputJSON[JSON_WEIGHTS])
		{
			int recast = *(int*)&weight;
			winningChromosomes[round] += std::bitset<32>(recast).to_string();
		}

		float bias = outputJSON[JSON_BIAS];
		int recast = *(int*)&bias;
		winningChromosomes[round] += std::bitset<32>(recast).to_string();

		/*for (float weight : winners[round]["weights"])
		{
			int recast = *(int*)&weight;
			winningChromosomes[round] += std::bitset<32>(recast).to_string();
		}*/
	}

	int bitsPerFloat = 32;
	int currentChildChromsome = 0;

	for (int first = 0; first < PARENT_COUNT; first++)
		for (int second = 0; second < PARENT_COUNT; second++)
		{
			std::string child;
			if (first == second)
				child = winningChromosomes[first];
			else
			{
				std::string chromosomeA = winningChromosomes[first];
				std::string chromosomeB = winningChromosomes[second];


				// Crossover
				for (int i = 0; i < chromosomeA.size() / bitsPerFloat; i++)
				{
					if (i % 2 == 0)
						child += chromosomeA.substr(i * bitsPerFloat, bitsPerFloat);
					else
						child += chromosomeB.substr(i * bitsPerFloat, bitsPerFloat);
				}

				/*children.push_back("");
				children.push_back("");
				children.push_back("");

				children[0] += chromosomeA.substr(0, bitsPerGene);
				children[0] += chromosomeA.substr(bitsPerGene, bitsPerGene);
				children[0] += chromosomeB.substr(bitsPerGene * 2, bitsPerGene);

				children[1] += chromosomeA.substr(0, bitsPerGene);
				children[1] += chromosomeB.substr(bitsPerGene, bitsPerGene);
				children[1] += chromosomeA.substr(bitsPerGene * 2, bitsPerGene);

				children[2] += chromosomeA.substr(0, bitsPerGene);
				children[2] += chromosomeB.substr(bitsPerGene, bitsPerGene);
				children[2] += chromosomeB.substr(bitsPerGene * 2, bitsPerGene);*/
			}

			int currentOffset = 0;

			// ----- Decode

			// Create a Network
			json networkJSON;

			// Iterate through each Layer
			for (int layer = 0; layer < HIDDEN_LAYER_COUNT; layer++)
			{
				json layerJSON;

				// Decode all Neurons in this Layer
				for (int neuron = 0; neuron < NEURONS_PER_HIDDEN_LAYER; neuron++)
				{
					json neuronJSON;

					int weightCount = NEURONS_PER_HIDDEN_LAYER;
					// If this is the first hidden layer, the inputs are the input values
					if (layer == 0)
						weightCount = INPUT_COUNT;

					// Decode this Neuron's Weights
					for (int i = 0; i < weightCount; i++)
					{
						int n = 0;
						for (int i = 0; i < bitsPerFloat; ++i)
							n |= (child[currentOffset + i] - 48) << 31 - i;
						currentOffset += bitsPerFloat;
						float decode = *(float*)&n;

						neuronJSON[JSON_WEIGHTS].push_back(Mutate(decode));
					}

					// Decode this Neuron's Bias
					int n = 0;
					for (int i = 0; i < bitsPerFloat; ++i)
						n |= (child[currentOffset + i] - 48) << 31 - i;
					currentOffset += bitsPerFloat;
					float decode = *(float*)&n;

					neuronJSON[JSON_BIAS] = Mutate(decode);

					// Add Neuron to Layer JSON
					layerJSON[JSON_NEURON + std::to_string(neuron)] = neuronJSON;
				}

				// Add Layer to Network JSON
				networkJSON[JSON_LAYER + std::to_string(layer)] = layerJSON;
			}

			// Decode this Layer's Output Neuron
			json outputJSON;

			for (int i = 0; i < NEURONS_PER_HIDDEN_LAYER; i++)
			{
				int n = 0;
				for (int i = 0; i < bitsPerFloat; ++i)
					n |= (child[currentOffset + i] - 48) << 31 - i;
				currentOffset += bitsPerFloat;
				float decode = *(float*)&n;

				outputJSON[JSON_WEIGHTS].push_back(Mutate(decode));
			}

			int n = 0;
			for (int i = 0; i < bitsPerFloat; ++i)
				n |= (child[currentOffset + i] - 48) << 31 - i;
			currentOffset += bitsPerFloat;
			float decode = *(float*)&n;

			outputJSON[JSON_BIAS] = Mutate(decode);

			// Add Output Neuron to Network JSON
			networkJSON[JSON_OUTPUT] = outputJSON;

			// Add to new Generation
			_currentGeneration[JSON_CHROMOSOME + std::to_string(currentChildChromsome)] = networkJSON;

			if (child.size() != currentOffset)
				Log("ERROR! DECODING FAILED!\n");
			currentChildChromsome++;
		}

	SaveCurrentGeneration();
}

void AIController::SaveCurrentGeneration()
{
	std::ofstream o("generation_" + std::to_string(_currentGenerationNum) + ".json");
	o << std::setw(4) << _currentGeneration << std::endl;
	o.close();
}

void AIController::Log(std::string output)
{
	//std::cout << output;
	std::ofstream myfile;
	myfile.open("log.txt", std::ios::out | std::ios::app);
	myfile << output;
	myfile.close();
}

float AIController::Mutate(float input)
{
	// Mutation
	if (rand() % 100 < 1)
	{
		float max = 1.0f;

		if (_currentGenerationNum < 100)
			max -= _currentGenerationNum * 0.01f;
		else
			max = 0.01f;

		// Evaluates to between -max and max, and shrinks by 0.01 each generation. Then stays at 0.01.
		float kohonenAdjustment = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (max * 2))) - max;

		std::cout << kohonenAdjustment << std::endl;
		input += kohonenAdjustment;
	}
	return input;
}

