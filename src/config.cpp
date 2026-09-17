// © 2020 Joseph Cameron - All Rights Reserved

#include <jfc/wanikani_reviews_icon/config.h>
#include <jfc/wanikani_reviews_icon/build_info.h>

#include <jfc/storage/exception.h>
#include <jfc/storage/store.h>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace jfc::wanikani_reviews_icon;

static std::string wanikani_token = "";

static std::string browser_command =
#if defined JFC_TARGET_PLATFORM_Linux
    "xdg-open";
#elif defined JFC_TARGET_PLATFORM_Darwin
    "open";
#endif

std::string jfc::wanikani_reviews_icon::config::get_wanikani_token() {
    return wanikani_token;
}

std::string jfc::wanikani_reviews_icon::config::get_browser_command() {
    return browser_command;
}

namespace {
    constexpr const char *PROGRAM_NAME = "wanikani_reviews_icon";
    constexpr const char *CONFIG_FILENAME = "conf.json";

    [[nodiscard]] jfc::storage::store &store() {
        static jfc::storage::store value(jfc::storage::store::make_config(PROGRAM_NAME));

        return value;
    }

    [[nodiscard]] std::vector<std::byte> to_bytes(const std::string &aText) {
        std::vector<std::byte> out;

        out.reserve(aText.size());

        for (const char character : aText) out.push_back(static_cast<std::byte>(character));

        return out;
    }

    [[nodiscard]] std::string to_text(const std::vector<std::byte> &aBytes) {
        std::string out;

        out.reserve(aBytes.size());

        for (const std::byte value : aBytes) out.push_back(static_cast<char>(value));

        return out;
    }
}

using namespace nlohmann;

void jfc::wanikani_reviews_icon::config::load_config_file() {
    std::optional<std::vector<std::byte>> contents;

    try {
        contents = store().load_file(CONFIG_FILENAME);
    }
    catch (const jfc::storage::exception &e) {
        throw std::invalid_argument(std::string("Could not read the config file. error: {")
            + e.what() + "}");
    }

    if (!contents) {
        std::cout << "Creating config file: \"" << PROGRAM_NAME << "/" << CONFIG_FILENAME << "\"\n";

        save_config_file();

        contents = store().load_file(CONFIG_FILENAME);
    }

    try {
        auto root = json::parse(to_text(*contents));

        wanikani_token = root.value("wanikani_token", std::string());
        browser_command = root.value("browser_command", browser_command);

        std::string error_string;

        if (wanikani_token.empty()) error_string +=
            "Config file does not contain a wanikani token. "
            "A token is required in order to read your review summary. "
            "A token can be generated after logging into the wanikani website. "
            "Please add a valid token to the config file.";

        if (!error_string.empty()) throw std::invalid_argument(error_string);
    }

    catch (const nlohmann::json::exception &e) {
        throw std::invalid_argument(std::string("Could not read the config file. error: {")
            + e.what() + "}");
    }
}

void jfc::wanikani_reviews_icon::config::save_config_file() {
    json root;

    root["wanikani_token"] = wanikani_token;
    root["browser_command"] = browser_command;

    store().save_file(CONFIG_FILENAME, to_bytes(root.dump(4, ' ')));
}
