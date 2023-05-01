#pragma once

#include <sstream>
#include "DEFINITIONS.hpp"
#include "GameState.hpp"
#include "GameOverState.hpp"
#include "AIController.h"

#include <iostream>
#include <fstream>
#include <bitset>

#define PLAY_WITH_AI 1

namespace Sonar
{
	GameState::GameState(GameDataRef data) : _data(data)
	{
		m_pAIController = new AIController();
		m_pAIController->setGameState(this);
	}

	void GameState::CleanUp()
	{
		if (!_init)
			return;

		CreateNewGeneration();

		for (Bird* bird : birds)
			delete bird;

		delete pipe;
		delete land;
		delete flash;
		delete hud;
	}

	void GameState::Init()
	{
		_init = true;

		if (!_hitSoundBuffer.loadFromFile(HIT_SOUND_FILEPATH))
		{
			std::cout << "Error Loading Hit Sound Effect" << std::endl;
		}

		if (!_wingSoundBuffer.loadFromFile(WING_SOUND_FILEPATH))
		{
			std::cout << "Error Loading Wing Sound Effect" << std::endl;
		}

		if (!_pointSoundBuffer.loadFromFile(POINT_SOUND_FILEPATH))
		{
			std::cout << "Error Loading Point Sound Effect" << std::endl;
		}

		_hitSound.setBuffer(_hitSoundBuffer);
		_wingSound.setBuffer(_wingSoundBuffer);
		_pointSound.setBuffer(_pointSoundBuffer);

		this->_data->assets.LoadTexture("Game Background", GAME_BACKGROUND_FILEPATH);
		this->_data->assets.LoadTexture("Pipe Up", PIPE_UP_FILEPATH);
		this->_data->assets.LoadTexture("Pipe Down", PIPE_DOWN_FILEPATH);
		this->_data->assets.LoadTexture("Land", LAND_FILEPATH);
		this->_data->assets.LoadTexture("Bird Frame 1", BIRD_FRAME_1_FILEPATH);
		this->_data->assets.LoadTexture("Bird Frame 2", BIRD_FRAME_2_FILEPATH);
		this->_data->assets.LoadTexture("Bird Frame 3", BIRD_FRAME_3_FILEPATH);
		this->_data->assets.LoadTexture("Bird Frame 4", BIRD_FRAME_4_FILEPATH);
		this->_data->assets.LoadTexture("Scoring Pipe", SCORING_PIPE_FILEPATH);
		this->_data->assets.LoadFont("Flappy Font", FLAPPY_FONT_FILEPATH);

		pipe = new Pipe(_data);
		land = new Land(_data);
		//bird = new Bird(_data);
		flash = new Flash(_data);
		hud = new HUD(_data);

		_background.setTexture(this->_data->assets.GetTexture("Game Background"));

		_score = 0;
		hud->UpdateScore(_score);

		// JSON Loading
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
				if (!_currentGeneration["chromosome_" + std::to_string(chromosome)].contains("score"))
				{
					_currentChromosomeNum = chromosome;
					break;
				}
		}

		if (_currentGenerationNum < 0)
		{
			for (int chromosome = 0; chromosome < BIRD_COUNT; chromosome++)
			{
				for (int i = 0; i < PERCEPTRON_WEIGHT_COUNT; i++)
					_currentGeneration["chromosome_" + std::to_string(chromosome)]["weights"][i] = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 20)) - 10.0f;

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

		for (int i = 0; i < BIRD_COUNT; i++)
		{
			float weights[PERCEPTRON_WEIGHT_COUNT];
			_currentGeneration["chromosome_" + std::to_string(i)]["weights"].get_to(weights);
			birds.push_back(new Bird(_data, i, Perceptron(weights)));
		}

		_gameState = GameStates::eReady;
	}

	void GameState::HandleInput()
	{
#if PLAY_WITH_AI
		if (GameStates::eGameOver != _gameState)
		{
			_gameState = GameStates::ePlaying;

			for (Bird* bird : birds)
			{
				m_pAIController->update(bird);

				if (m_pAIController->shouldFlap())
				{
					bird->Tap();
#if !SILENT
					_wingSound.play();
#endif
				}
			}
		}
#endif
		sf::Event event;
		while (this->_data->window.pollEvent(event))
		{
			if (sf::Event::Closed == event.type)
			{
				this->_data->window.close();
			}

			if (this->_data->input.IsSpriteClicked(this->_background, sf::Mouse::Left, this->_data->window))
			{
				if (GameStates::eGameOver != _gameState)
				{
					_gameState = GameStates::ePlaying;
					birds[0]->Tap();

#if !SILENT
					_wingSound.play();
#endif
				}
			}
		}
	}

	void GameState::Update(float dt)
	{
		if (GameStates::eGameOver != _gameState)
		{
			for (Bird* bird : birds)
				bird->Animate(dt);
			land->MoveLand(dt);
		}

		if (GameStates::ePlaying == _gameState)
		{
			pipe->MovePipes(dt);

			if (clock.getElapsedTime().asSeconds() > PIPE_SPAWN_FREQUENCY)
			{
				pipe->RandomisePipeOffset();

				pipe->SpawnInvisiblePipe();
				pipe->SpawnBottomPipe();
				pipe->SpawnTopPipe();
				pipe->SpawnScoringPipe();

				clock.restart();
			}

			std::vector<sf::Sprite> landSprites = land->GetSprites();

			// Assume all birds are dead
			_gameState = GameStates::eGameOver;
			bool scored = false;

			for (Bird* bird : birds)
			{
				bird->Update(dt);

				if (bird->IsDead())
					continue;

				// A bird is not dead, so we keep playing
				_gameState = GameStates::ePlaying;

				for (unsigned int i = 0; i < landSprites.size(); i++)
				{
					if (collision.CheckSpriteCollision(bird->GetSprite(), 0.7f, landSprites.at(i), 1.0f, false))
					{
						bird->Die(_score);
						Log(std::to_string(bird->GetID()) + " died at " + std::to_string(_score) + "\n");
						_currentGeneration["chromosome_" + std::to_string(bird->GetID())]["score"] = _score;

#if !SILENT
						_hitSound.play();
#endif
						break;
					}
				}

				if (bird->IsDead())
					continue;

				std::vector<sf::Sprite> pipeSprites = pipe->GetSprites();

				for (unsigned int i = 0; i < pipeSprites.size(); i++)
				{
					if (collision.CheckSpriteCollision(bird->GetSprite(), 0.625f, pipeSprites.at(i), 1.0f, true))
					{
						bird->Die(_score);
						Log(std::to_string(bird->GetID()) + " died at " + std::to_string(_score) + "\n");
						_currentGeneration["chromosome_" + std::to_string(bird->GetID())]["score"] = _score;

#if !SILENT
						_hitSound.play();
#endif
						break;
					}
				}

				if (bird->IsDead())
					continue;

				if (GameStates::ePlaying == _gameState)
				{
					std::vector<sf::Sprite>& scoringSprites = pipe->GetScoringSprites();

					for (unsigned int i = 0; i < scoringSprites.size(); i++)
					{
						if (collision.CheckSpriteCollision(bird->GetSprite(), 0.625f, scoringSprites.at(i), 1.0f, false))
						{
							scored = true;
							scoringSprites.erase(scoringSprites.begin() + i);
						}
					}
				}
			}

			if (scored)
			{
				_score++;
				hud->UpdateScore(_score);
#if !SILENT
				_pointSound.play();
#endif
			}

			// If all the birds died, reset
			if (_gameState == GameStates::eGameOver)
				clock.restart();
		}

		if (GameStates::eGameOver == _gameState)
		{
			flash->Show(dt);

			if (clock.getElapsedTime().asSeconds() > TIME_BEFORE_GAME_OVER_APPEARS)
			{
				// TODO record data
				this->_data->machine.AddState(StateRef(new GameState(_data)), true);
			}
		}
	}

	void GameState::Draw(float dt)
	{
		this->_data->window.clear(sf::Color::Red);

		this->_data->window.draw(this->_background);

		pipe->DrawPipes();
		land->DrawLand();

		for (Bird* bird : birds)
			bird->Draw();

		flash->Draw();

		hud->Draw();

		this->_data->window.display();
	}

	void GameState::CreateNewGeneration()
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
					std::to_string((int)_currentGeneration["chromosome_" + std::to_string(tournament[groupNum][i])]["score"])
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
				if (_currentGeneration["chromosome_" + std::to_string(current)]["score"]
			> _currentGeneration["chromosome_" + std::to_string(max)]["score"])
					max = current;
			}

			winners[group] = _currentGeneration["chromosome_" + std::to_string(max)];
			Log(std::to_string(max) +
				"(" +
				std::to_string((int)_currentGeneration["chromosome_" + std::to_string(max)]["score"]) +
				")");
			if (group < PARENT_COUNT - 1)
				Log(", ");
		}
		Log("\n");

		_currentGeneration.clear();

		// Encode
		for (int round = 0; round < PARENT_COUNT; round++)
			for (float weight : winners[round]["weights"])
			{
				int recast = *(int*)&weight;
				winningChromosomes[round] += std::bitset<32>(recast).to_string();
			}

		int bitsPerGene = 32;
		int currentChildChromsome = 0;
		for (int first = 0; first < PARENT_COUNT; first++)
			for (int second = 0; second < PARENT_COUNT; second++)
			{
				std::vector<std::string> children;
				if (first == second)
					children.push_back(winningChromosomes[first]);
				else
				{
					std::string chromosomeA = winningChromosomes[first];
					std::string chromosomeB = winningChromosomes[second];

					// Crossover
					// A bird is 96 bits total - 3 * 32 (3 floats)
					/*for (int i = 0; i < chromosomeA.size() / bitsPerGene; i++)
					{
						if (i % 2 == 0)
							children += chromosomeA.substr(i * bitsPerGene, bitsPerGene);
						else
							child += chromosomeB.substr(i * bitsPerGene, bitsPerGene);
					}*/

					children.push_back("");
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
					children[2] += chromosomeB.substr(bitsPerGene * 2, bitsPerGene);
				}

				for (int childNum = 0; childNum < children.size(); childNum++)
				{
					while (!children[childNum].empty())
					{
						json weights;
						for (int perceptronNum = 0; perceptronNum < PERCEPTRON_WEIGHT_COUNT; perceptronNum++)
						{
							int n = 0;
							for (int i = 0; i < 32; ++i)
								n |= (children[childNum][(perceptronNum * 32) + i] - 48) << 31 - i;
							float f = *(float*)&n;
							// Mutation
							if (rand() % 100 < 1)
							{
								int max = 10;

								if (_currentGenerationNum <= 100)
									max = 10000 - _currentGenerationNum * 100;

								// Evaluates to between -10 and 10, and shrinks by 0.1 each generation. Then stays at 0.01.
								float kohonenAdjustment = ((rand() % (max * 2)) - max) / 1000.0f;
								f += kohonenAdjustment;
								std::cout << kohonenAdjustment << std::endl;
							}
							_currentGeneration["chromosome_" + std::to_string(currentChildChromsome)]["weights"].push_back(f);
						}
						children[childNum].erase(0, 96);
					}
					currentChildChromsome++;
				}
			}


		SaveCurrentGeneration();
	}

	void GameState::SaveCurrentGeneration()
	{
		std::ofstream o("generation_" + std::to_string(_currentGenerationNum) + ".json");
		o << std::setw(4) << _currentGeneration << std::endl;
		o.close();
	}

	void GameState::Log(std::string output)
	{
		std::cout << output;
		std::ofstream myfile;
		myfile.open("log.txt", std::ios::out | std::ios::app);
		myfile << output;
		myfile.close();
	}
}