/*	$NetBSD: bus_space.c,v 1.2 2006/06/01 14:18:41 tsutsui Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.2 2006/06/01 14:18:41 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#define	_EWS4800MIPS_BUS_SPACE_PRIVATE
#include <machine/bus.h>
#include <machine/sbdvar.h>

#ifdef BUS_SPACE_DEBUG
int	bus_space_debug = 0;
#define	DPRINTF(fmt, args...)						\
	if (bus_space_debug)						\
		printf("%s: " fmt, __FUNCTION__ , ##args)
#define	DPRINTFN(n, arg)						\
	if (bus_space_debug > (n))					\
		printf("%s: " fmt, __FUNCTION__ , ##args)
#else
#define	DPRINTF(arg...)		((void)0)
#define	DPRINTFN(n, arg...)	((void)0)
#endif

#define	VADDR(h, o)	(h + o)
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

static const struct ews4800mips_bus_space _default_bus_space = {
	.ebs_map	= _default_map,
	.ebs_unmap	= _default_unmap,
	.ebs_subregion	= _default_subregion,
	.ebs_alloc	= _default_alloc,
	.ebs_free	= _default_free,
	.ebs_vaddr	= _default_vaddr,
	.ebs_r_1	= _default_read_1,
	.ebs_r_2	= _default_read_2,
	.ebs_r_4	= _default_read_4,
	.ebs_r_8	= _default_read_8,
	.ebs_rm_1	= _default_read_multi_1,
	.ebs_rm_2	= _default_read_multi_2,
	.ebs_rm_4	= _default_read_multi_4,
	.ebs_rm_8	= _default_read_multi_8,
	.ebs_rr_1	= _default_read_region_1,
	.ebs_rr_2	= _default_read_region_2,
	.ebs_rr_4	= _default_read_region_4,
	.ebs_rr_8	= _default_read_region_8,
	.ebs_w_1	= _default_write_1,
	.ebs_w_2	= _default_write_2,
	.ebs_w_4	= _default_write_4,
	.ebs_w_8	= _default_write_8,
	.ebs_wm_1	= _default_write_multi_1,
	.ebs_wm_2	= _default_write_multi_2,
	.ebs_wm_4	= _default_write_multi_4,
	.ebs_wm_8	= _default_write_multi_8,
	.ebs_wr_1	= _default_write_region_1,
	.ebs_wr_2	= _default_write_region_2,
	.ebs_wr_4	= _default_write_region_4,
	.ebs_wr_8	= _default_write_region_8,
	.ebs_sm_1	= _default_set_multi_1,
	.ebs_sm_2	= _default_set_multi_2,
	.ebs_sm_4	= _default_set_multi_4,
	.ebs_sm_8	= _default_set_multi_8,
	.ebs_sr_1	= _default_set_region_1,
	.ebs_sr_2	= _default_set_region_2,
	.ebs_sr_4	= _default_set_region_4,
	.ebs_sr_8	= _default_set_region_8,
	.ebs_c_1	= _default_copy_region_1,
	.ebs_c_2	= _default_copy_region_2,
	.ebs_c_4	= _default_copy_region_4,
	.ebs_c_8	= _default_copy_region_8
};

/* create default bus_space_tag */
int
bus_space_create(bus_space_tag_t t, const char *name,
    bus_addr_t addr, bus_size_t size)
{
	struct ews4800mips_bus_space *ebs = t;

	if (ebs == NULL)
		return EFAULT;

	/* set default method */
	*ebs = _default_bus_space;	/* structure assignment */
	ebs->ebs_cookie = ebs;

	/* set access region */
	if (size == 0) {
		/* no extent */
		ebs->ebs_base_addr = addr;
		ebs->ebs_size = size;
	} else {
		ebs->ebs_extent = extent_create(name, addr, addr + size - 1,
		    M_DEVBUF, 0, 0, EX_NOWAIT);
		if (ebs->ebs_extent == NULL) {
			panic("%s:: unable to create bus_space for "
			    "0x%08lx-%#lx", __FUNCTION__, addr, size);
		}
	}

	return 0;
}

void
bus_space_destroy(bus_space_tag_t t)
{
	struct ews4800mips_bus_space *ebs = t;
	struct extent *ex = ebs->ebs_extent;

	if (ex != NULL)
		extent_destroy(ex);
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
	struct ews4800mips_bus_space *ebs = t;
	struct extent *ex = ebs->ebs_extent;
	int error;

	if (ex == NULL) {
		if (bpa + size > ebs->ebs_size)
			return EFAULT;
		*bshp = (bus_space_handle_t)(ebs->ebs_base_addr + bpa);
		return 0;
	}

	bpa += ex->ex_start;
	error = extent_alloc_region(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK);

	if (error) {
		DPRINTF("failed.\n");
		return error;
	}

	*bshp = (bus_space_handle_t)bpa;

	DPRINTF("success.\n");

	return 0;
}

static int
_default_subregion(void *t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;

	return 0;
}

static int
_default_alloc(void *t, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary,
    int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	struct ews4800mips_bus_space *ebs = t;
	struct extent *ex = ebs->ebs_extent;
	u_long bpa, base;
	int error;

	if (ex == NULL) {
		if (rend > ebs->ebs_size)
			return EFAULT;
		*bshp = *bpap = rstart + ebs->ebs_base_addr;
		return 0;
	}

	base = ex->ex_start;

	error = extent_alloc_subregion(ex, rstart + base, rend + base, size,
	    alignment, boundary,
	    EX_FAST | EX_NOWAIT | EX_MALLOCOK,
	    &bpa);

	if (error) {
		DPRINTF("failed.\n");
		return error;
	}

	*bshp = (bus_space_handle_t)bpa;

	if (bpap)
		*bpap = bpa;

	DPRINTF("success.\n");

	return 0;
}

static void
_default_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct ews4800mips_bus_space *ebs = t;
	struct extent *ex = ebs->ebs_extent;

	if (ex != NULL)
		_default_unmap(t, bsh, size);
}

static void
_default_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct ews4800mips_bus_space *ebs = t;
	struct extent *ex = ebs->ebs_extent;
	int error;

	if (ex == NULL)
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
