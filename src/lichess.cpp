#include "../include/lichess.h"
#include "../include/gamestate.h"
#include "../include/bot.h"
#include "../include/algebraic.h"

#include <curl/curl.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

size_t write_to_string(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* s = static_cast<std::string*>(userp);
    s->append(static_cast<const char*>(contents), total);
    return total;
}

struct CurlHandle {
    CURL* h = nullptr;
    CurlHandle() : h(curl_easy_init()) {}
    ~CurlHandle() {
        if (h) curl_easy_cleanup(h);
    }
    CurlHandle(const CurlHandle&) = delete;
    CurlHandle& operator=(const CurlHandle&) = delete;
};

struct CurlHeaders {
    curl_slist* list = nullptr;
    ~CurlHeaders() {
        if (list) curl_slist_free_all(list);
    }
};

std::string bearer_auth(const std::string& token) { return "Authorization: Bearer " + token; }

struct HttpResponse {
    long status = 0;
    std::string body;
};

HttpResponse http_post(const LichessConfig& cfg, const std::string& path, const std::string& body, const std::string& content_type) {
    CurlHandle curl;
    if (!curl.h) throw std::runtime_error("curl_easy_init failed");

    std::string url = cfg.base_url + path;

    CurlHeaders headers;
    headers.list = curl_slist_append(headers.list, bearer_auth(cfg.api_key).c_str());
    if (!content_type.empty()) {
        headers.list = curl_slist_append(headers.list, ("Content-Type: " + content_type).c_str());
    }

    std::string response;

    curl_easy_setopt(curl.h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER, headers.list);
    curl_easy_setopt(curl.h, CURLOPT_POST, 1L);
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl.h, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl.h, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl.h);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl_easy_perform failed: ") + curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl.h, CURLINFO_RESPONSE_CODE, &status);
    return HttpResponse{status, response};
}

HttpResponse http_get(const LichessConfig& cfg, const std::string& path) {
    CurlHandle curl;
    if (!curl.h) throw std::runtime_error("curl_easy_init failed");

    std::string url = cfg.base_url + path;

    CurlHeaders headers;
    headers.list = curl_slist_append(headers.list, bearer_auth(cfg.api_key).c_str());

    std::string response;

    curl_easy_setopt(curl.h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER, headers.list);
    curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl.h, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl.h);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl_easy_perform failed: ") + curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl.h, CURLINFO_RESPONSE_CODE, &status);
    return HttpResponse{status, response};
}

// Very small NDJSON split: each line is a JSON object.
std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream iss(s);
    while (std::getline(iss, cur)) {
        if (!cur.empty()) out.push_back(cur);
    }
    return out;
}

// Extract a simple string field value from a flat-ish JSON object.
// This is NOT a general JSON parser; it is intentionally minimal.
std::optional<std::string> json_get_string(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return std::nullopt;
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ')) ++pos;
    if (pos >= json.size() || json[pos] != '\"') return std::nullopt;
    ++pos;
    std::string value;
    while (pos < json.size() && json[pos] != '\"') {
        // handle basic escapes
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            value.push_back(json[pos + 1]);
            pos += 2;
            continue;
        }
        value.push_back(json[pos++]);
    }
    return value;
}

// Extract moves from a field like "moves":"e2e4 e7e5 ..."
std::string json_get_moves_field(const std::string& json) {
    auto v = json_get_string(json, "moves");
    return v.value_or("");
}

bool json_has_type(const std::string& json, const std::string& typeValue) {
    auto t = json_get_string(json, "type");
    return t && *t == typeValue;
}

std::vector<std::string> split_space(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

Move uci_to_move(const std::string& uci) {
    // uci: e2e4 or e7e8q
    auto file_to_col = [](char f) { return int(f - 'a'); };
    auto rank_to_row = [](char r) { return int('8' - r); };

    Move m{};
    m.start_col = file_to_col(uci[0]);
    m.start_row = rank_to_row(uci[1]);
    m.end_col = file_to_col(uci[2]);
    m.end_row = rank_to_row(uci[3]);
    if (uci.size() >= 5) {
        m.flags |= MOVE_PROMOTION;
        char pc = uci[4];
        // map promotion piece by color later in make_move; here set White by default and fix in legality if needed
        switch (pc) {
            case 'q': m.promotion = WHITEQUEEN; break;
            case 'r': m.promotion = WHITEROOK; break;
            case 'b': m.promotion = WHITEBISHOP; break;
            case 'n': m.promotion = WHITEKNIGHT; break;
            default: break;
        }
    }
    return m;
}

} // namespace

int run_lichess_bot(const LichessConfig& cfg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::cerr << "Connecting to Lichess as bot...\n";
    // sanity check token
    {
        auto me = http_get(cfg, "/api/account");
        if (me.status != 200) {
            std::cerr << "Failed /api/account: HTTP " << me.status << "\n";
            std::cerr << me.body << "\n";
            return 1;
        }
        auto username = json_get_string(me.body, "username").value_or("?");
        std::cerr << "Authenticated as: " << username << "\n";
    }

    std::cerr << "NOTE: This client is minimal. It can accept challenges and play moves, but JSON parsing is simplistic.\n";

    // Main loop: poll event stream by fetching in chunks.
    // Proper implementation should stream /api/stream/event; here we poll for simplicity.
    while (true) {
        try {
            auto events = http_get(cfg, "/api/stream/event");
            if (events.status != 200) {
                std::cerr << "Event stream failed: HTTP " << events.status << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            for (const auto& line : split_lines(events.body)) {
                if (json_has_type(line, "challenge")) {
                    auto id = json_get_string(line, "id");
                    if (!id) continue;
                    // accept all challenges
                    auto resp = http_post(cfg, "/api/challenge/" + *id + "/accept", "", "");
                    std::cerr << "Accepted challenge " << *id << " (HTTP " << resp.status << ")\n";
                } else if (json_has_type(line, "gameStart")) {
                    auto gameId = json_get_string(line, "id");
                    if (!gameId) continue;
                    std::cerr << "Game started: " << *gameId << "\n";

                    // stream game state (simplified polling)
                    GameState gs;
                    std::string last_moves;
                    while (true) {
                        auto st = http_get(cfg, "/api/bot/game/stream/" + *gameId);
                        if (st.status != 200) {
                            std::cerr << "Game stream failed: HTTP " << st.status << "\n";
                            break;
                        }

                        for (const auto& gline : split_lines(st.body)) {
                            // gameFull contains initial state and "white"/"black" usernames.
                            // gameState contains ongoing moves.
                            if (json_has_type(gline, "gameState") || json_has_type(gline, "gameFull")) {
                                std::string movesField = json_get_moves_field(gline);
                                if (movesField == last_moves) continue;
                                last_moves = movesField;

                                // reconstruct position by replaying moves from start
                                gs = GameState();
                                std::vector<std::string> uciMoves = split_space(movesField);
                                for (const auto& uci : uciMoves) {
                                    Move m = uci_to_move(uci);
                                    // Promotion piece must have correct color
                                    if ((m.flags & MOVE_PROMOTION) != 0) {
                                        bool white = gs.is_white_to_move();
                                        if (m.promotion == WHITEQUEEN || m.promotion == WHITEROOK || m.promotion == WHITEBISHOP || m.promotion == WHITEKNIGHT) {
                                            // if black to move, shift to black enum equivalents
                                            if (!white) {
                                                if (m.promotion == WHITEQUEEN) m.promotion = BLACKQUEEN;
                                                else if (m.promotion == WHITEROOK) m.promotion = BLACKROOK;
                                                else if (m.promotion == WHITEBISHOP) m.promotion = BLACKBISHOP;
                                                else if (m.promotion == WHITEKNIGHT) m.promotion = BLACKKNIGHT;
                                            }
                                        }
                                    }
                                    gs.make_move(m);
                                }

                                // find legal moves and play if it's our turn.
                                // We don't yet robustly detect our color; assume bot plays side-to-move for now.
                                std::vector<Move> legal;
                                gs.generate_legal_moves(legal);
                                if (legal.empty()) {
                                    continue;
                                }

                                Move bm = choose_bot_move(gs, legal);
                                std::string uci = move_to_uci(bm);
                                auto play = http_post(cfg, "/api/bot/game/" + *gameId + "/move/" + uci, "", "");
                                std::cerr << "Played " << uci << " (HTTP " << play.status << ")\n";
                            }
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(250));
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Lichess loop error: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    // unreachable
    // curl_global_cleanup();
    return 0;
}
