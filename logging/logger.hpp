// logger.hpp
#pragma once
#include <string>
#include <string_view>
#include <memory>
#include <spdlog/fmt/bundled/format.h>  // Use spdlog's bundled fmt for templates

class Logger {
public:
    enum class Level { Debug, Info, Warn, Error };
    
    static Logger& instance();
    
    // Level management
    void set_level(Level level);
    Level get_level() const;
    
    // Check if level is enabled (for lazy evaluation)
    bool should_log(Level level) const;
    
    // Basic log - string_view avoids copies
    void log(Level level, std::string_view sandbox_id, 
             std::string_view message);
    
    // Formatted log - perfect forwarding, format only if enabled
    template<typename... Args>
    void log(Level level, std::string_view sandbox_id, 
             std::string_view fmt, Args&&... args) {
        // Format happens here - args are forwarded, no copies
        // Use spdlog's bundled fmt
        std::string formatted = ::fmt::format(::fmt::runtime(fmt), std::forward<Args>(args)...);
        log(level, sandbox_id, formatted);
    }
    
    // Convenience methods
    template<typename... Args>
    void debug(std::string_view sandbox_id, std::string_view fmt, Args&&... args) {
        log(Level::Debug, sandbox_id, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(std::string_view sandbox_id, std::string_view fmt, Args&&... args) {
        log(Level::Info, sandbox_id, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warn(std::string_view sandbox_id, std::string_view fmt, Args&&... args) {
        log(Level::Warn, sandbox_id, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(std::string_view sandbox_id, std::string_view fmt, Args&&... args) {
        log(Level::Error, sandbox_id, fmt, std::forward<Args>(args)...);
    }
    
private:
    Logger();
    ~Logger();  // Declare destructor (defined in .cpp after Impl is fully defined)
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// Convenience macros that call Logger methods directly
// spdlog handles level checking internally
#define LOG_DEBUG(id, ...) Logger::instance().debug(id, __VA_ARGS__)
#define LOG_INFO(id, ...)  Logger::instance().info(id, __VA_ARGS__)
#define LOG_WARN(id, ...)  Logger::instance().warn(id, __VA_ARGS__)
#define LOG_ERROR(id, ...) Logger::instance().error(id, __VA_ARGS__)
