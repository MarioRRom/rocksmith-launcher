#include "cli_ui.h"

#include <iomanip>
#include <iostream>
#include <sstream>

bool IsForceFlag(std::string_view arg)
{
    return arg == "-f" || arg == "--force";
}

bool IsKnownSubcommand(std::string_view command, std::string_view sub)
{
    if (command == "runner") {
        return sub == "-u" || sub == "list" || sub == "set" || sub == "search"
            || sub == "install" || sub == "remove";
    }
    if (command == "patch") {
        return sub == "list" || sub == "add" || sub == "remove" || sub == "status";
    }
    if (command == "profile") {
        return sub == "list" || sub == "new" || sub == "show" || sub == "remove";
    }
    return false;
}

// Usage line for an error block; falls back to the command group when the
// subcommand is unknown or absent.
std::string CommandUsage(const std::string &command, const std::string &sub)
{
    if (command == "patch") {
        if (sub == "list") {
            return "rocklaunch-cli patch list [<profile>]";
        }
        if (sub == "add") {
            return "rocklaunch-cli patch add [-f] <profile> <patch>";
        }
        if (sub == "remove") {
            return "rocklaunch-cli patch remove [-f] <profile> <patch>";
        }
        if (sub == "status") {
            return "rocklaunch-cli patch status <profile> <patch>";
        }
        return "rocklaunch-cli patch <COMMAND>";
    }
    if (command == "runner") {
        if (sub == "-u") {
            return "rocklaunch-cli runner -u";
        }
        if (sub == "list") {
            return "rocklaunch-cli runner list";
        }
        if (sub == "set") {
            return "rocklaunch-cli runner set <profile> <runner_id>";
        }
        if (sub == "search") {
            return "rocklaunch-cli runner search <query>";
        }
        if (sub == "install") {
            return "rocklaunch-cli runner install <name> [<asset>]";
        }
        if (sub == "remove") {
            return "rocklaunch-cli runner remove <name>";
        }
        return "rocklaunch-cli runner <COMMAND>";
    }
    if (command == "profile") {
        if (sub == "list") {
            return "rocklaunch-cli profile list";
        }
        if (sub == "new") {
            return "rocklaunch-cli profile new [<name>]";
        }
        if (sub == "show") {
            return "rocklaunch-cli profile show <profile>";
        }
        if (sub == "remove") {
            return "rocklaunch-cli profile remove [-f] <profile>";
        }
        return "rocklaunch-cli profile <COMMAND>";
    }
    if (command == "launch") {
        return "rocklaunch-cli launch <profile>";
    }
    if (command == "set-path") {
        return "rocklaunch-cli set-path <profile> <path>";
    }
    return "rocklaunch-cli <command> [options]";
}

void PrintUsageEntry(const std::string &command, const std::string &description)
{
    std::cout << "    " << std::left << std::setw(40) << command << description << '\n';
}

void PrintUsage()
{
    std::cout << "rocklaunch-cli — Linux launcher for Rocksmith 2014\n"
              << "\n"
              << "USAGE:\n"
              << "    rocklaunch-cli <command> [options]\n"
              << "\n"
              << "PROFILES:\n";
    PrintUsageEntry("profile list", "List all profiles.");
    PrintUsageEntry("profile new [<name>]", "Create a profile (auto-named if omitted).");
    PrintUsageEntry("profile show <profile>", "Show profile details.");
    PrintUsageEntry("profile remove <profile>", "Remove a profile and its prefix.");
    PrintUsageEntry("set-path <profile> <path>", "Validate and set the game install path.");
    std::cout << "\nRUNNERS:\n";
    PrintUsageEntry("runner list", "List runners installed on this machine.");
    PrintUsageEntry("runner set <profile> <runner_id>", "Assign a runner to a profile.");
    PrintUsageEntry("runner search <query>", "Search online runner releases.");
    PrintUsageEntry("runner install <name> [<asset>]",
                    "Install a runner; picks the right file, or you specify one.");
    PrintUsageEntry("runner remove <name>", "Remove an installed runner.");
    std::cout << "\nPATCHES:\n";
    PrintUsageEntry("patch list", "List all available patches.");
    PrintUsageEntry("patch list <profile>", "List patches for a profile.");
    PrintUsageEntry("patch add [-f] <profile> <patch>", "Enable a patch (-f to force re-enable).");
    PrintUsageEntry("patch remove [-f] <profile> <patch>", "Disable a patch (-f to force).");
    PrintUsageEntry("patch status <profile> <patch>", "Show a patch's state.");
    std::cout << "\nLAUNCH:\n";
    PrintUsageEntry("launch <profile>", "Launch the profile's game in its prefix.");
    std::cout << "\nGENERAL:\n";
    PrintUsageEntry("--help, -h", "Show this help message.");
    PrintUsageEntry("--version", "Show the launcher version.");
}

std::string Color(const std::string &text, const char *code)
{
    return std::string("\033[") + code + "m" + text + "\033[0m";
}

// Left-aligns text in a fixed-width cell so ANSI colors do not disturb setw.
std::string PadLeft(const std::string &text, std::size_t width)
{
    return text.size() >= width ? text : text + std::string(width - text.size(), ' ');
}

void PrintError(const std::string &message)
{
    std::cerr << Color(message, "31") << '\n';
}

void PrintWarning(const std::string &message)
{
    std::cerr << Color(message, "33") << '\n';
}

void PrintUsageError(const std::string &message,
                     const std::string &command,
                     const std::string &subcommand)
{
    std::cerr << Color("error:", "31") << ' ' << message << '\n'
              << "\n"
              << "Usage: " << CommandUsage(command, subcommand) << '\n'
              << "\n"
              << "For more information, try '--help'.\n";
}

std::string HumanSize(uint64_t bytes)
{
    static const char *kUnits[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value << ' ' << kUnits[unit];
    return out.str();
}

std::string RepoShortName(const std::string &repo)
{
    std::size_t slash = repo.find_last_of('/');
    return slash == std::string::npos ? repo : repo.substr(slash + 1);
}