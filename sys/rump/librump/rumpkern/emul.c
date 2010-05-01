/*	$NetBSD: emul.c,v 1.135 2010/05/01 09:00:06 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
__KERNEL_RCSID(0, "$NetBSD: emul.c,v 1.135 2010/05/01 09:00:06 pooka Exp $");

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
#include <sys/filedesc.h>
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

#include <dev/cons.h>

#include <rump/rumpuser.h>

#include <uvm/uvm_map.h>

#include "rump_private.h"

kmutex_t *proc_lock;
struct lwp lwp0;
struct vnode *rootvp;
dev_t rootdev = NODEV;
int physmem = 256*256; /* 256 * 1024*1024 / 4k, PAGE_SIZE not always set */
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

struct device *booted_device;
struct device *booted_wedge;
int booted_partition;

/* XXX: unused */
kmutex_t tty_lock;
krwlock_t exec_lock;

struct lwplist alllwp = LIST_HEAD_INITIALIZER(alllwp);

/* sparc doesn't sport constant page size */
#ifdef __sparc__
int nbpg = 4096;
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
};

struct proc *
p_find(pid_t pid, uint flags)
{

	panic("%s: not implemented", __func__);
}

struct pgrp *
pg_find(pid_t pid, uint flags)
{

	panic("%s: not implemented", __func__);
}

int
pgid_in_session(struct proc *p, pid_t pg_id)
{

	panic("%s: not implemented", __func__);
}

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

int
proc_uidmatch(kauth_cred_t cred, kauth_cred_t target)
{

	panic("%s: not implemented", __func__);
}

void
proc_crmod_enter(void)
{

	panic("%s: not implemented", __func__);
}

void
proc_crmod_leave(kauth_cred_t c1, kauth_cred_t c2, bool sugid)
{

	panic("%s: not implemented", __func__);
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

void
proc_sesshold(struct session *ss)
{

	panic("proc_sesshold() impossible, session %p", ss);
}

void
proc_sessrele(struct session *ss)
{

	panic("proc_sessrele() impossible, session %p", ss);
}

int
proc_vmspace_getref(struct proc *p, struct vmspace **vm)
{

	/* XXX */
	*vm = p->p_vmspace;
	return 0;
}

int
ttycheckoutq(struct tty *tp, int wait)
{

	return 1;
}

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

int
tputchar(int c, int flags, struct tty *tp)
{

	cnputc(c);
	return 0;
}

void
cpu_reboot(int howto, char *bootstr)
{

	rump_reboot(howto);

	/* this function is __dead, we must exit */
	rumpuser_exit(0);
}
