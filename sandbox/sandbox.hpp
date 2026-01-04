#pragma once

#include "types.hpp"
#include <string>

namespace Sandboxd::Sandbox {

class Sandbox
{
public:
    Sandbox(const Types::SandboxConfig& config);
    ~Sandbox();

    void Run(const std::string& command);

private:
    Types::SandboxConfig mConfig;
};

}