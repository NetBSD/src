/* $NetBSD: aarch64_machdep.c,v 1.1 2014/08/10 05:47:37 matt Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: aarch64_machdep.c,v 1.1 2014/08/10 05:47:37 matt Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <uvm/uvm.h>

#include <dev/mm.h>

#include <aarch64/locore.h>
#include <aarch64/vmparam.h>

char cpu_model[32]; 
char machine[] = MACHINE;
char machine_arch[] = MACHINE_ARCH;

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &pcu_fpu_ops,
};

struct vm_map *phys_map;

bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	if (physical_start <= pa && pa < physical_end) {
		*vap = AARCH64_PA_TO_KVA(pa);
		return true;
	}

	return false;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot) 
{
 
	return (pa < ctob(physmem)) ? 0 : EFAULT;
}

void
cpu_startup(void)
{
	char pbuf[sizeof("xxxx xx") + 2];

	vaddr_t maxaddr, minaddr = 0;
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	   VM_PHYS_SIZE, 0, FALSE, NULL);

	printf("%s%s%s", copyright, version, cpu_model);

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	format_bytes(pbuf, sizeof(pbuf), ctob(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	bus_space_mallocok();
}

void
cpu_dumpconf(void)
{
}
