#pragma once

#include <string>

namespace Sandboxd::Sandbox::Types {

// TODO: incorporate all the config from the yaml policy file
struct SandboxConfig
{
    std::string sandbox_id;
};

}