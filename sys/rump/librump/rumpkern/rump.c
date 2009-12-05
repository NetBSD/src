/*	$NetBSD: rump.c,v 1.146 2009/12/05 13:01:31 pooka Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: rump.c,v 1.146 2009/12/05 13:01:31 pooka Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/event.h>
#include <sys/exec_elf.h>
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
#include <sys/pipe.h>
#include <sys/queue.h>
#include <sys/reboot.h>
#include <sys/resourcevar.h>
#include <sys/select.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/tty.h>
#include <sys/uidinfo.h>
#include <sys/vmem.h>
#include <sys/xcall.h>

#include <rump/rumpuser.h>

#include <secmodel/suser/suser.h>

#include <prop/proplib.h>

#include <uvm/uvm_readahead.h>

#include "rump_private.h"
#include "rump_net_private.h"
#include "rump_vfs_private.h"
#include "rump_dev_private.h"

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
struct filedesc rump_filedesc0;
struct proclist allproc;
char machine[] = "rump";
static kauth_cred_t rump_susercred;

/* pretend the master rump proc is init */
struct proc *initproc = &proc0;

struct rumpuser_mtx *rump_giantlock;

sigset_t sigcantmask;

struct device rump_rootdev = {
	.dv_class = DV_VIRTUAL
};

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
static struct emul emul_rump = {
	.e_vm_default_addr = uvm_default_mapaddr,
};

int rump__unavailable(void);
int rump__unavailable() {return EOPNOTSUPP;}
__weak_alias(rump_net_init,rump__unavailable);
__weak_alias(rump_vfs_init,rump__unavailable);
__weak_alias(rump_dev_init,rump__unavailable);

__weak_alias(rump_vfs_fini,rump__unavailable);

__weak_alias(biodone,rump__unavailable);
__weak_alias(sopoll,rump__unavailable);

void rump__unavailable_vfs_panic(void);
void rump__unavailable_vfs_panic() {panic("vfs component not available");}
__weak_alias(usermount_common_policy,rump__unavailable_vfs_panic);

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

/*
 * Stir up the stack a bit.  These are exported functions to help
 * convince the compiler that we don't want these routines completely
 * optimized out or inlined.  Is there an easier way to do this?
 */
void nullfn(uint32_t *);
void nullfn(uint32_t *arg){}
void messthestack(void);
void
messthestack(void)
{
	uint32_t mess[64];
	uint64_t d1, d2;
	int i, error;

	for (i = 0; i < 64; i++) {
		rumpuser_gettime(&d1, &d2, &error);
		mess[i] = d2;
	}
	nullfn(mess);
}

int
rump__init(int rump_version)
{
	char buf[256];
	struct proc *p;
	struct lwp *l;
	int i;
	int error;

	/* not reentrant */
	if (rump_inited)
		return 0;
	else if (rump_inited == -1)
		panic("rump_init: host process restart required");
	else
		rump_inited = 1;

	/*
	 * Seed arc4random() with a "reasonable" amount of randomness.
	 * Yes, this is a quick kludge which depends on the arc4random
	 * implementation.
	 */
	messthestack();
	arc4random();

	if (rump_version != RUMP_VERSION) {
		printf("rump version mismatch, %d vs. %d\n",
		    rump_version, RUMP_VERSION);
		return EPROGMISMATCH;
	}

	if (rumpuser_getenv("RUMP_THREADS", buf, sizeof(buf), &error) == 0) {
		rump_threads = *buf != '0';
	}
	rumpuser_thrinit(rump_user_schedule, rump_user_unschedule,
	    rump_threads);
	rump_intr_init();

	/* init minimal lwp/cpu context */
	l = &lwp0;
	l->l_lid = 1;
	l->l_cpu = rump_cpu;
	rumpuser_set_curlwp(l);

	mutex_init(&tty_lock, MUTEX_DEFAULT, IPL_NONE);
	rumpuser_mutex_recursive_init(&rump_giantlock);
	ksyms_init();
	rumpvm_init();
	evcnt_init();

	once_init();
	prop_kern_init();

	pool_subsystem_init();
	kmem_init();

	uvm_ra_init();

	mutex_obj_init();
	callout_startup();

	kprintf_init();
	loginit();

	kauth_init();
	rump_susercred = rump_cred_create(0, 0, 0, NULL);

	/* init proc0 and rest of lwp0 now that we can allocate memory */
	p = &proc0;
	p->p_stats = &rump_stats;
	p->p_limit = &rump_limits;
	p->p_pgrp = &rump_pgrp;
	p->p_pid = 0;
	p->p_fd = &rump_filedesc0;
	p->p_vmspace = &rump_vmspace;
	p->p_emul = &emul_rump;
	p->p_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	l->l_cred = rump_cred_suserget();
	l->l_proc = p;
	LIST_INIT(&allproc);
	LIST_INSERT_HEAD(&allproc, &proc0, p_list);
	proc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	rump_limits.pl_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	rump_limits.pl_rlimit[RLIMIT_NOFILE].rlim_cur = RLIM_INFINITY;
	rump_limits.pl_rlimit[RLIMIT_SBSIZE].rlim_cur = RLIM_INFINITY;

	rump_scheduler_init();
	/* revert temporary context and schedule a real context */
	l->l_cpu = NULL;
	rumpuser_set_curlwp(NULL);
	rump_schedule();

	/* we are mostly go.  do per-cpu subsystem init */
	for (i = 0; i < ncpu; i++) {
		struct cpu_info *ci = cpu_lookup(i);

		callout_init_cpu(ci);
		softint_init(ci);
		xc_init_cpu(ci);
		pool_cache_cpu_init(ci);
		selsysinit(ci);
	}

	sysctl_init();
	kqueue_init();
	iostat_init();
	uid_init();
	percpu_init();
	fd_sys_init();
	module_init();
	devsw_init();
	pipe_init();

	/* these do nothing if not present */
	rump_vfs_init();
	rump_net_init();
	rump_dev_init();
	cold = 0;

	/* aieeeedondest */
	if (rump_threads) {
		if (workqueue_create(&uvm.aiodone_queue, "aiodoned",
		    rump_aiodone_worker, NULL, 0, 0, WQ_MPSAFE))
			panic("aiodoned");
	}

	sysctl_finalize();

	rumpuser_dl_module_bootstrap(rump_module_init, rump_kernelfsym_load);

	rumpuser_gethostname(hostname, MAXHOSTNAMELEN, &error);
	hostnamelen = strlen(hostname);

	sigemptyset(&sigcantmask);

	lwp0.l_fd = proc0.p_fd = fd_init(&rump_filedesc0);

	if (rump_threads)
		vmem_rehash_start();

	rump_unschedule();

	return 0;
}

/* maybe support sys_reboot some day for remote shutdown */
void
rump_reboot(int howto)
{

	/* dump means we really take the dive here */
	if ((howto & RB_DUMP) || panicstr) {
		rumpuser_exit(RUMPUSER_PANIC);
		/*NOTREACHED*/
	}

	/* try to sync */
	if (!((howto & RB_NOSYNC) || panicstr)) {
		rump_vfs_fini();
	}

	/* your wish is my command */
	if (howto & RB_HALT) {
		for (;;) {
			uint64_t sec = 5, nsec = 0;
			int error;

			rumpuser_nanosleep(&sec, &nsec, &error);
		}
	}
	rump_inited = -1;
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

static pid_t nextpid = 1;
struct lwp *
rump_newproc_switch()
{
	struct lwp *l;
	pid_t mypid;

	mypid = atomic_inc_uint_nv(&nextpid);
	if (__predict_false(mypid == 0))
		mypid = atomic_inc_uint_nv(&nextpid);

	l = rump_lwp_alloc(mypid, 0);
	rump_lwp_switch(l);

	return l;
}

struct lwp *
rump_lwp_alloc_and_switch(pid_t pid, lwpid_t lid)
{
	struct lwp *l;

	l = rump_lwp_alloc(pid, lid);
	rump_lwp_switch(l);

	return l;
}

struct lwp *
rump_lwp_alloc(pid_t pid, lwpid_t lid)
{
	struct lwp *l;
	struct proc *p;

	l = kmem_zalloc(sizeof(*l), KM_SLEEP);
	if (pid != 0) {
		p = kmem_zalloc(sizeof(*p), KM_SLEEP);
		rump_proc_vfs_init(p);
		p->p_stats = &rump_stats;
		p->p_limit = &rump_limits;
		p->p_pid = pid;
		p->p_vmspace = &rump_vmspace;
		p->p_emul = &emul_rump;
		p->p_fd = fd_init(NULL);
		p->p_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
		l->l_cred = rump_cred_suserget();
	} else {
		p = &proc0;
		l->l_cred = rump_susercred;
	}

	l->l_proc = p;
	l->l_lid = lid;
	l->l_fd = p->p_fd;
	l->l_cpu = NULL;

	return l;
}

void
rump_lwp_switch(struct lwp *newlwp)
{
	struct lwp *l = curlwp;

	rumpuser_set_curlwp(NULL);
	newlwp->l_cpu = l->l_cpu;
	newlwp->l_mutex = l->l_mutex;
	l->l_mutex = NULL;
	l->l_cpu = NULL;
	rumpuser_set_curlwp(newlwp);
	if (l->l_flag & LW_WEXIT)
		rump_lwp_free(l);
}

/* XXX: this has effect only on non-pid0 lwps */
void
rump_lwp_release(struct lwp *l)
{
	struct proc *p;

	p = l->l_proc;
	if (p->p_pid != 0) {
		mutex_obj_free(p->p_lock);
		fd_free();
		rump_proc_vfs_release(p);
		rump_cred_put(l->l_cred);
		kmem_free(p, sizeof(*p));
	}
	KASSERT((l->l_flag & LW_WEXIT) == 0);
	l->l_flag |= LW_WEXIT;
}

void
rump_lwp_free(struct lwp *l)
{

	KASSERT(l->l_flag & LW_WEXIT);
	KASSERT(l->l_mutex == NULL);
	kmem_free(l, sizeof(*l));
}

struct lwp *
rump_lwp_curlwp(void)
{
	struct lwp *l = curlwp;

	if (l->l_flag & LW_WEXIT)
		return NULL;
	return l;
}

/* rump private.  NEEDS WORK! */
void
rump_set_vmspace(struct vmspace *vm)
{
	struct proc *p = curproc;

	p->p_vmspace = vm;
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
rump_cred_put(kauth_cred_t cred)
{

	kauth_cred_free(cred);
}

kauth_cred_t
rump_cred_suserget(void)
{

	kauth_cred_hold(rump_susercred);
	return rump_susercred;
}

/*
 * Return the next system lwpid
 */
lwpid_t
rump_nextlid(void)
{
	lwpid_t retid;

	mutex_enter(proc0.p_lock);
	/*
	 * Take next one, don't return 0
	 * XXX: most likely we'll have collisions in case this
	 * wraps around.
	 */
	if (++proc0.p_nlwpid == 0)
		++proc0.p_nlwpid;
	retid = proc0.p_nlwpid;
	mutex_exit(proc0.p_lock);

	return retid;
}

#define ERROUT(err) do { rv = err; goto out; } while (/*CONSTCOND*/0)
int
rump_module_init(struct modinfo *mi, prop_dictionary_t props)
{
	struct module *mod;
	int rv;

	/* module_dummy */
	if (mi->mi_name == NULL)
		return EINVAL;

	mutex_enter(&module_lock);
	if (module_lookup(mi->mi_name))
		ERROUT(EEXIST);

	if (!module_compatible(mi->mi_version, __NetBSD_Version__))
		ERROUT(EPROGMISMATCH);

	rv = mi->mi_modcmd(MODULE_CMD_INIT, props);
	if (rv == 0) {
		mod = kmem_zalloc(sizeof(*mod), KM_SLEEP);
		mod->mod_info = mi;
		module_enqueue(mod);
		if (mi->mi_class == MODULE_CLASS_SECMODEL)
			secmodel_register();
	}

 out:
	mutex_exit(&module_lock);
	return rv;
}

int
rump_module_fini(struct modinfo *mi)
{
	int rv;

	rv = mi->mi_modcmd(MODULE_CMD_FINI, NULL);
	if (rv == 0 && mi->mi_class == MODULE_CLASS_SECMODEL)
		secmodel_deregister();

	return rv;
}

int
rump_kernelfsym_load(void *symtab, uint64_t symsize,
	char *strtab, uint64_t strsize)
{
	static int inited = 0;
	Elf64_Ehdr ehdr;

	if (inited)
		return EBUSY;
	inited = 1;

	/*
	 * Use 64bit header since it's bigger.  Shouldn't make a
	 * difference, since we're passing in all zeroes anyway.
	 */
	memset(&ehdr, 0, sizeof(ehdr));
	ksyms_addsyms_explicit(&ehdr, symtab, symsize, strtab, strsize);

	return 0;
}

static int
rump_sysproxy_local(int num, void *arg, uint8_t *data, size_t dlen,
	register_t *retval)
{
	struct lwp *l;
	struct sysent *callp;
	int rv;

	if (__predict_false(num >= SYS_NSYSENT))
		return ENOSYS;

	callp = rump_sysent + num;
	rump_schedule();
	l = curlwp;
	rv = callp->sy_call(l, (void *)data, retval);
	rump_unschedule();

	return rv;
}

int
rump_boot_gethowto()
{

	return boothowto;
}

void
rump_boot_sethowto(int howto)
{

	boothowto = howto;
}

rump_sysproxy_t rump_sysproxy = rump_sysproxy_local;
void *rump_sysproxy_arg;

/*
 * This whole syscall-via-rpc is still taking form.  For example, it
 * may be necessary to set syscalls individually instead of lobbing
 * them all to the same place.  So don't think this interface is
 * set in stone.
 */
int
rump_sysproxy_set(rump_sysproxy_t proxy, void *arg)
{

	if (rump_sysproxy_arg)
		return EBUSY;

	rump_sysproxy_arg = arg;
	rump_sysproxy = proxy;

	return 0;
}

int
rump_getversion(void)
{

	return __NetBSD_Version__;
}
