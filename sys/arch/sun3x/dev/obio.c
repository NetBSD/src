/*	$NetBSD: obio.c,v 1.3.4.1 1997/03/12 14:21:55 is Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/machdep.h>
#include <machine/mon.h>
#include <machine/obio.h>
#include <machine/pte.h>

short *enable_reg;

static int  obio_match __P((struct device *, struct cfdata *, void *));
static void obio_attach __P((struct device *, struct device *, void *));
static int  obio_print __P((void *, const char *parentname));
static int	obio_submatch __P((struct device *, struct cfdata *, void *));

struct cfattach obio_ca = {
	sizeof(struct device), obio_match, obio_attach
};

struct cfdriver obio_cd = {
	NULL, "obio", DV_DULL
};

static int
obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (ca->ca_bustype != BUS_OBIO)
		return (0);
	return(1);
}

/*
 * We need some control over the order of attachment on OBIO,
 * and all OBIO device addresses are known and fixed foerver.
 * Therefore, this uses a list of addresses to attach.
 * XXX - Any other way to control search/attach order?
 */
static int obio_alist[] = {
	OBIO_ZS_KBD_MS,
	OBIO_ZS_TTY_AB,

	OBIO_EEPROM,	/* the next two are part of the eeprom */
	OBIO_IDPROM2,	/* 3/80 only */
	OBIO_CLOCK2,	/* 3/80 only */
	OBIO_CLOCK1,	/* 3/470 only */

	/* These are all in the same page - could be just one driver... */
	OBIO_ENABLEREG,
	OBIO_BUSERRREG,
	OBIO_DIAGREG,
	OBIO_IDPROM1,
	OBIO_MEMREG,
	OBIO_INTERREG,

	/*
	 * The addresses listed before this point were already found
	 * and mapped in by the startup code, so keep those in the
	 * order shown, and separated from what follows.
	 * (Just to be honest about attach order.)
	 *
	 * The following are carefully ordered.
	 */

	/* This is used by video board drivers... */
	OBIO_P4_REG,

	/* This is used by the Ethernet and SCSI drivers. */
	OBIO_IOMMU,

	OBIO_INTEL_ETHER,
	OBIO_LANCE_ETHER,

	OBIO_EMULEX_SCSI, /* 3/80 only */

	/* ...todo... */
	OBIO_FDC,
	OBIO_PRINTER_PORT,
};
#define OBIO_ALIST_LEN (sizeof(obio_alist) / \
                        sizeof(obio_alist[0]))

static void
obio_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs *ca = aux;
	int	i;

	printf("\n");

	/* Configure these in order of address. */
	for (i = 0; i < OBIO_ALIST_LEN; i++) {
		/* We know ca_bustype == BUS_OBIO */
		ca->ca_paddr = obio_alist[i];
		ca->ca_intpri = -1;
		ca->ca_intvec = -1;
		(void) config_found_sm(self, ca, obio_print, obio_submatch);
	}
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
obio_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	/* Be quiet about empty OBIO locations. */
	if (name)
		return(QUIET);

	if (ca->ca_paddr != -1)
		printf(" addr 0x%x", ca->ca_paddr);
	if (ca->ca_intpri != -1)
		printf(" level %d", ca->ca_intpri);

	return(UNCONF);
}

int
obio_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	cfmatch_t submatch;

	/*
	 * Default addresses are mostly useless for OBIO.
	 * The address assignments are fixed for all time,
	 * so our config files might as well reflect that.
	 */
#ifdef	DIAGNOSTIC
	if (cf->cf_paddr == -1)
		panic("obio_submatch: invalid address for: %s%d\n",
			cf->cf_driver->cd_name, cf->cf_unit);
#endif

	/* This enforces exact address match. */
	if (cf->cf_paddr != ca->ca_paddr)
		return 0;

	/* Now call the match function of the potential child. */
	submatch = cf->cf_attach->ca_match;
	if (submatch == NULL)
		panic("obio_submatch: no match function for: %s\n",
			  cf->cf_driver->cd_name);

	return ((*submatch)(parent, cf, aux));
}


/*****************************************************************/

/*
 * This is our record of "interesting" OBIO mappings that
 * the PROM has left in the virtual space reserved for it.
 * Each non-null array element holds the virtual address
 * of an OBIO mapping where the OBIO address mapped is:
 *     (array_index * SAVE_INCR)
 * and the length of the mapping is one page.
 */
static struct prom_map {
	vm_offset_t pa, va;
} prom_mappings[] = {
	{ OBIO_ZS_KBD_MS, 0 },	/* Keyboard and Mouse */
	{ OBIO_ZS_TTY_AB, 0 },	/* Serial Ports */
	{ OBIO_EEPROM,    0 },	/* EEPROM/IDPROM/clock */
	{ OBIO_ENABLEREG, 0 },	/* regs: Sys ENA, Bus ERR, etc. */
};
#define PROM_MAP_CNT (sizeof(prom_mappings) / \
		      sizeof(prom_mappings[0]))

/*
 * Find a virtual address for a device at physical address 'pa'.
 * If one is found among the mappings already made by the PROM
 * at power-up time, use it.  Otherwise return 0 as a sign that
 * a mapping will have to be created.
 */
caddr_t
obio_find_mapping(int pa, int size)
{
	int i, off;

	if (size >= NBPG)
		return (caddr_t)0;

	off = pa & PGOFSET;
	pa -= off;

	for (i = 0; i < PROM_MAP_CNT; i++) {
		if (pa == prom_mappings[i].pa) {
			return ((caddr_t)(prom_mappings[i].va + off));
		}
	}
	return (caddr_t)0;
}

/*
 * Search the PROM page tables for OBIO mappings that
 * we would like to borrow.
 */
static void
save_prom_mappings __P((void))
{
	int *mon_pte;
	vm_offset_t va, pa;
	int i;

	/* Note: mon_ctbl[0] maps MON_KDB_START */
	mon_pte = *romVectorPtr->monptaddr;

	for (va = MON_KDB_START; va < MONEND;
		 va += NBPG, mon_pte++)
	{
		/* Is this a valid mapping to OBIO? */
		/* XXX - Some macros would be nice... */
		if ((*mon_pte & 0xF0000003) != 0x60000001)
			continue;

		/* Yes it is.  Is this a mapping we want? */
		pa = *mon_pte & MMU_SHORT_PTE_BASEADDR;
		for (i = 0; i < PROM_MAP_CNT; i++) {
			if (pa != prom_mappings[i].pa)
				continue;
			/* Yes, we want it.  Save the va? */
			if (prom_mappings[i].va == 0) {
				prom_mappings[i].va = va;
			}
		}
	}

}

/*
 * These are all the OBIO address that are required early in
 * the life of the kernel.  All are less than one page long.
 * This function should make any required mappings that we
 * were not able to find among the PROM monitor's mappings.
 */
static void
make_required_mappings __P((void))
{
	int i;

	for (i = 0; i < PROM_MAP_CNT; i++) {
		if (prom_mappings[i].va == 0) {
			/*
			 * Actually, the PROM always has all the
			 * "required" mappings we need, (smile)
			 * but this makes sure that is true.
			 */
			mon_printf("obio: no mapping for pa=0x%x\n",
			    prom_mappings[i].pa);
			sunmon_abort();  /* Ancient PROM? */
		}
	}
}


/*
 * Find mappings for devices that are needed before autoconfiguration.
 * We first look for and record any useful PROM mappings, then call
 * the "init" functions for drivers that we need to use before the
 * normal autoconfiguration calls configure().  Warning: this is
 * called before pmap_bootstrap, so no allocation allowed!
 */
void
obio_init()
{
	save_prom_mappings();
	make_required_mappings();

	/* Init drivers that use the required OBIO mappings. */
	idprom_init();
	eeprom_init();
	zs_init();

	enable_reg = (short*) obio_find_mapping(OBIO_ENABLEREG, 2);
	intreg_init();
	clock_init();
}

caddr_t
obio_alloc(obio_addr, obio_size)
	int obio_addr, obio_size;
{
	caddr_t cp;

	cp = obio_find_mapping((vm_offset_t)obio_addr, obio_size);
	if (cp) return (cp);

	cp = bus_mapin(BUS_OBIO, obio_addr, obio_size);
	return (cp);
}
