#ifndef GAME_H
#define GAME_H

#include "GameState.h"
#include "Move.h"

#include <string>
#include <algorithm>
#include <unordered_map>
#include <chrono>

// just for debugging
inline int counter = 0;
inline int branches = 0;
inline int extCounter = 0;

class Game
{
public:
	enum class AfterMoveState
	{
		INCORRECT_MOVE,
		CORRECT_MOVE,
		REPETITION,
		FIFTY_MOVES_RULE
	};

	Game();

	AfterMoveState makeMove(const Move& move);

	Move findBestMove();

	Move constructMoveFromStr(const std::string& moveDescription) const;

private:
	std::pair<Move, int> findMove(std::uint8_t depth, int bestPrevBalance, const GameState& gameState);

	std::pair<Move, int> quiescenceSearch(std::uint8_t depth, int bestPrevBalance, const GameState& gameState);

	bool addGamePosition(const Board& board);

	void eraseGamePosition(const Board& board);

	GameState m_gamePos{};

	std::unordered_map<Board, std::uint8_t, HashBoard, EqualBoard> m_gamePositions{};
	std::unordered_map<Board, std::pair<std::pair<Move, int>, std::uint8_t>, HashBoard, EqualBoard> m_dpEvaluation{};
};

#endif
