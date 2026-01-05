#pragma once

#include "types.hpp"
#include <string>

namespace Sandboxd::Sandbox {

class Sandbox
{
public:
    Sandbox(const Types::SandboxConfig& config);
    ~Sandbox();

    int Run(int argc, char* argv[]);

private:
    Types::SandboxConfig mConfig;
};

}