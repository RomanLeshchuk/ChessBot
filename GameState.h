#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Data.h"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

int counter = 0;
int branches = 0;
int extCounter = 0;

class GameState
{
public:
	using Move = std::pair<std::uint8_t, std::uint8_t>;

	enum class AfterMoveState
	{
		INCORRECT_MOVE,
		CORRECT_MOVE,
		REPETITION,
		FIFTY_MOVES_RULE
	};

	GameState() = default;

	GameState(bool) :
		m_board{ BoardData::startBoard },
		m_consecutiveNonConfrontingMoves{ 0 }
	{
		EngineData::initHash();
		s_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);

		for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
		{
			if ((m_board[pos] & Bit::type) == Bit::king)
			{
				if (m_board[pos] & Bit::white)
				{
					m_whiteKingPos = pos;
				}
				else
				{
					m_blackKingPos = pos;
				}
			}
		}

		this->addGamePosition();
	}

	GameState(
		const std::array<std::uint8_t, BoardData::cellsCount>& board,
		std::uint8_t consecutiveNonConfrontingMoves,
		std::uint8_t whiteKingPos,
		std::uint8_t blackKingPos
	) :
		m_board{ board },
		m_consecutiveNonConfrontingMoves{ consecutiveNonConfrontingMoves },
		m_whiteKingPos{ whiteKingPos },
		m_blackKingPos{ blackKingPos }
	{
	}

	std::pair<GameState, AfterMoveState> makeMove(Move move) const
	{
		std::pair<GameState, bool> moveData = this->move(move);

		if (moveData.second)
		{
			if (moveData.first.addGamePosition())
			{
				return { moveData.first, AfterMoveState::REPETITION };
			}

			if (moveData.first.m_consecutiveNonConfrontingMoves == GameData::maxConsecutiveNonConfrontingMoves)
			{
				return { moveData.first, AfterMoveState::FIFTY_MOVES_RULE };
			}

			return { moveData.first, AfterMoveState::CORRECT_MOVE };
		}

		return { moveData.first, AfterMoveState::INCORRECT_MOVE };
	}

	Move findBestMove()
	{
		this->evaluate();

		return this->findMove(
			EngineData::searchDepth,
			m_board[m_whiteKingPos] & Bit::specialData ? FigureData::noMoveBalance : -FigureData::noMoveBalance
		).first;
	}

	Move constructMove(const std::string& description)
	{
		Move move{
			GameState::constructPos(description.substr(0, BoardData::posDescriptionSize)),
			GameState::constructPos(description.substr(
				BoardData::posDescriptionSize + BoardData::spaceDescriptionSize,
				BoardData::posDescriptionSize
			))
		};

		if (!(m_board[move.first] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black)))
		{
			return GameState::noMove;
		}

		if (description.size() != BoardData::moveDescriptionSize)
		{
			move.first ^= Bit::promote;
			std::uint8_t promotedFigureType = std::find(
				FigureData::promotedFiguresNames.begin(),
				FigureData::promotedFiguresNames.end(),
				description.substr(BoardData::moveDescriptionSize + BoardData::spaceDescriptionSize)
			) - FigureData::promotedFiguresNames.begin();
			move.second ^= (promotedFigureType << Bit::posSize);
		}
		else if ((m_board[move.first] & Bit::type) == Bit::king
			&& !(m_board[move.first] & Bit::moved)
			&& (m_board[move.second] & Bit::type) == Bit::rook
			&& !(m_board[move.second] & Bit::moved))
		{
			move.first ^= ((move.second & Bit::coord) == FigureData::startRookX ? Bit::longCastle : Bit::shortCastle);
		}

		std::vector<Move> allMoves = this->findAllMoves();

		if (std::find(allMoves.begin(), allMoves.end(), move) == allMoves.end())
		{
			return GameState::noMove;
		}

		return this->move(move).second ? move : GameState::noMove;
	}

	static std::uint8_t constructPos(const std::string& description)
	{
		return ((std::uint8_t)(Bit::coord - (description[1] - '1')) << Bit::sideSize) + (std::uint8_t)(description[0] - 'a');
	}

	static std::string describePos(std::uint8_t pos)
	{
		return std::string(1, 'a' + (pos & Bit::coord)) + std::string(1, '1' + Bit::coord - ((pos >> Bit::sideSize) & Bit::coord));
	}

	static std::string describeMove(Move move)
	{
		if (move == GameState::noMove)
		{
			return "no move";
		}

		if (move == GameState::mate)
		{
			return "mate";
		}

		if (move == GameState::stalemate)
		{
			return "stalemate";
		}

		std::string description = GameState::describePos(move.first) + ' ' + GameState::describePos(move.second);
		if ((move.first & Bit::specialMove) == Bit::promote)
		{
			description += ' ' + FigureData::promotedFiguresNames[move.second >> Bit::posSize];
		}

		return description;
	}

	static void clearGamePositions()
	{
		s_gamePositions.clear();
	}

	constexpr static Move noMove = { 0, 0 };
	constexpr static Move mate = { 1, 1 };
	constexpr static Move stalemate = { 2, 2 };

	struct HashBoard
	{
		std::uint64_t operator()(const GameState& board) const
		{
			std::uint64_t res = 0;

			for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
			{
				if (!(board.m_board[pos] & Bit::empty))
				{
					res ^= EngineData::zobristHash[static_cast<int>(pos) * ((board.m_board[pos] & Bit::type) + 1) * ((bool)(board.m_board[pos] & Bit::white) + 1)];
				}
				if ((board.m_board[pos] & Bit::type) == Bit::pawn
					&& (board.m_board[pos] & Bit::specialData))
				{
					res ^= EngineData::zobristHash[static_cast<int>(BoardData::cellsCount) * Evaluation::figuresCount * 2 + (pos & Bit::coord)];
				}
			}

			std::uint8_t castleState = 0;
			for (std::uint8_t figureColor = 0; figureColor <= 1; figureColor++)
			{
				for (std::uint8_t castleType = 0; castleType <= 1; castleType++)
				{
					if (board.hasCastleRight(figureColor, castleType))
					{
						castleState ^= 1 << ((figureColor << 1) ^ castleType);
					}
				}
			}

			res ^= EngineData::zobristHash[static_cast<int>(BoardData::cellsCount) * Evaluation::figuresCount * 2 + (1 << Bit::sideSize) + castleState];

			if (board.m_board[board.m_whiteKingPos] & Bit::specialData)
			{
				res ^= EngineData::zobristHash[static_cast<int>(BoardData::cellsCount) * Evaluation::figuresCount * 2 + (1 << Bit::sideSize) + (1 << 4)];
			}

			return res;
		}
	};

	struct EqualBoard
	{
		bool operator()(const GameState& first, const GameState& second) const
		{
			for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
			{
				// here Bit::specialData stands for both current move side and possibly en passant files
				if ((first.m_board[pos] & (Bit::allData ^ Bit::moved))
					!= (second.m_board[pos] & (Bit::allData ^ Bit::moved)))
				{
					return false;
				}
			}

			for (std::uint8_t figureColor = 0; figureColor <= 1; figureColor++)
			{
				for (std::uint8_t castleType = 0; castleType <= 1; castleType++)
				{
					if (first.hasCastleRight(figureColor, castleType) != second.hasCastleRight(figureColor, castleType))
					{
						return false;
					}
				}
			}

			return true;
		}
	};

	std::pair<GameState, bool> move(Move move) const
	{
		GameState nextState(m_board, m_consecutiveNonConfrontingMoves + 1, m_whiteKingPos, m_blackKingPos);
		nextState.m_board[nextState.m_whiteKingPos] ^= Bit::specialData;

		std::uint8_t figure = nextState.m_board[move.first & Bit::pos];
		nextState.m_board[move.first & Bit::pos] = Bit::empty;

		figure |= Bit::moved;

		if (nextState.m_board[move.second & Bit::pos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::black : Bit::white))
		{
			nextState.m_consecutiveNonConfrontingMoves = 0;
		}
		else if ((figure & Bit::type) == Bit::pawn)
		{
			nextState.m_consecutiveNonConfrontingMoves = 0;
		}

		bool speciallyMoved = false;

		if ((move.first & Bit::specialMove) == Bit::promote)
		{
			figure = ((move.second >> Bit::posSize) + FigureData::promotedFiguresShift)
				^ (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black) ^ Bit::moved;
		}
		else if (
			(figure & Bit::type) == Bit::pawn
			&& std::abs((std::int8_t)move.second - (std::int8_t)move.first) == (FigureData::doublePawnMoveLength << Bit::sideSize)
			&& (((move.second & Bit::coord)
					&& (m_board[move.second - 1] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::black : Bit::white))
					&& (m_board[move.second - 1] & Bit::type) == Bit::pawn)
				|| ((move.second & Bit::coord) != Bit::coord
					&& (m_board[move.second + 1] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::black : Bit::white))
					&& (m_board[move.second + 1] & Bit::type) == Bit::pawn)
				)
			)
		{
			figure ^= Bit::specialData;
		}
		else if ((figure & Bit::type) == Bit::pawn)
		{
			std::uint8_t sidePos = (move.first & (Bit::coord << Bit::sideSize)) ^ (move.second & Bit::coord);
			if ((nextState.m_board[sidePos] & Bit::type) == Bit::pawn
				&& (nextState.m_board[sidePos] & Bit::specialData))
			{
				nextState.m_board[sidePos] = Bit::empty;
			}
		}
		else if ((move.first & Bit::specialMove) == Bit::shortCastle)
		{
			nextState.m_board[(move.second & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleKingX] = figure;
			nextState.m_board[(move.second & (Bit::coord << Bit::sideSize)) + Bit::coord - FigureData::startRookX] = Bit::empty;
			nextState.m_board[(move.second & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleRookX] =
				Bit::rook ^ (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black) ^ Bit::moved;

			if (m_board[m_whiteKingPos] & Bit::specialData)
			{
				nextState.m_whiteKingPos = (move.second & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleKingX;
			}
			else
			{
				nextState.m_blackKingPos = (move.second & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleKingX;
			}

			speciallyMoved = true;
		}
		else if ((move.first & Bit::specialMove) == Bit::longCastle)
		{
			nextState.m_board[(move.second & (Bit::coord << Bit::sideSize)) + FigureData::longCastleKingX] = figure;
			nextState.m_board[(move.second & (Bit::coord << Bit::sideSize)) + FigureData::startRookX] = Bit::empty;
			nextState.m_board[(move.second & (Bit::coord << Bit::sideSize)) + FigureData::longCastleRookX] =
				Bit::rook ^ (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black) ^ Bit::moved;

			if (m_board[m_whiteKingPos] & Bit::specialData)
			{
				nextState.m_whiteKingPos = (move.second & (Bit::coord << Bit::sideSize)) + FigureData::longCastleKingX;
			}
			else
			{
				nextState.m_blackKingPos = (move.second & (Bit::coord << Bit::sideSize)) + FigureData::longCastleKingX;
			}

			speciallyMoved = true;
		}

		if (!speciallyMoved)
		{
			nextState.m_board[move.second & Bit::pos] = figure;

			if ((figure & Bit::type) == Bit::king)
			{
				if (m_board[m_whiteKingPos] & Bit::specialData)
				{
					nextState.m_whiteKingPos = move.second & Bit::pos;
				}
				else
				{
					nextState.m_blackKingPos = move.second & Bit::pos;
				}
			}
		}

		nextState.updatePossiblyEnPassantedPawn();

		return {
			nextState,
			!nextState.isPosUnderPressure(
				m_board[m_whiteKingPos] & Bit::specialData ? nextState.m_whiteKingPos : nextState.m_blackKingPos,
				(m_board[m_whiteKingPos] & Bit::specialData) == 0
			)
		};
	}

	std::pair<Move, int> findMove(std::uint8_t depth, int bestPrevBalance)
	{
		auto it = s_dpEvaluation.find(*this);
		if (it != s_dpEvaluation.end() && it->second.second >= depth + EngineData::quiescenceSearchDepth)
		{
			return it->second.first;
		}

		if (m_gamephase
			&& depth > EngineData::nullSearchDepthReduction
			&& !this->isPosUnderPressure(
				m_board[m_whiteKingPos] & Bit::specialData ? m_whiteKingPos : m_blackKingPos,
				(m_board[m_whiteKingPos] & Bit::specialData) == 0
			))
		{
			GameState skipMoveState(m_board, m_consecutiveNonConfrontingMoves, m_whiteKingPos, m_blackKingPos);
			skipMoveState.m_board[skipMoveState.m_whiteKingPos] ^= Bit::specialData;

			int afterSkipEvaluation = skipMoveState.findMove(
				depth - EngineData::nullSearchDepthReduction - 1,
				m_board[m_whiteKingPos] & Bit::specialData ? -FigureData::noMoveBalance : FigureData::noMoveBalance
			).second;

			if (m_board[m_whiteKingPos] & Bit::specialData ? afterSkipEvaluation >= bestPrevBalance : afterSkipEvaluation <= bestPrevBalance)
			{
				return { GameState::noMove, afterSkipEvaluation };
			}
		}

		std::vector<std::pair<GameState, Move>> allStates{};
		for (Move move : this->findAllMoves())
		{
			std::pair<GameState, bool> state = this->move(move);
			if (state.second)
			{
				state.first.evaluate();
				allStates.emplace_back(state.first, move);
			}
		}

		std::sort(allStates.begin(), allStates.end(), [&](
				std::pair<GameState, Move>& firstState,
				std::pair<GameState, Move>& secondState
			) -> bool
			{
				// history heuristic - no improvement
				// killer moves - no improvement
				// capture first - no improvement
				// how to improve from 7-8 to 4-5?
				// maybe don't generate all positions, try step by step

				return m_board[m_whiteKingPos] & Bit::specialData
					? firstState.first.m_evaluation > secondState.first.m_evaluation
					: firstState.first.m_evaluation < secondState.first.m_evaluation;
			}
		);

		std::pair<Move, int> bestMove{
			GameState::noMove,
			m_board[m_whiteKingPos] & Bit::specialData ? -FigureData::noMoveBalance : FigureData::noMoveBalance
		};
		branches++;
		for (std::pair<GameState, Move>& nextState : allStates)
		{
			counter++;
			if (m_board[m_whiteKingPos] & Bit::specialData ? bestMove.second >= bestPrevBalance : bestMove.second <= bestPrevBalance)
			{
				return bestMove;
			}

			if ((nextState.first.addGamePosition()
				|| nextState.first.m_consecutiveNonConfrontingMoves == GameData::maxConsecutiveNonConfrontingMoves)
				&& (m_board[m_whiteKingPos] & Bit::specialData ? bestMove.second < 0 : bestMove.second > 0))
			{
				bestMove = { nextState.second, 0 };
			}
			else
			{
				std::pair<Move, int> nextBestMove{ nextState.second, nextState.first.m_evaluation };

				if (depth > 1)
				{
					nextBestMove = nextState.first.findMove(depth - 1, bestMove.second);
				}
				else if (depth == 1 && (
					this->isMoveCapture(nextState.second)
					|| nextState.first.isPosUnderPressure(
						m_board[m_whiteKingPos] & Bit::specialData ? nextState.first.m_blackKingPos : nextState.first.m_whiteKingPos, m_board[m_whiteKingPos] & Bit::specialData
					)
				))
				{
					std::pair<Move, int> possibleCaptureExtendedMove = nextState.first.quiescenceSearch(
						EngineData::quiescenceSearchDepth,
						bestMove.second
					);

					if (possibleCaptureExtendedMove.first != GameState::noMove)
					{
						nextBestMove = possibleCaptureExtendedMove;
					}
				}

				if (m_board[m_whiteKingPos] & Bit::specialData ? nextBestMove.second > bestMove.second : nextBestMove.second < bestMove.second)
				{
					bestMove = { nextState.second, nextBestMove.second };
				}
			}

			nextState.first.eraseGamePosition();
		}

		if (bestMove.first == GameState::noMove)
		{
			if (this->isPosUnderPressure(m_board[m_whiteKingPos] & Bit::specialData ? m_whiteKingPos : m_blackKingPos, (m_board[m_whiteKingPos] & Bit::specialData) == 0))
			{
				bestMove = {
					GameState::mate,
					m_board[m_whiteKingPos] & Bit::specialData ? -FigureData::noMoveBalance : FigureData::noMoveBalance
				};
			}
			else
			{
				bestMove = { GameState::stalemate, 0 };
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
			if (s_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
			{
				s_dpEvaluation.clear();
				s_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
			}

			s_dpEvaluation[*this] = { bestMove, depth + EngineData::quiescenceSearchDepth };
		}

		return bestMove;
	}

	std::pair<Move, int> quiescenceSearch(std::uint8_t depth, int bestPrevBalance)
	{
		if (s_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
		{
			s_dpEvaluation.clear();
		}
		if (s_dpEvaluation.size() == 0)
		{
			s_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
		}

		auto it = s_dpEvaluation.find(*this);
		if (it != s_dpEvaluation.end() && it->second.second >= depth)
		{
			return it->second.first;
		}

		std::vector<std::pair<GameState, Move>> captureStates{};

		for (Move move : this->findAllMoves())
		{
			std::pair<GameState, bool> nextState = this->move(move);

			if (nextState.second && (
				this->isMoveCapture(move)
				|| nextState.first.isPosUnderPressure(
					m_board[m_whiteKingPos] & Bit::specialData ? nextState.first.m_blackKingPos : nextState.first.m_whiteKingPos,
					m_board[m_whiteKingPos] & Bit::specialData
				)
			))
			{
				nextState.first.evaluate();
				captureStates.emplace_back(nextState.first, move);
			}
		}

		if (captureStates.empty())
		{
			if (depth % 2 == 0)
			{
				return { GameState::noMove, m_evaluation };
			}

			std::pair<Move, int> bestMove = this->findMove(0, bestPrevBalance);

			if (s_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
			{
				s_dpEvaluation.clear();
				s_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
			}

			s_dpEvaluation[*this] = { bestMove, depth };

			return bestMove;
		}

		std::sort(captureStates.begin(), captureStates.end(), [&](
			const std::pair<GameState, Move>& firstState,
			const std::pair<GameState, Move>& secondState
			) -> bool
			{
				return m_board[m_whiteKingPos] & Bit::specialData
					? firstState.first.m_evaluation > secondState.first.m_evaluation
					: firstState.first.m_evaluation < secondState.first.m_evaluation;
			}
		);

		std::pair<Move, int> bestMove{
			GameState::noMove,
			m_board[m_whiteKingPos] & Bit::specialData ? -FigureData::noMoveBalance : FigureData::noMoveBalance
		};

		for (std::pair<GameState, Move>& captureState : captureStates)
		{
			extCounter++;
			if (m_board[m_whiteKingPos] & Bit::specialData ? bestMove.second >= bestPrevBalance : bestMove.second <= bestPrevBalance)
			{
				return bestMove;
			}

			std::pair<Move, int> nextBestMove{ captureState.second, captureState.first.m_evaluation };

			if (depth > 1)
			{
				std::pair<Move, int> possibleCaptureExtendedMove = captureState.first.quiescenceSearch(
					depth - 1,
					bestMove.second
				);

				if (possibleCaptureExtendedMove.first != GameState::noMove)
				{
					nextBestMove = possibleCaptureExtendedMove;
				}
			}

			if (m_board[m_whiteKingPos] & Bit::specialData ? nextBestMove.second > bestMove.second : nextBestMove.second < bestMove.second)
			{
				bestMove = { captureState.second, nextBestMove.second };
			}
		}

		if (bestMove.first == GameState::noMove)
		{
			if (this->isPosUnderPressure(
				m_board[m_whiteKingPos] & Bit::specialData ? m_whiteKingPos : m_blackKingPos,
				(m_board[m_whiteKingPos] & Bit::specialData) == 0
			))
			{
				bestMove = {
					GameState::mate,
					m_board[m_whiteKingPos] & Bit::specialData ? -FigureData::noMoveBalance : FigureData::noMoveBalance
				};
			}
			else
			{
				bestMove = { GameState::stalemate, 0 };
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

		if (s_dpEvaluation.size() == EngineData::maxDpEvaluationSize)
		{
			s_dpEvaluation.clear();
			s_dpEvaluation.reserve(EngineData::maxDpEvaluationSize);
		}

		s_dpEvaluation[*this] = { bestMove, depth };

		return bestMove;
	}

	bool isMoveCapture(Move move) const
	{
		// important to handle pawn captures and en passants explicitly
		return (
			m_board[move.second & Bit::pos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::black : Bit::white)
			|| (
				(m_board[move.first & Bit::pos] & Bit::type) == Bit::pawn
				&& ((m_board[m_whiteKingPos] & Bit::specialData ? move.first - move.second : move.second - move.first) & 1)
			)
		);
	}

	std::vector<Move> findAllMoves() const
	{
		std::vector<Move> allMoves{};

		for (std::uint8_t y = 0; y <= Bit::coord; y++)
		{
			for (std::uint8_t x = 0; x <= Bit::coord; x++)
			{
				if (!(m_board[(y << Bit::sideSize) + x] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black)))
				{
					continue;
				}

				std::uint8_t figureType = m_board[(y << Bit::sideSize) + x] & Bit::type;

				if (figureType == Bit::pawn)
				{
					std::uint8_t forwardY = y + (m_board[m_whiteKingPos] & Bit::specialData ? -1 : 1);
					if (m_board[(forwardY << Bit::sideSize) + x] & Bit::empty)
					{
						if (y == (m_board[m_whiteKingPos] & Bit::specialData ? FigureData::startPawnRow : Bit::coord - FigureData::startPawnRow))
						{
							std::uint8_t doubleForwardY = y + (m_board[m_whiteKingPos] & Bit::specialData ? -FigureData::doublePawnMoveLength : FigureData::doublePawnMoveLength);
							if (m_board[(doubleForwardY << Bit::sideSize) + x] == Bit::empty)
							{
								// go double forward
								allMoves.emplace_back((y << Bit::sideSize) + x, (doubleForwardY << Bit::sideSize) + x);
							}
						}
						if (forwardY == (m_board[m_whiteKingPos] & Bit::specialData ? FigureData::promotePawnRow : Bit::coord - FigureData::promotePawnRow))
						{
							// promote
							for (std::uint8_t promotedFigure = 0; promotedFigure < FigureData::promotedFiguresCount; promotedFigure++)
							{
								allMoves.emplace_back(
									((y << Bit::sideSize) + x) ^ Bit::promote,
									((forwardY << Bit::sideSize) + x) ^ (promotedFigure << Bit::posSize)
								);
							}
						}
						else
						{
							// go forward
							allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x);
						}
					}
					if (x)
					{
						std::uint8_t left = m_board[(y << Bit::sideSize) + x - 1];
						if ((m_board[(forwardY << Bit::sideSize) + x - 1] & Bit::empty)
							&& (m_board[m_whiteKingPos] & Bit::specialData ? left & Bit::black : left & Bit::white)
							&& (left & Bit::specialData))
						{
							// en passant left
							allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x - 1);
						}
					}
					if (x != Bit::coord)
					{
						std::uint8_t right = m_board[(y << Bit::sideSize) + x + 1];
						if ((m_board[(forwardY << Bit::sideSize) + x + 1] & Bit::empty)
							&& (m_board[m_whiteKingPos] & Bit::specialData ? right & Bit::black : right & Bit::white)
							&& (right & Bit::specialData))
						{
							// en passant right
							allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x + 1);
						}
					}
					if (x)
					{
						std::uint8_t leftForward = m_board[(forwardY << Bit::sideSize) + x - 1];
						if (m_board[m_whiteKingPos] & Bit::specialData ? leftForward & Bit::black : leftForward & Bit::white)
						{
							if (forwardY == (m_board[m_whiteKingPos] & Bit::specialData ? FigureData::promotePawnRow : Bit::coord - FigureData::promotePawnRow))
							{
								// take left forward and promote
								for (std::uint8_t promotedFigure = 0; promotedFigure < FigureData::promotedFiguresCount; promotedFigure++)
								{
									allMoves.emplace_back(
										((y << Bit::sideSize) + x) ^ Bit::promote,
										((forwardY << Bit::sideSize) + x - 1) ^ (promotedFigure << Bit::posSize)
									);
								}
							}
							else
							{
								// take left forward
								allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x - 1);
							}
						}
					}
					if (x != Bit::coord)
					{
						std::uint8_t rightForward = m_board[(forwardY << Bit::sideSize) + x + 1];
						if (m_board[m_whiteKingPos] & Bit::specialData ? rightForward & Bit::black : rightForward & Bit::white)
						{
							if (forwardY == (m_board[m_whiteKingPos] & Bit::specialData ? FigureData::promotePawnRow : Bit::coord - FigureData::promotePawnRow))
							{
								// take right forward and promote
								for (std::uint8_t promotedFigure = 0; promotedFigure < FigureData::promotedFiguresCount; promotedFigure++)
								{
									allMoves.emplace_back(
										((y << Bit::sideSize) + x) ^ Bit::promote,
										((forwardY << Bit::sideSize) + x + 1) ^ (promotedFigure << Bit::posSize)
									);
								}
							}
							else
							{
								// take right forward
								allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x + 1);
							}
						}
					}
				}
				if (figureType == Bit::knight)
				{
					for (std::pair<std::int8_t, std::int8_t> move : FigureData::knightMoves)
					{
						// move knight
						std::pair<std::int8_t, std::int8_t> nextPosCoords = { (std::int8_t)x + move.first, (std::int8_t)y + move.second };
						if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
							&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
						{
							std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
							if (m_board[nextPos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black))
							{
								continue;
							}
							allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
						}
					}
				}
				if (figureType == Bit::bishop || figureType == Bit::queen)
				{
					for (std::pair<std::int8_t, std::int8_t> move : FigureData::bishopMoves)
					{
						// move bishop or queen
						std::pair<std::int8_t, std::int8_t> nextPosCoords{ x, y };
						while (true)
						{
							nextPosCoords.first += move.first;
							nextPosCoords.second += move.second;
							if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
								&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
							{
								std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
								if (m_board[nextPos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black))
								{
									break;
								}
								allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
								if (m_board[nextPos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::black : Bit::white))
								{
									break;
								}
							}
							else
							{
								break;
							}
						}
					}
				}
				if (figureType == Bit::rook || figureType == Bit::queen)
				{
					for (std::pair<std::int8_t, std::int8_t> move : FigureData::rookMoves)
					{
						// move rook or queen
						std::pair<std::int8_t, std::int8_t> nextPosCoords{ x, y };
						while (true)
						{
							nextPosCoords.first += move.first;
							nextPosCoords.second += move.second;
							if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
								&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
							{
								std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
								if (m_board[nextPos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black))
								{
									break;
								}
								allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
								if (m_board[nextPos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::black : Bit::white))
								{
									break;
								}
							}
							else
							{
								break;
							}
						}
					}
				}
				if (figureType == Bit::king)
				{
					for (std::pair<std::int8_t, std::int8_t> move : FigureData::kingMoves)
					{
						// move king
						std::pair<std::int8_t, std::int8_t> nextPosCoords = { (std::int8_t)x + move.first, (std::int8_t)y + move.second };
						if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
							&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
						{
							std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
							if (m_board[nextPos] & (m_board[m_whiteKingPos] & Bit::specialData ? Bit::white : Bit::black))
							{
								continue;
							}
							allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
						}
					}

					if (this->canCastle(m_board[m_whiteKingPos] & Bit::specialData, true))
					{
						// short castle
						allMoves.emplace_back(((y << Bit::sideSize) + x) ^ Bit::shortCastle, (y << Bit::sideSize) + Bit::coord - FigureData::startRookX);
					}

					if (this->canCastle(m_board[m_whiteKingPos] & Bit::specialData, false))
					{
						// long castle
						allMoves.emplace_back(((y << Bit::sideSize) + x) ^ Bit::longCastle, (y << Bit::sideSize) + FigureData::startRookX);
					}
				}
			}
		}

		return allMoves;
	}

	bool isPosUnderPressure(std::uint8_t pos, bool isWhiteAttack) const
	{
		std::uint8_t posX = pos & Bit::coord;
		std::uint8_t posY = pos >> Bit::sideSize;

		if (posY != (isWhiteAttack ? Bit::coord : 0))
		{
			std::uint8_t forwardY = posY + (isWhiteAttack ? 1 : -1);
			if (posX)
			{
				std::uint8_t leftForward = m_board[(forwardY << Bit::sideSize) + posX - 1];
				if ((isWhiteAttack ? leftForward & Bit::white : leftForward & Bit::black)
					&& (leftForward & Bit::type) == Bit::pawn)
				{
					return true;
				}
			}
			if (posX != Bit::coord)
			{
				std::uint8_t rightForward = m_board[(forwardY << Bit::sideSize) + posX + 1];
				if ((isWhiteAttack ? rightForward & Bit::white : rightForward & Bit::black)
					&& (rightForward & Bit::type) == Bit::pawn)
				{
					return true;
				}
			}
		}

		for (std::pair<std::int8_t, std::int8_t> move : FigureData::knightMoves)
		{
			std::pair<std::int8_t, std::int8_t> nextPosCoords = { (std::int8_t)posX + move.first, (std::int8_t)posY + move.second };
			if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
				&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
			{
				std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
				if ((m_board[nextPos] & (isWhiteAttack ? Bit::white : Bit::black))
					&& (m_board[nextPos] & Bit::type) == Bit::knight)
				{
					return true;
				}
			}
		}

		for (std::pair<std::int8_t, std::int8_t> move : FigureData::bishopMoves)
		{
			std::pair<std::int8_t, std::int8_t> nextPosCoords{ posX, posY };
			while (true)
			{
				nextPosCoords.first += move.first;
				nextPosCoords.second += move.second;
				if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
					&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
				{
					std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
					if (m_board[nextPos] & (isWhiteAttack ? Bit::black : Bit::white))
					{
						break;
					}
					if (m_board[nextPos] & (isWhiteAttack ? Bit::white : Bit::black))
					{
						if ((m_board[nextPos] & Bit::type) == Bit::bishop
							|| (m_board[nextPos] & Bit::type) == Bit::queen)
						{
							return true;
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					break;
				}
			}
		}

		for (std::pair<std::int8_t, std::int8_t> move : FigureData::rookMoves)
		{
			std::pair<std::int8_t, std::int8_t> nextPosCoords{ posX, posY };
			while (true)
			{
				nextPosCoords.first += move.first;
				nextPosCoords.second += move.second;
				if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
					&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
				{
					std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
					if (m_board[nextPos] & (isWhiteAttack ? Bit::black : Bit::white))
					{
						break;
					}
					if (m_board[nextPos] & (isWhiteAttack ? Bit::white : Bit::black))
					{
						if ((m_board[nextPos] & Bit::type) == Bit::rook
							|| (m_board[nextPos] & Bit::type) == Bit::queen)
						{
							return true;
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					break;
				}
			}
		}

		for (std::pair<std::int8_t, std::int8_t> move : FigureData::kingMoves)
		{
			std::pair<std::int8_t, std::int8_t> nextPosCoords = { (std::int8_t)posX + move.first, (std::int8_t)posY + move.second };
			if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
				&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
			{
				std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
				if ((m_board[nextPos] & (isWhiteAttack ? Bit::white : Bit::black))
					&& (m_board[nextPos] & Bit::type) == Bit::king)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool hasCastleRight(bool isWhite, bool isShort) const
	{
		std::uint8_t y = isWhite ? Bit::coord : 0;
		std::uint8_t kingPos = (y << Bit::sideSize) ^ FigureData::startKingX;

		if ((m_board[kingPos] & Bit::type) != Bit::king || (m_board[kingPos] & Bit::moved))
		{
			return false;
		}

		std::uint8_t castleRookPos = (y << Bit::sideSize) ^ (isShort ? Bit::coord - FigureData::startRookX : FigureData::startRookX);
		if ((m_board[castleRookPos] & Bit::type) != Bit::rook
			|| m_board[castleRookPos] & Bit::moved)
		{
			return false;
		}

		return true;
	}

	bool canCastle(bool isWhite, bool isShort) const
	{
		std::uint8_t y = isWhite ? Bit::coord : 0;
		std::uint8_t kingPos = (y << Bit::sideSize) ^ FigureData::startKingX;

		if ((m_board[kingPos] & Bit::type) != Bit::king || (m_board[kingPos] & Bit::moved))
		{
			return false;
		}

		std::uint8_t castleRookPos = (y << Bit::sideSize) ^ (isShort ? Bit::coord - FigureData::startRookX : FigureData::startRookX);
		if ((m_board[castleRookPos] & Bit::type) != Bit::rook
			|| m_board[castleRookPos] & Bit::moved)
		{
			return false;
		}

		for (std::uint8_t checkX = FigureData::startKingX + (isShort ? 1 : -1);
			isShort ? checkX < Bit::coord - FigureData::startRookX : checkX > FigureData::startRookX;
			isShort ? checkX++ : checkX--)
		{
			if (!(m_board[(y << Bit::sideSize) + checkX] & Bit::empty))
			{
				return false;
			}
		}

		for (std::uint8_t checkX = FigureData::startKingX;
			isShort ? checkX < FigureData::shortCastleKingX : checkX > FigureData::longCastleKingX;
			isShort ? checkX++ : checkX--)
		{
			if (this->isPosUnderPressure((y << Bit::sideSize) + checkX, !isWhite))
			{
				return false;
			}
		}

		return true;
	}

	void updatePossiblyEnPassantedPawn()
	{
		std::uint8_t rowVal = (
			m_board[m_whiteKingPos] & Bit::specialData
			? FigureData::doubleMovedPawnRow
			: Bit::coord - FigureData::doubleMovedPawnRow
		) << Bit::sideSize;

		for (std::uint8_t x = 0; x <= Bit::coord; x++)
		{
			if ((m_board[rowVal + x] & Bit::type) == Bit::pawn
				&& (m_board[rowVal + x] & Bit::specialData))
			{
				m_board[rowVal + x] ^= Bit::specialData;
				break;
			}
		}
	}

	bool addGamePosition() const
	{
		return ++s_gamePositions[*this] == GameData::maxRepetitions;
	}

	void eraseGamePosition() const
	{
		auto it = s_gamePositions.find(*this);

		if (it == s_gamePositions.end())
		{
			return;
		}

		if (it->second > 1)
		{
			it->second--;
		}
		else
		{
			s_gamePositions.erase(it);
		}
	}

	void evaluate()
	{
		int midgameEvaluation = 0;
		int endgameEvaluation = 0;

		m_gamephase = 0;

		for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
		{
			if (!(m_board[pos] & Bit::empty) && (m_board[pos] & Bit::type) != Bit::king)
			{
				if (m_board[pos] & Bit::white)
				{
					midgameEvaluation += (
						Evaluation::midgamePositionTables[m_board[pos] & Bit::type][pos]
						+ Evaluation::midgameValues[m_board[pos] & Bit::type]
					);
					endgameEvaluation += (
						Evaluation::endgamePositionTables[m_board[pos] & Bit::type][pos]
						+ Evaluation::endgameValues[m_board[pos] & Bit::type]
					);
				}
				else
				{
					midgameEvaluation -= (
						Evaluation::midgamePositionTables[m_board[pos] & Bit::type][pos ^ (Bit::coord << Bit::sideSize)]
						+ Evaluation::midgameValues[m_board[pos] & Bit::type]
					);
					endgameEvaluation -= (
						Evaluation::endgamePositionTables[m_board[pos] & Bit::type][pos ^ (Bit::coord << Bit::sideSize)]
						+ Evaluation::endgameValues[m_board[pos] & Bit::type]
					);
				}

				m_gamephase += Evaluation::gamephaseIncrement[m_board[pos] & Bit::type];
			}
		}

		m_gamephase = std::min(m_gamephase, Evaluation::maxGamephase);

		m_evaluation = (
			midgameEvaluation * m_gamephase
			+ endgameEvaluation * (Evaluation::maxGamephase - m_gamephase)
		) / Evaluation::maxGamephase;
	}

	std::array<std::uint8_t, BoardData::cellsCount> m_board;
	std::uint8_t m_consecutiveNonConfrontingMoves;
	std::uint8_t m_whiteKingPos;
	std::uint8_t m_blackKingPos;
	int m_evaluation;
	std::uint8_t m_gamephase;

	inline static std::unordered_map<GameState, std::uint8_t, GameState::HashBoard, GameState::EqualBoard> s_gamePositions{};
	inline static std::unordered_map<GameState, std::pair<std::pair<Move, int>, std::uint8_t>, GameState::HashBoard, GameState::EqualBoard> s_dpEvaluation{};
};

#endif
