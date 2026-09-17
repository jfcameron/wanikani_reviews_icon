// © 2020 Joseph Cameron - All Rights Reserved
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>

#include <jfc/wanikani_reviews_icon/config.h>
#include <jfc/wanikani_reviews_icon/icon.h>
#include <jfc/wanikani_reviews_icon/notification.h>
#include <jfc/wanikani_reviews_icon/request.h>

#include <jfc/wanikani_reviews_icon/build_info.h>

using namespace jfc::wanikani_reviews_icon;

size_t last_review_count(0);

void failed_handler(request::failure aFailure);

namespace {
    constexpr unsigned POLL_SECONDS = 60;
    constexpr unsigned RETRY_SECONDS = 10;
    constexpr unsigned MAXIMUM_RETRY_SECONDS = 40;

    unsigned retry_seconds = 0;

    std::chrono::steady_clock::time_point next_poll;

    void schedule_next(const unsigned aSeconds) {
        next_poll = std::chrono::steady_clock::now() + std::chrono::seconds(aSeconds);
    }

    void schedule_after_success() {
        retry_seconds = 0;

        schedule_next(POLL_SECONDS);
    }

    void schedule_after_network_failure() {
        retry_seconds = retry_seconds
            ? std::min(retry_seconds * 2, MAXIMUM_RETRY_SECONDS)
            : RETRY_SECONDS;

        schedule_next(retry_seconds);
    }
}

void response_handler(std::vector<unsigned char> output) {
    const auto count = request::review_count(output);

    if (!count) {
        std::cerr << "could not read the summary response\n";
        return failed_handler(request::failure::other);
    }

    const std::size_t review_count = *count;
  
    if (review_count) {
        icon::set_graphic(icon::graphic::reviews);
        icon::set_badge(review_count);
    }
    else {
        icon::set_graphic(icon::graphic::no_reviews);
        icon::set_badge(std::nullopt);
    }

    if (review_count > last_review_count) notify::review_count_changed(review_count);

    last_review_count = review_count;

    schedule_after_success();
}

void failed_handler(const request::failure aFailure) {
    icon::set_graphic(icon::graphic::disconnected);
    icon::set_badge(std::nullopt);

    if (aFailure == request::failure::network) schedule_after_network_failure();
    else {
        retry_seconds = 0;
        schedule_next(POLL_SECONDS);
    }
}

gboolean update(gpointer) {
    request::submit_summary(&response_handler, &failed_handler);
    return G_SOURCE_CONTINUE;
}

gboolean pump(gpointer) {
    request::pump();

    if (std::chrono::steady_clock::now() >= next_poll) {
        schedule_next(POLL_SECONDS);
        update(nullptr);
    }

    return G_SOURCE_CONTINUE;
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 1) {
            config::load_config_file();

            gtk_init(&argc, &argv);

            icon::set_graphic(icon::graphic::init);
            
            g_timeout_add_seconds(1, [](void *) -> gboolean {
                    schedule_next(0);
                    g_timeout_add(250, pump, nullptr);
                    return G_SOURCE_REMOVE;
                }, 
                nullptr);
           
            gtk_main();

            request::shutdown();
        }
        else {
            std::vector<std::string> args(argv, argv + argc);

            if (args[1] == "-h" || args[1] == "--help") {
                std::cout
                    << "=== " << jfc::wanikani_reviews_icon::build_info::project_name << " ===" << "\n" 
                    << "a tray icon application that notifies you of new reviews on your wanikani account.\n"
                    << "=== build info ===\n"
                    << "project remote: " << jfc::wanikani_reviews_icon::build_info::git_remote_url << "\n"
                    << "git hash: " << jfc::wanikani_reviews_icon::build_info::git_commit << "\n"
                    << "build date: " << jfc::wanikani_reviews_icon::build_info::git_date << "\n";
            }
        }
    }
    catch (const std::runtime_error &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "error: unhandled exception" << std::endl;
    }

    return EXIT_SUCCESS;
}

