#include "sandbox.hpp"
#include "logging/logger.hpp"
#include <iostream>

namespace Sandboxd::Sandbox {

Sandbox::Sandbox(const Types::SandboxConfig& config)
    : mConfig(config)
{
    // TODO: Initialize sandbox resources (if needed)
    LOG_INFO(mConfig.sandbox_id, "Sandbox created");
}

Sandbox::~Sandbox()
{
    // TODO: Cleanup sandbox resources (if needed)
    LOG_INFO(mConfig.sandbox_id, "Sandbox destroyed");
}

void Sandbox::Run(const std::string& command)
{
    LOG_DEBUG(mConfig.sandbox_id, "Starting to run command '{}' in sandbox environment", command);

    // TODO: Run command

    LOG_DEBUG(mConfig.sandbox_id, "Finished running command '{}'", command);
}

}