#include "logger.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Sandboxd::Logging {

class Logger::Impl {
public:
    std::shared_ptr<spdlog::logger> logger;
    
    Impl() {
        logger = spdlog::stdout_color_mt("sandbox");
        logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
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

void Logger::log(Level level, std::string_view sandbox_id, 
                 std::string_view message) {
    // Convert to string only when needed (spdlog will handle it efficiently)
    switch(level) {
        case Level::Debug: 
            pImpl->logger->debug("[sandbox_id={}] {}", sandbox_id, message);
            break;
        case Level::Info:  
            pImpl->logger->info("[sandbox_id={}] {}", sandbox_id, message);
            break;
        case Level::Warn:  
            pImpl->logger->warn("[sandbox_id={}] {}", sandbox_id, message);
            break;
        case Level::Error: 
            pImpl->logger->error("[sandbox_id={}] {}", sandbox_id, message);
            break;
    }
}

}