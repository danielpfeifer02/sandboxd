#include "logging/logger.hpp"
#include "sandbox/sandbox.hpp"
#include <iostream>

int main(int argc, char** argv) {
    Logger::instance().set_level(Logger::Level::Debug);

    std::string rawCmd = argv[1];
    for (int i = 2; i < argc; i++) {
        rawCmd += " " + std::string(argv[i]);
    }
    LOG_INFO("main", "Starting sandbox-cli");

    try {
        Sandbox sandbox(SandboxConfig{ .sandbox_id = "sandbox-1" });
        sandbox.Run(rawCmd);
    } catch (const std::exception& e) {
        LOG_ERROR("main", "Error starting sandbox: {}", e.what());
        return 1;
    }
    return 0;
}