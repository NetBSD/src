/*	$NetBSD: bus_space.c,v 1.1.1.1 1999/09/16 12:23:20 takemura Exp $	*/

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*	$NetBSD: bus_space.c,v 1.1.1.1 1999/09/16 12:23:20 takemura Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/extent.h>

#include <mips/cpuregs.h>
#include <machine/bus.h>

#define MAX_BUSSPACE_TAG 3
/* vr_internal_alloc_bus_space_tag called before pmap_bootstrap. */
static  struct hpcmips_bus_space sys_bus_space[MAX_BUSSPACE_TAG]; /* System internal, ISA port, ISA mem */
static int bus_space_index = 0;

bus_space_tag_t
hpcmips_alloc_bus_space_tag()
{
	bus_space_tag_t	t;

	if (bus_space_index >= MAX_BUSSPACE_TAG)
		panic("vr_internal_alloc_bus_space_tag: tag full.");
	t = &sys_bus_space[bus_space_index++];
	return t;
}

void
hpcmips_init_bus_space_extent(bus_space_tag_t t)
{
	t->t_extent = (void*)extent_create(t->t_name, t->t_base, 
					   t->t_base + t->t_size, M_DEVBUF,
					   0, 0, EX_NOWAIT);
	if (!t->t_extent)
		panic("init_resource: unable to allocate %s map", t->t_name);
}

/* ARGSUSED */
int
bus_space_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int err;
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;

	if (!t->t_extent) { /* Before autoconfiguration, can't use extent */
		printf("bus_space_map: map temprary region:0x%08x-0x%08x\n", bpa, bpa+size);
		bpa += t->t_base;
	} else {
		bpa += t->t_base;
		if ((err = extent_alloc_region(t->t_extent, bpa, size, EX_NOWAIT|EX_MALLOCOK)))
			return err;
	}
	if (cacheable)
		*bshp = MIPS_PHYS_TO_KSEG0(bpa);
	else
		*bshp = MIPS_PHYS_TO_KSEG1(bpa);
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
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;
	u_long bpa;
	int err;

	if (!t->t_extent)
		panic("bus_space_alloc: no extent");

	rstart += t->t_base;
	if ((err = extent_alloc_subregion(t->t_extent, rstart, rend, size,
					  alignment, boundary, 
					  EX_FAST|EX_NOWAIT|EX_MALLOCOK, &bpa)))
		return err;
	if (cacheable)
		*bshp = MIPS_PHYS_TO_KSEG0(bpa);
	else
		*bshp = MIPS_PHYS_TO_KSEG1(bpa);
	*bpap = bpa;

	return 0;
}

/* ARGSUSED */
void
bus_space_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	/* bus_space_unmap() does all that we need to do. */
	bus_space_unmap(t, bsh, size);
}

void
bus_space_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	int err;
	u_int32_t addr;
	addr = MIPS_KSEG1_TO_PHYS(bsh);
	if (!t->t_extent)
		return; /* Before autoconfiguration, can't use extent */
	if ((err = extent_free(t->t_extent, addr, size, EX_NOWAIT)))
		printf("warning: %#x-%#x of %s space lost\n",
		       bsh, bsh+size, t->t_name);

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
