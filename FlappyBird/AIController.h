#pragma once

#include "GameState.hpp"
#include <nlohmann/json.hpp>
#include "NeuralNetwork.h"

using json = nlohmann::json;

class AIController
{
public:
	AIController();
	~AIController();

	void Init();

	void setGameState(GameState* pGameState) { m_pGameState = pGameState; }
	void update(Bird* bird);
	bool shouldFlap(); // note when this is called, it resets the flap state

	void BirdDied(Bird* bird, int score);

	void CreateNewGeneration();
	void SaveCurrentGeneration();
	void Log(std::string output);

public:

private:
	float distanceToFloor(Land* land, Bird* bird);
	float distanceToNearestPipes(Pipe* pipe, Bird* bird);
	float distanceToCentreOfPipeGap(Pipe* pipe, Bird* bird);

	float Mutate(float input);
private:
	GameState*	m_pGameState;
	bool		m_bShouldFlap;

	std::vector<NeuralNetwork*> _neuralNetworks;

	json _currentGeneration;
	int _currentGenerationNum;
	int _currentChromosomeNum;

};

