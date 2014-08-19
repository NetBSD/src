/*	$NetBSD: bus_space.c,v 1.10.4.2 2014/08/20 00:03:18 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.10.4.2 2014/08/20 00:03:18 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#define _PLAYSTATION2_BUS_SPACE_PRIVATE
#include <machine/bus.h>

#ifdef BUS_SPACE_DEBUG
int	bus_space_debug = 0;
#define	DPRINTF(fmt, args...)						\
	if (bus_space_debug)						\
		printf("%s: " fmt, __func__ , ##args) 
#define	DPRINTFN(n, arg)						\
	if (bus_space_debug > (n))					\
		printf("%s: " fmt, __func__ , ##args) 
#else
#define	DPRINTF(arg...)		((void)0)
#define DPRINTFN(n, arg...)	((void)0)
#endif

#define VADDR(h, o)	(h + o)
_BUS_SPACE_READ(_default, 1, 8)
_BUS_SPACE_READ(_default, 2, 16)
_BUS_SPACE_READ(_default, 4, 32)
_BUS_SPACE_READ(_default, 8, 64)
_BUS_SPACE_READ_MULTI(_default, 1, 8)
_BUS_SPACE_READ_MULTI(_default, 2, 16)
_BUS_SPACE_READ_MULTI(_default, 4, 32)
_BUS_SPACE_READ_MULTI(_default, 8, 64)
_BUS_SPACE_READ_REGION(_default, 1, 8)
_BUS_SPACE_READ_REGION(_default, 2, 16)
_BUS_SPACE_READ_REGION(_default, 4, 32)
_BUS_SPACE_READ_REGION(_default, 8, 64)
_BUS_SPACE_WRITE(_default, 1, 8)
_BUS_SPACE_WRITE(_default, 2, 16)
_BUS_SPACE_WRITE(_default, 4, 32)
_BUS_SPACE_WRITE(_default, 8, 64)
_BUS_SPACE_WRITE_MULTI(_default, 1, 8)
_BUS_SPACE_WRITE_MULTI(_default, 2, 16)
_BUS_SPACE_WRITE_MULTI(_default, 4, 32)
_BUS_SPACE_WRITE_MULTI(_default, 8, 64)
_BUS_SPACE_WRITE_REGION(_default, 1, 8)
_BUS_SPACE_WRITE_REGION(_default, 2, 16)
_BUS_SPACE_WRITE_REGION(_default, 4, 32)
_BUS_SPACE_WRITE_REGION(_default, 8, 64)
_BUS_SPACE_SET_MULTI(_default, 1, 8)
_BUS_SPACE_SET_MULTI(_default, 2, 16)
_BUS_SPACE_SET_MULTI(_default, 4, 32)
_BUS_SPACE_SET_MULTI(_default, 8, 64)
_BUS_SPACE_SET_REGION(_default, 1, 8)
_BUS_SPACE_SET_REGION(_default, 2, 16)
_BUS_SPACE_SET_REGION(_default, 4, 32)
_BUS_SPACE_SET_REGION(_default, 8, 64)
_BUS_SPACE_COPY_REGION(_default, 1, 8)
_BUS_SPACE_COPY_REGION(_default, 2, 16)
_BUS_SPACE_COPY_REGION(_default, 4, 32)
_BUS_SPACE_COPY_REGION(_default, 8, 64)
#undef VADDR

static int _default_map(void *, bus_addr_t, bus_size_t, int,
    bus_space_handle_t *);
static void _default_unmap(void *, bus_space_handle_t, bus_size_t);
static int _default_subregion(void *, bus_space_handle_t, bus_size_t,
    bus_size_t, bus_space_handle_t *);
static int _default_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t,
    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
static void _default_free(void *, bus_space_handle_t, bus_size_t);
static void *_default_vaddr(void *, bus_space_handle_t);

static const struct playstation2_bus_space _default_bus_space = {
	pbs_map		: _default_map,
	pbs_unmap	: _default_unmap,
	pbs_subregion	: _default_subregion,
	pbs_alloc	: _default_alloc,
	pbs_free	: _default_free,
	pbs_vaddr	: _default_vaddr,
	pbs_r_1		: _default_read_1,
	pbs_r_2		: _default_read_2,
	pbs_r_4		: _default_read_4,
	pbs_r_8		: _default_read_8,
	pbs_rm_1	: _default_read_multi_1,
	pbs_rm_2	: _default_read_multi_2,
	pbs_rm_4	: _default_read_multi_4,
	pbs_rm_8	: _default_read_multi_8,
	pbs_rr_1	: _default_read_region_1,
	pbs_rr_2	: _default_read_region_2,
	pbs_rr_4	: _default_read_region_4,
	pbs_rr_8	: _default_read_region_8,
	pbs_w_1		: _default_write_1,
	pbs_w_2		: _default_write_2,
	pbs_w_4		: _default_write_4,
	pbs_w_8		: _default_write_8,
	pbs_wm_1	: _default_write_multi_1,
	pbs_wm_2	: _default_write_multi_2,
	pbs_wm_4	: _default_write_multi_4,
	pbs_wm_8	: _default_write_multi_8,
	pbs_wr_1	: _default_write_region_1,
	pbs_wr_2	: _default_write_region_2,
	pbs_wr_4	: _default_write_region_4,
	pbs_wr_8	: _default_write_region_8,
	pbs_sm_1	: _default_set_multi_1,
	pbs_sm_2	: _default_set_multi_2,
	pbs_sm_4	: _default_set_multi_4,
	pbs_sm_8	: _default_set_multi_8,
	pbs_sr_1	: _default_set_region_1,
	pbs_sr_2	: _default_set_region_2,
	pbs_sr_4	: _default_set_region_4,
	pbs_sr_8	: _default_set_region_8,
	pbs_c_1		: _default_copy_region_1,
	pbs_c_2		: _default_copy_region_2,
	pbs_c_4		: _default_copy_region_4,
	pbs_c_8		: _default_copy_region_8
};

/* create default bus_space_tag */
bus_space_tag_t
bus_space_create(bus_space_tag_t t, const char *name,
    bus_addr_t addr, bus_size_t size)
{
	struct playstation2_bus_space *pbs = (void *)__UNCONST(t);

	if (pbs == 0)
		pbs = malloc(sizeof(*pbs), M_DEVBUF, M_NOWAIT);
	KDASSERT(pbs);

	memset(pbs, 0, sizeof(*pbs));

	/* set default method */
	*pbs = _default_bus_space;
	pbs->pbs_cookie = pbs;

	/* set access region */
	if (size == 0) {
		pbs->pbs_base_addr = addr; /* no extent */
	} else {
		pbs->pbs_extent = extent_create(name, addr, addr + size - 1,
		    0, 0, EX_NOWAIT);
		if (pbs->pbs_extent == 0) {
			panic("%s:: unable to create bus_space for "
			    "0x%08lx-%#lx", __func__, addr, size);
		}
	}

	return (pbs);
}

void
bus_space_destroy(bus_space_tag_t t)
{
	struct playstation2_bus_space *pbs = (void *)__UNCONST(t);
	struct extent *ex = pbs->pbs_extent;

	if (ex != 0)
		extent_destroy(ex);

	free(pbs, M_DEVBUF);
}

void
_bus_space_invalid_access(void)
{
	panic("invalid bus space access.");
}

/* default bus_space tag */
static int
_default_map(void *t, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	struct playstation2_bus_space *pbs = t;
	struct extent *ex = pbs->pbs_extent;
	int error;

	if (ex == 0) {
		*bshp = (bus_space_handle_t)(bpa + pbs->pbs_base_addr);
		return (0);
	}

	bpa += ex->ex_start;
	error = extent_alloc_region(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK);

	if (error) {
		DPRINTF("failed.\n");
		return (error);
	}

	*bshp = (bus_space_handle_t)bpa;

	DPRINTF("success.\n");

	return (0);
}

static int
_default_subregion(void *t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;

	return (0);
}

static int
_default_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary,
    int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	struct playstation2_bus_space *pbs = t;
	struct extent *ex = pbs->pbs_extent;
	u_long bpa, base;
	int error;

	if (ex == 0) {
		*bshp = *bpap = rstart + pbs->pbs_base_addr;
		return (0);
	}

	base = ex->ex_start;

	error = extent_alloc_subregion(ex, rstart + base, rend + base, size,
	    alignment, boundary, 
	    EX_FAST | EX_NOWAIT | EX_MALLOCOK,
	    &bpa);

	if (error) {
		DPRINTF("failed.\n");
		return (error);
	}

	*bshp = (bus_space_handle_t)bpa;

	if (bpap)
		*bpap = bpa;

	DPRINTF("success.\n");

	return (0);
}

static void
_default_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct playstation2_bus_space *pbs = t;
	struct extent *ex = pbs->pbs_extent;
	
	if (ex != 0)
		_default_unmap(t, bsh, size);
}

static void
_default_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct playstation2_bus_space *pbs = t;
	struct extent *ex = pbs->pbs_extent;
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
_default_vaddr(void *t, bus_space_handle_t h)
{
	return (void *)h;
}
