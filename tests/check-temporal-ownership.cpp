// SPDX-License-Identifier: GPL-3.0-or-later
// Independent-provider source-ownership contract. No Python runtime.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
std::string read(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    require(static_cast<bool>(stream), "Cannot read " + path.string());
    return std::string(std::istreambuf_iterator<char>(stream), {});
}
bool contains(const std::string& text, const std::string& value) {
    return text.find(value) != std::string::npos;
}
std::string json_field(const std::string& body, const std::string& key) {
    const std::regex expression("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    require(std::regex_search(body, match, expression),
            "Provider has no JSON string field: " + key);
    return match[1].str();
}
void validate(const fs::path& root) {
    require(!fs::exists(root / "resources/temporal"),
            "Provider source, translations and model tests must live in independent repos");
    const std::string makefile = read(root / "Makefile");
    for (const char* name : {"temporal-resources", "temporal-resource-package",
                              "temporal-resource-check"}) {
        require(!std::regex_search(makefile,
            std::regex(std::string("(^|\\n)") + name + R"(\s*:)")),
            std::string("Retired combined build target: ") + name);
    }
    require(!contains(makefile, "resources/temporal"),
            "Provider source paths must not be present in Makefile");
    require(contains(makefile, "tools/check-temporal-providers.sh"),
            "Independent-provider validation entry point missing");
    require(!contains(read(root / "CMakeLists.txt"), "resources/temporal/src/"),
            "Common CMake must not compile independent provider implementation");
    const std::string ci = read(root / ".github/workflows/ci.yml");
    const std::string release = read(root / ".github/workflows/release.yml");
    for (const auto* text : {&ci, &release}) {
        for (const char* forbidden : {"common-temporal-resources",
                                      "temporal-resource-package",
                                      "infiltrator-calendar-resources_"})
            require(!contains(*text, forbidden),
                    std::string("Retired combined calendar package reference: ") + forbidden);
    }
    require(contains(ci, "temporal-provider-check"),
            "CI no longer verifies independent providers");
    const std::string manifest = read(root / "tools/temporal-providers.json");
    const std::regex object(R"(\{[^{}]*\})");
    std::set<std::string> packages;
    unsigned calendars = 0, clocks = 0, count = 0;
    for (auto it = std::sregex_iterator(manifest.begin(), manifest.end(), object);
         it != std::sregex_iterator(); ++it) {
        const std::string body = it->str();
        const std::string package = json_field(body, "package");
        const std::string kind = json_field(body, "kind");
        require(!package.empty() && packages.insert(package).second,
                "Duplicate or empty independent provider package: " + package);
        if (kind == "calendar") ++calendars;
        else if (kind == "clock") ++clocks;
        else require(false, "Unknown temporal provider kind: " + kind);
        ++count;
    }
    require(count == 54 && packages.size() == 54 && calendars == 31 && clocks == 23,
            "Independent provider manifest must contain 31 calendars, 23 clocks and 54 unique packages");
    std::cout << "PASS: Common owns no provider sources; 31 calendar and 23 clock repositories own their implementations\n";
}
} // namespace
int main(int argc, char** argv) {
    try {
        require(argc <= 2, "Usage: check-temporal-ownership [common-root]");
        validate(fs::absolute(argc == 2 ? fs::path(argv[1]) : fs::path(".")));
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
