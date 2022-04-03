#pragma once

#include <string>
#include <vector>
#include <string_view>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

std::string as_string(std::string_view v);