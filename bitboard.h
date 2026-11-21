#pragma once
// Bitboard helpers and precomputed attack tables.
// A bitboard is a 64-bit integer where bit N says "something is on square N".

#include "types.h"
#include <string>

constexpr U64 FILE_A = 0x0101010101010101ULL;
constexpr U64 FILE_H = FILE_A << 7;
constexpr U64 RANK_1 = 0xFFULL;
constexpr U64 RANK_8 = RANK_1 << 56;

inline U64 bit(int sq) { return 1ULL << sq; }
inline int popcount(U64 b) { return __builtin_popcountll(b); }
inline int lsb(U64 b) { return __builtin_ctzll(b); }          // index of lowest set bit
inline int msb(U64 b) { return 63 - __builtin_clzll(b); }     // index of highest set bit
inline int popLsb(U64& b) { int s = lsb(b); b &= b - 1; return s; }

// Precomputed tables. Call initBitboards() once at startup (initEngine() does it).
extern U64 KnightAttacks[64];
extern U64 KingAttacks[64];
extern U64 PawnAttacks[2][64];   // PawnAttacks[color][sq] = squares a pawn of that color attacks

void initBitboards();

// Sliding piece attacks given the board occupancy (blockers included as targets).
U64 bishopAttacks(int sq, U64 occ);
U64 rookAttacks(int sq, U64 occ);
inline U64 queenAttacks(int sq, U64 occ) { return bishopAttacks(sq, occ) | rookAttacks(sq, occ); }

// Debug helper: draws a bitboard as an 8x8 grid.
std::string bitboardToString(U64 b);
