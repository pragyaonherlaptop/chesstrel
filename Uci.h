#pragma once
// UCI (Universal Chess Interface): the text protocol chess GUIs and Lichess use
// to talk to an engine.

#include <string>

// Reads commands from stdin until "quit" (or end of input).
void uciLoop();

// Thread-safe: prints one line to stdout and flushes.
void sendLine(const std::string& line);
