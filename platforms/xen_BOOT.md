# NetBSD/xen Boot Process

**Platform:** xen (Xen Hypervisor)
**Architecture:** x86 (i386/amd64) and ARM
**Location:** `/sys/arch/xen/`
**Version:** 1.0
**Last Updated:** 2025-11-12

---

## Overview

NetBSD/xen runs as a paravirtualized guest (domU) or privileged domain (dom0) under the Xen hypervisor.

### Supported Configurations

- **DomU (Guest):** Paravirtualized guest domain
- **Dom0 (Privileged):** Control domain with hardware access
- **x86 (i386/amd64):** Primary platforms
- **ARM:** ARM-based Xen systems

---

## Boot Sequence

### DomU Boot

```
Xen Hypervisor → Domain Config → NetBSD/Xen Kernel
```

**Domain Configuration:**
```python
# /etc/xen/netbsd-domU.cfg
name = "netbsd-guest"
kernel = "/usr/pkg/share/examples/xen/netbsd-XEN3_DOMU.gz"
memory = 512
disk = [ 'phy:/dev/wd0e,0x1,w' ]
vif = [ 'bridge=bridge0' ]
```

**Starting Domain:**
```
# xl create /etc/xen/netbsd-domU.cfg
# xl console netbsd-guest
```

### Dom0 Boot

```
Boot Loader → Xen Hypervisor → NetBSD Dom0 Kernel
```

**GRUB Configuration:**
```
menuentry 'NetBSD-Xen' {
    multiboot /xen.gz dom0_mem=512M
    module /netbsd-XEN3_DOM0.gz root=wd0a
}
```

---

## Memory Management

### Paravirtualization

- **No real page tables:** Uses Xen's virtualized MMU
- **Hypercalls:** System calls to hypervisor
- **Grant tables:** Shared memory between domains
- **Event channels:** Inter-domain communication

### Memory Layout (DomU)

```
0x00000000 - 0xBFFFFFFF  Guest pseudo-physical memory
0xC0000000 - 0xFFFFFFFF  Hypervisor reserved
```

---

## Xen Devices

### Virtual Block Devices (VBD)

```
# xbdN - Virtual block device
# Backend: File, LVM, physical partition
```

### Virtual Network Interfaces (VIF)

```
# xennetN - Virtual network interface
# Backend: Bridge, NAT, routed
```

### Console

```
# xencons - Xen console
# Access via 'xl console' command
```

---

## Hypercalls

Communication with Xen hypervisor:

```c
/* Common hypercalls */
#define __HYPERVISOR_set_trap_table        0
#define __HYPERVISOR_mmu_update            1
#define __HYPERVISOR_set_gdt               2
#define __HYPERVISOR_stack_switch          3
#define __HYPERVISOR_set_callbacks         4
#define __HYPERVISOR_fpu_taskswitch        5
#define __HYPERVISOR_sched_op_compat       6
#define __HYPERVISOR_dom0_op               7
#define __HYPERVISOR_set_debugreg          8
#define __HYPERVISOR_get_debugreg          9
#define __HYPERVISOR_update_descriptor    10
#define __HYPERVISOR_memory_op            12
#define __HYPERVISOR_multicall            13
#define __HYPERVISOR_update_va_mapping    14
#define __HYPERVISOR_event_channel_op     16
#define __HYPERVISOR_xen_version          17
#define __HYPERVISOR_console_io           18
#define __HYPERVISOR_grant_table_op       20
```

---

## Management Commands

**Domain Management:**
```
# xl create <config>              Create domain
# xl destroy <domain>             Destroy domain
# xl console <domain>             Attach console
# xl list                         List domains
# xl info                         Show host info
```

**Resource Management:**
```
# xl mem-set <domain> <mem>       Set memory
# xl vcpu-set <domain> <vcpus>    Set VCPUs
```

---

## Troubleshooting

### Common Issues

**Problem:** Domain won't start
**Solutions:**
- Check config file syntax
- Verify kernel path exists
- Check available memory
- Review Xen logs: `/var/log/xen/`

**Problem:** No network connectivity
**Solutions:**
- Verify bridge configuration
- Check vif settings in config
- Ensure dom0 has network

**Problem:** Disk not accessible
**Solutions:**
- Check disk path in config
- Verify permissions on block device
- Check Xen block backend

---

## References

- **Xen Project Documentation**
- **Xen Interface Manual**
- **NetBSD Xen HowTo**
- NetBSD source: `/sys/arch/xen/`

---

**END OF DOCUMENT**
