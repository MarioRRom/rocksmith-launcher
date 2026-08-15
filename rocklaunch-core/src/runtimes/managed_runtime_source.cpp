#include "rocklaunch/core/runtimes/managed_runtime_source.h"

#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace rocklaunch
{

ManagedRuntimeSource::ManagedRuntimeSource(fs::path runtimeDir)
    : m_runtimeDir(std::move(runtimeDir))
{
}

std::vector<Runtime> ManagedRuntimeSource::Discover() const
{
    std::vector<Runtime> runtimes;
    std::error_code error;
    if (!fs::is_directory(m_runtimeDir, error)) {
        return runtimes;
    }

    for (const fs::directory_entry &entry : fs::directory_iterator(m_runtimeDir)) {
        fs::path proton = entry.path() / "proton";
        if (entry.is_directory() && fs::is_regular_file(proton, error)) {
            std::string name = entry.path().filename().string();
            runtimes.push_back({ "managed-" + SanitizeRuntimeId(name), name,
                                 RuntimeType::Proton, "managed", entry.path(), proton });
        }
    }

    return runtimes;
}

fs::path ManagedRuntimeSource::DefaultRuntimeDir()
{
    const char *dataHome = std::getenv("XDG_DATA_HOME");
    if (dataHome != nullptr) {
        return fs::path(dataHome) / "rocksmith-launcher" / "runtimes";
    }

    const char *home = std::getenv("HOME");
    if (home != nullptr) {
        return fs::path(home) / ".local" / "share" / "rocksmith-launcher" / "runtimes";
    }

    throw std::runtime_error("Neither XDG_DATA_HOME nor HOME is set");
}

} // namespace rocklaunch
