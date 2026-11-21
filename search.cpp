#include "search.h"

#include "eval.h"
#include "movegen.h"
#include "uci.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

constexpr int MAX_PLY = 96;
constexpr int INF = 32000;
constexpr int MATE = 30000;
constexpr int MATE_BOUND = MATE - MAX_PLY;   // scores beyond this are "mate in N"

std::atomic<bool> g_stop(false);
U64 g_nodes = 0;
std::chrono::steady_clock::time_point g_start;
long long g_hardLimitMs = 0;   // 0 = no time limit

Move g_killers[MAX_PLY][2];
int g_history[2][64][64];
Move g_pv[MAX_PLY][MAX_PLY];
int g_pvLen[MAX_PLY];
Move g_rootBest;

long long elapsedMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - g_start).count();
}

void checkTime() {
    if (g_hardLimitMs > 0 && elapsedMs() >= g_hardLimitMs) g_stop = true;
}

// Move ordering: try the most promising moves first so alpha-beta cuts more.

int scoreMove(const Position& pos, Move m, int ply) {
    if (ply == 0 && m == g_rootBest) return 1000000;

    if (m.isCapture()) {
        // MVV-LVA: most valuable victim, least valuable attacker.
        const int victim = (m.flags() == EP_CAPTURE) ? PAWN : typeOf(pos.pieceOn(m.to()));
        const int attacker = typeOf(pos.pieceOn(m.from()));
        return 20000 + victim * 16 - attacker + (m.isPromotion() ? 500 : 0);
    }
    if (m.isPromotion()) return 19000 + m.promoType();
    if (m == g_killers[ply][0]) return 18000;
    if (m == g_killers[ply][1]) return 17999;
    return g_history[pos.sideToMove()][m.from()][m.to()];
}

inline void pickBest(Move* moves, int* scores, int count, int start) {
    int best = start;
    for (int i = start + 1; i < count; ++i)
        if (scores[i] > scores[best]) best = i;
    std::swap(moves[start], moves[best]);
    std::swap(scores[start], scores[best]);
}

// Quiescence search: at the horizon keep resolving captures so we never
// evaluate a position in the middle of an exchange.

int quiescence(Position& pos, int alpha, int beta, int ply) {
    ++g_nodes;
    if ((g_nodes & 2047) == 0) checkTime();
    if (g_stop) return 0;
    if (ply >= MAX_PLY - 1) return evaluate(pos);

    const bool inCheck = pos.inCheck();
    int bestScore;
    if (inCheck) {
        bestScore = -INF;   // no "stand pat" while in check: we must escape
    } else {
        bestScore = evaluate(pos);
        if (bestScore >= beta) return bestScore;
        if (bestScore > alpha) alpha = bestScore;
    }

    MoveList list;
    generateLegal(pos, list, !inCheck);
    if (inCheck && list.size() == 0) return -MATE + ply;

    int scores[256];
    for (int i = 0; i < list.size(); ++i) scores[i] = scoreMove(pos, list.moves[i], ply);

    for (int i = 0; i < list.size(); ++i) {
        pickBest(list.moves, scores, list.size(), i);
        pos.makeMove(list.moves[i]);
        const int score = -quiescence(pos, -beta, -alpha, ply + 1);
        pos.unmakeMove();
        if (g_stop) return 0;

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
                if (alpha >= beta) break;
            }
        }
    }
    return bestScore;
}

// Main search (negamax + alpha-beta)

int negamax(Position& pos, int depth, int alpha, int beta, int ply) {
    g_pvLen[ply] = 0;

    if (ply > 0 && (pos.halfmoveClock() >= 100 || pos.isRepetition())) return 0;
    if (ply >= MAX_PLY - 1) return evaluate(pos);

    const bool inCheck = pos.inCheck();
    if (inCheck) ++depth;   // check extension
    if (depth <= 0) return quiescence(pos, alpha, beta, ply);

    ++g_nodes;
    if ((g_nodes & 2047) == 0) checkTime();
    if (g_stop) return 0;

    // Mate distance pruning: we can't do better than mating right now.
    alpha = std::max(alpha, -MATE + ply);
    beta = std::min(beta, MATE - ply - 1);
    if (alpha >= beta) return alpha;

    MoveList list;
    generateLegal(pos, list);
    if (list.size() == 0) return inCheck ? -MATE + ply : 0;   // checkmate or stalemate

    int scores[256];
    for (int i = 0; i < list.size(); ++i) scores[i] = scoreMove(pos, list.moves[i], ply);

    const int us = pos.sideToMove();
    int bestScore = -INF;

    for (int i = 0; i < list.size(); ++i) {
        pickBest(list.moves, scores, list.size(), i);
        const Move m = list.moves[i];

        pos.makeMove(m);
        const int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);
        pos.unmakeMove();
        if (g_stop) return 0;

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;

                // Update the principal variation.
                g_pv[ply][0] = m;
                for (int j = 0; j < g_pvLen[ply + 1]; ++j) g_pv[ply][j + 1] = g_pv[ply + 1][j];
                g_pvLen[ply] = g_pvLen[ply + 1] + 1;

                if (alpha >= beta) {
                    // Beta cutoff: remember quiet moves that refute the opponent.
                    if (!m.isCapture()) {
                        if (g_killers[ply][0] != m) {
                            g_killers[ply][1] = g_killers[ply][0];
                            g_killers[ply][0] = m;
                        }
                        int& h = g_history[us][m.from()][m.to()];
                        h = std::min(h + depth * depth, 6500);
                    }
                    break;
                }
            }
        }
    }
    return bestScore;
}

std::string scoreToUci(int score) {
    if (std::abs(score) >= MATE_BOUND) {
        const int movesToMate = (MATE - std::abs(score) + 1) / 2;
        return "mate " + std::to_string(score > 0 ? movesToMate : -movesToMate);
    }
    return "cp " + std::to_string(score);
}

}  // namespace

void searchReset() { g_stop = false; }
void searchStop() { g_stop = true; }
bool searchIsStopped() { return g_stop; }

void searchClear() {
    std::memset(g_killers, 0, sizeof(g_killers));
    std::memset(g_history, 0, sizeof(g_history));
}

Move search(Position& pos, const SearchLimits& lim) {
    g_start = std::chrono::steady_clock::now();
    g_nodes = 0;
    g_rootBest = Move();
    std::memset(g_killers, 0, sizeof(g_killers));
    // Keep history between searches but age it so old information fades.
    for (auto& side : g_history)
        for (auto& from : side)
            for (int& v : from) v /= 2;

    const int us = pos.sideToMove();

    // Work out how long we may think.
    long long softMs = 0;
    g_hardLimitMs = 0;
    if (!lim.infinite) {
        if (lim.movetime > 0) {
            softMs = g_hardLimitMs = std::max(1LL, lim.movetime - 20);
        } else if (lim.time[us] > 0) {
            const long long t = lim.time[us];
            const long long inc = lim.inc[us];
            const int mtg = lim.movestogo > 0 ? std::min(lim.movestogo, 50) : 30;
            softMs = t / mtg + inc * 3 / 4;
            g_hardLimitMs = std::min(softMs * 4, t * 8 / 10);
            g_hardLimitMs = std::max(1LL, g_hardLimitMs - 20);
            softMs = std::min(softMs, g_hardLimitMs);
        }
    }

    MoveList root;
    generateLegal(pos, root);
    if (root.size() == 0) return Move();

    Move best = root.moves[0];
    if (root.size() == 1 && softMs > 0) return best;   // only one legal move: play it at once

    const int maxDepth = std::min(lim.depth, MAX_PLY - 2);
    for (int depth = 1; depth <= maxDepth; ++depth) {
        const int score = negamax(pos, depth, -INF, INF, 0);

        // g_pv[0][0] is only ever set by a root move that was searched completely, and the
        // previous best move is searched first, so even an interrupted iteration gives a
        // move at least as good as the previous depth's.
        if (g_pvLen[0] > 0) {
            best = g_pv[0][0];
            g_rootBest = best;
        }
        if (g_stop) break;   // unfinished iteration: don't report it

        const long long ms = elapsedMs();
        std::string line = "info depth " + std::to_string(depth) + " score " + scoreToUci(score) +
                           " nodes " + std::to_string(g_nodes) +
                           " nps " + std::to_string(g_nodes * 1000 / std::max(1LL, ms)) +
                           " time " + std::to_string(ms) + " pv";
        for (int i = 0; i < g_pvLen[0]; ++i) line += " " + g_pv[0][i].toString();
        sendLine(line);

        if (softMs > 0 && ms >= softMs * 6 / 10) break;               // next depth would not finish in time
        if (!lim.infinite && std::abs(score) >= MATE_BOUND) break;     // forced mate found
    }
    return best;
}
