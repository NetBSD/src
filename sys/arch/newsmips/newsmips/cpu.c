/*	$NetBSD: cpu.c,v 1.4 2001/06/27 08:20:45 nisimura Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

/* Definition of the driver for autoconfig. */
static int	cpumatch(struct device *, struct cfdata *, void *);
static void	cpuattach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

extern struct cfdriver cpu_cd;

static int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct confargs *ca = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0) {
		return 0;
	}
	return 1;
}

static void
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{
	char *cpu_name, *fpu_name;

	printf("\n");
	switch (MIPS_PRID_IMPL(cpu_id)) {
	case MIPS_R3000:
		cpu_name = "MIPS R3000 CPU";
		break;
	case MIPS_R4000:
		cpu_name = "MIPS R4000 CPU";
		break;
	default:
		cpu_name = "Unknown CPU";
	}
	if (MIPS_PRID_IMPL(cpu_id) == MIPS_R4000
	    && mips_L1ICacheSize == 16384)
		cpu_name = "MIPS R4400 CPU";

	switch (MIPS_PRID_IMPL(fpu_id)) {
	case MIPS_R3010:
		fpu_name = "MIPS R3010 FPA";
		break;
	case MIPS_R4010:
		fpu_name = "MIPS R4010 FPA";
		break;
	default:
		fpu_name = "unknown FPA";
		break;
	}
	printf("%s: %s (0x%04x) with %s (0x%04x)\n",
	    dev->dv_xname, cpu_name, cpu_id, fpu_name, fpu_id);
	if (MIPS_PRID_IMPL(cpu_id) != MIPS_R4000) {
		printf("%s: ", dev->dv_xname);
		printf("%dKB Instruction, %dKB Data, direct mapped cache\n",
		    mips_L1ICacheSize/1024, mips_L1DCacheSize/1024);
	}
	else {
		printf("%s: L1 cache: ", dev->dv_xname);
		printf("%dKB Instruction, %dKB Data, direct mapped\n",
		    mips_L1ICacheSize/1024, mips_L1DCacheSize/1024);
		printf("%s: ", dev->dv_xname);
		
		if (!mips_L2CachePresent)
			printf("no L2 cache\n");
		else
			printf("L2 cache: %dKB/%dB %s, %s\n",
			    mips_L2CacheSize/1024, mips_L2CacheLSize,
			    mips_L2CacheMixed ? "mixed" : "separated",
			    mips_L2CacheIsSnooping? "snooping" : "no snooping");
	}
}
