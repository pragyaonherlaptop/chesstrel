#pragma once
// Move generation.

#include "position.h"
#include "types.h"

struct MoveList {
    Move moves[256];
    int count = 0;

    void add(Move m) { moves[count++] = m; }
    int size() const { return count; }
    Move* begin() { return moves; }
    Move* end() { return moves + count; }
    const Move* begin() const { return moves; }
    const Move* end() const { return moves + count; }
};

// Pseudo-legal: obeys piece movement rules but may leave the own king in check.
// capturesOnly = captures, en passant and promotions only (used by quiescence search).
void generatePseudoLegal(const Position& pos, MoveList& list, bool capturesOnly = false);

// Fully legal moves (pseudo-legal moves filtered by "is my king safe afterwards?").
void generateLegal(Position& pos, MoveList& list, bool capturesOnly = false);
