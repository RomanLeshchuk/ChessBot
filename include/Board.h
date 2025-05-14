#ifndef BOARD_H
#define BOARD_H

#include "Data.h"

class Board
{
public:
    Board() = default;

    Board(const Board&) = default;
    Board(Board&&) = default;

    Board& operator=(const Board&) = default;
    Board& operator=(Board&&) = default;

    ~Board() = default;

    [[nodiscard]] std::uint8_t get(std::uint8_t x, std::uint8_t y) const;
    [[nodiscard]] std::uint8_t get(std::uint8_t pos) const;

    void set(std::uint8_t x, std::uint8_t y, std::uint8_t figure);
    void set(std::uint8_t pos, std::uint8_t figure);

	[[nodiscard]] bool hasCastleRight(bool isWhite, bool isShort) const;

private:
    std::array<std::uint8_t, BoardData::cellsCount> m_board{
        Bit::rook ^ Bit::black, Bit::knight ^ Bit::black, Bit::bishop ^ Bit::black, Bit::queen ^ Bit::black, Bit::king ^ Bit::black, Bit::bishop ^ Bit::black, Bit::knight ^ Bit::black, Bit::rook ^ Bit::black,
        Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black, Bit::pawn ^ Bit::black,
        Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty,
        Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty,
        Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty,
        Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty, Bit::empty,
        Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white, Bit::pawn ^ Bit::white,
        Bit::rook ^ Bit::white, Bit::knight ^ Bit::white, Bit::bishop ^ Bit::white, Bit::queen ^ Bit::white, Bit::king ^ Bit::white ^ Bit::specialData, Bit::bishop ^ Bit::white, Bit::knight ^ Bit::white, Bit::rook ^ Bit::white,
    };
};

struct HashBoard
{
	std::uint64_t operator()(const Board& board) const;
};

struct EqualBoard
{
	bool operator()(const Board& first, const Board& second) const;
};

#endif
