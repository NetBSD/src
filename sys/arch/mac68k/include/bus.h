/*	$NetBSD: bus.h,v 1.19 2002/04/09 01:53:47 briggs Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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
 * Copyright (C) 1997 Scott Reynolds.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MAC68K_BUS_H_
#define _MAC68K_BUS_H_

/*
 * Value for the mac68k bus space tag, not to be used directly by MI code.
 */
#define MAC68K_BUS_SPACE_MEM	0	/* space is mem space */

#define __BUS_SPACE_HAS_STREAM_METHODS 1

/*
 * Bus address and size types
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
#define BSH_T	struct bus_space_handle_s
typedef int	bus_space_tag_t;
typedef struct bus_space_handle_s {
	u_long	base;
	int	swapped;
	int	stride;
	
	u_int8_t	(*bsr1) __P((bus_space_tag_t t,
					BSH_T *h, bus_size_t o));
	u_int16_t	(*bsr2) __P((bus_space_tag_t t,
					BSH_T *h, bus_size_t o));
	u_int32_t	(*bsr4) __P((bus_space_tag_t t,
					BSH_T *h, bus_size_t o));
	u_int8_t	(*bsrs1) __P((bus_space_tag_t t,
					BSH_T *h, bus_size_t o));
	u_int16_t	(*bsrs2) __P((bus_space_tag_t t,
					BSH_T *h, bus_size_t o));
	u_int32_t	(*bsrs4) __P((bus_space_tag_t t,
					BSH_T *h, bus_size_t o));
	void		(*bsrm1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int8_t *a, size_t c));
	void		(*bsrm2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int16_t *a, size_t c));
	void		(*bsrm4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int32_t *a, size_t c));
	void		(*bsrms1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int8_t *a, size_t c));
	void		(*bsrms2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int16_t *a, size_t c));
	void		(*bsrms4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int32_t *a, size_t c));
	void		(*bsrr1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int8_t *a, size_t c));
	void		(*bsrr2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int16_t *a, size_t c));
	void		(*bsrr4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int32_t *a, size_t c));
	void		(*bsrrs1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int8_t *a, size_t c));
	void		(*bsrrs2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int16_t *a, size_t c));
	void		(*bsrrs4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int32_t *a, size_t c));
	void		(*bsw1) __P((bus_space_tag_t t, BSH_T *h,
				  bus_size_t o, u_int8_t v));
	void		(*bsw2) __P((bus_space_tag_t t, BSH_T *h,
				  bus_size_t o, u_int16_t v));
	void		(*bsw4) __P((bus_space_tag_t t, BSH_T *h,
				  bus_size_t o, u_int32_t v));
	void		(*bsws1) __P((bus_space_tag_t t, BSH_T *h,
				  bus_size_t o, u_int8_t v));
	void		(*bsws2) __P((bus_space_tag_t t, BSH_T *h,
				  bus_size_t o, u_int16_t v));
	void		(*bsws4) __P((bus_space_tag_t t, BSH_T *h,
				  bus_size_t o, u_int32_t v));
	void		(*bswm1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int8_t *a, size_t c));
	void		(*bswm2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int16_t *a, size_t c));
	void		(*bswm4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int32_t *a, size_t c));
	void		(*bswms1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int8_t *a, size_t c));
	void		(*bswms2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int16_t *a, size_t c));
	void		(*bswms4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int32_t *a, size_t c));
	void		(*bswr1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int8_t *a, size_t c));
	void		(*bswr2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int16_t *a, size_t c));
	void		(*bswr4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int32_t *a, size_t c));
	void		(*bswrs1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int8_t *a, size_t c));
	void		(*bswrs2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int16_t *a, size_t c));
	void		(*bswrs4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, const u_int32_t *a, size_t c));
	void		(*bssm1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int8_t v, size_t c));
	void		(*bssm2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int16_t v, size_t c));
	void		(*bssm4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int32_t v, size_t c));
	void		(*bssr1) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int8_t v, size_t c));
	void		(*bssr2) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int16_t v, size_t c));
	void		(*bssr4) __P((bus_space_tag_t t, BSH_T *h,
				   bus_size_t o, u_int32_t v, size_t c));
} bus_space_handle_t;
#undef BSH_T

void	mac68k_bus_space_handle_swapped __P((bus_space_tag_t t,
		bus_space_handle_t *h));
void	mac68k_bus_space_handle_set_stride __P((bus_space_tag_t t,
		bus_space_handle_t *h, int stride));

/*
 *	int bus_space_map __P((bus_space_tag_t t, bus_addr_t addr,
 *	    bus_size_t size, int flags, bus_space_handle_t *bshp));
 *
 * Map a region of bus space.
 */

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE	0x04

int	bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t,
	    int, bus_space_handle_t *));

/*
 *	void bus_space_unmap __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Unmap a region of bus space.
 */

void	bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

/*
 *	int bus_space_subregion __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp));
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */

int	bus_space_subregion __P((bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp));

/*
 *	int bus_space_alloc __P((bus_space_tag_t t, bus_addr_t, rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp));
 *
 * Allocate a region of bus space.
 */

int	bus_space_alloc __P((bus_space_tag_t t, bus_addr_t rstart,
	    bus_addr_t rend, bus_size_t size, bus_size_t align,
	    bus_size_t boundary, int cacheable, bus_addr_t *addrp,
	    bus_space_handle_t *bshp));

/*
 *	int bus_space_free __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size));
 *
 * Free a region of bus space.
 */

void	bus_space_free __P((bus_space_tag_t t, bus_space_handle_t bsh,
	    bus_size_t size));

/*
 *	int mac68k_bus_space_probe __P((bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, int sz));
 *
 * Probe the bus at t/bsh/offset, using sz as the size of the load.
 *
 * This is a machine-dependent extension, and is not to be used by
 * machine-independent code.
 */

int	mac68k_bus_space_probe __P((bus_space_tag_t t,
	    bus_space_handle_t bsh, bus_size_t offset, int sz));

/*
 *	u_intN_t bus_space_read_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset));
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

u_int8_t mac68k_bsr1 __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset));
u_int8_t mac68k_bsr1_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset));
u_int16_t mac68k_bsr2 __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset));
u_int16_t mac68k_bsr2_swap __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset));
u_int16_t mac68k_bsr2_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset));
u_int16_t mac68k_bsrs2_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset));
u_int32_t mac68k_bsr4 __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset));
u_int32_t mac68k_bsr4_swap __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset));
u_int32_t mac68k_bsr4_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset));
u_int32_t mac68k_bsrs4_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset));

#define	bus_space_read_1(t,h,o)	(h).bsr1((t), &(h), (o))
#define	bus_space_read_2(t,h,o)	(h).bsr2((t), &(h), (o))
#define	bus_space_read_4(t,h,o)	(h).bsr4((t), &(h), (o))
#define	bus_space_read_stream_1(t,h,o)	(h).bsrs1((t), &(h), (o))
#define	bus_space_read_stream_2(t,h,o)	(h).bsrs2((t), &(h), (o))
#define	bus_space_read_stream_4(t,h,o)	(h).bsrs4((t), &(h), (o))

#if 0	/* Cause a link error for bus_space_read_8 */
#define	bus_space_read_8(t, h, o)	!!! bus_space_read_8 unimplemented !!!
#define	bus_space_read_stream_8(t, h, o) \
				!!! bus_space_read_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

void mac68k_bsrm1 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int8_t *a, size_t c));
void mac68k_bsrm1_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int8_t *a, size_t c));
void mac68k_bsrm2 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrm2_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrm2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrms2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrm4 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrms4 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrm4_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrm4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrms4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));

#if defined(DIAGNOSTIC)
#define	bus_space_read_multi_1(t, h, o, a, c) do {			 \
	if (!c) panic("bus_space_read_multi_1 called with zero count."); \
	(h).bsrm1(t,&(h),o,a,c); } while (0)
#define	bus_space_read_multi_2(t, h, o, a, c) do {			 \
	if (!c) panic("bus_space_read_multi_2 called with zero count."); \
	(h).bsrm2(t,&(h),o,a,c); } while (0)
#define	bus_space_read_multi_4(t, h, o, a, c) do {			 \
	if (!c) panic("bus_space_read_multi_4 called with zero count."); \
	(h).bsrm4(t,&(h),o,a,c); } while (0)
#define	bus_space_read_multi_stream_1(t, h, o, a, c) do {		 \
	if (!c) panic("bus_space_read_multi_stream_1 called with count=0."); \
	(h).bsrms1(t,&(h),o,a,c); } while (0)
#define	bus_space_read_multi_stream_2(t, h, o, a, c) do {		 \
	if (!c) panic("bus_space_read_multi_stream_2 called with count=0."); \
	(h).bsrms2(t,&(h),o,a,c); } while (0)
#define	bus_space_read_multi_stream_4(t, h, o, a, c) do {		 \
	if (!c) panic("bus_space_read_multi_stream_4 called with count=0."); \
	(h).bsrms4(t,&(h),o,a,c); } while (0)
#else
#define	bus_space_read_multi_1(t, h, o, a, c) \
	do { if (c) (h).bsrm1(t, &(h), o, a, c); } while (0)
#define	bus_space_read_multi_2(t, h, o, a, c) \
	do { if (c) (h).bsrm2(t, &(h), o, a, c); } while (0)
#define	bus_space_read_multi_4(t, h, o, a, c) \
	do { if (c) (h).bsrm4(t, &(h), o, a, c); } while (0)
#define	bus_space_read_multi_stream_1(t, h, o, a, c) \
	do { if (c) (h).bsrms1(t, &(h), o, a, c); } while (0)
#define	bus_space_read_multi_stream_2(t, h, o, a, c) \
	do { if (c) (h).bsrms2(t, &(h), o, a, c); } while (0)
#define	bus_space_read_multi_stream_4(t, h, o, a, c) \
	do { if (c) (h).bsrms4(t, &(h), o, a, c); } while (0)
#endif

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define	bus_space_read_multi_8	!!! bus_space_read_multi_8 unimplemented !!!
#define	bus_space_read_multi_stream_8	\
			!!! bus_space_read_multi_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

void mac68k_bsrr1 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int8_t *a, size_t c));
void mac68k_bsrr1_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int8_t *a, size_t c));
void mac68k_bsrr2 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrr2_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrr2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrrs2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int16_t *a, size_t c));
void mac68k_bsrr4 __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrr4_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrr4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));
void mac68k_bsrrs4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
				   bus_size_t o, u_int32_t *a, size_t c));

#if defined(DIAGNOSTIC)
#define	bus_space_read_region_1(t, h, o, a, c) do {			  \
	if (!c) panic("bus_space_read_region_1 called with zero count."); \
	(h).bsrr1(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_2(t, h, o, a, c) do {			  \
	if (!c) panic("bus_space_read_region_2 called with zero count."); \
	(h).bsrr2(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_4(t, h, o, a, c) do {			  \
	if (!c) panic("bus_space_read_region_4 called with zero count."); \
	(h).bsrr4(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_stream_1(t, h, o, a, c) do {		  \
	if (!c) panic("bus_space_read_region_stream_1 called with count=0."); \
	(h).bsrrs1(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_stream_2(t, h, o, a, c) do {		  \
	if (!c) panic("bus_space_read_region_stream_2 called with count=0."); \
	(h).bsrrs2(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_stream_4(t, h, o, a, c) do {		  \
	if (!c) panic("bus_space_read_region_stream_4 called with count=0."); \
	(h).bsrrs4(t,&(h),o,a,c); } while (0)
#else
#define	bus_space_read_region_1(t, h, o, a, c) \
	do { if (c) (h).bsrr1(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_2(t, h, o, a, c) \
	do { if (c) (h).bsrr2(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_4(t, h, o, a, c) \
	do { if (c) (h).bsrr4(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_stream_1(t, h, o, a, c) \
	do { if (c) (h).bsrrs1(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_stream_2(t, h, o, a, c) \
	do { if (c) (h).bsrrs2(t,&(h),o,a,c); } while (0)
#define	bus_space_read_region_stream_4(t, h, o, a, c) \
	do { if (c) (h).bsrrs4(t,&(h),o,a,c); } while (0)
#endif

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8	!!! bus_space_read_region_8 unimplemented !!!
#define	bus_space_read_region_stream_8	\
			!!! bus_space_read_region_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value));
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

void mac68k_bsw1 __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset, u_int8_t v));
void mac68k_bsw1_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset, u_int8_t v));
void mac68k_bsw2 __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset, u_int16_t v));
void mac68k_bsw2_swap __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset, u_int16_t v));
void mac68k_bsw2_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset, u_int16_t v));
void mac68k_bsws2_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset, u_int16_t v));
void mac68k_bsw4 __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
			   bus_size_t offset, u_int32_t v));
void mac68k_bsw4_swap __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset, u_int32_t v));
void mac68k_bsw4_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset, u_int32_t v));
void mac68k_bsws4_gen __P((bus_space_tag_t tag, bus_space_handle_t *bsh,
				bus_size_t offset, u_int32_t v));

#define	bus_space_write_1(t, h, o, v) (h).bsw1(t, &(h), o, v)
#define	bus_space_write_2(t, h, o, v) (h).bsw2(t, &(h), o, v)
#define	bus_space_write_4(t, h, o, v) (h).bsw4(t, &(h), o, v)
#define	bus_space_write_stream_1(t, h, o, v) (h).bsws1(t, &(h), o, v)
#define	bus_space_write_stream_2(t, h, o, v) (h).bsws2(t, &(h), o, v)
#define	bus_space_write_stream_4(t, h, o, v) (h).bsws4(t, &(h), o, v)

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_8	!!! bus_space_write_8 not implemented !!!
#define	bus_space_write_stream_8 \
			!!! bus_space_write_stream_8 not implemented !!!
#endif

/*
 *	void bus_space_write_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

void mac68k_bswm1 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int8_t *a, size_t c));
void mac68k_bswm1_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int8_t *a, size_t c));
void mac68k_bswm2 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswm2_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswm2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswms2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswm4 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));
void mac68k_bswm4_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));
void mac68k_bswm4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));
void mac68k_bswms4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));

#if defined(DIAGNOSTIC)
#define	bus_space_write_multi_1(t, h, o, a, c) do {			  \
	if (!c) panic("bus_space_write_multi_1 called with zero count."); \
	(h).bswm1(t,&(h),o,a,c); } while (0)
#define	bus_space_write_multi_2(t, h, o, a, c) do {			  \
	if (!c) panic("bus_space_write_multi_2 called with zero count."); \
	(h).bswm2(t,&(h),o,a,c); } while (0)
#define	bus_space_write_multi_4(t, h, o, a, c) do {			  \
	if (!c) panic("bus_space_write_multi_4 called with zero count."); \
	(h).bswm4(t,&(h),o,a,c); } while (0)
#define	bus_space_write_multi_stream_1(t, h, o, a, c) do {		  \
	if (!c) panic("bus_space_write_multi_stream_1 called with count=0."); \
	(h).bswms1(t,&(h),o,a,c); } while (0)
#define	bus_space_write_multi_stream_2(t, h, o, a, c) do {		  \
	if (!c) panic("bus_space_write_multi_stream_2 called with count=0."); \
	(h).bswms2(t,&(h),o,a,c); } while (0)
#define	bus_space_write_multi_stream_4(t, h, o, a, c) do {		  \
	if (!c) panic("bus_space_write_multi_stream_4 called with count=0."); \
	(h).bswms4(t,&(h),o,a,c); } while (0)
#else
#define	bus_space_write_multi_1(t, h, o, a, c) \
	do { if (c) (h).bswm1(t, &(h), o, a, c); } while (0)
#define	bus_space_write_multi_2(t, h, o, a, c) \
	do { if (c) (h).bswm2(t, &(h), o, a, c); } while (0)
#define	bus_space_write_multi_4(t, h, o, a, c) \
	do { if (c) (h).bswm4(t, &(h), o, a, c); } while (0)
#define	bus_space_write_multi_stream_1(t, h, o, a, c) \
	do { if (c) (h).bswms1(t, &(h), o, a, c); } while (0)
#define	bus_space_write_multi_stream_2(t, h, o, a, c) \
	do { if (c) (h).bswms2(t, &(h), o, a, c); } while (0)
#define	bus_space_write_multi_stream_4(t, h, o, a, c) \
	do { if (c) (h).bswms4(t, &(h), o, a, c); } while (0)
#endif

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_multi_8(t, h, o, a, c)				\
			!!! bus_space_write_multi_8 unimplimented !!!
#define	bus_space_write_multi_stream_8(t, h, o, a, c)			\
			!!! bus_space_write_multi_stream_8 unimplimented !!!
#endif

/*
 *	void bus_space_write_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

void mac68k_bswr1 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int8_t *a, size_t c));
void mac68k_bswr1_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int8_t *a, size_t c));
void mac68k_bswr2 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswr2_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswr2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswrs2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int16_t *a, size_t c));
void mac68k_bswr4 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));
void mac68k_bswr4_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));
void mac68k_bswr4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));
void mac68k_bswrs4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, const u_int32_t *a, size_t c));

#if defined(DIAGNOSTIC)
#define	bus_space_write_region_1(t, h, o, a, c) do {			   \
	if (!c) panic("bus_space_write_region_1 called with zero count."); \
	(h).bswr1(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_2(t, h, o, a, c) do {			   \
	if (!c) panic("bus_space_write_region_2 called with zero count."); \
	(h).bswr2(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_4(t, h, o, a, c) do {			   \
	if (!c) panic("bus_space_write_region_4 called with zero count."); \
	(h).bswr4(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_stream_1(t, h, o, a, c) do {		   \
	if (!c) panic("bus_space_write_region_stream_1 called with count=0."); \
	(h).bswrs1(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_stream_2(t, h, o, a, c) do {		   \
	if (!c) panic("bus_space_write_region_stream_2 called with count=0."); \
	(h).bswrs2(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_stream_4(t, h, o, a, c) do {		   \
	if (!c) panic("bus_space_write_region_stream_4 called with count=0."); \
	(h).bswrs4(t,&(h),o,a,c); } while (0)
#else
#define	bus_space_write_region_1(t, h, o, a, c) \
	do { if (c) (h).bswr1(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_2(t, h, o, a, c) \
	do { if (c) (h).bswr2(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_4(t, h, o, a, c) \
	do { if (c) (h).bswr4(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_stream_1(t, h, o, a, c) \
	do { if (c) (h).bswrs1(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_stream_2(t, h, o, a, c) \
	do { if (c) (h).bswrs2(t,&(h),o,a,c); } while (0)
#define	bus_space_write_region_stream_4(t, h, o, a, c) \
	do { if (c) (h).bswrs4(t,&(h),o,a,c); } while (0)
#endif

#if 0	/* Cause a link error for bus_space_write_region_8 */
#define	bus_space_write_region_8					\
			!!! bus_space_write_region_8 unimplemented !!!
#define	bus_space_write_region_stream_8				\
			!!! bus_space_write_region_stream_8 unimplemented !!!
#endif

/*
 *	void bus_space_set_multi_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

void mac68k_bssm1 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int8_t v, size_t c));
void mac68k_bssm1_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int8_t v, size_t c));
void mac68k_bssm2 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int16_t v, size_t c));
void mac68k_bssm2_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int16_t v, size_t c));
void mac68k_bssm2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int16_t v, size_t c));
void mac68k_bssm4 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int32_t v, size_t c));
void mac68k_bssm4_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int32_t v, size_t c));
void mac68k_bssm4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int32_t v, size_t c));

#if defined(DIAGNOSTIC)
#define	bus_space_set_multi_1(t, h, o, val, c) do {			\
	if (!c) panic("bus_space_set_multi_1 called with zero count."); \
	(h).bssm1(t,&(h),o,val,c); } while (0)
#define	bus_space_set_multi_2(t, h, o, val, c) do {			\
	if (!c) panic("bus_space_set_multi_2 called with zero count."); \
	(h).bssm2(t,&(h),o,val,c); } while (0)
#define	bus_space_set_multi_4(t, h, o, val, c) do {			\
	if (!c) panic("bus_space_set_multi_4 called with zero count."); \
	(h).bssm4(t,&(h),o,val,c); } while (0)
#else
#define	bus_space_set_multi_1(t, h, o, val, c) \
	do { if (c) (h).bssm1(t,&(h),o,val,c); } while (0)
#define	bus_space_set_multi_2(t, h, o, val, c) \
	do { if (c) (h).bssm2(t,&(h),o,val,c); } while (0)
#define	bus_space_set_multi_4(t, h, o, val, c) \
	do { if (c) (h).bssm4(t,&(h),o,val,c); } while (0)
#endif

#if 0	/* Cause a link error for bus_space_set_multi_8 */
#define	bus_space_set_multi_8						\
			!!! bus_space_set_multi_8 unimplemented !!!
#endif

/*
 *	void bus_space_set_region_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

void mac68k_bssr1 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int8_t v, size_t c));
void mac68k_bssr1_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int8_t v, size_t c));
void mac68k_bssr2 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int16_t v, size_t c));
void mac68k_bssr2_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int16_t v, size_t c));
void mac68k_bssr2_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int16_t v, size_t c));
void mac68k_bssr4 __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int32_t v, size_t c));
void mac68k_bssr4_swap __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int32_t v, size_t c));
void mac68k_bssr4_gen __P((bus_space_tag_t t, bus_space_handle_t *h,
			   bus_size_t o, u_int32_t v, size_t c));

#if defined(DIAGNOSTIC)
#define	bus_space_set_region_1(t, h, o, val, c) do {			 \
	if (!c) panic("bus_space_set_region_1 called with zero count."); \
	(h).bssr1(t,&(h),o,val,c); } while (0)
#define	bus_space_set_region_2(t, h, o, val, c) do {			 \
	if (!c) panic("bus_space_set_region_2 called with zero count."); \
	(h).bssr2(t,&(h),o,val,c); } while (0)
#define	bus_space_set_region_4(t, h, o, val, c) do {			 \
	if (!c) panic("bus_space_set_region_4 called with zero count."); \
	(h).bssr4(t,&(h),o,val,c); } while (0)
#else
#define	bus_space_set_region_1(t, h, o, val, c) \
	do { if (c) (h).bssr1(t,&(h),o,val,c); } while (0)
#define	bus_space_set_region_2(t, h, o, val, c) \
	do { if (c) (h).bssr2(t,&(h),o,val,c); } while (0)
#define	bus_space_set_region_4(t, h, o, val, c) \
	do { if (c) (h).bssr4(t,&(h),o,val,c); } while (0)
#endif

#if 0	/* Cause a link error for bus_space_set_region_8 */
#define	bus_space_set_region_8						\
			!!! bus_space_set_region_8 unimplemented !!!
#endif

/*
 *	void bus_space_copy_N __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count));
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

#define	__MAC68K_copy_region_N(BYTES)					\
static __inline void __CONCAT(bus_space_copy_region_,BYTES)		\
	__P((bus_space_tag_t,						\
	    bus_space_handle_t bsh1, bus_size_t off1,			\
	    bus_space_handle_t bsh2, bus_size_t off2,			\
	    bus_size_t count));						\
									\
static __inline void							\
__CONCAT(bus_space_copy_region_,BYTES)(t, h1, o1, h2, o2, c)		\
	bus_space_tag_t t;						\
	bus_space_handle_t h1, h2;					\
	bus_size_t o1, o2, c;						\
{									\
	bus_size_t o;							\
									\
	if ((h1.base + o1) >= (h2.base + o2)) {				\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	}								\
}
__MAC68K_copy_region_N(1)
__MAC68K_copy_region_N(2)
__MAC68K_copy_region_N(4)
#if 0	/* Cause a link error for bus_space_copy_8 */
#define	bus_space_copy_8						\
			!!! bus_space_copy_8 unimplemented !!!
#endif

#undef __MAC68K_copy_region_N

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier __P((bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags));
 *
 * Note: the 680x0 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

#endif /* _MAC68K_BUS_H_ */
