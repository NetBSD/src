/*	$NetBSD: rmixl_cpu.c,v 1.1.2.1 2010/01/16 23:47:30 cliff Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_cpu.c,v 1.1.2.1 2010/01/16 23:47:30 cliff Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <mips/rmi/rmixl_cpucorevar.h>

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpu_match, cpu_attach, NULL, NULL);

static int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cpucore_attach_args *ca = aux;
	int thread = cf->cf_loc[CPUCORECF_THREAD];

	if (strncmp(ca->ca_name, cf->cf_name, strlen(cf->cf_name)) == 0
#ifndef MULTIPROCESSOR
	    && ca->ca_thread == 0
#endif
	    && (thread == CPUCORECF_THREAD_DEFAULT || thread == ca->ca_thread))
			return 1;

	return 0;
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpucore_attach_args *ca = aux;
	if (ca->ca_thread == 0 && ca->ca_core == 0) {
		struct cpu_info * const ci = curcpu();
		ci->ci_dev = self;
		self->dv_private = ci;
#ifdef MULTIPROCESSOR
	} else {
		struct pglist pglist;

		/*
		 * Grab a page from the first 256MB to use to store
		 * exception vectors and cpu_info for this cpu.
		 */
		error = uvm_pglistalloc(PAGE_SIZE,
		    0, 0x10000000,
		    PAGE_SIZE, PAGE_SIZE, &pglist, 1, false);
		if (error) {
			aprint_error(": failed to allocte exception vectors\n");
			return;
		}

		const paddr_t pa = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
		const vaddr_t va = MIPS_PHYS_TO_KSEG0(pa);
		struct cpu_info * const ci = (void *) (va + 0x400);
		memset(va, 0, PAGE_SIZE);
		ci->ci_ebase = va;
		ci->ci_ebase_pa = pa;
		ci->ci_dev = self;
		self->dv_private = ci;
#endif
	}
	aprint_normal("\n");
}
