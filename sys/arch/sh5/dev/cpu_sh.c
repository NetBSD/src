/*	$NetBSD: cpu_sh.c,v 1.6 2003/07/15 03:35:58 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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

/*
 * SH-5 CPU Module
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu_sh.c,v 1.6 2003/07/15 03:35:58 lukem Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bootparams.h>
#include <machine/cacheops.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/sh5/stb1var.h>
#include <sh5/dev/superhywayvar.h>

#include "locators.h"

static int cpu_shmatch(struct device *, struct cfdata *, void *);
static void cpu_shattach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu_sh, sizeof(struct device),
    cpu_shmatch, cpu_shattach, NULL, NULL);
extern struct cfdriver cpu_cd;

static void cpu_prcache(struct device *, struct sh5_cache_info *, const char *);

/*ARGSUSED*/
static int
cpu_shmatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct superhyway_attach_args *sa = args;

	if (strcmp(sa->sa_name, cpu_cd.cd_name))
		return (0);

	if (cf->cf_loc[SUPERHYWAYCF_PPORT] != bootparams.bp_cpu[0].pport)
		return (0);

	sa->sa_pport = cf->cf_loc[SUPERHYWAYCF_PPORT];

	return (1);
}

/*ARGSUSED*/
static void
cpu_shattach(struct device *parent, struct device *self, void *args)
{
	struct superhyway_attach_args *sa = args;
	bus_space_handle_t bh;
	u_int64_t vcr;
	u_int cpuid, version;
	const char *cpustr;
	char str[64];

	bus_space_map(sa->sa_bust, SUPERHYWAY_PPORT_TO_BUSADDR(sa->sa_pport),
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	/*
	 * There seems to be a hardware bug which causes reads of CPU.VCR
	 * to return zero under certain circumstances.
	 */
	if (vcr == 0) {
		cpuid = bootparams.bp_cpu[0].cpuid;
		version = bootparams.bp_cpu[0].version;
	} else {
		cpuid = SUPERHYWAY_VCR_MOD_ID(vcr);
		version = SUPERHYWAY_VCR_MOD_VERS(vcr);
	}

	switch (cpuid) {
	case SH5_CPUID_STB1:
		cpustr = "SH5 STB1 Evaluation Silicon";
		break;

	default:
		sprintf(str, "Unknown CPU ID: 0x%x", cpuid);
		cpustr = str;
		break;
	}

	printf("\n%s: %s, Version %d, %d.%02d MHz\n", self->dv_xname, cpustr,
	    version, bootparams.bp_cpu[0].speed / 1000000,
	    (bootparams.bp_cpu[0].speed % 1000000) / 10000);

	if (sh5_cache_ops.iinfo.type == SH5_CACHE_INFO_TYPE_NONE) {
		/* Unified cache. */
		cpu_prcache(self, &sh5_cache_ops.dinfo, "Unified");
	} else {
		/* Separate I/D caches */
		cpu_prcache(self, &sh5_cache_ops.dinfo, "D");
		cpu_prcache(self, &sh5_cache_ops.iinfo, "I");
	}
}

static void
cpu_prcache(struct device *dv, struct sh5_cache_info *ci, const char *name)
{
	static const char *ctype[] = {NULL, "VIVT", "VIPT", "PIPT"};
	static const char *wtype[] = {NULL, "thru", "back"};
	int i;

	i = (strcmp(name, "I") == 0);

	if (ci->type == 0 || ci->type >= (sizeof(ctype)/sizeof(char *))) {
		printf("%s: WARNING: Invalid %s-cache type: %d\n",
		    dv->dv_xname, name, ci->type);
		ci->type = SH5_CACHE_INFO_TYPE_VIVT;	/* XXX: Safe default */
	}

	if (i == 0 &&
	    (ci->write == 0 || ci->write >= (sizeof(wtype)/sizeof(char *)))) {
		printf("%s: WARNING: Invalid %s-cache write type: %d\n",
		    dv->dv_xname, name, ci->write);
		ci->write = SH5_CACHE_INFO_WRITE_BACK;	/* XXX: Safe default */
	}

	printf("%s: %s-cache %d KB %db/line %d-way %d-sets %s", dv->dv_xname,
	    name, ci->size / 1024, ci->line_size, ci->nways,
	    ci->nsets, ctype[ci->type]);
 
	if (i == 0)
		printf(" write-%s", wtype[ci->write]);
	printf("\n");
}
