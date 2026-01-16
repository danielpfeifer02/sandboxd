#define _GNU_SOURCE
#include "policy.hpp"
#include <seccomp.h>
#include <err.h>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <cstring>
#include <cerrno>
#include "logging/logger.hpp"

// TODO: LOG_ERROR instead of perror

namespace Sandboxd::Sandbox::Policy {

// Safety check: Verify we're in a mount namespace before pivot_root
// This prevents accidentally breaking the system if CLONE_NEWNS was not used
 void VerifyMountNamespace(const Types::SandboxConfig& config)
{
    // Check if we can access our mount namespace
    struct stat st;
    if (stat("/proc/self/ns/mnt", &st) != 0) {
        LOG_ERROR("VerifyMountNamespace", "SAFETY CHECK FAILED: Cannot access mount namespace. "
                "pivot_root requires CLONE_NEWNS. Aborting to prevent system damage.");
        exit(1);
    }
    
    // Read our mount namespace ID
    char our_ns[64] = {0};
    ssize_t len = readlink("/proc/self/ns/mnt", our_ns, sizeof(our_ns) - 1);
    if (len < 0) {
        LOG_ERROR("VerifyMountNamespace", "SAFETY CHECK FAILED: Cannot read mount namespace. "
                "pivot_root requires CLONE_NEWNS. Aborting to prevent system damage.");
        exit(1);
    }

    LOG_DEBUG("VerifyMountNamespace", "Our mount namespace ID: {}", our_ns);
    LOG_DEBUG("VerifyMountNamespace", "Outside mount namespace ID: {}", config.outside_ns_id);

    if (len != strlen(config.outside_ns_id) || strncmp(our_ns, config.outside_ns_id, len) == 0) {
        LOG_ERROR("VerifyMountNamespace", "SAFETY CHECK FAILED: Our mount namespace ID does match the outside namespace ID. "
                "pivot_root requires CLONE_NEWNS. Aborting to prevent system damage.");
        exit(1);
    }
    
    // Additional verification: If MS_PRIVATE mount succeeded earlier, we're likely
    // in a mount namespace. But the definitive check is that we can perform mount
    // operations without affecting the host (which is what MS_PRIVATE ensures).
    // 
    // Note: This check verifies namespace infrastructure exists, but doesn't
    // definitively prove we're isolated. However, if CLONE_NEWNS was not used,
    // the MS_PRIVATE mount on line 72 would have affected the host, which is
    // a different problem. This check catches cases where namespace setup failed.
    //
    // For development: If you see this error, verify that ConfigureNamespaces()
    // is called before ConfigureMounts() and that CLONE_NEWNS is included.
}

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
    uid_t uid = getuid();
    gid_t gid = getgid();

    if (unshare(CLONE_NEWUSER)) {
        perror("unshare failed for user namespace");
        exit(1);
    }

    std::ofstream setgroups("/proc/self/setgroups");
    setgroups << "deny" << std::endl;

    std::ofstream uid_map("/proc/self/uid_map");
    uid_map << "0 " << uid << " 1" << std::endl;

    std::ofstream gid_map("/proc/self/gid_map");
    gid_map << "0 " << gid << " 1" << std::endl;

    if (unshare(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWNET)) {
        perror("unshare failed for mount namespace");
        exit(1);
    }
}

void ConfigureMounts(const Types::SandboxConfig& config)
{
    // 1. Set mount propagation
    if (mount("none", "/", NULL, MS_PRIVATE | MS_REC, NULL)) {
        perror("mount MS_PRIVATE failed");
        exit(1);
    }

    // 2. Create rootfs directory with unique path per sandbox
    // Using sandboxId ensures no collisions between concurrent sandboxes
    std::string rootfsDir = "/tmp/sandbox-rootfs-" + config.sandboxId;
    mkdir(rootfsDir.c_str(), 0755);
    
    // Create subdirectories inside rootfs
    mkdir((rootfsDir + "/proc").c_str(), 0755);
    mkdir((rootfsDir + "/sys").c_str(), 0755);
    mkdir((rootfsDir + "/dev").c_str(), 0755);
    mkdir((rootfsDir + "/tmp").c_str(), 0755);
    mkdir((rootfsDir + "/usr").c_str(), 0755);
    mkdir((rootfsDir + "/lib").c_str(), 0755);
    mkdir((rootfsDir + "/lib64").c_str(), 0755);
    mkdir((rootfsDir + "/bin").c_str(), 0755);
    
    // Create putold directory for pivot_root
    std::string putold = rootfsDir + "/.oldroot";
    mkdir(putold.c_str(), 0755);

    // 3. Bind mount rootfs to itself (REQUIRED for pivot_root)
    if (mount(rootfsDir.c_str(), rootfsDir.c_str(), NULL, MS_BIND | MS_REC, NULL)) {
        perror("bind mount rootfs failed");
        exit(1);
    }

    // 4. Mount filesystems INSIDE rootfs
    // Note: MS_NOSUID | MS_NOEXEC | MS_NODEV are required for procfs in user namespaces:
    //   - MS_NODEV: /proc shouldn't have device nodes (it's a virtual filesystem)
    //   - MS_NOSUID: Prevents setuid/setgid abuse (security)
    //   - MS_NOEXEC: Prevents executing binaries from /proc (security)
    if (mount("proc", (rootfsDir + "/proc").c_str(), "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL)) {
        LOG_ERROR("ConfigureMounts", "mount /proc failed: {} (errno: {})", strerror(errno), errno);
        exit(1);
    }
    
    if (mount("sysfs", (rootfsDir + "/sys").c_str(), "sysfs", 0, NULL)) {
        LOG_ERROR("ConfigureMounts", "mount /sys failed: {} (errno: {})", strerror(errno), errno);
        exit(1);
    }
    
    if (mount("tmpfs", (rootfsDir + "/tmp").c_str(), "tmpfs", 0, NULL)) {
        LOG_ERROR("ConfigureMounts", "mount /tmp failed: {} (errno: {})", strerror(errno), errno);
        exit(1);
    }
    
    // Try to bind mount the entire /dev directory from host
    // This is the most reliable approach in user namespaces
    // If this fails, we'll fall back to an empty tmpfs (some programs may fail without /dev)
    struct stat dev_st;
    if (stat("/dev", &dev_st) == 0 && S_ISDIR(dev_st.st_mode) &&
        mount("/dev", (rootfsDir + "/dev").c_str(), NULL, MS_BIND | MS_REC, NULL) == 0) {
            LOG_DEBUG("ConfigureMounts", "Successfully bind mounted /dev from host");
    } else {
        // Fallback: mount empty tmpfs
        LOG_ERROR("ConfigureMounts", "bind mount /dev failed, falling back to empty tmpfs");
        if (mount("tmpfs", (rootfsDir + "/dev").c_str(), "tmpfs", MS_NOEXEC | MS_NOSUID, "mode=755") != 0) {
            LOG_ERROR("ConfigureMounts", "mount /dev (tmpfs) failed: {} (errno: {})", strerror(errno), errno);
            // Non-fatal - continue without /dev
        }
    }

    // 5. Bind mount host directories into rootfs
    struct {
        const char* source;
        const char* target;
    } bind_mounts[] = {
        {"/usr", "/usr"},
        {"/lib", "/lib"},
        {"/lib64", "/lib64"},
        {"/bin", "/bin"},
    };

    for (size_t i = 0; i < sizeof(bind_mounts) / sizeof(bind_mounts[0]); i++) {
        struct stat st;
        if (stat(bind_mounts[i].source, &st) == 0 && S_ISDIR(st.st_mode)) {
            std::string target = rootfsDir + bind_mounts[i].target;
            if (mount(bind_mounts[i].source, target.c_str(), NULL, MS_BIND | MS_RDONLY, NULL)) {
                // Non-fatal
            }
        }
    }

    // 6. SAFETY CHECK: Verify we're in a mount namespace before pivot_root
    // This prevents accidentally breaking the system during development
    VerifyMountNamespace(config);
    
    // 7. pivot_root to switch to new rootfs
    // Note: pivot_root has no glibc wrapper, must use syscall()
    if (syscall(SYS_pivot_root, rootfsDir.c_str(), putold.c_str())) {
        perror("pivot_root failed");
        exit(1);
    }

    // 8. Change to new root
    if (chdir("/")) {
        perror("chdir to new root failed");
        exit(1);
    }

    // 9. Unmount old root
    if (umount2("/.oldroot", MNT_DETACH)) {
        perror("umount old root failed");
        exit(1);
    }

    // 10. Remove old root directory
    rmdir("/.oldroot");
}

}
