/*	$NetBSD: bus_space.c,v 1.1.2.1 2000/03/11 20:51:52 scw Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford and Jason R. Thorpe.
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
 * Implementation of bus_space mapping for the mvme68k.
 * Derived from the hp300 bus_space implementation by Jason Thorpe.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/bus.h>

static struct extent *vmeioextent;

/* ARGSUSED */
int
bus_space_map(bust, addr, size, flags, bushp)
	bus_space_tag_t bust;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bushp;
{
	u_long kva;
	int rv;

	if ( bust == MVME68K_INTIO_BUS_SPACE ) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); just
		 * do the translation.
		 */
		if ( &(intiobase[addr + size]) >= intiolimit )
			return (EINVAL);

		*bushp = (bus_space_handle_t) &(intiobase[addr]);
		return (0);
	}

	if ( bust != MVME68K_VME_BUS_SPACE )
		panic("bus_space_map: bad space tag");

	/*
	 * The first time through, allocate an extent to manage the
	 * KVA space available for mapping the VMEbus.
	 */
	if ( vmeioextent == NULL ) {
		vmeioextent = extent_create("vmekva", (u_long) vmeiobase,
		    (u_long) (vmeiobase + VMEIOMAPSIZE - 1),
		    M_DEVBUF, NULL, 0, 0);
		if ( vmeioextent == NULL )
			return (ENOMEM);
	}

	/*
	 * Allocate virtual address space from the vmeio extent.
	 */
	size = m68k_round_page(size);
	rv = extent_alloc(vmeioextent, size, NBPG, 0, EX_NOWAIT, &kva);
	if ( rv != 0 )
		return (rv);

	/*
	 * Map the range.  The range is always cache-inhibited on
	 * the mvme68k.
	 */
	physaccess((caddr_t) kva, (caddr_t) addr, size, PG_RW|PG_CI);

	/*
	 * The handle is really the virtual address we just mapped
	 */
	*bushp = (bus_space_handle_t) (kva + m68k_page_offset(addr));

	return (0);
}

void
bus_space_unmap(bust, bush, size)
	bus_space_tag_t bust;
	bus_space_handle_t bush;
	bus_size_t size;
{
	if ( bust == MVME68K_INTIO_BUS_SPACE ) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); nothing
		 * to do
		 */
		return;
	}

	if ( bust != MVME68K_VME_BUS_SPACE )
		panic("bus_space_unmap: bad space tag");

#ifdef DEBUG
	if ((caddr_t)bush < vmeiobase ||
	    (caddr_t)(bush + size) >= (vmeiobase + VMEIOMAPSIZE))
		panic("bus_space_unmap: bad bus space handle");
#endif

	bush = m68k_trunc_page(bush);
	size = m68k_round_page(size);

	/*
	 * Unmap the range.
	 */
	physunaccess((caddr_t)bush, size);

	/*
	 * Free it from the vmeio extent.
	 */
	(void) extent_free(vmeioextent, (u_long)bush, (u_long)size, M_NOWAIT);
}
