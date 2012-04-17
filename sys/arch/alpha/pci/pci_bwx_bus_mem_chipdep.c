/* $NetBSD: pci_bwx_bus_mem_chipdep.c,v 1.23.2.1 2012/04/17 00:05:57 yamt Exp $ */

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Common PCI Chipset "bus I/O" functions, for chipsets which have to
 * deal with only a single PCI interface chip in a machine.
 *
 * uses:
 *	CHIP		name of the 'chip' it's being compiled for.
 *	CHIP_MEM_BASE	Mem space base to use.
 *	CHIP_MEM_EX_STORE
 *			If defined, device-provided static storage area
 *			for the memory space extent.  If this is
 *			defined, CHIP_MEM_EX_STORE_SIZE must also be
 *			defined.  If this is not defined, a static area
 *			will be declared.
 *	CHIP_MEM_EX_STORE_SIZE
 *			Size of the device-provided static storage area
 *			for the memory space extent.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: pci_bwx_bus_mem_chipdep.c,v 1.23.2.1 2012/04/17 00:05:57 yamt Exp $");

#include <sys/extent.h>

#include <machine/bwx.h>

#define	__C(A,B)	__CONCAT(A,B)
#define	__S(S)		__STRING(S)

/* mapping/unmapping */
int		__C(CHIP,_mem_map)(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int);
void		__C(CHIP,_mem_unmap)(void *, bus_space_handle_t,
		    bus_size_t, int);
int		__C(CHIP,_mem_subregion)(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, bus_space_handle_t *);

int		__C(CHIP,_mem_translate)(void *, bus_addr_t, bus_size_t,
		    int, struct alpha_bus_space_translation *);
int		__C(CHIP,_mem_get_window)(void *, int,
		    struct alpha_bus_space_translation *);

/* allocation/deallocation */
int		__C(CHIP,_mem_alloc)(void *, bus_addr_t, bus_addr_t,
		    bus_size_t, bus_size_t, bus_addr_t, int, bus_addr_t *,
	            bus_space_handle_t *);
void		__C(CHIP,_mem_free)(void *, bus_space_handle_t,
		    bus_size_t);

/* get kernel virtual address */
void *		__C(CHIP,_mem_vaddr)(void *, bus_space_handle_t);

/* mmap for user */
paddr_t		__C(CHIP,_mem_mmap)(void *, bus_addr_t, off_t, int, int);

/* barrier */
static inline void	__C(CHIP,_mem_barrier)(void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, int);

/* read (single) */
static inline uint8_t	__C(CHIP,_mem_read_1)(void *, bus_space_handle_t,
		    bus_size_t);
static inline uint16_t __C(CHIP,_mem_read_2)(void *, bus_space_handle_t,
		    bus_size_t);
static inline uint32_t __C(CHIP,_mem_read_4)(void *, bus_space_handle_t,
		    bus_size_t);
static inline uint64_t __C(CHIP,_mem_read_8)(void *, bus_space_handle_t,
		    bus_size_t);

/* read multiple */
void		__C(CHIP,_mem_read_multi_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);
void		__C(CHIP,_mem_read_multi_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t *, bus_size_t);
void		__C(CHIP,_mem_read_multi_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t *, bus_size_t);
void		__C(CHIP,_mem_read_multi_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t *, bus_size_t);

/* read region */
void		__C(CHIP,_mem_read_region_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t *, bus_size_t);
void		__C(CHIP,_mem_read_region_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t *, bus_size_t);
void		__C(CHIP,_mem_read_region_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t *, bus_size_t);
void		__C(CHIP,_mem_read_region_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t *, bus_size_t);

/* write (single) */
static inline void	__C(CHIP,_mem_write_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t);
static inline void	__C(CHIP,_mem_write_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t);
static inline void	__C(CHIP,_mem_write_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t);
static inline void	__C(CHIP,_mem_write_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t);

/* write multiple */
void		__C(CHIP,_mem_write_multi_1)(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);
void		__C(CHIP,_mem_write_multi_2)(void *, bus_space_handle_t,
		    bus_size_t, const uint16_t *, bus_size_t);
void		__C(CHIP,_mem_write_multi_4)(void *, bus_space_handle_t,
		    bus_size_t, const uint32_t *, bus_size_t);
void		__C(CHIP,_mem_write_multi_8)(void *, bus_space_handle_t,
		    bus_size_t, const uint64_t *, bus_size_t);

/* write region */
void		__C(CHIP,_mem_write_region_1)(void *, bus_space_handle_t,
		    bus_size_t, const uint8_t *, bus_size_t);
void		__C(CHIP,_mem_write_region_2)(void *, bus_space_handle_t,
		    bus_size_t, const uint16_t *, bus_size_t);
void		__C(CHIP,_mem_write_region_4)(void *, bus_space_handle_t,
		    bus_size_t, const uint32_t *, bus_size_t);
void		__C(CHIP,_mem_write_region_8)(void *, bus_space_handle_t,
		    bus_size_t, const uint64_t *, bus_size_t);

/* set multiple */
void		__C(CHIP,_mem_set_multi_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);
void		__C(CHIP,_mem_set_multi_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t, bus_size_t);
void		__C(CHIP,_mem_set_multi_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t, bus_size_t);
void		__C(CHIP,_mem_set_multi_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t, bus_size_t);

/* set region */
void		__C(CHIP,_mem_set_region_1)(void *, bus_space_handle_t,
		    bus_size_t, uint8_t, bus_size_t);
void		__C(CHIP,_mem_set_region_2)(void *, bus_space_handle_t,
		    bus_size_t, uint16_t, bus_size_t);
void		__C(CHIP,_mem_set_region_4)(void *, bus_space_handle_t,
		    bus_size_t, uint32_t, bus_size_t);
void		__C(CHIP,_mem_set_region_8)(void *, bus_space_handle_t,
		    bus_size_t, uint64_t, bus_size_t);

/* copy */
void		__C(CHIP,_mem_copy_region_1)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		__C(CHIP,_mem_copy_region_2)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		__C(CHIP,_mem_copy_region_4)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);
void		__C(CHIP,_mem_copy_region_8)(void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t);

#ifndef	CHIP_MEM_EX_STORE
static long
    __C(CHIP,_mem_ex_storage)[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
#define	CHIP_MEM_EX_STORE(v)		(__C(CHIP,_mem_ex_storage))
#define	CHIP_MEM_EX_STORE_SIZE(v)	(sizeof __C(CHIP,_mem_ex_storage))
#endif

void
__C(CHIP,_bus_mem_init)(
	bus_space_tag_t t,
	void *v)
{
	struct extent *ex;

	/*
	 * Initialize the bus space tag.
	 */

	/* cookie */
	t->abs_cookie =		v;

	/* mapping/unmapping */
	t->abs_map =		__C(CHIP,_mem_map);
	t->abs_unmap =		__C(CHIP,_mem_unmap);
	t->abs_subregion =	__C(CHIP,_mem_subregion);

	t->abs_translate =	__C(CHIP,_mem_translate);
	t->abs_get_window =	__C(CHIP,_mem_get_window);

	/* allocation/deallocation */
	t->abs_alloc =		__C(CHIP,_mem_alloc);
	t->abs_free = 		__C(CHIP,_mem_free);

	/* get kernel virtual address */
	t->abs_vaddr =		__C(CHIP,_mem_vaddr);

	/* mmap for user */
	t->abs_mmap =		__C(CHIP,_mem_mmap);

	/* barrier */
	t->abs_barrier =	__C(CHIP,_mem_barrier);
	
	/* read (single) */
	t->abs_r_1 =		__C(CHIP,_mem_read_1);
	t->abs_r_2 =		__C(CHIP,_mem_read_2);
	t->abs_r_4 =		__C(CHIP,_mem_read_4);
	t->abs_r_8 =		__C(CHIP,_mem_read_8);
	
	/* read multiple */
	t->abs_rm_1 =		__C(CHIP,_mem_read_multi_1);
	t->abs_rm_2 =		__C(CHIP,_mem_read_multi_2);
	t->abs_rm_4 =		__C(CHIP,_mem_read_multi_4);
	t->abs_rm_8 =		__C(CHIP,_mem_read_multi_8);
	
	/* read region */
	t->abs_rr_1 =		__C(CHIP,_mem_read_region_1);
	t->abs_rr_2 =		__C(CHIP,_mem_read_region_2);
	t->abs_rr_4 =		__C(CHIP,_mem_read_region_4);
	t->abs_rr_8 =		__C(CHIP,_mem_read_region_8);
	
	/* write (single) */
	t->abs_w_1 =		__C(CHIP,_mem_write_1);
	t->abs_w_2 =		__C(CHIP,_mem_write_2);
	t->abs_w_4 =		__C(CHIP,_mem_write_4);
	t->abs_w_8 =		__C(CHIP,_mem_write_8);
	
	/* write multiple */
	t->abs_wm_1 =		__C(CHIP,_mem_write_multi_1);
	t->abs_wm_2 =		__C(CHIP,_mem_write_multi_2);
	t->abs_wm_4 =		__C(CHIP,_mem_write_multi_4);
	t->abs_wm_8 =		__C(CHIP,_mem_write_multi_8);
	
	/* write region */
	t->abs_wr_1 =		__C(CHIP,_mem_write_region_1);
	t->abs_wr_2 =		__C(CHIP,_mem_write_region_2);
	t->abs_wr_4 =		__C(CHIP,_mem_write_region_4);
	t->abs_wr_8 =		__C(CHIP,_mem_write_region_8);

	/* set multiple */
	t->abs_sm_1 =		__C(CHIP,_mem_set_multi_1);
	t->abs_sm_2 =		__C(CHIP,_mem_set_multi_2);
	t->abs_sm_4 =		__C(CHIP,_mem_set_multi_4);
	t->abs_sm_8 =		__C(CHIP,_mem_set_multi_8);
	
	/* set region */
	t->abs_sr_1 =		__C(CHIP,_mem_set_region_1);
	t->abs_sr_2 =		__C(CHIP,_mem_set_region_2);
	t->abs_sr_4 =		__C(CHIP,_mem_set_region_4);
	t->abs_sr_8 =		__C(CHIP,_mem_set_region_8);

	/* copy */
	t->abs_c_1 =		__C(CHIP,_mem_copy_region_1);
	t->abs_c_2 =		__C(CHIP,_mem_copy_region_2);
	t->abs_c_4 =		__C(CHIP,_mem_copy_region_4);
	t->abs_c_8 =		__C(CHIP,_mem_copy_region_8);

	ex = extent_create(__S(__C(CHIP,_bus_mem)), 0x0UL, 0xffffffffUL,
	    (void *)CHIP_MEM_EX_STORE(v), CHIP_MEM_EX_STORE_SIZE(v),
	    EX_NOWAIT|EX_NOCOALESCE);

	CHIP_MEM_EXTENT(v) = ex;
}

int
__C(CHIP,_mem_translate)(
	void *v,
	bus_addr_t memaddr,
	bus_size_t memlen,
	int flags,
	struct alpha_bus_space_translation *abst)
{

	/* XXX */
	return (EOPNOTSUPP);
}

int
__C(CHIP,_mem_get_window)(
	void *v,
	int window,
	struct alpha_bus_space_translation *abst)
{

	switch (window) {
	case 0:
		abst->abst_bus_start = 0;
		abst->abst_bus_end = 0xffffffffUL;
		abst->abst_sys_start = CHIP_MEM_SYS_START(v);
		abst->abst_sys_end = CHIP_MEM_SYS_START(v) + abst->abst_bus_end;
		abst->abst_addr_shift = 0;
		abst->abst_size_shift = 0;
		abst->abst_flags = ABST_DENSE|ABST_BWX;
		break;

	default:
		panic(__S(__C(CHIP,_mem_get_window)) ": invalid window %d",
		    window);
	}

	return (0);
}

int
__C(CHIP,_mem_map)(
	void *v,
	bus_addr_t memaddr,
	bus_size_t memsize,
	int flags,
	bus_space_handle_t *memhp,
	int acct)
{
	int error;

	if (acct == 0)
		goto mapit;

#ifdef EXTENT_DEBUG
	printf("mem: allocating 0x%lx to 0x%lx\n", memaddr,
	    memaddr + memsize - 1);
#endif
	error = extent_alloc_region(CHIP_MEM_EXTENT(v), memaddr, memsize,
	    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
	if (error) {
#ifdef EXTENT_DEBUG
		printf("mem: allocation failed (%d)\n", error);
		extent_print(CHIP_MEM_EXTENT(v));
#endif
		return (error);
	}

 mapit:
	*memhp = ALPHA_PHYS_TO_K0SEG(CHIP_MEM_SYS_START(v)) + memaddr;

	return (0);
}

void
__C(CHIP,_mem_unmap)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t memsize,
	int acct)
{
	bus_addr_t memaddr;
	int error;

	if (acct == 0)
		return;

#ifdef EXTENT_DEBUG
	printf("mem: freeing handle 0x%lx for 0x%lx\n", memh, memsize);
#endif

	memaddr = memh - ALPHA_PHYS_TO_K0SEG(CHIP_MEM_SYS_START(v));

#ifdef EXTENT_DEBUG
	printf("mem: freeing 0x%lx to 0x%lx\n", memaddr, memaddr + memsize - 1);
#endif

	error = extent_free(CHIP_MEM_EXTENT(v), memaddr, memsize,
	    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
	if (error) {
		printf("%s: WARNING: could not unmap 0x%lx-0x%lx (error %d)\n",
		    __S(__C(CHIP,_mem_unmap)), memaddr, memaddr + memsize - 1,
		    error);
#ifdef EXTENT_DEBUG
		extent_print(CHIP_MEM_EXTENT(v));
#endif
	}
}

int
__C(CHIP,_mem_subregion)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t offset,
	bus_size_t size,
	bus_space_handle_t *nmemh)
{

	*nmemh = memh + offset;
	return (0);
}

int
__C(CHIP,_mem_alloc)(
	void *v,
	bus_addr_t rstart,
	bus_addr_t rend,
	bus_size_t size,
	bus_size_t align,
	bus_size_t boundary,
	int flags,
	bus_addr_t *addrp,
	bus_space_handle_t *bshp)
{
	bus_addr_t memaddr;
	int error;

	/*
	 * Do the requested allocation.
	 */
#ifdef EXTENT_DEBUG
	printf("mem: allocating from 0x%lx to 0x%lx\n", rstart, rend);
#endif
	error = extent_alloc_subregion(CHIP_MEM_EXTENT(v), rstart, rend,
	    size, align, boundary,
	    EX_FAST | EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0),
	    &memaddr);
	if (error) {
#ifdef EXTENT_DEBUG
		printf("mem: allocation failed (%d)\n", error);
		extent_print(CHIP_MEM_EXTENT(v));
#endif
	}

#ifdef EXTENT_DEBUG
	printf("mem: allocated 0x%lx to 0x%lx\n", memaddr, memaddr + size - 1);
#endif

	*addrp = memaddr;
	*bshp = ALPHA_PHYS_TO_K0SEG(CHIP_MEM_SYS_START(v)) + memaddr;

	return (0);
}

void
__C(CHIP,_mem_free)(
	void *v,
	bus_space_handle_t bsh,
	bus_size_t size)
{

	/* Unmap does all we need to do. */
	__C(CHIP,_mem_unmap)(v, bsh, size, 1);
}

void *
__C(CHIP,_mem_vaddr)(
	void *v,
	bus_space_handle_t bsh)
{

	return ((void *)bsh);
}

paddr_t
__C(CHIP,_mem_mmap)(
	void *v,
	bus_addr_t addr,
	off_t off,
	int prot,
	int flags)
{

	return (alpha_btop(CHIP_MEM_SYS_START(v) + addr + off));
}

static inline void
__C(CHIP,_mem_barrier)(
	void *v,
	bus_space_handle_t h,
	bus_size_t o,
	bus_size_t l,
	int f)
{

	if ((f & BUS_SPACE_BARRIER_READ) != 0)
		alpha_mb();
	else if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		alpha_wmb();
}

static inline uint8_t
__C(CHIP,_mem_read_1)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off)
{
	bus_addr_t addr;

	addr = memh + off;
	alpha_mb();
	return (alpha_ldbu((uint8_t *)addr));
}

static inline uint16_t
__C(CHIP,_mem_read_2)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off)
{
	bus_addr_t addr;

	addr = memh + off;
#ifdef DIAGNOSTIC
	if (addr & 1)
		panic(__S(__C(CHIP,_mem_read_2)) ": addr 0x%lx not aligned",
		    addr);
#endif
	alpha_mb();
	return (alpha_ldwu((uint16_t *)addr));
}

static inline uint32_t
__C(CHIP,_mem_read_4)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off)
{
	bus_addr_t addr;

	addr = memh + off;
#ifdef DIAGNOSTIC
	if (addr & 3)
		panic(__S(__C(CHIP,_mem_read_4)) ": addr 0x%lx not aligned",
		    addr);
#endif
	alpha_mb();
	return (*(uint32_t *)addr);
}

static inline uint64_t
__C(CHIP,_mem_read_8)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off)
{

	alpha_mb();

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_read_8)));
}

#define CHIP_mem_read_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_read_multi_),BYTES)(					\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(CHIP,_mem_barrier)(v, h, o, sizeof *a,		\
		    BUS_SPACE_BARRIER_READ);				\
		*a++ = __C(__C(CHIP,_mem_read_),BYTES)(v, h, o);	\
	}								\
}
CHIP_mem_read_multi_N(1,uint8_t)
CHIP_mem_read_multi_N(2,uint16_t)
CHIP_mem_read_multi_N(4,uint32_t)
CHIP_mem_read_multi_N(8,uint64_t)

#define CHIP_mem_read_region_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_read_region_),BYTES)(					\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __C(__C(CHIP,_mem_read_),BYTES)(v, h, o);	\
		o += sizeof *a;						\
	}								\
}
CHIP_mem_read_region_N(1,uint8_t)
CHIP_mem_read_region_N(2,uint16_t)
CHIP_mem_read_region_N(4,uint32_t)
CHIP_mem_read_region_N(8,uint64_t)

static inline void
__C(CHIP,_mem_write_1)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off,
	uint8_t val)
{
	bus_addr_t addr;

	addr = memh + off;
	alpha_stb((uint8_t *)addr, val);
	alpha_mb();
}

static inline void
__C(CHIP,_mem_write_2)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off,
	uint16_t val)
{
	bus_addr_t addr;

	addr = memh + off;
#ifdef DIAGNOSTIC
	if (addr & 1)
		panic(__S(__C(CHIP,_mem_write_2)) ": addr 0x%lx not aligned",
		    addr);
#endif
	alpha_stw((uint16_t *)addr, val);
	alpha_mb();
}

static inline void
__C(CHIP,_mem_write_4)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off,
	uint32_t val)
{
	bus_addr_t addr;

	addr = memh + off;
#ifdef DIAGNOSTIC
	if (addr & 3)
		panic(__S(__C(CHIP,_mem_write_4)) ": addr 0x%lx not aligned",
		    addr);
#endif
	*(uint32_t *)addr = val;
	alpha_mb();
}

static inline void
__C(CHIP,_mem_write_8)(
	void *v,
	bus_space_handle_t memh,
	bus_size_t off,
	uint64_t val)
{

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_write_8)));
	alpha_mb();
}

#define CHIP_mem_write_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_write_multi_),BYTES)(					\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, *a++);	\
		__C(CHIP,_mem_barrier)(v, h, o, sizeof *a,		\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_mem_write_multi_N(1,uint8_t)
CHIP_mem_write_multi_N(2,uint16_t)
CHIP_mem_write_multi_N(4,uint32_t)
CHIP_mem_write_multi_N(8,uint64_t)

#define CHIP_mem_write_region_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_write_region_),BYTES)(				\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	const TYPE *a,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, *a++);	\
		o += sizeof *a;						\
	}								\
}
CHIP_mem_write_region_N(1,uint8_t)
CHIP_mem_write_region_N(2,uint16_t)
CHIP_mem_write_region_N(4,uint32_t)
CHIP_mem_write_region_N(8,uint64_t)

#define CHIP_mem_set_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_set_multi_),BYTES)(					\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE val,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, val);		\
		__C(CHIP,_mem_barrier)(v, h, o, sizeof val,		\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_mem_set_multi_N(1,uint8_t)
CHIP_mem_set_multi_N(2,uint16_t)
CHIP_mem_set_multi_N(4,uint32_t)
CHIP_mem_set_multi_N(8,uint64_t)

#define CHIP_mem_set_region_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_set_region_),BYTES)(					\
	void *v,							\
	bus_space_handle_t h,						\
	bus_size_t o,							\
	TYPE val,							\
	bus_size_t c)							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, val);		\
		o += sizeof val;					\
	}								\
}
CHIP_mem_set_region_N(1,uint8_t)
CHIP_mem_set_region_N(2,uint16_t)
CHIP_mem_set_region_N(4,uint32_t)
CHIP_mem_set_region_N(8,uint64_t)

#define	CHIP_mem_copy_region_N(BYTES)					\
void									\
__C(__C(CHIP,_mem_copy_region_),BYTES)(					\
	void *v,							\
	bus_space_handle_t h1,						\
	bus_size_t o1,							\
	bus_space_handle_t h2,						\
	bus_size_t o2,							\
	bus_size_t c)							\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES) {			\
			__C(__C(CHIP,_mem_write_),BYTES)(v, h2, o2 + o,	\
			    __C(__C(CHIP,_mem_read_),BYTES)(v, h1, o1 + o)); \
		}							\
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES) {	\
			__C(__C(CHIP,_mem_write_),BYTES)(v, h2, o2 + o,	\
			    __C(__C(CHIP,_mem_read_),BYTES)(v, h1, o1 + o)); \
		}							\
	}								\
}
CHIP_mem_copy_region_N(1)
CHIP_mem_copy_region_N(2)
CHIP_mem_copy_region_N(4)
CHIP_mem_copy_region_N(8)
