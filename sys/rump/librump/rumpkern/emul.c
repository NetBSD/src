/*	$NetBSD: emul.c,v 1.115 2010/01/08 20:04:06 dyoung Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: emul.c,v 1.115 2010/01/08 20:04:06 dyoung Exp $");

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

time_t time_second = 1;

kmutex_t *proc_lock;
struct lwp lwp0;
struct vnode *rootvp;
dev_t rootdev;
int physmem = 256*256; /* 256 * 1024*1024 / 4k, PAGE_SIZE not always set */
int doing_shutdown;
const int schedppq = 1;
int hardclock_ticks;
bool mp_online = false;
struct vm_map *mb_map;
struct timeval boottime;
struct emul emul_netbsd;
int cold = 1;
int boothowto = AB_SILENT;
struct tty *constty;

char hostname[MAXHOSTNAMELEN];
size_t hostnamelen;

const char *panicstr;
const char *domainname;
int domainnamelen;

const struct filterops sig_filtops;

#define DEVSW_SIZE 255
const struct bdevsw *bdevsw0[DEVSW_SIZE]; /* XXX storage size */
const struct bdevsw **bdevsw = bdevsw0;
const int sys_cdevsws = DEVSW_SIZE;
int max_cdevsws = DEVSW_SIZE;

const struct cdevsw *cdevsw0[DEVSW_SIZE]; /* XXX storage size */
const struct cdevsw **cdevsw = cdevsw0;
const int sys_bdevsws = DEVSW_SIZE;
int max_bdevsws = DEVSW_SIZE;

struct devsw_conv devsw_conv0;
struct devsw_conv *devsw_conv = &devsw_conv0;
int max_devsw_convs = 0;
int mem_no = 2;

struct device *booted_device;
struct device *booted_wedge;
int booted_partition;

kmutex_t tty_lock;

kmutex_t sysctl_file_marker_lock;

/* sparc doesn't sport constant page size */
#ifdef __sparc__
int nbpg = 4096;
#endif

devclass_t
device_class(device_t dev)
{

	return dev->dv_class;
}

void
getnanouptime(struct timespec *ts)
{

	rump_getuptime(ts);
}

void
getmicrouptime(struct timeval *tv)
{
	struct timespec ts;

	getnanouptime(&ts);
	TIMESPEC_TO_TIMEVAL(tv, &ts);
}

static void
gettime(struct timespec *ts)
{
	uint64_t sec, nsec;
	int error;

	rumpuser_gettime(&sec, &nsec, &error);
	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

void
nanotime(struct timespec *ts)
{

	if (rump_threads) {
		rump_gettime(ts);
	} else {
		gettime(ts);
	}
}

/* hooray for mick, so what if I do */
void
getnanotime(struct timespec *ts)
{

	nanotime(ts);
}

void
microtime(struct timeval *tv)
{
	struct timespec ts;

	if (rump_threads) {
		rump_gettime(&ts);
		TIMESPEC_TO_TIMEVAL(tv, &ts);
	} else {
		gettime(&ts);
		TIMESPEC_TO_TIMEVAL(tv, &ts);
	}
}

void
getmicrotime(struct timeval *tv)
{

	microtime(tv);
}

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

void
psignal(struct proc *p, int signo)
{

	switch (signo) {
	case SIGSYS:
		break;
	default:
		panic("unhandled signal %d\n", signo);
	}
}

void
kpsignal(struct proc *p, ksiginfo_t *ksi, void *data)
{

	panic("%s: not implemented", __func__);
}

void
kpgsignal(struct pgrp *pgrp, ksiginfo_t *ksi, void *data, int checkctty)
{

	panic("%s: not implemented", __func__);
}

int
pgid_in_session(struct proc *p, pid_t pg_id)
{

	panic("%s: not implemented", __func__);
}

int
sigispending(struct lwp *l, int signo)
{

	return 0;
}

void
sigpending1(struct lwp *l, sigset_t *ss)
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
suspendsched(void)
{

	/* we don't control scheduling currently, can't do anything now */
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

const char *
device_xname(device_t dv)
{
	return "bogus0";
}

void
assert_sleepable(void)
{

	/* always sleepable, although we should improve this */
}

void
tc_setclock(const struct timespec *ts)
{

	panic("%s: not implemented", __func__);
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
kpreempt_disable(void)
{

	/* XXX: see below */
	KPREEMPT_DISABLE(curlwp);
}

void
kpreempt_enable(void)
{

	/* try to make sure kpreempt_disable() is only used from panic() */
	panic("kpreempt not supported");
}

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

bool
pmf_device_register1(struct device *dev,
	bool (*suspend)(device_t, pmf_qual_t),
	bool (*resume)(device_t, pmf_qual_t),
	bool (*shutdown)(device_t, int))
{

	return true;
}

void
pmf_device_deregister(struct device *dev)
{

	/* nada */
}

int
syscall_establish(const struct emul *em, const struct syscall_package *sp)
{
	extern struct sysent rump_sysent[];
	int i;

	KASSERT(em == NULL || em == &emul_netbsd);

	for (i = 0; sp[i].sp_call; i++)
		rump_sysent[sp[i].sp_code].sy_call = sp[i].sp_call;

	return 0;
}

int
syscall_disestablish(const struct emul *em, const struct syscall_package *sp)
{

	return 0;
}
