/*	$NetBSD: bus_subr.c,v 1.21.2.1 2001/10/01 12:42:52 fvdl Exp $	*/

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
 * bus_xxx support functions, Sun3X-specific part.
 * The common stuff is in autoconf.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/mon.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun3/sun3/machdep.h>
#include <sun3/sun3x/vme.h>

label_t *nofault;

/* These are defined in pmap.c */
extern vaddr_t tmp_vpages[];
extern int tmp_vpages_inuse;

static const struct {
	long base;
	long mask;
} bus_info[BUS__NTYPES] = {
	{ /* OBIO  */ 0, ~0 },
	{ /* OBMEM */ 0, ~0 },
	/* VME A16 */
	{ VME16D16_BASE, VME16_MASK },
	{ VME16D32_BASE, VME16_MASK },
	/* VME A24 */
	{ VME24D16_BASE, VME24_MASK },
	{ VME24D32_BASE, VME24_MASK },
	/* VME A32 */
	{ VME32D16_BASE, VME32_MASK },
	{ VME32D32_BASE, VME32_MASK },
};

/*
 * Create a temporary, one-page mapping for a device.
 * This is used by some device probe routines that
 * need to do peek/write/read tricks.
 */
void *
bus_tmapin(bustype, pa)
	int bustype, pa;
{
	vaddr_t pgva;
	int off, s;

	if ((bustype < 0) || (bustype >= BUS__NTYPES))
		panic("bus_tmapin: bustype");

	off = pa & PGOFSET;
	pa -= off;

	pa &= bus_info[bustype].mask;
	pa |= bus_info[bustype].base;
	pa |= PMAP_NC;

	s = splvm();
	if (tmp_vpages_inuse)
		panic("bus_tmapin: tmp_vpages_inuse");
	tmp_vpages_inuse++;

	pgva = tmp_vpages[1];
	pmap_kenter_pa(pgva, pa, VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());
	splx(s);

	return ((void *)(pgva + off));
}

void bus_tmapout(vp)
	void *vp;
{
	vaddr_t pgva;
	int s;

	pgva = m68k_trunc_page(vp);
	if (pgva != tmp_vpages[1])
		return;

	s = splvm();
	pmap_kremove(pgva, NBPG);
	pmap_update(pmap_kernel());
	--tmp_vpages_inuse;
	splx(s);
}

/*
 * Make a permanent mapping for a device.
 */
void *
bus_mapin(bustype, pa, sz)
	int bustype, pa, sz;
{
	vaddr_t va;
	int off;

	if ((bustype < 0) || (bustype >= BUS__NTYPES))
		panic("bus_mapin: bustype");

	off = pa & PGOFSET;
	pa -= off;
	sz += off;
	sz = m68k_round_page(sz);

	/* Borrow PROM mappings if we can. */
	if (bustype == BUS_OBIO) {
		va = (vaddr_t) obio_find_mapping(pa, sz);
		if (va != 0)
			goto done;
	}

	pa &= bus_info[bustype].mask;
	pa |= bus_info[bustype].base;
	pa |= PMAP_NC;	/* non-cached */

	/* Get some kernel virtual address space. */
	va = uvm_km_valloc_wait(kernel_map, sz);
	if (va == 0)
		panic("bus_mapin");

	/* Map it to the specified bus. */
	pmap_map(va, pa, pa + sz, VM_PROT_ALL);

done:
	return ((void*)(va + off));
}

void
bus_mapout(ptr, sz)
	void *ptr;
	int sz;
{
	vaddr_t va;
	int off;

	va = (vaddr_t)ptr;

	/* If it was a PROM mapping, do NOT free it! */
	if ((va >= SUN3X_MONSTART) && (va < SUN3X_MONEND))
		return;

	off = va & PGOFSET;
	va -= off;
	sz += off;
	sz = m68k_round_page(sz);

	uvm_km_free_wakeup(kernel_map, va, sz);
}
