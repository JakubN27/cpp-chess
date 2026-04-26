#include "../include/lichess.h"
#include "../include/gamestate.h"
#include "../include/bot.h"
#include "../include/algebraic.h"

#include <curl/curl.h>

#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

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

size_t write_to_string(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* s = static_cast<std::string*>(userp);
    s->append(static_cast<const char*>(contents), total);
    return total;
}

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

// Minimal JSON helpers (not a full parser)
// NOTE: these helpers are intentionally limited; they assume `"key":"value"` appears
// in the same object and is not ambiguous.
std::optional<std::string> json_get_string(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    size_t search_from = 0;

    while (true) {
        auto pos = json.find(needle, search_from);
        if (pos == std::string::npos) return std::nullopt;

        pos += needle.size();
        while (pos < json.size() && (json[pos] == ' ')) ++pos;

        // only return string-valued keys; skip numeric/object values with same key name
        if (pos >= json.size() || json[pos] != '\"') {
            search_from = pos;
            continue;
        }

        ++pos;
        std::string value;
        while (pos < json.size() && json[pos] != '\"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                value.push_back(json[pos + 1]);
                pos += 2;
                continue;
            }
            value.push_back(json[pos++]);
        }
        return value;
    }
}

static bool json_type_equals(const std::string& json, const std::string& typeValue) {
    auto t = json_get_string(json, "type");
    return t && *t == typeValue;
}

static bool json_side_has_identity(const std::string& json, const std::string& side, const std::string& username, const std::string& user_id) {
    // Extremely small helper for patterns like: "white":{..."name":"x","id":"x"...}
    const std::string k = "\"" + side + "\":";
    auto kpos = json.find(k);
    if (kpos == std::string::npos) return false;

    bool by_name = json.find("\"name\":\"" + username + "\"", kpos) != std::string::npos;
    bool by_id = !user_id.empty() && (json.find("\"id\":\"" + user_id + "\"", kpos) != std::string::npos);
    return by_name || by_id;
}

std::string json_get_moves_field(const std::string& json) {
    auto v = json_get_string(json, "moves");
    return v.value_or("");
}

bool account_is_bot(const std::string& json) {
    return json.find("\"title\":\"BOT\"") != std::string::npos;
}

bool is_game_stream_update(const std::string& json) {
    if (json_type_equals(json, "gameState") || json_type_equals(json, "gameFull")) {
        return true;
    }

    // Lichess can send the initial full game object without a "type" field.
    // It still contains nested "state" / "moves" data we need in order to move as White.
    return json.find("\"state\":") != std::string::npos || json.find("\"moves\":") != std::string::npos;
}

std::vector<std::string> split_space(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

bool apply_uci_legal_move(GameState& gs, const std::string& uci) {
    std::vector<Move> legal;
    gs.generate_legal_moves(legal);
    for (const Move& m : legal) {
        if (move_to_uci(m) == uci) {
            gs.make_move(m);
            return true;
        }
    }
    return false;
}

struct NdjsonStreamContext {
    std::string buffer;
    std::function<void(const std::string&)> on_line;
};

bool looks_like_complete_json_object(const std::string& s) {
    if (s.empty()) return false;

    int depth = 0;
    bool in_string = false;
    bool escape = false;

    for (char ch : s) {
        if (in_string) {
            if (escape) {
                escape = false;
                continue;
            }
            if (ch == '\\') {
                escape = true;
                continue;
            }
            if (ch == '\"') {
                in_string = false;
            }
            continue;
        }

        if (ch == '\"') {
            in_string = true;
            continue;
        }
        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth < 0) return false;
        }
    }

    return !in_string && !escape && depth == 0;
}

size_t ndjson_write_cb(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* ctx = static_cast<NdjsonStreamContext*>(userp);
    ctx->buffer.append(static_cast<const char*>(contents), total);

    // process full lines
    size_t start = 0;
    while (true) {
        size_t nl = ctx->buffer.find('\n', start);
        if (nl == std::string::npos) break;
        std::string line = ctx->buffer.substr(start, nl - start);
        if (!line.empty()) ctx->on_line(line);
        start = nl + 1;
    }
    if (start > 0) {
        ctx->buffer.erase(0, start);
    }

    // Some streams/chunks can arrive as plain JSON objects without a trailing newline.
    // Handle those so we do not stall waiting forever for '\n'.
    if (!ctx->buffer.empty() && looks_like_complete_json_object(ctx->buffer)) {
        std::string line = ctx->buffer;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ctx->buffer.clear();
        if (!line.empty()) ctx->on_line(line);
    }

    return total;
}

// Blocking ndjson stream. Returns when curl call ends (disconnect/error). Caller should reconnect.
void stream_ndjson(const LichessConfig& cfg, const std::string& path, const std::function<void(const std::string&)>& on_line) {
    CurlHandle curl;
    if (!curl.h) throw std::runtime_error("curl_easy_init failed");

    std::string url = cfg.base_url + path;

    CurlHeaders headers;
    headers.list = curl_slist_append(headers.list, bearer_auth(cfg.api_key).c_str());

    NdjsonStreamContext ctx;
    ctx.on_line = on_line;

    std::cerr << "Opening stream: " << path << "\n";

    curl_easy_setopt(curl.h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.h, CURLOPT_HTTPHEADER, headers.list);
    curl_easy_setopt(curl.h, CURLOPT_WRITEFUNCTION, ndjson_write_cb);
    curl_easy_setopt(curl.h, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl.h, CURLOPT_TCP_KEEPALIVE, 1L);

    CURLcode res = curl_easy_perform(curl.h);
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl_easy_perform failed: ") + curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl.h, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200) {
        throw std::runtime_error("stream request failed: HTTP " + std::to_string(status));
    }

    std::cerr << "Stream closed: " << path << "\n";
}

} // namespace

int run_lichess_bot(const LichessConfig& cfg) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::cerr << "Connecting to Lichess as bot...\n";
    std::string my_username;
    std::string my_user_id;
    {
        auto me = http_get(cfg, "/api/account");
        if (me.status != 200) {
            std::cerr << "Failed /api/account: HTTP " << me.status << "\n";
            std::cerr << me.body << "\n";
            return 1;
        }
        my_username = json_get_string(me.body, "username").value_or("?");
        my_user_id = json_get_string(me.body, "id").value_or("");
        if (!account_is_bot(me.body)) {
            std::cerr << "This account is not a Lichess Bot account.\n";
            std::cerr << "Upgrade it first with /api/bot/account/upgrade before using --lichess.\n";
            return 1;
        }
        std::cerr << "Authenticated as: " << my_username << "\n";
    }

    std::cerr << "Listening for events...\n";

    while (true) {
        try {
            // Stream events forever; reconnect on errors.
            stream_ndjson(cfg, "/api/stream/event", [&](const std::string& line) {
                if (json_type_equals(line, "challenge")) {
                    // We accept all challenges for now.
                    auto id = json_get_string(line, "id");
                    if (!id) {
                        std::cerr << "Challenge event without id: " << line << "\n";
                        return;
                    }
                    auto resp = http_post(cfg, "/api/challenge/" + *id + "/accept", "", "");
                    std::cerr << "Accepted challenge " << *id << " (HTTP " << resp.status << ")\n";
                } else if (json_type_equals(line, "gameStart")) {
                    auto gameId = json_get_string(line, "gameId");
                    if (!gameId) {
                        gameId = json_get_string(line, "id");
                    }
                    if (!gameId) {
                        std::cerr << "gameStart without id: " << line << "\n";
                        return;
                    }

                    std::cerr << "Game started: " << *gameId << "\n";

                    std::optional<bool> event_color_is_white;
                    auto event_color = json_get_string(line, "color");
                    if (event_color) {
                        if (*event_color == "white") event_color_is_white = true;
                        if (*event_color == "black") event_color_is_white = false;
                    }

                    // Stream game updates on a separate thread.
                    std::thread([cfg, gameId, my_username, my_user_id, event_color_is_white]() {
                        try {
                            GameState gs;
                            std::string last_moves;
                            bool seen_position = false;
                            bool awaiting_our_move = false;
                            std::optional<bool> bot_is_white = event_color_is_white;

                            std::cerr << "Starting game stream thread for " << *gameId << "\n";

                            stream_ndjson(cfg, "/api/bot/game/stream/" + *gameId, [&](const std::string& gline) {
                                if (!is_game_stream_update(gline)) {
                                    return;
                                }

                                // Determine our color once from gameFull.
                                if (!bot_is_white && (json_type_equals(gline, "gameFull") || gline.find("\"white\":") != std::string::npos)) {
                                    bool white_match = json_side_has_identity(gline, "white", my_username, my_user_id);
                                    bool black_match = json_side_has_identity(gline, "black", my_username, my_user_id);

                                    if (white_match && !black_match) {
                                        bot_is_white = true;
                                    } else if (black_match && !white_match) {
                                        bot_is_white = false;
                                    }
                                    if (bot_is_white) {
                                        std::cerr << "Detected bot color: " << (*bot_is_white ? "white" : "black") << "\n";
                                    } else {
                                        std::cerr << "Warning: could not detect bot color from gameFull\n";
                                    }
                                }

                                std::string movesField = json_get_moves_field(gline);
                                if (seen_position && movesField == last_moves && !awaiting_our_move && bot_is_white) {
                                    return;
                                }
                                last_moves = movesField;
                                seen_position = true;

                                // Rebuild position by replaying all UCI moves from the start.
                                gs = GameState();
                                std::vector<std::string> uciMoves = split_space(movesField);
                                for (const auto& uci : uciMoves) {
                                    if (!apply_uci_legal_move(gs, uci)) {
                                        std::cerr << "Failed to replay move " << uci
                                                  << " in game " << *gameId << "\n";
                                        return;
                                    }
                                }

                                // Only play when it's our turn.
                                if (!bot_is_white) {
                                    awaiting_our_move = false;
                                    return;
                                }

                                bool our_turn = (*bot_is_white == gs.is_white_to_move());
                                if (!our_turn) {
                                    awaiting_our_move = false;
                                    return;
                                }
                                awaiting_our_move = true;

                                std::vector<Move> legal;
                                gs.generate_legal_moves(legal);
                                if (legal.empty()) {
                                    awaiting_our_move = false;
                                    return;
                                }

                                Move bm = choose_bot_move(gs, legal);
                                std::string uci = move_to_uci(bm);

                                auto play = http_post(cfg, "/api/bot/game/" + *gameId + "/move/" + uci, "", "");
                                std::cerr << "Played " << uci << " (HTTP " << play.status << ")\n";
                                if (play.status != 200 && !play.body.empty()) {
                                    std::cerr << "Move response body: " << play.body << "\n";
                                }
                                if (play.status == 200) {
                                    awaiting_our_move = false;
                                }
                            });
                        } catch (const std::exception& e) {
                            std::cerr << "Game stream error: " << e.what() << "\n";
                        }
                    }).detach();
                }
            });

        } catch (const std::exception& e) {
            std::cerr << "Event stream error: " << e.what() << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    return 0;
}
