/* $NetBSD: ipi_bebox.c,v 1.1 2012/10/20 14:53:37 kiyohara Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
__KERNEL_RCSID(0, "$NetBSD: ipi_bebox.c,v 1.1 2012/10/20 14:53:37 kiyohara Exp $");

#include <sys/atomic.h>
#include <sys/cpu.h>

#include <powerpc/pic/picvar.h>
#include <powerpc/pic/ipivar.h>


static void bebox_send_ipi(cpuid_t, uint32_t);
static void bebox_establish_ipi(int, int, void *);

void
setup_bebox_ipi(void)
{

	ipiops.ppc_send_ipi = bebox_send_ipi;
	ipiops.ppc_establish_ipi = bebox_establish_ipi;
	ipiops.ppc_ipi_vector = IPI_VECTOR;
}

static void
bebox_send_ipi(cpuid_t target, uint32_t mesg)
{
	struct cpu_info * const ci = curcpu();
	int i;

	switch (target) {
	case IPI_DST_ALL:
	case IPI_DST_NOTME:
		for (i = 0; i < ncpu; i++) {
			struct cpu_info * const dst_ci = cpu_lookup(i);

			if (target == IPI_DST_ALL || dst_ci != ci) {
				atomic_or_32(&dst_ci->ci_pending_ipis, mesg);
			}
		}
		break;

	default:
	    {
		struct cpu_info * const dst_ci = cpu_lookup(target);

		atomic_or_32(&dst_ci->ci_pending_ipis, mesg);
		break;
	    }
	}
}

static void
bebox_establish_ipi(int type, int level, void *ih_args)
{
	/* XXXXX: nothing? */
}
