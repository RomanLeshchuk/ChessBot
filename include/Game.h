#ifndef Game_H
#define Game_H

#include "Data.h"
#include "GameState.h"
#include "Move.h"

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <chrono>

int counter = 0;
int branches = 0;
int extCounter = 0;

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

	Game()
	{
		EngineData::initHash();
		m_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
		addGamePosition(m_gamePos.getBoard());
	}

	AfterMoveState makeMove(const Move& move)
	{
		const auto& [gameState, isCorrect] = m_gamePos.move(move);

		if (!isCorrect)
		{
			return AfterMoveState::INCORRECT_MOVE;
		}

		if (addGamePosition(gameState.getBoard()))
		{
			return AfterMoveState::REPETITION;
		}

		if (gameState.isFiftyMovesRule())
		{
			return AfterMoveState::FIFTY_MOVES_RULE;
		}

		m_gamePos = gameState;

		return AfterMoveState::CORRECT_MOVE;
	}

	Move findBestMove()
	{
		std::uint8_t depth = 0;
		const auto startTime = std::chrono::steady_clock::now();
		while (true)
		{
			const auto& [move, evaluation] = findMove(
				depth,
				m_gamePos.isWhiteMove() ? FigureData::noMoveBalance : -FigureData::noMoveBalance,
				m_gamePos
			);
			if (depth >= EngineData::minSearchLevel && std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - startTime
				).count() > EngineData::searchTimeMilliseconds)
			{
				return move;
			}
			depth += 2;
		}
	}

	Move constructMoveFromStr(const std::string& moveDescription) const
	{
		Move move{};
		move.constructMoveFromStr(moveDescription, m_gamePos);
		return move;
	}

private:
	std::pair<Move, int> findMove(std::uint8_t depth, int bestPrevBalance, const GameState& gameState)
	{
		auto it = m_dpEvaluation.find(gameState.getBoard());
		if (it != m_dpEvaluation.end() && it->second.second >= depth + EngineData::quiescenceSearchDepth)
		{
			return it->second.first;
		}

		// null move pruning
		if (gameState.getGamephase()
			&& depth > EngineData::nullSearchDepthReduction
			&& !gameState.isPosUnderPressure(
				gameState.isWhiteMove() ? gameState.getWhiteKingPos() : gameState.getBlackKingPos(),
				!gameState.isWhiteMove()
			))
		{
			GameState skipMoveState = gameState.skipMove();

			int afterSkipEvaluation = findMove(
				depth - EngineData::nullSearchDepthReduction - 1,
				skipMoveState.isWhiteMove() ? FigureData::noMoveBalance : -FigureData::noMoveBalance,
				skipMoveState
			).second;

			if (gameState.isWhiteMove() ? afterSkipEvaluation >= bestPrevBalance : afterSkipEvaluation <= bestPrevBalance)
			{
				return { Move::noMove, afterSkipEvaluation };
			}
		}

		std::vector<std::pair<GameState, Move>> allStates{};
		for (Move move : gameState.findAllMoves())
		{
			const auto& [nextGameState, isCorrect] = gameState.move(move);
			if (isCorrect)
			{
				allStates.emplace_back(nextGameState, move);
			}
		}

		std::sort(allStates.begin(), allStates.end(), [&](
			const std::pair<GameState, Move>& firstState,
			const std::pair<GameState, Move>& secondState
		) -> bool
		{
			if (it != m_dpEvaluation.end())
			{
				if (firstState.second == it->second.first.first)
				{
					return true;
				}
				if (secondState.second == it->second.first.first)
				{
					return false;
				}
			}

			return gameState.isWhiteMove()
				? firstState.first.getEvaluation() > secondState.first.getEvaluation()
				: firstState.first.getEvaluation() < secondState.first.getEvaluation();
		});

		std::pair<Move, int> bestMove{
			Move::noMove,
			gameState.isWhiteMove() ? -FigureData::noMoveBalance : FigureData::noMoveBalance
		};
		branches++;
		for (std::pair<GameState, Move>& nextState : allStates)
		{
			counter++;
			if (gameState.isWhiteMove() ? bestMove.second >= bestPrevBalance : bestMove.second <= bestPrevBalance)
			{
				return bestMove;
			}

			if ((addGamePosition(nextState.first.getBoard())
				|| nextState.first.isFiftyMovesRule())
				&& (gameState.isWhiteMove() ? bestMove.second < 0 : bestMove.second > 0))
			{
				bestMove = { nextState.second, 0 };
			}
			else
			{
				std::pair<Move, int> nextBestMove{ nextState.second, nextState.first.getEvaluation() };

				if (depth > 1)
				{
					nextBestMove = findMove(depth - 1, bestMove.second, nextState.first);
				}
				else if (depth == 1 && (
					gameState.isMoveCapture(nextState.second)
					|| nextState.first.isPosUnderPressure(
						gameState.isWhiteMove() ? nextState.first.getBlackKingPos() : nextState.first.getWhiteKingPos(),
						gameState.isWhiteMove()
					)
				))
				{
					std::pair<Move, int> possibleCaptureExtendedMove = quiescenceSearch(
						EngineData::quiescenceSearchDepth,
						bestMove.second,
						nextState.first
					);

					if (possibleCaptureExtendedMove.first != Move::noMove)
					{
						nextBestMove = possibleCaptureExtendedMove;
					}
				}

				if (gameState.isWhiteMove() ? nextBestMove.second > bestMove.second : nextBestMove.second < bestMove.second)
				{
					bestMove = { nextState.second, nextBestMove.second };
				}
			}

			eraseGamePosition(nextState.first.getBoard());
		}

		if (bestMove.first == Move::noMove)
		{
			if (gameState.isPosUnderPressure(gameState.isWhiteMove() ? gameState.getWhiteKingPos() : gameState.getBlackKingPos(), !gameState.isWhiteMove()))
			{
				bestMove = {
					Move::mate,
					gameState.isWhiteMove() ? -FigureData::noMoveBalance : FigureData::noMoveBalance
				};
			}
			else
			{
				bestMove = { Move::stalemate, 0 };
			}
		}

		if (bestMove.second > FigureData::mateLimit)
		{
			bestMove.second--;
		}
		else if (bestMove.second < -FigureData::mateLimit)
		{
			bestMove.second++;
		}

		if (depth)
		{
			if (m_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
			{
				m_dpEvaluation.clear();
				m_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
			}

			m_dpEvaluation[gameState.getBoard()] = { bestMove, depth + EngineData::quiescenceSearchDepth };
		}

		return bestMove;
	}

	std::pair<Move, int> quiescenceSearch(std::uint8_t depth, int bestPrevBalance, const GameState& gameState)
	{
		if (m_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
		{
			m_dpEvaluation.clear();
		}
		if (m_dpEvaluation.empty())
		{
			m_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
		}

		auto it = m_dpEvaluation.find(gameState.getBoard());
		if (it != m_dpEvaluation.end() && it->second.second >= depth)
		{
			return it->second.first;
		}

		std::vector<std::pair<GameState, Move>> captureStates{};

		for (Move move : gameState.findAllMoves())
		{
			const auto& [nextGameState, isCorrect] = gameState.move(move);

			if (isCorrect && (
				gameState.isMoveCapture(move)
				|| nextGameState.isPosUnderPressure(
					nextGameState.isWhiteMove() ? nextGameState.getWhiteKingPos() : nextGameState.getBlackKingPos(),
					gameState.isWhiteMove()
				)
			))
			{
				captureStates.emplace_back(nextGameState, move);
			}
		}

		if (captureStates.empty())
		{
			if (depth % 2 == 0)
			{
				return { Move::noMove, gameState.getEvaluation() };
			}

			std::pair<Move, int> bestMove = findMove(0, bestPrevBalance, gameState);

			if (m_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
			{
				m_dpEvaluation.clear();
				m_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
			}

			m_dpEvaluation[gameState.getBoard()] = { bestMove, depth };

			return bestMove;
		}

		std::sort(captureStates.begin(), captureStates.end(), [&](
			const std::pair<GameState, Move>& firstState,
			const std::pair<GameState, Move>& secondState
			) -> bool
			{
				if (it != m_dpEvaluation.end())
				{
					if (firstState.second == it->second.first.first)
					{
						return true;
					}
					if (secondState.second == it->second.first.first)
					{
						return false;
					}
				}

				return gameState.isWhiteMove()
					? firstState.first.getEvaluation() > secondState.first.getEvaluation()
					: firstState.first.getEvaluation() < secondState.first.getEvaluation();
			}
		);

		std::pair<Move, int> bestMove{
			Move::noMove,
			gameState.isWhiteMove() ? -FigureData::noMoveBalance : FigureData::noMoveBalance
		};

		for (std::pair<GameState, Move>& captureState : captureStates)
		{
			extCounter++;
			if (gameState.isWhiteMove() ? bestMove.second >= bestPrevBalance : bestMove.second <= bestPrevBalance)
			{
				return bestMove;
			}

			std::pair<Move, int> nextBestMove{ captureState.second, captureState.first.getEvaluation() };

			if (depth > 1)
			{
				std::pair<Move, int> possibleCaptureExtendedMove = quiescenceSearch(
					depth - 1,
					bestMove.second,
					captureState.first
				);

				if (possibleCaptureExtendedMove.first != Move::noMove)
				{
					nextBestMove = possibleCaptureExtendedMove;
				}
			}

			if (gameState.isWhiteMove() ? nextBestMove.second > bestMove.second : nextBestMove.second < bestMove.second)
			{
				bestMove = { captureState.second, nextBestMove.second };
			}
		}

		if (bestMove.first == Move::noMove)
		{
			if (gameState.isPosUnderPressure(
				gameState.isWhiteMove() ? gameState.getWhiteKingPos() : gameState.getBlackKingPos(),
				!gameState.isWhiteMove()
			))
			{
				bestMove = {
					Move::mate,
					gameState.isWhiteMove() ? -FigureData::noMoveBalance : FigureData::noMoveBalance
				};
			}
			else
			{
				bestMove = { Move::stalemate, 0 };
			}
		}

		if (bestMove.second > FigureData::mateLimit)
		{
			bestMove.second--;
		}
		else if (bestMove.second < -FigureData::mateLimit)
		{
			bestMove.second++;
		}

		if (m_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
		{
			m_dpEvaluation.clear();
			m_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
		}

		m_dpEvaluation[gameState.getBoard()] = { bestMove, depth };

		return bestMove;
	}

	bool addGamePosition(const Board& board)
	{
		return ++m_gamePositions[board] == GameData::maxRepetitions;
	}

	void eraseGamePosition(const Board& board)
	{
		const auto it = m_gamePositions.find(board);

		if (it == m_gamePositions.end())
		{
			return;
		}

		if (it->second > 1)
		{
			it->second--;
		}
		else
		{
			m_gamePositions.erase(it);
		}
	}

	GameState m_gamePos{};

	std::unordered_map<Board, std::uint8_t, HashBoard, EqualBoard> m_gamePositions{};
	std::unordered_map<Board, std::pair<std::pair<Move, int>, std::uint8_t>, HashBoard, EqualBoard> m_dpEvaluation{};
};

#endif
