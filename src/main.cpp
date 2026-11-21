#include "perft.h"
#include "position.h"
#include "uci.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

static void printUsage() {
    std::cout <<
        "Chesstrel - a UCI chess engine\n"
        "\n"
        "Usage:\n"
        "  chesstrel                    start in UCI mode (for chess GUIs / Lichess)\n"
        "  chesstrel perft N [FEN]      count positions N plies deep\n"
        "  chesstrel divide N [FEN]     perft with a per-move breakdown\n"
        "  chesstrel board [FEN]        print a position\n"
        "  chesstrel test               run the built-in correctness tests\n";
}

int main(int argc, char** argv) {
    initEngine();

    if (argc <= 1) {
        uciLoop();
        return 0;
    }

    const std::string cmd = argv[1];

    if (cmd == "perft" || cmd == "divide") {
        const int depth = (argc > 2) ? std::atoi(argv[2]) : 5;
        Position pos;
        if (argc > 3) pos.setFen(argv[3]);

        if (cmd == "divide") {
            perftDivide(pos, depth);
        } else {
            const auto t0 = std::chrono::steady_clock::now();
            const U64 nodes = perft(pos, depth);
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - t0).count();
            std::cout << "Nodes: " << nodes << "  Time: " << ms << " ms  NPS: "
                      << (ms > 0 ? nodes * 1000 / ms : nodes) << "\n";
        }
        return 0;
    }

    if (cmd == "board") {
        Position pos;
        if (argc > 2) pos.setFen(argv[2]);
        pos.print();
        return 0;
    }

    if (cmd == "test") return runPerftSuite() ? 0 : 1;

    printUsage();
    return cmd == "help" || cmd == "--help" ? 0 : 1;
}
