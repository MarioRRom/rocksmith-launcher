#include "rocklaunch/core/runtimes/system_wine_source.h"

#include <cstdlib>
#include <sstream>

namespace rocklaunch
{

std::vector<Runtime> SystemWineSource::Discover() const
{
    const char *pathValue = std::getenv("PATH");
    if (pathValue == nullptr) {
        return {};
    }

    std::stringstream paths(pathValue);
    std::string directory;
    while (std::getline(paths, directory, ':')) {
        fs::path executable = fs::path(directory) / "wine";
        std::error_code error;
        if (fs::is_regular_file(executable, error)) {
            return {{ "system-wine", "System Wine", RuntimeType::Wine, "system",
                      executable.parent_path(), executable }};
        }
    }

    return {};
}

} // namespace rocklaunch
