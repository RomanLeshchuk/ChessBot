#include "GameState.h"
#include "Move.h"

GameState::GameState()
{
    for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
    {
        if ((m_board.get(pos) & Bit::type) == Bit::king)
        {
            if (m_board.get(pos) & Bit::white)
            {
                m_whiteKingPos = pos;
            }
            else
            {
                m_blackKingPos = pos;
            }
        }
    }
}

bool GameState::isMoveCapture(const Move& move) const
{
	// important to handle pawn captures and en passants explicitly
	return (
		m_board.get(move.getTo() & Bit::pos) & (isWhiteMove() ? Bit::black : Bit::white)
		|| (
			(m_board.get(move.getFrom() & Bit::pos) & Bit::type) == Bit::pawn
			&& ((isWhiteMove() ? move.getFrom() - move.getTo() : move.getTo() - move.getFrom()) & 1)
		)
	);
}

bool GameState::isPosUnderPressure(const std::uint8_t pos, const bool isWhiteAttack) const
{
	std::uint8_t posX = pos & Bit::coord;
	std::uint8_t posY = pos >> Bit::sideSize;

	if (posY != (isWhiteAttack ? Bit::coord : 0))
	{
		std::uint8_t forwardY = posY + (isWhiteAttack ? 1 : -1);
		if (posX)
		{
			std::uint8_t leftForward = m_board.get(posX - 1, forwardY);
			if ((isWhiteAttack ? leftForward & Bit::white : leftForward & Bit::black)
				&& (leftForward & Bit::type) == Bit::pawn)
			{
				return true;
			}
		}
		if (posX != Bit::coord)
		{
			std::uint8_t rightForward = m_board.get(posX + 1, forwardY);
			if ((isWhiteAttack ? rightForward & Bit::white : rightForward & Bit::black)
				&& (rightForward & Bit::type) == Bit::pawn)
			{
				return true;
			}
		}
	}

	for (std::pair<std::int8_t, std::int8_t> move : FigureData::knightMoves)
	{
		std::pair<std::int8_t, std::int8_t> nextPosCoords{
			static_cast<std::int8_t>(posX) + move.first,
			static_cast<std::int8_t>(posY) + move.second
		};
		if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
			&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
		{
			std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
			if ((m_board.get(nextPos) & (isWhiteAttack ? Bit::white : Bit::black))
				&& (m_board.get(nextPos) & Bit::type) == Bit::knight)
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
				if (m_board.get(nextPos) & (isWhiteAttack ? Bit::black : Bit::white))
				{
					break;
				}
				if (m_board.get(nextPos) & (isWhiteAttack ? Bit::white : Bit::black))
				{
					if ((m_board.get(nextPos) & Bit::type) == Bit::bishop
						|| (m_board.get(nextPos) & Bit::type) == Bit::queen)
					{
						return true;
					}
					break;
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
				if (m_board.get(nextPos) & (isWhiteAttack ? Bit::black : Bit::white))
				{
					break;
				}
				if (m_board.get(nextPos) & (isWhiteAttack ? Bit::white : Bit::black))
				{
					if ((m_board.get(nextPos) & Bit::type) == Bit::rook
						|| (m_board.get(nextPos) & Bit::type) == Bit::queen)
					{
						return true;
					}
					break;
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
		std::pair<std::int8_t, std::int8_t> nextPosCoords{
			static_cast<std::int8_t>(posX) + move.first,
			static_cast<std::int8_t>(posY) + move.second
		};
		if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
			&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
		{
			std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
			if ((m_board.get(nextPos) & (isWhiteAttack ? Bit::white : Bit::black))
				&& (m_board.get(nextPos) & Bit::type) == Bit::king)
			{
				return true;
			}
		}
	}

	return false;
}

bool GameState::canCastle(const bool isWhite, const bool isShort) const
{
    std::uint8_t y = isWhite ? Bit::coord : 0;
    std::uint8_t kingPos = (y << Bit::sideSize) ^ FigureData::startKingX;

    if ((m_board.get(kingPos) & Bit::type) != Bit::king || (m_board.get(kingPos) & Bit::moved))
    {
    	return false;
    }

    std::uint8_t castleRookPos = (y << Bit::sideSize) ^ (isShort ? Bit::coord - FigureData::startRookX : FigureData::startRookX);
    if ((m_board.get(castleRookPos) & Bit::type) != Bit::rook
		|| m_board.get(castleRookPos) & Bit::moved)
    {
    	return false;
    }

    for (std::uint8_t checkX = FigureData::startKingX + (isShort ? 1 : -1);
		isShort ? checkX < Bit::coord - FigureData::startRookX : checkX > FigureData::startRookX;
		isShort ? checkX++ : checkX--)
    {
    	if (!(m_board.get(checkX, y) & Bit::empty))
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

std::vector<Move> GameState::findAllMoves() const
{
	std::vector<Move> allMoves{};

	for (std::uint8_t y = 0; y <= Bit::coord; y++)
	{
		for (std::uint8_t x = 0; x <= Bit::coord; x++)
		{
			if (!(m_board.get(x, y) & (isWhiteMove() ? Bit::white : Bit::black)))
			{
				continue;
			}

			std::uint8_t figureType = m_board.get(x, y) & Bit::type;

			if (figureType == Bit::pawn)
			{
				std::uint8_t forwardY = y + (isWhiteMove() ? -1 : 1);
				if (m_board.get(x, forwardY) & Bit::empty)
				{
					if (y == (isWhiteMove() ? FigureData::startPawnRow : Bit::coord - FigureData::startPawnRow))
					{
						std::uint8_t doubleForwardY = y + (isWhiteMove() ? -FigureData::doublePawnMoveLength : FigureData::doublePawnMoveLength);
						if (m_board.get(x, doubleForwardY) == Bit::empty)
						{
							// go double forward
							allMoves.emplace_back(x, y, x, doubleForwardY);
						}
					}
					if (forwardY == (isWhiteMove() ? FigureData::promotePawnRow : Bit::coord - FigureData::promotePawnRow))
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
						allMoves.emplace_back(x, y, x, forwardY);
					}
				}
				if (x)
				{
					std::uint8_t left = m_board.get(x - 1, y);
					if ((m_board.get(x - 1, forwardY) & Bit::empty)
						&& (isWhiteMove() ? left & Bit::black : left & Bit::white)
						&& (left & Bit::specialData))
					{
						// en passant left
						allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x - 1);
					}
				}
				if (x != Bit::coord)
				{
					std::uint8_t right = m_board.get(x + 1, y);
					if ((m_board.get(x + 1, forwardY) & Bit::empty)
						&& (isWhiteMove() ? right & Bit::black : right & Bit::white)
						&& (right & Bit::specialData))
					{
						// en passant right
						allMoves.emplace_back((y << Bit::sideSize) + x, (forwardY << Bit::sideSize) + x + 1);
					}
				}
				if (x)
				{
					std::uint8_t leftForward = m_board.get(x - 1, forwardY);
					if (isWhiteMove() ? leftForward & Bit::black : leftForward & Bit::white)
					{
						if (forwardY == (isWhiteMove() ? FigureData::promotePawnRow : Bit::coord - FigureData::promotePawnRow))
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
							allMoves.emplace_back(x, y, x - 1, forwardY);
						}
					}
				}
				if (x != Bit::coord)
				{
					std::uint8_t rightForward = m_board.get(x + 1, forwardY);
					if (isWhiteMove() ? rightForward & Bit::black : rightForward & Bit::white)
					{
						if (forwardY == (isWhiteMove() ? FigureData::promotePawnRow : Bit::coord - FigureData::promotePawnRow))
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
							allMoves.emplace_back(x, y, x + 1, forwardY);
						}
					}
				}
			}
			if (figureType == Bit::knight)
			{
				for (std::pair<std::int8_t, std::int8_t> move : FigureData::knightMoves)
				{
					// move knight
					std::pair<std::int8_t, std::int8_t> nextPosCoords{
						static_cast<std::int8_t>(x) + move.first,
						static_cast<std::int8_t>(y) + move.second
					};
					if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
						&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
					{
						std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
						if (m_board.get(nextPos) & (isWhiteMove() ? Bit::white : Bit::black))
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
							if (m_board.get(nextPos) & (isWhiteMove() ? Bit::white : Bit::black))
							{
								break;
							}
							allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
							if (m_board.get(nextPos) & (isWhiteMove() ? Bit::black : Bit::white))
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
							if (m_board.get(nextPos) & (isWhiteMove() ? Bit::white : Bit::black))
							{
								break;
							}
							allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
							if (m_board.get(nextPos) & (isWhiteMove() ? Bit::black : Bit::white))
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
					std::pair<std::int8_t, std::int8_t> nextPosCoords{
						static_cast<std::int8_t>(x) + move.first,
						static_cast<std::int8_t>(y) + move.second
					};
					if (0 <= nextPosCoords.first && nextPosCoords.first <= Bit::coord
						&& 0 <= nextPosCoords.second && nextPosCoords.second <= Bit::coord)
					{
						std::uint8_t nextPos = (nextPosCoords.second << Bit::sideSize) + nextPosCoords.first;
						if (m_board.get(nextPos) & (isWhiteMove() ? Bit::white : Bit::black))
						{
							continue;
						}
						allMoves.emplace_back((y << Bit::sideSize) + x, nextPos);
					}
				}

				if (this->canCastle(isWhiteMove(), true))
				{
					// short castle
					allMoves.emplace_back(
						((y << Bit::sideSize) + x) ^ Bit::shortCastle,
						(y << Bit::sideSize) + Bit::coord - FigureData::startRookX
					);
				}

				if (this->canCastle(isWhiteMove(), false))
				{
					// long castle
					allMoves.emplace_back(
						((y << Bit::sideSize) + x) ^ Bit::longCastle,
						(y << Bit::sideSize) + FigureData::startRookX
					);
				}
			}
		}
	}

	return allMoves;
}

std::pair<GameState, bool> GameState::move(const Move& move) const
{
	GameState nextState = *this;
	nextState.m_board.set(nextState.m_whiteKingPos, nextState.m_board.get(nextState.m_whiteKingPos) ^ Bit::specialData);

	std::uint8_t figure = nextState.m_board.get(move.getFrom() & Bit::pos);
	nextState.m_board.set(move.getFrom() & Bit::pos, Bit::empty);

	figure |= Bit::moved;

	if ((nextState.m_board.get(move.getTo() & Bit::pos) & (isWhiteMove() ? Bit::black : Bit::white))
		|| (figure & Bit::type) == Bit::pawn)
	{
		nextState.m_consecutiveNonConfrontingMoves = 0;
	}
	else
	{
		nextState.m_consecutiveNonConfrontingMoves++;
	}

	bool speciallyMoved = false;

	if ((move.getFrom() & Bit::specialMove) == Bit::promote)
	{
		figure = ((move.getTo() >> Bit::posSize) + FigureData::promotedFiguresShift)
			^ (isWhiteMove() ? Bit::white : Bit::black) ^ Bit::moved;
	}
	else if (
		(figure & Bit::type) == Bit::pawn
		&& std::abs(static_cast<std::int8_t>(move.getTo()) - static_cast<std::int8_t>(move.getFrom())) == (FigureData::doublePawnMoveLength << Bit::sideSize)
		&& (((move.getTo() & Bit::coord)
				&& (m_board.get(move.getTo() - 1) & (isWhiteMove() ? Bit::black : Bit::white))
				&& (m_board.get(move.getTo() - 1) & Bit::type) == Bit::pawn)
			|| ((move.getTo() & Bit::coord) != Bit::coord
				&& (m_board.get(move.getTo() + 1) & (isWhiteMove() ? Bit::black : Bit::white))
				&& (m_board.get(move.getTo() + 1) & Bit::type) == Bit::pawn)
			)
		)
	{
		figure ^= Bit::specialData;
	}
	else if ((figure & Bit::type) == Bit::pawn)
	{
		std::uint8_t sidePos = (move.getFrom() & (Bit::coord << Bit::sideSize)) ^ (move.getTo() & Bit::coord);
		if ((nextState.m_board.get(sidePos) & Bit::type) == Bit::pawn
			&& (nextState.m_board.get(sidePos) & Bit::specialData))
		{
			nextState.m_board.set(sidePos, Bit::empty);
		}
	}
	else if ((move.getFrom() & Bit::specialMove) == Bit::shortCastle)
	{
		nextState.m_board.set((move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleKingX, figure);
		nextState.m_board.set((move.getTo() & (Bit::coord << Bit::sideSize)) + Bit::coord - FigureData::startRookX, Bit::empty);
		nextState.m_board.set(
			(move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleRookX,
			Bit::rook ^ (isWhiteMove() ? Bit::white : Bit::black) ^ Bit::moved
		);

		if (isWhiteMove())
		{
			nextState.m_whiteKingPos = (move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleKingX;
		}
		else
		{
			nextState.m_blackKingPos = (move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::shortCastleKingX;
		}

		speciallyMoved = true;
	}
	else if ((move.getFrom() & Bit::specialMove) == Bit::longCastle)
	{
		nextState.m_board.set((move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::longCastleKingX, figure);
		nextState.m_board.set((move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::startRookX, Bit::empty);
		nextState.m_board.set(
			(move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::longCastleRookX,
			Bit::rook ^ (isWhiteMove() ? Bit::white : Bit::black) ^ Bit::moved
		);

		if (isWhiteMove())
		{
			nextState.m_whiteKingPos = (move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::longCastleKingX;
		}
		else
		{
			nextState.m_blackKingPos = (move.getTo() & (Bit::coord << Bit::sideSize)) + FigureData::longCastleKingX;
		}

		speciallyMoved = true;
	}

	if (!speciallyMoved)
	{
		nextState.m_board.set(move.getTo() & Bit::pos, figure);

		if ((figure & Bit::type) == Bit::king)
		{
			if (isWhiteMove())
			{
				nextState.m_whiteKingPos = move.getTo() & Bit::pos;
			}
			else
			{
				nextState.m_blackKingPos = move.getTo() & Bit::pos;
			}
		}
	}

	nextState.updatePossiblyEnPassantedPawn();
	nextState.evaluate();

	return {
		nextState,
		!nextState.isPosUnderPressure(
			isWhiteMove() ? nextState.m_whiteKingPos : nextState.m_blackKingPos,
			(isWhiteMove()) == 0
		)
	};
}

GameState GameState::skipMove() const
{
	GameState skipState = *this;
	skipState.m_board.set(skipState.m_whiteKingPos, skipState.m_board.get(skipState.m_whiteKingPos) ^ Bit::specialData);
	return skipState;
}

void GameState::updatePossiblyEnPassantedPawn()
{
	std::uint8_t y = isWhiteMove()
		? FigureData::doubleMovedPawnRow
		: Bit::coord - FigureData::doubleMovedPawnRow;

	for (std::uint8_t x = 0; x <= Bit::coord; x++)
	{
		if ((m_board.get(x, y) & Bit::type) == Bit::pawn
			&& (m_board.get(x, y) & Bit::specialData))
		{
			m_board.set(x, y, m_board.get(x, y) ^ Bit::specialData);
			break;
		}
	}
}

const Board& GameState::getBoard() const
{
	return m_board;
}

bool GameState::isFiftyMovesRule() const
{
	return m_consecutiveNonConfrontingMoves == GameData::maxConsecutiveNonConfrontingMoves;
}

bool GameState::isWhiteMove() const
{
	return m_board.get(m_whiteKingPos) & Bit::specialData;
}

std::uint8_t GameState::getWhiteKingPos() const
{
	return m_whiteKingPos;
}

std::uint8_t GameState::getBlackKingPos() const
{
	return m_blackKingPos;
}

void GameState::evaluate()
{
	int midgameEvaluation = 0;
	int endgameEvaluation = 0;

	m_gamephase = 0;

	std::array<bool, 1 << Bit::sideSize> whitePawnOnCol{};
	std::array<bool, 1 << Bit::sideSize> blackPawnOnCol{};

	for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
	{
		if (!(m_board.get(pos) & Bit::empty) && (m_board.get(pos) & Bit::type) != Bit::king)
		{
			if ((m_board.get(pos) & Bit::type) == Bit::pawn)
			{
				if ((m_board.get(pos) & Bit::white ? whitePawnOnCol : blackPawnOnCol)[pos & Bit::coord])
				{
					midgameEvaluation += Evaluation::midgameValues[Bit::pawn] / 2 * (m_board.get(pos) & Bit::white ? -1 : 1);
					endgameEvaluation += Evaluation::midgameValues[Bit::pawn] / 2 * (m_board.get(pos) & Bit::white ? -1 : 1);
				}
				else
				{
					(m_board.get(pos) & Bit::white ? whitePawnOnCol : blackPawnOnCol)[pos & Bit::coord] = true;
				}
			}
			midgameEvaluation += (
				Evaluation::midgamePositionTables[m_board.get(pos) & Bit::type]
				[m_board.get(pos) & Bit::white ? pos : pos ^ (Bit::coord << Bit::sideSize)]
				+ Evaluation::midgameValues[m_board.get(pos) & Bit::type]
			) * (m_board.get(pos) & Bit::white ? 1 : -1);
			endgameEvaluation += (
				Evaluation::endgamePositionTables[m_board.get(pos) & Bit::type]
				[m_board.get(pos) & Bit::white ? pos : pos ^ (Bit::coord << Bit::sideSize)]
				+ Evaluation::endgameValues[m_board.get(pos) & Bit::type]
			) * (m_board.get(pos) & Bit::white ? 1 : -1);

			m_gamephase += Evaluation::gamephaseIncrement[m_board.get(pos) & Bit::type];
		}
	}

	m_gamephase = std::min(m_gamephase, Evaluation::maxGamephase);

	m_evaluation = (
		midgameEvaluation * m_gamephase
		+ endgameEvaluation * (Evaluation::maxGamephase - m_gamephase)
	) / Evaluation::maxGamephase;
}

std::uint8_t GameState::getGamephase() const
{
	return m_gamephase;
}

int GameState::getEvaluation() const
{
	return m_evaluation;
}
