#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Board.h"

class Move;

class GameState
{
public:
    GameState();

	GameState(const GameState& other) = default;
	GameState(GameState&& other) = default;

	GameState& operator=(const GameState& other) = default;
	GameState& operator=(GameState&& other) = default;

    [[nodiscard]] bool isMoveCapture(const Move& move) const;

	[[nodiscard]] bool isPosUnderPressure(std::uint8_t pos, bool isWhiteAttack) const;

	[[nodiscard]] bool canCastle(bool isWhite, bool isShort) const;

	[[nodiscard]] std::vector<Move> findAllMoves() const;

	[[nodiscard]] std::pair<GameState, bool> move(const Move& move) const;
	[[nodiscard]] GameState skipMove() const;

	void updatePossiblyEnPassantedPawn();

	[[nodiscard]] const Board& getBoard() const;

	[[nodiscard]] bool isFiftyMovesRule() const;

	[[nodiscard]] bool isWhiteMove() const;

	[[nodiscard]] std::uint8_t getWhiteKingPos() const;
	[[nodiscard]] std::uint8_t getBlackKingPos() const;

	void evaluate();

	[[nodiscard]] std::uint8_t getGamephase() const;
	[[nodiscard]] int getEvaluation() const;

private:
    Board m_board{};
    std::uint8_t m_consecutiveNonConfrontingMoves = 0;
    std::uint8_t m_whiteKingPos;
	std::uint8_t m_blackKingPos;
	int m_evaluation = 0;
	std::uint8_t m_gamephase = 0;
};

#endif
