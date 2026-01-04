#pragma once

#include "types.hpp"
#include <string>

namespace Sandboxd::Sandbox::Policy {

Types::SandboxConfig ParsePolicyFile(const std::string& policyFile)
{
    Types::SandboxConfig config;
    config.sandbox_id = "sandbox1";
    return config;
};

}