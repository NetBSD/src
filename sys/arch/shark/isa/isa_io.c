/*	$NetBSD: isa_io.c,v 1.12.24.1 2016/10/05 20:55:35 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: isa_io.c,v 1.12.24.1 2016/10/05 20:55:35 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <uvm/uvm.h>
#include <machine/pio.h>
#include <machine/isa_machdep.h>
#include <machine/ofw.h>
#include <machine/pmap.h>
#include "igsfb_ofbus.h"
#include "chipsfb_ofbus.h"

#if NIGSFB_OFBUS > 0
extern vaddr_t igsfb_mem_vaddr, igsfb_mmio_vaddr;
extern paddr_t igsfb_mem_paddr;
#endif

#if NCHIPSFB_OFBUS > 0
extern vaddr_t chipsfb_mem_vaddr, chipsfb_mmio_vaddr;
extern paddr_t chipsfb_mem_paddr;
#endif

/* Proto types for all the bus_space structure functions */

bs_protos(isa);
bs_protos(bs_notimpl);

/*
 * Declare the isa bus space tags
 * The IO and MEM structs are identical, except for the cookies,
 * which contain the address space bases.
 */

/*
 * NOTE: ASSEMBLY LANGUAGE RELIES ON THE COOKIE -- THE FIRST MEMBER OF 
 *       THIS STRUCTURE -- TO BE THE VIRTUAL ADDRESS OF ISA/IO!
 */
struct bus_space isa_io_bs_tag = {
	/* cookie */
	NULL,	/* initialized below */

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
	isa_bs_mmap,

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
	isa_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,

	/* stream methods are identical to regular read/write here */
	/* read stream single */
	isa_bs_r_1,
	isa_bs_r_2,
	isa_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read stream multiple */
	isa_bs_rm_1,
	isa_bs_rm_2,
	isa_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region stream */
	isa_bs_rr_1,
	isa_bs_rr_2,
	isa_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write stream single */
	isa_bs_w_1,
	isa_bs_w_2,
	isa_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write stream multiple */
	isa_bs_wm_1,
	isa_bs_wm_2,
	isa_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region stream */
	isa_bs_wr_1,
	isa_bs_wr_2,
	isa_bs_wr_4,
	bs_notimpl_bs_wr_8,
	
};

/*
 * NOTE: ASSEMBLY LANGUAGE RELIES ON THE COOKIE -- THE FIRST MEMBER OF 
 *       THIS STRUCTURE -- TO BE THE VIRTUAL ADDRESS OF ISA/MEMORY!
 */
struct bus_space isa_mem_bs_tag = {
	/* cookie */
        NULL,	/* initialized below */

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
	isa_bs_mmap,

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
	isa_bs_c_2,
	bs_notimpl_bs_c_4,
	bs_notimpl_bs_c_8,

	/* stream methods are identical to regular read/write here */
	/* read stream single */
	isa_bs_r_1,
	isa_bs_r_2,
	isa_bs_r_4,
	bs_notimpl_bs_r_8,

	/* read stream multiple */
	isa_bs_rm_1,
	isa_bs_rm_2,
	isa_bs_rm_4,
	bs_notimpl_bs_rm_8,

	/* read region stream */
	isa_bs_rr_1,
	isa_bs_rr_2,
	isa_bs_rr_4,
	bs_notimpl_bs_rr_8,

	/* write stream single */
	isa_bs_w_1,
	isa_bs_w_2,
	isa_bs_w_4,
	bs_notimpl_bs_w_8,

	/* write stream multiple */
	isa_bs_wm_1,
	isa_bs_wm_2,
	isa_bs_wm_4,
	bs_notimpl_bs_wm_8,

	/* write region stream */
	isa_bs_wr_1,
	isa_bs_wr_2,
	isa_bs_wr_4,
	bs_notimpl_bs_wr_8,
};

/* bus space functions */

void
isa_io_init(vaddr_t isa_io_addr, vaddr_t isa_mem_addr)
{
	isa_io_bs_tag.bs_cookie = (void *)isa_io_addr;
	isa_mem_bs_tag.bs_cookie = (void *)isa_mem_addr;
}

/*
 * break the abstraction: sometimes, other parts of the system
 * (e.g. X servers) need to map ISA space directly.  use these
 * functions sparingly!
 */
vaddr_t
isa_io_data_vaddr(void)
{
	return (vaddr_t)isa_io_bs_tag.bs_cookie;
}

vaddr_t
isa_mem_data_vaddr(void)
{
	return (vaddr_t)isa_mem_bs_tag.bs_cookie;
}

int
isa_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable, bus_space_handle_t *bshp)
{
	*bshp = bpa + (bus_addr_t)t;
	return(0);
}

void
isa_bs_unmap(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	/* Nothing to do. */
}

paddr_t
isa_bs_mmap(void *cookie, bus_addr_t addr, off_t off, int prot,
    int flags)
{
	paddr_t paddr, ret;

#ifdef OFISA_DEBUG
	printf("mmap %08x %08x %08x", (uint32_t)cookie, (uint32_t)addr, (uint32_t)off);
#endif
#if NIGSFB_OFBUS > 0
	if ((vaddr_t)cookie == igsfb_mem_vaddr) {
		paddr = igsfb_mem_paddr;
	} else
#endif
#if NCHIPSFB_OFBUS > 0
	if ((vaddr_t)cookie == chipsfb_mem_vaddr) {
		paddr = 0;
	} else
#endif
	paddr = ofw_gettranslation((vaddr_t)cookie);
	
	if (paddr == -1) {
#ifdef OFISA_DEBUG
		printf(" no translation\n");
#endif
		return -1;
	}
	ret = paddr + addr + off;
#ifdef OFISA_DEBUG
	printf(" -> %08x %08x\n", (uint32_t)paddr, (uint32_t)ret);
#endif
	if (flags & BUS_SPACE_MAP_PREFETCHABLE) {
		return (arm_btop(ret) | ARM32_MMAP_WRITECOMBINE);
	} else
		return arm_btop(ret);	
}

int
isa_bs_subregion(void *t, bus_space_handle_t bsh, bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{
/*	printf("isa_subregion(tag=%p, bsh=%lx, off=%lx, sz=%lx)\n",
	    t, bsh, offset, size);*/
	*nbshp = bsh + offset;
	return(0);
}

int
isa_bs_alloc(void *t, bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
	bus_size_t alignment, bus_size_t boundary, int cacheable,
	bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	panic("isa_alloc(): Help!");
}

void    
isa_bs_free(void *t, bus_space_handle_t bsh, bus_size_t size)
{
	panic("isa_free(): Help!");
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
