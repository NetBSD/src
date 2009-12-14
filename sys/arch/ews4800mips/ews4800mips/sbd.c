/*	$NetBSD: sbd.c,v 1.3 2009/12/14 00:46:03 matt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: sbd.c,v 1.3 2009/12/14 00:46:03 matt Exp $");

/* System board */
#include "opt_sbd.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/sbdvar.h>
#include <machine/sbd.h>

struct sbd platform;		/* System board itself */

void
sbd_init(void)
{

	switch (SBD_INFO->machine) { /* Get model information from ROM */
	default:
		printf("This model is not supported.\n");
		printf("machine [0x%04x] model [0x%04x]\n",
		    SBD_INFO->machine, SBD_INFO->model);
		for (;;)
			;
		/* NOTREACHED */
		break;
#ifdef EWS4800_TR2
	case MACHINE_TR2:		/* EWS4800/350 */
		tr2_init();
		break;
#endif
#ifdef EWS4800_TR2A
	case MACHINE_TR2A:
		tr2a_init();		/* EWS4800/360 */
		break;
#endif
	}
}

void
sbd_memcluster_init(uint32_t m)
{
	/* Initialize memory_cluster *** mainfo_type2 only *** */
	size_t size, total = 0;
	int i, j, k, n;
	uint32_t start_addr[] = {
		__M0_BANK0_ADDR,
		__M0_BANK1_ADDR,
		__M1_BANK0_ADDR,
		__M1_BANK1_ADDR,
		__M2_BANK0_ADDR,
		__M2_BANK1_ADDR,
	};
	n = sizeof start_addr / sizeof start_addr[0];

	for (i = 0, k = 0, j = 1; i < 8; i++, m >>= 4) {
		size = m & 0xf ? ((m & 0xf) << 4) : 0;
		if (size == 0)
			continue;
		if (k == n) {
			/* don't load over 0x20000000 memory */
			printf("M%d=%dMB(reserved)\n", i, size);
			continue;
		}

		switch (size) {
		case 128:	/* oooooooo */
		case 16:	/* o....... */
			mem_clusters[j].size = size * 1024 * 1024;
			mem_clusters[j].start = start_addr[k];
			j += 1;
			k += 2;
			break;
		case 32:	/* o...o... */
			mem_clusters[j].size = 16 * 1024 * 1024;
			mem_clusters[j].start = start_addr[k++];
			j++;
			mem_clusters[j].size = 16 * 1024 * 1024;
			mem_clusters[j].start = start_addr[k++];
			j++;
			break;
		default:
			printf("UNKNOWN MEMORY CLUSTER SIZE%d\n", size);
			for (;;)
				;
		}
		total += size;
		printf("M%d=%dMB ", i, size);
	}
	printf(" total %dMB\n", total);
	mem_cluster_cnt = j;
	mem_clusters[0].size = total << 20;
}

void
sbd_memcluster_setup(void *kernstart, void *kernend)
{
	paddr_t start;
	size_t size;

	physmem = atop(mem_clusters[0].size);

	start = (paddr_t)round_page(MIPS_KSEG0_TO_PHYS(kernend));
	size = mem_clusters[1].size - start;

	/* kernel itself */
	mem_clusters[0].start = trunc_page(MIPS_KSEG0_TO_PHYS(kernstart));
	mem_clusters[0].size = start - mem_clusters[0].start;

	/* heap start */
	mem_clusters[1].start = start;
	mem_clusters[1].size = size;
}

void
sbd_memcluster_check(void)
{
	uint32_t *m, *mend;
	phys_ram_seg_t *p;
	paddr_t start;
	size_t size;
	size_t i, j;

	/* Very slow */
	for (i = 1; i < mem_cluster_cnt; i++) {
		p = &mem_clusters[i];
		start = p->start;
		size = p->size;
		printf("[%u] %#"PRIxPADDR"-%#"PRIxPADDR", %#x (%uMB)\n",
		    i, start, start + size, size, size >>20);
		m = (uint32_t *)MIPS_PHYS_TO_KSEG1(start);
		mend = (uint32_t *)MIPS_PHYS_TO_KSEG1(start + size);
		for (; m < mend; m++) {
			uint32_t pattern[4] =
			{ 0xffffffff, 0xa5a5a5a5, 0x5a5a5a5a, 0x00000000 };
			for (j = 0; j < 4; j++) {
				*m = pattern[j];
				if (*m != pattern[j])
					panic("memory error %p\n", m);
			}
		}
		printf("checked.\r");
		memset((void *)MIPS_PHYS_TO_KSEG0(start), 0, size);
		printf("cleared.\r");
	}
}
