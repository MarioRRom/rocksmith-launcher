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

} // namespace

Logger::Logger(fs::path logDir)
    : m_logFile(std::move(logDir) / "rocklaunch.log")
{
    fs::create_directories(m_logFile.parent_path());
}

void Logger::Info(std::string_view message) const
{
    Write("INFO", message);
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
    std::string timestamp = Timestamp();
    std::string fileLine = timestamp + " [" + std::string(level) + "] " + std::string(message);

    std::cerr << "[" << level << "] " << message << '\n';

    std::ofstream output(m_logFile, std::ios::app);
    if (!output.is_open()) {
        throw std::runtime_error("Unable to open log file: " + m_logFile.string());
    }

    output << fileLine << '\n';
}

} // namespace rocklaunch
