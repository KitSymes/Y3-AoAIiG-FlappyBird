#pragma once

#include "Neuron.h"

#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class NeuralNetwork
{
public:
	// Create Neural Network from JSON
	NeuralNetwork(json json);
	~NeuralNetwork();

	float Calculate(std::vector<float> inputs, int layer);
private:
	std::vector<std::vector<Neuron*>> _hiddenLayers;
	Neuron* _output;
};

