/* $NetBSD: xenmem.c,v 1.4 2022/09/02 05:09:49 kre Exp $ */
/*
 * Copyright (c) 2022 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenmem.c,v 1.4 2022/09/02 05:09:49 kre Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/kmem.h>
#include <uvm/uvm_physseg.h>

#include <xen/xenmem.h>

/*
 * Xen physical space management
 * The xenmem_ex extent manage the VM's pseudo-physical memory space.
 * This contains the mémory allocated to the VM, and is also used to allocate
 * extra physical space used for XENMEM_add_to_physmap (in come cases)
 * In the !XENPV case, the physical space is managed by bus_space, so
 * we reuse the iomem_ex
 */

extern struct  extent *iomem_ex;
#define XENMEM_EX iomem_ex

paddr_t
xenmem_alloc_pa(u_long size, u_long align, bool waitok)
{
	u_long result;
	int error;

#ifdef _LP64
	/* allocate above the 4Gb range to not collide wit devices */
	error = extent_alloc_subregion(XENMEM_EX, 0x100000000UL, MAXIOMEM,
	    size, align, 0,
	    (waitok ? (EX_WAITSPACE | EX_WAITOK) : EX_NOWAIT) | EX_MALLOCOK,
	    &result);
#else
	error = extent_alloc(XENMEM_EX, size, align, 0,
	    (waitok ? (EX_WAITSPACE | EX_WAITOK) : EX_NOWAIT) | EX_MALLOCOK,
	    &result);
#endif
	if (error) {
		printf("xenmem_alloc_pa: failed %d\n", error);
		return 0;
	}
	return result;
}

void
xenmem_free_pa(paddr_t start, u_long size)
{
	int error;
	error = extent_free(XENMEM_EX, start, size, EX_NOWAIT);
	if (error) {
		printf("WARNING: xenmem_alloc_pa failed: %d\n", error);
	}
}
