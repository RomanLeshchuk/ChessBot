#include "Move.h"

#include "Data.h"
#include "GameState.h"

const Move Move::noMove(0, 0);
const Move Move::mate(1, 1);
const Move Move::stalemate(2, 2);

Move::Move(const std::uint8_t from, const std::uint8_t to) :
    m_from{ from },
    m_to{ to }
{
}

Move::Move(const std::uint8_t fromX, const std::uint8_t fromY, const std::uint8_t toX, const std::uint8_t toY) :
    m_from{ static_cast<std::uint8_t>((fromY << Bit::sideSize) ^ fromX) },
    m_to{ static_cast<std::uint8_t>((toY << Bit::sideSize) ^ toX) }
{
}

std::uint8_t Move::getFrom() const
{
    return m_from;
}

std::uint8_t Move::getTo() const
{
    return m_to;
}

std::string Move::describeMoveAsStr() const
{
    if (*this == noMove)
    {
        return "No move";
    }

    if (*this == mate)
    {
        return "Mate";
    }

    if (*this == stalemate)
    {
        return "Stalemate";
    }

    std::string description = describePosAsStr(m_from) + ' ' + describePosAsStr(m_to);
    if ((m_from & Bit::specialMove) == Bit::promote)
    {
        description += ' ' + FigureData::promotedFiguresNames[m_to >> Bit::posSize];
    }

    return description;
}

void Move::constructMoveFromStr(const std::string& description, const GameState& gameState)
{
    m_from = constructPosFromStr(description.substr(0, BoardData::posDescriptionSize));
    m_to = constructPosFromStr(description.substr(
        BoardData::posDescriptionSize + BoardData::spaceDescriptionSize,
        BoardData::posDescriptionSize
    ));

    if (!(gameState.getBoard().get(m_from) & (gameState.isWhiteMove() ? Bit::white : Bit::black)))
    {
        m_from = noMove.getFrom();
        m_to = noMove.getTo();
        return;
    }

    if (description.size() != BoardData::moveDescriptionSize)
    {
        m_from ^= Bit::promote;
        std::uint8_t promotedFigureType = std::find(
            FigureData::promotedFiguresNames.begin(),
            FigureData::promotedFiguresNames.end(),
            description.substr(BoardData::moveDescriptionSize + BoardData::spaceDescriptionSize)
        ) - FigureData::promotedFiguresNames.begin();
        m_to ^= (promotedFigureType << Bit::posSize);
    }
    else if ((gameState.getBoard().get(m_from) & Bit::type) == Bit::king
        && !(gameState.getBoard().get(m_from) & Bit::moved)
        && (gameState.getBoard().get(m_to) & Bit::type) == Bit::rook
        && !(gameState.getBoard().get(m_to) & Bit::moved))
    {
        m_from ^= ((m_to & Bit::coord) == FigureData::startRookX ? Bit::longCastle : Bit::shortCastle);
    }

    std::vector<Move> allMoves = gameState.findAllMoves();

    if (std::find(allMoves.begin(), allMoves.end(), *this) == allMoves.end() || !gameState.move(*this).second)
    {
        m_from = noMove.getFrom();
        m_to = noMove.getTo();
    }
}

std::uint8_t Move::constructPosFromStr(const std::string& description)
{
    return (
        (static_cast<std::uint8_t>(Bit::coord - (description[1] - '1')) << Bit::sideSize)
        + static_cast<std::uint8_t>(description[0] - 'a')
    );
}

std::string Move::describePosAsStr(const std::uint8_t pos)
{
    return (
        std::string(1, static_cast<char>('a' + (pos & Bit::coord)))
        + std::string(1, static_cast<char>('1' + Bit::coord - ((pos >> Bit::sideSize) & Bit::coord)))
    );
}
