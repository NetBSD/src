/*	$NetBSD: rump.c,v 1.91 2009/01/26 14:41:28 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rump.c,v 1.91 2009/01/26 14:41:28 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/filedesc.h>
#include <sys/iostat.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kprintf.h>
#include <sys/ksyms.h>
#include <sys/msgbuf.h>
#include <sys/module.h>
#include <sys/once.h>
#include <sys/percpu.h>
#include <sys/queue.h>
#include <sys/resourcevar.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/tty.h>
#include <sys/uidinfo.h>
#include <sys/vmem.h>

#include <rump/rumpuser.h>

#include "rump_private.h"
#include "rump_net_private.h"
#include "rump_vfs_private.h"

struct proc proc0;
struct session rump_session = {
	.s_count = 1,
	.s_flags = 0,
	.s_leader = &proc0,
	.s_login = "rumphobo",
	.s_sid = 0,
};
struct pgrp rump_pgrp = {
	.pg_members = LIST_HEAD_INITIALIZER(pg_members),
	.pg_session = &rump_session,
	.pg_jobc = 1,
};
struct pstats rump_stats;
struct plimit rump_limits;
struct cpu_info rump_cpu;
struct filedesc rump_filedesc0;
struct proclist allproc;
char machine[] = "rump";
static kauth_cred_t rump_susercred;

struct rumpuser_mtx *rump_giantlock;

sigset_t sigcantmask;

#ifdef RUMP_WITHOUT_THREADS
int rump_threads = 0;
#else
int rump_threads = 1;
#endif

static void
rump_aiodone_worker(struct work *wk, void *dummy)
{
	struct buf *bp = (struct buf *)wk;

	KASSERT(&bp->b_work == wk);
	bp->b_iodone(bp);
}

static int rump_inited;
static struct emul emul_rump;

void rump__unavailable(void);
void rump__unavailable() {}
__weak_alias(rump_net_init,rump__unavailable);
__weak_alias(rump_vfs_init,rump__unavailable);

static void
pvfsinit_nop(struct proc *p)
{

	return;
}

static void
pvfsrele_nop(struct proc *p)
{

	return;
}

rump_proc_vfs_init_fn rump_proc_vfs_init = pvfsinit_nop;
rump_proc_vfs_release_fn rump_proc_vfs_release = pvfsrele_nop;

int
rump__init(int rump_version)
{
	char buf[256];
	struct proc *p;
	struct lwp *l;
	int error;

	/* XXX */
	if (rump_inited)
		return 0;
	rump_inited = 1;

	if (rump_version != RUMP_VERSION) {
		printf("rump version mismatch, %d vs. %d\n",
		    rump_version, RUMP_VERSION);
		return EPROGMISMATCH;
	}

	if (rumpuser_getenv("RUMP_THREADS", buf, sizeof(buf), &error) == 0) {
		rump_threads = *buf != '0';
	}
	rumpuser_thrinit(_kernel_lock, _kernel_unlock, rump_threads);

	mutex_init(&tty_lock, MUTEX_DEFAULT, IPL_NONE);
	rumpuser_mutex_recursive_init(&rump_giantlock);
	ksyms_init();
	rumpvm_init();

	once_init();

	rump_sleepers_init();
#ifdef RUMP_USE_REAL_ALLOCATORS
	pool_subsystem_init();
	kmem_init();
#endif
	kprintf_init();
	loginit();

	kauth_init();
	rump_susercred = rump_cred_create(0, 0, 0, NULL);

	l = &lwp0;
	p = &proc0;
	p->p_stats = &rump_stats;
	p->p_limit = &rump_limits;
	p->p_pgrp = &rump_pgrp;
	p->p_pid = 0;
	p->p_fd = &rump_filedesc0;
	p->p_vmspace = &rump_vmspace;
	p->p_emul = &emul_rump;
	l->l_cred = rump_cred_suserget();
	l->l_proc = p;
	l->l_lid = 1;
	LIST_INSERT_HEAD(&allproc, p, p_list);
	proc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	rump_limits.pl_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	rump_limits.pl_rlimit[RLIMIT_NOFILE].rlim_cur = RLIM_INFINITY;
	rump_limits.pl_rlimit[RLIMIT_SBSIZE].rlim_cur = RLIM_INFINITY;

	callout_startup();
	callout_init_cpu(&rump_cpu);

	iostat_init();
	uid_init();
	percpu_init();
	fd_sys_init();
	module_init();
	sysctl_init();
	softint_init(&rump_cpu);
	cold = 0;
	devsw_init();

	/* these do nothing if not present */
	rump_vfs_init();
	rump_net_init();

	/* aieeeedondest */
	if (rump_threads) {
		if (workqueue_create(&uvm.aiodone_queue, "aiodoned",
		    rump_aiodone_worker, NULL, 0, 0, 0))
			panic("aiodoned");
	}

	rumpuser_gethostname(hostname, MAXHOSTNAMELEN, &error);
	hostnamelen = strlen(hostname);

	sigemptyset(&sigcantmask);

	lwp0.l_fd = proc0.p_fd = fd_init(&rump_filedesc0);

#ifdef RUMP_USE_REAL_ALLOCATORS
	if (rump_threads)
		vmem_rehash_start();
#endif

	return 0;
}

struct uio *
rump_uio_setup(void *buf, size_t bufsize, off_t offset, enum rump_uiorw rw)
{
	struct uio *uio;
	enum uio_rw uiorw;

	switch (rw) {
	case RUMPUIO_READ:
		uiorw = UIO_READ;
		break;
	case RUMPUIO_WRITE:
		uiorw = UIO_WRITE;
		break;
	default:
		panic("%s: invalid rw %d", __func__, rw);
	}

	uio = kmem_alloc(sizeof(struct uio), KM_SLEEP);
	uio->uio_iov = kmem_alloc(sizeof(struct iovec), KM_SLEEP);

	uio->uio_iov->iov_base = buf;
	uio->uio_iov->iov_len = bufsize;

	uio->uio_iovcnt = 1;
	uio->uio_offset = offset;
	uio->uio_resid = bufsize;
	uio->uio_rw = uiorw;
	uio->uio_vmspace = UIO_VMSPACE_SYS;

	return uio;
}

size_t
rump_uio_getresid(struct uio *uio)
{

	return uio->uio_resid;
}

off_t
rump_uio_getoff(struct uio *uio)
{

	return uio->uio_offset;
}

size_t
rump_uio_free(struct uio *uio)
{
	size_t resid;

	resid = uio->uio_resid;
	kmem_free(uio->uio_iov, sizeof(*uio->uio_iov));
	kmem_free(uio, sizeof(*uio));

	return resid;
}

struct lwp *
rump_setup_curlwp(pid_t pid, lwpid_t lid, int set)
{
	struct lwp *l;
	struct proc *p;

	l = kmem_zalloc(sizeof(struct lwp), KM_SLEEP);
	if (pid != 0) {
		p = kmem_zalloc(sizeof(struct proc), KM_SLEEP);
		rump_proc_vfs_init(p);
		p->p_stats = &rump_stats;
		p->p_limit = &rump_limits;
		p->p_pid = pid;
		p->p_vmspace = &rump_vmspace;
		p->p_fd = fd_init(NULL);
	} else {
		p = &proc0;
	}

	l->l_cred = rump_cred_suserget();
	l->l_proc = p;
	l->l_lid = lid;
	l->l_fd = p->p_fd;
	l->l_mutex = RUMP_LMUTEX_MAGIC;
	l->l_cpu = &rump_cpu;

	if (set)
		rumpuser_set_curlwp(l);

	return l;
}

void
rump_clear_curlwp()
{
	struct lwp *l;

	l = rumpuser_get_curlwp();
	if (l->l_proc->p_pid != 0) {
		fd_free();
		rump_proc_vfs_release(l->l_proc);
		rump_cred_destroy(l->l_cred);
		kmem_free(l->l_proc, sizeof(*l->l_proc));
	}
	kmem_free(l, sizeof(*l));
	rumpuser_set_curlwp(NULL);
}

struct lwp *
rump_get_curlwp()
{
	struct lwp *l;

	l = rumpuser_get_curlwp();
	if (l == NULL)
		l = &lwp0;

	return l;
}

int
rump_splfoo()
{

	if (rumpuser_whatis_ipl() != RUMPUSER_IPL_INTR) {
		rumpuser_rw_enter(&rumpspl, 0);
		rumpuser_set_ipl(RUMPUSER_IPL_SPLFOO);
	}

	return 0;
}

void
rump_intr_enter(void)
{

	rumpuser_set_ipl(RUMPUSER_IPL_INTR);
	rumpuser_rw_enter(&rumpspl, 1);
}

void
rump_intr_exit(void)
{

	rumpuser_rw_exit(&rumpspl);
	rumpuser_clear_ipl(RUMPUSER_IPL_INTR);
}

void
rump_splx(int dummy)
{

	if (rumpuser_whatis_ipl() != RUMPUSER_IPL_INTR) {
		rumpuser_clear_ipl(RUMPUSER_IPL_SPLFOO);
		rumpuser_rw_exit(&rumpspl);
	}
}

kauth_cred_t
rump_cred_create(uid_t uid, gid_t gid, size_t ngroups, gid_t *groups)
{
	kauth_cred_t cred;
	int rv;

	cred = kauth_cred_alloc();
	kauth_cred_setuid(cred, uid);
	kauth_cred_seteuid(cred, uid);
	kauth_cred_setsvuid(cred, uid);
	kauth_cred_setgid(cred, gid);
	kauth_cred_setgid(cred, gid);
	kauth_cred_setegid(cred, gid);
	kauth_cred_setsvgid(cred, gid);
	rv = kauth_cred_setgroups(cred, groups, ngroups, 0, UIO_SYSSPACE);
	/* oh this is silly.  and by "this" I mean kauth_cred_setgroups() */
	assert(rv == 0);

	return cred;
}

void
rump_cred_destroy(kauth_cred_t cred)
{

	kauth_cred_free(cred);
}

kauth_cred_t
rump_cred_suserget()
{

	kauth_cred_hold(rump_susercred);
	return rump_susercred;
}

/* XXX: if they overflow, we're screwed */
lwpid_t
rump_nextlid()
{
	static unsigned lwpid = 2;

	do {
		lwpid = atomic_inc_uint_nv(&lwpid);
	} while (lwpid == 0);

	return (lwpid_t)lwpid;
}

int
rump_module_load(struct modinfo **mi)
{

	if (!module_compatible((*mi)->mi_version, __NetBSD_Version__))
		return EPROGMISMATCH;

	return (*mi)->mi_modcmd(MODULE_CMD_INIT, NULL);
}

int _syspuffs_stub(int, int *);
int
_syspuffs_stub(int fd, int *newfd)
{

	return ENODEV;
}
__weak_alias(rump_syspuffs_glueinit,_syspuffs_stub);
