#include "rocklaunch/core/manual_source.h"

#include <utility>

namespace rocklaunch
{

ManualSource::ManualSource(fs::path installDir)
    : m_installDir(std::move(installDir))
{
}

// Resolve the path so symlinks never let one installation split across two profiles.
std::optional<fs::path> ManualSource::Locate(const IGameProfile &profile) const
{
    std::error_code error;
    fs::path resolvedDir = fs::weakly_canonical(m_installDir, error);
    if (error || !profile.ValidateInstall(resolvedDir)) {
        return std::nullopt;
    }

    return resolvedDir;
}

} // namespace rocklaunch
