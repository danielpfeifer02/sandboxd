#include "types.hpp"
#include <string>

class Sandbox
{
public:
    Sandbox(const SandboxConfig& config);
    ~Sandbox();

    void Run(const std::string& command);

private:
    SandboxConfig mConfig;
};
