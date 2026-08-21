#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace rocklaunch
{

namespace fs = std::filesystem;

// File-backed logger. Logging from startup keeps silent failures inside Proton debuggable.
class Logger
{
public:
    explicit Logger(fs::path logDir = DefaultLogDir());

    void Debug(std::string_view message) const;
    void Info(std::string_view message) const;
    void Warn(std::string_view message) const;
    void Error(std::string_view message) const;

    fs::path LogFile() const;

    static fs::path DefaultLogDir();

private:
    void Write(std::string_view level, std::string_view message) const;

    fs::path m_logFile;
};

} // namespace rocklaunch
