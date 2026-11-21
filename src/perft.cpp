#include "perft.h"

#include "movegen.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

U64 perft(Position& pos, int depth) {
    if (depth == 0) return 1;

    MoveList list;
    generateLegal(pos, list);
    if (depth == 1) return (U64)list.size();   // bulk counting at the leaves

    U64 nodes = 0;
    for (const Move m : list) {
        pos.makeMove(m);
        nodes += perft(pos, depth - 1);
        pos.unmakeMove();
    }
    return nodes;
}

void perftDivide(Position& pos, int depth) {
    MoveList list;
    generateLegal(pos, list);

    U64 total = 0;
    for (const Move m : list) {
        pos.makeMove(m);
        const U64 n = (depth <= 1) ? 1 : perft(pos, depth - 1);
        pos.unmakeMove();
        std::cout << m.toString() << ": " << n << "\n";
        total += n;
    }
    std::cout << "\nNodes searched: " << total << "\n";
}

namespace {

// Walks the tree and checks that (a) the incrementally-updated hash equals a
// from-scratch hash after every move, and (b) unmakeMove restores the exact
// previous position (FEN and hash).
bool selfCheck(Position& pos, int depth) {
    if (depth == 0) return true;

    MoveList list;
    generateLegal(pos, list);
    const std::string before = pos.fen();
    const U64 hashBefore = pos.key();

    for (const Move m : list) {
        pos.makeMove(m);
        bool ok = (pos.key() == pos.computeHash()) && selfCheck(pos, depth - 1);
        pos.unmakeMove();
        if (!ok || pos.fen() != before || pos.key() != hashBefore) {
            std::cerr << "  self-check failed around move " << m.toString() << " in " << before << "\n";
            return false;
        }
    }
    return true;
}

struct PerftCase {
    const char* name;
    const char* fen;
    std::vector<U64> expected;   // expected[i] = nodes at depth i + 1
};

}  // namespace

bool runPerftSuite() {
    // Standard positions from the Chess Programming Wiki.
    const std::vector<PerftCase> cases = {
        {"startpos", START_FEN, {20, 400, 8902, 197281, 4865609}},
        {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
         {48, 2039, 97862, 4085603}},
        {"position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
         {14, 191, 2812, 43238, 674624}},
        {"position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
         {6, 264, 9467, 422333}},
        {"position 4 (mirrored)", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
         {6, 264, 9467, 422333}},
        {"position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
         {44, 1486, 62379, 2103487}},
        {"position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
         {46, 2079, 89890, 3894594}},
    };

    bool allOk = true;
    for (const PerftCase& c : cases) {
        Position pos;
        pos.setFen(c.fen);
        std::cout << c.name << "\n";

        if (!selfCheck(pos, 3)) {
            std::cout << "  hash / make-unmake self-check: FAIL\n";
            allOk = false;
        }

        for (size_t i = 0; i < c.expected.size(); ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            const U64 n = perft(pos, (int)i + 1);
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - t0).count();
            const bool ok = (n == c.expected[i]);
            allOk = allOk && ok;
            std::cout << "  depth " << (i + 1) << ": " << std::setw(10) << n
                      << (ok ? "  OK" : "  FAIL (expected " + std::to_string(c.expected[i]) + ")")
                      << "  " << ms << " ms\n";
        }
    }

    std::cout << (allOk ? "\nAll perft tests passed.\n" : "\nPERFT TESTS FAILED.\n");
    return allOk;
}
