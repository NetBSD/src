/*	$NetBSD: subr_kcov.c,v 1.3 2019/02/23 12:07:40 kamil Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Siddharth Muralee.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/module.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/conf.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>
#include <sys/kcov.h>

#define KCOV_BUF_MAX_ENTRIES	(256 << 10)

/*
 * The KCOV descriptors are allocated during open(), and are associated with
 * the calling proc. They are freed lazily when their refcount reaches zero,
 * only when the process exits; this guarantees that kd->buf is not mmapped
 * in a currently running LWP. A KCOV descriptor is active on only one LWP
 * at the same time within the proc.
 *
 * In the refcount, one ref is for the proc, and one ref is for the LWP where
 * the descriptor is active. In each case, the descriptor is pointed to in
 * the proc's and LWP's specificdata.
 */

typedef struct kcov_desc {
	kmutex_t lock;
	int refcnt;
	kcov_int_t *buf;
	size_t bufnent;
	size_t bufsize;
	TAILQ_ENTRY(kcov_desc) entry;
} kcov_t;

static specificdata_key_t kcov_proc_key;
static specificdata_key_t kcov_lwp_key;

static void
kcov_lock(kcov_t *kd)
{

	mutex_enter(&kd->lock);
	KASSERT(kd->refcnt > 0);
}

static void
kcov_unlock(kcov_t *kd)
{

	mutex_exit(&kd->lock);
}

static void
kcov_lwp_take(kcov_t *kd)
{

	kd->refcnt++;
	KASSERT(kd->refcnt == 2);
	lwp_setspecific(kcov_lwp_key, kd);
}

static void
kcov_lwp_release(kcov_t *kd)
{

	KASSERT(kd->refcnt == 2);
	kd->refcnt--;
	lwp_setspecific(kcov_lwp_key, NULL);
}

static inline bool
kcov_is_owned(kcov_t *kd)
{

	return (kd->refcnt > 1);
}

static void
kcov_free(void *arg)
{
	kcov_t *kd = (kcov_t *)arg;
	bool dofree;

	if (kd == NULL) {
		return;
	}

	kcov_lock(kd);
	kd->refcnt--;
	kcov_unlock(kd);
	dofree = (kd->refcnt == 0);

	if (!dofree) {
		return;
	}
	if (kd->buf != NULL) {
		uvm_km_free(kernel_map, (vaddr_t)kd->buf, kd->bufsize,
		    UVM_KMF_WIRED);
	}
	mutex_destroy(&kd->lock);
	kmem_free(kd, sizeof(*kd));
}

static int
kcov_allocbuf(kcov_t *kd, uint64_t nent)
{
	size_t size;

	if (nent < 2 || nent > KCOV_BUF_MAX_ENTRIES)
		return EINVAL;
	if (kd->buf != NULL)
		return EEXIST;

	size = roundup(nent * KCOV_ENTRY_SIZE, PAGE_SIZE);
	kd->buf = (kcov_int_t *)uvm_km_alloc(kernel_map, size, 0,
	    UVM_KMF_WIRED|UVM_KMF_ZERO);
	if (kd->buf == NULL)
		return ENOMEM;

	kd->bufnent = nent - 1;
	kd->bufsize = size;

	return 0;
}

/* -------------------------------------------------------------------------- */

static int
kcov_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct proc *p = l->l_proc;
	kcov_t *kd;

	kd = proc_getspecific(p, kcov_proc_key);
	if (kd != NULL)
		return EBUSY;

	kd = kmem_zalloc(sizeof(*kd), KM_SLEEP);
	mutex_init(&kd->lock, MUTEX_DEFAULT, IPL_NONE);
	kd->refcnt = 1;
	proc_setspecific(p, kcov_proc_key, kd);

	return 0;
}

static int
kcov_close(dev_t dev, int flag, int mode, struct lwp *l)
{

   	return 0;
}

static int
kcov_ioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct proc *p = l->l_proc;
	int error = 0;
	kcov_t *kd;

	kd = proc_getspecific(p, kcov_proc_key);
	if (kd == NULL)
		return ENXIO;
	kcov_lock(kd);

	switch (cmd) {
	case KCOV_IOC_SETBUFSIZE:
		if (kcov_is_owned(kd)) {
			error = EBUSY;
			break;
		}
		error = kcov_allocbuf(kd, *((uint64_t *)addr));
		break;
	case KCOV_IOC_ENABLE:
		if (kcov_is_owned(kd)) {
			error = EBUSY;
			break;
		}
		if (kd->buf == NULL) {
			error = ENOBUFS;
			break;
		}
		KASSERT(l == curlwp);
		kcov_lwp_take(kd);
		break;
	case KCOV_IOC_DISABLE:
		if (lwp_getspecific(kcov_lwp_key) == NULL) {
			error = ENOENT;
			break;
		}
		KASSERT(l == curlwp);
		kcov_lwp_release(kd);
		break;
	default:
		error = EINVAL;
	}

	kcov_unlock(kd);
	return error;
}

static paddr_t
kcov_mmap(dev_t dev, off_t offset, int prot)
{
	kcov_t *kd;
	paddr_t pa;
	vaddr_t va;

	kd = proc_getspecific(curproc, kcov_proc_key);
	KASSERT(kd != NULL);

	if ((offset < 0) || (offset >= kd->bufnent * KCOV_ENTRY_SIZE)) {
		return (paddr_t)-1;
	}
	if (offset & PAGE_MASK) {
		return (paddr_t)-1;
	}
	va = (vaddr_t)kd->buf + offset;
	if (!pmap_extract(pmap_kernel(), va, &pa)) {
		return (paddr_t)-1;
	}

	return atop(pa);
}

static inline bool
in_interrupt(void)
{
	return curcpu()->ci_idepth >= 0;
}

void __sanitizer_cov_trace_pc(void);

void
__sanitizer_cov_trace_pc(void)
{
	extern int cold;
	uint64_t idx;
	kcov_t *kd;

	if (__predict_false(cold)) {
		/* Do not trace during boot. */
		return;
	}

	if (in_interrupt()) {
		/* Do not trace in interrupts. */
		return;
	}

	kd = lwp_getspecific(kcov_lwp_key);
	if (__predict_true(kd == NULL)) {
		/* Not traced. */
		return;
	}

	idx = kd->buf[0];
	if (idx < kd->bufnent) {
		kd->buf[idx+1] = (intptr_t)__builtin_return_address(0);
		kd->buf[0]++;
	}
}

/* -------------------------------------------------------------------------- */

const struct cdevsw kcov_cdevsw = {
	.d_open = kcov_open,
	.d_close = kcov_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = kcov_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = kcov_mmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

MODULE(MODULE_CLASS_ANY, kcov, NULL);

static void
kcov_init(void)
{

	proc_specific_key_create(&kcov_proc_key, kcov_free);
	lwp_specific_key_create(&kcov_lwp_key, kcov_free);
}

static int
kcov_modcmd(modcmd_t cmd, void *arg)
{

   	switch (cmd) {
	case MODULE_CMD_INIT:
		kcov_init();
		return 0;
	case MODULE_CMD_FINI:
		return EINVAL;
	default:
		return ENOTTY;
	}
}
