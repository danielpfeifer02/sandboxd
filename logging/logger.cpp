#include "logger.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <algorithm>

namespace Sandboxd::Logging {

static const std::string default_pattern = "[%Y-%m-%d %H:%M:%S] \t[ %^%l%$\t] %v";

class Logger::Impl {
public:
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink;
    
    Impl() {
        // Create console sink
        console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern(default_pattern);
        
        // Create logger with console sink
        logger = std::make_shared<spdlog::logger>("sandbox", console_sink);
        logger->set_pattern(default_pattern);
    }
};

Logger::Logger() : pImpl(std::make_unique<Impl>()) {}
Logger::~Logger() = default;

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(Level level) {
    spdlog::level::level_enum spdlog_level;
    switch(level) {
        case Level::Debug: spdlog_level = spdlog::level::debug; break;
        case Level::Info:  spdlog_level = spdlog::level::info; break;
        case Level::Warn:  spdlog_level = spdlog::level::warn; break;
        case Level::Error: spdlog_level = spdlog::level::err; break;
    }
    pImpl->logger->set_level(spdlog_level);
}

Logger::Level Logger::get_level() const {
    auto spdlog_level = pImpl->logger->level();
    switch(spdlog_level) {
        case spdlog::level::debug: return Level::Debug;
        case spdlog::level::info:  return Level::Info;
        case spdlog::level::warn:  return Level::Warn;
        case spdlog::level::err:   return Level::Error;
        default: return Level::Info;
    }
}

bool Logger::should_log(Level level) const {
    return static_cast<int>(level) >= static_cast<int>(get_level());
}

void Logger::log(Level level, std::string_view sandboxId, 
                 std::string_view message) {
    // Convert to string only when needed (spdlog will handle it efficiently)
    switch(level) {
        case Level::Debug: 
            pImpl->logger->debug("[sandboxId={}\t] [Process={}] {}", sandboxId, getpid(), message);
            break;
        case Level::Info:  
            pImpl->logger->info("[sandboxId={}\t] [Process={}] {}", sandboxId, getpid(), message);
            break;
        case Level::Warn:  
            pImpl->logger->warn("[sandboxId={}\t] [Process={}] {}", sandboxId, getpid(), message);
            break;
        case Level::Error: 
            pImpl->logger->error("[sandboxId={}\t] [Process={}] {}", sandboxId, getpid(), message);
            break;
    }
}

void Logger::enable_file_logging(const std::string& filepath, bool truncate) {
    try {
        // Create file sink
        pImpl->file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, truncate);
        pImpl->file_sink->set_pattern(default_pattern);
        
        // Add file sink to logger (logger now has both console and file)
        pImpl->logger->sinks().push_back(pImpl->file_sink);
    } catch (const spdlog::spdlog_ex& ex) {
        // Log error to console if file logging fails
        pImpl->logger->error("Failed to enable file logging: {}", ex.what());
    }
}

void Logger::disable_file_logging() {
    if (pImpl->file_sink) {
        // Remove file sink from logger
        auto& sinks = pImpl->logger->sinks();
        sinks.erase(
            std::remove(sinks.begin(), sinks.end(), pImpl->file_sink),
            sinks.end()
        );
        pImpl->file_sink.reset();
    }
}

}