#pragma once
// Perft ("performance test"): count all leaf positions reachable in N plies.
// The numbers are known for standard test positions, so any bug in move
// generation or make/unmake shows up as a wrong count.

#include "position.h"
#include "types.h"

U64 perft(Position& pos, int depth);

// Like perft, but prints the count under every root move (great for hunting bugs
// by comparing with another engine's output).
void perftDivide(Position& pos, int depth);

// Runs the standard perft suite plus hash/make-unmake consistency checks.
// Returns true if everything matches.
bool runPerftSuite();
