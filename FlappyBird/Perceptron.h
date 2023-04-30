#pragma once
#define PERCEPTRON_WEIGHT_COUNT 3
class Perceptron
{
public:
	Perceptron();
	Perceptron(float weights[PERCEPTRON_WEIGHT_COUNT]);

	float Calculate(float inputs[PERCEPTRON_WEIGHT_COUNT]);
private:
	float _weights[PERCEPTRON_WEIGHT_COUNT];
};