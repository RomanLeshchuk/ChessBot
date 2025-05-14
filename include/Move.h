#ifndef MOVE_H
#define MOVE_H

#include <cstdint>
#include <string>
#include <algorithm>

class GameState;

class Move
{
public:
    Move() = default;
    Move(std::uint8_t from, std::uint8_t to);
    Move(std::uint8_t fromX, std::uint8_t fromY, std::uint8_t toX, std::uint8_t toY);

    [[nodiscard]] std::uint8_t getFrom() const;
    [[nodiscard]] std::uint8_t getTo() const;

    bool operator==(const Move& rhs) const = default;

    [[nodiscard]] std::string describeMoveAsStr() const;

    void constructMoveFromStr(const std::string& description, const GameState& gameState);

    const static Move noMove;
    const static Move mate;
    const static Move stalemate;

private:
    static std::uint8_t constructPosFromStr(const std::string& description);

    static std::string describePosAsStr(std::uint8_t pos);

    std::uint8_t m_from = 0;
    std::uint8_t m_to = 0;
};

#endif
