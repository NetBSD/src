/*	$NetBSD: bus_space.c,v 1.13 2014/09/21 16:34:53 christos Exp $	*/

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
 * Implementation of bus_space mapping for the news68k.
 * Just taken from hp300.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.13 2014/09/21 16:34:53 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <uvm/uvm_extern.h>

extern int *nofault;

/* ARGSUSED */
int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{

	if (t == NEWS68K_BUS_SPACE_INTIO) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); just
		 * do the translation.
		 */
		*bshp = (bus_space_handle_t)bpa;
		return 0;
	}

	if (t == NEWS68K_BUS_SPACE_EIO) {
		*bshp = (bus_space_handle_t)bpa; /* XXX use tt0 mapping */
		return 0;
	}

	return 1;
}

/* ARGSUSED */
int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	/*
	 * Not meaningful on any currently-supported news68k bus.
	 */
	return EINVAL;
}

/* ARGSUSED */
void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	/*
	 * Not meaningful on any currently-supported news68k bus.
	 */
	panic("bus_space_free: shouldn't be here");
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{

	if (t == NEWS68K_BUS_SPACE_INTIO) {
		/*
		 * Intio space is direct-mapped in pmap_bootstrap(); nothing
		 * to do
		 */
		return;
	}

	if (t != NEWS68K_BUS_SPACE_EIO)
		panic("bus_space_map: bad space tag");

	return;
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
news68k_bus_space_probe(bus_space_tag_t t, bus_space_handle_t bsh,
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
	__USE(i);
	nofault = NULL;
	return 1;
}
