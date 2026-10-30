// SPDX-License-Identifier: GPL-3.0-or-later
// Verify canonical engineering philosophy adoption in public Infiltrator repos.
#include <curl/curl.h>
#include <json-c/json.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr const char* organisation = "Infiltrator-Projects";
constexpr const char* canonical_document =
    "https://github.com/Infiltrator-Projects/Libraries/blob/main/docs/PHILOSOPHY.md";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
size_t append_response(char* data, size_t count, size_t size, void* opaque) {
    auto& output = *static_cast<std::string*>(opaque);
    const size_t bytes = count * size;
    if (size != 0 && bytes / size != count) return 0;
    if (output.size() + bytes > 16 * 1024 * 1024) return 0;
    output.append(data, bytes);
    return bytes;
}
std::string get(const std::string& url, bool api) {
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> client(curl_easy_init(), curl_easy_cleanup);
    require(client != nullptr, "Cannot initialize HTTPS client");
    std::string output;
    curl_easy_setopt(client.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(client.get(), CURLOPT_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(client.get(), CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTPS);
    curl_easy_setopt(client.get(), CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(client.get(), CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(client.get(), CURLOPT_USERAGENT, "infiltrator-philosophy-check");
    curl_easy_setopt(client.get(), CURLOPT_WRITEFUNCTION, append_response);
    curl_easy_setopt(client.get(), CURLOPT_WRITEDATA, &output);
    curl_easy_setopt(client.get(), CURLOPT_FAILONERROR, 1L);
    struct curl_slist* headers = nullptr;
    if (api) {
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        const char* token = std::getenv("GITHUB_TOKEN");
        if (token != nullptr && token[0] != '\0')
            headers = curl_slist_append(headers, (std::string("Authorization: Bearer ") + token).c_str());
    }
    curl_easy_setopt(client.get(), CURLOPT_HTTPHEADER, headers);
    const CURLcode error = curl_easy_perform(client.get());
    if (headers != nullptr) curl_slist_free_all(headers);
    require(error == CURLE_OK, url + ": " + curl_easy_strerror(error));
    return output;
}
std::string string_field(json_object* object, const char* key) {
    json_object* member = nullptr;
    require(json_object_object_get_ex(object, key, &member) &&
            json_object_get_type(member) == json_type_string,
            std::string("Missing JSON field: ") + key);
    return json_object_get_string(member);
}
bool archived(json_object* object) {
    json_object* member = nullptr;
    return json_object_object_get_ex(object, "archived", &member) &&
           json_object_get_boolean(member);
}
int check() {
    int checked = 0;
    std::vector<std::string> failures;
    for (int page = 1; page <= 100; ++page) {
        const std::string url = std::string("https://api.github.com/orgs/") +
            organisation + "/repos?type=public&per_page=100&page=" + std::to_string(page);
        const std::string raw = get(url, true);
        json_tokener* parser = json_tokener_new();
        require(parser != nullptr, "Cannot allocate JSON parser");
        json_object* array = json_tokener_parse_ex(parser, raw.data(), static_cast<int>(raw.size()));
        const json_tokener_error parse_error = json_tokener_get_error(parser);
        json_tokener_free(parser);
        require(parse_error == json_tokener_success && array != nullptr &&
                json_object_is_type(array, json_type_array), "GitHub repo API did not return an array");
        const size_t batch = json_object_array_length(array);
        for (size_t index = 0; index < batch; ++index) {
            json_object* repo = json_object_array_get_idx(array, index);
            if (archived(repo)) continue;
            const std::string name = string_field(repo, "name");
            json_object* branch_field = nullptr;
            std::string branch = "main";
            if (json_object_object_get_ex(repo, "default_branch", &branch_field) &&
                json_object_is_type(branch_field, json_type_string))
                branch = json_object_get_string(branch_field);
            const std::string raw_url = std::string("https://raw.githubusercontent.com/") +
                organisation + "/" + name + "/" + branch + "/docs/PHILOSOPHY.md";
            try {
                const std::string content = get(raw_url, false);
                ++checked;
                if (name == "Libraries") {
                    if (content.find("# Infiltrator Engineering Philosophy") == std::string::npos)
                        failures.push_back(name + ": canonical document heading missing");
                } else if (content.find(canonical_document) == std::string::npos) {
                    failures.push_back(name + ": missing link to canonical philosophy");
                }
            } catch (const std::exception& error) {
                failures.push_back(name + ": unreadable docs/PHILOSOPHY.md (" + error.what() + ")");
            }
        }
        json_object_put(array);
        if (batch < 100) break;
    }
    if (!failures.empty()) {
        std::cerr << "Infiltrator philosophy adoption check FAILED:\n";
        for (const auto& failure : failures) std::cerr << " - " << failure << '\n';
        return 1;
    }
    std::cout << "Infiltrator philosophy adoption check passed for " << checked
              << " repositories.\n";
    return 0;
}
} // namespace
int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Cannot initialize HTTPS\n";
        return 1;
    }
    int status = 1;
    try { status = check(); }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; }
    curl_global_cleanup();
    return status;
}
