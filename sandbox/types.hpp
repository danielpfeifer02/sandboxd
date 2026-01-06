#pragma once

#include <string>

namespace Sandboxd::Sandbox::Types {

// TODO: incorporate all the config from the yaml policy file
struct SandboxConfig
{
    std::string sandboxId;
    char outside_ns_id[64] = {0};
};

}