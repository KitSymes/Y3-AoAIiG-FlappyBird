#include "Game.hpp"
#include "DEFINITIONS.hpp"

int main()
{
	srand(time(NULL));

	Sonar::Game(SCREEN_WIDTH, SCREEN_HEIGHT, "Flappy Bird");

	return EXIT_SUCCESS;
}