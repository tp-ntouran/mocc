// Global definitions for output files. This keeps us from having to pass
// instances of the log and output files all over the place.
#pragma once

#include <fstream>
#include "util/tee_stream.hpp"

extern std::fstream LogFile;
extern std::fstream OutFile;
extern std::string CaseName;

// Print to both the log file and standard output
extern teestream LogScreen;

void StartLogFile(const char* arg);

void StopLogFile();
