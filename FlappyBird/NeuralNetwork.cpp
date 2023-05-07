#include "NeuralNetwork.h"
#include "DEFINITIONS.hpp"

NeuralNetwork::NeuralNetwork(json networkJSON)
{
	for (int layer = 0; layer < HIDDEN_LAYER_COUNT; layer++)
	{
		json layerJSON = networkJSON[JSON_LAYER + std::to_string(layer)];

		std::vector<Neuron*> layerNeurons;

		for (int neuron = 0; neuron < NEURONS_PER_HIDDEN_LAYER; neuron++)
		{
			json neuronJSON = layerJSON[JSON_NEURON + std::to_string(neuron)];

			std::vector<float> weights;
			for (float weight : neuronJSON[JSON_WEIGHTS])
				weights.push_back(weight);

			float bias = neuronJSON[JSON_BIAS];

			layerNeurons.push_back(new Neuron(weights, bias));
		}

		_hiddenLayers.push_back(layerNeurons);
	}

	json outputJSON = networkJSON[JSON_OUTPUT];

	std::vector<float> weights;
	for (float weight : outputJSON[JSON_WEIGHTS])
		weights.push_back(weight);

	float bias = outputJSON[JSON_BIAS];

	_output = new Neuron(weights, bias);
}

NeuralNetwork::~NeuralNetwork()
{
	for (int layer = 0; layer < _hiddenLayers.size(); layer++)
		for (int neuron = 0; neuron < _hiddenLayers[layer].size(); neuron++)
			delete _hiddenLayers[layer][neuron];

	delete _output;
}

float NeuralNetwork::Calculate(std::vector<float> inputs, int layer)
{
	if (layer >= HIDDEN_LAYER_COUNT)
		return _output->Calculate(inputs);

	std::vector<float> outputs;

	//for (int layer = 0; layer < _hiddenLayers.size(); layer++)
	for (int neuron = 0; neuron < _hiddenLayers[layer].size(); neuron++)
		outputs.push_back(_hiddenLayers[layer][neuron]->Calculate(inputs));

	return Calculate(outputs, layer + 1);
}
