/*	$NetBSD: bus_space.c,v 1.13.2.1 2007/03/13 16:49:57 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.13.2.1 2007/03/13 16:49:57 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <uvm/uvm_extern.h>

extern char *extiobase;
extern int *nofault;

/* ARGSUSED */
int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	vaddr_t kva;
	vsize_t offset;
	int error;

	if (t->bustype == HP300_BUS_SPACE_INTIO) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); just
		 * do the translation.
		 */
		*bshp = (bus_space_handle_t)IIOV(INTIOBASE + bpa);
		return 0;
	}

	if (t->bustype != HP300_BUS_SPACE_DIO)
		panic("bus_space_map: bad space tag");

	/*
	 * Allocate virtual address space from the extio extent map.
	 */
	offset = m68k_page_offset(bpa);
	size = m68k_round_page(offset + size);
	error = extent_alloc(extio_ex, size, PAGE_SIZE, 0,
	    EX_FAST | EX_NOWAIT | (extio_ex_malloc_safe ? EX_MALLOCOK : 0),
	    &kva);
	if (error)
		return error;

	/*
	 * Map the range.  The range is always cache-inhibited on the hp300.
	 */
	physaccess((void *)kva, (void *)bpa, size, PG_RW|PG_CI);

	/*
	 * All done.
	 */
	*bshp = (bus_space_handle_t)(kva + offset);
	return 0;
}

/* ARGSUSED */
int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	/*
	 * Not meaningful on any currently-supported hp300 bus.
	 */
	return EINVAL;
}

/* ARGSUSED */
void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	/*
	 * Not meaningful on any currently-supported hp300 bus.
	 */
	panic("bus_space_free: shouldn't be here");
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	vaddr_t kva;
	vsize_t offset;

	if (t->bustype == HP300_BUS_SPACE_INTIO) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); nothing
		 * to do
		 */
		return;
	}

	if (t->bustype != HP300_BUS_SPACE_DIO)
		panic("bus_space_map: bad space tag");

	kva = m68k_trunc_page(bsh);
	offset = m68k_page_offset(bsh);
	size = m68k_round_page(offset + size);

#ifdef DIAGNOSTIC
	if (bsh < (vaddr_t)extiobase ||
	    bsh >= ((vaddr_t)extiobase + ptoa(EIOMAPSIZE)))
		panic("bus_space_unmap: bad bus space handle");
#endif

	/*
	 * Unmap the range.
	 */
	physunaccess((void *)kva, size);

	/*
	 * Free it from the extio extent map.
	 */
	if (extent_free(extio_ex, kva, size,
	    EX_NOWAIT | (extio_ex_malloc_safe ? EX_MALLOCOK : 0)))
		printf("bus_space_unmap: kva 0x%lx size 0x%lx: "
		    "can't free region\n", (u_long) bsh, size);
}

/* ARGSUSED */
int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

/* ARGSUSED */
int
hp300_bus_space_probe(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, int sz)
{
	label_t faultbuf;
	int i;

	nofault = (int *)&faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = NULL;
		return 0;
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
		panic("bus_space_probe: unupported data size %d", sz);
		/* NOTREACHED */
	}

	nofault = NULL;
	return 1;
}
