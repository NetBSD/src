/*	$NetBSD: bus_space.c,v 1.4.2.2 2002/03/16 15:58:07 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include "debug_hpcsh.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <machine/bus.h>

/* bus.h turn on BUS_SPACE_DEBUG if the global DEBUG option is enabled. */
#ifdef	BUS_SPACE_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	bus_space_debug
#endif
#include <machine/debug.h>

#define _BUS_SPACE_ACCESS_HOOK()	((void)0)
_BUS_SPACE_READ(_bus_space, 1, 8)
_BUS_SPACE_READ(_bus_space, 2, 16)
_BUS_SPACE_READ(_bus_space, 4, 32)
_BUS_SPACE_READ(_bus_space, 8, 64)
_BUS_SPACE_READ_MULTI(_bus_space, 1, 8)
_BUS_SPACE_READ_MULTI(_bus_space, 2, 16)
_BUS_SPACE_READ_MULTI(_bus_space, 4, 32)
_BUS_SPACE_READ_MULTI(_bus_space, 8, 64)
_BUS_SPACE_READ_REGION(_bus_space, 1, 8)
_BUS_SPACE_READ_REGION(_bus_space, 2, 16)
_BUS_SPACE_READ_REGION(_bus_space, 4, 32)
_BUS_SPACE_READ_REGION(_bus_space, 8, 64)
_BUS_SPACE_WRITE(_bus_space, 1, 8)
_BUS_SPACE_WRITE(_bus_space, 2, 16)
_BUS_SPACE_WRITE(_bus_space, 4, 32)
_BUS_SPACE_WRITE(_bus_space, 8, 64)
_BUS_SPACE_WRITE_MULTI(_bus_space, 1, 8)
_BUS_SPACE_WRITE_MULTI(_bus_space, 2, 16)
_BUS_SPACE_WRITE_MULTI(_bus_space, 4, 32)
_BUS_SPACE_WRITE_MULTI(_bus_space, 8, 64)
_BUS_SPACE_WRITE_REGION(_bus_space, 1, 8)
_BUS_SPACE_WRITE_REGION(_bus_space, 2, 16)
_BUS_SPACE_WRITE_REGION(_bus_space, 4, 32)
_BUS_SPACE_WRITE_REGION(_bus_space, 8, 64)
_BUS_SPACE_SET_MULTI(_bus_space, 1, 8)
_BUS_SPACE_SET_MULTI(_bus_space, 2, 16)
_BUS_SPACE_SET_MULTI(_bus_space, 4, 32)
_BUS_SPACE_SET_MULTI(_bus_space, 8, 64)
_BUS_SPACE_COPY_REGION(_bus_space, 1, 8)
_BUS_SPACE_COPY_REGION(_bus_space, 2, 16)
_BUS_SPACE_COPY_REGION(_bus_space, 4, 32)
_BUS_SPACE_COPY_REGION(_bus_space, 8, 64)
#undef _BUS_SPACE_ACCESS_HOOK

static int _bus_space_map(void *, bus_addr_t, bus_size_t, int,
			  bus_space_handle_t *);
static void _bus_space_unmap(void *, bus_space_handle_t, bus_size_t);
static int _bus_space_subregion(void *, bus_space_handle_t, bus_size_t,
				bus_size_t, bus_space_handle_t *);
static int _bus_space_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t,
			    bus_size_t, bus_size_t, int,
			    bus_addr_t *, bus_space_handle_t *);
static void _bus_space_free(void *, bus_space_handle_t, bus_size_t);
static void *_bus_space_vaddr(void *, bus_space_handle_t);

static struct hpcsh_bus_space __default_bus_space = {
	hbs_extent	: 0,
	hbs_map		: _bus_space_map,
	hbs_unmap	: _bus_space_unmap,
	hbs_subregion	: _bus_space_subregion,
	hbs_alloc	: _bus_space_alloc,
	hbs_free	: _bus_space_free,
	hbs_vaddr	: _bus_space_vaddr,
	hbs_r_1		: _bus_space_read_1,
	hbs_r_2		: _bus_space_read_2,
	hbs_r_4		: _bus_space_read_4,
	hbs_r_8		: _bus_space_read_8,
	hbs_rm_1	: _bus_space_read_multi_1,
	hbs_rm_2	: _bus_space_read_multi_2,
	hbs_rm_4	: _bus_space_read_multi_4,
	hbs_rm_8	: _bus_space_read_multi_8,
	hbs_rr_1	: _bus_space_read_region_1,
	hbs_rr_2	: _bus_space_read_region_2,
	hbs_rr_4	: _bus_space_read_region_4,
	hbs_rr_8	: _bus_space_read_region_8,
	hbs_w_1		: _bus_space_write_1,
	hbs_w_2		: _bus_space_write_2,
	hbs_w_4		: _bus_space_write_4,
	hbs_w_8		: _bus_space_write_8,
	hbs_wm_1	: _bus_space_write_multi_1,
	hbs_wm_2	: _bus_space_write_multi_2,
	hbs_wm_4	: _bus_space_write_multi_4,
	hbs_wm_8	: _bus_space_write_multi_8,
	hbs_wr_1	: _bus_space_write_region_1,
	hbs_wr_2	: _bus_space_write_region_2,
	hbs_wr_4	: _bus_space_write_region_4,
	hbs_wr_8	: _bus_space_write_region_8,
	hbs_sm_1	: _bus_space_set_multi_1,
	hbs_sm_2	: _bus_space_set_multi_2,
	hbs_sm_4	: _bus_space_set_multi_4,
	hbs_sm_8	: _bus_space_set_multi_8,
	hbs_c_1		: _bus_space_copy_region_1,
	hbs_c_2		: _bus_space_copy_region_2,
	hbs_c_4		: _bus_space_copy_region_4,
	hbs_c_8		: _bus_space_copy_region_8
};

/* create default bus_space_tag */
bus_space_tag_t
bus_space_create(struct hpcsh_bus_space *hbs, const char *name,
		 bus_addr_t addr, bus_size_t size)
{
	if (hbs == 0)
		hbs = malloc(sizeof(*hbs), M_DEVBUF, M_NOWAIT);
	KASSERT(hbs);

	memset(hbs, 0, sizeof(*hbs));

	/* set default method */
	*hbs = __default_bus_space;
	hbs->hbs_cookie = hbs;

	/* set access region */
	if (size == 0) {
		hbs->hbs_base_addr = addr; /* no extent */
	} else {
		hbs->hbs_extent = extent_create(name, addr, addr + size - 1,
						M_DEVBUF, 0, 0, EX_NOWAIT);
		if (hbs->hbs_extent == 0) {
			panic("%s:: unable to create bus_space for "
			      "0x%08lx-%#lx\n", __FUNCTION__, addr, size);
		}
	}

	return hbs;
}

void
bus_space_destroy(bus_space_tag_t t)
{
	struct hpcsh_bus_space *hbs = t;
	struct extent *ex = hbs->hbs_extent;

	if (ex != 0)
		extent_destroy(ex);

	free(t, M_DEVBUF);
}

/* default bus_space tag */
static int
_bus_space_map(void *t, bus_addr_t bpa, bus_size_t size, int flags,
	       bus_space_handle_t *bshp)
{
	struct hpcsh_bus_space *hbs = t;
	struct extent *ex = hbs->hbs_extent;
	int error;

	if (ex == 0) {
		*bshp = (bus_space_handle_t)(bpa + hbs->hbs_base_addr);
		return (0);
	}

	bpa += ex->ex_start;
	error = extent_alloc_region(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK);

	if (error) {
		DPRINTF("failed.\n");
		return (error);
	}

	*bshp = (bus_space_handle_t)bpa;

	return (0);
}

static int
_bus_space_subregion(void *t, bus_space_handle_t bsh,
		     bus_size_t offset, bus_size_t size,
		     bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;

	return (0);
}

static int
_bus_space_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
		 bus_size_t size, bus_size_t alignment, bus_size_t boundary,
		 int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	struct hpcsh_bus_space *hbs = t;
	struct extent *ex = hbs->hbs_extent;
	u_long bpa, base;
	int error;

	if (ex == 0) {
		*bshp = *bpap = rstart + hbs->hbs_base_addr;
		return (0);
	}

	base = ex->ex_start;

	error = extent_alloc_subregion(ex, rstart + base, rend + base, size,
				       alignment, boundary, 
				       EX_FAST | EX_NOWAIT | EX_MALLOCOK,
				       &bpa);

	if (error) {
		DPRINTF("failed. base=0x%08x rstart=0x%08x, rend=0x%08x"
		    " size=0x%08x\n", (u_int32_t)base, (u_int32_t)rstart,
		    (u_int32_t)rend, (u_int32_t)size);
		return (error);
	}

	*bshp = (bus_space_handle_t)bpa;

	if (bpap)
		*bpap = bpa;

	return (0);
}

static void
_bus_space_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct hpcsh_bus_space *hbs = t;
	struct extent *ex = hbs->hbs_extent;
	
	if (ex != 0)
		_bus_space_unmap(t, bsh, size);
}

static void
_bus_space_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct hpcsh_bus_space *hbs = t;
	struct extent *ex = hbs->hbs_extent;
	int error;

	if (ex == 0)
		return;

	error = extent_free(ex, bsh, size, EX_NOWAIT);

	if (error) {
		DPRINTF("%#lx-%#lx of %s space lost\n", bsh, bsh + size,
			ex->ex_name);
	}
}

void *
_bus_space_vaddr(void *t, bus_space_handle_t h)
{
	return (void *)h;
}
