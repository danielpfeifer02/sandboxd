# Week 1: Skeleton + Threat Model + Minimal Isolate

## Goal
Establish basic process isolation by implementing the foundational CLI and sandbox creation mechanism using Linux namespaces.

## Objectives
- Create project structure and build system
- Implement basic `sbx run` CLI command
- Spawn isolated child process using `clone()`/`clone3()`
- Unshare mount and PID namespaces
- Set up minimal rootfs (prepared directory)
- Verify isolation through acceptance tests

---

## Part-by-Part Breakdown

### Part 1: Project Setup & Threat Model Documentation

- [x] Initialize project structure:
  - [x] Create directory layout: `daemon/`, `cli/`, `sandbox/`, `tests/`, `docs/`, `examples/`
  - [x] Set up build system (CMake or Makefile)
  - [x] Configure C++ compiler flags (C++23, warnings enabled)
  - [x] Create basic project configuration files
- [ ] Write threat model document (`docs/THREAT_MODEL.md`):
  - [ ] Document what we defend against (process escape, privilege escalation, resource exhaustion, syscall abuse, filesystem access, network access)
  - [ ] Document non-goals (kernel exploits, hardware attacks, side-channel attacks, full container runtime)
  - [ ] Define security boundaries for Week 1 scope
  - [ ] Identify potential attack vectors for basic isolation

**Deliverables:**
- Project structure in place
- Build system functional (can compile empty main)
- Threat model document completed

---

### Part 2: CLI Skeleton Implementation

- [ ] Implement basic CLI framework:
  - [ ] Choose CLI library (e.g., CLI11, argparse, or manual parsing)
  - [ ] Create `sbx` command entry point
  - [ ] Implement `sbx run` subcommand structure
  - [ ] Add basic argument parsing (command to run, optional policy file)
  - [ ] Add help text and usage information
- [ ] Implement CLI error handling:
  - [ ] Input validation
  - [ ] Error messages and exit codes
  - [ ] Basic logging infrastructure (structured logs with sandbox ID)
- [ ] Create minimal test for CLI:
  - [ ] Test help output
  - [ ] Test argument parsing
  - [ ] Test error cases

**Deliverables:**
- `sbx run` command accepts arguments
- CLI prints help and handles errors gracefully
- Basic CLI tests pass

---

### Part 3: Process Spawning with clone()

- [ ] Research and understand `clone()` vs `clone3()`:
  - [ ] Review Linux man pages and documentation
  - [ ] Understand required flags for namespace creation
  - [ ] Plan namespace creation order
- [ ] Implement child process spawning:
  - [ ] Create wrapper function for `clone()` or `clone3()`
  - [ ] Set up child function that will execute the user command
  - [ ] Handle child process setup and teardown
- [ ] Implement PID namespace isolation:
  - [ ] Use `CLONE_NEWPID` flag
  - [ ] Verify child sees its own PID namespace
  - [ ] Ensure proper PID 1 behavior in child
- [ ] Implement mount namespace isolation:
  - [ ] Use `CLONE_NEWNS` flag (mount namespace)
  - [ ] Verify mount table isolation
  - [ ] Test that child cannot see host mounts

**Deliverables:**
- Child process spawned with `clone()`/`clone3()`
- PID namespace isolation working
- Mount namespace isolation working
- Basic test: child reports PID 1, mount table is isolated

---

### Part 4: Minimal Rootfs Setup

- [ ] Design minimal rootfs structure:
  - [ ] Plan directory layout (minimal `/bin`, `/usr`, `/lib`, `/lib64`, `/tmp`, `/proc`, `/sys`)
  - [ ] Document what binaries/libraries are needed for basic commands
  - [ ] Create rootfs preparation script or manual instructions
- [ ] Implement rootfs preparation:
  - [ ] Create function to prepare minimal rootfs directory
  - [ ] Copy necessary binaries and libraries (or use bind mounts initially)
  - [ ] Set up basic directory structure
- [ ] Implement rootfs mounting in child:
  - [ ] Use `pivot_root()` or `chroot()` for initial implementation
  - [ ] Set up basic bind mounts for `/usr`, `/lib`, `/lib64` (read-only)
  - [ ] Create `/tmp` as tmpfs (writable)
  - [ ] Ensure `/proc` and `/sys` are mounted if needed

**Deliverables:**
- Minimal rootfs directory structure created
- Rootfs mounted in child process
- Child can execute basic commands (e.g., `ls`, `echo`)
- Test: child cannot access host filesystem outside rootfs

---

### Part 5: Integration & Acceptance Tests

- [ ] Integrate all components:
  - [ ] Connect CLI → process spawning → namespace setup → rootfs mounting
  - [ ] Ensure proper error handling throughout the chain
  - [ ] Add logging at each stage (sandbox ID, setup steps)
- [ ] Fix integration issues:
  - [ ] Debug namespace creation order
  - [ ] Fix mount propagation issues
  - [ ] Ensure proper cleanup on failure
- [ ] Implement acceptance tests:
  - [ ] **Test 1**: Command runs successfully
    - [ ] Run simple command (e.g., `sbx run echo "hello"`)
    - [ ] Verify output is captured and displayed
    - [ ] Verify exit code is correct
  - [ ] **Test 2**: Sandbox sees its own PID 1
    - [ ] Run `sbx run ps aux` or `sbx run cat /proc/self/pid`
    - [ ] Verify PID is 1 (or isolated namespace)
    - [ ] Verify cannot see host processes
  - [ ] **Test 3**: Mount table is isolated from host
    - [ ] Run `sbx run mount` or `sbx run cat /proc/mounts`
    - [ ] Verify mount table shows only sandbox mounts
    - [ ] Verify cannot see host mount points
- [ ] Write test automation:
  - [ ] Create test runner script
  - [ ] Ensure tests are deterministic
  - [ ] Document how to run tests

**Deliverables:**
- All components integrated and working
- All three acceptance tests pass
- Test suite is runnable and documented

---

## Technical Details

### Namespace Creation Order
1. Create mount namespace first (to isolate filesystem operations)
2. Create PID namespace (to isolate process tree)
3. Set up rootfs and mounts
4. Enter PID namespace (fork or setns)

### Key Linux System Calls
- `clone()` or `clone3()`: Create child with namespaces
- `unshare()`: Alternative approach (create namespaces in current process)
- `pivot_root()`: Change root filesystem
- `mount()`: Set up bind mounts and tmpfs
- `setns()`: Join existing namespace (if needed)

### Minimal Rootfs Contents
- Basic shell (`/bin/sh` or `/bin/bash`)
- Core utilities (`ls`, `echo`, `cat`, `ps`, `mount`)
- Essential libraries (`libc.so`, `ld-linux.so`)
- Directory structure: `/bin`, `/usr`, `/lib`, `/lib64`, `/tmp`, `/proc`, `/sys`

### Error Handling
- Check return values of all system calls
- Provide meaningful error messages
- Clean up resources on failure (unmount, kill processes)
- Return appropriate exit codes

### Logging
- Structured logs with fields:
  - Sandbox ID (unique identifier)
  - Timestamp
  - Log level
  - Message
- Log key events:
  - Sandbox creation
  - Namespace setup
  - Rootfs mounting
  - Command execution start/end
  - Errors and failures

---

## Success Criteria

### Must Have (Acceptance Tests)
1. **Command runs successfully**
   - `sbx run echo "hello"` executes and prints "hello"
   - Exit code is 0 for successful commands
   - Exit code is non-zero for failed commands

2. **Sandbox sees its own PID 1**
   - `sbx run ps aux` shows processes starting from PID 1
   - `sbx run cat /proc/self/pid` shows PID 1 (or isolated namespace)
   - Cannot see host processes (e.g., `ps aux` doesn't show host processes)

3. **Mount table is isolated from host**
   - `sbx run mount` shows only sandbox mounts
   - `sbx run cat /proc/mounts` shows isolated mount namespace
   - Cannot see host mount points

### Nice to Have
- Basic logging infrastructure
- Helpful error messages
- Test suite automation
- Documentation for running tests

---

## Potential Challenges & Mitigations

### Challenge 1: Namespace Creation Order
**Problem**: Incorrect order can cause mount propagation issues or PID namespace problems.

**Mitigation**: 
- Research best practices for namespace ordering
- Test each namespace individually before combining
- Use `MS_PRIVATE` or `MS_SLAVE` for mount propagation

### Challenge 2: Rootfs Preparation
**Problem**: Creating a minimal rootfs with all necessary binaries and libraries can be complex.

**Mitigation**:
- Start with bind mounts to host directories (read-only)
- Gradually reduce to minimal set
- Use existing tools (e.g., `debootstrap` for reference) or manual copying

### Challenge 3: PID Namespace Behavior
**Problem**: PID namespace behavior can be tricky, especially with `CLONE_NEWPID`.

**Mitigation**:
- Understand that `CLONE_NEWPID` only affects children, not the calling process
- May need to fork twice (once to enter PID namespace, once to run command)
- Test PID visibility carefully

### Challenge 4: Mount Propagation
**Problem**: Mount events can propagate to parent namespace if not handled correctly.

**Mitigation**:
- Use `MS_PRIVATE` or `MS_SLAVE` mount propagation flags
- Set mount propagation before creating mounts
- Test that host mounts are not visible in sandbox

---

## Resources & References

### Linux Documentation
- `man 2 clone` - Process creation with namespaces
- `man 2 unshare` - Unshare namespaces
- `man 2 pivot_root` - Change root filesystem
- `man 2 mount` - Mount filesystems
- `man 7 namespaces` - Overview of Linux namespaces
- `man 7 pid_namespaces` - PID namespace details
- `man 7 mount_namespaces` - Mount namespace details

### Code Examples
- Linux kernel source: `kernel/fork.c` (clone implementation)
- systemd source: namespace handling
- runc/containerd: container runtime examples (reference only)

### Testing Tools
- `strace` - Trace system calls
- `lsns` - List namespaces
- `nsenter` - Enter namespaces for debugging

---

## End of Week Checklist

- [x] Project structure created and organized
- [x] Build system functional
- [ ] Threat model document written
- [ ] `sbx run` CLI implemented and tested
- [ ] Child process spawning with `clone()` working
- [ ] PID namespace isolation verified
- [ ] Mount namespace isolation verified
- [ ] Minimal rootfs setup and mounting working
- [ ] All three acceptance tests passing
- [ ] Test suite documented and runnable
- [ ] Basic logging infrastructure in place
- [ ] Code is clean and well-commented
- [ ] Ready to proceed to Week 2 (User Namespace + Privilege Dropping)

---

## Notes

- Focus on correctness over features this week
- Keep implementation simple and understandable
- Document any deviations from the plan
- Test frequently as you build each component
- Don't worry about performance optimization yet (that comes later)

