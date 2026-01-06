#include "sandbox.hpp"
#include "logging/logger.hpp"
#include "policy.hpp"
#include <iostream>
#include <sys/wait.h>

namespace Sandboxd::Sandbox {

Sandbox::Sandbox(const Types::SandboxConfig& config)
    : mConfig(config)
{
    // TODO: Initialize sandbox resources (if needed)
    LOG_INFO(mConfig.sandboxId, "Sandbox created");
}

Sandbox::~Sandbox()
{
    LOG_INFO(mConfig.sandboxId, "Sandbox destroyed");
}

int Sandbox::Run(int argc, char* argv[])
{
    LOG_DEBUG(mConfig.sandboxId, "Starting to run sandbox");

    ssize_t len = readlink("/proc/self/ns/mnt", mConfig.outside_ns_id, sizeof(mConfig.outside_ns_id) - 1);
    if (len < 0) {
        LOG_ERROR(mConfig.sandboxId, "Failed to read outside mount namespace ID: {}", strerror(errno));
        return 1;
    }

    pid_t pidLauncher = fork();
    if (pidLauncher == -1) {
        LOG_ERROR(mConfig.sandboxId, "Failed to fork: {}", strerror(errno));
        return 1;
    }

    if (pidLauncher == 0) {
        // --- LAUNCHER ---

        // - unshare()
        Policy::ConfigureNamespaces(mConfig);
        
        // - setrlimit()
        // - chroot()
        // - drop privileges()

        pid_t child = fork();
        if (child == -1) {
            dprintf(2, "fork (pidns child) failed: %s\n", strerror(errno));
            _exit(101);
        }

        if (child == 0) {
            // --- SANDBOXED GRANDCHILD (PID 1 inside PID namespace) ---

            // Configure mounts AFTER forking into PID namespace
            // This ensures procfs is mounted in the context of PID 1
            Policy::ConfigureMounts(mConfig);

            // 4) Install seccomp LAST (after all setup)
            // - seccomp()
            Policy::ConfigureSeccomp(mConfig);

            // 5) Exec target
            execvp(argv[0], &argv[0]);

            // exec only returns on failure
            LOG_ERROR(mConfig.sandboxId, "Failed to execute command: {}", strerror(errno));
            _exit(127);
        }

        // --- LAUNCHER waits and returns child status ---
        int status = 0;
        waitpid(child, &status, 0);

        LOG_DEBUG(mConfig.sandboxId, "Sandboxed grandchild [PID={}] exited with status: {}", child, status);

        if (WIFEXITED(status))
            return WEXITSTATUS(status);

        return 1;
    }

    // --- PARENT ---
    int status;
    waitpid(pidLauncher, &status, 0);

    LOG_DEBUG(mConfig.sandboxId, "Launcher [PID={}] exited with status: {}", pidLauncher, status);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 1;
}

}