#include "string_processing.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;

    while (true) {
        size_t space = text.find(' ');
        words.push_back(text.substr(0u, space));
        if (space == text.npos) {
            break;
        }
        else {
            text.remove_prefix(space + 1);
        }
    }

    return words;
}

std::string as_string(std::string_view v) {
    return { v.data(), v.size() };
}