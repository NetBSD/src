/*	$NetBSD: cpu.c,v 1.25.32.1 2008/01/09 01:47:47 matt Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.25.32.1 2008/01/09 01:47:47 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/evcnt.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <machine/cpu.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

struct cputab {
	u_int version;
	u_int mask;
	const char *name;
};
static struct cputab models[] = {
	{ PVR_401A1, 	0xffff0000,	"401A1" 	},
	{ PVR_401B2, 	0xffff0000,	"401B21" 	},
	{ PVR_401C2, 	0xffff0000,	"401C2" 	},
	{ PVR_401D2, 	0xffff0000,	"401D2" 	},
	{ PVR_401E2, 	0xffff0000,	"401E2" 	},
	{ PVR_401F2, 	0xffff0000,	"401F2" 	},
	{ PVR_401G2, 	0xffff0000,	"401G2" 	},
	{ PVR_403, 	0xffff0000,	"403" 		},
	{ PVR_405GP, 	0xffff0000,	"405GP" 	},
	{ PVR_405GPR, 	0xffff0000,	"405GPr" 	},
	{ PVR_405D5X1, 	0xfffff000, 	"Xilinx Virtex II Pro" 	},
	{ PVR_405D5X2, 	0xfffff000, 	"Xilinx Virtex 4 FX" 	},
	{ 0, 		0,		NULL 		}
};

static int	cpumatch(struct device *, struct cfdata *, void *);
static void	cpuattach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpumatch, cpuattach, NULL, NULL);

int ncpus;

struct cpu_info cpu_info[1] = {
	{
		/* XXX add more ci_ev_* as we teach 4xx about them */
		.ci_ev_clock = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "clock"),
		.ci_ev_statclock = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "stat clock"),
		.ci_ev_softclock = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "soft clock"),
		.ci_ev_softnet = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "soft net"),
		.ci_ev_softserial = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "soft serial"),
		.ci_curlwp = &lwp0,
	}
};

char cpu_model[80];

int cpufound = 0;

static int
cpumatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct plb_attach_args *paa = aux;

	/* make sure that we're looking for a CPU */
	if (strcmp(paa->plb_name, cf->cf_name) != 0)
		return (0);

	return !cpufound;
}

static void
cpuattach(struct device *parent, struct device *self, void *aux)
{
	struct cputab *cp = models;
	u_int pvr;
	u_int processor_freq;
	prop_number_t freq;

	freq = prop_dictionary_get(board_properties, "processor-frequency");
	KASSERT(freq != NULL);
	processor_freq = (unsigned int) prop_number_integer_value(freq);

	cpufound++;
	ncpus++;

	pvr = mfpvr();
	while (cp->name) {
		if ((pvr & cp->mask) == cp->version)
			break;
		cp++;
	}
	if (cp->name)
		strcpy(cpu_model, cp->name);
	else
		sprintf(cpu_model, "Version 0x%x", pvr);

	printf(": %dMHz %s (PVR 0x%x)\n", processor_freq / 1000 / 1000,
	    cp->name ? cp->name : "unknown model", pvr);

	cpu_probe_cache();

	/* We would crash later on anyway so just make the reason obvious */
	if (curcpu()->ci_ci.icache_size == 0 &&
	    curcpu()->ci_ci.dcache_size == 0)
		panic("%s could not detect cache size", device_xname(self));

	printf("%s: Instruction cache size %d line size %d\n",
	    device_xname(self),
	    curcpu()->ci_ci.icache_size, curcpu()->ci_ci.icache_line_size);
	printf("%s: Data cache size %d line size %d\n",
	    device_xname(self),
	    curcpu()->ci_ci.dcache_size, curcpu()->ci_ci.dcache_line_size);
}

/*
 * This routine must be explicitly called to initialize the
 * CPU cache information so cache flushe and memcpy operation
 * work.
 */
void
cpu_probe_cache()
{
	/*
	 * First we need to identify the CPU and determine the
	 * cache line size, or things like memset/memcpy may lose
	 * badly.
	 */
	switch (mfpvr() & 0xffff0000) {
	case PVR_401A1:
		curcpu()->ci_ci.dcache_size = 1024;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 2848;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401B2:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 16384;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401C2:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 0;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401D2:
		curcpu()->ci_ci.dcache_size = 2848;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 4096;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401E2:
		curcpu()->ci_ci.dcache_size = 0;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 0;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401F2:
		curcpu()->ci_ci.dcache_size = 2048;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 2848;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_401G2:
		curcpu()->ci_ci.dcache_size = 2848;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 8192;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_403:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 16;
		curcpu()->ci_ci.icache_size = 16384;
		curcpu()->ci_ci.icache_line_size = 16;
		break;
	case PVR_405GP:
		curcpu()->ci_ci.dcache_size = 8192;
		curcpu()->ci_ci.dcache_line_size = 32;
		curcpu()->ci_ci.icache_size = 8192;
		curcpu()->ci_ci.icache_line_size = 32;
		break;
	case PVR_405GPR:
	case PVR_405D5X1:
	case PVR_405D5X2:
		curcpu()->ci_ci.dcache_size = 16384;
		curcpu()->ci_ci.dcache_line_size = 32;
		curcpu()->ci_ci.icache_size = 16384;
		curcpu()->ci_ci.icache_line_size = 32;
		break;
	default:
		/*
		 * Unknown CPU type.  For safety we'll specify a
		 * cache with a 4-byte line size.  That way cache
		 * flush routines won't miss any lines.
		 */
		curcpu()->ci_ci.dcache_line_size = 4;
		curcpu()->ci_ci.icache_line_size = 4;
		break;
	}

}

/*
 * These small routines may have to be replaced,
 * if/when we support processors other that the 604.
 */

void
dcache_flush_page(vaddr_t va)
{
	int i;

	if (curcpu()->ci_ci.dcache_line_size)
		for (i = 0; i < PAGE_SIZE;
		     i += curcpu()->ci_ci.dcache_line_size)
			__asm volatile("dcbf %0,%1" : : "r" (va), "r" (i));
	__asm volatile("sync;isync" : : );
}

void
icache_flush_page(vaddr_t va)
{
	int i;

	if (curcpu()->ci_ci.icache_line_size)
		for (i = 0; i < PAGE_SIZE;
		     i += curcpu()->ci_ci.icache_line_size)
			__asm volatile("icbi %0,%1" : : "r" (va), "r" (i));
	__asm volatile("sync;isync" : : );
}

void
dcache_flush(vaddr_t va, vsize_t len)
{
	int i;

	if (len == 0)
		return;

	/* Make sure we flush all cache lines */
	len += va & (curcpu()->ci_ci.dcache_line_size-1);
	if (curcpu()->ci_ci.dcache_line_size)
		for (i = 0; i < len; i += curcpu()->ci_ci.dcache_line_size)
			__asm volatile("dcbf %0,%1" : : "r" (va), "r" (i));
	__asm volatile("sync;isync" : : );
}

void
icache_flush(vaddr_t va, vsize_t len)
{
	int i;

	if (len == 0)
		return;

	/* Make sure we flush all cache lines */
	len += va & (curcpu()->ci_ci.icache_line_size-1);
	if (curcpu()->ci_ci.icache_line_size)
		for (i = 0; i < len; i += curcpu()->ci_ci.icache_line_size)
			__asm volatile("icbi %0,%1" : : "r" (va), "r" (i));
	__asm volatile("sync;isync" : : );
}
