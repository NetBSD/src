# NetBSD/usermode Boot Process

**Platform:** usermode (User-mode NetBSD)
**Architecture:** Host architecture (runs as userspace process)
**Location:** `/sys/arch/usermode/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/usermode runs NetBSD as a userspace process on top of another NetBSD system, similar to User-Mode Linux.

### Purpose

- **Development:** Kernel development and testing
- **Debugging:** Easy kernel debugging
- **Virtualization:** Lightweight virtualization
- **Education:** Learning kernel internals

---

## Boot Sequence

```
Host NetBSD → netbsd-USERMODE binary → NetBSD usermode kernel
```

### Starting Usermode NetBSD

```bash
# Run the usermode kernel
$ ./netbsd-USERMODE

# With options
$ ./netbsd-USERMODE -r /path/to/rootfs
```

---

## Features

### Architecture

- **No real hardware:** Uses host system calls
- **Virtual devices:** Block, network devices via host
- **Debugging:** Can use gdb on kernel

### System Calls

Usermode kernel translates NetBSD syscalls to host syscalls.

### Memory

Uses host process memory, no actual MMU control.

---

## Use Cases

**Development:**
- Test kernel changes without rebooting
- Fast iteration cycle
- Safe experimentation

**Debugging:**
- Use standard debugging tools (gdb)
- No need for serial console
- Easy crash dumps

---

## Limitations

- **Performance:** Slower than native
- **Hardware:** No actual hardware access
- **Drivers:** Limited to virtual devices

---

## References

- **NetBSD Usermode Documentation**
- NetBSD source: `/sys/arch/usermode/`

---

**END OF DOCUMENT**
