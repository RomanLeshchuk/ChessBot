#include <iostream>

#include "Game.h"

int main()
{
	Game game{};

	std::cout << "Two numbers, 0 for bot, 1 for player, first for white, second for black:\n";

	bool whiteType, blackType;
	std::cin >> whiteType >> blackType;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	for (int i = 0;; i++)
	{
		Game::AfterMoveState flag;
		if (i & 1 ? blackType : whiteType)
		{
			std::string moveDescription;
			while (true)
			{
				std::cout << (i & 1 ? "Black" : "White") << ": ";
				std::getline(std::cin, moveDescription);
				try
				{
					Move move = game.constructMoveFromStr(moveDescription);
					if (move != Move::noMove)
					{
						flag = game.makeMove(move);
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
			Move move = game.findBestMove();
			if (move.describeMoveAsStr() == "mate")
			{
				std::cout << (i & 1 ? "Black" : "White") << ": got matted\n";
				break;
			}
			if (move.describeMoveAsStr() == "stalemate")
			{
				std::cout << (i & 1 ? "Black" : "White") << ": got stalemated\n";
				break;
			}
			flag = game.makeMove(move);
			std::cout << (i & 1 ? "Black" : "White") << ": " << move.describeMoveAsStr() << '\n';
		}

		if (flag == Game::AfterMoveState::INCORRECT_MOVE)
		{
			std::cout << "Incorrect move\n";
			i--;
			continue;
		}

		if (flag == Game::AfterMoveState::REPETITION)
		{
			std::cout << "Repetition\n";
			break;
		}

		if (flag == Game::AfterMoveState::FIFTY_MOVES_RULE)
		{
			std::cout << "Fifty moves rule\n";
			break;
		}

		std::cout << "Average branch factor: " << static_cast<double>(counter) / branches << '\n';
		std::cout << "Total positions generated: " << counter + extCounter << "\n\n";

		counter = 0;
		branches = 0;
		extCounter = 0;
	}
}
