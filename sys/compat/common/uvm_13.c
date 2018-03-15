/*	$NetBSD: uvm_13.c,v 1.1 2018/03/15 03:13:51 christos Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: uvm_13.c,v 1.1 2018/03/15 03:13:51 christos Exp $");

#if defined(_KERNEL) || defined(_MODULE)
#if defined(_KERNEL_OPT)
#include "opt_vmswap.h"
#else
#define VMSWAP	/* XXX */
#endif
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>
#include <sys/swap.h>
#include <uvm/uvm.h>
#include <uvm/uvm_swap.h>

#include <compat/sys/uvm.h>

static void
swapent13_cvt(void *p, const struct swapent *se)
{
	struct swapent13 *sep13 = p;

	sep13->se13_dev = se->se_dev;
	sep13->se13_flags = se->se_flags;
	sep13->se13_nblks = se->se_nblks;
	sep13->se13_inuse = se->se_inuse;
	sep13->se13_priority = se->se_priority;
}


static int
compat_uvm_swap_stats13(const struct sys_swapctl_args *uap, register_t *retval)
{
     return uvm_swap_stats(SCARG(uap, arg), SCARG(uap, misc),
	 swapent13_cvt, sizeof(struct swapent13), retval);

}

void
uvm_13_init(void)
{
	uvm_swap_stats13 = compat_uvm_swap_stats13;
}

void
uvm_13_fini(void)
{
	uvm_swap_stats13 = (void *)enosys;
}
