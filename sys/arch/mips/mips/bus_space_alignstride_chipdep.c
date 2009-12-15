/* $NetBSD: bus_space_alignstride_chipdep.c,v 1.12 2009/12/15 06:07:14 rmind Exp $ */

/*-
 * Copyright (c) 1998, 2000, 2001 The NetBSD Foundation, Inc.
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
 * Common Chipset "bus I/O" functions.
 *
 * uses:
 *	CHIP		name of the 'chip' it's being compiled for.
 *	CHIP_BASE	memory or I/O space base to use.
 *	CHIP_EX_STORE
 *			If defined, device-provided static storage area
 *			for the memory or I/O space extent.  If this is
 *			defined, CHIP_EX_STORE_SIZE must also be defined.
 *			If this is not defined, a static area will be
 *			declared.
 *	CHIP_EX_STORE_SIZE
 *			Size of the device-provided static storage area
 *			for the memory or I/O memory space extent.
 *	CHIP_LITTLE_ENDIAN | CHIP_BIG_ENDIAN
 *			For endian-specific busses, like PCI (little).
 *	CHIP_ACCESS_SIZE
 *			Size (in bytes) of minimum bus access, e.g. 4
 *			to indicate all bus cycles are 32-bits.  Defaults
 *			to 1, indicating any access size is valid.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space_alignstride_chipdep.c,v 1.12 2009/12/15 06:07:14 rmind Exp $");

#ifdef CHIP_EXTENT
#include <sys/extent.h>
#endif
#include <sys/malloc.h>

#include <machine/locore.h>

#include <uvm/uvm_extern.h>

#define	__C(A,B)	__CONCAT(A,B)
#define	__S(S)		__STRING(S)

#ifdef CHIP_IO
#define	__BS(A)		__C(__C(CHIP,_bus_io_),A)
#endif
#ifdef CHIP_MEM
#define	__BS(A)		__C(__C(CHIP,_bus_mem_),A)
#endif

#if defined(CHIP_LITTLE_ENDIAN)
#define	CHIP_SWAP16(x)	le16toh(x)
#define	CHIP_SWAP32(x)	le32toh(x)
#define	CHIP_SWAP64(x)	le64toh(x)
#define	CHIP_NEED_STREAM	1
#elif defined(CHIP_BIG_ENDIAN)
#define	CHIP_SWAP16(x)	be16toh(x)
#define	CHIP_SWAP32(x)	be32toh(x)
#define	CHIP_SWAP64(x)	be64toh(x)
#define	CHIP_NEED_STREAM	1	
#else
#define	CHIP_SWAP16(x)	(x)
#define	CHIP_SWAP32(x)	(x)
#define	CHIP_SWAP64(x)	(x)
#endif

#ifndef	CHIP_ACCESS_SIZE
#define	CHIP_ACCESS_SIZE	1
#endif

#if CHIP_ACCESS_SIZE==1
# define CHIP_SWAP_ACCESS(x)	(x)
#elif CHIP_ACCESS_SIZE==2
# define CHIP_SWAP_ACCESS(x)	CHIP_SWAP16(x)
#elif CHIP_ACCESS_SIZE==4
# define CHIP_SWAP_ACCESS(x)	CHIP_SWAP32(x)
#elif CHIP_ACCESS_SIZE==8
# define CHIP_SWAP_ACCESS(x)	CHIP_SWAP64(x)
#else
# error your access size not implemented
#endif

/*
 * The logic here determines a few macros to support requirements for
 * whole-word accesses:
 *
 * CHIP_TYPE is a uintXX_t that represents the native access type for the bus.
 *
 * CHIP_SHIFTXX is the number of bits to shift a big-endian value to convert
 * convert between the CHIP_TYPE and uintXX_t.
 *
 * The idea is that if we want to do a 16bit load from a bus that only
 * supports 32-bit accesses, we will access the first 16 bits of the
 * addressed 32-bit word.
 *
 * Obviously (hopefully) this method is inadequate to support addressing the
 * second half of a 16-bit word, or the upper 3/4 of a 32-bit value, etc.
 * In other words, the drivers should probably not be relying on this!
 *
 * We should probably come back in here some day and handle offsets properly.
 * to do that, we need to mask off the low order bits of the address, and
 * then figure out which bits they correspond to.
 *
 * If we have fixed access size limitations, we need to make sure that
 * handle shifting required for big-endian storage.  The reality is
 * that if the bus only supports size "n", then drivers should
 * probably only access it using "n" sized (or bigger) accesses.
 */

#if	CHIP_ACCESS_SIZE == 1
#define	CHIP_TYPE	uint8_t
#endif

#if	CHIP_ACCESS_SIZE == 2
#define	CHIP_TYPE	uint16_t
#endif

#if	CHIP_ACCESS_SIZE == 4
#define	CHIP_TYPE	uint32_t
#endif

#if	CHIP_ACCESS_SIZE == 8
#define	CHIP_TYPE	uint64_t
#endif

#ifndef CHIP_TYPE
#error	"Invalid chip access size!"
#endif

/* mapping/unmapping */
int		__BS(map)(void *, bus_addr_t, bus_size_t, int,
		    bus_space_handle_t *, int);
void		__BS(unmap)(void *, bus_space_handle_t, bus_size_t, int);
int		__BS(subregion)(void *, bus_space_handle_t, bus_size_t,
		    bus_size_t, bus_space_handle_t *);

int		__BS(translate)(void *, bus_addr_t, bus_size_t, int,
		    struct mips_bus_space_translation *);
int		__BS(get_window)(void *, int,
		    struct mips_bus_space_translation *);

/* allocation/deallocation */
int		__BS(alloc)(void *, bus_addr_t, bus_addr_t, bus_size_t,
		    bus_size_t, bus_addr_t, int, bus_addr_t *,
		    bus_space_handle_t *);
void		__BS(free)(void *, bus_space_handle_t, bus_size_t);

/* get kernel virtual address */
void *		__BS(vaddr)(void *, bus_space_handle_t);

/* mmap for user */
paddr_t		__BS(mmap)(void *, bus_addr_t, off_t, int, int);

/* barrier */
inline void	__BS(barrier)(void *, bus_space_handle_t, bus_size_t,
		    bus_size_t, int);

/* read (single) */
inline uint8_t	__BS(read_1)(void *, bus_space_handle_t, bus_size_t);
inline uint16_t	__BS(read_2)(void *, bus_space_handle_t, bus_size_t);
inline uint32_t	__BS(read_4)(void *, bus_space_handle_t, bus_size_t);
inline uint64_t	__BS(read_8)(void *, bus_space_handle_t, bus_size_t);

/* read multiple */
void		__BS(read_multi_1)(void *, bus_space_handle_t, bus_size_t,
		    uint8_t *, bus_size_t);
void		__BS(read_multi_2)(void *, bus_space_handle_t, bus_size_t,
		    uint16_t *, bus_size_t);
void		__BS(read_multi_4)(void *, bus_space_handle_t, bus_size_t,
		    uint32_t *, bus_size_t);
void		__BS(read_multi_8)(void *, bus_space_handle_t, bus_size_t,
		    uint64_t *, bus_size_t);

/* read region */
void		__BS(read_region_1)(void *, bus_space_handle_t, bus_size_t,
		    uint8_t *, bus_size_t);
void		__BS(read_region_2)(void *, bus_space_handle_t, bus_size_t,
		    uint16_t *, bus_size_t);
void		__BS(read_region_4)(void *, bus_space_handle_t, bus_size_t,
		    uint32_t *, bus_size_t);
void		__BS(read_region_8)(void *, bus_space_handle_t, bus_size_t,
		    uint64_t *, bus_size_t);

/* write (single) */
inline void	__BS(write_1)(void *, bus_space_handle_t, bus_size_t, uint8_t);
inline void	__BS(write_2)(void *, bus_space_handle_t, bus_size_t, uint16_t);
inline void	__BS(write_4)(void *, bus_space_handle_t, bus_size_t, uint32_t);
inline void	__BS(write_8)(void *, bus_space_handle_t, bus_size_t, uint64_t);

/* write multiple */
void		__BS(write_multi_1)(void *, bus_space_handle_t, bus_size_t,
		    const uint8_t *, bus_size_t);
void		__BS(write_multi_2)(void *, bus_space_handle_t, bus_size_t,
		    const uint16_t *, bus_size_t);
void		__BS(write_multi_4)(void *, bus_space_handle_t, bus_size_t,
		    const uint32_t *, bus_size_t);
void		__BS(write_multi_8)(void *, bus_space_handle_t, bus_size_t,
		    const uint64_t *, bus_size_t);

/* write region */
void		__BS(write_region_1)(void *, bus_space_handle_t, bus_size_t,
		    const uint8_t *, bus_size_t);
void		__BS(write_region_2)(void *, bus_space_handle_t, bus_size_t,
		    const uint16_t *, bus_size_t);
void		__BS(write_region_4)(void *, bus_space_handle_t, bus_size_t,
		    const uint32_t *, bus_size_t);
void		__BS(write_region_8)(void *, bus_space_handle_t, bus_size_t,
		    const uint64_t *, bus_size_t);

/* set multiple */
void		__BS(set_multi_1)(void *, bus_space_handle_t, bus_size_t,
		    uint8_t, bus_size_t);
void		__BS(set_multi_2)(void *, bus_space_handle_t, bus_size_t,
		    uint16_t, bus_size_t);
void		__BS(set_multi_4)(void *, bus_space_handle_t, bus_size_t,
		    uint32_t, bus_size_t);
void		__BS(set_multi_8)(void *, bus_space_handle_t, bus_size_t,
		    uint64_t, bus_size_t);

/* set region */
void		__BS(set_region_1)(void *, bus_space_handle_t, bus_size_t,
		    uint8_t, bus_size_t);
void		__BS(set_region_2)(void *, bus_space_handle_t, bus_size_t,
		    uint16_t, bus_size_t);
void		__BS(set_region_4)(void *, bus_space_handle_t, bus_size_t,
		    uint32_t, bus_size_t);
void		__BS(set_region_8)(void *, bus_space_handle_t, bus_size_t,
		    uint64_t, bus_size_t);

/* copy */
void		__BS(copy_region_1)(void *, bus_space_handle_t, bus_size_t,
		    bus_space_handle_t, bus_size_t, bus_size_t);
void		__BS(copy_region_2)(void *, bus_space_handle_t, bus_size_t,
		    bus_space_handle_t, bus_size_t, bus_size_t);
void		__BS(copy_region_4)(void *, bus_space_handle_t, bus_size_t,
		    bus_space_handle_t, bus_size_t, bus_size_t);
void		__BS(copy_region_8)(void *, bus_space_handle_t, bus_size_t,
		    bus_space_handle_t, bus_size_t, bus_size_t);

#ifdef	CHIP_NEED_STREAM

/* read (single), stream */
inline uint8_t	__BS(read_stream_1)(void *, bus_space_handle_t, bus_size_t);
inline uint16_t	__BS(read_stream_2)(void *, bus_space_handle_t, bus_size_t);
inline uint32_t	__BS(read_stream_4)(void *, bus_space_handle_t, bus_size_t);
inline uint64_t	__BS(read_stream_8)(void *, bus_space_handle_t, bus_size_t);

/* read multiple, stream */
void	__BS(read_multi_stream_1)(void *, bus_space_handle_t, bus_size_t,
			   uint8_t *, bus_size_t);
void	__BS(read_multi_stream_2)(void *, bus_space_handle_t, bus_size_t,
				  uint16_t *, bus_size_t);
void	__BS(read_multi_stream_4)(void *, bus_space_handle_t, bus_size_t,
				  uint32_t *, bus_size_t);
void	__BS(read_multi_stream_8)(void *, bus_space_handle_t, bus_size_t,
				  uint64_t *, bus_size_t);

/* read region, stream */
void	__BS(read_region_stream_1)(void *, bus_space_handle_t, bus_size_t,
				   uint8_t *, bus_size_t);
void	__BS(read_region_stream_2)(void *, bus_space_handle_t, bus_size_t,
				   uint16_t *, bus_size_t);
void	__BS(read_region_stream_4)(void *, bus_space_handle_t, bus_size_t,
				   uint32_t *, bus_size_t);
void	__BS(read_region_stream_8)(void *, bus_space_handle_t, bus_size_t,
				   uint64_t *, bus_size_t);

/* write (single), stream */
inline void	__BS(write_stream_1)(void *, bus_space_handle_t, bus_size_t, uint8_t);
inline void	__BS(write_stream_2)(void *, bus_space_handle_t, bus_size_t, uint16_t);
inline void	__BS(write_stream_4)(void *, bus_space_handle_t, bus_size_t, uint32_t);
inline void	__BS(write_stream_8)(void *, bus_space_handle_t, bus_size_t, uint64_t);

/* write multiple, stream */
void	__BS(write_multi_stream_1)(void *, bus_space_handle_t, bus_size_t,
				   const uint8_t *, bus_size_t);
void	__BS(write_multi_stream_2)(void *, bus_space_handle_t, bus_size_t,
				   const uint16_t *, bus_size_t);
void	__BS(write_multi_stream_4)(void *, bus_space_handle_t, bus_size_t,
				   const uint32_t *, bus_size_t);
void	__BS(write_multi_stream_8)(void *, bus_space_handle_t, bus_size_t,
				   const uint64_t *, bus_size_t);

/* write region, stream */
void	__BS(write_region_stream_1)(void *, bus_space_handle_t, bus_size_t,
				    const uint8_t *, bus_size_t);
void	__BS(write_region_stream_2)(void *, bus_space_handle_t, bus_size_t,
				    const uint16_t *, bus_size_t);
void	__BS(write_region_stream_4)(void *, bus_space_handle_t, bus_size_t,
				    const uint32_t *, bus_size_t);
void	__BS(write_region_stream_8)(void *, bus_space_handle_t, bus_size_t,
				    const uint64_t *, bus_size_t);

#endif	/* CHIP_NEED_STREAM */

#ifdef CHIP_EXTENT
#ifndef	CHIP_EX_STORE
static long
    __BS(ex_storage)[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
#define	CHIP_EX_STORE(v)	(__BS(ex_storage))
#define	CHIP_EX_STORE_SIZE(v)	(sizeof __BS(ex_storage))
#endif
#endif /* CHIP_EXTENT */

#ifndef CHIP_ALIGN_STRIDE
#define	CHIP_ALIGN_STRIDE	0
#endif

#if CHIP_ALIGN_STRIDE > 0
#define	CHIP_OFF8(o)	((o) << (CHIP_ALIGN_STRIDE))
#else
#define	CHIP_OFF8(o)	(o)
#endif

#if CHIP_ALIGN_STRIDE > 1
#define	CHIP_OFF16(o)	((o) << (CHIP_ALIGN_STRIDE - 1))
#else
#define	CHIP_OFF16(o)	(o)
#endif

#if CHIP_ALIGN_STRIDE > 2
#define	CHIP_OFF32(o)	((o) << (CHIP_ALIGN_STRIDE - 2))
#else
#define	CHIP_OFF32(o)	(o)
#endif

#if CHIP_ALIGN_STRIDE > 3
#define	CHIP_OFF64(o)	((o) << (CHIP_ALIGN_STRIDE - 3))
#else
#define	CHIP_OFF64(o)	(o)
#endif


void
__BS(init)(bus_space_tag_t t, void *v)
{
#ifdef CHIP_EXTENT
	struct extent *ex;
#endif

	/*
	 * Initialize the bus space tag.
	 */

	/* cookie */
	t->bs_cookie =		v;

	/* mapping/unmapping */
	t->bs_map =		__BS(map);
	t->bs_unmap =		__BS(unmap);
	t->bs_subregion =	__BS(subregion);

	t->bs_translate =	__BS(translate);
	t->bs_get_window =	__BS(get_window);

	/* allocation/deallocation */
	t->bs_alloc =		__BS(alloc);
	t->bs_free =		__BS(free);

	/* get kernel virtual address */
	t->bs_vaddr =		__BS(vaddr);

	/* mmap for user */
	t->bs_mmap =		__BS(mmap);

	/* barrier */
	t->bs_barrier =		__BS(barrier);
	
	/* read (single) */
	t->bs_r_1 =		__BS(read_1);
	t->bs_r_2 =		__BS(read_2);
	t->bs_r_4 =		__BS(read_4);
	t->bs_r_8 =		__BS(read_8);
	
	/* read multiple */
	t->bs_rm_1 =		__BS(read_multi_1);
	t->bs_rm_2 =		__BS(read_multi_2);
	t->bs_rm_4 =		__BS(read_multi_4);
	t->bs_rm_8 =		__BS(read_multi_8);
	
	/* read region */
	t->bs_rr_1 =		__BS(read_region_1);
	t->bs_rr_2 =		__BS(read_region_2);
	t->bs_rr_4 =		__BS(read_region_4);
	t->bs_rr_8 =		__BS(read_region_8);
	
	/* write (single) */
	t->bs_w_1 =		__BS(write_1);
	t->bs_w_2 =		__BS(write_2);
	t->bs_w_4 =		__BS(write_4);
	t->bs_w_8 =		__BS(write_8);
	
	/* write multiple */
	t->bs_wm_1 =		__BS(write_multi_1);
	t->bs_wm_2 =		__BS(write_multi_2);
	t->bs_wm_4 =		__BS(write_multi_4);
	t->bs_wm_8 =		__BS(write_multi_8);
	
	/* write region */
	t->bs_wr_1 =		__BS(write_region_1);
	t->bs_wr_2 =		__BS(write_region_2);
	t->bs_wr_4 =		__BS(write_region_4);
	t->bs_wr_8 =		__BS(write_region_8);

	/* set multiple */
	t->bs_sm_1 =		__BS(set_multi_1);
	t->bs_sm_2 =		__BS(set_multi_2);
	t->bs_sm_4 =		__BS(set_multi_4);
	t->bs_sm_8 =		__BS(set_multi_8);
	
	/* set region */
	t->bs_sr_1 =		__BS(set_region_1);
	t->bs_sr_2 =		__BS(set_region_2);
	t->bs_sr_4 =		__BS(set_region_4);
	t->bs_sr_8 =		__BS(set_region_8);

	/* copy */
	t->bs_c_1 =		__BS(copy_region_1);
	t->bs_c_2 =		__BS(copy_region_2);
	t->bs_c_4 =		__BS(copy_region_4);
	t->bs_c_8 =		__BS(copy_region_8);

#ifdef CHIP_NEED_STREAM
	/* read (single), stream */
	t->bs_rs_1 =		__BS(read_stream_1);
	t->bs_rs_2 =		__BS(read_stream_2);
	t->bs_rs_4 =		__BS(read_stream_4);
	t->bs_rs_8 =		__BS(read_stream_8);
	
	/* read multiple, stream */
	t->bs_rms_1 =		__BS(read_multi_stream_1);
	t->bs_rms_2 =		__BS(read_multi_stream_2);
	t->bs_rms_4 =		__BS(read_multi_stream_4);
	t->bs_rms_8 =		__BS(read_multi_stream_8);
	
	/* read region, stream */
	t->bs_rrs_1 =		__BS(read_region_stream_1);
	t->bs_rrs_2 =		__BS(read_region_stream_2);
	t->bs_rrs_4 =		__BS(read_region_stream_4);
	t->bs_rrs_8 =		__BS(read_region_stream_8);
	
	/* write (single), stream */
	t->bs_ws_1 =		__BS(write_stream_1);
	t->bs_ws_2 =		__BS(write_stream_2);
	t->bs_ws_4 =		__BS(write_stream_4);
	t->bs_ws_8 =		__BS(write_stream_8);
	
	/* write multiple, stream */
	t->bs_wms_1 =		__BS(write_multi_stream_1);
	t->bs_wms_2 =		__BS(write_multi_stream_2);
	t->bs_wms_4 =		__BS(write_multi_stream_4);
	t->bs_wms_8 =		__BS(write_multi_stream_8);
	
	/* write region, stream */
	t->bs_wrs_1 =		__BS(write_region_stream_1);
	t->bs_wrs_2 =		__BS(write_region_stream_2);
	t->bs_wrs_4 =		__BS(write_region_stream_4);
	t->bs_wrs_8 =		__BS(write_region_stream_8);

#else	/* CHIP_NEED_STREAM */

	/* read (single), stream */
	t->bs_rs_1 =		__BS(read_1);
	t->bs_rs_2 =		__BS(read_2);
	t->bs_rs_4 =		__BS(read_4);
	t->bs_rs_8 =		__BS(read_8);
	
	/* read multiple, stream */
	t->bs_rms_1 =		__BS(read_multi_1);
	t->bs_rms_2 =		__BS(read_multi_2);
	t->bs_rms_4 =		__BS(read_multi_4);
	t->bs_rms_8 =		__BS(read_multi_8);
	
	/* read region, stream */
	t->bs_rrs_1 =		__BS(read_region_1);
	t->bs_rrs_2 =		__BS(read_region_2);
	t->bs_rrs_4 =		__BS(read_region_4);
	t->bs_rrs_8 =		__BS(read_region_8);
	
	/* write (single), stream */
	t->bs_ws_1 =		__BS(write_1);
	t->bs_ws_2 =		__BS(write_2);
	t->bs_ws_4 =		__BS(write_4);
	t->bs_ws_8 =		__BS(write_8);
	
	/* write multiple, stream */
	t->bs_wms_1 =		__BS(write_multi_1);
	t->bs_wms_2 =		__BS(write_multi_2);
	t->bs_wms_4 =		__BS(write_multi_4);
	t->bs_wms_8 =		__BS(write_multi_8);
	
	/* write region, stream */
	t->bs_wrs_1 =		__BS(write_region_1);
	t->bs_wrs_2 =		__BS(write_region_2);
	t->bs_wrs_4 =		__BS(write_region_4);
	t->bs_wrs_8 =		__BS(write_region_8);
#endif	/* CHIP_NEED_STREAM */

#ifdef CHIP_EXTENT
	/* XXX WE WANT EXTENT_NOCOALESCE, BUT WE CAN'T USE IT. XXX */
	ex = extent_create(__S(__BS(bus)), 0x0UL, ~0UL, M_DEVBUF,
	    (void *)CHIP_EX_STORE(v), CHIP_EX_STORE_SIZE(v), EX_NOWAIT);
	extent_alloc_region(ex, 0, ~0UL, EX_NOWAIT);

#ifdef CHIP_W1_BUS_START
	/*
	 * The window may be disabled.  We notice this by seeing
	 * -1 as the bus base address.
	 */
	if (CHIP_W1_BUS_START(v) == (bus_addr_t) -1) {
#ifdef EXTENT_DEBUG
		printf("%s: this space is disabled\n", __S(__BS(init)));
#endif
		return;
	}

#ifdef EXTENT_DEBUG
	printf("%s: freeing from %#"PRIxBUSADDR" to %#"PRIxBUSADDR"\n",
	    __S(__BS(init)), (bus_addr_t)CHIP_W1_BUS_START(v),
	    (bus_addr_t)CHIP_W1_BUS_END(v));
#endif
	extent_free(ex, CHIP_W1_BUS_START(v),
	    CHIP_W1_BUS_END(v) - CHIP_W1_BUS_START(v) + 1, EX_NOWAIT);
#endif
#ifdef CHIP_W2_BUS_START
	if (CHIP_W2_BUS_START(v) != CHIP_W1_BUS_START(v)) {
#ifdef EXTENT_DEBUG
		printf("xxx: freeing from 0x%lx to 0x%lx\n",
		    (u_long)CHIP_W2_BUS_START(v), (u_long)CHIP_W2_BUS_END(v));
#endif
		extent_free(ex, CHIP_W2_BUS_START(v),
		    CHIP_W2_BUS_END(v) - CHIP_W2_BUS_START(v) + 1, EX_NOWAIT);
	} else {
#ifdef EXTENT_DEBUG
		printf("xxx: window 2 (0x%lx to 0x%lx) overlaps window 1\n",
		    (u_long)CHIP_W2_BUS_START(v), (u_long)CHIP_W2_BUS_END(v));
#endif
	}
#endif
#ifdef CHIP_W3_BUS_START
	if (CHIP_W3_BUS_START(v) != CHIP_W1_BUS_START(v) &&
	    CHIP_W3_BUS_START(v) != CHIP_W2_BUS_START(v)) {
#ifdef EXTENT_DEBUG
		printf("xxx: freeing from 0x%lx to 0x%lx\n",
		    (u_long)CHIP_W3_BUS_START(v), (u_long)CHIP_W3_BUS_END(v));
#endif
		extent_free(ex, CHIP_W3_BUS_START(v),
		    CHIP_W3_BUS_END(v) - CHIP_W3_BUS_START(v) + 1, EX_NOWAIT);
	} else {
#ifdef EXTENT_DEBUG
		printf("xxx: window 2 (0x%lx to 0x%lx) overlaps window 1\n",
		    (u_long)CHIP_W2_BUS_START(v), (u_long)CHIP_W2_BUS_END(v));
#endif
	}
#endif

#ifdef EXTENT_DEBUG
	extent_print(ex);
#endif
	CHIP_EXTENT(v) = ex;
#endif /* CHIP_EXTENT */
}

int
__BS(translate)(void *v, bus_addr_t addr, bus_size_t len, int flags,
    struct mips_bus_space_translation *mbst)
{
	bus_addr_t end = addr + (len - 1);
#if CHIP_ALIGN_STRIDE != 0
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/*
	 * Can't map xxx space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);
#endif

#ifdef CHIP_W1_BUS_START
	if (addr >= CHIP_W1_BUS_START(v) && end <= CHIP_W1_BUS_END(v))
		return (__BS(get_window)(v, 0, mbst));
#endif

#ifdef CHIP_W2_BUS_START
	if (addr >= CHIP_W2_BUS_START(v) && end <= CHIP_W2_BUS_END(v))
		return (__BS(get_window)(v, 1, mbst));
#endif

#ifdef CHIP_W3_BUS_START
	if (addr >= CHIP_W3_BUS_START(v) && end <= CHIP_W3_BUS_END(v))
		return (__BS(get_window)(v, 2, mbst));
#endif

#ifdef EXTENT_DEBUG
	printf("\n");
#ifdef CHIP_W1_BUS_START
	printf("%s: window[1]=0x%lx-0x%lx\n", __S(__BS(map)),
	    (u_long)CHIP_W1_BUS_START(v), (u_long)CHIP_W1_BUS_END(v));
#endif
#ifdef CHIP_W2_BUS_START
	printf("%s: window[2]=0x%lx-0x%lx\n", __S(__BS(map)),
	    (u_long)CHIP_W2_BUS_START(v), (u_long)CHIP_W2_BUS_END(v));
#endif
#ifdef CHIP_W3_BUS_START
	printf("%s: window[3]=0x%lx-0x%lx\n", __S(__BS(map)),
	    (u_long)CHIP_W3_BUS_START(v), (u_long)CHIP_W3_BUS_END(v));
#endif
#endif /* EXTENT_DEBUG */
	/* No translation. */
	return (EINVAL);
}

int
__BS(get_window)(void *v, int window, struct mips_bus_space_translation *mbst)
{

	switch (window) {
#ifdef CHIP_W1_BUS_START
	case 0:
		mbst->mbst_bus_start = CHIP_W1_BUS_START(v);
		mbst->mbst_bus_end = CHIP_W1_BUS_END(v);
		mbst->mbst_sys_start = CHIP_W1_SYS_START(v);
		mbst->mbst_sys_end = CHIP_W1_SYS_END(v);
		mbst->mbst_align_stride = CHIP_ALIGN_STRIDE;
		mbst->mbst_flags = 0;
		break;
#endif

#ifdef CHIP_W2_BUS_START
	case 1:
		mbst->mbst_bus_start = CHIP_W2_BUS_START(v);
		mbst->mbst_bus_end = CHIP_W2_BUS_END(v);
		mbst->mbst_sys_start = CHIP_W2_SYS_START(v);
		mbst->mbst_sys_end = CHIP_W2_SYS_END(v);
		mbst->mbst_align_stride = CHIP_ALIGN_STRIDE;
		mbst->mbst_flags = 0;
		break;
#endif

#ifdef CHIP_W3_BUS_START
	case 2:
		mbst->mbst_bus_start = CHIP_W3_BUS_START(v);
		mbst->mbst_bus_end = CHIP_W3_BUS_END(v);
		mbst->mbst_sys_start = CHIP_W3_SYS_START(v);
		mbst->mbst_sys_end = CHIP_W3_SYS_END(v);
		mbst->mbst_align_stride = CHIP_ALIGN_STRIDE;
		mbst->mbst_flags = 0;
		break;
#endif

	default:
		panic(__S(__BS(get_window)) ": invalid window %d",
		    window);
	}

	return (0);
}

int
__BS(map)(void *v, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *hp, int acct)
{
	struct mips_bus_space_translation mbst;
	int error;

	/*
	 * Get the translation for this address.
	 */
	error = __BS(translate)(v, addr, size, flags, &mbst);
	if (error)
		return (error);

#ifdef CHIP_EXTENT
	if (acct == 0)
		goto mapit;

#ifdef EXTENT_DEBUG
	printf("%s: allocating %#"PRIxBUSADDR" to %#"PRIxBUSADDR"\n",
		__S(__BS(map)), addr, addr + size - 1);
#endif
        error = extent_alloc_region(CHIP_EXTENT(v), addr, size,
            EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
	if (error) {
#ifdef EXTENT_DEBUG
		printf("%s: allocation failed (%d)\n", __S(__BS(map)), error);
		extent_print(CHIP_EXTENT(v));
#endif
		return (error);
	}

 mapit:
#endif /* CHIP_EXTENT */

	addr = mbst.mbst_sys_start + (addr - mbst.mbst_bus_start);

#ifdef _LP64
	if (flags & BUS_SPACE_MAP_CACHEABLE)
		*hp = MIPS_PHYS_TO_XKPHYS_CACHED(addr);
	else
		*hp = MIPS_PHYS_TO_XKPHYS_UNCACHED(addr);
#else
	if (((addr + size) & ~MIPS_PHYS_MASK) != 0) {
		vaddr_t va;
		paddr_t pa;
		int s;

		size = round_page((addr % PAGE_SIZE) + size);
		va = uvm_km_alloc(kernel_map, size, PAGE_SIZE,
			UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
		if (va == 0)
			return ENOMEM;

		/* check use of handle_is_kseg2 in BS(unmap) */
		KASSERT((va & ~MIPS_PHYS_MASK) == MIPS_KSEG2_START);

		*hp = va + (addr & PAGE_MASK);
		pa = trunc_page(addr);

		s = splhigh();
		while (size != 0) {
			pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
			pa += PAGE_SIZE;
			va += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		pmap_update(pmap_kernel());
		splx(s);
	} else {
		if (flags & BUS_SPACE_MAP_CACHEABLE)
			*hp = MIPS_PHYS_TO_KSEG0(addr);
		else
			*hp = MIPS_PHYS_TO_KSEG1(addr);
	}
#endif

	return (0);
}

void
__BS(unmap)(void *v, bus_space_handle_t h, bus_size_t size, int acct)
{
#if !defined(_LP64) || defined(CHIP_EXTENT)
	bus_addr_t addr = 0;	/* initialize to appease gcc */
#endif
#ifndef _LP64
	bool handle_is_kseg2;

	/* determine if h is addr obtained from uvm_km_alloc */
	handle_is_kseg2 = ((h & ~MIPS_PHYS_MASK) == MIPS_KSEG2_START);
#if 0
	printf("%s:%d: is_kseg2 %d\n", __func__, __LINE__, handle_is_kseg2);
#endif
	if (handle_is_kseg2 == true) {
		paddr_t pa;
		vaddr_t va = (vaddr_t)trunc_page(h);
		vsize_t sz = (vsize_t)round_page((h % PAGE_SIZE) + size);
		int s;

		s = splhigh();

		if (pmap_extract(pmap_kernel(), (vaddr_t)h, &pa) == false)
			panic("%s: pmap_extract failed", __func__);
		addr = (bus_addr_t)pa;
#if 0
		printf("%s:%d: addr %#"PRIxBUSADDR", sz %#"PRIxVSIZE"\n",
			__func__, __LINE__, addr, sz);
#endif
		/* sanity check: this is why we couldn't map w/ kseg[0,1] */
		KASSERT (((addr + sz) & ~MIPS_PHYS_MASK) != 0);

		pmap_kremove(va, sz);
		pmap_update(pmap_kernel());
		uvm_km_free(kernel_map, va, sz, UVM_KMF_VAONLY);

		splx(s);
	}
#endif	/* _LP64 */

#ifdef CHIP_EXTENT

	if (acct == 0)
		return;

#ifdef EXTENT_DEBUG
	printf("%s: freeing handle %#"PRIxBSH" for %#"PRIxBUSSIZE"\n",
		__S(__BS(unmap)), h, size);
#endif

#ifdef _LP64
	KASSERT(MIPS_XKPHYS_P(h));
	addr = MIPS_XKPHYS_TO_PHYS(h);
#else
	if (handle_is_kseg2 == false) {
		if (MIPS_KSEG0_P(h))
			addr = MIPS_KSEG0_TO_PHYS(h);
		else
			addr = MIPS_KSEG1_TO_PHYS(h);
	}
#endif

#ifdef CHIP_W1_BUS_START
	if (addr >= CHIP_W1_SYS_START(v) && addr <= CHIP_W1_SYS_END(v)) {
		addr = CHIP_W1_BUS_START(v) + (addr - CHIP_W1_SYS_START(v));
	} else
#endif
#ifdef CHIP_W2_BUS_START
	if (addr >= CHIP_W2_SYS_START(v) && addr <= CHIP_W2_SYS_END(v)) {
		addr = CHIP_W2_BUS_START(v) + (addr - CHIP_W2_SYS_START(v));
	} else
#endif
#ifdef CHIP_W3_BUS_START
	if (addr >= CHIP_W3_SYS_START(v) && addr <= CHIP_W3_SYS_END(v)) {
		addr = CHIP_W3_BUS_START(v) + (addr - CHIP_W3_SYS_START(v));
	} else
#endif
	{
		printf("\n");
#ifdef CHIP_W1_BUS_START
		printf("%s: sys window[1]=0x%lx-0x%lx\n",
		    __S(__BS(map)), (u_long)CHIP_W1_SYS_START(v),
		    (u_long)CHIP_W1_SYS_END(v));
#endif
#ifdef CHIP_W2_BUS_START
		printf("%s: sys window[2]=0x%lx-0x%lx\n",
		    __S(__BS(map)), (u_long)CHIP_W2_SYS_START(v),
		    (u_long)CHIP_W2_SYS_END(v));
#endif
#ifdef CHIP_W3_BUS_START
		printf("%s: sys window[3]=0x%lx-0x%lx\n",
		    __S(__BS(unmap)), (u_long)CHIP_W3_SYS_START(v),
		    (u_long)CHIP_W3_SYS_END(v));
#endif
		panic("%s: don't know how to unmap %#"PRIxBSH, __S(__BS(unmap)), h);
	}

#ifdef EXTENT_DEBUG
	printf("%s: freeing %#"PRIxBUSADDR" to %#"PRIxBUSADDR"\n",
	    __S(__BS(unmap)), addr, addr + size - 1);
#endif
        int error = extent_free(CHIP_EXTENT(v), addr, size,
            EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
	if (error) {
		printf("%s: WARNING: could not unmap"
		    " %#"PRIxBUSADDR"-%#"PRIxBUSADDR" (error %d)\n",
		    __S(__BS(unmap)), addr, addr + size - 1, error);
#ifdef EXTENT_DEBUG
		extent_print(CHIP_EXTENT(v));
#endif
	}	
#endif /* CHIP_EXTENT */
}

int
__BS(subregion)(void *v, bus_space_handle_t h, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nh)
{

	*nh = h + (offset << CHIP_ALIGN_STRIDE);
	return (0);
}

int
__BS(alloc)(void *v, bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
    bus_size_t align, bus_size_t boundary, int flags, bus_addr_t *addrp,
    bus_space_handle_t *bshp)
{
#ifdef CHIP_EXTENT
	struct mips_bus_space_translation mbst;
	u_long addr;	/* bogus but makes extent happy */
	int error;
#if CHIP_ALIGN_STRIDE != 0
	int linear = flags & BUS_SPACE_MAP_LINEAR;

	/*
	 * Can't map xxx space linearly.
	 */
	if (linear)
		return (EOPNOTSUPP);
#endif

	/*
	 * Do the requested allocation.
	 */
#ifdef EXTENT_DEBUG
	printf("%s: allocating from %#"PRIxBUSADDR" to %#"PRIxBUSADDR"\n",
		__S(__BS(alloc)), rstart, rend);
#endif
	error = extent_alloc_subregion(CHIP_EXTENT(v), rstart, rend, size,
	    align, boundary,
	    EX_FAST | EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0),
	    &addr);
	if (error) {
#ifdef EXTENT_DEBUG
		printf("%s: allocation failed (%d)\n", __S(__BS(alloc)), error);
		extent_print(CHIP_EXTENT(v));
#endif
		return (error);
	}

#ifdef EXTENT_DEBUG
	printf("%s: allocated 0x%lx to %#"PRIxBUSSIZE"\n",
		__S(__BS(alloc)), addr, addr + size - 1);
#endif

	error = __BS(translate)(v, addr, size, flags, &mbst);
	if (error) {
		(void) extent_free(CHIP_EXTENT(v), addr, size,
		    EX_NOWAIT | (CHIP_EX_MALLOC_SAFE(v) ? EX_MALLOCOK : 0));
		return (error);
	}

	*addrp = addr;
#ifdef _LP64
	if (flags & BUS_SPACE_MAP_CACHEABLE)
		*bshp = MIPS_PHYS_TO_XKPHYS_CACHED(mbst.mbst_sys_start +
		    (addr - mbst.mbst_bus_start));
	else
		*bshp = MIPS_PHYS_TO_XKPHYS_UNCACHED(mbst.mbst_sys_start +
		    (addr - mbst.mbst_bus_start));
#else
	if (flags & BUS_SPACE_MAP_CACHEABLE)
		*bshp = MIPS_PHYS_TO_KSEG0(mbst.mbst_sys_start +
		    (addr - mbst.mbst_bus_start));
	else
		*bshp = MIPS_PHYS_TO_KSEG1(mbst.mbst_sys_start +
		    (addr - mbst.mbst_bus_start));
#endif

	return (0);
#else /* ! CHIP_EXTENT */
	return (EOPNOTSUPP);
#endif /* CHIP_EXTENT */
}

void
__BS(free)(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	/* Unmap does all we need to do. */
	__BS(unmap)(v, bsh, size, 1);
}

void *
__BS(vaddr)(void *v, bus_space_handle_t bsh)
{

#if CHIP_ALIGN_STRIDE != 0
	/* Linear mappings not possible. */
	return (NULL);
#else
	return ((void *)bsh);
#endif
}

paddr_t
__BS(mmap)(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{
#ifdef CHIP_IO

	/* Not supported for I/O space. */
	return (-1);
#elif defined(CHIP_MEM)
	struct mips_bus_space_translation mbst;
	int error;

	/*
	 * Get the translation for this address.
	 */
	error = __BS(translate)(v, addr, off + PAGE_SIZE, flags,
	    &mbst);
	if (error)
		return (-1);

	return (mips_btop(mbst.mbst_sys_start +
	    (addr - mbst.mbst_bus_start) + off));
#else
# error must define one of CHIP_IO or CHIP_MEM
#endif
}

inline void
__BS(barrier)(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l, int f)
{

	/* XXX XXX XXX */
	if ((f & BUS_SPACE_BARRIER_WRITE) != 0)
		wbflush();
}

inline uint8_t
__BS(read_1)(void *v, bus_space_handle_t h, bus_size_t off)
{
#if CHIP_ACCESS_SIZE > 1
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 1 */
	volatile uint8_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 1 */
	uint8_t r;
        int shift;

        h += CHIP_OFF8(off);
        shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
        ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
        r = (uint8_t)(CHIP_SWAP_ACCESS(*ptr) >> shift);

        return r;
}

inline uint16_t
__BS(read_2)(void *v, bus_space_handle_t h, bus_size_t off)
{
#if CHIP_ACCESS_SIZE > 2
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 2 */
	volatile uint16_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 2 */
	uint16_t r;
        int shift;

	KASSERT((off & 1) == 0);
        h += CHIP_OFF16(off);
        shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
        ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
	r = (uint16_t)CHIP_SWAP16(*ptr >> shift);

	return r;
}

inline uint32_t
__BS(read_4)(void *v, bus_space_handle_t h, bus_size_t off)
{
#if CHIP_ACCESS_SIZE > 4
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 4 */
	volatile uint32_t *ptr;
#endif
	uint32_t r;
        int shift;

	KASSERT((off & 3) == 0);
        h += CHIP_OFF32(off);
        shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
        ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
	r = (uint32_t)CHIP_SWAP32(*ptr >> shift);

	return r;
}

inline uint64_t
__BS(read_8)(void *v, bus_space_handle_t h, bus_size_t off)
{
	volatile uint64_t *ptr;
	volatile uint64_t r;
        int shift;

	KASSERT((off & 7) == 0);
        h += CHIP_OFF64(off);
        shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
        ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
	r =  CHIP_SWAP64(*ptr >> shift);

	return r;
}


#define CHIP_read_multi_N(BYTES,TYPE)					\
void									\
__C(__BS(read_multi_),BYTES)(void *v, bus_space_handle_t h,		\
    bus_size_t o, TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__BS(barrier)(v, h, o, sizeof *a,			\
		    BUS_SPACE_BARRIER_READ);				\
		*a++ = __C(__BS(read_),BYTES)(v, h, o);			\
	}								\
}
CHIP_read_multi_N(1,uint8_t)
CHIP_read_multi_N(2,uint16_t)
CHIP_read_multi_N(4,uint32_t)
CHIP_read_multi_N(8,uint64_t)

#define CHIP_read_region_N(BYTES,TYPE)					\
void									\
__C(__BS(read_region_),BYTES)(void *v, bus_space_handle_t h,		\
    bus_size_t o, TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __C(__BS(read_),BYTES)(v, h, o);			\
		o += sizeof *a;						\
	}								\
}
CHIP_read_region_N(1,uint8_t)
CHIP_read_region_N(2,uint16_t)
CHIP_read_region_N(4,uint32_t)
CHIP_read_region_N(8,uint64_t)


inline void
__BS(write_1)(void *v, bus_space_handle_t h, bus_size_t off, uint8_t val)
{
#if CHIP_ACCESS_SIZE > 1
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 1 */
	volatile uint8_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 1 */
	int shift;

	h += CHIP_OFF8(off);
	shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
	ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
	*ptr = CHIP_SWAP_ACCESS(((CHIP_TYPE)val) << shift);
}

inline void
__BS(write_2)(void *v, bus_space_handle_t h, bus_size_t off, uint16_t val)
{
#if CHIP_ACCESS_SIZE > 2
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 2 */
	volatile uint16_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 2 */
	int shift;

	KASSERT((off & 1) == 0);
	h += CHIP_OFF16(off);
	shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
	ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
	if (CHIP_ACCESS_SIZE > 2)
		*ptr = (CHIP_TYPE)(CHIP_SWAP16(val)) << shift;
	else
		*ptr = CHIP_SWAP16(val);
}

inline void
__BS(write_4)(void *v, bus_space_handle_t h, bus_size_t off, uint32_t val)
{
#if CHIP_ACCESS_SIZE > 4
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 4 */
	volatile uint32_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 4 */
        int shift;

	KASSERT((off & 3) == 0);
        h += CHIP_OFF32(off);
        shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
        ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
        if (CHIP_ACCESS_SIZE > 4)
		*ptr = (CHIP_TYPE)(CHIP_SWAP32(val)) << shift;
        else
                *ptr = CHIP_SWAP32(val);
}

inline void
__BS(write_8)(void *v, bus_space_handle_t h, bus_size_t off, uint64_t val)
{
	volatile uint64_t *ptr;
        int shift;

	KASSERT((off & 7) == 0);
        h += CHIP_OFF64(off);
        shift = (off & (CHIP_ACCESS_SIZE - 1)) * 8;
        ptr = (void *)(h & ~((bus_space_handle_t)(CHIP_ACCESS_SIZE - 1)));
	*ptr = CHIP_SWAP64(val) << shift;
}

#define CHIP_write_multi_N(BYTES,TYPE)					\
void									\
__C(__BS(write_multi_),BYTES)(void *v, bus_space_handle_t h,		\
    bus_size_t o, const TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__C(__BS(write_),BYTES)(v, h, o, *a++);			\
		__BS(barrier)(v, h, o, sizeof *a,			\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_write_multi_N(1,uint8_t)
CHIP_write_multi_N(2,uint16_t)
CHIP_write_multi_N(4,uint32_t)
CHIP_write_multi_N(8,uint64_t)

#define CHIP_write_region_N(BYTES,TYPE)					\
void									\
__C(__BS(write_region_),BYTES)(void *v, bus_space_handle_t h,		\
    bus_size_t o, const TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__C(__BS(write_),BYTES)(v, h, o, *a++);			\
		o += sizeof *a;						\
	}								\
}
CHIP_write_region_N(1,uint8_t)
CHIP_write_region_N(2,uint16_t)
CHIP_write_region_N(4,uint32_t)
CHIP_write_region_N(8,uint64_t)

#define CHIP_set_multi_N(BYTES,TYPE)					\
void									\
__C(__BS(set_multi_),BYTES)(void *v, bus_space_handle_t h,		\
    bus_size_t o, TYPE val, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__C(__BS(write_),BYTES)(v, h, o, val);			\
		__BS(barrier)(v, h, o, sizeof val,			\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_set_multi_N(1,uint8_t)
CHIP_set_multi_N(2,uint16_t)
CHIP_set_multi_N(4,uint32_t)
CHIP_set_multi_N(8,uint64_t)

#define CHIP_set_region_N(BYTES,TYPE)					\
void									\
__C(__BS(set_region_),BYTES)(void *v, bus_space_handle_t h,		\
    bus_size_t o, TYPE val, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__C(__BS(write_),BYTES)(v, h, o, val);			\
		o += sizeof val;					\
	}								\
}
CHIP_set_region_N(1,uint8_t)
CHIP_set_region_N(2,uint16_t)
CHIP_set_region_N(4,uint32_t)
CHIP_set_region_N(8,uint64_t)

#define	CHIP_copy_region_N(BYTES)					\
void									\
__C(__BS(copy_region_),BYTES)(void *v, bus_space_handle_t h1,		\
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)	\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__C(__BS(write_),BYTES)(v, h2, o2 + o,		\
			    __C(__BS(read_),BYTES)(v, h1, o1 + o));	\
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__C(__BS(write_),BYTES)(v, h2, o2 + o,		\
			    __C(__BS(read_),BYTES)(v, h1, o1 + o));	\
	}								\
}
CHIP_copy_region_N(1)
CHIP_copy_region_N(2)
CHIP_copy_region_N(4)
CHIP_copy_region_N(8)

#ifdef	CHIP_NEED_STREAM

inline uint8_t
__BS(read_stream_1)(void *v, bus_space_handle_t h, bus_size_t off)
{
#if CHIP_ACCESS_SIZE > 1
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 1 */
	volatile uint8_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 1 */

	ptr = (void *)(intptr_t)(h + CHIP_OFF8(off));
	return *ptr & 0xff;
}

inline uint16_t
__BS(read_stream_2)(void *v, bus_space_handle_t h, bus_size_t off)
{
#if CHIP_ACCESS_SIZE > 2
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 2 */
	volatile uint16_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 2 */

	ptr = (void *)(intptr_t)(h + CHIP_OFF16(off));
	return *ptr & 0xffff;
}

inline uint32_t
__BS(read_stream_4)(void *v, bus_space_handle_t h, bus_size_t off)
{
#if CHIP_ACCESS_SIZE > 4
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 4 */
	volatile uint32_t *ptr;
#endif

	ptr = (void *)(intptr_t)(h + CHIP_OFF32(off));
	return *ptr & 0xffffffff;
}

inline uint64_t
__BS(read_stream_8)(void *v, bus_space_handle_t h, bus_size_t off)
{
	volatile uint64_t *ptr;

	ptr = (void *)(intptr_t)(h + CHIP_OFF64(off));
	return *ptr;
}

#define CHIP_read_multi_stream_N(BYTES,TYPE)				\
void									\
__C(__BS(read_multi_stream_),BYTES)(void *v, bus_space_handle_t h,	\
    bus_size_t o, TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__BS(barrier)(v, h, o, sizeof *a,			\
		    BUS_SPACE_BARRIER_READ);				\
		*a++ = __C(__BS(read_stream_),BYTES)(v, h, o);		\
	}								\
}
CHIP_read_multi_stream_N(1,uint8_t)
CHIP_read_multi_stream_N(2,uint16_t)
CHIP_read_multi_stream_N(4,uint32_t)
CHIP_read_multi_stream_N(8,uint64_t)

#define CHIP_read_region_stream_N(BYTES,TYPE)				\
void									\
__C(__BS(read_region_stream_),BYTES)(void *v, bus_space_handle_t h,	\
    bus_size_t o, TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		*a++ = __C(__BS(read_stream_),BYTES)(v, h, o);		\
		o += sizeof *a;						\
	}								\
}
CHIP_read_region_stream_N(1,uint8_t)
CHIP_read_region_stream_N(2,uint16_t)
CHIP_read_region_stream_N(4,uint32_t)
CHIP_read_region_stream_N(8,uint64_t)

inline void
__BS(write_stream_1)(void *v, bus_space_handle_t h, bus_size_t off,
		     uint8_t val)
{
#if CHIP_ACCESS_SIZE > 1
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 1 */
	volatile uint8_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 1 */

	ptr = (void *)(intptr_t)(h + CHIP_OFF8(off));
	*ptr = val;
}

inline void
__BS(write_stream_2)(void *v, bus_space_handle_t h, bus_size_t off,
	      uint16_t val)
{
#if CHIP_ACCESS_SIZE > 2
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 2 */
	volatile uint16_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 2 */

	ptr = (void *)(intptr_t)(h + CHIP_OFF16(off));
	*ptr = val;
}

inline void
__BS(write_stream_4)(void *v, bus_space_handle_t h, bus_size_t off,
		     uint32_t val)
{
#if CHIP_ACCESS_SIZE > 4
	volatile CHIP_TYPE *ptr;
#else	/* CHIP_ACCESS_SIZE > 4 */
	volatile uint32_t *ptr;
#endif	/* CHIP_ACCESS_SIZE > 4 */

	ptr = (void *)(intptr_t)(h + CHIP_OFF32(off));
	*ptr = val;
}

inline void
__BS(write_stream_8)(void *v, bus_space_handle_t h, bus_size_t off,
		     uint64_t val)
{
	volatile uint64_t *ptr;

	ptr = (void *)(intptr_t)(h + CHIP_OFF64(off));
	*ptr = val;
}

#define CHIP_write_multi_stream_N(BYTES,TYPE)				\
void									\
__C(__BS(write_multi_stream_),BYTES)(void *v, bus_space_handle_t h,	\
    bus_size_t o, const TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__C(__BS(write_stream_),BYTES)(v, h, o, *a++);		\
		__BS(barrier)(v, h, o, sizeof *a,			\
		    BUS_SPACE_BARRIER_WRITE);				\
	}								\
}
CHIP_write_multi_stream_N(1,uint8_t)
CHIP_write_multi_stream_N(2,uint16_t)
CHIP_write_multi_stream_N(4,uint32_t)
CHIP_write_multi_stream_N(8,uint64_t)

#define CHIP_write_region_stream_N(BYTES,TYPE)				\
void									\
__C(__BS(write_region_stream_),BYTES)(void *v, bus_space_handle_t h,	\
    bus_size_t o, const TYPE *a, bus_size_t c)				\
{									\
									\
	while (c-- > 0) {						\
		__C(__BS(write_stream_),BYTES)(v, h, o, *a++);		\
		o += sizeof *a;						\
	}								\
}
CHIP_write_region_stream_N(1,uint8_t)
CHIP_write_region_stream_N(2,uint16_t)
CHIP_write_region_stream_N(4,uint32_t)
CHIP_write_region_stream_N(8,uint64_t)

#endif	/* CHIP_NEED_STREAM */
