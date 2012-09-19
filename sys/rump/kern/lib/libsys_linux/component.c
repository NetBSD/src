/*	$NetBSD: component.c,v 1.1 2012/09/19 21:45:40 pooka Exp $	*/

#include <sys/param.h>
#include <sys/proc.h>

#include "rump_private.h"

extern struct sysent rump_linux_sysent[];

struct emul emul_rump_sys_linux = {
	.e_name = "linux-rump",
	.e_sysent = rump_linux_sysent,
	.e_vm_default_addr = uvm_default_mapaddr,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern = syscall_intern,
#endif
};

/*
 * XXX: the linux emulation code is not split into factions
 */
void rumplinux__stub(void);
void rumplinux__stub(void) {panic("unavailable");}
__weak_alias(rumpns_linux_machdepioctl,rumplinux__stub);
__weak_alias(rumpns_nanosleep1,rumplinux__stub);
__weak_alias(rumpns_vm_map_unlock,rumplinux__stub);
__weak_alias(rumpns_compat_50_sys_settimeofday,rumplinux__stub);
__weak_alias(rumpns_oss_ioctl_mixer,rumplinux__stub);
__weak_alias(rumpns_linux_ioctl_sg,rumplinux__stub);
__weak_alias(rumpns_oss_ioctl_sequencer,rumplinux__stub);
__weak_alias(rumpns_uvm_mremap,rumplinux__stub);
__weak_alias(rumpns_sysent,rumplinux__stub);
__weak_alias(rumpns_sys_swapctl,rumplinux__stub);
__weak_alias(rumpns_vm_map_lock,rumplinux__stub);
__weak_alias(rumpns_compat_50_sys_gettimeofday,rumplinux__stub);
__weak_alias(rumpns_rusage_to_rusage50,rumplinux__stub);
__weak_alias(rumpns_sys_obreak,rumplinux__stub);
__weak_alias(rumpns_do_sys_wait,rumplinux__stub);
__weak_alias(rumpns_sys_mmap,rumplinux__stub);
__weak_alias(rumpns_oss_ioctl_audio,rumplinux__stub);
__weak_alias(rumpns_clock_gettime1,rumplinux__stub);
__weak_alias(rumpns_uvm_map_lookup_entry,rumplinux__stub);
__weak_alias(rumpns_clock_settime1,rumplinux__stub);
__weak_alias(rumpns_clock_getres1,rumplinux__stub);
