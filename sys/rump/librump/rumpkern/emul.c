/*	$NetBSD: emul.c,v 1.96 2009/09/24 21:00:09 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: emul.c,v 1.96 2009/09/24 21:00:09 pooka Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
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
#include <sys/kthread.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/timetc.h>
#include <sys/tprintf.h>
#include <sys/module.h>
#include <sys/tty.h>
#include <sys/reboot.h>

#include <dev/cons.h>

#include <machine/stdarg.h>

#include <rump/rumpuser.h>

#include <uvm/uvm_map.h>

#include "rump_private.h"

time_t time_second = 1;

kmutex_t *proc_lock;
struct lwp lwp0;
struct vnode *rootvp;
struct device *root_device;
dev_t rootdev;
int physmem = 256*256; /* 256 * 1024*1024 / 4k, PAGE_SIZE not always set */
int doing_shutdown;
int ncpu = 1;
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
const char ostype[] = "NetBSD";
const char osrelease[] = "999"; /* paradroid 4evah */
const char kernel_ident[] = "RUMP-ROAST";
const char *domainname;
int domainnamelen;

const struct filterops seltrue_filtops;
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

int
copyin(const void *uaddr, void *kaddr, size_t len)
{

	if (curproc->p_vmspace == &rump_vmspace)
		memcpy(kaddr, uaddr, len);
	else
		rump_sysproxy_copyin(uaddr, kaddr, len);
	return 0;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{

	if (curproc->p_vmspace == &rump_vmspace)
		memcpy(uaddr, kaddr, len);
	else
		rump_sysproxy_copyout(kaddr, uaddr, len);
	return 0;
}

int
copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done)
{

	return copyinstr(kfaddr, kdaddr, len, done);
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{

	if (curproc->p_vmspace == &rump_vmspace)
		strlcpy(kaddr, uaddr, len);
	else
		rump_sysproxy_copyin(uaddr, kaddr, len);
	if (done)
		*done = strlen(kaddr)+1; /* includes termination */
	return 0;
}

int
copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{

	if (curproc->p_vmspace == &rump_vmspace)
		strlcpy(uaddr, kaddr, len);
	else
		rump_sysproxy_copyout(kaddr, uaddr, len);
	if (done)
		*done = strlen(uaddr)+1; /* includes termination */
	return 0;
}

int
copyin_vmspace(struct vmspace *vm, const void *uaddr, void *kaddr, size_t len)
{

	return copyin(uaddr, kaddr, len);
}

int
copyout_vmspace(struct vmspace *vm, const void *kaddr, void *uaddr, size_t len)
{

	return copyout(kaddr, uaddr, len);
}

int
kcopy(const void *src, void *dst, size_t len)
{

	memcpy(dst, src, len);
	return 0;
}

int
uiomove(void *buf, size_t n, struct uio *uio)
{
	struct iovec *iov;
	uint8_t *b = buf;
	size_t cnt;

	if (uio->uio_vmspace != UIO_VMSPACE_SYS)
		panic("%s: vmspace != UIO_VMSPACE_SYS", __func__);

	while (n && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		if (uio->uio_rw == UIO_READ)
			memcpy(iov->iov_base, b, cnt);
		else
			memcpy(b, iov->iov_base, cnt);

		iov->iov_base = (uint8_t *)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		b += cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		n -= cnt;
	}

	return 0;
}

void
uio_setup_sysspace(struct uio *uio)
{

	uio->uio_vmspace = UIO_VMSPACE_SYS;
}

devclass_t
device_class(device_t dev)
{

	if (dev != root_device)
		panic("%s: dev != root_device not supported", __func__);

	return DV_DISK;
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

void
malloc_type_attach(struct malloc_type *type)
{

	return;
}

void
malloc_type_detach(struct malloc_type *type)
{

	return;
}

void *
kern_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	void *rv;

	rv = rumpuser_malloc(size, (flags & (M_CANFAIL | M_NOWAIT)) != 0);
	if (rv && flags & M_ZERO)
		memset(rv, 0, size);

	return rv;
}

void *
kern_realloc(void *ptr, unsigned long size, struct malloc_type *type, int flags)
{

	return rumpuser_realloc(ptr, size, (flags & (M_CANFAIL|M_NOWAIT)) != 0);
}

void
kern_free(void *ptr, struct malloc_type *type)
{

	rumpuser_free(ptr);
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

struct kthdesc {
	void (*f)(void *);
	void *arg;
	struct lwp *mylwp;
};

static void *
threadbouncer(void *arg)
{
	struct kthdesc *k = arg;
	void (*f)(void *);
	void *thrarg;

	f = k->f;
	thrarg = k->arg;
	rumpuser_set_curlwp(k->mylwp);
	kmem_free(k, sizeof(struct kthdesc));

	if ((curlwp->l_pflag & LP_MPSAFE) == 0)
		KERNEL_LOCK(1, NULL);
	f(thrarg);
	panic("unreachable, should kthread_exit()");
}

int
kthread_create(pri_t pri, int flags, struct cpu_info *ci,
	void (*func)(void *), void *arg, lwp_t **newlp, const char *fmt, ...)
{
	char thrstore[MAXCOMLEN];
	const char *thrname = NULL;
	va_list ap;
	struct kthdesc *k;
	struct lwp *l;
	int rv;

	thrstore[0] = '\0';
	if (fmt) {
		va_start(ap, fmt);
		vsnprintf(thrstore, sizeof(thrstore), fmt, ap);
		va_end(ap);
		thrname = thrstore;
	}

	/*
	 * We don't want a module unload thread.
	 * (XXX: yes, this is a kludge too, and the kernel should
	 * have a more flexible method for configuring which threads
	 * we want).
	 */
	if (strcmp(thrstore, "modunload") == 0) {
		return 0;
	}

	if (!rump_threads) {
		/* fake them */
		if (strcmp(thrstore, "vrele") == 0) {
			printf("rump warning: threads not enabled, not starting"
			   " vrele thread\n");
			return 0;
		} else if (strcmp(thrstore, "cachegc") == 0) {
			printf("rump warning: threads not enabled, not starting"
			   " namecache g/c thread\n");
			return 0;
		} else if (strcmp(thrstore, "nfssilly") == 0) {
			printf("rump warning: threads not enabled, not enabling"
			   " nfs silly rename\n");
			return 0;
		} else if (strcmp(thrstore, "unpgc") == 0) {
			printf("rump warning: threads not enabled, not enabling"
			   " UNP garbage collection\n");
			return 0;
		} else
			panic("threads not available, setenv RUMP_THREADS 1");
	}

	KASSERT(fmt != NULL);
	if (ci != NULL)
		panic("%s: bounded threads not supported", __func__);

	k = kmem_alloc(sizeof(struct kthdesc), KM_SLEEP);
	k->f = func;
	k->arg = arg;
	k->mylwp = l = rump_setup_curlwp(0, rump_nextlid(), 0);
	if (flags & KTHREAD_MPSAFE)
		l->l_pflag |= LP_MPSAFE;
	rv = rumpuser_thread_create(threadbouncer, k, thrname);
	if (rv)
		return rv;

	if (newlp)
		*newlp = l;
	return 0;
}

void
kthread_exit(int ecode)
{

	if ((curlwp->l_pflag & LP_MPSAFE) == 0)
		KERNEL_UNLOCK_ONE(NULL);
	rump_clear_curlwp();
	rumpuser_thread_exit();
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
		panic("unhandled signal %d", signo);
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

	panic("%s: not implemented", __func__);
}

u_int
lwp_unsleep(lwp_t *l, bool cleanup)
{

	KASSERT(mutex_owned(l->l_mutex));

	return (*l->l_syncobj->sobj_unsleep)(l, cleanup);
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

int
seltrue(dev_t dev, int events, struct lwp *l)
{
        return (events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

void
selrecord(lwp_t *selector, struct selinfo *sip)
{
}

void
selinit(struct selinfo *sip)
{
}

void
selnotify(struct selinfo *sip, int events, long knhint)
{
}

void
seldestroy(struct selinfo *sip)
{
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

	rumpuser_panic();
}

/*
 * XXX: from sys_select.c, see that file for license.
 * (these will go away really soon in favour of the real sys_select.c)
 * ((really, the select code just needs cleanup))
 * (((seriously)))
 */
int
inittimeleft(struct timespec *ts, struct timespec *sleepts)
{
	if (itimespecfix(ts))
		return -1;
	getnanouptime(sleepts);
	return 0;
}

int
gettimeleft(struct timespec *ts, struct timespec *sleepts)
{
	/*
	 * We have to recalculate the timeout on every retry.
	 */
	struct timespec sleptts;
	/*
	 * reduce ts by elapsed time
	 * based on monotonic time scale
	 */
	getnanouptime(&sleptts);
	timespecadd(ts, sleepts, ts);
	timespecsub(ts, &sleptts, ts);
	*sleepts = sleptts;
	return tstohz(ts);
}

bool
pmf_device_register1(struct device *dev,
	bool (*suspend)(device_t PMF_FN_PROTO),
	bool (*resume)(device_t PMF_FN_PROTO),
	bool (*shutdown)(device_t, int))
{

	return true;
}

void
pmf_device_deregister(struct device *dev)
{

	/* nada */
}
