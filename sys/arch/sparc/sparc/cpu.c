/*	$NetBSD: cpu.c,v 1.38 1997/04/11 20:32:13 pk Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Aaron Brown and
 *	Harvard University.
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)cpu.c	8.5 (Berkeley) 11/23/93
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/ctlreg.h>
#include <machine/trap.h>
#include <machine/pmap.h>

#include <machine/oldmon.h>
#include <machine/idprom.h>

#include <sparc/sparc/cache.h>
#include <sparc/sparc/asm.h>
#include <sparc/sparc/cpuvar.h>

/* The following are used externally (sysctl_hw). */
char	machine[] = "sparc";
char	cpu_model[100];

/* The CPU configuration driver. */
static void cpu_attach __P((struct device *, struct device *, void *));
int  cpu_match __P((struct device *, struct cfdata *, void *));

struct cfattach cpu_ca = {
	sizeof(struct cpu_softc), cpu_match, cpu_attach
};

struct cfdriver cpu_cd = {
	NULL, "cpu", DV_CPU
};

static char *fsrtoname __P((int, int, int, char *));
void cache_print __P((struct cpu_softc *));
void cpu_spinup __P((struct cpu_softc *));

#define	IU_IMPL(psr)	((u_int)(psr) >> 28)
#define	IU_VERS(psr)	(((psr) >> 24) & 0xf)

#define SRMMU_IMPL(mmusr)	((u_int)(mmusr) >> 28)
#define SRMMU_VERS(mmusr)	(((mmusr) >> 24) & 0xf)


#ifdef notdef
/*
 * IU implementations are parceled out to vendors (with some slight
 * glitches).  Printing these is cute but takes too much space.
 */
static char *iu_vendor[16] = {
	"Fujitsu",	/* and also LSI Logic */
	"ROSS",		/* ROSS (ex-Cypress) */
	"BIT",
	"LSIL",		/* LSI Logic finally got their own */
	"TI",		/* Texas Instruments */
	"Matsushita",
	"Philips",
	"Harvest",	/* Harvest VLSI Design Center */
	"SPEC",		/* Systems and Processes Engineering Corporation */
	"Weitek",
	"vendor#10",
	"vendor#11",
	"vendor#12",
	"vendor#13",
	"vendor#14",
	"vendor#15"
};
#endif

/*
 * 4/110 comment: the 4/110 chops off the top 4 bits of an OBIO address.
 *	this confuses autoconf.  for example, if you try and map
 *	0xfe000000 in obio space on a 4/110 it actually maps 0x0e000000.
 *	this is easy to verify with the PROM.   this causes problems
 *	with devices like "esp0 at obio0 addr 0xfa000000" because the
 *	4/110 treats it as esp0 at obio0 addr 0x0a000000" which is the
 *	address of the 4/110's "sw0" scsi chip.   the same thing happens
 *	between zs1 and zs2.    since the sun4 line is "closed" and
 *	we know all the "obio" devices that will ever be on it we just
 *	put in some special case "if"'s in the match routines of esp,
 *	dma, and zs.
 */

int
cpu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct confargs *ca = aux;

	return (strcmp(cf->cf_driver->cd_name, ca->ca_ra.ra_name) == 0);
}

/*
 * Attach the CPU.
 * Discover interesting goop about the virtual address cache
 * (slightly funny place to do it, but this is where it is to be found).
 */
static void
cpu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cpu_softc *sc = (struct cpu_softc *)self;
	register int node;
	register char *fpuname;
	struct confargs *ca = aux;
	struct fpstate fpstate;
	char fpbuf[40];

	sc->node = node = ca->ca_ra.ra_node;

	/*
	 * First, find out if we're attaching the boot CPU.
	 */
	if (node == 0)
		sc->master = 1;
	else {
		sc->mid = getpropint(node, "mid", 0);
		if (sc->mid == 0 || sc->mid == getmid() + 8 /*XXX*/)
			sc->master = 1;
	}

	if (sc->master) {
		/*
		 * Gross, but some things in cpuinfo may already have
		 * been setup by early routines like pmap_bootstrap().
		 */
		bcopy(&sc->dv, &cpuinfo, sizeof(sc->dv));
		bcopy(&cpuinfo, sc, sizeof(cpuinfo));
	}

	getcpuinfo(sc, node);

	/*
	 * Get the FSR and clear any exceptions.  If we do not unload
	 * the queue here and it is left over from a previous crash, we
	 * will panic in the first loadfpstate(), due to a sequence error,
	 * so we need to dump the whole state anyway.
	 *
	 * If there is no FPU, trap.c will advance over all the stores,
	 * so we initialize fs_fsr here.
	 */
	fpstate.fs_fsr = 7 << FSR_VER_SHIFT;	/* 7 is reserved for "none" */
	savefpstate(&fpstate);
	sc->fpuvers =
		(fpstate.fs_fsr >> FSR_VER_SHIFT) & (FSR_VER >> FSR_VER_SHIFT);

	if (sc->fpuvers != 7) {
		foundfpu = 1;
		fpuname = fsrtoname(sc->cpu_impl, sc->cpu_vers,
				    sc->fpuvers, fpbuf);
	} else
		fpuname = "no";

	sprintf(cpu_model, "%s @ %s MHz, %s FPU",
		sc->cpu_name,
		clockfreq(sc->hz), fpuname);
	printf(": %s\n", cpu_model);

	if (sc->cacheinfo.c_totalsize != 0)
		cache_print(sc);

	if (sc->master) {
		bcopy(sc, &cpuinfo, sizeof(cpuinfo));
		/* Enable the cache */
		if (sc->hotfix)
			sc->hotfix(sc);
		sc->cache_enable();
		return;
	}

	/* Now start this CPU */
}

#if 0
void
cpu_(sc)
	struct cpu_softc *sc;
{
	if (sc->hotfix)
		sc->hotfix(sc);
	/* Enable the cache */
	sc->cache_enable();
}
#endif

void
cpu_spinup(sc)
	struct cpu_softc *sc;
{
#if 0
	pmap_cpusetup();
#endif
}


void
cache_print(sc)
	struct cpu_softc *sc;
{
	char *sep;

	sep = " ";
	if (sc->cacheinfo.c_split) {
		printf("%s:%s", sc->dv.dv_xname, 
		       (sc->cacheinfo.c_physical ? " physical" : ""));
		if (sc->cacheinfo.ic_totalsize > 0) {
			printf("%s%dK instruction (%d b/l)", sep,
			       sc->cacheinfo.ic_totalsize/1024,
			       sc->cacheinfo.ic_linesize);
			sep = ", ";
		}
		if (sc->cacheinfo.dc_totalsize > 0) {
			printf("%s%dK data (%d b/l)", sep,
			       sc->cacheinfo.dc_totalsize/1024,
			       sc->cacheinfo.dc_linesize);
			sep = ", ";
		}
		printf(" ");
	} else if (sc->cacheinfo.c_physical) {
		/* combined, physical */
		printf("%s: physical %dK combined cache (%d bytes/line) ",
		       sc->dv.dv_xname,
		       sc->cacheinfo.c_totalsize/1024,
		       sc->cacheinfo.c_linesize);
	} else {
		/* combined, virtual */
		printf("%s: %d byte write-%s, %d bytes/line, %cw flush ",
		    sc->dv.dv_xname, sc->cacheinfo.c_totalsize,
		    (sc->cacheinfo.c_vactype == VAC_WRITETHROUGH) 
		    ? "through" : "back",
		    sc->cacheinfo.c_linesize,
		    sc->cacheinfo.c_hwflush ? 'h' : 's');
	}

	if (sc->cacheinfo.ec_totalsize > 0) {
		printf(", %dK external (%d b/l)",
		       sc->cacheinfo.ec_totalsize/1024,
		       sc->cacheinfo.ec_linesize);
	}
}


/*------------*/


void cpumatch_unknown __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_sun4 __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_sun4c __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_viking __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_ms1 __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_ms2 __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_swift __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_hypersparc __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_cypress __P((struct cpu_softc *, struct module_info *, int));
void cpumatch_turbosparc __P((struct cpu_softc *, struct module_info *, int));

void getcacheinfo_sun4 __P((struct cpu_softc *, int node));
void getcacheinfo_sun4c __P((struct cpu_softc *, int node));
void getcacheinfo_obp __P((struct cpu_softc *, int node));

void sun4_hotfix __P((struct cpu_softc *));
void viking_hotfix __P((struct cpu_softc *));

void ms1_mmu_enable __P((void));
void viking_mmu_enable __P((void));
void swift_mmu_enable __P((void));
void hypersparc_mmu_enable __P((void));

void srmmu_get_fltstatus __P((void));
void ms1_get_fltstatus __P((void));
void viking_get_fltstatus __P((void));
void swift_get_fltstatus __P((void));
void hypersparc_get_fltstatus __P((void));
void cypress_get_fltstatus __P((void));

struct module_info module_unknown = {
	cpumatch_unknown
};


void
cpumatch_unknown(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	panic("Unknown CPU type: "
	      "cpu: impl %d, vers %d; mmu: impl %d, vers %d",
		sc->cpu_impl, sc->cpu_vers,
		sc->mmu_impl, sc->mmu_vers);
}

#if defined(SUN4)
struct module_info module_sun4 = {
	cpumatch_sun4,
	getcacheinfo_sun4,
	sun4_hotfix,
	0,
	sun4_cache_enable,
	0,			/* ncontext set in `match' function */
	0,			/* get fault regs: unused */
	sun4_cache_flush,
	sun4_vcache_flush_page,
	sun4_vcache_flush_segment,
	sun4_vcache_flush_region,
	sun4_vcache_flush_context,
	noop_pcache_flush_line
};

void
getcacheinfo_sun4(sc, node)
	struct cpu_softc *sc;
	int	node;
{
	switch (sc->cpu_type) {
	case CPUTYP_4_100:
		sc->cacheinfo.c_vactype = VAC_NONE;
		sc->cacheinfo.c_totalsize = 0;
		sc->cacheinfo.c_hwflush = 0;
		sc->cacheinfo.c_linesize = 0;
		sc->cacheinfo.c_l2linesize = 0;
		sc->cacheinfo.c_split = 0;
		sc->cacheinfo.c_nlines = 0;

		/* Override cache flush functions */
		sc->cache_flush = noop_cache_flush;
		sc->vcache_flush_page = noop_vcache_flush_page;
		sc->vcache_flush_segment = noop_vcache_flush_segment;
		sc->vcache_flush_region = noop_vcache_flush_region;
		sc->vcache_flush_context = noop_vcache_flush_context;
		break;
	case CPUTYP_4_200:
		sc->cacheinfo.c_vactype = VAC_WRITEBACK;
		sc->cacheinfo.c_totalsize = 128*1024;
		sc->cacheinfo.c_hwflush = 0;
		sc->cacheinfo.c_linesize = 16;
		sc->cacheinfo.c_l2linesize = 4;
		sc->cacheinfo.c_split = 0;
		sc->cacheinfo.c_nlines =
			sc->cacheinfo.c_totalsize << sc->cacheinfo.c_l2linesize;
		break;
	case CPUTYP_4_300:
		sc->cacheinfo.c_vactype = VAC_WRITEBACK;
		sc->cacheinfo.c_totalsize = 128*1024;
		sc->cacheinfo.c_hwflush = 0;
		sc->cacheinfo.c_linesize = 16;
		sc->cacheinfo.c_l2linesize = 4;
		sc->cacheinfo.c_split = 0;
		sc->cacheinfo.c_nlines =
			sc->cacheinfo.c_totalsize << sc->cacheinfo.c_l2linesize;
		sc->flags |= CPUFLG_SUN4CACHEBUG;
		break;
	case CPUTYP_4_400:
		sc->cacheinfo.c_vactype = VAC_WRITEBACK;
		sc->cacheinfo.c_totalsize = 128 * 1024;
		sc->cacheinfo.c_hwflush = 0;
		sc->cacheinfo.c_linesize = 32;
		sc->cacheinfo.c_l2linesize = 5;
		sc->cacheinfo.c_split = 0;
		sc->cacheinfo.c_nlines =
			sc->cacheinfo.c_totalsize << sc->cacheinfo.c_l2linesize;
		break;
	}
}

struct	idprom idprom;
void	getidprom __P((struct idprom *, int size));

void
cpumatch_sun4(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{

	getidprom(&idprom, sizeof(idprom));
	switch (idprom.id_machine) {
	/* XXX: don't know about Sun4 types */
	case ID_SUN4_100:
		sc->cpu_type = CPUTYP_4_100;
		sc->classlvl = 100;
		sc->mmu_ncontext = 8;
		sc->mmu_nsegment = 256;
/*XXX*/		sc->hz = 0;
		break;
	case ID_SUN4_200:
		sc->cpu_type = CPUTYP_4_200;
		sc->classlvl = 200;
		sc->mmu_nsegment = 512;
		sc->mmu_ncontext = 16;
/*XXX*/		sc->hz = 0;
		break;
	case ID_SUN4_300:
		sc->cpu_type = CPUTYP_4_300;
		sc->classlvl = 300;
		sc->mmu_nsegment = 256;
		sc->mmu_ncontext = 16;
/*XXX*/		sc->hz = 0;
		break;
	case ID_SUN4_400:
		sc->cpu_type = CPUTYP_4_400;
		sc->classlvl = 400;
		sc->mmu_nsegment = 1024;
		sc->mmu_ncontext = 64;
		sc->mmu_nregion = 256;
/*XXX*/		sc->hz = 0;
		sc->sun4_mmu3l = 1;
		break;
	}

}
#endif /* SUN4 */

#if defined(SUN4C)
struct module_info module_sun4c = {
	cpumatch_sun4c,
	getcacheinfo_sun4c,
	sun4_hotfix,
	0,
	sun4_cache_enable,
	0,			/* ncontext set in `match' function */
	0,
	sun4_cache_flush,
	sun4_vcache_flush_page,
	sun4_vcache_flush_segment,
	sun4_vcache_flush_region,
	sun4_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_sun4c(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	int	rnode;

	rnode = findroot();
	sc->mmu_npmeg = sc->mmu_nsegment =
		getpropint(rnode, "mmu-npmg", 128);
	sc->mmu_ncontext = getpropint(rnode, "mmu-nctx", 8);
                              
	/* Get clock frequency */ 
	sc->hz = getpropint(rnode, "clock-frequency", 0);
}

void
getcacheinfo_sun4c(sc, node)
	struct cpu_softc *sc;
	int node;
{
	int i, l;

	if (node == 0)
		/* Bootstrapping */
		return;

	/* Sun4c's have only virtually-addressed caches */
	sc->cacheinfo.c_physical = 0; 
	sc->cacheinfo.c_totalsize = getpropint(node, "vac-size", 65536);
	/*
	 * Note: vac-hwflush is spelled with an underscore
	 * on the 4/75s.
	 */
	sc->cacheinfo.c_hwflush =
		getpropint(node, "vac_hwflush", 0) |
		getpropint(node, "vac-hwflush", 0);

	sc->cacheinfo.c_linesize = l = getpropint(node, "vac-linesize", 16);
	for (i = 0; (1 << i) < l; i++)
		/* void */;
	if ((1 << i) != l)
		panic("bad cache line size %d", l);
	sc->cacheinfo.c_l2linesize = i;
	sc->cacheinfo.c_associativity = 1;
	sc->cacheinfo.c_nlines = sc->cacheinfo.c_totalsize << i;

	sc->cacheinfo.c_vactype = VAC_WRITETHROUGH;

	/*
	 * Machines with "buserr-type" 1 have a bug in the cache
	 * chip that affects traps.  (I wish I knew more about this
	 * mysterious buserr-type variable....)
	 */
	if (getpropint(node, "buserr-type", 0) == 1)
		sc->flags |= CPUFLG_SUN4CACHEBUG;
}
#endif /* SUN4C */

void
sun4_hotfix(sc)
	struct cpu_softc *sc;
{
	if ((sc->flags & CPUFLG_SUN4CACHEBUG) != 0) {
		kvm_uncache((caddr_t)trapbase, 1);
		printf("cache chip bug; trap page uncached ");
	}

}

#if defined(SUN4M)
void
getcacheinfo_obp(sc, node)
	struct cpu_softc *sc;
	int	node;
{
	int i, l;

	if (node == 0)
		/* Bootstrapping */
		return;

	/*
	 * Determine the Sun4m cache organization
	 */

	sc->cacheinfo.c_physical = node_has_property(node, "cache-physical?");
	if (sc->cacheinfo.c_physical == 0 &&
	    sc->cacheinfo.c_vactype == VAC_NONE)
		sc->cacheinfo.c_vactype = VAC_WRITETHROUGH; /*???*/

	if (getpropint(node, "ncaches", 1) == 2)
		sc->cacheinfo.c_split = 1;
	else
		sc->cacheinfo.c_split = 0;

	/* hwflush is used only by sun4/4c code */
	sc->cacheinfo.c_hwflush = 0; 

	if (node_has_property(node, "icache-nlines") &&
	    node_has_property(node, "dcache-nlines") &&
	    sc->cacheinfo.c_split) {
		/* Harvard architecture: get I and D cache sizes */
		sc->cacheinfo.ic_nlines = getpropint(node, "icache-nlines", 0);
		sc->cacheinfo.ic_linesize = l =
			getpropint(node, "icache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad icache line size %d", l);
		sc->cacheinfo.ic_l2linesize = i;
		sc->cacheinfo.ic_associativity =
			getpropint(node, "icache-associativity", 1);
		sc->cacheinfo.ic_totalsize = l *
			sc->cacheinfo.ic_nlines *
			sc->cacheinfo.ic_associativity;
	
		sc->cacheinfo.dc_nlines = getpropint(node, "dcache-nlines", 0);
		sc->cacheinfo.dc_linesize = l =
			getpropint(node, "dcache-line-size",0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad dcache line size %d", l);
		sc->cacheinfo.dc_l2linesize = i;
		sc->cacheinfo.dc_associativity =
			getpropint(node, "dcache-associativity", 1);
		sc->cacheinfo.dc_totalsize = l *
			sc->cacheinfo.dc_nlines *
			sc->cacheinfo.dc_associativity;

		sc->cacheinfo.c_l2linesize =
			min(sc->cacheinfo.ic_l2linesize,
			    sc->cacheinfo.dc_l2linesize);
		sc->cacheinfo.c_linesize =
			min(sc->cacheinfo.ic_linesize,
			    sc->cacheinfo.dc_linesize);
		sc->cacheinfo.c_totalsize =
			sc->cacheinfo.ic_totalsize +
			sc->cacheinfo.dc_totalsize;
	} else {
		/* unified I/D cache */
		sc->cacheinfo.c_nlines = getpropint(node, "cache-nlines", 128);
		sc->cacheinfo.c_linesize = l = 
			getpropint(node, "cache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad cache line size %d", l);
		sc->cacheinfo.c_l2linesize = i;
		sc->cacheinfo.c_totalsize = l *
			sc->cacheinfo.c_nlines *
			getpropint(node, "cache-associativity", 1);
	}
	
	if (node_has_property(node, "ecache-nlines")) {
		/* we have a L2 "e"xternal cache */
		sc->cacheinfo.ec_nlines =
			getpropint(node, "ecache-nlines", 32768);
		sc->cacheinfo.ec_linesize = l =
			getpropint(node, "ecache-line-size", 0);
		for (i = 0; (1 << i) < l && l; i++)
			/* void */;
		if ((1 << i) != l && l)
			panic("bad ecache line size %d", l);
		sc->cacheinfo.ec_l2linesize = i;
		sc->cacheinfo.ec_associativity =
			getpropint(node, "ecache-associativity", 1);
		sc->cacheinfo.ec_totalsize = l *
			sc->cacheinfo.ec_nlines *
			sc->cacheinfo.ec_associativity;
	}
	if (sc->cacheinfo.c_totalsize == 0)
		printf("warning: couldn't identify cache\n");
}

/*
 * We use the max. number of contexts on the micro and
 * hyper SPARCs. The SuperSPARC would let us use up to 65536
 * contexts (by powers of 2), but we keep it at 4096 since
 * the table must be aligned to #context*4. With 4K contexts,
 * we waste at most 16K of memory. Note that the context
 * table is *always* page-aligned, so there can always be
 * 1024 contexts without sacrificing memory space (given
 * that the chip supports 1024 contexts).
 *
 * Currently known limits: MS1=64, MS2=256, HS=4096, SS=65536
 * 	some old SS's=4096
 */

/* TI Microsparc I */
struct module_info module_ms1 = {
	cpumatch_ms1,
	getcacheinfo_obp,
	0,
	ms1_mmu_enable,
	ms1_cache_enable,
	64,
	ms1_get_fltstatus,
	ms1_cache_flush,
	noop_vcache_flush_page,
	noop_vcache_flush_segment,
	noop_vcache_flush_region,
	noop_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_ms1(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	sc->flags |= CPUFLG_CACHEPAGETABLES;
	sc->cpu_type = CPUTYP_MS1;
}

void
ms1_mmu_enable()
{
}

/* TI Microsparc II */
struct module_info module_ms2 = {		/* UNTESTED */
	cpumatch_ms2,
	getcacheinfo_obp,
	0,
	0,
	swift_cache_enable,
	256,
	srmmu_get_fltstatus,
	srmmu_cache_flush,
	srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region,
	srmmu_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_ms2(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	sc->flags |= CPUFLG_CACHEPAGETABLES;
	sc->cpu_type = CPUTYP_MS2;
}


struct module_info module_swift = {		/* UNTESTED */
	cpumatch_swift,
	getcacheinfo_obp,
	0,
	0,
	swift_cache_enable,
	256,
	swift_get_fltstatus,
	srmmu_cache_flush,
	srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region,
	srmmu_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_swift(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	sc->flags |= CPUFLG_CACHEPAGETABLES;
	sc->cpu_type = CPUTYP_MS2;
}

void
swift_mmu_enable()
{
}

struct module_info module_viking = {		/* UNTESTED */
	cpumatch_viking,
	getcacheinfo_obp,
	viking_hotfix,
	viking_mmu_enable,
	viking_cache_enable,
	4096,
	viking_get_fltstatus,
	/* supersparcs use cached DVMA, no need to flush */
	noop_cache_flush,
	noop_vcache_flush_page,
	noop_vcache_flush_segment,
	noop_vcache_flush_region,
	noop_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_viking(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	if (node == 0)
		viking_hotfix(sc);
}

void
viking_hotfix(sc)
	struct cpu_softc *sc;
{
	/* Test if we're directly on the MBus */
	if (!(lda(SRMMU_PCR, ASI_SRMMU) & VIKING_PCR_MB)) {
		sc->mxcc = 1;
		sc->flags |= CPUFLG_CACHEPAGETABLES;
		sc->flags |= CPUFLG_CACHE_MANDATORY;
	} else {
		sc->cache_flush = viking_cache_flush;
		sc->pcache_flush_line = viking_pcache_flush_line;
	}

	/* XXX! */
	if (sc->mxcc)
		sc->cpu_type = CPUTYP_SS1_MBUS_MXCC;
	else
		sc->cpu_type = CPUTYP_SS1_MBUS_NOMXCC;
}

void
viking_mmu_enable()
{
	int pcr;

	/*
	 * Before enabling the MMU, flush the data cache so the
	 * in-memory MMU page tables are up-to-date.
	 */
	sta(0x80000000, ASI_DCACHECLR, 0);      /* Unlock */
	sta(0, ASI_DCACHECLR, 0);

	pcr = lda(SRMMU_PCR, ASI_SRMMU);
	if (cpuinfo.mxcc)
		pcr |= VIKING_PCR_TC;
	else
		pcr &= ~VIKING_PCR_TC;
	sta(SRMMU_PCR, ASI_SRMMU, pcr);
}


/* ROSS Hypersparc */
struct module_info module_hypersparc = {		/* UNTESTED */
	cpumatch_hypersparc,
	getcacheinfo_obp,
	0,
	hypersparc_mmu_enable,
	hypersparc_cache_enable,
	4096,
	hypersparc_get_fltstatus,
	srmmu_cache_flush,
	srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region,
	srmmu_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_hypersparc(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	sc->cpu_type = CPUTYP_HS_MBUS;/*XXX*/
}

void
hypersparc_mmu_enable()
{
	int pcr;

	pcr = lda(SRMMU_PCR, ASI_SRMMU);
	pcr |= HYPERSPARC_PCR_C;
	pcr &= ~HYPERSPARC_PCR_CE;

	sta(SRMMU_PCR, ASI_SRMMU, pcr);
}

/* Cypress 605 */
struct module_info module_cypress = {		/* UNTESTED */
	cpumatch_cypress,
	getcacheinfo_obp,
	0,
	0,
	cypress_cache_enable,
	4096,
	cypress_get_fltstatus,
	srmmu_cache_flush,
	srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region,
	srmmu_vcache_flush_context,
	cypress_pcache_flush_line
};

void
cpumatch_cypress(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	sc->cpu_type = CPUTYP_CYPRESS;
	if (node)
		/* Put in write-thru mode for now */
		sc->cacheinfo.c_vactype = VAC_WRITEBACK;
}

/* Fujitsu Turbosparc */
struct module_info module_turbosparc = {	/* UNTESTED */
	cpumatch_turbosparc,
	getcacheinfo_obp,
	0,
	0,
	turbosparc_cache_enable,
	4096,
	srmmu_get_fltstatus,
	srmmu_cache_flush,
	srmmu_vcache_flush_page,
	srmmu_vcache_flush_segment,
	srmmu_vcache_flush_region,
	srmmu_vcache_flush_context,
	noop_pcache_flush_line
};

void
cpumatch_turbosparc(sc, mp, node)
	struct cpu_softc *sc;
	struct module_info *mp;
	int	node;
{
	sc->flags |= CPUFLG_CACHEPAGETABLES;
	sc->cpu_type = CPUTYP_MS2;
}

#endif /* SUN4M */


#define	ANY	-1	/* match any version */

struct cpu_conf {
	int	arch;
	int	cpu_impl;
	int	cpu_vers;
	int	mmu_impl;
	int	mmu_vers;
	char	*name;
	struct	module_info *minfo;
} cpu_conf[] = {
#if defined(SUN4)
	{ CPU_SUN4, 0, 0, ANY, ANY, "MB86900/1A or L64801", &module_sun4 },
	{ CPU_SUN4, 1, 0, ANY, ANY, "L64811", &module_sun4 },
#endif

#if defined(SUN4C)
	{ CPU_SUN4C, 0, 0, ANY, ANY, "MB86900/1A or L64801", &module_sun4c },
	{ CPU_SUN4C, 1, 0, ANY, ANY, "L64811", &module_sun4c },
	{ CPU_SUN4C, 1, 1, ANY, ANY, "CY7C601", &module_sun4c },
	{ CPU_SUN4C, 9, 0, ANY, ANY, "W8601/8701 or MB86903", &module_sun4c },
#endif

#if defined(SUN4M)
	{ CPU_SUN4M, 0, 4, 0, 4, "MB86904", &module_swift },
	{ CPU_SUN4M, 0, 5, 0, 5, "MB86907", &module_turbosparc },
	{ CPU_SUN4M, 1, 1, 1, 0, "CY7C601/604", &module_cypress },
	{ CPU_SUN4M, 1, 1, 1, 0xb, "CY7C601/605 (v.b)", &module_cypress },
	{ CPU_SUN4M, 1, 1, 1, 0xc, "CY7C601/605 (v.c)", &module_cypress },
	{ CPU_SUN4M, 1, 1, 1, 0xf, "CY7C601/605 (v.f)", &module_cypress },
	{ CPU_SUN4M, 1, 3, 1, ANY, "CY7C611", &module_cypress },
	{ CPU_SUN4M, 1, 0xf, 1, 1, "RT620/625", &module_hypersparc },
	{ CPU_SUN4M, 4, 0, 0, ANY, "TMS390Z50 v0 or TMS390Z55", &module_viking },
	{ CPU_SUN4M, 4, 1, 0, ANY, "TMS390Z50 v1", &module_viking },
	{ CPU_SUN4M, 4, 1, 4, ANY, "TMS390S10", &module_ms1 },
	{ CPU_SUN4M, 4, 2, 0, ANY, "TI_MS2", &module_ms2 },
	{ CPU_SUN4M, 4, 3, ANY, ANY, "TI_4_3", &module_viking },
	{ CPU_SUN4M, 4, 4, ANY, ANY, "TI_4_4", &module_viking },
#endif

	{ ANY, ANY, ANY, ANY, ANY, "Unknown", &module_unknown }
};

void
getcpuinfo(sc, node)
	struct cpu_softc *sc;
	int	node;
{
	struct cpu_conf *mp;
	int i;
	int cpu_impl, cpu_vers;
	int mmu_impl, mmu_vers;

	/*
	 * Set up main criteria for selection from the CPU configuration
	 * table: the CPU implementation/version fields from the PSR
	 * register, and -- on sun4m machines -- the MMU
	 * implementation/version from the SCR register.
	 */
	if (sc->master) {
		i = getpsr();
		cpu_impl = IU_IMPL(i);
		cpu_vers = IU_VERS(i);
		if (CPU_ISSUN4M) {
			i = lda(SRMMU_PCR, ASI_SRMMU);
			mmu_impl = SRMMU_IMPL(i);
			mmu_vers = SRMMU_VERS(i);
		} else {
			mmu_impl = ANY;
			mmu_vers = ANY;
		}
	} else {
		/*
		 * Get CPU version/implementation from ROM. If not
		 * available, assume same as boot CPU.
		 */
		cpu_impl = getpropint(node, "cpu-implementation", -1);
		if (cpu_impl == -1)
			cpu_impl = cpuinfo.cpu_impl;
		cpu_vers = getpropint(node, "cpu-version", -1);
		if (cpu_vers == -1)
			cpu_vers = cpuinfo.cpu_vers;

		/* Get MMU version/implementation from ROM always */
		mmu_impl = getpropint(node, "implementation", -1);
		mmu_vers = getpropint(node, "version", -1);
	}

	for (mp = cpu_conf; ; mp++) {
		if (mp->arch != cputyp && mp->arch != ANY)
			continue;

#define MATCH(x)	(mp->x == x || mp->x == ANY)
		if (!MATCH(cpu_impl) ||
		    !MATCH(cpu_vers) ||
		    !MATCH(mmu_impl) ||
		    !MATCH(mmu_vers))
			continue;
#undef MATCH

		/*
		 * Got CPU type.
		 */
		sc->cpu_impl = cpu_impl;
		sc->cpu_vers = cpu_vers;
		sc->mmu_impl = mmu_impl;
		sc->mmu_vers = mmu_vers;

		if (mp->minfo->cpu_match) {
			/* Additional fixups */
			mp->minfo->cpu_match(sc, mp->minfo, node);
		}
		if (sc->cpu_name == 0)
			sc->cpu_name = mp->name;

		if (sc->mmu_ncontext == 0)
			sc->mmu_ncontext = mp->minfo->ncontext;

		mp->minfo->getcacheinfo(sc, node);

		if (node && sc->hz == 0 && !CPU_ISSUN4/*XXX*/) {
			sc->hz = getpropint(node, "clock-frequency", 0);
			if (sc->hz == 0) {
				/*
				 * Try to find it in the OpenPROM root...
				 */     
				sc->hz = getpropint(findroot(),
							"clock-frequency", 0);
			}
		}

		/*
		 * Copy CPU/MMU/Cache specific routines into cpu_softc.
		 */
#define MPCOPY(x)	if (sc->x == 0) sc->x = mp->minfo->x;
		MPCOPY(hotfix);
		MPCOPY(mmu_enable);
		MPCOPY(cache_enable);
		MPCOPY(get_faultstatus);
		MPCOPY(cache_flush);
		MPCOPY(vcache_flush_page);
		MPCOPY(vcache_flush_segment);
		MPCOPY(vcache_flush_region);
		MPCOPY(vcache_flush_context);
		MPCOPY(pcache_flush_line);
#undef MPCOPY
		return;
	}
	panic("Out of CPUs");
}

/*
 * The following tables convert <IU impl, IU version, FPU version> triples
 * into names for the CPU and FPU chip.  In most cases we do not need to
 * inspect the FPU version to name the IU chip, but there is one exception
 * (for Tsunami), and this makes the tables the same.
 *
 * The table contents (and much of the structure here) are from Guy Harris.
 *
 */
struct info {
	int	valid;
	int	iu_impl;
	int	iu_vers;
	int	fpu_vers;
	char	*name;
};

/* NB: table order matters here; specific numbers must appear before ANY. */
static struct info fpu_types[] = {
	/*
	 * Vendor 0, IU Fujitsu0.
	 */
	{ 1, 0x0, ANY, 0, "MB86910 or WTL1164/5" },
	{ 1, 0x0, ANY, 1, "MB86911 or WTL1164/5" },
	{ 1, 0x0, ANY, 2, "L64802 or ACT8847" },
	{ 1, 0x0, ANY, 3, "WTL3170/2" },
	{ 1, 0x0, 4,   4, "on-chip" },		/* Swift */
	{ 1, 0x0, ANY, 4, "L64804" },

	/*
	 * Vendor 1, IU ROSS0/1 or Pinnacle.
	 */
	{ 1, 0x1, 0xf, 0, "on-chip" },		/* Pinnacle */
	{ 1, 0x1, ANY, 0, "L64812 or ACT8847" },
	{ 1, 0x1, ANY, 1, "L64814" },
	{ 1, 0x1, ANY, 2, "TMS390C602A" },
	{ 1, 0x1, ANY, 3, "RT602 or WTL3171" },

	/*
	 * Vendor 2, IU BIT0.
	 */
	{ 1, 0x2, ANY, 0, "B5010 or B5110/20 or B5210" },

	/*
	 * Vendor 4, Texas Instruments.
	 */
	{ 1, 0x4, ANY, 0, "on-chip" },		/* Viking */
	{ 1, 0x4, ANY, 4, "on-chip" },		/* Tsunami */

	/*
	 * Vendor 5, IU Matsushita0.
	 */
	{ 1, 0x5, ANY, 0, "on-chip" },

	/*
	 * Vendor 9, Weitek.
	 */
	{ 1, 0x9, ANY, 3, "on-chip" },

	{ 0 }
};

static char *
fsrtoname(impl, vers, fver, buf)
	register int impl, vers, fver;
	char *buf;
{
	register struct info *p;

	for (p = fpu_types; p->valid; p++)
		if (p->iu_impl == impl &&
		    (p->iu_vers == vers || p->iu_vers == ANY) &&
		    (p->fpu_vers == fver))
			return (p->name);
	sprintf(buf, "version %x", fver);
	return (buf);
}
