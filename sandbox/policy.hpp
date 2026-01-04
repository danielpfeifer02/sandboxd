#pragma once

#include "types.hpp"
#include <seccomp.h>
#include <string>
#include <err.h>

namespace Sandboxd::Sandbox::Policy {

Types::SandboxConfig ParsePolicyFile(const std::string& policyFile)
{
    Types::SandboxConfig config;
    config.sandbox_id = "sandbox1";
    return config;
};

void ConfigureSeccomp(const Types::SandboxConfig& config)
{
    scmp_filter_ctx seccomp_ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!seccomp_ctx)
        err(1, "seccomp_init failed");

    if (seccomp_rule_add_exact(seccomp_ctx, SCMP_ACT_KILL, seccomp_syscall_resolve_name("uname"), 0)) {
        perror("seccomp_rule_add_exact failed");
        exit(1);
    }

    if (seccomp_load(seccomp_ctx)) {
        perror("seccomp_load failed");
        exit(1);
    }

    seccomp_release(seccomp_ctx);
}

}