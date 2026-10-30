// SPDX-License-Identifier: GPL-3.0-or-later
// Acquire, authenticate and verify the independent temporal provider releases.
// This native C++17 tool replaces the Python release-package test harness.
#include <curl/curl.h>
#include <json-c/json.h>
#include <openssl/evp.h>
#include <unistd.h>
#include <sys/wait.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace {
using std::string;
using Json = std::unique_ptr<json_object, decltype(&json_object_put)>;
[[noreturn]] void fail(const string& message) { throw std::runtime_error(message); }
void require(bool condition, const string& message) { if (!condition) fail(message); }
string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "Cannot read " + path.string());
    return string(std::istreambuf_iterator<char>(input), {});
}
void write(const fs::path& path, const string& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    require(static_cast<bool>(output), "Cannot write " + path.string());
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();
    require(static_cast<bool>(output), "Incomplete write: " + path.string());
}
Json parse(const string& source) {
    json_tokener* parser = json_tokener_new();
    require(parser != nullptr, "Could not allocate JSON parser");
    json_object* object = json_tokener_parse_ex(parser, source.data(),
                                                static_cast<int>(source.size()));
    const auto error = json_tokener_get_error(parser);
    json_tokener_free(parser);
    require(error == json_tokener_success && object != nullptr, "Invalid JSON document");
    return Json(object, json_object_put);
}
json_object* get(json_object* object, const char* name) {
    json_object* member = nullptr;
    require(object != nullptr && json_object_object_get_ex(object, name, &member),
            string("Missing JSON property: ") + name);
    return member;
}
string str(json_object* object, const char* name) {
    json_object* member = get(object, name);
    require(json_object_is_type(member, json_type_string),
            string("Expected JSON string: ") + name);
    return json_object_get_string(member);
}
string optional_str(json_object* object, const char* name, const string& fallback = "") {
    json_object* member = nullptr;
    if (!json_object_object_get_ex(object, name, &member)) return fallback;
    require(json_object_is_type(member, json_type_string), string("Expected JSON string: ") + name);
    return json_object_get_string(member);
}
int64_t integer(json_object* object, const char* name) {
    json_object* member = get(object, name);
    require(json_object_is_type(member, json_type_int), string("Expected JSON integer: ") + name);
    return json_object_get_int64(member);
}
bool boolean(json_object* object, const char* name) {
    json_object* member = get(object, name);
    require(json_object_is_type(member, json_type_boolean),
            string("Expected JSON boolean: ") + name);
    return json_object_get_boolean(member);
}
size_t append(char* data, size_t count, size_t size, void* target) {
    if (count && size > SIZE_MAX / count) return 0;
    const size_t n = count * size;
    auto* output = static_cast<string*>(target);
    if (output->size() > 512u * 1024u * 1024u - n || n > 512u * 1024u * 1024u) return 0;
    output->append(data, n);
    return n;
}
string request(const string& url, bool api) {
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
        require(curl != nullptr, "Cannot initialize HTTPS client");
        string output;
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "Infiltrator-independent-provider-test");
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
        curl_easy_setopt(curl.get(), CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 120L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, append);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &output);
        struct curl_slist* headers = nullptr;
        if (api) {
            headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
            const char* token = std::getenv("GITHUB_TOKEN");
            if (!token || !*token) token = std::getenv("GH_TOKEN");
            if (token && *token) headers = curl_slist_append(
                headers, (string("Authorization: Bearer ") + token).c_str());
        }
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);
        const CURLcode error = curl_easy_perform(curl.get());
        long status = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status);
        if (headers != nullptr) curl_slist_free_all(headers);
        if (error == CURLE_OK && status >= 200 && status < 300) return output;
        if (status >= 400 && status < 500)
            fail("HTTP " + std::to_string(status) + " for " + url);
        if (attempt == 3)
            fail("Cannot download " + url + " (HTTP " + std::to_string(status) +
                 ", " + curl_easy_strerror(error) + ")");
        std::this_thread::sleep_for(std::chrono::seconds(1u << attempt));
    }
    fail("Download retries exhausted");
}
string sha256(const string& payload) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned len = 0;
    require(EVP_Digest(payload.data(), payload.size(), digest.data(), &len,
                       EVP_sha256(), nullptr) == 1 && len == 32, "SHA-256 failure");
    static constexpr char hex[] = "0123456789abcdef";
    string output;
    output.reserve(64);
    for (unsigned i = 0; i < len; ++i) {
        output.push_back(hex[digest[i] >> 4]);
        output.push_back(hex[digest[i] & 15]);
    }
    return output;
}
string lowercase(string source) {
    std::transform(source.begin(), source.end(), source.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return source;
}
int execute(const std::vector<string>& args, string* captured = nullptr) {
    require(!args.empty(), "Cannot execute an empty command");
    int fd[2]{-1, -1};
    if (captured) require(pipe(fd) == 0, "Cannot create output pipe");
    const pid_t pid = fork();
    require(pid >= 0, "Cannot fork package metadata process");
    if (pid == 0) {
        if (captured) {
            close(fd[0]); dup2(fd[1], STDOUT_FILENO); close(fd[1]);
        }
        std::vector<char*> argv;
        for (const string& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
        argv.push_back(nullptr);
        execv(argv[0], argv.data());
        _exit(127);
    }
    if (captured) {
        close(fd[1]);
        char buffer[4096];
        ssize_t n;
        while ((n = ::read(fd[0], buffer, sizeof buffer)) > 0) captured->append(buffer, n);
        close(fd[0]);
    }
    int status = 0;
    require(waitpid(pid, &status, 0) == pid, "Cannot wait for package process");
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
}
string trim(string data) {
    const auto first = data.find_first_not_of(" \r\n\t");
    if (first == string::npos) return {};
    return data.substr(first, data.find_last_not_of(" \r\n\t") - first + 1);
}
Json catalogue(const fs::path& root) {
    Json entries = parse(read(root / "tools/temporal-providers.json"));
    require(json_object_is_type(entries.get(), json_type_array) &&
            json_object_array_length(entries.get()) == 54,
            "Expected exactly 54 independent providers");
    std::set<string> names;
    for (int i = 0; i < 54; ++i) {
        auto* entry = json_object_array_get_idx(entries.get(), i);
        const string repository = str(entry, "repository");
        const string package = str(entry, "package");
        require(std::regex_match(repository, std::regex(R"((Calendar|Clock)-[A-Za-z0-9-]+)")) &&
                package == "infiltrator-" + lowercase(repository) &&
                names.insert(package).second,
                "Invalid independent provider identity");
    }
    return entries;
}
Json fetch_one(json_object* entry, const fs::path& directory) {
    const string repository = "Infiltrator-Projects/" + str(entry, "repository");
    const Json release = parse(request("https://api.github.com/repos/" +
                                       repository + "/releases/latest", true));
    const string tag = optional_str(release.get(), "tag_name");
    require(!boolean(release.get(), "draft") && !boolean(release.get(), "prerelease") &&
            std::regex_match(tag, std::regex(R"(v[0-9]+\.[0-9]+\.[0-9]+)")),
            repository + " has no stable canonical release");
    const string version = tag.substr(1);
    require(execute({"/usr/bin/dpkg", "--compare-versions", version, "ge",
                     optional_str(entry, "minimum_version", "1.0.0")}) == 0,
            repository + " is older than independent resource baseline");
    const string slug = lowercase(str(entry, "repository"));
    const string filename = str(entry, "package") + "_" + version + "_amd64.deb";
    const std::set<string> names{filename, slug + "-" + version + "-native-installer.run",
                                   slug + "-" + version + "-source.zip"};
    json_object* assets = get(release.get(), "assets");
    require(json_object_is_type(assets, json_type_array) &&
            json_object_array_length(assets) == 3,
            repository + " does not have the required three release assets");
    std::set<string> observed;
    json_object* package_asset = nullptr;
    for (int i = 0; i < 3; ++i) {
        json_object* asset = json_object_array_get_idx(assets, i);
        const string name = str(asset, "name");
        observed.insert(name);
        if (name == filename) package_asset = asset;
    }
    require(observed == names && package_asset, repository + " release asset names mismatch");
    const string expected_url = "https://github.com/" + repository +
                                "/releases/download/" + tag + "/" + filename;
    const string digest = optional_str(package_asset, "digest");
    require(str(package_asset, "browser_download_url") == expected_url &&
            std::regex_match(digest, std::regex(R"(sha256:[0-9a-f]{64})")),
            repository + " has no trusted package URL or digest");
    const int64_t expected_size = integer(package_asset, "size");
    require(expected_size >= 0 && expected_size <= 512LL * 1024 * 1024,
            repository + " has unsupported asset size");
    const string payload = request(expected_url, false);
    require(static_cast<int64_t>(payload.size()) == expected_size &&
            sha256(payload) == digest.substr(7),
            filename + " does not match the published bytes");
    write(directory / filename, payload);
    Json result(json_object_get(entry), json_object_put);
    // Copy fields into an independent object, so the same entry cannot be modified by another run.
    Json record(json_object_new_object(), json_object_put);
    json_object_object_foreach(result.get(), key, value) {
        json_object_object_add(record.get(), key, json_object_get(value));
    }
    json_object_object_add(record.get(), "version", json_object_new_string(version.c_str()));
    json_object_object_add(record.get(), "filename", json_object_new_string(filename.c_str()));
    json_object_object_add(record.get(), "sha256", json_object_new_string(digest.substr(7).c_str()));
    json_object_object_add(record.get(), "size", json_object_new_int64(expected_size));
    std::cout << "Verified independent package: " << filename << std::endl;
    return record;
}
void verify(const fs::path& root, const fs::path& directory, json_object*& records, Json& owner) {
    owner = parse(read(directory / "providers.json"));
    records = owner.get();
    require(json_object_is_type(records, json_type_array) &&
            json_object_array_length(records) > 0,
            "No independent package snapshot");
    Json definitions = catalogue(root);
    std::set<string> seen, expected_debs;
    for (size_t i = 0; i < json_object_array_length(records); ++i) {
        json_object* record = json_object_array_get_idx(records, static_cast<int>(i));
        const string package = str(record, "package");
        require(seen.insert(package).second, "Duplicate package snapshot identity");
        json_object* entry = nullptr;
        for (int j = 0; j < 54; ++j) {
            json_object* candidate = json_object_array_get_idx(definitions.get(), j);
            if (str(candidate, "package") == package) { entry = candidate; break; }
        }
        require(entry != nullptr, "Unknown provider package: " + package);
        json_object_object_foreach(entry, key, value) {
            json_object* actual = get(record, key);
            require(json_object_equal(value, actual),
                    "Snapshot differs from catalogue for " + package + "." + key);
        }
        const string version = str(record, "version");
        const string filename = package + "_" + version + "_amd64.deb";
        require(filename == str(record, "filename"), "Package filename does not match identity");
        const string payload = read(directory / filename);
        require(static_cast<int64_t>(payload.size()) == integer(record, "size") &&
                sha256(payload) == str(record, "sha256"), "Provider snapshot checksum mismatch: " + filename);
        for (const auto& [field, expected] : std::array<std::pair<string,string>,3>{{
            {"Package", package}, {"Version", version}, {"Architecture", "amd64"}}) {
            string output;
            require(execute({"/usr/bin/dpkg-deb", "-f", (directory / filename).string(),
                             field}, &output) == 0 && trim(output) == expected,
                    "Debian " + field + " differs from its published identity: " + filename);
        }
        expected_debs.insert(filename);
    }
    for (const auto& item : fs::directory_iterator(directory))
        if (item.path().extension() == ".deb")
            require(expected_debs.erase(item.path().filename().string()) == 1,
                    "Unexpected Debian package in independent snapshot");
    require(expected_debs.empty(), "Missing Debian package from independent snapshot");
}
void run(int argc, char** argv) {
    require(argc >= 4, "Usage: temporal-packages <fetch|verify|install> <directory> <common-root> [--select kind:id]...");
    const string command = argv[1];
    require(command == "fetch" || command == "verify" || command == "install",
            "Unrecognised provider test command");
    const fs::path directory = fs::absolute(argv[2]);
    const fs::path root = fs::absolute(argv[3]);
    std::set<string> selected;
    for (int i = 4; i < argc; ++i) {
        require(string(argv[i]) == "--select" && i + 1 < argc,
                "Expected --select kind:id");
        selected.insert(argv[++i]);
    }
    if (command == "fetch") {
        fs::create_directories(directory);
        require(fs::directory_iterator(directory) == fs::directory_iterator{},
                "Fetch destination must be empty; an existing snapshot is verified without refetching");
        Json entries = catalogue(root);
        std::vector<json_object*> chosen;
        for (int i = 0; i < 54; ++i) {
            json_object* item = json_object_array_get_idx(entries.get(), i);
            const string identity = str(item, "kind") + ":" + str(item, "id");
            if (selected.empty() || selected.count(identity)) chosen.push_back(item);
        }
        require(selected.empty() || chosen.size() == selected.size(), "Unknown requested provider");
        Json snapshot(json_object_new_array(), json_object_put);
        // At most four simultaneous HTTPS requests, as with the original tool.
        for (size_t offset = 0; offset < chosen.size(); offset += 4) {
            std::vector<std::future<Json>> jobs;
            for (size_t i = offset; i < std::min(chosen.size(), offset + 4); ++i) {
                json_object* item = chosen[i];
                jobs.push_back(std::async(std::launch::async, [item, &directory] {
                    return fetch_one(item, directory);
                }));
            }
            for (auto& job : jobs) {
                Json result = job.get();
                json_object_array_add(snapshot.get(), result.release());
            }
        }
        write(directory / "providers.json",
              string(json_object_to_json_string_ext(snapshot.get(), JSON_C_TO_STRING_PRETTY)) + "\n");
    }
    json_object* records = nullptr;
    Json storage(nullptr, json_object_put);
    verify(root, directory, records, storage);
    if (command == "install") {
        std::vector<string> arguments{"/usr/bin/apt-get", "-o",
            "DPkg::Lock::Timeout=300", "install", "-y", "--no-install-recommends"};
        for (size_t i = 0; i < json_object_array_length(records); ++i) {
            json_object* record = json_object_array_get_idx(records, static_cast<int>(i));
            arguments.push_back((directory / str(record, "filename")).string());
        }
        if (geteuid() != 0) {
            arguments.insert(arguments.begin(), "--");
            arguments.insert(arguments.begin(), "/usr/bin/sudo");
        }
        require(execute(arguments) == 0, "APT refused qualified provider packages");
    }
    std::cout << "Qualified independent provider snapshot: "
              << json_object_array_length(records) << " packages\n";
}
} // namespace
int main(int argc, char** argv) {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Could not initialise HTTPS client\n";
        return 1;
    }
    int status = 0;
    try { run(argc, argv); }
    catch (const std::exception& error) {
        std::cerr << "Independent provider setup failed: " << error.what() << '\n';
        status = 1;
    }
    curl_global_cleanup();
    return status;
}
