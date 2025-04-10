#include <iostream>

#include "GameState.h"

int main()
{
	GameState game(true);

	std::cout << "Two numbers, 0 for bot, 1 for player, first for white, second for black:\n";

	bool whiteType, blackType;
	std::cin >> whiteType >> blackType;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	for (int i = 0;; i++)
	{
		std::pair<GameState, GameState::AfterMoveState> moveState;
		if (i & 1 ? blackType : whiteType)
		{
			std::string moveDescription;
			while (true)
			{
				std::cout << (i & 1 ? "Black" : "White") << ": ";
				std::getline(std::cin, moveDescription);
				try
				{
					GameState::Move move = game.constructMove(moveDescription);
					if (move != GameState::noMove)
					{
						moveState = game.makeMove(move);
						break;
					}
					std::cout << "Incorrect move\n";
				}
				catch (std::exception&)
				{
					std::cout << "Incorrect move\n";
				}
			}
		}
		else
		{
			GameState::Move move = game.findBestMove();
			if (GameState::describeMove(move) == "mate")
			{
				std::cout << (i & 1 ? "Black" : "White") << ": got matted\n";
				break;
			}
			if (GameState::describeMove(move) == "stalemate")
			{
				std::cout << (i & 1 ? "Black" : "White") << ": got stalemated\n";
				break;
			}
			moveState = game.makeMove(move);
			std::cout << (i & 1 ? "Black" : "White") << ": " << GameState::describeMove(move) << '\n';
		}

		if (moveState.second == GameState::AfterMoveState::INCORRECT_MOVE)
		{
			std::cout << "Incorrect move\n";
			i--;
			continue;
		}

		if (moveState.second == GameState::AfterMoveState::REPETITION)
		{
			std::cout << "Repetition\n";
			break;
		}

		if (moveState.second == GameState::AfterMoveState::FIFTY_MOVES_RULE)
		{
			std::cout << "Fifty moves rule\n";
			break;
		}

		game = moveState.first;

		std::cout << "Average branch factor: " << static_cast<double>(counter) / branches << '\n';
		std::cout << "Total positions generated: " << counter + extCounter << "\n\n";

		counter = 0;
		branches = 0;
		extCounter = 0;
	}
}
