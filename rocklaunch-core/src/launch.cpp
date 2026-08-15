#include "rocklaunch/core/launch.h"

#include "rocklaunch/core/launch_context.h"

#include <cstdlib>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace rocklaunch
{

namespace
{

int RunProcess(const std::vector<std::string> &command, const std::vector<std::string> &environment)
{
    pid_t child = fork();
    if (child == -1) {
        return -1;
    }

    if (child == 0) {
        for (const std::string &variable : environment) {
            std::size_t separator = variable.find('=');
            if (separator != std::string::npos) {
                setenv(variable.substr(0, separator).c_str(),
                       variable.substr(separator + 1).c_str(), 1);
            }
        }

        std::vector<char *> argv;
        argv.reserve(command.size() + 1);
        for (const std::string &argument : command) {
            argv.push_back(const_cast<char *>(argument.c_str()));
        }
        argv.push_back(nullptr);

        execvp(command.front().c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    if (waitpid(child, &status, 0) == -1) {
        return -1;
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// The wine binary used to configure a prefix. GE-Proton ships under files/;
// stock Steam Proton uses dist/.
fs::path WineBinaryFor(const Runtime &runtime)
{
    if (runtime.type == RuntimeType::Wine) {
        return runtime.executable;
    }

    for (const fs::path &layout : { "files", "dist" }) {
        fs::path candidate = runtime.rootDir / layout / "bin" / "wine";
        std::error_code error;
        if (fs::is_regular_file(candidate, error)) {
            return candidate;
        }
    }

    return runtime.rootDir / "dist" / "bin" / "wine";
}

} // namespace

std::vector<std::string> EnsurePrefix(const fs::path &prefixDir, const Runtime &runtime)
{
    std::vector<std::string> warnings;
    std::error_code error;
    fs::create_directories(prefixDir, error);
    if (error) {
        throw std::runtime_error("Unable to create prefix: " + prefixDir.string());
    }

    fs::path wine = WineBinaryFor(runtime);
    if (!fs::is_regular_file(wine, error)) {
        warnings.emplace_back("wine binary not found at " + wine.string()
                              + "; dsound=alsa was not applied");
        return warnings;
    }

    // dsound=alsa lets the game see any audio input as the Real Tone cable. Applied on every
    // launch because it is idempotent and covers prefixes created without it.
    int result = RunProcess({ wine.string(), "reg", "add", "HKCU\\Software\\Wine\\Drivers",
                              "/v", "dsound", "/d", "alsa", "/f" },
                            { "WINEPREFIX=" + prefixDir.string() });
    if (result != 0) {
        warnings.emplace_back("dsound=alsa could not be applied to the prefix (exit "
                              + std::to_string(result) + ")");
    }

    return warnings;
}

LaunchCommand BuildLaunchCommand(const ProfileConfig &profile,
                                 const Runtime &runtime,
                                 const IGameProfile &game)
{
    LaunchCommand launch;
    fs::path executable = game.Executable(profile.installDir);
    if (runtime.type == RuntimeType::Proton) {
        launch.command = { runtime.executable.string(), "run", executable.string() };
    } else {
        launch.command = { runtime.executable.string(), executable.string() };
    }

    LaunchContext context;
    context.installDir = profile.installDir;
    context.prefixDir = profile.prefixDir;
    context.runtimeId = runtime.id;

    if (runtime.type == RuntimeType::Wine) {
        launch.environment.emplace_back("WINEPREFIX=" + profile.prefixDir.string());
    } else {
        // Proton runs the game inside the prefix; point it at a real directory even without Steam.
        launch.environment.emplace_back("STEAM_COMPAT_DATA_PATH=" + profile.prefixDir.string());
        launch.environment.emplace_back("STEAM_COMPAT_CLIENT_INSTALL_PATH="
                                        + profile.prefixDir.parent_path().string());
    }
    // Universal audio buffer adjustment; harmless when PipeWire is not in use.
    launch.environment.emplace_back("PIPEWIRE_LATENCY=256/48000");

    for (const std::string &variable : game.RequiredEnv(context)) {
        launch.environment.emplace_back(variable);
    }

    return launch;
}

bool ExecLaunchCommand(const LaunchCommand &command)
{
    for (const std::string &variable : command.environment) {
        std::size_t separator = variable.find('=');
        if (separator != std::string::npos) {
            setenv(variable.substr(0, separator).c_str(),
                   variable.substr(separator + 1).c_str(), 1);
        }
    }

    std::vector<char *> argv;
    argv.reserve(command.command.size() + 1);
    for (const std::string &argument : command.command) {
        argv.push_back(const_cast<char *>(argument.c_str()));
    }
    argv.push_back(nullptr);

    execvp(command.command.front().c_str(), argv.data());
    return false;
}

} // namespace rocklaunch
