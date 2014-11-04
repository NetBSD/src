/*	$NetBSD: linux_writecomb.c,v 1.2 2014/11/04 11:27:31 jmcneill Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: linux_writecomb.c,v 1.2 2014/11/04 11:27:31 jmcneill Exp $");

#ifdef _KERNEL_OPT
#include "opt_mtrr.h"
#endif

#include <sys/kmem.h>
#include <sys/mutex.h>

#include <machine/mtrr.h>

#include <linux/idr.h>
#include <linux/io.h>

static struct {
	kmutex_t	lock;
	struct idr	idr;
} linux_writecomb __cacheline_aligned;

int
linux_writecomb_init(void)
{

	mutex_init(&linux_writecomb.lock, MUTEX_DEFAULT, IPL_SCHED);
	idr_init(&linux_writecomb.idr);

	return 0;
}

void
linux_writecomb_fini(void)
{

	KASSERT(idr_is_empty(&linux_writecomb.idr));
	idr_destroy(&linux_writecomb.idr);
	mutex_destroy(&linux_writecomb.lock);
}

int
arch_phys_wc_add(unsigned long base, unsigned long size)
{
	struct mtrr *mtrr;
	int n = 1;
	int id;
	int ret;

	mtrr = kmem_alloc(sizeof(*mtrr), KM_SLEEP);
	mtrr->base = base;
	mtrr->len = size;
	mtrr->type = MTRR_TYPE_WC;
	mtrr->flags = MTRR_VALID;

	/* XXX errno NetBSD->Linux */
	ret = -mtrr_set(mtrr, &n, NULL, MTRR_GETSET_KERNEL);
	if (ret) {
		KASSERT(n == 0);
		goto fail0;
	}
	KASSERT(n == 1);

	idr_preload(GFP_KERNEL);
	mutex_spin_enter(&linux_writecomb.lock);
	id = idr_alloc(&linux_writecomb.idr, mtrr, 0, 0, GFP_NOWAIT);
	mutex_spin_exit(&linux_writecomb.lock);
	idr_preload_end();
	if (id < 0)
		goto fail1;

	return id;

fail1:	KASSERT(id < 0);
	mtrr->type = 0;
	mtrr->flags = 0;
	/* XXX errno NetBSD->Linux */
	ret = -mtrr_set(mtrr, &n, NULL, MTRR_GETSET_KERNEL);
	KASSERT(ret == 0);
	KASSERT(n == 1);
	ret = id;
fail0:	KASSERT(ret < 0);
	kmem_free(mtrr, sizeof(*mtrr));
	return ret;
}

void
arch_phys_wc_del(int id)
{
	struct mtrr *mtrr;
	int n;
	int ret __diagused;

	KASSERT(0 <= id);

	mutex_spin_enter(&linux_writecomb.lock);
	mtrr = idr_find(&linux_writecomb.idr, id);
	idr_remove(&linux_writecomb.idr, id);
	mutex_spin_enter(&linux_writecomb.lock);

	if (mtrr != NULL) {
		mtrr->type = 0;
		mtrr->flags = 0;
		/* XXX errno NetBSD->Linux */
		ret = -mtrr_set(mtrr, &n, NULL, MTRR_GETSET_KERNEL);
		KASSERT(ret == 0);
		KASSERT(n == 1);
		kmem_free(mtrr, sizeof(*mtrr));
	}
}

int
phys_wc_to_mtrr_index(int handle)
{

	/* XXX Actually implement this...requires changes to our MTRR API.  */
	return handle;
}
