/*	$NetBSD: cpu.c,v 1.32.12.1 2014/08/20 00:03:19 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.32.12.1 2014/08/20 00:03:19 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <prop/proplib.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

struct cputab {
	u_int version;
	u_int mask;
	const char *name;
	struct cache_info ci;
};

static const struct cputab models[] = {
	{
		.version = PVR_401A1, 
		.mask = 0xffff0000,
		.name = "401A1",
		.ci = {
			.dcache_size = 1024,
			.dcache_line_size = 16,
			.icache_size = 2848,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_401B2, 
		.mask = 0xffff0000,
		.name = "401B21",
		.ci = {
			.dcache_size = 8192,
			.dcache_line_size = 16,
			.icache_size = 16384,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_401C2, 
		.mask = 0xffff0000,
		.name = "401C2",
		.ci = {
			.dcache_size = 8192,
			.dcache_line_size = 16,
			.icache_size = 0,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_401D2, 
		.mask = 0xffff0000,
		.name = "401D2",
		.ci = {
			.dcache_size = 2848,
			.dcache_line_size = 16,
			.icache_size = 4096,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_401E2, 
		.mask = 0xffff0000,
		.name = "401E2",
		.ci = {
			.dcache_size = 0,
			.dcache_line_size = 16,
			.icache_size = 0,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_401F2, 
		.mask = 0xffff0000,
		.name = "401F2",
		.ci = {
			.dcache_size = 2048,
			.dcache_line_size = 16,
			.icache_size = 2848,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_401G2, 
		.mask = 0xffff0000,
		.name = "401G2",
		.ci = {
			.dcache_size = 2848,
			.dcache_line_size = 16,
			.icache_size = 8192,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_403, 
		.mask = 0xffff0000,
		.name = "403",
		.ci = {
			.dcache_size = 8192,
			.dcache_line_size = 16,
			.icache_size = 16384,
			.icache_line_size = 16,
		}
	}, {
		.version = PVR_405GP, 
		.mask = 0xffff0000,
		.name = "405GP",
		.ci = {
			.dcache_size = 8192,
			.dcache_line_size = 32,
			.icache_size = 8192,
			.icache_line_size = 32,
		}
	}, {
		.version = PVR_405GPR, 
		.mask = 0xffff0000,
		.name = "405GPr",
		.ci = {
			.dcache_size = 16384,
			.dcache_line_size = 32,
			.icache_size = 16384,
			.icache_line_size = 32,
		}
	}, {
		.version = PVR_405D5X1, 
		.mask = 0xfffff000, 
		.name = "Xilinx Virtex II Pro",
		.ci = {
			.dcache_size = 16384,
			.dcache_line_size = 32,
			.icache_size = 16384,
			.icache_line_size = 32,
		}
	}, {
		.version = PVR_405D5X2, 
		.mask = 0xfffff000, 
		.name = "Xilinx Virtex 4 FX",
		.ci = {
			.dcache_size = 16384,
			.dcache_line_size = 32,
			.icache_size = 16384,
			.icache_line_size = 32,
		}
	}, {
		.version = PVR_405EX, 
		.mask = 0xffff0000, 
		.name = "405EX",
		.ci = {
			.dcache_size = 16384,
			.dcache_line_size = 32,
			.icache_size = 16384,
			.icache_line_size = 32,
		}
	}, {
		.version = 0,
		.mask = 0,
		.name = NULL,
		.ci = {
			/*
			 * Unknown CPU type.  For safety we'll specify a
			 * cache with a 4-byte line size.  That way cache
			 * flush routines won't miss any lines.
			 */
			.dcache_line_size = 4,
			.icache_line_size = 4,
		},
	},
};

static int	cpumatch(device_t, cfdata_t, void *);
static void	cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpumatch, cpuattach, NULL, NULL);

int ncpus;

struct cpu_info cpu_info[1] = {
	{
		/* XXX add more ci_ev_* as we teach 4xx about them */
		.ci_ev_clock = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "clock"),
		.ci_ev_statclock = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
		    NULL, "cpu0", "stat clock"),
		.ci_curlwp = &lwp0,
	}
};

bool cpufound;

static int
cpumatch(device_t parent, cfdata_t cf, void *aux)
{
	struct plb_attach_args *paa = aux;

	/* make sure that we're looking for a CPU */
	if (strcmp(paa->plb_name, cf->cf_name) != 0)
		return (0);

	return !cpufound;
}

static void
cpuattach(device_t parent, device_t self, void *aux)
{
	struct cpu_info * const ci = curcpu();
	const struct cputab *cp;
	u_int processor_freq;
	prop_number_t freq;

	freq = prop_dictionary_get(board_properties, "processor-frequency");
	KASSERT(freq != NULL);
	processor_freq = (unsigned int) prop_number_integer_value(freq);

	cpufound = true;
	ncpus++;

	const u_int pvr = mfpvr();
	for (cp = models; cp->name != NULL; cp++) {
		if ((pvr & cp->mask) == cp->version) {
			cpu_setmodel("%s", cp->name);
			break;
		}
	}
	if (__predict_false(cp->name == NULL))
		cpu_setmodel("Version 0x%x", pvr);

	aprint_normal(": %uMHz %s (PVR 0x%x)\n",
	    (processor_freq + 500000) / 1000000,
	    (cp->name != NULL ? cpu_getmodel() : "unknown model"),
	    pvr);

	cpu_probe_cache();

	/* We would crash later on anyway so just make the reason obvious */
	if (ci->ci_ci.icache_size == 0 && ci->ci_ci.dcache_size == 0)
		panic("%s: %s: could not detect cache size",
		    __func__, device_xname(self));

	aprint_normal_dev(self, "%uKB/%uB L1 instruction cache\n",
	    ci->ci_ci.icache_size / 1024, ci->ci_ci.icache_line_size);
	aprint_normal_dev(self, "%uKB/%uB L1 data cache\n",
	    ci->ci_ci.dcache_size / 1024, ci->ci_ci.dcache_line_size);
}

/*
 * This routine must be explicitly called to initialize the
 * CPU cache information so cache flushe and memcpy operation
 * work.
 */
void
cpu_probe_cache(void)
{
	struct cpu_info * const ci = curcpu();
	const struct cputab *cp = models;

	const u_int pvr = mfpvr();
	for (cp = models; cp->name != NULL; cp++) {
		if ((pvr & cp->mask) == cp->version)
			break;
	}

	/*
	 * Copy the cache from the cputab into cpu_info.
	 */
	ci->ci_ci = cp->ci;
}

/*
 * These small routines may have to be replaced,
 * if/when we support processors other that the 604.
 */

void
dcache_wbinv_page(vaddr_t va)
{
	const size_t dcache_line_size = curcpu()->ci_ci.dcache_line_size;

	if (dcache_line_size) {
		for (size_t i = 0; i < PAGE_SIZE; i += dcache_line_size) {
			__asm volatile("dcbf %0,%1" : : "b" (va), "r" (i));
		}
		__asm volatile("sync;isync" : : );
	}
}
