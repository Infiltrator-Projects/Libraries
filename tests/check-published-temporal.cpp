// SPDX-License-Identifier: GPL-3.0-or-later
// Common ABI qualification driver for 54 independently installed modules.
// Tests the actual .so providers: never compiles or embeds their implementations.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
namespace {
void require(bool good, const std::string& message) {
    if (!good) throw std::runtime_error(message);
}
std::string read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    require(static_cast<bool>(file), "Cannot read " + path.string());
    return std::string(std::istreambuf_iterator<char>(file), {});
}
std::string field(const std::string& entry, const std::string& name) {
    const std::regex pattern("\"" + name + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    require(std::regex_search(entry, match, pattern),
            "Provider JSON entry lacks expected string: " + name);
    return match[1].str();
}
fs::path resource_dir(const char* env, const fs::path& fallback) {
    const char* value = std::getenv(env);
    return value == nullptr ? fallback : fs::path(value);
}
void run(const fs::path& probe, const fs::path& root) {
    const fs::path calendar_root = resource_dir(
        "INFILTRATR_CALENDAR_RESOURCE_DIRECTORY", "/System/Resources/Calendars");
    const fs::path clock_root = resource_dir(
        "INFILTRATR_CLOCK_RESOURCE_DIRECTORY", "/System/Resources/Clocks");
    const bool installed = calendar_root == fs::path("/System/Resources/Calendars") &&
                           clock_root == fs::path("/System/Resources/Clocks");
    const std::string manifest = read(root / "tools/temporal-providers.json");
    const std::regex object(R"(\{[^{}]*\})");
    std::vector<std::string> args{probe.string(), installed ? "--installed" : "--direct"};
    std::set<std::string> seen;
    unsigned count = 0;
    for (auto it = std::sregex_iterator(manifest.begin(), manifest.end(), object);
         it != std::sregex_iterator(); ++it) {
        const std::string entry = it->str();
        const std::string kind = field(entry, "kind");
        const std::string id = field(entry, "id");
        const std::string name = field(entry, "name");
        require(kind == "calendar" || kind == "clock", "Invalid temporal provider kind");
        require(!id.empty() && seen.insert(id).second, "Duplicate or empty provider id");
        require(!name.empty() && name.find('/') == std::string::npos,
                "Invalid temporal provider resource name");
        const fs::path directory = kind == "calendar" ? calendar_root : clock_root;
        const fs::path path = directory / (name + ".so");
        require(fs::is_regular_file(path), "Expected installed .so not found: " + path.string());
        args.push_back(kind);
        args.push_back(id);
        args.push_back(path.string());
        args.push_back(name);
        ++count;
    }
    require(count == 54, "Exactly 54 independent calendar/clock providers required");
    require(setenv("LANGUAGE", "en_US", 1) == 0 && setenv("LC_ALL", "C.UTF-8", 1) == 0,
            "Failed to configure deterministic test locale");
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& value : args) argv.push_back(value.data());
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    throw std::runtime_error("Cannot execute native Common ABI consumer: " + probe.string());
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 3, "Usage: check-published-temporal <native-ABI-probe> <common-repo-root>");
        run(fs::path(argv[1]), fs::path(argv[2]));
        return 0; // execvp only returns on failure.
    } catch (const std::exception& error) {
        std::cerr << "Independent provider qualification failed: " << error.what() << '\n';
        return 1;
    }
}
