/*	$NetBSD: bus_subr.c,v 1.4 1998/02/05 04:57:26 gwr Exp $	*/

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
 * bus_xxx support functions
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun3/sun3/control.h>
#include <sun3/sun3/machdep.h>
#include <sun3/sun3/vme.h>

/*
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

	/*
	 * Copy the locators into our confargs.
	 * Our parent set ca->ca_bustype already.
	 */
	ca->ca_paddr  = cf->cf_paddr;
	ca->ca_intpri = cf->cf_intpri;
	ca->ca_intvec = cf->cf_intvec;

	/*
	 * Note that this allows the match function to save
	 * defaulted locators in the confargs that will be
	 * preserved for the related attach call.
	 * XXX - This is a hack...
	 */
	mf = cf->cf_attach->ca_match;
	if ((*mf)(parent, cf, ca) > 0) {
		config_attach(parent, cf, ca, bus_print);
	}
	return (0);
}

/*
 * bus_print:
 * Just print out the final (non-default) locators.
 * The parent name is non-NULL when there was no match
 * found by config_found().
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
		printf(" ipl %d", ca->ca_intpri);
	if (ca->ca_intvec != -1)
		printf(" vect 0x%x", ca->ca_intvec);

	return(UNCONF);
}

/****************************************************************/
/* support functions */

label_t *nofault;

extern vm_offset_t tmp_vpages[];

#define OBIO_MASK 0xFFffff
#define PMAP_OBMEM 0

static const struct {
	int  type;
	long base;
	long mask;
} bus_info[BUS__NTYPES] = {
	{ PMAP_OBIO,  0, OBIO_MASK },
	{ PMAP_OBMEM, 0, ~0 },
	/* VME A16 */
	{ PMAP_VME16, VME16_BASE, VME16_MASK },
	{ PMAP_VME32, VME16_BASE, VME16_MASK },
	/* VME A24 */
	{ PMAP_VME16, VME24_BASE, VME24_MASK },
	{ PMAP_VME32, VME24_BASE, VME24_MASK },
	/* VME A32 */
	{ PMAP_VME16, VME32_BASE, VME32_MASK },
	{ PMAP_VME32, VME32_BASE, VME32_MASK },
};

/*
 * Read addr with size len (1,2,4) into val.
 * If this generates a bus error, return -1
 *
 *	Create a temporary mapping,
 *	Try the access using peek_*
 *	Clean up temp. mapping
 */
int
bus_peek(bustype, pa, sz)
	int bustype, pa, sz;
{
	int off, pte, rv;
	vm_offset_t pgva;
	caddr_t va;

	if ((bustype < 0) || (bustype >= BUS__NTYPES))
		panic("bus_peek: bustype");

	pa &= bus_info[bustype].mask;
	pa |= bus_info[bustype].base;

	off = pa & PGOFSET;
	pa -= off;
	pte = PA_PGNUM(pa);

	pte |= (bus_info[bustype].type << PG_MOD_SHIFT);
	pte |= (PG_VALID | PG_WRITE | PG_SYSTEM | PG_NC);

	pgva = tmp_vpages[0];
	va = (caddr_t)pgva + off;

	/* All mappings in tmp_vpages are non-cached, so no flush. */
	set_pte(pgva, pte);

	switch (sz) {
	case 1:
		rv = peek_byte(va);
		break;
	case 2:
		rv = peek_word(va);
		break;
	case 4:
		rv = peek_long(va);
		break;
	default:
		printf(" bus_peek: invalid size=%d\n", sz);
		rv = -1;
	}

	/* All mappings in tmp_vpages are non-cached, so no flush. */
	set_pte(pgva, PG_INVAL);

	return (rv);
}

void *
bus_mapin(bustype, pa, sz)
	int bustype, pa, sz;
{
	void *rv;
	vm_offset_t va;
	int off;

	if ((bustype < 0) || (bustype >= BUS__NTYPES))
		panic("bus_mapin: bustype");

	/* Borrow PROM mappings if we can. */
	if (bustype == BUS_OBIO) {
		rv = obio_find_mapping(pa, sz);
		if (rv)
			return (rv);
	}

	pa &= bus_info[bustype].mask;
	pa |= bus_info[bustype].base;

	off = pa & PGOFSET;
	pa -= off;
	sz += off;
	sz = m68k_round_page(sz);

	pa |= bus_info[bustype].type;
	pa |= PMAP_NC;	/* non-cached */

	/* Get some kernel virtual address space. */
	va = kmem_alloc_wait(kernel_map, sz);
	if (va == 0)
		panic("bus_mapin");
	rv = (void*)(va + off);

	/* Map it to the specified bus. */
#if 0
	/* XXX: This has a problem with wrap-around... */
	pmap_map((int)va, pa, pa + sz, VM_PROT_ALL);
#else
	do {
		pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, FALSE);
		va += NBPG;
		pa += NBPG;
		sz -= NBPG;
	} while (sz > 0);
#endif

	return (rv);
}

/* from hp300: badbaddr() */
int
peek_byte(addr)
	register caddr_t addr;
{
	label_t 	faultbuf;
	register int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf))
		x = -1;
	else
		x = *(volatile u_char *)addr;

	nofault = NULL;
	return(x);
}

int
peek_word(addr)
	register caddr_t addr;
{
	label_t		faultbuf;
	register int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf))
		x = -1;
	else
		x = *(volatile u_short *)addr;

	nofault = NULL;
	return(x);
}

int
peek_long(addr)
	register caddr_t addr;
{
	label_t		faultbuf;
	register int x;

	nofault = &faultbuf;
	if (setjmp(&faultbuf))
		x = -1;
	else {
		x = *(volatile int *)addr;
		if (x == -1) {
			printf("peek_long: uh-oh, actually read -1!\n");
			x &= 0x7FFFffff; /* XXX */
		}
	}

	nofault = NULL;
	return(x);
}
