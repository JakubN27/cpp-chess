#include "../include/cli.h"
#include <string>

BotConfig parse_bot_config(int argc, char** argv) {
    BotConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--white-bot" || a == "--whitebot") {
            cfg.white_bot = true;
        } else if (a == "--black-bot" || a == "--blackbot") {
            cfg.black_bot = true;
        } else if (a == "--both-bot" || a == "--bothbot") {
            cfg.white_bot = true;
            cfg.black_bot = true;
        } else if (a == "--no-bot" || a == "--nobot") {
            cfg.white_bot = false;
            cfg.black_bot = false;
        }
    }

    return cfg;
}
