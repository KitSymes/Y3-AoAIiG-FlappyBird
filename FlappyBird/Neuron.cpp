#include "Neuron.h"
#include <stdlib.h>

Neuron::Neuron(std::vector<float> weights, float bias)
{
	_weights = weights;
	_bias = bias;
}

float Neuron::Calculate(std::vector<float> inputs)
{
	float sum = 0;
	for (int i = 0; i < inputs.size(); i++)
		sum += _weights[i] * inputs[i];

	return Activate(sum + _bias);
}

// Sign activation function (is the number positive or not)
float Neuron::Activate(float sum)
{
	if (sum < 0.0f)
		return -1.0f;
	return 1.0f;
}
