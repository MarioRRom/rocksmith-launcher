#include "rocklaunch/core/logger.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace rocklaunch
{

namespace
{

constexpr std::uintmax_t kMaxLogSize = 100 * 1024; // 100 KB
constexpr int kMaxRotatedFiles = 2; // keep .1 and .2

fs::path EnvironmentPath(const char *name)
{
    const char *value = std::getenv(name);
    return value != nullptr ? fs::path(value) : fs::path();
}

std::string Timestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime {};
    localtime_r(&now, &localTime);

    std::ostringstream timestamp;
    timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return timestamp.str();
}

// ANSI color for a log level, applied to the level token on the console only.
std::string LevelColor(std::string_view level)
{
    if (level == "DEBUG") {
        return "\033[36m";
    }
    if (level == "INFO") {
        return "\033[32m";
    }
    if (level == "WARN") {
        return "\033[33m";
    }
    if (level == "ERROR") {
        return "\033[31m";
    }
    return "\033[0m";
}

void RotateLog(const fs::path &logFile)
{
    std::error_code ec;
    if (!fs::is_regular_file(logFile, ec)) {
        return;
    }

    if (fs::file_size(logFile, ec) <= kMaxLogSize) {
        return;
    }

    // Remove the oldest rotated file.
    fs::path oldest = logFile.string() + "." + std::to_string(kMaxRotatedFiles);
    fs::remove(oldest, ec);

    // Shift: .1 -> .2, .log -> .1
    for (int i = kMaxRotatedFiles - 1; i >= 1; --i) {
        fs::path from = logFile.string() + "." + std::to_string(i);
        fs::path to = logFile.string() + "." + std::to_string(i + 1);
        fs::rename(from, to, ec);
    }

    fs::path first = logFile.string() + ".1";
    fs::rename(logFile, first, ec);
}

} // namespace

Logger::Logger(fs::path logDir)
    : m_logFile(std::move(logDir) / "rocklaunch.log")
{
    fs::create_directories(m_logFile.parent_path());
}

void Logger::Debug(std::string_view message) const
{
    Write("DEBUG", message);
}

void Logger::Info(std::string_view message) const
{
    Write("INFO", message);
}

void Logger::Warn(std::string_view message) const
{
    Write("WARN", message);
}

void Logger::Error(std::string_view message) const
{
    Write("ERROR", message);
}

fs::path Logger::LogFile() const
{
    return m_logFile;
}

fs::path Logger::DefaultLogDir()
{
    fs::path dataHome = EnvironmentPath("XDG_DATA_HOME");
    if (!dataHome.empty()) {
        return dataHome / "rocksmith-launcher" / "logs";
    }

    fs::path homeDir = EnvironmentPath("HOME");
    if (!homeDir.empty()) {
        return homeDir / ".local" / "share" / "rocksmith-launcher" / "logs";
    }

    throw std::runtime_error("Neither XDG_DATA_HOME nor HOME is set");
}

void Logger::Write(std::string_view level, std::string_view message) const
{
    std::cerr << "[" << LevelColor(level) << level << "\033[0m] " << message << '\n';

    if (level == "DEBUG") {
        return;
    }

    std::string timestamp = Timestamp();
    std::string fileLine = timestamp + " [" + std::string(level) + "] " + std::string(message);

    RotateLog(m_logFile);

    std::ofstream output(m_logFile, std::ios::app);
    if (!output.is_open()) {
        throw std::runtime_error("Unable to open log file: " + m_logFile.string());
    }

    output << fileLine << '\n';
}

} // namespace rocklaunch
