/*	$NetBSD: autoconf.c,v 1.7.2.1 1997/03/02 16:17:47 mrg Exp $	*/

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

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/map.h>
#include <sys/dkstat.h>
#include <sys/dmap.h>
#include <sys/reboot.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/machdep.h>
#include <machine/mon.h>

#include "pmap_pvt.h"

int cold;

static int net_mkunit __P((int, int));
static int sd_mkunit __P((int, int));
static int xx_mkunit __P((int, int));

void	findroot __P((struct device **, int *));

static struct devnametobdevmaj nam2blk[] = {
	{ "xy",		3 },
	{ "sd",		7 },
	{ "xd",		10 },
	{ "md",		13 },
	{ "cd",		18 },
	{ NULL,		0 },
};

void configure()
{
	struct device *booted_device;
	int booted_partition;

	/* General device autoconfiguration. */
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not found");

	printf("enabling interrupts\n");
	(void)spl0();

	/* Choose root and swap devices. */
	findroot(&booted_device, &booted_partition);

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition, nam2blk);

	dumpconf();
	cold = 0;
}

/*
 * Support code to find the boot device.
 */

/*
 * Map Sun PROM device names to unit numbers.
 */
static int
net_mkunit(ctlr, unit)
	int ctlr, unit;
{

	/* XXX - Not sure which is set. */
	return (ctlr + unit);
}

static int
sd_mkunit(ctlr, unit)
	int ctlr, unit;
{
	int target, lun;

	/* This only supports LUNs 0, 1 */
	target = unit >> 3;
	lun = unit & 1;
	return (target * 2 + lun);
}

static int
xx_mkunit(ctlr, unit)
	int ctlr, unit;
{

	return (ctlr * 2 + unit);
}

static struct {
	const char *name;
	int (*mkunit) __P((int, int));
} name2unit[] = {
	{ "ie",		net_mkunit },
	{ "le",		net_mkunit },
	{ "sd",		sd_mkunit },
	{ "xy",		xx_mkunit },
	{ "xd",		xx_mkunit },
	{ NULL,		0 },
};

void
findroot(devpp, partp)
	struct device **devpp;
	int *partp;
{
	MachMonRomVector *rvec;
	MachMonBootParam *bpp;
	struct device *dv;
	char name[32];
	int unit, part, i;

	rvec = romVectorPtr;
	bpp = *rvec->bootParam;

	/* Default to "Not found". */
	*devpp = NULL;
	*partp = 0;

	/* Extract device name (always two letters). */
	name[0] = bpp->devName[0];
	name[1] = bpp->devName[1];
	name[3] = '\0';

	/* Do we know how to get a unit number for this device? */
	for (i = 0; name2unit[i].name != NULL; i++)
		if (name2unit[i].name[0] == name[0] &&
		    name2unit[i].name[1] == name[1])
			break;
	if (name2unit[i].name == NULL)
		return;

	unit = (*name2unit[i].mkunit)(bpp->ctlrNum, bpp->unitNum);
	part = bpp->partNum;

	/* Look up the device. */
	sprintf(name, "%s%d", name2unit[i].name, unit);
	for (dv = alldevs.tqh_first; dv != NULL;
	    dv = dv->dv_list.tqe_next) {
		if (strcmp(name, dv->dv_xname) == 0) {
			*devpp = dv;
			*partp = part;
			return;
		}
	}
}

/*
 * Generic "bus" support functions.
 *
 * bus_scan:
 * This function is passed to config_search() by the attach function
 * for each of the "bus" drivers (obctl, obio, obmem, vmes, vmel).
 * The purpose of this function is to copy the "locators" into our
 * confargs structure, so child drivers may use the confargs both
 * as match parameters and as temporary storage for the defaulted
 * locator values determined in the child_match and preserved for
 * the child_attach function.  If the bus attach functions just
 * used config_found, then we would not have an opportunity to
 * setup the confargs for each child match and attach call.
 *
 * bus_print:
 * Just prints out the final (non-default) locators.
 */
int bus_scan(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	cfmatch_t mf;

#ifdef	DIAGNOSTIC
	if (cf->cf_fstate == FSTATE_STAR)
		panic("bus_scan: FSTATE_STAR");
#endif

	/* ca->ca_bustype set by parent */
	ca->ca_paddr  = cf->cf_loc[0];
	ca->ca_intpri = cf->cf_loc[1];
	ca->ca_intvec = -1;

	if ((ca->ca_bustype == BUS_VME16) ||
		(ca->ca_bustype == BUS_VME32))
	{
		ca->ca_intvec = cf->cf_loc[2];
	}

	/*
	 * Note that this allows the match function to save
	 * defaulted locators in the confargs that will be
	 * preserved for the related attach call.
	 */
	mf = cf->cf_attach->ca_match;
	if ((*mf)(parent, cf, ca) > 0) {
		config_attach(parent, cf, ca, bus_print);
	}
	return (0);
}

/*
 * Print out the confargs.  The parent name is non-NULL
 * when there was no match found by config_found().
 */
int
bus_print(args, name)
	void *args;
	const char *name;
{
	struct confargs *ca = args;

	if (name)
		printf("%s:", name);

	if (ca->ca_paddr != -1)
		printf(" addr 0x%x", ca->ca_paddr);
	if (ca->ca_intpri != -1)
		printf(" level %d", ca->ca_intpri);
	if (ca->ca_intvec != -1)
		printf(" vector 0x%x", ca->ca_intvec);

	return(UNCONF);
}

/* This is defined in _startup.c */
extern vm_offset_t tmp_vpages[];

#ifdef	SUN3X_NOT_YET
/* I really don't get this. -J
   Yeah, this is sun3-specific, but we will eventually need
   something similar to convert VME address spaces to our
   physical addresses.  The conversion is pretty similar to
   what the sun3 does with "page type bits". -gwr */
static const int bustype_to_ptetype[4] = {
	PGT_OBMEM,
	PGT_OBIO,
	PGT_VME_D16,
	PGT_VME_D32,
};
#else	/* SUN3X_NOT_YET */
static const int bustype_to_patype[4] = {
	0,		/* OBMEM  */
	0,		/* OBIO   */
	PMAP_VME16,	/* VMED16 */
	PMAP_VME32,	/* VMED32 */
};
#endif	/* SUN3X_NOT_YET */

/*
 * Read addr with size len (1,2,4) into val.
 * If this generates a bus error, return -1
 *
 *	Create a temporary mapping,
 *	Try the access using peek_*
 *	Clean up temp. mapping
 */
#ifdef	SUN3X_NOT_YET
int bus_peek(bustype, paddr, sz)
	int bustype, paddr, sz;
{
	int off, pte, rv;
	vm_offset_t pgva;
	caddr_t va;

	if (bustype & ~3)
		return -1;

	off = paddr & PGOFSET;
	paddr -= off;
	pte = PA_PGNUM(paddr);
	pte |= bustype_to_ptetype[bustype];
	pte |= (PG_VALID | PG_WRITE | PG_SYSTEM | PG_NC);

	pgva = tmp_vpages[0];
	va = (caddr_t)pgva + off;

	/* All mappings in tmp_vpages are non-cached, so no flush. */
	set_pte(pgva, pte);

	/*
	 * OK, try the access using one of the assembly routines
	 * that will set pcb_onfault and catch any bus errors.
	 */
	switch (sz) {
	case 1:
		rv = peek_byte(va);
		break;
	case 2:
		rv = peek_word(va);
		break;
	default:
		printf(" bus_peek: invalid size=%d\n", sz);
		rv = -1;
	}

	/* All mappings in tmp_vpages are non-cached, so no flush. */
	set_pte(pgva, PG_INVAL);

	return rv;
}
#else	/************ SUN3X_NOT_YET ***********/
int bus_peek(bustype, paddr, sz)
	int bustype, paddr, sz;
{
	int	offset, rtn;
	vm_offset_t va_page;
	caddr_t va;

	if (bustype != OBIO && bustype != OBMEM)
		return -1;

	offset = paddr & ~(MMU_PAGE_MASK);
	paddr = sun3x_trunc_page(paddr);
	paddr |= bustype_to_patype[bustype];
	paddr |= PMAP_NC;

	va_page = tmp_vpages[0];
	va      = (caddr_t) va_page + offset;

	pmap_enter_kernel(va_page, paddr, (VM_PROT_READ|VM_PROT_WRITE));

	switch (sz) {
		case 1:
			rtn = peek_byte(va);
			break;
		case 2:
			rtn = peek_word(va);
			break;
		default:
			printf(" bus_peek: invalid size=%d\n", sz);
			rtn = -1;
	}

	/*
	 * XXX - This function is definitely NOT performance-critical,
	 * so I would be more comfortable with the paranoid habit of
	 * leaving the tmp_vpages unmapped when not in use.
	 * (It might help prevent accidents...)
	 */

	return rtn;
}
#endif


void *
bus_mapin(bustype, paddr, sz)
	int bustype, paddr, sz;
{
	int off, pa, pmt;
	vm_offset_t va, retval;

	if (bustype & ~3)
		return (NULL);

	off = paddr & PGOFSET;
	pa = paddr - off;
	sz += off;
	sz = round_page(sz);

	pmt = PMAP_NC;	/* non-cached */

	/* Get some kernel virtual address space. */
	va = kmem_alloc_wait(kernel_map, sz);
	if (va == 0)
		panic("bus_mapin");
	retval = va + off;

	/* Map it to the specified bus. */
#if 0	/* XXX */
	/* This has a problem with wrap-around... */
	pmap_map((int)va, pa | pmt, pa + sz, VM_PROT_ALL);
#else
	do {
		pmap_enter(pmap_kernel(), va, pa | pmt, VM_PROT_ALL, FALSE);
		va += NBPG;
		pa += NBPG;
	} while ((sz -= NBPG) > 0);
#endif

	return ((void*)retval);
}

int
peek_word(addr)
	register caddr_t addr;
{
	label_t		faultbuf;
	register int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return(-1);
	}
	x = *(volatile u_short *)addr;
	nofault = NULL;
	return(x);
}

/* from hp300: badbaddr() */
int
peek_byte(addr)
	register caddr_t addr;
{
	label_t 	faultbuf;
	register int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf)) {
		nofault = NULL;
		return(-1);
	}
	x = *(volatile u_char *)addr;
	nofault = NULL;
	return(x);
}
