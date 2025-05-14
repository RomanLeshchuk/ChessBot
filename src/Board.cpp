#include "Board.h"

std::uint8_t Board::get(const std::uint8_t x, const std::uint8_t y) const
{
    return m_board[(y << Bit::sideSize) ^ x];
}

std::uint8_t Board::get(const std::uint8_t pos) const
{
    return m_board[pos];
}

void Board::set(const std::uint8_t x, const std::uint8_t y, const std::uint8_t figure)
{
    m_board[(y << Bit::sideSize) ^ x] = figure;
}

void Board::set(const std::uint8_t pos, const std::uint8_t figure)
{
    m_board[pos] = figure;
}

bool Board::hasCastleRight(bool isWhite, bool isShort) const
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

std::uint64_t HashBoard::operator()(const Board& board) const
{
    std::uint64_t res = 0;

    for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
    {
        if (!(board.get(pos) & Bit::empty))
        {
            res ^= EngineData::zobristHash[
                static_cast<int>(pos)
                * ((board.get(pos) & Bit::type) + 1)
                * (static_cast<bool>(board.get(pos) & Bit::white) + 1)
            ];
        }
        if ((board.get(pos) & Bit::type) == Bit::pawn
            && (board.get(pos) & Bit::specialData))
        {
            res ^= EngineData::zobristHash[
                static_cast<int>(BoardData::cellsCount)
                * Evaluation::figuresCount * 2
                + (pos & Bit::coord)
            ];
        }
        if ((board.get(pos) & Bit::type) == Bit::king
            && board.get(pos) & Bit::white
            && board.get(pos) & Bit::specialData)
        {
            res ^= EngineData::zobristHash[
                static_cast<int>(BoardData::cellsCount)
                * Evaluation::figuresCount * 2
                + (1 << Bit::sideSize)
                + (1 << 4)
            ];
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

    res ^= EngineData::zobristHash[
        static_cast<int>(BoardData::cellsCount)
        * Evaluation::figuresCount * 2
        + (1 << Bit::sideSize)
        + castleState
    ];

    return res;
}

bool EqualBoard::operator()(const Board& first, const Board& second) const
{
    for (std::uint8_t pos = 0; pos < BoardData::cellsCount; pos++)
    {
        // here Bit::specialData stands for both current move side and possibly en passant files
        if ((first.get(pos) & (Bit::allData ^ Bit::moved))
            != (second.get(pos) & (Bit::allData ^ Bit::moved)))
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
