/*	$NetBSD: emul.c,v 1.152 2013/01/20 17:09:47 rmind Exp $	*/

/*
 * Copyright (c) 2007-2011 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: emul.c,v 1.152 2013/01/20 17:09:47 rmind Exp $");

#include <sys/param.h>
#include <sys/null.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/syslog.h>
#include <sys/namei.h>
#include <sys/kauth.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/file.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/timetc.h>
#include <sys/tprintf.h>
#include <sys/module.h>
#include <sys/tty.h>
#include <sys/reboot.h>
#include <sys/syscallvar.h>
#include <sys/xcall.h>
#include <sys/sleepq.h>

#include <dev/cons.h>

#include <rump/rumpuser.h>

#include <uvm/uvm_map.h>

#include "rump_private.h"

/*
 * physmem is largely unused (except for nmbcluster calculations),
 * so pick a default value which suits ZFS.  if an application wants
 * a very small memory footprint, it can still adjust this before
 * calling rump_init()
 */
#define PHYSMEM 512*256
int physmem = PHYSMEM;
int nkmempages = PHYSMEM/2; /* from le chapeau */
#undef PHYSMEM

struct lwp lwp0;
struct vnode *rootvp;
dev_t rootdev = NODEV;

const int schedppq = 1;
int hardclock_ticks;
bool mp_online = false;
struct timeval boottime;
int cold = 1;
int boothowto = AB_SILENT;
struct tty *constty;

const struct bdevsw *bdevsw0[255];
const struct bdevsw **bdevsw = bdevsw0;
const int sys_cdevsws = 255;
int max_cdevsws = 255;

const struct cdevsw *cdevsw0[255];
const struct cdevsw **cdevsw = cdevsw0;
const int sys_bdevsws = 255;
int max_bdevsws = 255;

int mem_no = 2;

device_t booted_device;
device_t booted_wedge;
int booted_partition;

/* XXX: unused */
kmutex_t tty_lock;
krwlock_t exec_lock;

struct lwplist alllwp = LIST_HEAD_INITIALIZER(alllwp);

/* sparc doesn't sport constant page size, pretend we have 4k pages */
#ifdef __sparc__
int nbpg = 4096;
int pgofset = 4096-1;
int pgshift = 12;
#endif

/* on sun3 VM_MAX_ADDRESS is a const variable */
/* XXX: should be moved into rump.c and initialize for sun3 and sun3x? */
#ifdef sun3
const vaddr_t kernbase = KERNBASE3;
#endif

struct loadavg averunnable = {
	{ 0 * FSCALE,
	  1 * FSCALE,
	  11 * FSCALE, },
	FSCALE,
};

struct emul emul_netbsd = {
	.e_name = "netbsd-rump",
	.e_sysent = rump_sysent,
	.e_vm_default_addr = uvm_default_mapaddr,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern = syscall_intern,
#endif
};

u_int nprocs = 1;

int
kpause(const char *wmesg, bool intr, int timeo, kmutex_t *mtx)
{
	extern int hz;
	int rv, error;
	uint64_t sec, nsec;

	if (mtx)
		mutex_exit(mtx);

	sec = timeo / hz;
	nsec = (timeo % hz) * (1000000000 / hz);
	rv = rumpuser_nanosleep(&sec, &nsec, &error);
	
	if (mtx)
		mutex_enter(mtx);

	if (rv)
		return error;

	return 0;
}

void
lwp_unsleep(lwp_t *l, bool cleanup)
{

	KASSERT(mutex_owned(l->l_mutex));

	(*l->l_syncobj->sobj_unsleep)(l, cleanup);
}

void
lwp_update_creds(struct lwp *l)
{
	struct proc *p;
	kauth_cred_t oldcred;

	p = l->l_proc;
	oldcred = l->l_cred;
	l->l_prflag &= ~LPR_CRMOD;

	mutex_enter(p->p_lock);
	kauth_cred_hold(p->p_cred);
	l->l_cred = p->p_cred;
	mutex_exit(p->p_lock);

	if (oldcred != NULL)
		kauth_cred_free(oldcred);
}

vaddr_t
calc_cache_size(struct vm_map *map, int pct, int va_pct)
{
	paddr_t t;

	t = (paddr_t)physmem * pct / 100 * PAGE_SIZE;
	if ((vaddr_t)t != t) {
		panic("%s: needs tweak", __func__);
	}
	return t;
}

void
assert_sleepable(void)
{

	/* always sleepable, although we should improve this */
}

void
module_init_md(void)
{

	/*
	 * Nothing for now.  However, we should load the librump
	 * symbol table.
	 */
}

/* us and them, after all we're only ordinary seconds */
static void
rump_delay(unsigned int us)
{
	uint64_t sec, nsec;
	int error;

	sec = us / 1000000;
	nsec = (us % 1000000) * 1000;

	if (__predict_false(sec != 0))
		printf("WARNING: over 1s delay\n");

	rumpuser_nanosleep(&sec, &nsec, &error);
}
void (*delay_func)(unsigned int) = rump_delay;

/*
 * Provide weak aliases for tty routines used by printf.
 * They will be used unless the rumpkern_tty component is present.
 */

int rump_ttycheckoutq(struct tty *, int);
int
rump_ttycheckoutq(struct tty *tp, int wait)
{

	return 1;
}
__weak_alias(ttycheckoutq,rump_ttycheckoutq);

int rump_tputchar(int, int, struct tty *);
int
rump_tputchar(int c, int flags, struct tty *tp)
{

	cnputc(c);
	return 0;
}
__weak_alias(tputchar,rump_tputchar);

void
cnputc(int c)
{
	int error;

	rumpuser_putchar(c, &error);
}

void
cnflush(void)
{

	/* done */
}

#ifdef __HAVE_SYSCALL_INTERN
void
syscall_intern(struct proc *p)
{

	/* no you don't */
}
#endif

void
xc_send_ipi(struct cpu_info *ci)
{
	const struct cpu_info *curci = curcpu();
	CPU_INFO_ITERATOR cii;

	/*
	 * IPI are considered asynchronous, therefore no need to wait for
	 * unicast call delivery (nor the order of calls matters).  Our LWP
	 * needs to be bound to the CPU, since xc_unicast(9) may block.
	 *
	 * WARNING: These must be low-priority calls, as this routine is
	 * used to emulate high-priority (XC_HIGHPRI) mechanism.
	 */

	if (ci) {
		KASSERT(curci != ci);
		(void)xc_unicast(0, (xcfunc_t)xc_ipi_handler, NULL, NULL, ci);
		return;
	}

	curlwp->l_pflag |= LP_BOUND;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (curci == ci) {
			continue;
		}
		(void)xc_unicast(0, (xcfunc_t)xc_ipi_handler, NULL, NULL, ci);
	}
	curlwp->l_pflag &= ~LP_BOUND;
}

int
trace_enter(register_t code, const register_t *args, int narg)
{

	return 0;
}

void
trace_exit(register_t code, register_t rval[], int error)
{

	/* nada */
}

#ifdef LOCKDEBUG
void
turnstile_print(volatile void *obj, void (*pr)(const char *, ...))
{

	/* nada */
}
#endif
