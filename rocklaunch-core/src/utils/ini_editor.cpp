#include "rocklaunch/core/utils/ini_editor.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace rocklaunch
{
namespace IniEditor
{

namespace
{

struct Entry
{
    std::string key;
    std::string value;
    bool isComment; // true for comment/blank lines (key holds the raw line)
};

struct Section
{
    std::string name; // empty for the global section (before any [header])
    std::vector<Entry> entries;
};

using IniData = std::vector<Section>;

std::string Trim(const std::string &s)
{
    auto start = s.find_first_not_of(" \t\r");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r");
    return s.substr(start, end - start + 1);
}

IniData Parse(const std::string &content)
{
    IniData data;
    data.push_back({{}, {}}); // global section

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Strip trailing \r for \r\n files
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string trimmed = Trim(line);

        // Comment or blank line
        if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#') {
            data.back().entries.push_back({line, {}, true});
            continue;
        }

        // Section header
        if (trimmed.front() == '[' && trimmed.back() == ']') {
            std::string name = trimmed.substr(1, trimmed.size() - 2);
            data.push_back({std::move(name), {}});
            continue;
        }

        // key=value
        auto eq = trimmed.find('=');
        if (eq != std::string::npos) {
            std::string key = Trim(trimmed.substr(0, eq));
            std::string val = Trim(trimmed.substr(eq + 1));
            data.back().entries.push_back({std::move(key), std::move(val), false});
        }
    }

    return data;
}

std::string Serialize(const IniData &data)
{
    std::ostringstream out;

    for (size_t si = 0; si < data.size(); ++si) {
        const Section &sec = data[si];

        // Skip empty global section at the start
        if (si == 0 && sec.name.empty() && sec.entries.empty()) {
            continue;
        }

        // Skip empty trailing global section
        if (si == data.size() - 1 && sec.name.empty() && sec.entries.empty()) {
            continue;
        }

        if (!sec.name.empty()) {
            out << "[" << sec.name << "]\n";
        }

        for (const Entry &e : sec.entries) {
            if (e.isComment) {
                out << e.key << "\n";
            } else {
                out << e.key << "=" << e.value << "\n";
            }
        }
    }

    return out.str();
}

Section *FindSection(IniData &data, const std::string &name)
{
    for (Section &s : data) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

} // anonymous namespace

std::string Get(const fs::path &file, const std::string &section,
                const std::string &key)
{
    if (!fs::exists(file)) return {};

    std::ifstream in(file);
    if (!in.is_open()) return {};

    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    in.close();

    IniData data = Parse(content);

    // Search from last section with matching name (handles duplicates)
    std::string result;
    for (const Section &sec : data) {
        if (sec.name == section) {
            for (const Entry &e : sec.entries) {
                if (!e.isComment && e.key == key) {
                    result = e.value;
                }
            }
        }
    }

    return result;
}

void Set(const fs::path &file, const std::string &section,
         const std::string &key, const std::string &value)
{
    IniData data;

    if (fs::exists(file)) {
        std::ifstream in(file);
        if (in.is_open()) {
            std::string content((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
            in.close();
            data = Parse(content);
        }
    }

    if (data.empty()) {
        data.push_back({{}, {}});
    }

    // Find or create section
    Section *sec = FindSection(data, section);
    if (!sec) {
        data.push_back({section, {}});
        sec = &data.back();
    }

    // Find and update existing key
    for (Entry &e : sec->entries) {
        if (!e.isComment && e.key == key) {
            e.value = value;
            // Write back
            std::ofstream out(file);
            out << Serialize(data);
            out.close();
            return;
        }
    }

    // Key not found — append
    sec->entries.push_back({key, value, false});

    std::ofstream out(file);
    out << Serialize(data);
    out.close();
}

} // namespace IniEditor
} // namespace rocklaunch
