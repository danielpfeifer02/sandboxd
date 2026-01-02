# sandboxd: Hermetic Sandbox Runner

## Project Overview

**sandboxd** is a secure, hermetic sandbox runner built with C++ and Linux primitives. It provides a daemon + CLI architecture for executing untrusted commands in a highly restricted environment using multiple layers of Linux security mechanisms.

### Core Components

1. **sandboxd** (daemon): Creates and manages sandboxes, tracks lifecycle, kills on timeout/OOM
2. **sbx run** (CLI): Submits jobs, streams stdout/stderr, returns structured results
3. **Policy format** (JSON/TOML/YAML): Declarative configuration describing security boundaries

## Architecture

### Security Layers

The sandbox implements defense-in-depth through multiple Linux security primitives:

#### 1. Process Isolation
- **PID namespace**: Isolated process tree (sandbox sees its own PID 1)
- **Mount namespace**: Isolated filesystem view
- **UTS namespace**: Isolated hostname/domainname
- **IPC namespace**: Isolated System V IPC and POSIX message queues
- **User namespace**: Rootless model (root inside, non-root on host)
- **Capability dropping**: Remove unnecessary capabilities
- **PR_SET_NO_NEW_PRIVS**: Prevent privilege escalation

#### 2. Filesystem Isolation
- **pivot_root**: Minimal rootfs, isolated from host
- **Read-only root**: Default read-only filesystem
- **Bind mount allowlist**: Controlled access to host paths (e.g., `/usr`, `/lib`, `/lib64` - read-only)
- **tmpfs**: Writable `/tmp` directory
- **Optional overlayfs**: Writable layer for specific directories

#### 3. Resource Isolation (cgroups v2)
- **memory.max**: Hard memory limit
- **cpu.max**: CPU bandwidth limit
- **pids.max**: Process count limit
- **io.max**: I/O bandwidth limit (optional)
- **Pressure metrics**: Monitor resource pressure (optional)
- **Wall-clock timeout**: Strict timeout with process tree kill

#### 4. Syscall Isolation (seccomp-bpf)
- **Allowlist-based**: Only permitted syscalls allowed
- **Profile-based**: Different profiles for different use cases (e.g., "compile", "run", "python")
- **Sensible defaults**: Blocks risky syscalls (keyctl, bpf, ptrace, mount, kexec, raw sockets)

#### 5. Network Isolation
- **Network namespace**: Isolated network stack
- **Default: no network**: No interfaces by default
- **Optional allowlist**: Controlled networking (stretch goal)

### Observability

- **Structured logs**: Sandbox ID, exit reason (normal/timeout/OOM/seccomp)
- **Metrics**: Setup latency, runtime, RSS peak, syscall violation count
- **Lifecycle tracking**: Complete audit trail of sandbox execution

## Policy Format

The policy format (JSON/TOML/YAML) describes:

```yaml
# Example policy structure
mounts:
  allowed:
    - source: /usr
      target: /usr
      readonly: true
    - source: /lib
      target: /lib
      readonly: true
  writable_dirs:
    - /tmp  # tmpfs
    - /workspace  # optional overlay

network:
  enabled: false  # default: none
  allowlist: []   # optional

resources:
  memory_max: "512M"
  cpu_max: "1.0"
  pids_max: 100
  io_max: "10M"  # optional
  timeout: "30s"

syscall_policy:
  profile: "default"  # or "compile", "run", "python", etc.
  mode: "allowlist"   # or "user-notify" for advanced mode
```

## Stretch Goal: Path-Aware File Access Control

### Option A: seccomp user-notify broker (Advanced)
- Use seccomp's user notification mechanism
- Intercept `open`/`openat` syscalls
- Enforce path allowlist in broker process
- Higher complexity, high payoff

### Option B: Landlock (Simpler)
- Apply Landlock rules for path-based restrictions
- Defense-in-depth alongside namespaces
- Clean conceptual model

## 8-Week Execution Plan

### Week 1: Skeleton + Threat Model + Minimal Isolate
**Goal**: Basic process isolation

**Deliverables**:
- Implement `sbx run` CLI
- Spawn child with `clone()`/`clone3()`
- Unshare mount + PID namespace
- Set up minimal rootfs (prepared directory)

**Acceptance Tests**:
- Command runs successfully
- Sandbox sees its own PID 1
- Mount table is isolated from host

### Week 2: User Namespace + Privilege Dropping (Rootless)
**Goal**: Rootless security model

**Deliverables**:
- User namespace mapping
- Drop capabilities
- Set `PR_SET_NO_NEW_PRIVS`
- Forbid setuid and privilege escalation

**Acceptance Tests**:
- Inside sandbox: `id` shows non-root (or root inside userns but not on host)
- Attempts to mount fail
- Attempts to ptrace host processes fail

### Week 3: Filesystem Policy (pivot_root, RO root, tmpfs)
**Goal**: Complete filesystem isolation

**Deliverables**:
- `pivot_root` into minimal filesystem
- Bind mounts allowlist (read-only)
- tmpfs for `/tmp`
- Optional "workspace" mount (RW) to specific directory

**Acceptance Tests**:
- Cannot read `/etc/shadow` from host
- Can write to `/tmp`
- Cannot write to `/usr` or other RO mounts

### Week 4: cgroups v2 + Timeouts + Kill-Tree Correctness
**Goal**: Resource limits and timeout handling

**Deliverables**:
- Create cgroup per sandbox
- Enforce memory/cpu/pids limits
- Implement watchdog timeout
- Robust kill process tree

**Acceptance Tests**:
- Fork bomb hits `pids.max` and is contained
- Memory hog hits `memory.max` and is killed; returns "OOM"
- Timeout kills all descendants deterministically

### Week 5: seccomp Allowlist v1 (Pragmatic)
**Goal**: Syscall filtering

**Deliverables**:
- Baseline allowlist for typical ELF binaries
- Per-profile filters (e.g., "compile", "run", "python")
- Block risky syscalls

**Acceptance Tests**:
- Blocked syscall returns EPERM (or kills, depending on mode)
- Verify `bpf`, `keyctl`, `perf_event_open`, `ptrace` blocked

### Week 6: Network Isolation
**Goal**: Network namespace isolation

**Deliverables**:
- Network namespace with no interfaces by default
- Optionally allow controlled networking (stretch)

**Acceptance Tests**:
- Cannot connect outbound (`curl` fails)
- No accidental host network access

### Week 7: Hardening + Escape Test Suite
**Goal**: Security validation

**Deliverables**:
- Tests for common container escapes:
  - Mount tricks
  - procfs tricks
  - `setns` attempts
  - ptrace attempts
  - Raw sockets
- Fuzzing at API boundary (policy parser + launcher)
- Property tests for setup invariants

**Acceptance Tests**:
- Test suite is deterministic and CI-friendly
- All escape attempts are blocked

### Week 8: Polish: Docs, Benchmarks, Demo
**Goal**: Production readiness

**Deliverables**:
- Threat model document: what we defend against / what we don't
- Performance numbers: setup latency, overhead vs native run
- "How to extend policy" guide
- One-command demo

**Acceptance Tests**:
- One-command demo reproduces results
- Everything runnable on a clean machine

## End-to-End Test Suite

The test suite includes:

1. **Regression tests**: Verify core functionality
2. **Escape attempts**: Test containment against known attack vectors
3. **Resource limit tests**: Verify cgroup enforcement
4. **Syscall policy tests**: Verify seccomp filtering
5. **Filesystem isolation tests**: Verify mount namespace isolation
6. **Network isolation tests**: Verify network namespace isolation

## Threat Model

### What We Defend Against

- **Process escape**: Isolation via namespaces
- **Privilege escalation**: User namespace + capability dropping + no_new_privs
- **Resource exhaustion**: cgroups v2 limits
- **Syscall abuse**: seccomp allowlist
- **Filesystem access**: Mount namespace + pivot_root + read-only root
- **Network access**: Network namespace (default: none)

### Non-Goals

- **Kernel exploits**: We assume a non-compromised kernel
- **Hardware attacks**: Out of scope
- **Side-channel attacks**: Not a focus (though some mitigations may help)
- **Full container runtime**: We're a sandbox runner, not a full container system

## Correctness Pitfalls to Handle

1. **Mount propagation**: Correct handling of MS_PRIVATE/MS_SLAVE
2. **Zombie reaping**: Proper cleanup of child processes
3. **Cgroup cleanup**: Ensure cgroups are removed after sandbox termination
4. **Signal handling**: Proper signal propagation and cleanup
5. **Namespace ordering**: Correct order of namespace creation
6. **Capability inheritance**: Ensure capabilities don't leak

## Performance Considerations

- **Cold-start latency**: Measure and optimize setup time
- **Overhead sources**: Identify and document performance impact
- **Resource usage**: Monitor daemon resource consumption

## Optional Companion Project

**seccomp profile generator**:
- Run a workload under tracing (ptrace or seccomp logging)
- Collect syscalls
- Generate candidate allowlist
- Minimize the allowlist
- Highly relevant for maintaining sandbox policies at scale

## Final Write-Up Highlights

What interviewers value:

1. **Clear threat model and non-goals**: Explicit about what we protect against
2. **Correctness pitfalls handled**: Documented solutions to tricky problems
3. **Minimal but defensible syscall policy**: Practical and secure defaults
4. **Reproducible tests**: Demonstrate containment effectively
5. **Performance discussion**: Cold-start latency and overhead analysis

## Technology Stack

- **Language**: C++ (for performance and system-level access)
- **Platform**: Linux (kernel 5.8+ for cgroups v2, modern seccomp features)
- **Dependencies**: 
  - Linux namespaces (via `clone()`, `unshare()`, `setns()`)
  - cgroups v2 (via filesystem interface)
  - seccomp-bpf (via libseccomp or direct syscalls)
  - Optional: Landlock (kernel 5.13+)

## Project Structure (Planned)

```
sandboxd/
├── daemon/          # sandboxd daemon implementation
├── cli/             # sbx CLI implementation
├── policy/          # Policy parser and validator
├── sandbox/         # Core sandbox creation and management
├── cgroups/         # cgroups v2 integration
├── seccomp/         # seccomp-bpf policy management
├── tests/           # Test suite (regression + escape attempts)
├── docs/            # Documentation
└── examples/        # Example policies and demos
```

## Success Criteria

1. ✅ Can run untrusted commands in isolated environment
2. ✅ Resource limits enforced (memory, CPU, PIDs)
3. ✅ Filesystem isolation prevents host access
4. ✅ Syscall filtering blocks dangerous operations
5. ✅ Escape test suite passes
6. ✅ Performance overhead is acceptable (< 50ms setup latency)
7. ✅ Clear documentation and reproducible demos
