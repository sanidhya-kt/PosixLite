#include "shell.h"
#include <cctype>
#include <cstdlib>
#include <glob.h>

static std::vector<std::string> expand_wildcard(const std::string& pattern) {
    std::vector<std::string> matches;
    glob_t glob_result;
    
    // GLOB_NOCHECK: return pattern if no match found
    // GLOB_TILDE: expand tilde in paths
    int ret = glob(pattern.c_str(), GLOB_NOCHECK | GLOB_TILDE, nullptr, &glob_result);
    if (ret == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            matches.push_back(glob_result.gl_pathv[i]);
        }
        globfree(&glob_result);
    } else {
        matches.push_back(pattern);
    }
    return matches;
}

std::vector<std::string> tokenise(const std::string& line) {
    std::vector<std::string> tokens;
    std::string tok;
    bool in_single = false, in_double = false;
    bool has_wildcard = false;

    auto push_token = [&](std::string& t, bool& wildcard_flag) {
        if (t.empty()) return;
        if (wildcard_flag) {
            auto matches = expand_wildcard(t);
            for (const auto& match : matches) {
                tokens.push_back(match);
            }
            wildcard_flag = false;
        } else {
            tokens.push_back(t);
        }
        t.clear();
    };

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '\'' && !in_double) { in_single = !in_single; continue; }
        if (c == '"'  && !in_single) { in_double = !in_double; continue; }

        if (c == '$' && !in_single) {
            size_t start = i + 1;
            size_t len = 0;
            while (start + len < line.size() && (std::isalnum(line[start + len]) || line[start + len] == '_')) {
                len++;
            }
            if (len > 0) {
                std::string var_name = line.substr(start, len);
                const char* val = std::getenv(var_name.c_str());
                if (val) {
                    tok += val;
                }
                i += len;
                continue;
            }
        }

        if (!in_single && !in_double) {
            if (c == '*' || c == '?' || c == '[') {
                has_wildcard = true;
            }
            if (std::isspace(c)) {
                push_token(tok, has_wildcard);
                continue;
            }
            // Keep special chars as separate tokens
            if (c == '|' || c == '<' || c == '>' || c == '&') {
                push_token(tok, has_wildcard);
                // Check '>>'
                if (c == '>' && i + 1 < line.size() && line[i+1] == '>') {
                    tokens.push_back(">>"); ++i; continue;
                }
                tokens.push_back(std::string(1, c));
                continue;
            }
        }
        tok += c;
    }
    push_token(tok, has_wildcard);
    return tokens;
}
