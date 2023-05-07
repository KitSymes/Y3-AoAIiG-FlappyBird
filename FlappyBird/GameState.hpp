#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "State.hpp"
#include "Game.hpp"
#include "Pipe.hpp"
#include "Land.hpp"
#include "Bird.hpp"
#include "Collision.hpp"
#include "Flash.hpp"
#include "HUD.hpp"

using namespace Sonar;

class AIController;

namespace Sonar
{
	class GameState : public State
	{
	public:
		GameState(GameDataRef data);

		void Init();
		void CleanUp();

		void HandleInput();
		void Update(float dt);
		void Draw(float dt);

		Pipe* GetPipeContainer() { return pipe; };
		Land* GetLandContainer() { return land; };
		//Bird* GetBird() { return bird; }

	private:
		GameDataRef _data;

		sf::Sprite _background;

		Pipe *pipe;
		Land *land;
		//Bird *bird;
		std::vector<Bird*> birds;
		Collision collision;
		Flash *flash;
		HUD *hud;

		bool _init = false;

		sf::Clock clock;

		int _gameState;

		sf::RectangleShape _gameOverFlash;
		bool _flashOn;

		int _score;

		sf::SoundBuffer _hitSoundBuffer;
		sf::SoundBuffer _wingSoundBuffer;
		sf::SoundBuffer _pointSoundBuffer;

		sf::Sound _hitSound;
		sf::Sound _wingSound;
		sf::Sound _pointSound;

		AIController* m_pAIController;
	};
}