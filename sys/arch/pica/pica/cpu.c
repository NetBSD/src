/*	$NetBSD: cpu.c,v 1.3 1996/10/10 23:45:22 christos Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Per Fogelstrom
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

#include <machine/cpu.h>
#include <machine/autoconf.h>


/* Definition of the driver for autoconfig. */
static int	cpumatch(struct device *, void *, void *);
static void	cpuattach(struct device *, struct device *, void *);

struct cfattach cpu_ca = {
	sizeof(struct device), cpumatch, cpuattach
};

struct cfdriver cpu_cd = {
	NULL, "cpu", DV_DULL
};

static int	cpuprint __P((void *, char *pnp));

static int
cpumatch(parent, cfdata, aux)
	struct device *parent;
	void *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;
	struct confargs *ca = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ca->ca_name, cpu_cd.cd_name) != 0)
		return (0);

	return (1);
}

static void
cpuattach(parent, dev, aux)
	struct device *parent;
	struct device *dev;
	void *aux;
{
        struct pcs *p;
	int needcomma, needrev, i;

	kprintf(": ");

	switch(cpu_id.cpu.cp_imp) {

	case MIPS_R2000:
		kprintf("MIPS R2000 CPU");
		break;
	case MIPS_R3000:
		kprintf("MIPS R3000 CPU");
		break;
	case MIPS_R6000:
		kprintf("MIPS R6000 CPU");
		break;
	case MIPS_R4000:
		if(machPrimaryInstCacheSize == 16384)
			kprintf("MIPS R4400 CPU");
		else
			kprintf("MIPS R4000 CPU");
		break;
	case MIPS_R3LSI:
		kprintf("LSI Logic R3000 derivate");
		break;
	case MIPS_R6000A:
		kprintf("MIPS R6000A CPU");
		break;
	case MIPS_R3IDT:
		kprintf("IDT R3000 derivate");
		break;
	case MIPS_R10000:
		kprintf("MIPS R10000/T5 CPU");
		break;
	case MIPS_R4200:
		kprintf("MIPS R4200 CPU (ICE)");
		break;
	case MIPS_R8000:
		kprintf("MIPS R8000 Blackbird/TFP CPU");
		break;
	case MIPS_R4600:
		kprintf("QED R4600 Orion CPU");
		break;
	case MIPS_R3SONY:
		kprintf("Sony R3000 based CPU");
		break;
	case MIPS_R3TOSH:
		kprintf("Toshiba R3000 based CPU");
		break;
	case MIPS_R3NKK:
		kprintf("NKK R3000 based CPU");
		break;
	case MIPS_UNKC1:
	case MIPS_UNKC2:
	default:
		kprintf("Unknown CPU type (0x%x)",cpu_id.cpu.cp_imp);
		break;
	}
	kprintf(" Rev. %d.%d with ", cpu_id.cpu.cp_majrev, cpu_id.cpu.cp_minrev);


	switch(fpu_id.cpu.cp_imp) {

	case MIPS_SOFT:
		kprintf("Software emulation float");
		break;
	case MIPS_R2360:
		kprintf("MIPS R2360 FPC");
		break;
	case MIPS_R2010:
		kprintf("MIPS R2010 FPC");
		break;
	case MIPS_R3010:
		kprintf("MIPS R3010 FPC");
		break;
	case MIPS_R6010:
		kprintf("MIPS R6010 FPC");
		break;
	case MIPS_R4010:
		kprintf("MIPS R4010 FPC");
		break;
	case MIPS_R31LSI:
		kprintf("FPC");
		break;
	case MIPS_R10010:
		kprintf("MIPS R10000/T5 FPU");
		break;
	case MIPS_R4210:
		kprintf("MIPS R4200 FPC (ICE)");
	case MIPS_R8000:
		kprintf("MIPS R8000 Blackbird/TFP");
		break;
	case MIPS_R4600:
		kprintf("QED R4600 Orion FPC");
		break;
	case MIPS_R3SONY:
		kprintf("Sony R3000 based FPC");
		break;
	case MIPS_R3TOSH:
		kprintf("Toshiba R3000 based FPC");
		break;
	case MIPS_R3NKK:
		kprintf("NKK R3000 based FPC");
		break;
	case MIPS_UNKF1:
	default:
		kprintf("Unknown FPU type (0x%x)", fpu_id.cpu.cp_imp);
		break;
	}
	kprintf(" Rev. %d.%d", fpu_id.cpu.cp_majrev, fpu_id.cpu.cp_minrev);
	kprintf("\n");

	kprintf("        Primary cache size: %dkb Instruction, %dkb Data.\n",
		machPrimaryInstCacheSize / 1024,
		machPrimaryDataCacheSize / 1024);
}

