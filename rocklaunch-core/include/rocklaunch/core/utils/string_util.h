#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace rocklaunch
{

inline std::string ToLower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

inline bool EndsWith(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size()
        && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace rocklaunch