/*	$NetBSD: bus_space.c,v 1.1.2.3 2001/03/12 13:28:52 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
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
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>
#include <machine/bus.h>

#ifdef BUS_SPACE_DEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define BUS_SPACE_TAG_MAX		16

static struct bus_space {
	vaddr_t t_base;		/* extent base */
	vsize_t t_size;		/* extent size */	
	struct extent *t_extent;
} bus_space[BUS_SPACE_TAG_MAX];
static int bus_space_cnt;

#define CONTEXT_REF(x, t)	struct bus_space *x = (struct bus_space *)(t)
#define ANONYMOUS(x)		(x == BUS_SPACE_TAG_ANONYMOUS)

bus_space_tag_t
bus_space_create(const char *name, bus_addr_t addr, bus_size_t size)
{
	struct bus_space *t;

	KASSERT(bus_space_cnt < BUS_SPACE_TAG_MAX);

	t = &bus_space[bus_space_cnt++];
	t->t_base = addr;
	t->t_size = size;
	t->t_extent = extent_create(name, t->t_base, t->t_base + t->t_size,
				    M_DEVBUF, 0, 0, EX_NOWAIT);

	if (!t->t_extent) {
		panic("bus_space_create: unable to create bus_space for"
		      " 0x%08x-%#x\n", (unsigned)addr, (unsigned)size);
	}

	return (bus_space_tag_t)t;
}

int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
	      bus_space_handle_t *bshp)
{
	int error;
	CONTEXT_REF(_t, t);

	if (ANONYMOUS(_t)) {
		*bshp = (bus_space_handle_t)bpa;
		return (0);
	}

	bpa += _t->t_base;
	error = extent_alloc_region(_t->t_extent, bpa, size,
				    EX_NOWAIT | EX_MALLOCOK);
	if (error)
		return (error);

	*bshp = (bus_space_handle_t)bpa;

	DPRINTF(("\tbus_space_map:%#x(%#x)+%#x\n", bpa,
		 bpa - t->t_base, size));

	return (0);
}

int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
		   bus_size_t offset, bus_size_t size,
		   bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;

	return (0);
}

int
bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
	       bus_size_t size, bus_size_t alignment, bus_size_t boundary,
	       int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	u_long bpa;
	int error;
	CONTEXT_REF(_t, t);

	if (ANONYMOUS(_t)) {
		*bshp = *bpap = rstart;
		return (0);
	}

	rstart += _t->t_base;
	rend += _t->t_base;
	error = extent_alloc_subregion(_t->t_extent, rstart, rend, size,
				       alignment, boundary, 
				       EX_FAST | EX_NOWAIT | EX_MALLOCOK,
				       &bpa);
	if (error)
		return (error);

	*bshp = (bus_space_handle_t)bpa;

	if (bpap)
		*bpap = bpa;

	DPRINTF(("\tbus_space_alloc:%#x(%#x)+%#x\n", (unsigned)bpa,
		 (unsigned)(bpa - t->t_base), size));

	return (0);
}

void
bus_space_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	CONTEXT_REF(_t, t);
	
	if (!ANONYMOUS(_t)) {
		bus_space_unmap(t, bsh, size);
	}
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	int error;
	CONTEXT_REF(_t, t);

	if (ANONYMOUS(_t))
		return;

	error = extent_free(_t->t_extent, bsh, size, EX_NOWAIT);

	if (error) {
		DPRINTF(("warning: %#x-%#x of %s space lost\n",
			 bsh, bsh+size, t->t_name));
	}
}
