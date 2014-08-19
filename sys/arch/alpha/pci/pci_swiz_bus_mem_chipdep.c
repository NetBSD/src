/* $NetBSD: pci_swiz_bus_mem_chipdep.c,v 1.46.6.1 2014/08/20 00:02:41 tls Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 *	CHIP_D_MEM_W1_SYS_START	Dense Mem space base to use.
 *	CHIP_D_MEM_EX_STORE
 *			If defined, device-provided static storage area
 *			for the dense memory space extent.  If this is
 *			defined, CHIP_D_MEM_EX_STORE_SIZE must also be
 *			defined.  If this is not defined, a static area
 *			will be declared.
 *	CHIP_D_MEM_EX_STORE_SIZE
 *			Size of the device-provided static storage area
 *			for the dense memory space extent.
 *	CHIP_S_MEM_BASE	Sparse Mem space base to use.
 *	CHIP_S_MEM_EX_STORE
 *			If defined, device-provided static storage area
 *			for the sparse memory space extent.  If this is
 *			defined, CHIP_S_MEM_EX_STORE_SIZE must also be
 *			defined.  If this is not defined, a static area
 *			will be declared.
 *	CHIP_S_MEM_EX_STORE_SIZE
 *			Size of the device-provided static storage area
 *			for the sparse memory space extent.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: pci_swiz_bus_mem_chipdep.c,v 1.46.6.1 2014/08/20 00:02:41 tls Exp $");

#include <sys/extent.h>

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

#ifdef CHIP_D_MEM_W1_SYS_START
#ifndef	CHIP_D_MEM_EX_STORE
static long
    __C(CHIP,_dmem_ex_storage)[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
#define	CHIP_D_MEM_EX_STORE(v)		(__C(CHIP,_dmem_ex_storage))
#define	CHIP_D_MEM_EX_STORE_SIZE(v)	(sizeof __C(CHIP,_dmem_ex_storage))
#endif
#endif /* CHIP_D_MEM_W1_SYS_START */

#ifndef	CHIP_S_MEM_EX_STORE
static long
    __C(CHIP,_smem_ex_storage)[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
#define	CHIP_S_MEM_EX_STORE(v)		(__C(CHIP,_smem_ex_storage))
#define	CHIP_S_MEM_EX_STORE_SIZE(v)	(sizeof __C(CHIP,_smem_ex_storage))
#endif

#ifndef CHIP_ADDR_SHIFT
#define	CHIP_ADDR_SHIFT		5
#endif

#ifndef CHIP_SIZE_SHIFT
#define	CHIP_SIZE_SHIFT		3
#endif

void
__C(CHIP,_bus_mem_init)(bus_space_tag_t t, void *v)
{
#ifdef CHIP_D_MEM_W1_SYS_START
	struct extent *dex;
#endif
	struct extent *sex;

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

#ifdef CHIP_D_MEM_W1_SYS_START
	/* XXX WE WANT EXTENT_NOCOALESCE, BUT WE CAN'T USE IT. XXX */
	dex = extent_create(__S(__C(CHIP,_bus_dmem)), 0x0UL,
	    0xffffffffffffffffUL,
	    (void *)CHIP_D_MEM_EX_STORE(v), CHIP_D_MEM_EX_STORE_SIZE(v),
	    EX_NOWAIT);
	extent_alloc_region(dex, 0, 0xffffffffffffffffUL, EX_NOWAIT);

#ifdef CHIP_D_MEM_W1_BUS_START
#ifdef EXTENT_DEBUG
	printf("dmem: freeing from 0x%lx to 0x%lx\n",
	    CHIP_D_MEM_W1_BUS_START(v), CHIP_D_MEM_W1_BUS_END(v));
#endif
	extent_free(dex, CHIP_D_MEM_W1_BUS_START(v),
	    CHIP_D_MEM_W1_BUS_END(v) - CHIP_D_MEM_W1_BUS_START(v) + 1,
	    EX_NOWAIT);
#endif

#ifdef EXTENT_DEBUG
	extent_print(dex);
#endif
	CHIP_D_MEM_EXTENT(v) = dex;
#endif /* CHIP_D_MEM_W1_SYS_START */

	/* XXX WE WANT EXTENT_NOCOALESCE, BUT WE CAN'T USE IT. XXX */
	sex = extent_create(__S(__C(CHIP,_bus_smem)), 0x0UL,
	    0xffffffffffffffffUL,
	    (void *)CHIP_S_MEM_EX_STORE(v), CHIP_S_MEM_EX_STORE_SIZE(v),
	    EX_NOWAIT);
	extent_alloc_region(sex, 0, 0xffffffffffffffffUL, EX_NOWAIT);

#ifdef CHIP_S_MEM_W1_BUS_START
#ifdef EXTENT_DEBUG
	printf("smem: freeing from 0x%lx to 0x%lx\n",
	    CHIP_S_MEM_W1_BUS_START(v), CHIP_S_MEM_W1_BUS_END(v));
#endif
	extent_free(sex, CHIP_S_MEM_W1_BUS_START(v),
	    CHIP_S_MEM_W1_BUS_END(v) - CHIP_S_MEM_W1_BUS_START(v) + 1,
	    EX_NOWAIT);
#endif
#ifdef CHIP_S_MEM_W2_BUS_START
	if (CHIP_S_MEM_W2_BUS_START(v) != CHIP_S_MEM_W1_BUS_START(v)) {
#ifdef EXTENT_DEBUG
		printf("smem: freeing from 0x%lx to 0x%lx\n",
		    CHIP_S_MEM_W2_BUS_START(v), CHIP_S_MEM_W2_BUS_END(v));
#endif
		extent_free(sex, CHIP_S_MEM_W2_BUS_START(v),
		    CHIP_S_MEM_W2_BUS_END(v) - CHIP_S_MEM_W2_BUS_START(v) + 1,
		    EX_NOWAIT);
	} else {
#ifdef EXTENT_DEBUG
		printf("smem: window 2 (0x%lx to 0x%lx) overlaps window 1\n",
		    CHIP_S_MEM_W2_BUS_START(v), CHIP_S_MEM_W2_BUS_END(v));
#endif
	}
#endif
#ifdef CHIP_S_MEM_W3_BUS_START
	if (CHIP_S_MEM_W3_BUS_START(v) != CHIP_S_MEM_W1_BUS_START(v) &&
	    CHIP_S_MEM_W3_BUS_START(v) != CHIP_S_MEM_W2_BUS_START(v)) {
#ifdef EXTENT_DEBUG
		printf("smem: freeing from 0x%lx to 0x%lx\n",
		    CHIP_S_MEM_W3_BUS_START(v), CHIP_S_MEM_W3_BUS_END(v));
#endif
		extent_free(sex, CHIP_S_MEM_W3_BUS_START(v),
		    CHIP_S_MEM_W3_BUS_END(v) - CHIP_S_MEM_W3_BUS_START(v) + 1,
		    EX_NOWAIT);
	} else {
#ifdef EXTENT_DEBUG
		printf("smem: window 2 (0x%lx to 0x%lx) overlaps window 1\n",
		    CHIP_S_MEM_W2_BUS_START(v), CHIP_S_MEM_W2_BUS_END(v));
#endif
	}
#endif

#ifdef EXTENT_DEBUG
	extent_print(sex);
#endif
	CHIP_S_MEM_EXTENT(v) = sex;
}

#ifdef CHIP_D_MEM_W1_SYS_START
static int	__C(CHIP,_xlate_addr_to_dense_handle)(void *,
		    bus_addr_t, bus_space_handle_t *);
static int	__C(CHIP,_xlate_dense_handle_to_addr)(void *,
		    bus_space_handle_t, bus_addr_t *);
#endif /* CHIP_D_MEM_W1_SYS_START */
static int	__C(CHIP,_xlate_addr_to_sparse_handle)(void *,
		    bus_addr_t, bus_space_handle_t *);
static int	__C(CHIP,_xlate_sparse_handle_to_addr)(void *,
		    bus_space_handle_t, bus_addr_t *);

#ifdef CHIP_D_MEM_W1_SYS_START
static int
__C(CHIP,_xlate_addr_to_dense_handle)(void *v, bus_addr_t memaddr,
   bus_space_handle_t *memhp)
{
#ifdef CHIP_D_MEM_W1_BUS_START
	if (memaddr >= CHIP_D_MEM_W1_BUS_START(v) &&
	    memaddr <= CHIP_D_MEM_W1_BUS_END(v)) {
		if (memhp != NULL)
			*memhp =
			    ALPHA_PHYS_TO_K0SEG(CHIP_D_MEM_W1_SYS_START(v)) +
			    (memaddr - CHIP_D_MEM_W1_BUS_START(v));
		return (1);
	} else
#endif
		return (0);
}

static int
__C(CHIP,_xlate_dense_handle_to_addr)(void *v, bus_space_handle_t memh,
    bus_addr_t *memaddrp)
{

	memh = ALPHA_K0SEG_TO_PHYS(memh);

#ifdef CHIP_D_MEM_W1_BUS_START
	if (memh >= CHIP_D_MEM_W1_SYS_START(v) &&
	    memh <= CHIP_D_MEM_W1_SYS_END(v)) {
		*memaddrp = CHIP_D_MEM_W1_BUS_START(v) +
		    (memh - CHIP_D_MEM_W1_SYS_START(v));
		return (1);
	} else
#endif
		return (0);
}
#endif /* CHIP_D_MEM_W1_SYS_START */

static int
__C(CHIP,_xlate_addr_to_sparse_handle)(void *v, bus_addr_t memaddr,
    bus_space_handle_t *memhp)
{

#ifdef CHIP_S_MEM_W1_BUS_START
	if (memaddr >= CHIP_S_MEM_W1_BUS_START(v) &&
	    memaddr <= CHIP_S_MEM_W1_BUS_END(v)) {
		if (memhp != NULL)
			*memhp =
			    (ALPHA_PHYS_TO_K0SEG(CHIP_S_MEM_W1_SYS_START(v)) >>
			     CHIP_ADDR_SHIFT) +
			    (memaddr - CHIP_S_MEM_W1_BUS_START(v));
		return (1);
	} else
#endif
#ifdef CHIP_S_MEM_W2_BUS_START
	if (memaddr >= CHIP_S_MEM_W2_BUS_START(v) &&
	    memaddr <= CHIP_S_MEM_W2_BUS_END(v)) {
		if (memhp != NULL)
			*memhp =
			    (ALPHA_PHYS_TO_K0SEG(CHIP_S_MEM_W2_SYS_START(v)) >>
			     CHIP_ADDR_SHIFT) +
			    (memaddr - CHIP_S_MEM_W2_BUS_START(v));
		return (1);
	} else
#endif
#ifdef CHIP_S_MEM_W3_BUS_START
	if (memaddr >= CHIP_S_MEM_W3_BUS_START(v) &&
	    memaddr <= CHIP_S_MEM_W3_BUS_END(v)) {
		if (memhp != NULL)
			*memhp =
			    (ALPHA_PHYS_TO_K0SEG(CHIP_S_MEM_W3_SYS_START(v)) >>
			     CHIP_ADDR_SHIFT) +
			    (memaddr - CHIP_S_MEM_W3_BUS_START(v));
		return (1);
	} else
#endif
		return (0);
}

static int
__C(CHIP,_xlate_sparse_handle_to_addr)(void *v, bus_space_handle_t memh,
    bus_addr_t *memaddrp)
{

	memh = ALPHA_K0SEG_TO_PHYS(memh << CHIP_ADDR_SHIFT) >> CHIP_ADDR_SHIFT;

#ifdef CHIP_S_MEM_W1_BUS_START
	if ((memh << CHIP_ADDR_SHIFT) >= CHIP_S_MEM_W1_SYS_START(v) &&
	    (memh << CHIP_ADDR_SHIFT) <= CHIP_S_MEM_W1_SYS_END(v)) {
		*memaddrp = CHIP_S_MEM_W1_BUS_START(v) +
		    (memh - (CHIP_S_MEM_W1_SYS_START(v) >> CHIP_ADDR_SHIFT));
		return (1);
	} else
#endif
#ifdef CHIP_S_MEM_W2_BUS_START
	if ((memh << CHIP_ADDR_SHIFT) >= CHIP_S_MEM_W2_SYS_START(v) &&
	    (memh << CHIP_ADDR_SHIFT) <= CHIP_S_MEM_W2_SYS_END(v)) {
		*memaddrp = CHIP_S_MEM_W2_BUS_START(v) +
		    (memh - (CHIP_S_MEM_W2_SYS_START(v) >> CHIP_ADDR_SHIFT));
		return (1);
	} else
#endif
#ifdef CHIP_S_MEM_W3_BUS_START
	if ((memh << CHIP_ADDR_SHIFT) >= CHIP_S_MEM_W3_SYS_START(v) &&
	    (memh << CHIP_ADDR_SHIFT) <= CHIP_S_MEM_W3_SYS_END(v)) {
		*memaddrp = CHIP_S_MEM_W3_BUS_START(v) +
		    (memh - (CHIP_S_MEM_W3_SYS_START(v) >> CHIP_ADDR_SHIFT));
		return (1);
	} else
#endif
		return (0);
}

int
__C(CHIP,_mem_translate)(void *v, bus_addr_t memaddr, bus_size_t memlen,
    int flags, struct alpha_bus_space_translation *abst)
{

	/* XXX */
	return (EOPNOTSUPP);
}

int
__C(CHIP,_mem_get_window)(void *v, int window,
    struct alpha_bus_space_translation *abst)
{

#ifdef CHIP_D_MEM_W1_BUS_START
#define	FIRST_SPARSE	1
#else
#define	FIRST_SPARSE	0
#endif

	switch (window) {
#ifdef CHIP_D_MEM_W1_BUS_START
	case 0:
		abst->abst_bus_start = CHIP_D_MEM_W1_BUS_START(v);
		abst->abst_bus_end = CHIP_D_MEM_W1_BUS_END(v);
		abst->abst_sys_start = CHIP_D_MEM_W1_SYS_START(v);
		abst->abst_sys_end = CHIP_D_MEM_W1_SYS_END(v);
		abst->abst_addr_shift = 0;
		abst->abst_size_shift = 0;
		abst->abst_flags = ABST_DENSE;
		break;
#endif

#ifdef CHIP_S_MEM_W1_BUS_START
	case (FIRST_SPARSE):
		abst->abst_bus_start = CHIP_S_MEM_W1_BUS_START(v);
		abst->abst_bus_end = CHIP_S_MEM_W1_BUS_END(v);
		abst->abst_sys_start = CHIP_S_MEM_W1_SYS_START(v);
		abst->abst_sys_end = CHIP_S_MEM_W1_SYS_END(v);
		abst->abst_addr_shift = CHIP_ADDR_SHIFT;
		abst->abst_size_shift = CHIP_SIZE_SHIFT;
		abst->abst_flags = 0;
		break;
#endif

#ifdef CHIP_S_MEM_W2_BUS_START
	case (FIRST_SPARSE + 1):
		abst->abst_bus_start = CHIP_S_MEM_W2_BUS_START(v);
		abst->abst_bus_end = CHIP_S_MEM_W2_BUS_END(v);
		abst->abst_sys_start = CHIP_S_MEM_W2_SYS_START(v);
		abst->abst_sys_end = CHIP_S_MEM_W2_SYS_END(v);
		abst->abst_addr_shift = CHIP_ADDR_SHIFT;
		abst->abst_size_shift = CHIP_SIZE_SHIFT;
		abst->abst_flags = 0;
		break;
#endif

#ifdef CHIP_S_MEM_W3_BUS_START
	case (FIRST_SPARSE + 2):
		abst->abst_bus_start = CHIP_S_MEM_W3_BUS_START(v);
		abst->abst_bus_end = CHIP_S_MEM_W3_BUS_END(v);
		abst->abst_sys_start = CHIP_S_MEM_W3_SYS_START(v);
		abst->abst_sys_end = CHIP_S_MEM_W3_SYS_END(v);
		abst->abst_addr_shift = CHIP_ADDR_SHIFT;
		abst->abst_size_shift = CHIP_SIZE_SHIFT;
		abst->abst_flags = 0;
		break;
#endif
	}

#undef FIRST_SPARSE

	return (0);
}

int
__C(CHIP,_mem_map)(void *v, bus_addr_t memaddr, bus_size_t memsize,
    int flags, bus_space_handle_t *memhp, int acct)
{
	bus_space_handle_t dh = 0, sh = 0;	/* XXX -Wuninitialized */
	int didd, dids, errord, errors, mustd, musts;
	int prefectchable = flags & BUS_SPACE_MAP_PREFETCHABLE;
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/*
	 * XXX Too hairy to not do accounting in this space.  Nothing
	 * XXX much uses this anyhow (only ISA PnP does, and only via
	 * XXX a machine-dependent hook), so we don't have to care.
	 */
	if (acct == 0)
		return (EOPNOTSUPP);

#ifdef CHIP_D_MEM_W1_SYS_START
	mustd = 1;
	if (!__C(CHIP,_xlate_addr_to_dense_handle)(v, memaddr, NULL)) {
		/*
		 * This address isn't mapped into dense space; don't
		 * require it.
		 */
		mustd = 0;
	}
#else
	mustd = 0;
#endif

	/* No prefectchable space without dense. */
	if (mustd == 0)
		prefectchable = 0;

	/*
	 * We must have dense space to map memory linearly.
	 */
	if (linear && !prefectchable)
		return (EOPNOTSUPP);

	musts = (prefectchable == 0);
	if (!__C(CHIP,_xlate_addr_to_sparse_handle)(v, memaddr, NULL)) {
		/*
		 * This address isn't mapped into sparse space; don't
		 * require it.
		 */
		musts = 0;
	}

	/*
	 * If this address isn't mapped into dense or sparse, we lose.
	 */
	if (mustd == 0 && musts == 0) {
#ifdef EXTENT_DEBUG
		printf("mem: address 0x%lx not in dense or sparse space\n",
		    memaddr);
#endif
		return (EINVAL);
	}

#ifdef EXTENT_DEBUG
	printf("mem: allocating 0x%lx to 0x%lx\n", memaddr,
	    memaddr + memsize - 1);
	printf("mem: %s dense, %s sparse\n", mustd ? "need" : "want",
	    musts ? "need" : "want");
#endif
#ifdef CHIP_D_MEM_W1_SYS_START
	errord = extent_alloc_region(CHIP_D_MEM_EXTENT(v), memaddr, memsize,
	    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
#else
	errord = EINVAL;
#endif
	didd = (errord == 0);
	errors = extent_alloc_region(CHIP_S_MEM_EXTENT(v), memaddr, memsize,
	    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
	dids = (errors == 0);

#ifdef EXTENT_DEBUG
	if (!didd)
		printf("mem: failed to get dense (%d)\n", errord);
	if (!dids)
		printf("mem: failed to get sparse (%d)\n", errors);
#endif

	if ((mustd && !didd) || (musts && !dids))
		goto bad;

#ifdef CHIP_D_MEM_W1_SYS_START
	if (didd && !__C(CHIP,_xlate_addr_to_dense_handle)(v, memaddr, &dh)) {
		printf("\n");
#ifdef CHIP_D_MEM_W1_BUS_START
		printf("%s: window[1]=0x%lx-0x%lx\n", __S(__C(CHIP,_mem_map)),
		    CHIP_D_MEM_W1_BUS_START(v), CHIP_D_MEM_W1_BUS_END(v));
#endif
		panic("%s: don't know how to map %lx prefectchable",
		    __S(__C(CHIP,_mem_map)), memaddr);
	}
#endif /* CHIP_D_MEM_W1_SYS_START */

	if (dids && !__C(CHIP,_xlate_addr_to_sparse_handle)(v, memaddr, &sh)) {
		printf("\n");
#ifdef CHIP_S_MEM_W1_BUS_START
		printf("%s: window[1]=0x%lx-0x%lx\n", __S(__C(CHIP,_mem_map)),
		    CHIP_S_MEM_W1_BUS_START(v), CHIP_S_MEM_W1_BUS_END(v));
#endif
#ifdef CHIP_S_MEM_W2_BUS_START
		printf("%s: window[2]=0x%lx-0x%lx\n", __S(__C(CHIP,_mem_map)),
		    CHIP_S_MEM_W2_BUS_START(v), CHIP_S_MEM_W2_BUS_END(v));
#endif
#ifdef CHIP_S_MEM_W3_BUS_START
		printf("%s: window[3]=0x%lx-0x%lx\n", __S(__C(CHIP,_mem_map)),
		    CHIP_S_MEM_W3_BUS_START(v), CHIP_S_MEM_W3_BUS_END(v));
#endif
		panic("%s: don't know how to map %lx non-prefectchable",
		    __S(__C(CHIP,_mem_map)), memaddr);
	}

	if (prefectchable)
		*memhp = dh;
	else
		*memhp = sh;
	return (0);

bad:
#ifdef EXTENT_DEBUG
	printf("mem: failed\n");
#endif
#ifdef CHIP_D_MEM_W1_SYS_START
	if (didd) {
#ifdef EXTENT_DEBUG
	printf("mem: freeing dense\n");
#endif
		if (extent_free(CHIP_D_MEM_EXTENT(v), memaddr, memsize,
		    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0)) != 0) {
			printf("%s: WARNING: couldn't free dense 0x%lx-0x%lx\n",
			    __S(__C(CHIP,_mem_map)), memaddr,
			    memaddr + memsize - 1);
		}
	}
#endif /* CHIP_D_MEM_W1_SYS_START */
	if (dids) {
#ifdef EXTENT_DEBUG
	printf("mem: freeing sparse\n");
#endif
		if (extent_free(CHIP_S_MEM_EXTENT(v), memaddr, memsize,
		    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0)) != 0) {
			printf("%s: WARNING: couldn't free sparse 0x%lx-0x%lx\n",
			    __S(__C(CHIP,_mem_map)), memaddr,
			    memaddr + memsize - 1);
		}
	}

#ifdef EXTENT_DEBUG
#ifdef CHIP_D_MEM_W1_SYS_START
	extent_print(CHIP_D_MEM_EXTENT(v));
#endif
	extent_print(CHIP_S_MEM_EXTENT(v));
#endif

	/*
	 * return dense error if we needed it but couldn't get it, else
	 * sparse error.  The error _has_ to be one of the two...
	 */
	return (mustd && !didd ? errord : (musts && !dids ? errors : EINVAL));
}

void
__C(CHIP,_mem_unmap)(void *v, bus_space_handle_t memh,
    bus_size_t memsize, int acct)
{
	bus_addr_t memaddr = 0;
#ifdef CHIP_D_MEM_W1_SYS_START
	bus_space_handle_t temph;
#endif
	int sparse, haves, haved;

	if (acct == 0)
		return;

#ifdef EXTENT_DEBUG
	printf("mem: freeing handle 0x%lx for 0x%lx\n", memh, memsize);
#endif

	/*
	 * Find out what space we're in.
	 */
	sparse = ((memh >> 63) == 0);

	/*
	 * Find out what address we're in in that space.
	 */
	haves = haved = 0;
	if (sparse)
		haves = __C(CHIP,_xlate_sparse_handle_to_addr)(v, memh,
		    &memaddr);
#ifdef CHIP_D_MEM_W1_SYS_START
	else
		haved = __C(CHIP,_xlate_dense_handle_to_addr)(v, memh,
		    &memaddr);
#endif /* CHIP_D_MEM_W1_SYS_START */

	if (!haves && !haved)
		panic("%s: couldn't get addr from %s handle 0x%lx",
		    __S(__C(CHIP,_mem_unmap)), sparse ? "sparse" : "dense",
		    memh);

#ifdef CHIP_D_MEM_W1_SYS_START
	/*
	 * Find out were/if that address lives in the other space.
	 */
	if (sparse)
		haved = __C(CHIP,_xlate_addr_to_dense_handle)(v, memaddr,
		    &temph);
	else
		haves = __C(CHIP,_xlate_addr_to_sparse_handle)(v, memaddr,
		    &temph);
#endif /* CHIP_D_MEM_W1_SYS_START */

	/*
	 * Free any ranges we have.
	 */
#ifdef EXTENT_DEBUG
	printf("mem: it's at 0x%lx (%sdense, %ssparse)\n", memaddr,
	    haved ? "" : "not ", haves ? "" : "not ");
#endif
#ifdef CHIP_D_MEM_W1_SYS_START
	if (haved && extent_free(CHIP_D_MEM_EXTENT(v), memaddr, memsize,
	    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0)) != 0) {
		printf("%s: WARNING: couldn't free dense 0x%lx-0x%lx\n",
		    __S(__C(CHIP,_mem_map)), memaddr,
		    memaddr + memsize - 1);
	}
#endif
	if (haves && extent_free(CHIP_S_MEM_EXTENT(v), memaddr, memsize,
	    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0)) != 0) {
		printf("%s: WARNING: couldn't free sparse 0x%lx-0x%lx\n",
		    __S(__C(CHIP,_mem_map)), memaddr,
		    memaddr + memsize - 1);
	}
}

int
__C(CHIP,_mem_subregion)(void *v, bus_space_handle_t memh,
   bus_size_t offset, bus_size_t size, bus_space_handle_t *nmemh)
{

	*nmemh = memh + offset;
	return (0);
}

int
__C(CHIP,_mem_alloc)(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t align, bus_size_t boundary, int flags,
    bus_addr_t *addrp, bus_space_handle_t *bshp)
{

	/* XXX XXX XXX XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_alloc)));
}

void
__C(CHIP,_mem_free)(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	/* XXX XXX XXX XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_free)));
}

void *
__C(CHIP,_mem_vaddr)(void *v, bus_space_handle_t bsh)
{
#ifdef CHIP_D_MEM_W1_SYS_START
	/*
	 * XXX should check that the range was mapped
	 * with BUS_SPACE_MAP_LINEAR for sanity
	 */
	if ((bsh >> 63) != 0)
		return ((void *)bsh);
#endif
	return (0);
}

paddr_t
__C(CHIP,_mem_mmap)(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{
	bus_space_handle_t dh = 0, sh = 0;	/* XXX -Wuninitialized */
	int linear = flags & BUS_SPACE_MAP_LINEAR;
	int haved = 0, haves = 0;

#ifdef CHIP_D_MEM_W1_SYS_START
	if (__C(CHIP,_xlate_addr_to_dense_handle)(v, addr + off, &dh)) {
		haved = 1;
		dh = ALPHA_K0SEG_TO_PHYS(dh);
	}
#endif
	if (__C(CHIP,_xlate_addr_to_sparse_handle)(v, addr + off, &sh)) {
		haves = 1;
		sh = ALPHA_K0SEG_TO_PHYS(sh);
	}

	if (linear) {
		if (haved == 0)
			return (-1);
		return (alpha_btop(dh));
	}

	if (haves == 0)
		return (-1);
	return (alpha_btop(sh));
}

static inline void
__C(CHIP,_mem_barrier)(void *v, bus_space_handle_t h,
    bus_size_t o, bus_size_t l, int f)
{

	if ((f & BUS_SPACE_BARRIER_READ) != 0)
		alpha_mb();
	else if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		alpha_wmb();
}

static inline uint8_t
__C(CHIP,_mem_read_1)(void *v, bus_space_handle_t memh, bus_size_t off)
{
	register bus_space_handle_t tmpmemh;
	register uint32_t *port, val;
	register uint8_t rval;
	register int offset;

	alpha_mb();

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		return (*(uint8_t *)(memh + off));
#endif

	tmpmemh = memh + off;
	offset = tmpmemh & 3;
	port = (uint32_t *)((tmpmemh << CHIP_ADDR_SHIFT) |
	    (0 << CHIP_SIZE_SHIFT));
	val = *port;
	rval = ((val) >> (8 * offset)) & 0xff;

	return rval;
}

static inline uint16_t
__C(CHIP,_mem_read_2)(void *v, bus_space_handle_t memh, bus_size_t off)
{
	register bus_space_handle_t tmpmemh;
	register uint32_t *port, val;
	register uint16_t rval;
	register int offset;

	alpha_mb();

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		return (*(uint16_t *)(memh + off));
#endif

	tmpmemh = memh + off;
	offset = tmpmemh & 3;
	port = (uint32_t *)((tmpmemh << CHIP_ADDR_SHIFT) |
	    (1 << CHIP_SIZE_SHIFT));
	val = *port;
	rval = ((val) >> (8 * offset)) & 0xffff;

	return rval;
}

static inline uint32_t
__C(CHIP,_mem_read_4)(void *v, bus_space_handle_t memh, bus_size_t off)
{
	register bus_space_handle_t tmpmemh;
	register uint32_t *port, val;
	register uint32_t rval;

	alpha_mb();

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		return (*(uint32_t *)(memh + off));
#endif

	tmpmemh = memh + off;
	port = (uint32_t *)((tmpmemh << CHIP_ADDR_SHIFT) |
	    (3 << CHIP_SIZE_SHIFT));
	val = *port;
#if 0
	int offset;
	offset = tmpmemh & 3;
	rval = ((val) >> (8 * offset)) & 0xffffffff;
#else
	rval = val;
#endif

	return rval;
}

static inline uint64_t
__C(CHIP,_mem_read_8)(void *v, bus_space_handle_t memh, bus_size_t off)
{

	alpha_mb();

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
	        return (*(uint64_t *)(memh + off));
#endif

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_read_8)));
}

#define CHIP_mem_read_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_read_multi_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE *a, bus_size_t c)			\
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
__C(__C(CHIP,_mem_read_region_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE *a, bus_size_t c)			\
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
__C(CHIP,_mem_write_1)(void *v, bus_space_handle_t memh, bus_size_t off, uint8_t val)
{
	register bus_space_handle_t tmpmemh;
	register uint32_t *port, nval;
	register int offset;

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		(*(uint8_t *)(memh + off)) = val;
	else
#endif
	{
		tmpmemh = memh + off;
		offset = tmpmemh & 3;
		nval = val << (8 * offset);
		port = (uint32_t *)((tmpmemh << CHIP_ADDR_SHIFT) |
		    (0 << CHIP_SIZE_SHIFT));
		*port = nval;
	}
	alpha_mb();
}

static inline void
__C(CHIP,_mem_write_2)(void *v, bus_space_handle_t memh, bus_size_t off, uint16_t val)
{
	register bus_space_handle_t tmpmemh;
	register uint32_t *port, nval;
	register int offset;

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		(*(uint16_t *)(memh + off)) = val;
	else
#endif
	{
		tmpmemh = memh + off;
		offset = tmpmemh & 3;
	        nval = val << (8 * offset);
	        port = (uint32_t *)((tmpmemh << CHIP_ADDR_SHIFT) |
	            (1 << CHIP_SIZE_SHIFT));
	        *port = nval;
	}
	alpha_mb();
}

static inline void
__C(CHIP,_mem_write_4)(void *v, bus_space_handle_t memh, bus_size_t off, uint32_t val)
{
	register bus_space_handle_t tmpmemh;
	register uint32_t *port, nval;

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		(*(uint32_t *)(memh + off)) = val;
	else
#endif
	{
		tmpmemh = memh + off;
#if 0
		int offset;
		offset = tmpmemh & 3;
#endif
	        nval = val /*<< (8 * offset)*/;
	        port = (uint32_t *)((tmpmemh << CHIP_ADDR_SHIFT) |
	            (3 << CHIP_SIZE_SHIFT));
	        *port = nval;
	}
	alpha_mb();
}

static inline void
__C(CHIP,_mem_write_8)(void *v, bus_space_handle_t memh, bus_size_t off, uint64_t val)
{

#ifdef CHIP_D_MEM_W1_SYS_START
	if ((memh >> 63) != 0)
		(*(uint64_t *)(memh + off)) = val;
	else
#endif
	{
		/* XXX XXX XXX */
		panic("%s not implemented",
		    __S(__C(CHIP,_mem_write_8)));
	}
	alpha_mb();
}

#define CHIP_mem_write_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_write_multi_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, const TYPE *a, bus_size_t c)			\
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
__C(__C(CHIP,_mem_write_region_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, const TYPE *a, bus_size_t c)			\
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
__C(__C(CHIP,_mem_set_multi_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE val, bus_size_t c)			\
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
__C(__C(CHIP,_mem_set_region_),BYTES)(void *v, bus_space_handle_t h, bus_size_t o, TYPE val, bus_size_t c)			\
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
__C(__C(CHIP,_mem_copy_region_),BYTES)(void *v, bus_space_handle_t h1, bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)		\
{									\
	bus_size_t o;							\
									\
	if ((h1 >> 63) != 0 && (h2 >> 63) != 0) {			\
		memmove((void *)(h2 + o2), (void *)(h1 + o1), c * BYTES); \
		return;							\
	}								\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__C(__C(CHIP,_mem_write_),BYTES)(v, h2, o2 + o,	\
			    __C(__C(CHIP,_mem_read_),BYTES)(v, h1, o1 + o)); \
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__C(__C(CHIP,_mem_write_),BYTES)(v, h2, o2 + o,	\
			    __C(__C(CHIP,_mem_read_),BYTES)(v, h1, o1 + o)); \
	}								\
}
CHIP_mem_copy_region_N(1)
CHIP_mem_copy_region_N(2)
CHIP_mem_copy_region_N(4)
CHIP_mem_copy_region_N(8)
