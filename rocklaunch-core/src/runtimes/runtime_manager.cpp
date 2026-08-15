#include "rocklaunch/core/runtimes/runtime_manager.h"

#include "rocklaunch/core/runtimes/managed_runtime_source.h"
#include "rocklaunch/core/runtimes/steam_runtime_source.h"
#include "rocklaunch/core/runtimes/system_wine_source.h"

#include <algorithm>
#include <set>

namespace rocklaunch
{

RuntimeManager::RuntimeManager(std::vector<std::unique_ptr<IRuntimeSource>> sources)
    : m_sources(std::move(sources))
{
}

std::vector<Runtime> RuntimeManager::List() const
{
    std::vector<Runtime> runtimes;
    std::set<std::string> ids;
    for (const std::unique_ptr<IRuntimeSource> &source : m_sources) {
        for (Runtime runtime : source->Discover()) {
            if (ids.insert(runtime.id).second) {
                runtimes.emplace_back(std::move(runtime));
            }
        }
    }

    std::sort(runtimes.begin(), runtimes.end(), [](const Runtime &left, const Runtime &right) {
        return left.id < right.id;
    });
    return runtimes;
}

std::optional<Runtime> RuntimeManager::Find(const std::string &runtimeId) const
{
    for (Runtime runtime : List()) {
        if (runtime.id == runtimeId) {
            return runtime;
        }
    }

    return std::nullopt;
}

RuntimeManager RuntimeManager::CreateDefault()
{
    std::vector<std::unique_ptr<IRuntimeSource>> sources;
    sources.emplace_back(std::make_unique<SystemWineSource>());
    sources.emplace_back(std::make_unique<SteamRuntimeSource>());
    sources.emplace_back(std::make_unique<ManagedRuntimeSource>());
    return RuntimeManager(std::move(sources));
}

} // namespace rocklaunch
