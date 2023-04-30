#include "Perceptron.h"
#include <stdlib.h>

Perceptron::Perceptron()
{
	for (int i = 0; i < PERCEPTRON_WEIGHT_COUNT; i++)
		_weights[i] = 0;
}

Perceptron::Perceptron(float weights[PERCEPTRON_WEIGHT_COUNT])
{
	for (int i = 0; i < PERCEPTRON_WEIGHT_COUNT; i++)
		_weights[i] = weights[i];
}

float Perceptron::Calculate(float inputs[PERCEPTRON_WEIGHT_COUNT])
{
	float sum = 0;
	for (int i = 0; i < PERCEPTRON_WEIGHT_COUNT; i++)
		sum += _weights[i] * inputs[i];
	return sum;
}
