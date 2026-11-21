#pragma once
// Search: negamax with alpha-beta pruning, iterative deepening, quiescence search,
// check extensions, and move ordering (previous best move, MVV-LVA, killers, history).

#include "position.h"
#include "types.h"

struct SearchLimits {
    int depth = 64;                 // maximum depth in plies
    long long movetime = 0;         // fixed time for this move in ms (0 = not set)
    long long time[2] = {0, 0};     // remaining clock time per color in ms
    long long inc[2] = {0, 0};      // increment per color in ms
    int movestogo = 0;              // moves until next time control (0 = unknown)
    bool infinite = false;          // search until told to stop
};

// Runs a search on `pos` (it is modified temporarily but restored) and returns the
// best move found. Call searchReset() before starting, searchStop() from another
// thread to abort early.
Move search(Position& pos, const SearchLimits& limits);

void searchReset();            // clear the stop flag
void searchStop();             // ask a running search to finish as soon as possible
bool searchIsStopped();
void searchClear();            // forget killers / history (new game)
