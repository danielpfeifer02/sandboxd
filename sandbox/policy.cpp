#include "policy.hpp"
#include <seccomp.h>
#include <err.h>
#include <cstdlib>

// TODO: LOG_ERROR instead of perror

namespace Sandboxd::Sandbox::Policy {

Types::SandboxConfig ParsePolicyFile(const std::string& policyFile)
{
    Types::SandboxConfig config;
    config.sandboxId = "sandbox1";
    return config;
}

void ConfigureSeccomp(const Types::SandboxConfig& config)
{
    scmp_filter_ctx seccomp_ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!seccomp_ctx)
        err(1, "seccomp_init failed");

    // if (seccomp_rule_add_exact(seccomp_ctx, SCMP_ACT_KILL, seccomp_syscall_resolve_name("uname"), 0)) {
    //     perror("seccomp_rule_add_exact failed");
    //     exit(1);
    // }

    if (seccomp_load(seccomp_ctx)) {
        perror("seccomp_load failed");
        exit(1);
    }

    seccomp_release(seccomp_ctx);
}

void ConfigureNamespaces(const Types::SandboxConfig& config)
{
    if (unshare(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWUSER)) {
        perror("unshare failed");
        exit(1);
    }
}

}
