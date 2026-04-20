#pragma once

#include <string>

// Minimal Lichess Bot API client.
// Requires an API token with "bot:play" scope.

struct LichessConfig {
    std::string api_key;      // LICHESS_API_KEY
    std::string base_url = "https://lichess.org";
};

// Runs an interactive Lichess bot loop.
// - Listens to incoming events
// - For each game, streams game state and plays moves using the engine
//
// Notes:
// - This is a blocking call.
// - Printed logs go to stdout/stderr.
int run_lichess_bot(const LichessConfig& cfg);
