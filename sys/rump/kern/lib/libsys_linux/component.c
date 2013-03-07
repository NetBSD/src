/*	$NetBSD: component.c,v 1.4 2013/03/07 18:53:40 pooka Exp $	*/

#include <sys/param.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

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

RUMP_COMPONENT(RUMP_COMPONENT_KERN)
{
	extern struct emul *emul_default;

	emul_default = &emul_rump_sys_linux;
}

/*
 * XXX: the linux emulation code is not split into factions
 */
void rumplinux__stub(void);
void rumplinux__stub(void) {panic("unavailable");}

/* timing */
__weak_alias(clock_gettime1,rumplinux__stub);
__weak_alias(clock_settime1,rumplinux__stub);
__weak_alias(clock_getres1,rumplinux__stub);
__weak_alias(compat_50_sys_gettimeofday,rumplinux__stub);
__weak_alias(compat_50_sys_settimeofday,rumplinux__stub);
__weak_alias(nanosleep1,rumplinux__stub);

/* vm-related */
__weak_alias(sys_mmap,rumplinux__stub);
__weak_alias(vm_map_unlock,rumplinux__stub);
__weak_alias(uvm_map_lookup_entry,rumplinux__stub);
__weak_alias(sys_obreak,rumplinux__stub);
__weak_alias(sys_swapctl,rumplinux__stub);
__weak_alias(vm_map_lock,rumplinux__stub);
__weak_alias(uvm_mremap,rumplinux__stub);

/* signal.c */
__weak_alias(sigaction1,rumplinux__stub);
__weak_alias(kpsignal2,rumplinux__stub);
__weak_alias(sys_kill,rumplinux__stub);
__weak_alias(sigsuspend1,rumplinux__stub);
__weak_alias(sigtimedwait1,rumplinux__stub);
__weak_alias(lwp_find,rumplinux__stub);

/* misc */
__weak_alias(linux_machdepioctl,rumplinux__stub);
__weak_alias(linux_ioctl_sg,rumplinux__stub);
__weak_alias(oss_ioctl_mixer,rumplinux__stub);
__weak_alias(oss_ioctl_sequencer,rumplinux__stub);
__weak_alias(oss_ioctl_audio,rumplinux__stub);
__weak_alias(rusage_to_rusage50,rumplinux__stub);
__weak_alias(do_sys_wait,rumplinux__stub);
