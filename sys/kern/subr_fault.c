/*	$NetBSD: subr_fault.c,v 1.2 2020/06/30 16:28:17 maxv Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: subr_fault.c,v 1.2 2020/06/30 16:28:17 maxv Exp $");

#include <sys/module.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <sys/conf.h>
#include <sys/types.h>
#include <sys/specificdata.h>
#include <sys/kmem.h>
#include <sys/atomic.h>
#include <sys/ioccom.h>
#include <sys/lwp.h>
#include <sys/fault.h>

typedef struct {
	volatile bool enabled;
	volatile bool oneshot;
	volatile unsigned long nth;
	volatile unsigned long cnt;
	volatile unsigned long nfaults;
} fault_t;

static fault_t fault_global __cacheline_aligned = {
	.enabled = false,
	.oneshot = false,
	.nth = FAULT_NTH_MIN,
	.cnt = 0,
	.nfaults = 0
};

static kmutex_t fault_global_lock __cacheline_aligned;
static specificdata_key_t fault_lwp_key;

/* -------------------------------------------------------------------------- */

bool
fault_inject(void)
{
	volatile unsigned long cnt;
	fault_t *f;

	if (__predict_false(cold))
		return false;

	if (__predict_false(atomic_load_acquire(&fault_global.enabled))) {
		f = &fault_global;
	} else {
		f = lwp_getspecific(fault_lwp_key);
		if (__predict_true(f == NULL))
			return false;
		if (__predict_false(!f->enabled))
			return false;
	}

	if (atomic_load_relaxed(&f->oneshot)) {
		if (__predict_true(atomic_load_relaxed(&f->nfaults) > 0))
			return false;
	}

	cnt = atomic_inc_ulong_nv(&f->cnt);
	if (__predict_false(cnt % atomic_load_relaxed(&f->nth) == 0)) {
		atomic_inc_ulong(&f->nfaults);
		return true;
	}

	return false;
}

/* -------------------------------------------------------------------------- */

static int
fault_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	return 0;
}

static int
fault_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	return 0;
}

static int
fault_ioc_enable(struct fault_ioc_enable *args)
{
	fault_t *f;

	if (args->mode != FAULT_MODE_NTH_ONESHOT)
		return EINVAL;
	if (args->nth < FAULT_NTH_MIN)
		return EINVAL;

	switch (args->scope) {
	case FAULT_SCOPE_GLOBAL:
		mutex_enter(&fault_global_lock);
		if (fault_global.enabled) {
			mutex_exit(&fault_global_lock);
			return EEXIST;
		}
		fault_global.oneshot = true;
		atomic_store_relaxed(&fault_global.nth, args->nth);
		fault_global.cnt = 0;
		fault_global.nfaults = 0;
		atomic_store_release(&fault_global.enabled, true);
		mutex_exit(&fault_global_lock);
		break;
	case FAULT_SCOPE_LWP:
		f = lwp_getspecific(fault_lwp_key);
		if (f != NULL) {
			if (f->enabled)
				return EEXIST;
		} else {
			f = kmem_zalloc(sizeof(*f), KM_SLEEP);
			lwp_setspecific(fault_lwp_key, f);
		}
		f->oneshot = true;
		atomic_store_relaxed(&f->nth, args->nth);
		f->cnt = 0;
		f->nfaults = 0;
		atomic_store_release(&f->enabled, true);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int
fault_ioc_disable(struct fault_ioc_disable *args)
{
	fault_t *f;

	switch (args->scope) {
	case FAULT_SCOPE_GLOBAL:
		mutex_enter(&fault_global_lock);
		if (!fault_global.enabled) {
			mutex_exit(&fault_global_lock);
			return ENOENT;
		}
		atomic_store_release(&fault_global.enabled, false);
		mutex_exit(&fault_global_lock);
		break;
	case FAULT_SCOPE_LWP:
		f = lwp_getspecific(fault_lwp_key);
		if (f == NULL)
			return ENOENT;
		if (!f->enabled)
			return ENOENT;
		atomic_store_release(&f->enabled, false);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int
fault_ioc_getinfo(struct fault_ioc_getinfo *args)
{
	fault_t *f;

	switch (args->scope) {
	case FAULT_SCOPE_GLOBAL:
		args->nfaults = atomic_load_relaxed(&fault_global.nfaults);
		break;
	case FAULT_SCOPE_LWP:
		f = lwp_getspecific(fault_lwp_key);
		if (f == NULL)
			return ENOENT;
		args->nfaults = atomic_load_relaxed(&f->nfaults);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static int
fault_ioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	switch (cmd) {
	case FAULT_IOC_ENABLE:
		return fault_ioc_enable(addr);
	case FAULT_IOC_DISABLE:
		return fault_ioc_disable(addr);
	case FAULT_IOC_GETINFO:
		return fault_ioc_getinfo(addr);
	default:
		return EINVAL;
	}
}

const struct cdevsw fault_cdevsw = {
	.d_open = fault_open,
	.d_close = fault_close,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = fault_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

/* -------------------------------------------------------------------------- */

MODULE(MODULE_CLASS_MISC, fault, NULL);

static void
fault_lwp_free(void *arg)
{
	fault_t *f = (fault_t *)arg;

	if (f == NULL) {
		return;
	}

	kmem_free(f, sizeof(*f));
}

static void
fault_init(void)
{
	mutex_init(&fault_global_lock, MUTEX_DEFAULT, IPL_NONE);
	lwp_specific_key_create(&fault_lwp_key, fault_lwp_free);
}

static int
fault_modcmd(modcmd_t cmd, void *arg)
{
   	switch (cmd) {
	case MODULE_CMD_INIT:
		fault_init();
		return 0;
	case MODULE_CMD_FINI:
		return EINVAL;
	default:
		return ENOTTY;
	}
}
