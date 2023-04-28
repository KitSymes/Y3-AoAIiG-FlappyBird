#pragma once

#include <sstream>
#include "DEFINITIONS.hpp"
#include "GameState.hpp"
#include "GameOverState.hpp"
#include "AIController.h"

#include <iostream>

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
		for (int i = 0; i < BIRD_COUNT; i++)
			birds.push_back(new Bird(_data, i));
		flash = new Flash(_data);
		hud = new HUD(_data);

		_background.setTexture(this->_data->assets.GetTexture("Game Background"));

		_score = 0;
		hud->UpdateScore(_score);

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
					_wingSound.play();
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

					_wingSound.play();
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

			for (Bird* bird : birds)
				bird->Update(dt);

			std::vector<sf::Sprite> landSprites = land->GetSprites();

			// Assume all birds are dead
			_gameState = GameStates::eGameOver;
			bool scored = false;

			for (Bird* bird : birds)
			{
				if (bird->IsDead())
					continue;

				// A bird is not dead, so we keep playing
				_gameState = GameStates::ePlaying;

				for (unsigned int i = 0; i < landSprites.size(); i++)
				{
					if (collision.CheckSpriteCollision(bird->GetSprite(), 0.7f, landSprites.at(i), 1.0f, false))
					{
						bird->Die(_score);

						_hitSound.play();
					}
				}

				std::vector<sf::Sprite> pipeSprites = pipe->GetSprites();

				for (unsigned int i = 0; i < pipeSprites.size(); i++)
				{
					if (collision.CheckSpriteCollision(bird->GetSprite(), 0.625f, pipeSprites.at(i), 1.0f, true))
					{
						bird->Die(_score);

						_hitSound.play();
					}
				}

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
				_pointSound.play();
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
				this->_data->machine.AddState(StateRef(new GameOverState(_data, _score)), true);
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
}