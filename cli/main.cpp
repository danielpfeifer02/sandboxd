#include "logging/logger.hpp"
#include "sandbox/sandbox.hpp"
#include <iostream>

using namespace Sandboxd;

void ValidateArguments(int argc, char** argv)
{
    if (argc < 2) {
        LOG_ERROR("main", "Usage: sbx run <command>");
        exit(1);
    }
}

int main(int argc, char** argv) {
    Logging::Logger::instance().set_level(Sandboxd::Logging::Logger::Level::Debug);

    ValidateArguments(argc, argv);
    
    std::string rawCmd = argv[1];
    for (int i = 2; i < argc; i++) {
        rawCmd += " " + std::string(argv[i]);
    }
    LOG_INFO("main", "Starting sandbox-cli");

    try {
        Sandbox::Sandbox sandbox(Sandbox::Types::SandboxConfig{ .sandbox_id = "sandbox-1" });
        sandbox.Run(rawCmd);
    } catch (const std::exception& e) {
        LOG_ERROR("main", "Error starting sandbox: {}", e.what());
        return 1;
    }
    return 0;
}
