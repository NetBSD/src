/* $NetBSD: cpu.c,v 1.10.4.4 1999/11/19 11:06:27 nisimura Exp $ */

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
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

static int  cpumatch __P((struct device *, struct cfdata *, void *));
static void cpuattach __P((struct device *, struct device *, void *));

struct cfattach cpu_ca = {
	sizeof (struct device), cpumatch, cpuattach
};
extern struct cfdriver cpu_cd;

extern void cpu_identify __P((void));

static int
cpumatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/* make sure that we're looking for a CPU. */
	if (strcmp(ma->ma_name, cpu_cd.cd_name) != 0) {
		return (0);
	}
	return (1);
}

static void
cpuattach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
	printf("\n");

	cpu_identify();
}

#if 0
#include <mips/locore.h>

extern int mips1_icsize __P((void));
extern int mips1_dcsize __P((void));
extern int mips1_iflush __P((vaddr_t, vsize_t));
extern int mips1_dflush __P((vaddr_t, vsize_t));

#define IC_LINE(rr)	(16 << ((rr) >> 5) & 1)
#define IC_SIZE(rr)	(4*1024 << ((rr) >> 12) & 7)
#define DC_LINE(rr)	(16 << ((rr) >> 4) & 1)
#define DC_SIZE(rr)	(4*1024 << ((rr) >>  9) & 7)
#define L2_EXIST(rr)	((rr) & 0x00020000)
#define L2_LINE(rr)	(4 << ((rr) >> 2) & 3)
#define L2_SIZE(rr)	(512*1024 << (((rr) >> 20) & 3))
#define L2_SPLITTED(rr)	((rr) & 0x00200000)

struct cpuinfo {
	int	arch;
	int	cpuprid;
	int	tlbsize;
	int	multiway;		/* multiway assoc. primary cache */
};
struct cpuinfo cpuinfo;

struct cacheinfo {
	int	c_physical;		/* true => cache is physical */
	int	c_split;		/* true => cache is split */
	int	ic_totalsize;		/* instruction cache */
	int	ic_enabled;
	int	ic_linesize;
	int	dc_totalsize;		/* data cache */
	int	dc_enabled;
	int	dc_linesize;
	int	sc_totalsize;		/* secondary cache */
	int	sc_linesize;
	int	sc_enabled;
};
struct cacheinfo cacheinfo;

struct cpuops {
	void	(*flush_tlbentire) __P((void));
	void	(*flush_tlbsingle) __P((vaddr_t));
	void	(*flush_icache) __P((vaddr_t, vsize_t));
	void	(*flush_dcache) __P((vaddr_t, vsize_t));
	void	(*flush_pcache) __P((void));
	void	(*flush_scache) __P((void));
	void	(*drain_wb) __P((void));
	void	(*set_asid) __P((int));
};

void cpuparams __P((void));
void mips1_flushcache __P((void));

void
cpuparams()
{
	u_int32_t cp0r16; /* CP0 config register */

	switch (cpu_id.cpu.cp_imp) {
	case MIPS_R2000:
	case MIPS_R3000:
		cpuinfo.arch = 1;
		cacheinfo.c_physical = 1;
		cacheinfo.ic_totalsize = mips1_icsize();
		cacheinfo.dc_totalsize = mips1_dcsize();
		cacheinfo.sc_totalsize = 0;
		break;
	
	case MIPS_R4000:
		cpuinfo.arch = 3;
		cpuinfo.multiway = 0;
		goto mips3descenders;
	case MIPS_R4200:
	case MIPS_R4300:
	case MIPS_R4100:
		cpuinfo.arch = 3;
		cpuinfo.multiway = 0;
		goto mips3descenders;
	case MIPS_R4600:
	case MIPS_R4700:
		cpuinfo.arch = 3;
		cpuinfo.multiway = 2;
		goto mips3descenders;
	case MIPS_R5000:
	case MIPS_R10000:
		cpuinfo.arch = 4;
		cpuinfo.multiway = 2;
		goto mips3descenders;

	mips3descenders:
		__asm __volatile("mtc0 %0, $16" : "=r"(cp0r16));
		cacheinfo.c_physical = 0;
		cacheinfo.c_split = 0;
		cacheinfo.ic_linesize =	 IC_LINE(cp0r16);
		cacheinfo.ic_totalsize = IC_SIZE(cp0r16);
		cacheinfo.dc_linesize =	 DC_LINE(cp0r16);
		cacheinfo.dc_totalsize = DC_SIZE(cp0r16);
		cacheinfo.sc_totalsize = L2_EXIST(cp0r16) ? -1 : 0;
		cacheinfo.sc_linesize =	 L2_LINE(cp0r16);
		if (cacheinfo.sc_totalsize == -1 && cpuinfo.arch == 4)
			cacheinfo.sc_totalsize = L2_SIZE(cp0r16);
		if (cpuinfo.arch == 3)
			cacheinfo.c_split = L2_SPLITTED(cp0r16);
		break;
	}
	cpu_arch = cpuinfo.arch; /* XXX */
}

void
mips1_flushcache()
{
	mips1_dflush(0x80000000, cacheinfo.dc_totalsize);
	mips1_iflush(0x80000000, cacheinfo.ic_totalsize);
}
#endif
