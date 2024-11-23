#pragma once

#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <lib2k/synchronized.hpp>
#include <map>
#include <vector>
#include "log_entry.hpp"
#include "utils.hpp"

class Logger final {
private:
    static constexpr auto buffer_flush_threshold = usize{ 90 };
    std::filesystem::path m_filepath;
    std::vector<LogEntry> m_log_entries;

    explicit Logger(std::filesystem::path const& base_path)
        : m_filepath{ base_path / std::format("{}.log", get_current_date_time()) } {
        create_empty_file(m_filepath);
    }

public:
    Logger(Logger const& other) = delete;
    Logger(Logger&& other) noexcept = default;
    Logger& operator=(Logger const& other) = delete;
    Logger& operator=(Logger&& other) noexcept = default;

    ~Logger() noexcept {
        flush(*this);
    }

    static void log(LogEntry&& entry) {
        instance().apply([&](Logger& logger) {
            logger.m_log_entries.emplace_back(std::move(entry));
            if (logger.m_log_entries.size() >= buffer_flush_threshold) {
                flush(logger);
            }
        });
    }

private:
    [[nodiscard]] static c2k::Synchronized<Logger>& instance() {
        static auto logger = c2k::Synchronized{ Logger{ "logs" } };
        return logger;
    }

    static void flush(Logger& logger) {
        auto file = std::ofstream{ logger.m_filepath, std::ios::out | std::ios::binary | std::ios::app };
        if (not file) {
            spdlog::error("Failed to open log file for writing.");
            logger.m_log_entries.clear();
            return;
        }
        for (auto const& entry : logger.m_log_entries) {
            file << entry;
        }
        logger.m_log_entries.clear();
    }
};
