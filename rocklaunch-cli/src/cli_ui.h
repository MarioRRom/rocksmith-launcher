#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// One color per entity type so every command renders them consistently.
constexpr const char *kRunnerColor = "33";  // yellow
constexpr const char *kPatchColor = "31";   // red
constexpr const char *kProfileColor = "36"; // cyan

std::string Color(const std::string &text, const char *code);
std::string PadLeft(const std::string &text, std::size_t width);
std::string HumanSize(uint64_t bytes);
std::string RepoShortName(const std::string &repo);

void PrintError(const std::string &message);
void PrintWarning(const std::string &message);
void PrintUsageError(const std::string &message,
                     const std::string &command,
                     const std::string &subcommand = "");

void PrintUsage();

bool IsForceFlag(std::string_view arg);
bool IsKnownSubcommand(std::string_view command, std::string_view sub);
std::string CommandUsage(const std::string &command, const std::string &sub);