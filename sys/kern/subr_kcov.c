/*	$NetBSD: subr_kcov.c,v 1.17 2022/07/06 01:12:46 riastradh Exp $	*/

/*
 * Copyright (c) 2019-2020 The NetBSD Foundation, Inc.
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
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/mutex.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>
#include <sys/kcov.h>

#define KCOV_BUF_MAX_ENTRIES	(256 << 10)

#define KCOV_CMP_CONST		1
#define KCOV_CMP_SIZE(x)	((x) << 1)

static dev_type_open(kcov_open);

const struct cdevsw kcov_cdevsw = {
	.d_open = kcov_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

static int kcov_fops_ioctl(file_t *, u_long, void *);
static int kcov_fops_close(file_t *);
static int kcov_fops_mmap(file_t *, off_t *, size_t, int, int *, int *,
    struct uvm_object **, int *);

const struct fileops kcov_fileops = {
	.fo_read = fbadop_read,
	.fo_write = fbadop_write,
	.fo_ioctl = kcov_fops_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = kcov_fops_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
	.fo_mmap = kcov_fops_mmap,
};

/*
 * The KCOV descriptors (KD) are allocated during open(), and are associated
 * with a file descriptor.
 *
 * An LWP can 'enable' a KD. When this happens, this LWP becomes the owner of
 * the KD, and no LWP can 'disable' this KD except the owner.
 *
 * A KD is freed when its file descriptor is closed _iff_ the KD is not active
 * on an LWP. If it is, we ask the LWP to free it when it exits.
 *
 * The buffers mmapped are in a dedicated uobj, therefore there is no risk
 * that the kernel frees a buffer still mmapped in a process: the uobj
 * refcount will be non-zero, so the backing is not freed until an munmap
 * occurs on said process.
 */

typedef struct kcov_desc {
	/* Local only */
	kmutex_t lock;
	bool lwpfree;
	bool silenced;

	/* Pointer to the end of the structure, if any */
	struct kcov_desc *remote;

	/* Can be remote */
	kcov_int_t *buf;
	struct uvm_object *uobj;
	size_t bufnent;
	size_t bufsize;
	int mode;
	bool enabled;
} kcov_t;

/* -------------------------------------------------------------------------- */

static void
kcov_lock(kcov_t *kd)
{

	mutex_enter(&kd->lock);
}

static void
kcov_unlock(kcov_t *kd)
{

	mutex_exit(&kd->lock);
}

static bool
kcov_mode_is_valid(int mode)
{
	switch (mode) {
	case KCOV_MODE_NONE:
	case KCOV_MODE_TRACE_PC:
	case KCOV_MODE_TRACE_CMP:
		return true;
	default:
		return false;
	}
}

/* -------------------------------------------------------------------------- */

static void
kcov_free(kcov_t *kd)
{

	KASSERT(kd != NULL);
	if (kd->buf != NULL) {
		uvm_deallocate(kernel_map, (vaddr_t)kd->buf, kd->bufsize);
	}
	mutex_destroy(&kd->lock);
	kmem_free(kd, sizeof(*kd));
}

void
kcov_lwp_free(struct lwp *l)
{
	kcov_t *kd = (kcov_t *)l->l_kcov;

	if (kd == NULL) {
		return;
	}
	kcov_lock(kd);
	kd->enabled = false;
	kcov_unlock(kd);
	if (kd->lwpfree) {
		kcov_free(kd);
	}
}

static int
kcov_allocbuf(kcov_t *kd, uint64_t nent)
{
	size_t size;
	int error;

	if (nent < 2 || nent > KCOV_BUF_MAX_ENTRIES)
		return EINVAL;
	if (kd->buf != NULL)
		return EEXIST;

	size = roundup(nent * KCOV_ENTRY_SIZE, PAGE_SIZE);
	kd->bufnent = nent - 1;
	kd->bufsize = size;
	kd->uobj = uao_create(kd->bufsize, 0);

	/* Map the uobj into the kernel address space, as wired. */
	kd->buf = NULL;
	error = uvm_map(kernel_map, (vaddr_t *)&kd->buf, kd->bufsize, kd->uobj,
	    0, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW, UVM_INH_SHARE,
	    UVM_ADV_RANDOM, 0));
	if (error) {
		uao_detach(kd->uobj);
		return error;
	}
	error = uvm_map_pageable(kernel_map, (vaddr_t)kd->buf,
	    (vaddr_t)kd->buf + size, false, 0);
	if (error) {
		uvm_deallocate(kernel_map, (vaddr_t)kd->buf, size);
		return error;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */

typedef struct kcov_remote {
	LIST_ENTRY(kcov_remote) list;
	uint64_t subsystem;
	uint64_t id;
	u_int refcount;
	kcov_t kd;
} kcov_remote_t;

typedef LIST_HEAD(, kcov_remote) kcov_remote_list_t;

static kcov_remote_list_t kcov_remote_list;

static kcov_remote_t *
kcov_remote_find(uint64_t subsystem, uint64_t id)
{
	kcov_remote_t *kr;

	LIST_FOREACH(kr, &kcov_remote_list, list) {
		if (kr->subsystem == subsystem && kr->id == id)
			return kr;
	}

	return NULL;
}

void
kcov_remote_register(uint64_t subsystem, uint64_t id)
{
	kcov_remote_t *kr;
	kcov_t *kd;
	int error;

	if (kcov_remote_find(subsystem, id) != NULL) {
		panic("%s: kr already exists", __func__);
	}

	kr = kmem_zalloc(sizeof(*kr), KM_SLEEP);
	kr->subsystem = subsystem;
	kr->id = id;
	kr->refcount = 0;
	kd = &kr->kd;

	mutex_init(&kd->lock, MUTEX_DEFAULT, IPL_NONE);
	error = kcov_allocbuf(kd, KCOV_BUF_MAX_ENTRIES);
	if (error != 0)
		panic("%s: failed to allocate buffer", __func__);

	LIST_INSERT_HEAD(&kcov_remote_list, kr, list);
}

void
kcov_remote_enter(uint64_t subsystem, uint64_t id)
{
	struct lwp *l = curlwp;
	kcov_remote_t *kr;
	kcov_t *kd;
	u_int refs __diagused;

	kr = kcov_remote_find(subsystem, id);
	if (__predict_false(kr == NULL)) {
		panic("%s: unable to find kr", __func__);
	}

	refs = atomic_inc_uint_nv(&kr->refcount);
	KASSERT(refs == 1);

	KASSERT(l->l_kcov == NULL);
	kd = &kr->kd;
	if (atomic_load_relaxed(&kd->enabled)) {
		l->l_kcov = kd;
	}
}

void
kcov_remote_leave(uint64_t subsystem, uint64_t id)
{
	struct lwp *l = curlwp;
	kcov_remote_t *kr;
	u_int refs __diagused;

	kr = kcov_remote_find(subsystem, id);
	if (__predict_false(kr == NULL)) {
		panic("%s: unable to find kr", __func__);
	}

	refs = atomic_dec_uint_nv(&kr->refcount);
	KASSERT(refs == 0);

	l->l_kcov = NULL;
}

static int
kcov_remote_enable(kcov_t *kd, int mode)
{
	kcov_lock(kd);
	if (kd->enabled) {
		kcov_unlock(kd);
		return EBUSY;
	}
	kd->mode = mode;
	atomic_store_relaxed(&kd->enabled, true);
	kcov_unlock(kd);

	return 0;
}

static int
kcov_remote_disable(kcov_t *kd)
{
	kcov_lock(kd);
	if (!kd->enabled) {
		kcov_unlock(kd);
		return ENOENT;
	}
	atomic_store_relaxed(&kd->enabled, false);
	kcov_unlock(kd);

	return 0;
}

static int
kcov_remote_attach(kcov_t *kd, struct kcov_ioc_remote_attach *args)
{
	kcov_remote_t *kr;

	if (kd->enabled)
		return EEXIST;

	kr = kcov_remote_find(args->subsystem, args->id);
	if (kr == NULL)
		return ENOENT;
	kd->remote = &kr->kd;

	return 0;
}

static int
kcov_remote_detach(kcov_t *kd)
{
	if (kd->enabled)
		return EEXIST;
	if (kd->remote == NULL)
		return ENOENT;
	(void)kcov_remote_disable(kd->remote);
	kd->remote = NULL;
	return 0;
}

/* -------------------------------------------------------------------------- */

static int
kcov_setbufsize(kcov_t *kd, uint64_t *args)
{
	if (kd->remote != NULL)
		return 0; /* buffer allocated remotely */
	if (kd->enabled)
		return EBUSY;
	return kcov_allocbuf(kd, *((uint64_t *)args));
}

static int
kcov_enable(kcov_t *kd, uint64_t *args)
{
	struct lwp *l = curlwp;
	int mode;

	mode = *((int *)args);
	if (!kcov_mode_is_valid(mode))
		return EINVAL;

	if (kd->remote != NULL)
		return kcov_remote_enable(kd->remote, mode);

	if (kd->enabled)
		return EBUSY;
	if (l->l_kcov != NULL)
		return EBUSY;
	if (kd->buf == NULL)
		return ENOBUFS;

	l->l_kcov = kd;
	kd->mode = mode;
	kd->enabled = true;
	return 0;
}

static int
kcov_disable(kcov_t *kd)
{
	struct lwp *l = curlwp;

	if (kd->remote != NULL)
		return kcov_remote_disable(kd->remote);

	if (!kd->enabled)
		return ENOENT;
	if (l->l_kcov != kd)
		return ENOENT;

	l->l_kcov = NULL;
	kd->enabled = false;
	return 0;
}

/* -------------------------------------------------------------------------- */

void
kcov_silence_enter(void)
{
	kcov_t *kd = curlwp->l_kcov;

	if (kd != NULL)
		kd->silenced = true;
}

void
kcov_silence_leave(void)
{
	kcov_t *kd = curlwp->l_kcov;

	if (kd != NULL)
		kd->silenced = false;
}

/* -------------------------------------------------------------------------- */

static int
kcov_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct file *fp;
	int error, fd;
	kcov_t *kd;

	error = fd_allocfile(&fp, &fd);
	if (error)
		return error;

	kd = kmem_zalloc(sizeof(*kd), KM_SLEEP);
	mutex_init(&kd->lock, MUTEX_DEFAULT, IPL_NONE);

	return fd_clone(fp, fd, flag, &kcov_fileops, kd);
}

static int
kcov_fops_close(file_t *fp)
{
	kcov_t *kd = fp->f_data;

	kcov_lock(kd);
	if (kd->remote != NULL)
		(void)kcov_remote_disable(kd->remote);
	if (kd->enabled) {
		kd->lwpfree = true;
		kcov_unlock(kd);
	} else {
		kcov_unlock(kd);
		kcov_free(kd);
	}
	fp->f_data = NULL;

   	return 0;
}

static int
kcov_fops_ioctl(file_t *fp, u_long cmd, void *addr)
{
	kcov_t *kd;
	int error;

	kd = fp->f_data;
	if (kd == NULL)
		return ENXIO;
	kcov_lock(kd);

	switch (cmd) {
	case KCOV_IOC_SETBUFSIZE:
		error = kcov_setbufsize(kd, addr);
		break;
	case KCOV_IOC_ENABLE:
		error = kcov_enable(kd, addr);
		break;
	case KCOV_IOC_DISABLE:
		error = kcov_disable(kd);
		break;
	case KCOV_IOC_REMOTE_ATTACH:
		error = kcov_remote_attach(kd, addr);
		break;
	case KCOV_IOC_REMOTE_DETACH:
		error = kcov_remote_detach(kd);
		break;
	default:
		error = EINVAL;
	}

	kcov_unlock(kd);
	return error;
}

static int
kcov_fops_mmap(file_t *fp, off_t *offp, size_t size, int prot, int *flagsp,
    int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	off_t off = *offp;
	kcov_t *kd, *kdbuf;
	int error = 0;

	KASSERT(size > 0);

	if (prot & PROT_EXEC)
		return EACCES;
	if (off < 0)
		return EINVAL;
	if (size > KCOV_BUF_MAX_ENTRIES * KCOV_ENTRY_SIZE)
		return EINVAL;
	if (off > KCOV_BUF_MAX_ENTRIES * KCOV_ENTRY_SIZE)
		return EINVAL;

	kd = fp->f_data;
	if (kd == NULL)
		return ENXIO;
	kcov_lock(kd);

	if (kd->remote != NULL)
		kdbuf = kd->remote;
	else
		kdbuf = kd;

	if ((size + off) > kdbuf->bufsize) {
		error = ENOMEM;
		goto out;
	}

	uao_reference(kdbuf->uobj);

	*uobjp = kdbuf->uobj;
	*maxprotp = prot;
	*advicep = UVM_ADV_RANDOM;

out:
	kcov_unlock(kd);
	return error;
}

/* -------------------------------------------------------------------------- */

/*
 * Constraints on the functions here: they must be marked with __nomsan, and
 * must not make any external call.
 */

static inline bool __nomsan
in_interrupt(void)
{
	return curcpu()->ci_idepth >= 0;
}

void __sanitizer_cov_trace_pc(void);

void __nomsan
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

	kd = curlwp->l_kcov;
	if (__predict_true(kd == NULL)) {
		/* Not traced. */
		return;
	}

	if (!kd->enabled) {
		/* Tracing not enabled */
		return;
	}

	if (__predict_false(kd->silenced)) {
		/* Silenced. */
		return;
	}

	if (kd->mode != KCOV_MODE_TRACE_PC) {
		/* PC tracing mode not enabled */
		return;
	}
	KASSERT(kd->remote == NULL);

	idx = kd->buf[0];
	if (idx < kd->bufnent) {
		kd->buf[idx+1] =
		    (intptr_t)__builtin_return_address(0);
		kd->buf[0] = idx + 1;
	}
}

static void __nomsan
trace_cmp(uint64_t type, uint64_t arg1, uint64_t arg2, intptr_t pc)
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

	kd = curlwp->l_kcov;
	if (__predict_true(kd == NULL)) {
		/* Not traced. */
		return;
	}

	if (!kd->enabled) {
		/* Tracing not enabled */
		return;
	}

	if (__predict_false(kd->silenced)) {
		/* Silenced. */
		return;
	}

	if (kd->mode != KCOV_MODE_TRACE_CMP) {
		/* CMP tracing mode not enabled */
		return;
	}
	KASSERT(kd->remote == NULL);

	idx = kd->buf[0];
	if ((idx * 4 + 4) <= kd->bufnent) {
		kd->buf[idx * 4 + 1] = type;
		kd->buf[idx * 4 + 2] = arg1;
		kd->buf[idx * 4 + 3] = arg2;
		kd->buf[idx * 4 + 4] = pc;
		kd->buf[0] = idx + 1;
	}
}

void __sanitizer_cov_trace_cmp1(uint8_t arg1, uint8_t arg2);

void __nomsan
__sanitizer_cov_trace_cmp1(uint8_t arg1, uint8_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(0), arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_cmp2(uint16_t arg1, uint16_t arg2);

void __nomsan
__sanitizer_cov_trace_cmp2(uint16_t arg1, uint16_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(1), arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_cmp4(uint32_t arg1, uint32_t arg2);

void __nomsan
__sanitizer_cov_trace_cmp4(uint32_t arg1, uint32_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(2), arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_cmp8(uint64_t arg1, uint64_t arg2);

void __nomsan
__sanitizer_cov_trace_cmp8(uint64_t arg1, uint64_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(3), arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_const_cmp1(uint8_t arg1, uint8_t arg2);

void __nomsan
__sanitizer_cov_trace_const_cmp1(uint8_t arg1, uint8_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(0) | KCOV_CMP_CONST, arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_const_cmp2(uint16_t arg1, uint16_t arg2);

void __nomsan
__sanitizer_cov_trace_const_cmp2(uint16_t arg1, uint16_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(1) | KCOV_CMP_CONST, arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_const_cmp4(uint32_t arg1, uint32_t arg2);

void __nomsan
__sanitizer_cov_trace_const_cmp4(uint32_t arg1, uint32_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(2) | KCOV_CMP_CONST, arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_const_cmp8(uint64_t arg1, uint64_t arg2);

void __nomsan
__sanitizer_cov_trace_const_cmp8(uint64_t arg1, uint64_t arg2)
{

	trace_cmp(KCOV_CMP_SIZE(3) | KCOV_CMP_CONST, arg1, arg2,
	    (intptr_t)__builtin_return_address(0));
}

void __sanitizer_cov_trace_switch(uint64_t val, uint64_t *cases);

void __nomsan
__sanitizer_cov_trace_switch(uint64_t val, uint64_t *cases)
{
	uint64_t i, nbits, ncases, type;
	intptr_t pc;

	pc = (intptr_t)__builtin_return_address(0);
	ncases = cases[0];
	nbits = cases[1];

	switch (nbits) {
	case 8:
		type = KCOV_CMP_SIZE(0);
		break;
	case 16:
		type = KCOV_CMP_SIZE(1);
		break;
	case 32:
		type = KCOV_CMP_SIZE(2);
		break;
	case 64:
		type = KCOV_CMP_SIZE(3);
		break;
	default:
		return;
	}
	type |= KCOV_CMP_CONST;

	for (i = 0; i < ncases; i++)
		trace_cmp(type, cases[i + 2], val, pc);
}

/* -------------------------------------------------------------------------- */

MODULE(MODULE_CLASS_MISC, kcov, NULL);

static int
kcov_modcmd(modcmd_t cmd, void *arg)
{

   	switch (cmd) {
	case MODULE_CMD_INIT:
		return 0;
	case MODULE_CMD_FINI:
		return EINVAL;
	default:
		return ENOTTY;
	}
}
