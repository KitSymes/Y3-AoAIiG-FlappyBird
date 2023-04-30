#pragma once

#include <SFML/Graphics.hpp>

#include "DEFINITIONS.hpp"
#include "Game.hpp"

#include <vector>
#include "Perceptron.h"

namespace Sonar
{
	class Bird
	{
	public:
		Bird(GameDataRef data, int id, Perceptron perceptron);
		~Bird();

		void Draw();

		void Animate(float dt);

		void Update(float dt);

		void Tap();

		const sf::Sprite &GetSprite() const;

		void getHeight(int& x, int& y);

		void Die(int score);
		bool IsDead() { return BIRD_STATE_DEAD == _birdState; }

		int GetID() { return _id; }
		Perceptron GetPerceptron() { return _perceptron; }
	private:
		GameDataRef _data;

		sf::Sprite _birdSprite;
		std::vector<sf::Texture> _animationFrames;

		unsigned int _animationIterator;

		sf::Clock _clock;

		sf::Clock _movementClock;

		int _birdState;

		float _rotation;

		int _id;
		Perceptron _perceptron;
	};
}