#include "rocklaunch/core/subprocess.h"

#include <cstring>
#include <stdexcept>

#include <sys/wait.h>
#include <unistd.h>

namespace rocklaunch
{

void RunSubprocess(const std::vector<std::string> &args,
                   const fs::path &workDir)
{
    if (args.empty()) {
        throw std::runtime_error("RunSubprocess: empty args");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork failed: " + std::string(std::strerror(errno)));
    }

    if (pid == 0) {
        if (!workDir.empty()) {
            if (chdir(workDir.c_str()) != 0) {
                _exit(127);
            }
        }

        std::vector<const char *> argv;
        argv.reserve(args.size() + 1);
        for (auto &a : args) {
            argv.push_back(a.c_str());
        }
        argv.push_back(nullptr);

        execvp(argv[0], const_cast<char *const *>(argv.data()));
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        throw std::runtime_error("waitpid failed: " + std::string(std::strerror(errno)));
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code != 0) {
            std::string cmd;
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) cmd += " ";
                cmd += args[i];
            }
            throw std::runtime_error("Command failed (exit " + std::to_string(code)
                                     + "): " + cmd);
        }
    } else {
        throw std::runtime_error("Command killed by signal");
    }
}

} // namespace rocklaunch
