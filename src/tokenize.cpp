#include "tokenize.hpp"
#include <cctype>

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    cur.reserve(text.size());
    for (unsigned char uc : text) {
        if (std::isalnum(uc)) {
            cur.push_back((char)std::tolower(uc));
        } else {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
