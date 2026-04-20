#ifndef CLI_H
#define CLI_H

struct BotConfig {
    bool white_bot = false;
    bool black_bot = false;
    bool lichess = false;
};

BotConfig parse_bot_config(int argc, char** argv);

#endif
