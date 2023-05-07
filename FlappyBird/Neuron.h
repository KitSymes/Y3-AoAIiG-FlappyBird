#pragma once
#include <vector>

class Neuron
{
public:
	Neuron(std::vector<float> weights, float bias);

	float Calculate(std::vector<float> inputs);
private:
	std::vector<float> _weights;
	float _bias;

	float Activate(float sum);
};