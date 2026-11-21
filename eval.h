#pragma once
// Static evaluation: material + piece-square tables (tapered between middlegame
// and endgame) + a small bishop-pair bonus.

#include "position.h"

// Score in centipawns (100 = one pawn) from the point of view of the SIDE TO MOVE.
int evaluate(const Position& pos);
