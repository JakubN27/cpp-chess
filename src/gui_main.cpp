#include <SFML/Graphics.hpp>
#include "../include/gamestate.h"
#include "../include/piece.h"
#include "../include/bot.h"
#include "../include/cli.h"
#include <iostream>
#include <vector>
#include <string>

static bool square_in_list(int row, int col, const std::vector<Move>& moves, int start_row, int start_col){
    for (const auto& m : moves){
        if (m.start_row == start_row && m.start_col == start_col &&
            m.end_row == row && m.end_col == col){
            return true;
        }
    }
    return false;
}

static std::string game_status_text(GameState& gs){
    if (gs.is_checkmate()){
        return gs.is_white_to_move() ? "Checkmate - Black wins" : "Checkmate - White wins";
    }
    if (gs.is_stalemate()){
        return "Stalemate - Draw";
    }
    if (gs.is_draw_threefold_repetition()){
        return "Draw - Threefold repetition";
    }
    if (gs.is_draw_fifty_move_rule()){
        return "Draw - 50-move rule";
    }
    if (gs.is_draw_insufficient_material()){
        return "Draw - Insufficient material";
    }
    return "";
}

int main(int argc, char** argv) {
    const unsigned int tileSize = 80;
    const unsigned int boardSize = 8;
    const unsigned int winSize = tileSize * boardSize;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(winSize, winSize)), "Chessboard");
    window.setFramerateLimit(60);

    sf::Color light(240, 217, 181);
    sf::Color dark(181, 136, 99);

    sf::Color selectColor(246, 246, 105, 170);
    sf::Color moveColor(106, 246, 105, 140);

    GameState gs;

    sf::Font font;
    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        std::cerr << "Failed to load font: /System/Library/Fonts/Supplemental/Arial.ttf\n";
        return 1;
    }

    bool has_selection = false;
    int selected_row = -1;
    int selected_col = -1;

    BotConfig cfg = parse_bot_config(argc, argv);
    bool white_bot = cfg.white_bot;
    bool black_bot = cfg.black_bot;

    std::vector<Move> legal_moves;
    gs.generate_legal_moves(legal_moves);

    while (window.isOpen()) {
        std::string status_pre = game_status_text(gs);
        if (status_pre.empty()){
            bool bot_to_move = (gs.is_white_to_move() && white_bot) || (!gs.is_white_to_move() && black_bot);
            if (bot_to_move){
                gs.generate_legal_moves(legal_moves);
                if (!legal_moves.empty()){
                    Move bm = choose_bot_move(gs, legal_moves);
                    gs.make_move(bm);
                    gs.generate_legal_moves(legal_moves);
                    has_selection = false;
                    selected_row = -1;
                    selected_col = -1;
                }
            }
        }

        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouse = event->getIf<sf::Event::MouseButtonPressed>();
                if (mouse && mouse->button == sf::Mouse::Button::Left) {
                    // Convert from window pixel coordinates to world coordinates (handles view/HiDPI/window scaling).
                    sf::Vector2f world = window.mapPixelToCoords(mouse->position);

                    // Only accept clicks inside the board area.
                    if (world.x < 0.f || world.y < 0.f ||
                        world.x >= (float)winSize || world.y >= (float)winSize){
                        continue;
                    }

                    // Stop accepting moves when the game has ended.
                    if (!game_status_text(gs).empty()){
                        continue;
                    }

                    // Disable clicks if it's currently a bot-controlled side.
                    bool bot_to_move = (gs.is_white_to_move() && white_bot) || (!gs.is_white_to_move() && black_bot);
                    if (bot_to_move){
                        continue;
                    }

                    int col = (int)(world.x / (float)tileSize);
                    int row = (int)(world.y / (float)tileSize);

                    if (row >= 0 && row < (int)boardSize && col >= 0 && col < (int)boardSize){
                        const Board& b = gs.get_board();
                        Piece clicked = b[row][col];

                        if (!has_selection){
                            if (clicked != EMPTY && is_white(clicked) == gs.is_white_to_move()){
                                has_selection = true;
                                selected_row = row;
                                selected_col = col;
                            }
                        }
                        else{
                            // attempt move if destination is legal
                            if (square_in_list(row, col, legal_moves, selected_row, selected_col)){
                                Move chosen{};
                                bool found = false;
                                for (const auto& lm : legal_moves){
                                    if (lm.start_row == selected_row && lm.start_col == selected_col &&
                                        lm.end_row == row && lm.end_col == col){
                                        chosen = lm;
                                        found = true;
                                        break;
                                    }
                                }

                                if (found){
                                    gs.make_move(chosen);
                                    gs.generate_legal_moves(legal_moves);
                                }
                            }

                            // new selection if clicking own piece, otherwise clear
                            if (clicked != EMPTY && is_white(clicked) == gs.is_white_to_move()){
                                has_selection = true;
                                selected_row = row;
                                selected_col = col;
                            }
                            else{
                                has_selection = false;
                                selected_row = -1;
                                selected_col = -1;
                            }
                        }
                    }
                }
            }
        }

        window.clear();

        const Board& b = gs.get_board();

        for (unsigned int r = 0; r < boardSize; r++) {
            for (unsigned int c = 0; c < boardSize; c++) {
                sf::RectangleShape square(sf::Vector2f((float)tileSize, (float)tileSize));
                square.setPosition(sf::Vector2f((float)(c * tileSize), (float)(r * tileSize)));
                square.setFillColor(((r + c) % 2 == 0) ? light : dark);
                window.draw(square);

                if (has_selection && (int)r == selected_row && (int)c == selected_col){
                    sf::RectangleShape overlay(sf::Vector2f((float)tileSize, (float)tileSize));
                    overlay.setPosition(sf::Vector2f((float)(c * tileSize), (float)(r * tileSize)));
                    overlay.setFillColor(selectColor);
                    window.draw(overlay);
                }
                else if (has_selection && square_in_list((int)r, (int)c, legal_moves, selected_row, selected_col)){
                    sf::RectangleShape overlay(sf::Vector2f((float)tileSize, (float)tileSize));
                    overlay.setPosition(sf::Vector2f((float)(c * tileSize), (float)(r * tileSize)));
                    overlay.setFillColor(moveColor);
                    window.draw(overlay);
                }

                Piece p = b[r][c];
                if (p == EMPTY){
                    continue;
                }

                char ch = piece_to_char(p);

                sf::Text text(font);
                text.setString(sf::String(ch));
                text.setCharacterSize((unsigned int)(tileSize * 0.7f));
                text.setFillColor(sf::Color::Black);

                sf::FloatRect bounds = text.getLocalBounds();
                text.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f,
                                           bounds.position.y + bounds.size.y / 2.f));
                text.setPosition(sf::Vector2f((float)(c * tileSize + tileSize / 2),
                                              (float)(r * tileSize + tileSize / 2)));

                window.draw(text);
            }
        }

        std::string status = game_status_text(gs);
        if (!status.empty()){
            sf::RectangleShape overlay(sf::Vector2f((float)winSize, (float)winSize));
            overlay.setPosition(sf::Vector2f(0.f, 0.f));
            overlay.setFillColor(sf::Color(0, 0, 0, 140));
            window.draw(overlay);

            sf::Text text(font);
            text.setString(status);
            text.setCharacterSize(34);
            text.setFillColor(sf::Color::White);

            sf::FloatRect bounds = text.getLocalBounds();
            text.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2.f,
                                       bounds.position.y + bounds.size.y / 2.f));
            text.setPosition(sf::Vector2f((float)winSize / 2.f, (float)winSize / 2.f));

            window.draw(text);
        }

        window.display();
    }

    return 0;
}
