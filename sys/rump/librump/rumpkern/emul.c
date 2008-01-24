/*	$NetBSD: emul.c,v 1.24 2008/01/24 08:34:49 pooka Exp $	*/

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

#define malloc(a,b,c) __wrap_malloc(a,b,c)

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/vnode.h>
#include <sys/stat.h>
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

#include <machine/stdarg.h>

#include <uvm/uvm_map.h>

#include "rump_private.h"
#include "rumpuser.h"

time_t time_second = 1;

kmutex_t proclist_mutex;
kmutex_t proclist_lock;
struct lwp lwp0;
struct vnode *rootvp;
struct device *root_device;
dev_t rootdev;
struct vm_map *kernel_map;
int physmem = 256*1024*1024 / PAGE_SIZE;
int doing_shutdown;
int ncpu = 1;
const int schedppq = 1;

MALLOC_DEFINE(M_MOUNT, "mount", "vfs mount struct");
MALLOC_DEFINE(M_UFSMNT, "UFS mount", "UFS mount structure");
MALLOC_DEFINE(M_TEMP, "temp", "misc. temporary data buffers");
MALLOC_DEFINE(M_DEVBUF, "devbuf", "device driver memory");
MALLOC_DEFINE(M_VNODE, "vnodes", "Dynamically allocated vnodes");
MALLOC_DEFINE(M_KEVENT, "kevent", "kevents/knotes");

char hostname[MAXHOSTNAMELEN];
size_t hostnamelen;

u_long	bufmem_valimit;
u_long	bufmem_hiwater;
u_long	bufmem_lowater;
u_long	bufmem;
u_int	nbuf;

const char *panicstr;

const struct filterops seltrue_filtops;

void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("panic: ");
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	abort();
}

void
log(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
uprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
printf_nolog(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

int
copyin(const void *uaddr, void *kaddr, size_t len)
{

	memcpy(kaddr, uaddr, len);
	return 0;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{

	memcpy(uaddr, kaddr, len);
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

	strlcpy(kaddr, uaddr, len);
	*done = strlen(kaddr);
	return 0;
}

int
uiomove(void *buf, size_t n, struct uio *uio)
{
	struct iovec *iov;
	uint8_t *b = buf;
	size_t cnt;
	int rv;

	if (uio->uio_vmspace != UIO_VMSPACE_SYS)
		panic("%s: vmspace != UIO_VMSPACE_SYS", __func__);

	/*
	 * See if rump ubc code claims the offset.  This is of course
	 * a blatant violation of abstraction levels, but let's keep
	 * me simple & stupid for now.
	 */
	if (rump_ubc_magic_uiomove(buf, n, uio, &rv, NULL))
		return rv;

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

const struct bdevsw *
bdevsw_lookup(dev_t dev)
{

	return (const struct bdevsw *)1;
}

devclass_t
device_class(device_t dev)
{

	if (dev != root_device)
		panic("%s: dev != root_device not supported", __func__);

	return DV_DISK;
}

void
getmicrouptime(struct timeval *tvp)
{
	int error;

	rumpuser_gettimeofday(tvp, &error);
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
__wrap_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	void *rv;

	rv = rumpuser_malloc(size, (flags & (M_CANFAIL | M_NOWAIT)) != 0);
	if (rv && flags & M_ZERO)
		memset(rv, 0, size);

	return rv;
}

void
nanotime(struct timespec *ts)
{
	struct timeval tv;
	int error;

	rumpuser_gettimeofday(&tv, &error);
	TIMEVAL_TO_TIMESPEC(&tv, ts);
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
	int error;

	rumpuser_gettimeofday(tv, &error);
}

void
getmicrotime(struct timeval *tv)
{
	int error;

	rumpuser_gettimeofday(tv, &error);
}

void
bdev_strategy(struct buf *bp)
{

	panic("%s: not supported", __func__);
}

int
bdev_type(dev_t dev)
{

	return D_DISK;
}

struct kthdesc {
	void (*f)(void *);
	void *arg;
	struct lwp *mylwp;
};

static lwpid_t curlid = 2;

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

	f(thrarg);
	panic("unreachable, should kthread_exit()");
}

int
kthread_create(pri_t pri, int flags, struct cpu_info *ci,
	void (*func)(void *), void *arg, lwp_t **newlp, const char *fmt, ...)
{
	struct kthdesc *k;
	struct lwp *l;
	int rv;

#ifdef RUMP_WITHOUT_THREADS
	panic("threads not available, undef RUMP_WITHOUT_THREADS");
#endif

	KASSERT(fmt != NULL);
	if (ci != NULL)
		panic("%s: bounded threads not supported", __func__);

	k = kmem_alloc(sizeof(struct kthdesc), KM_SLEEP);
	k->f = func;
	k->arg = arg;
	k->mylwp = l = rump_setup_curlwp(0, curlid++, 0);
	rv = rumpuser_thread_create(threadbouncer, k);
	if (rv)
		return rv;

	if (newlp)
		*newlp = l;
	return 0;
}

void
kthread_exit(int ecode)
{

	rumpuser_thread_exit();
}

void
callout_init(callout_t *c, u_int flags)
{

	panic("%s: not implemented", __func__);
}

void
callout_reset(callout_t *c, int ticks, void (*func)(void *), void *arg)
{

	panic("%s: not implemented", __func__);
}

bool
callout_stop(callout_t *c)
{

	panic("%s: not implemented", __func__);
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
knote_fdclose(struct lwp *l, int fd)
{

	/* since we don't add knotes, we don't have to remove them */
}

int
seltrue_kqfilter(dev_t dev, struct knote *kn)
{

	panic("%s: not implemented", __func__);
}

int
kpause(const char *wmesg, bool intr, int timeo, kmutex_t *mtx)
{
	extern int hz;
	int rv, error;

	if (mtx)
		mutex_exit(mtx);
	rv = rumpuser_usleep(timeo * (1000000 / hz), &error);
	if (mtx)
		mutex_enter(mtx);

	if (rv)
		return error;

	return 0;
}
