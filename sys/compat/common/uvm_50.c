/*	$NetBSD: uvm_50.c,v 1.1.2.2 2018/03/15 09:12:05 pgoyette Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: uvm_50.c,v 1.1.2.2 2018/03/15 09:12:05 pgoyette Exp $");

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

#include <compat/sys/uvm.h>

static void
swapent50_cvt(void *p, const struct swapent *se)
{
	struct swapent50 *sep50 = p;

	sep50->se50_dev = se->se_dev;
	sep50->se50_flags = se->se_flags;
	sep50->se50_nblks = se->se_nblks;
	sep50->se50_inuse = se->se_inuse;
	sep50->se50_priority = se->se_priority;
	KASSERT(sizeof(se->se_path) <= sizeof(sep50->se50_path));
	strcpy(sep50->se50_path, se->se_path);
}


static int
compat_uvm_swap_stats50(const struct sys_swapctl_args *uap, register_t *retval)
{
     return uvm_swap_stats(SCARG(uap, arg), SCARG(uap, misc),
	 swapent50_cvt, sizeof(struct swapent50), retval);

}

void
uvm_50_init(void)
{
	uvm_swap_stats50 = compat_uvm_swap_stats50;
}

void
uvm_50_fini(void)
{
	uvm_swap_stats50 = (void *)enosys;
}
