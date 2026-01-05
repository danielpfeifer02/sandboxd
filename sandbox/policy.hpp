#pragma once

#include "types.hpp"
#include <string>

namespace Sandboxd::Sandbox::Policy {

Types::SandboxConfig ParsePolicyFile(const std::string& policyFile);

void ConfigureSeccomp(const Types::SandboxConfig& config);
void ConfigureNamespaces(const Types::SandboxConfig& config);

}