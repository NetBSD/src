/*	$NetBSD: pci_alignstride_bus_mem_chipdep.c,v 1.1 2001/05/28 16:22:21 thorpej Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
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
 *			for the sparse memory space extent.  If this is
 *			defined, CHIP_MEM_EX_STORE_SIZE must also be
 *			defined.  If this is not defined, a static area
 *			will be declared.
 *	CHIP_MEM_EX_STORE_SIZE
 *			Size of the device-provided static storage area
 *			for the sparse memory space extent.
 */

#include <sys/extent.h>

#define	__C(A,B)	__CONCAT(A,B)
#define	__S(S)		__STRING(S)

/* mapping/unmapping */
int		__C(CHIP,_mem_map) __P((void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int));
void		__C(CHIP,_mem_unmap) __P((void *, bus_space_handle_t,
		    bus_size_t, int));
int		__C(CHIP,_mem_subregion) __P((void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, bus_space_handle_t *));

int		__C(CHIP,_mem_translate) __P((void *, bus_addr_t, bus_size_t,
		    int, struct mips_bus_space_translation *));
int		__C(CHIP,_mem_get_window) __P((void *, int,
		    struct mips_bus_space_translation *));

/* allocation/deallocation */
int		__C(CHIP,_mem_alloc) __P((void *, bus_addr_t, bus_addr_t,
		    bus_size_t, bus_size_t, bus_addr_t, int, bus_addr_t *,
                    bus_space_handle_t *));
void		__C(CHIP,_mem_free) __P((void *, bus_space_handle_t,
		    bus_size_t));

/* get kernel virtual address */
void *		__C(CHIP,_mem_vaddr) __P((void *, bus_space_handle_t));

/* barrier */
inline void	__C(CHIP,_mem_barrier) __P((void *, bus_space_handle_t,
		    bus_size_t, bus_size_t, int));

/* read (single) */
inline u_int8_t	__C(CHIP,_mem_read_1) __P((void *, bus_space_handle_t,
		    bus_size_t));
inline u_int16_t __C(CHIP,_mem_read_2) __P((void *, bus_space_handle_t,
		    bus_size_t));
inline u_int32_t __C(CHIP,_mem_read_4) __P((void *, bus_space_handle_t,
		    bus_size_t));
inline u_int64_t __C(CHIP,_mem_read_8) __P((void *, bus_space_handle_t,
		    bus_size_t));

/* read multiple */
void		__C(CHIP,_mem_read_multi_1) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int8_t *, bus_size_t));
void		__C(CHIP,_mem_read_multi_2) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int16_t *, bus_size_t));
void		__C(CHIP,_mem_read_multi_4) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int32_t *, bus_size_t));
void		__C(CHIP,_mem_read_multi_8) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int64_t *, bus_size_t));

/* read region */
void		__C(CHIP,_mem_read_region_1) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int8_t *, bus_size_t));
void		__C(CHIP,_mem_read_region_2) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int16_t *, bus_size_t));
void		__C(CHIP,_mem_read_region_4) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int32_t *, bus_size_t));
void		__C(CHIP,_mem_read_region_8) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int64_t *, bus_size_t));

/* write (single) */
inline void	__C(CHIP,_mem_write_1) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int8_t));
inline void	__C(CHIP,_mem_write_2) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int16_t));
inline void	__C(CHIP,_mem_write_4) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int32_t));
inline void	__C(CHIP,_mem_write_8) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int64_t));

/* write multiple */
void		__C(CHIP,_mem_write_multi_1) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int8_t *, bus_size_t));
void		__C(CHIP,_mem_write_multi_2) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int16_t *, bus_size_t));
void		__C(CHIP,_mem_write_multi_4) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int32_t *, bus_size_t));
void		__C(CHIP,_mem_write_multi_8) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int64_t *, bus_size_t));

/* write region */
void		__C(CHIP,_mem_write_region_1) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int8_t *, bus_size_t));
void		__C(CHIP,_mem_write_region_2) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int16_t *, bus_size_t));
void		__C(CHIP,_mem_write_region_4) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int32_t *, bus_size_t));
void		__C(CHIP,_mem_write_region_8) __P((void *, bus_space_handle_t,
		    bus_size_t, const u_int64_t *, bus_size_t));

/* set multiple */
void		__C(CHIP,_mem_set_multi_1) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int8_t, bus_size_t));
void		__C(CHIP,_mem_set_multi_2) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int16_t, bus_size_t));
void		__C(CHIP,_mem_set_multi_4) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int32_t, bus_size_t));
void		__C(CHIP,_mem_set_multi_8) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int64_t, bus_size_t));

/* set region */
void		__C(CHIP,_mem_set_region_1) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int8_t, bus_size_t));
void		__C(CHIP,_mem_set_region_2) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int16_t, bus_size_t));
void		__C(CHIP,_mem_set_region_4) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int32_t, bus_size_t));
void		__C(CHIP,_mem_set_region_8) __P((void *, bus_space_handle_t,
		    bus_size_t, u_int64_t, bus_size_t));

/* copy */
void		__C(CHIP,_mem_copy_region_1) __P((void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));
void		__C(CHIP,_mem_copy_region_2) __P((void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));
void		__C(CHIP,_mem_copy_region_4) __P((void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));
void		__C(CHIP,_mem_copy_region_8) __P((void *, bus_space_handle_t,
		    bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));

#ifndef	CHIP_MEM_EX_STORE
static long
    __C(CHIP,_mem_ex_storage)[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
#define	CHIP_MEM_EX_STORE(v)		(__C(CHIP,_mem_ex_storage))
#define	CHIP_MEM_EX_STORE_SIZE(v)	(sizeof __C(CHIP,_mem_ex_storage))
#endif

#ifndef CHIP_ALIGN_STRIDE
#define	CHIP_ALIGN_STRIDE	0
#endif

void
__C(CHIP,_bus_mem_init)(t, v)
	bus_space_tag_t t;
	void *v;
{
	struct extent *ex;

	/*
	 * Initialize the bus space tag.
	 */

	/* cookie */
	t->bs_cookie =		v;

	/* mapping/unmapping */
	t->bs_map =		__C(CHIP,_mem_map);
	t->bs_unmap =		__C(CHIP,_mem_unmap);
	t->bs_subregion =	__C(CHIP,_mem_subregion);

	t->bs_translate =	__C(CHIP,_mem_translate);
	t->bs_get_window =	__C(CHIP,_mem_get_window);

	/* allocation/deallocation */
	t->bs_alloc =		__C(CHIP,_mem_alloc);
	t->bs_free = 		__C(CHIP,_mem_free);

	/* get kernel virtual address */
	t->bs_vaddr =		__C(CHIP,_mem_vaddr);

	/* barrier */
	t->bs_barrier =	__C(CHIP,_mem_barrier);
	
	/* read (single) */
	t->bs_r_1 =		__C(CHIP,_mem_read_1);
	t->bs_r_2 =		__C(CHIP,_mem_read_2);
	t->bs_r_4 =		__C(CHIP,_mem_read_4);
	t->bs_r_8 =		__C(CHIP,_mem_read_8);
	
	/* read multiple */
	t->bs_rm_1 =		__C(CHIP,_mem_read_multi_1);
	t->bs_rm_2 =		__C(CHIP,_mem_read_multi_2);
	t->bs_rm_4 =		__C(CHIP,_mem_read_multi_4);
	t->bs_rm_8 =		__C(CHIP,_mem_read_multi_8);
	
	/* read region */
	t->bs_rr_1 =		__C(CHIP,_mem_read_region_1);
	t->bs_rr_2 =		__C(CHIP,_mem_read_region_2);
	t->bs_rr_4 =		__C(CHIP,_mem_read_region_4);
	t->bs_rr_8 =		__C(CHIP,_mem_read_region_8);
	
	/* write (single) */
	t->bs_w_1 =		__C(CHIP,_mem_write_1);
	t->bs_w_2 =		__C(CHIP,_mem_write_2);
	t->bs_w_4 =		__C(CHIP,_mem_write_4);
	t->bs_w_8 =		__C(CHIP,_mem_write_8);
	
	/* write multiple */
	t->bs_wm_1 =		__C(CHIP,_mem_write_multi_1);
	t->bs_wm_2 =		__C(CHIP,_mem_write_multi_2);
	t->bs_wm_4 =		__C(CHIP,_mem_write_multi_4);
	t->bs_wm_8 =		__C(CHIP,_mem_write_multi_8);
	
	/* write region */
	t->bs_wr_1 =		__C(CHIP,_mem_write_region_1);
	t->bs_wr_2 =		__C(CHIP,_mem_write_region_2);
	t->bs_wr_4 =		__C(CHIP,_mem_write_region_4);
	t->bs_wr_8 =		__C(CHIP,_mem_write_region_8);

	/* set multiple */
	t->bs_sm_1 =		__C(CHIP,_mem_set_multi_1);
	t->bs_sm_2 =		__C(CHIP,_mem_set_multi_2);
	t->bs_sm_4 =		__C(CHIP,_mem_set_multi_4);
	t->bs_sm_8 =		__C(CHIP,_mem_set_multi_8);
	
	/* set region */
	t->bs_sr_1 =		__C(CHIP,_mem_set_region_1);
	t->bs_sr_2 =		__C(CHIP,_mem_set_region_2);
	t->bs_sr_4 =		__C(CHIP,_mem_set_region_4);
	t->bs_sr_8 =		__C(CHIP,_mem_set_region_8);

	/* copy */
	t->bs_c_1 =		__C(CHIP,_mem_copy_region_1);
	t->bs_c_2 =		__C(CHIP,_mem_copy_region_2);
	t->bs_c_4 =		__C(CHIP,_mem_copy_region_4);
	t->bs_c_8 =		__C(CHIP,_mem_copy_region_8);

	/* XXX WE WANT EXTENT_NOCOALESCE, BUT WE CAN'T USE IT. XXX */
	ex = extent_create(__S(__C(CHIP,_bus_mem)), 0x0UL, 0xffffffffUL,
	    M_DEVBUF, (caddr_t)CHIP_MEM_EX_STORE(v), CHIP_MEM_EX_STORE_SIZE(v),
	    EX_NOWAIT);
	extent_alloc_region(ex, 0, 0xffffffffUL, EX_NOWAIT);

#ifdef CHIP_MEM_W1_BUS_START
#ifdef EXTENT_DEBUG
	printf("mem: freeing from 0x%lx to 0x%lx\n",
	    CHIP_MEM_W1_BUS_START(v), CHIP_MEM_W1_BUS_END(v));
#endif
	extent_free(ex, CHIP_MEM_W1_BUS_START(v),
	    CHIP_MEM_W1_BUS_END(v) - CHIP_MEM_W1_BUS_START(v) + 1,
	    EX_NOWAIT);
#endif
#ifdef CHIP_MEM_W2_BUS_START
	if (CHIP_MEM_W2_BUS_START(v) != CHIP_MEM_W1_BUS_START(v)) {
#ifdef EXTENT_DEBUG
		printf("mem: freeing from 0x%lx to 0x%lx\n",
		    CHIP_MEM_W2_BUS_START(v), CHIP_MEM_W2_BUS_END(v));
#endif
		extent_free(ex, CHIP_MEM_W2_BUS_START(v),
		    CHIP_MEM_W2_BUS_END(v) - CHIP_MEM_W2_BUS_START(v) + 1,
		    EX_NOWAIT);
	} else {
#ifdef EXTENT_DEBUG
		printf("mem: window 2 (0x%lx to 0x%lx) overlaps window 1\n",
		    CHIP_MEM_W2_BUS_START(v), CHIP_MEM_W2_BUS_END(v));
#endif
	}
#endif
#ifdef CHIP_MEM_W3_BUS_START
	if (CHIP_MEM_W3_BUS_START(v) != CHIP_MEM_W1_BUS_START(v) &&
	    CHIP_MEM_W3_BUS_START(v) != CHIP_MEM_W2_BUS_START(v)) {
#ifdef EXTENT_DEBUG
		printf("mem: freeing from 0x%lx to 0x%lx\n",
		    CHIP_MEM_W3_BUS_START(v), CHIP_MEM_W3_BUS_END(v));
#endif
		extent_free(ex, CHIP_MEM_W3_BUS_START(v),
		    CHIP_MEM_W3_BUS_END(v) - CHIP_MEM_W3_BUS_START(v) + 1,
		    EX_NOWAIT);
	} else {
#ifdef EXTENT_DEBUG
		printf("mem: window 2 (0x%lx to 0x%lx) overlaps window 1\n",
		    CHIP_MEM_W2_BUS_START(v), CHIP_MEM_W2_BUS_END(v));
#endif
	}
#endif

#ifdef EXTENT_DEBUG
        extent_print(ex);
#endif
        CHIP_MEM_EXTENT(v) = ex;
}

int
__C(CHIP,_mem_translate)(v, memaddr, memlen, flags, mbst)
	void *v;
	bus_addr_t memaddr;
	bus_size_t memlen;
	int flags;
	struct mips_bus_space_translation *mbst;
{
	bus_addr_t memend = memaddr + (memlen - 1);
#if CHIP_ALIGN_STRIDE != 0
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/*
	 * Can't map memory space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);
#endif

#ifdef CHIP_MEM_W1_BUS_START
	if (memaddr >= CHIP_MEM_W1_BUS_START(v) &&
	    memend <= CHIP_MEM_W1_BUS_END(v))
		return (__C(CHIP,_mem_get_window)(v, 0, mbst));
#endif

#ifdef CHIP_MEM_W2_BUS_START
	if (memaddr >= CHIP_MEM_W2_BUS_START(v) &&
	    memend <= CHIP_MEM_W2_BUS_END(v))
		return (__C(CHIP,_mem_get_window)(v, 1, mbst));
#endif

#ifdef CHIP_MEM_W3_BUS_START
	if (memaddr >= CHIP_MEM_W3_BUS_START(v) &&
	    memend <= CHIP_MEM_W3_BUS_END(v))
		return (__C(CHIP,_mem_get_window)(v, 2, mbst));
#endif

#ifdef EXTENT_DEBUG
	printf("\n");
#ifdef CHIP_MEM_W1_BUS_START
	printf("%s: window[1]=0x%lx-0x%lx\n",
	    __S(__C(CHIP,_mem_map)), CHIP_MEM_W1_BUS_START(v),
	    CHIP_MEM_W1_BUS_END(v));
#endif
#ifdef CHIP_MEM_W2_BUS_START
	printf("%s: window[2]=0x%lx-0x%lx\n",
	    __S(__C(CHIP,_mem_map)), CHIP_MEM_W2_BUS_START(v),
	    CHIP_MEM_W2_BUS_END(v));
#endif
#ifdef CHIP_MEM_W3_BUS_START
	printf("%s: window[3]=0x%lx-0x%lx\n",
	    __S(__C(CHIP,_mem_map)), CHIP_MEM_W3_BUS_START(v),
	    CHIP_MEM_W3_BUS_END(v));
#endif
#endif /* EXTENT_DEBUG */
	/* No translation. */
	return (EINVAL);
}

int
__C(CHIP,_mem_get_window)(v, window, mbst)
	void *v;
	int window;
	struct mips_bus_space_translation *mbst;
{

	switch (window) {
#ifdef CHIP_MEM_W1_BUS_START
	case 0:
		mbst->mbst_bus_start = CHIP_MEM_W1_BUS_START(v);
		mbst->mbst_bus_end = CHIP_MEM_W1_BUS_END(v);
		mbst->mbst_sys_start = CHIP_MEM_W1_SYS_START(v);
		mbst->mbst_sys_end = CHIP_MEM_W1_SYS_END(v);
		mbst->mbst_align_stride = CHIP_ALIGN_STRIDE;
		mbst->mbst_flags = 0;
		break;
#endif

#ifdef CHIP_MEM_W2_BUS_START
	case 1:
		mbst->mbst_bus_start = CHIP_MEM_W2_BUS_START(v);
		mbst->mbst_bus_end = CHIP_MEM_W2_BUS_END(v);
		mbst->mbst_sys_start = CHIP_MEM_W2_SYS_START(v);
		mbst->mbst_sys_end = CHIP_MEM_W2_SYS_END(v);
		mbst->mbst_align_stride = CHIP_ALIGN_STRIDE;
		mbst->mbst_flags = 0;
		break;
#endif

#ifdef CHIP_MEM_W3_BUS_START
	case 2:
		mbst->mbst_bus_start = CHIP_MEM_W3_BUS_START(v);
		mbst->mbst_bus_end = CHIP_MEM_W3_BUS_END(v);
		mbst->mbst_sys_start = CHIP_MEM_W3_SYS_START(v);
		mbst->mbst_sys_end = CHIP_MEM_W3_SYS_END(v);
		mbst->mbst_align_stride = CHIP_ALIGN_STRIDE;
		mbst->mbst_flags = 0;
		break;
#endif

	default:
		panic(__S(__C(CHIP,_mem_get_window)) ": invalid window %d",
		    window);
	}

	return (0);
}

int
__C(CHIP,_mem_map)(v, memaddr, memsize, flags, memhp, acct)
	void *v;
	bus_addr_t memaddr;
	bus_size_t memsize;
	int flags;
	bus_space_handle_t *memhp;
	int acct;
{
	struct mips_bus_space_translation mbst;
	int error;

	/*
	 * Get the translation for this address.
	 */
	error = __C(CHIP,_mem_translate)(v, memaddr, memsize, flags, &mbst);
	if (error)
		return (error);

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
	if (flags & BUS_SPACE_MAP_CACHEABLE)
		*memhp = MIPS_PHYS_TO_KSEG0(mbst.mbst_sys_start +
		    (memaddr - mbst.mbst_bus_start));
	else
		*memhp = MIPS_PHYS_TO_KSEG1(mbst.mbst_sys_start +
		    (memaddr - mbst.mbst_bus_start));

	return (0);
}

void
__C(CHIP,_mem_unmap)(v, memh, memsize, acct)
	void *v;
	bus_space_handle_t memh;
	bus_size_t memsize;
	int acct;
{
	bus_addr_t memaddr;
	int error;

	if (acct == 0)
		return;

#ifdef EXTENT_DEBUG
	printf("mem: freeing handle 0x%lx for 0x%lx\n", memh, memsize);
#endif

	if (memh >= MIPS_KSEG0_START && memh < MIPS_KSEG1_START)
		memh = MIPS_KSEG0_TO_PHYS(memh);
	else
		memh = MIPS_KSEG1_TO_PHYS(memh);

#ifdef CHIP_MEM_W1_BUS_START
	if (memh >= CHIP_MEM_W1_SYS_START(v) &&
	    memh <= CHIP_MEM_W1_SYS_END(v)) {
		memaddr = CHIP_MEM_W1_BUS_START(v) +
		    (memh - CHIP_MEM_W1_SYS_START(v));
	} else
#endif
#ifdef CHIP_MEM_W2_BUS_START
	if (memh >= CHIP_MEM_W2_SYS_START(v) &&
	    memh <= CHIP_MEM_W2_SYS_END(v)) {
		memaddr = CHIP_MEM_W2_BUS_START(v) +
		    (memh - CHIP_MEM_W2_SYS_START(v));
	} else
#endif
#ifdef CHIP_MEM_W3_BUS_START
	if (memh >= CHIP_MEM_W3_SYS_START(v) &&
	    memh <= CHIP_MEM_W3_SYS_END(v)) {
		memaddr = CHIP_MEM_W3_BUS_START(v) +
		    (memh - CHIP_MEM_W3_SYS_START(v));
	} else
#endif
	{
		printf("\n");
#ifdef CHIP_MEM_W1_BUS_START
		printf("%s: sys window[1]=0x%lx-0x%lx\n",
		    __S(__C(CHIP,_mem_map)), CHIP_MEM_W1_SYS_START(v),
		    CHIP_MEM_W1_SYS_END(v));
#endif
#ifdef CHIP_MEM_W2_BUS_START
		printf("%s: sys window[2]=0x%lx-0x%lx\n",
		    __S(__C(CHIP,_mem_map)), CHIP_MEM_W2_SYS_START(v),
		    CHIP_MEM_W2_SYS_END(v));
#endif
#ifdef CHIP_MEM_W3_BUS_START
		printf("%s: sys window[3]=0x%lx-0x%lx\n",
		    __S(__C(CHIP,_mem_map)), CHIP_MEM_W3_SYS_START(v),
		    CHIP_MEM_W3_SYS_END(v));
#endif
		panic("%s: don't know how to unmap %lx",
		    __S(__C(CHIP,_mem_unmap)), memh);
	}

#ifdef EXTENT_DEBUG
	printf("mem: freeing 0x%lx to 0x%lx\n", memaddr,
	    memaddr + memsize - 1);
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
__C(CHIP,_mem_subregion)(v, memh, offset, size, nmemh)
	void *v;
	bus_space_handle_t memh, *nmemh;
	bus_size_t offset, size;
{

	*nmemh = memh + (offset << CHIP_ALIGN_STRIDE);
	return (0);
}

int
__C(CHIP,_mem_alloc)(v, rstart, rend, size, align, boundary, flags,
    addrp, bshp)
	void *v;
	bus_addr_t rstart, rend, *addrp;
	bus_size_t size, align, boundary;
	int flags;
	bus_space_handle_t *bshp;
{
	struct mips_bus_space_translation mbst;
	bus_addr_t memaddr;
	int error;
#if CHIP_ALIGN_STRIDE != 0
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/*
	 * Can't map memory space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);
#endif

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
		return (error);
	}

#ifdef EXTENT_DEBUG
	printf("mem: allocated 0x%lx to 0x%lx\n", memaddr,
	    memaddr + size - 1);
#endif

	error = __C(CHIP,_mem_translate)(v, memaddr, size, flags, &mbst);
	if (error) {
		(void) extent_free(CHIP_MEM_EXTENT(v), memaddr, size,
		    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
		return (error);
	}

	*addrp = memaddr;
	if (flags & BUS_SPACE_MAP_CACHEABLE)
		*bshp = MIPS_PHYS_TO_KSEG0(mbst.mbst_sys_start +
		    (memaddr - mbst.mbst_bus_start));
	else
		*bshp = MIPS_PHYS_TO_KSEG1(mbst.mbst_sys_start +
		    (memaddr - mbst.mbst_bus_start));

	return (0);
}

void
__C(CHIP,_mem_free)(v, bsh, size)
	void *v;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/* Unmap does all we need to do. */
	__C(CHIP,_mem_unmap)(v, bsh, size, 1);
}

void *
__C(CHIP,_mem_vaddr)(v, bsh)
	void *v;
	bus_space_handle_t bsh;
{

#if CHIP_ALIGN_STRIDE != 0
	/* Linear mappings not possible. */
	return (NULL);
#else
	return ((void *)bsh);
#endif
}

inline void
__C(CHIP,_mem_barrier)(v, h, o, l, f)
	void *v;
	bus_space_handle_t h;
	bus_size_t o, l;
	int f;
{

	/* XXX XXX XXX */
	if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		wbflush();
}

inline u_int8_t
__C(CHIP,_mem_read_1)(v, memh, off)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
{
	u_int8_t *ptr = (void *)(memh + (off << CHIP_ALIGN_STRIDE));

	return (*ptr);
}

inline u_int16_t
__C(CHIP,_mem_read_2)(v, memh, off)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
{
#if CHIP_ALIGN_STRIDE >= 1 
	u_int16_t *ptr = (void *)(memh + (off << (CHIP_ALIGN_STRIDE - 1)));
#else
	u_int16_t *ptr = (void *)(memh + off);
#endif

	return (*ptr);
}

inline u_int32_t
__C(CHIP,_mem_read_4)(v, memh, off)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
{
#if CHIP_ALIGN_STRIDE >= 2
	u_int32_t *ptr = (void *)(memh + (off << (CHIP_ALIGN_STRIDE - 2)));
#else
	u_int32_t *ptr = (void *)(memh + off);
#endif

	return (*ptr);
}

inline u_int64_t
__C(CHIP,_mem_read_8)(v, memh, off)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
{

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_read_8)));
}

#define CHIP_mem_read_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_read_multi_),BYTES)(v, h, o, a, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		__C(CHIP,_mem_barrier)(v, h, o, sizeof *a,		\
		    BUS_SPACE_BARRIER_READ);				\
		*a++ = __C(__C(CHIP,_mem_read_),BYTES)(v, h, o);	\
	}								\
}
CHIP_mem_read_multi_N(1,u_int8_t)
CHIP_mem_read_multi_N(2,u_int16_t)
CHIP_mem_read_multi_N(4,u_int32_t)
CHIP_mem_read_multi_N(8,u_int64_t)

#define CHIP_mem_read_region_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_read_region_),BYTES)(v, h, o, a, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __C(__C(CHIP,_mem_read_),BYTES)(v, h, o);	\
		o += sizeof *a;						\
	}								\
}
CHIP_mem_read_region_N(1,u_int8_t)
CHIP_mem_read_region_N(2,u_int16_t)
CHIP_mem_read_region_N(4,u_int32_t)
CHIP_mem_read_region_N(8,u_int64_t)

inline void
__C(CHIP,_mem_write_1)(v, memh, off, val)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
	u_int8_t val;
{
	u_int8_t *ptr = (void *)(memh + (off << CHIP_ALIGN_STRIDE));

	*ptr = val;
}

inline void
__C(CHIP,_mem_write_2)(v, memh, off, val)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
	u_int16_t val;
{
#if CHIP_ALIGN_STRIDE >= 1
	u_int16_t *ptr = (void *)(memh + (off << (CHIP_ALIGN_STRIDE - 1)));
#else
	u_int16_t *ptr = (void *)(memh + off);
#endif

	*ptr = val;
}

inline void
__C(CHIP,_mem_write_4)(v, memh, off, val)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
	u_int32_t val;
{
#if CHIP_ALIGN_STRIDE >= 2
	u_int32_t *ptr = (void *)(memh + (off << (CHIP_ALIGN_STRIDE - 2)));
#else
	u_int32_t *ptr = (void *)(memh + off);
#endif

	*ptr = val;
}

inline void
__C(CHIP,_mem_write_8)(v, memh, off, val)
	void *v;
	bus_space_handle_t memh;
	bus_size_t off;
	u_int64_t val;
{

	/* XXX XXX XXX */
	panic("%s not implemented", __S(__C(CHIP,_mem_write_8)));
}

#define CHIP_mem_write_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_write_multi_),BYTES)(v, h, o, a, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	const TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, *a++);	\
		__C(CHIP,_mem_barrier)(v, h, o, sizeof *a,		\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_mem_write_multi_N(1,u_int8_t)
CHIP_mem_write_multi_N(2,u_int16_t)
CHIP_mem_write_multi_N(4,u_int32_t)
CHIP_mem_write_multi_N(8,u_int64_t)

#define CHIP_mem_write_region_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_write_region_),BYTES)(v, h, o, a, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	const TYPE *a;							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, *a++);	\
		o += sizeof *a;						\
	}								\
}
CHIP_mem_write_region_N(1,u_int8_t)
CHIP_mem_write_region_N(2,u_int16_t)
CHIP_mem_write_region_N(4,u_int32_t)
CHIP_mem_write_region_N(8,u_int64_t)

#define CHIP_mem_set_multi_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_set_multi_),BYTES)(v, h, o, val, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE val;							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, val);		\
		__C(CHIP,_mem_barrier)(v, h, o, sizeof val,		\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_mem_set_multi_N(1,u_int8_t)
CHIP_mem_set_multi_N(2,u_int16_t)
CHIP_mem_set_multi_N(4,u_int32_t)
CHIP_mem_set_multi_N(8,u_int64_t)

#define CHIP_mem_set_region_N(BYTES,TYPE)				\
void									\
__C(__C(CHIP,_mem_set_region_),BYTES)(v, h, o, val, c)			\
	void *v;							\
	bus_space_handle_t h;						\
	bus_size_t o, c;						\
	TYPE val;							\
{									\
									\
	while (c-- > 0) {						\
		__C(__C(CHIP,_mem_write_),BYTES)(v, h, o, val);		\
		o += sizeof val;					\
	}								\
}
CHIP_mem_set_region_N(1,u_int8_t)
CHIP_mem_set_region_N(2,u_int16_t)
CHIP_mem_set_region_N(4,u_int32_t)
CHIP_mem_set_region_N(8,u_int64_t)

#define	CHIP_mem_copy_region_N(BYTES)					\
void									\
__C(__C(CHIP,_mem_copy_region_),BYTES)(v, h1, o1, h2, o2, c)		\
	void *v;							\
	bus_space_handle_t h1, h2;					\
	bus_size_t o1, o2, c;						\
{									\
	bus_size_t o;							\
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
