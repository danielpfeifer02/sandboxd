#pragma once

#include "types.hpp"
#include <string>

namespace Sandboxd::Sandbox::Policy {

void VerifyMountNamespace(const Types::SandboxConfig& config);
Types::SandboxConfig ParsePolicyFile(const std::string& policyFile);

void ConfigureSeccomp(const Types::SandboxConfig& config);
void ConfigureNamespaces(const Types::SandboxConfig& config);
void ConfigureMounts(const Types::SandboxConfig& config);

}