#include "uci.h"

#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "search.h"

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#define ENGINE_NAME "Chesstrel 0.1"
#define ENGINE_AUTHOR "Pragya"   // TODO: put your name

namespace {
std::mutex g_outMutex;

// Find the legal move whose UCI text matches (e.g. "e2e4", "e7e8q").
Move parseMove(Position& pos, const std::string& text) {
    MoveList list;
    generateLegal(pos, list);
    for (const Move m : list)
        if (m.toString() == text) return m;
    return Move();
}
}  // namespace

void sendLine(const std::string& line) {
    std::lock_guard<std::mutex> lock(g_outMutex);
    std::cout << line << '\n' << std::flush;
}

void uciLoop() {
    Position pos;
    std::thread worker;
    bool infiniteSearch = false;

    // Searching runs on its own thread so we can still read "stop" and "quit".
    auto stopSearch = [&]() {
        searchStop();
        if (worker.joinable()) worker.join();
        infiniteSearch = false;
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream is(line);
        std::string cmd;
        is >> cmd;

        if (cmd == "uci") {
            sendLine("id name " ENGINE_NAME);
            sendLine("id author " ENGINE_AUTHOR);
            sendLine("uciok");
        } else if (cmd == "isready") {
            sendLine("readyok");
        } else if (cmd == "ucinewgame") {
            stopSearch();
            searchClear();
            pos.setFen(START_FEN);
        } else if (cmd == "position") {
            stopSearch();
            std::string tok;
            is >> tok;
            if (tok == "startpos") {
                pos.setFen(START_FEN);
                is >> tok;
            } else if (tok == "fen") {
                std::string fen;
                while (is >> tok && tok != "moves") fen += tok + " ";
                pos.setFen(fen);
            }
            if (tok == "moves") {
                while (is >> tok) {
                    const Move m = parseMove(pos, tok);
                    if (m.isNull()) break;   // illegal / unknown move: stop here
                    pos.makeMove(m);
                }
            }
        } else if (cmd == "go") {
            stopSearch();

            SearchLimits lim;
            bool hasDepth = false;
            std::string tok;
            while (is >> tok) {
                if (tok == "depth") { is >> lim.depth; hasDepth = true; }
                else if (tok == "movetime") is >> lim.movetime;
                else if (tok == "wtime") is >> lim.time[WHITE];
                else if (tok == "btime") is >> lim.time[BLACK];
                else if (tok == "winc") is >> lim.inc[WHITE];
                else if (tok == "binc") is >> lim.inc[BLACK];
                else if (tok == "movestogo") is >> lim.movestogo;
                else if (tok == "infinite") lim.infinite = true;
            }
            const int us = pos.sideToMove();
            if (!hasDepth && lim.movetime == 0 && lim.time[us] == 0) lim.infinite = true;

            infiniteSearch = lim.infinite;
            searchReset();
            Position copy = pos;
            worker = std::thread([copy, lim]() mutable {
                const Move best = search(copy, lim);
                // In infinite mode the protocol says: don't answer until told to stop.
                while (lim.infinite && !searchIsStopped())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                sendLine("bestmove " + (best.isNull() ? std::string("0000") : best.toString()));
            });
        } else if (cmd == "stop") {
            stopSearch();
        } else if (cmd == "quit") {
            stopSearch();
            return;
        } else if (cmd == "d") {
            pos.print();   // handy debug command: show the board
        } else if (cmd == "perft") {
            int depth = 1;
            is >> depth;
            perftDivide(pos, depth);
        }
    }

    // Input closed (e.g. piped commands ran out).
    if (infiniteSearch) searchStop();
    if (worker.joinable()) worker.join();
}
