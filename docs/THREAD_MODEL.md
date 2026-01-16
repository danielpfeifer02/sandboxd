# Threat Model: sandboxd

## Overview

This document defines the threat model for **sandboxd**, a secure sandbox runner that executes untrusted commands in isolated environments using Linux namespaces and security primitives. This threat model explicitly defines what we defend against, what we do not defend against, and the security boundaries for each development phase.

---

## What We Defend Against

### 1. Process Escape
**Threat**: A malicious process inside the sandbox attempts to escape isolation and access or interfere with host processes.

**Attack Vectors**:
- Accessing host process tree via `/proc` or `/sys`
- Using `ptrace()` to attach to host processes
- Escaping PID namespace to see or kill host processes
- Using `setns()` to join host namespaces

**Mitigations**:
- **PID namespace isolation**: Sandbox processes see only their own process tree (PID 1 inside sandbox)
- **Mount namespace isolation**: `/proc` and `/sys` are isolated and show only sandbox processes
- **seccomp-bpf filtering**: Block `ptrace`, `setns`, and other namespace manipulation syscalls
- **User namespace**: Even if escape occurs, processes run as non-root on host

**Week 1 Scope**: PID namespace isolation prevents seeing host processes. Mount namespace isolation prevents accessing host `/proc` and `/sys`.

---

### 2. Privilege Escalation
**Threat**: A process inside the sandbox attempts to gain elevated privileges on the host system.

**Attack Vectors**:
- Exploiting setuid binaries to gain root privileges
- Using `mount()` to mount filesystems with setuid capabilities
- Exploiting kernel vulnerabilities (out of scope, but we prevent common vectors)
- Using capabilities to perform privileged operations

**Mitigations**:
- **User namespace**: Map root (UID 0) inside sandbox to non-root UID on host
- **Capability dropping**: Remove all unnecessary capabilities (CAP_SYS_ADMIN, CAP_SYS_MOUNT, etc.)
- **PR_SET_NO_NEW_PRIVS**: Prevent privilege escalation via setuid binaries
- **Read-only rootfs**: Prevent installation of malicious setuid binaries
- **seccomp-bpf**: Block `mount`, `umount`, `chroot`, and other privilege-escalation syscalls

**Week 1 Scope**: Not yet implemented. Will be addressed in Week 2 (User Namespace + Privilege Dropping).

---

### 3. Resource Exhaustion
**Threat**: A malicious or buggy process consumes excessive system resources (CPU, memory, disk I/O, process count), causing denial of service to the host system.

**Attack Vectors**:
- Fork bombs creating unlimited processes
- Memory exhaustion via memory leaks or intentional allocation
- CPU spinning in tight loops
- Disk I/O exhaustion (writing large files, filling filesystem)
- File descriptor exhaustion

**Mitigations**:
- **cgroups v2**: Enforce hard limits on memory (`memory.max`), CPU (`cpu.max`), process count (`pids.max`), and I/O (`io.max`)
- **Wall-clock timeout**: Kill sandbox after maximum execution time
- **OOM killer integration**: cgroups v2 automatically kills processes exceeding memory limits
- **Process tree killing**: Ensure all descendants are killed when limits are exceeded

**Week 1 Scope**: Not yet implemented. Will be addressed in Week 4 (cgroups v2 + Timeouts).

---

### 4. Syscall Abuse
**Threat**: A malicious process uses dangerous system calls to compromise the host system or escape isolation.

**Attack Vectors**:
- `keyctl()`: Access kernel keyring, potentially exposing secrets
- `bpf()`: Load eBPF programs to manipulate kernel behavior
- `ptrace()`: Attach to host processes for debugging/exploitation
- `mount()`/`umount()`: Mount filesystems to escape isolation
- `perf_event_open()`: Access performance counters, potential side-channel attacks
- `kexec_load()`: Load new kernel, reboot system
- Raw sockets: Craft malicious network packets
- `io_uring`: Complex I/O operations that may bypass restrictions

**Mitigations**:
- **seccomp-bpf allowlist**: Only permit necessary syscalls for the workload
- **Profile-based policies**: Different syscall sets for different use cases (e.g., "compile" vs "run")
- **Block risky syscalls by default**: `keyctl`, `bpf`, `ptrace`, `mount`, `kexec_load`, raw sockets
- **Strict mode**: Fail-closed approach - if syscall not explicitly allowed, deny it

**Week 1 Scope**: Not yet implemented. Will be addressed in Week 5 (seccomp Allowlist).

---

### 5. Filesystem Access
**Threat**: A malicious process attempts to read sensitive host files or write malicious content to the host filesystem.

**Attack Vectors**:
- Reading `/etc/shadow`, `/etc/passwd`, SSH keys, or other sensitive files
- Writing malicious scripts or binaries to host filesystem
- Modifying system libraries or binaries
- Accessing user data outside the sandbox
- Using symlinks to escape rootfs boundaries

**Mitigations**:
- **Mount namespace isolation**: Sandbox sees only its own mount table
- **pivot_root**: Change root filesystem to minimal, prepared directory
- **Read-only rootfs**: Default filesystem is read-only
- **Bind mount allowlist**: Only specific host paths are accessible (e.g., `/usr`, `/lib`, `/lib64`) and only as read-only
- **tmpfs for `/tmp`**: Writable temporary directory is isolated tmpfs
- **No host filesystem access**: By default, no access to host files outside the allowlist
- **Symlink protection**: Ensure symlinks cannot escape rootfs (pivot_root handles this)

**Week 1 Scope**: Mount namespace isolation prevents seeing host mounts. Minimal rootfs setup begins, but full filesystem policy will be implemented in Week 3.

---

### 6. Network Access
**Threat**: A malicious process attempts to communicate with external networks or the host network, potentially exfiltrating data or receiving commands.

**Attack Vectors**:
- Connecting to external servers to exfiltrate data
- Receiving commands from command-and-control servers
- Scanning internal networks
- Participating in DDoS attacks
- Accessing host network interfaces

**Mitigations**:
- **Network namespace isolation**: Sandbox has its own network stack, isolated from host
- **Default: no network**: No network interfaces by default
- **Optional allowlist**: If networking is needed, only specific hosts/ports are allowed (stretch goal)
- **seccomp-bpf**: Block socket creation syscalls if network is disabled

**Week 1 Scope**: Not yet implemented. Will be addressed in Week 6 (Network Isolation).

---

## Non-Goals

We explicitly **do not** defend against the following threats. These are considered out of scope for sandboxd:

### 1. Kernel Exploits
**What**: Attacks that exploit vulnerabilities in the Linux kernel itself (e.g., use-after-free, buffer overflows in kernel code).

**Why Out of Scope**: 
- We assume a non-compromised, up-to-date kernel
- Kernel security is the responsibility of the kernel maintainers and system administrators
- Defending against kernel exploits would require kernel-level security mechanisms beyond our scope

**Mitigation in Practice**: Keep kernel updated, use kernel hardening features (e.g., `kernel.unprivileged_bpf_disabled=1`).

---

### 2. Hardware Attacks
**What**: Physical attacks on hardware, side-channel attacks via hardware (e.g., Spectre, Meltdown), or attacks on firmware/BIOS.

**Why Out of Scope**:
- Hardware security is outside the scope of a userspace sandbox runner
- Side-channel attacks require hardware-level mitigations
- Firmware security is a separate concern

**Note**: Some seccomp policies may incidentally help with certain side-channel attack vectors (e.g., blocking `perf_event_open`), but this is not a primary goal.

---

### 3. Full Side-Channel Attacks
**What**: Sophisticated side-channel attacks that leak information through timing, cache behavior, or other indirect means.

**Why Out of Scope**:
- Comprehensive side-channel defense requires hardware and kernel-level mitigations
- Our focus is on isolation and containment, not information leakage prevention
- Some mitigations (e.g., blocking `perf_event_open`) may help, but we don't guarantee side-channel resistance

**Partial Mitigations**: 
- Resource limits may reduce timing attack surface
- seccomp blocking of performance monitoring syscalls

---

### 4. Full Container Runtime
**What**: sandboxd is not a full container runtime like Docker, Podman, or containerd.

**What We Don't Provide**:
- Image management and distribution
- Container orchestration
- Multi-container networking
- Volume management beyond basic bind mounts
- Container image building
- Registry integration

**What We Do Provide**:
- Secure execution of single commands in isolated environments
- Policy-based security configuration
- Resource limits and timeouts
- Syscall filtering

**Use Case**: sandboxd is designed for running untrusted code in CI/CD, code execution platforms, or similar scenarios where you need to execute a command securely, not manage full container lifecycles.

---

### 5. Host System Compromise via Sandboxed Process
**What**: If the host system itself is compromised (e.g., root access obtained through other means), sandboxd cannot prevent the attacker from bypassing sandbox restrictions.

**Why Out of Scope**:
- If an attacker has root on the host, they can modify or bypass any userspace security mechanism
- We assume the host system is trusted and not compromised
- Our goal is to prevent the sandboxed process from compromising the host, not to defend against an already-compromised host

---

## Security Boundaries: Week 1 Scope

For **Week 1**, the security boundaries are intentionally limited. We focus on establishing basic process isolation as a foundation for future security enhancements.

### What Is Protected (Week 1)

1. **Process Tree Isolation**
   - Sandbox processes cannot see host processes
   - Sandbox sees its own PID 1
   - Host processes are invisible to sandbox

2. **Mount Table Isolation**
   - Sandbox has its own mount namespace
   - Host mount points are not visible to sandbox
   - Basic rootfs setup begins (minimal directory structure)

### What Is NOT Protected (Week 1)

1. **Privilege Escalation**: No user namespace, no capability dropping, no `PR_SET_NO_NEW_PRIVS`
   - **Risk**: Processes may run as root on host if not carefully managed
   - **Mitigation**: Run sandboxd as non-root user, or ensure proper setup (Week 2 addresses this)

2. **Resource Limits**: No cgroups v2 integration
   - **Risk**: Fork bombs, memory exhaustion, CPU spinning
   - **Mitigation**: Run sandboxd with appropriate system limits, or limit concurrent sandboxes

3. **Syscall Filtering**: No seccomp-bpf
   - **Risk**: Dangerous syscalls are not blocked
   - **Mitigation**: Limit what commands are allowed to run

4. **Network Isolation**: No network namespace
   - **Risk**: Sandbox can access host network
   - **Mitigation**: Use firewall rules, or limit network access at system level

5. **Complete Filesystem Isolation**: Minimal rootfs only
   - **Risk**: May have access to more host filesystem than intended
   - **Mitigation**: Careful rootfs preparation, read-only mounts where possible

### Week 1 Attack Vectors

Given the limited scope, potential attack vectors in Week 1 include:

1. **Mount Namespace Escape**
   - **Attack**: Attempting to remount host filesystems or access host mounts
   - **Mitigation**: Proper mount namespace isolation, but no explicit blocking of `mount()` syscall yet

2. **PID Namespace Confusion**
   - **Attack**: Attempting to use `/proc` to access host processes (should fail due to isolation, but worth testing)
   - **Mitigation**: PID namespace isolation prevents this

3. **Resource Exhaustion**
   - **Attack**: Fork bombs, memory allocation, CPU spinning
   - **Mitigation**: None in Week 1 - must rely on system-level limits

4. **Privilege Escalation Attempts**
   - **Attack**: Using setuid binaries, attempting to mount filesystems
   - **Mitigation**: None in Week 1 - must run sandboxd carefully

5. **Network-Based Attacks**
   - **Attack**: Connecting to external servers, scanning networks
   - **Mitigation**: None in Week 1 - must use firewall rules

### Week 1 Security Recommendations

1. **Run sandboxd as non-root user**: Even without user namespace, running as non-root limits damage
2. **Limit concurrent sandboxes**: Prevent resource exhaustion
3. **Use system-level resource limits**: Set `ulimit` or systemd limits
4. **Firewall rules**: Block network access if needed
5. **Careful rootfs preparation**: Ensure minimal, read-only access to host filesystem
6. **Test isolation**: Verify PID and mount namespace isolation works as expected

---

## Security Boundaries: Full Project Scope

As the project progresses through all 8 weeks, the security boundaries expand:

- **Week 2**: User namespace + privilege dropping (addresses privilege escalation)
- **Week 3**: Complete filesystem isolation (addresses filesystem access)
- **Week 4**: Resource limits via cgroups v2 (addresses resource exhaustion)
- **Week 5**: Syscall filtering via seccomp-bpf (addresses syscall abuse)
- **Week 6**: Network isolation (addresses network access)
- **Week 7**: Hardening and escape test suite (validates all mitigations)
- **Week 8**: Polish and documentation

---

## Attack Surface Analysis

### Entry Points

1. **CLI (`sbx run`)**: Command-line interface that accepts commands and policy files
   - **Risk**: Malformed input, path traversal in policy file paths
   - **Mitigation**: Input validation, path sanitization, policy file validation

2. **Policy File Parsing**: YAML policy files define security boundaries
   - **Risk**: Malformed policies, policy injection, path traversal
   - **Mitigation**: Strict YAML parsing, path validation, policy schema validation

3. **Command Execution**: The actual command executed inside the sandbox
   - **Risk**: Command injection, malicious binaries, script execution
   - **Mitigation**: Proper argument handling, seccomp filtering, filesystem isolation

### Trust Boundaries

1. **Host System → sandboxd**: We trust the host system and the user running sandboxd
2. **sandboxd → Sandbox**: sandboxd creates and manages sandboxes (trusted)
3. **Sandbox → Host**: Sandboxed processes are untrusted and must be isolated

### Assumptions

1. **Kernel is secure and up-to-date**: We rely on kernel security mechanisms
2. **Host system is not compromised**: We assume the host is trusted
3. **User running sandboxd is trusted**: The user has appropriate permissions
4. **Policy files are trusted**: Policy files come from trusted sources
5. **Rootfs is prepared correctly**: The rootfs directory is set up as intended

---

## Threat Prioritization

### High Priority (Must Defend Against)

1. **Process escape**: Core isolation mechanism
2. **Privilege escalation**: Critical for security
3. **Resource exhaustion**: Prevents DoS
4. **Filesystem access**: Protects host data

### Medium Priority (Important but Can Be Phased)

1. **Syscall abuse**: Important but can start with basic allowlist
2. **Network access**: Important but can be disabled by default

### Low Priority (Nice to Have)

1. **Side-channel mitigations**: Some syscall blocking helps, but not primary focus
2. **Advanced path-based access control**: Stretch goal (Landlock or seccomp user-notify)

---

## Validation and Testing

To validate our threat model, we implement:

1. **Escape Test Suite** (Week 7): Tests for common container escape techniques
2. **Resource Limit Tests**: Verify cgroups enforcement
3. **Syscall Policy Tests**: Verify seccomp filtering
4. **Filesystem Isolation Tests**: Verify mount namespace isolation
5. **Network Isolation Tests**: Verify network namespace isolation
6. **Fuzzing**: Fuzz policy parser and launcher at API boundaries

---

## Conclusion

This threat model defines a clear security boundary: we defend against process escape, privilege escalation, resource exhaustion, syscall abuse, filesystem access, and network access through Linux namespaces and security primitives. We explicitly do not defend against kernel exploits, hardware attacks, or side-channel attacks, and we are not a full container runtime.

The security model is built incrementally, with Week 1 establishing basic isolation foundations, and subsequent weeks adding layers of defense-in-depth.

---

## References

- Linux namespaces: `man 7 namespaces`
- PID namespaces: `man 7 pid_namespaces`
- Mount namespaces: `man 7 mount_namespaces`
- User namespaces: `man 7 user_namespaces`
- seccomp: `man 2 seccomp`
- cgroups v2: https://www.kernel.org/doc/Documentation/cgroup-v2.txt

