/*	$NetBSD: bus_space.c,v 1.2.4.2 2002/06/23 17:36:11 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Implementation of bus_space mapping for the hp300.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.2.4.2 2002/06/23 17:36:11 jdolecek Exp $");                                                  

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>

#include <machine/bus.h>

#include <uvm/uvm_extern.h>

extern struct map extiomap[];
extern caddr_t extiobase;
extern int *nofault;

/* ARGSUSED */
int
bus_space_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int idx, npgs;
	caddr_t kva;

	if (t == HP300_BUS_SPACE_INTIO) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); just
		 * do the translation.
		 */
		*bshp = (bus_space_handle_t)IIOV(INTIOBASE + bpa);
		return (0);
	}

	if (t != HP300_BUS_SPACE_DIO)
		panic("bus_space_map: bad space tag");

	/*
	 * Allocate virtual address space from the extio resource map.
	 */
	size = m68k_round_page(size);
	npgs = btoc(size);
	idx = rmalloc(extiomap, npgs);
	if (idx == 0)
		return (ENOMEM);
	kva = extiobase + ctob(idx - 1);

	/*
	 * Map the range.  The range is always cache-inhibited on the hp300.
	 */
	physaccess(kva, (caddr_t)bpa, size, PG_RW|PG_CI);

	/*
	 * All done.
	 */
	*bshp = (bus_space_handle_t)kva;
	return (0);
}

/* ARGSUSED */
int
bus_space_alloc(t, rstart, rend, size, alignment, boundary, flags,
    bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{

	/*
	 * Not meaningful on any currently-supported hp300 bus.
	 */
	return (EINVAL);
}

/* ARGSUSED */
void
bus_space_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/*
	 * Not meaningful on any currently-supported hp300 bus.
	 */
	panic("bus_space_free: shouldn't be here");
}

void
bus_space_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	int idx;

	if (t == HP300_BUS_SPACE_INTIO) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); nothing
		 * to do
		 */
		return;
	}

	if (t != HP300_BUS_SPACE_DIO)
		panic("bus_space_map: bad space tag");

	size = m68k_round_page(size);

#ifdef DIAGNOSTIC
	if (bsh & PGOFSET)
		panic("bus_space_unmap: unaligned");
	if ((caddr_t)bsh < extiobase ||
	    (caddr_t)bsh >= (extiobase + ctob(EIOMAPSIZE)))
		panic("bus_space_unmap: bad bus space handle");
#endif

	/*
	 * Unmap the range.
	 */
	physunaccess((caddr_t)bsh, size);

	/*
	 * Free it from the extio resource map.
	 */
	idx = btoc((caddr_t)bsh - extiobase) + 1;
	rmfree(extiomap, btoc(size), idx);
}

/* ARGSUSED */
int
bus_space_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

/* ARGSUSED */
int
hp300_bus_space_probe(t, bsh, offset, sz)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	int sz;
{
	label_t faultbuf;
	int i;

	nofault = (int *)&faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = NULL;
		return (0);
	}

	switch (sz) {
	case 1:
		i = bus_space_read_1(t, bsh, offset);
		break;

	case 2:
		i = bus_space_read_2(t, bsh, offset);
		break;

	case 4:
		i = bus_space_read_4(t, bsh, offset);
		break;

	default:
		panic("bus_space_probe: unupported data size %d\n", sz);
		/* NOTREACHED */
	}

	nofault = NULL;
	return (1);
}
