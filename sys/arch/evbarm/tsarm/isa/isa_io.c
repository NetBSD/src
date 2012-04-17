/*	$NetBSD: isa_io.c,v 1.8.2.1 2012/04/17 00:06:16 yamt Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * bus_space I/O functions for isa
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isa_io.c,v 1.8.2.1 2012/04/17 00:06:16 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/bus.h>
#include <machine/pio.h>
#include <machine/isa_machdep.h>

/* Proto types for all the bus_space structure functions */

bs_protos(isa);
bs_protos(bs_notimpl);
void isa_bs_mallocok(void);

/*
 * Declare the isa bus space tags
 * The IO and MEM structs are identical, except for the cookies,
 * which contain the address space bases.
 */

/*
 * NOTE: ASSEMBLY LANGUAGE RELIES ON THE COOKIE -- THE FIRST MEMBER OF 
 *       THIS STRUCTURE -- TO BE THE VIRTUAL ADDRESS OF 16 BIT ISA/IO!
 */
struct bus_space isa_io_bs_tag = {
	/* cookie */
	NULL, /* initialized below */

	/* mapping/unmapping */
	isa_bs_map,
	isa_bs_unmap,
	isa_bs_subregion,

	/* allocation/deallocation */
	isa_bs_alloc,
	isa_bs_free,

	/* get kernel virtual address */
	isa_bs_vaddr,

	/* mmap bus space for userland */
	bs_notimpl_bs_mmap,		/* XXX possible even? XXX */

	/* barrier */
	isa_bs_barrier,

	/* read (single) */
	isa_bs_r_1,
	isa_bs_r_2,
	isa_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read multiple */
	isa_bs_rm_1,
	isa_bs_rm_2,
	isa_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region */
	isa_bs_rr_1,
	isa_bs_rr_2,
	isa_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write (single) */
	isa_bs_w_1,
	isa_bs_w_2,
	isa_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	isa_bs_wm_1,
	isa_bs_wm_2,
	isa_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region */
	isa_bs_wr_1,
	isa_bs_wr_2,
	isa_bs_wr_4,
	bs_notimpl_bs_wr_8,

	/* set multiple */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	bs_notimpl_bs_sr_1,
	isa_bs_sr_2,
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	bs_notimpl_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,
};

/*
 * NOTE: ASSEMBLY LANGUAGE RELIES ON THE COOKIE -- THE FIRST MEMBER OF 
 *       THIS STRUCTURE -- TO BE THE VIRTUAL ADDRESS OF ISA/MEMORY!
 */
struct bus_space isa_mem_bs_tag = {
	/* cookie */
        NULL, /* initialized below */

	/* mapping/unmapping */
	isa_bs_map,
	isa_bs_unmap,
	isa_bs_subregion,

	/* allocation/deallocation */
	isa_bs_alloc,
	isa_bs_free,

	/* get kernel virtual address */
	isa_bs_vaddr,

	/* mmap bus space for userland */
	bs_notimpl_bs_mmap,		/* XXX open for now ... XXX */

	/* barrier */
	isa_bs_barrier,

	/* read (single) */
	isa_bs_r_1,
	isa_bs_r_2,
	isa_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read multiple */
	isa_bs_rm_1,
	isa_bs_rm_2,
	isa_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region */
	isa_bs_rr_1,
	isa_bs_rr_2,
	isa_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write (single) */
	isa_bs_w_1,
	isa_bs_w_2,
	isa_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write multiple */
	isa_bs_wm_1,
	isa_bs_wm_2,
	isa_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region */
	isa_bs_wr_1,
	isa_bs_wr_2,
	isa_bs_wr_4,
	bs_notimpl_bs_wr_8,

	/* set multiple */
	bs_notimpl_bs_sm_1,
	bs_notimpl_bs_sm_2,
	bs_notimpl_bs_sm_4,
	bs_notimpl_bs_sm_8,

	/* set region */
	bs_notimpl_bs_sr_1,
	isa_bs_sr_2,
	bs_notimpl_bs_sr_4,
	bs_notimpl_bs_sr_8,

	/* copy */
	bs_notimpl_bs_c_1,
	bs_notimpl_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,
};

static long isaio_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
static long isamem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
static int malloc_safe = 0;	
struct extent	*isaio_ex;
struct extent	*isamem_ex;

void
isa_bs_mallocok(void)
{
	malloc_safe = 1;
}

/* bus space functions */

void
isa_io_init(vm_offset_t isa_io_addr, vm_offset_t isa_mem_addr)
{
	isa_io_bs_tag.bs_cookie = (void *)isa_io_addr;
	isa_mem_bs_tag.bs_cookie = (void *)isa_mem_addr;

	isaio_ex = extent_create("isaio", 0x0, 0xffff, 
		(void *)isaio_ex_storage, sizeof(isaio_ex_storage),
		EX_NOWAIT|EX_NOCOALESCE);
	isamem_ex = extent_create("isamem", 0x0, 0xfffff, 
		(void *)isamem_ex_storage, sizeof(isamem_ex_storage),
		EX_NOWAIT|EX_NOCOALESCE);
	if (isaio_ex == NULL || isamem_ex == NULL)
		panic("isa_io_init(): can't alloc extent maps");
}

/*
 * break the abstraction: sometimes, other parts of the system
 * (e.g. X servers) need to map ISA space directly.  use these
 * functions sparingly!
 */
vm_offset_t
isa_io_data_vaddr(void)
{
	return (vm_offset_t)isa_io_bs_tag.bs_cookie;
}

vm_offset_t
isa_mem_data_vaddr(void)
{
	return (vm_offset_t)isa_mem_bs_tag.bs_cookie;
}

int
isa_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	struct extent *ex;
	int err;

	if (t == isa_io_bs_tag.bs_cookie) 
		ex = isaio_ex;
	else
		ex = isamem_ex;
	
	err = extent_alloc_region(ex, bpa, size,
		EX_NOWAIT|(malloc_safe ? EX_MALLOCOK : 0));
	if (err)
		return err;

	*bshp = bpa + (bus_addr_t)t;
	return(0);
}

void
isa_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	isa_bs_free(t, bsh, size);
}

int
isa_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;
	return(0);
}

int
isa_bs_alloc(
	void *t,
	bus_addr_t rstart,
	bus_addr_t rend,
	bus_size_t size,
	bus_size_t alignment,
	bus_size_t boundary,
	int cacheable,
	bus_addr_t *bpap,
	bus_space_handle_t *bshp)
{
	struct extent *ex;
	u_long bpa;
	int err;

	if (t == isa_io_bs_tag.bs_cookie) 
		ex = isaio_ex;
	else
		ex = isamem_ex;
	
	err = extent_alloc_subregion(ex, rstart, rend, size, alignment,
		boundary, (EX_FAST|EX_NOWAIT|(malloc_safe ? EX_MALLOCOK : 0)), 
		&bpa);

	if (err)
		return err;

	*bshp = *bpap = bpa + (bus_addr_t)t;
	return 0;
}

void    
isa_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	struct extent *ex;

	if (t == isa_io_bs_tag.bs_cookie) 
		ex = isaio_ex;
	else
		ex = isamem_ex;

	extent_free(ex, bsh - (bus_addr_t)t, size,
		EX_NOWAIT|(malloc_safe ? EX_MALLOCOK : 0));
}

void *
isa_bs_vaddr(void *t, bus_space_handle_t bsh)
{

	return ((void *)bsh);
}

void
isa_bs_barrier(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t len, int flags)
{
	/* just return */
}	
