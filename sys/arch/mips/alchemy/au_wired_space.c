/* $NetBSD: au_wired_space.c,v 1.3.4.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 
/*
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: au_wired_space.c,v 1.3.4.2 2006/04/22 11:37:41 simonb Exp $");

/*
 * This provides mappings for the upper I/O regions used on some
 * Alchemy parts, e.g. PCI and PCMCIA spaces.  These spaces can be
 * accessed using wired TLB entries.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/endian.h>

#include <machine/bus.h>
#include <machine/locore.h>
#include <machine/wired_map.h>
#include <mips/alchemy/include/au_wired_space.h>

#ifndef	AU_WIRED_EXTENT_SZ
#define	AU_WIRED_EXTENT_SZ	EXTENT_FIXED_STORAGE_SIZE(10)
#endif

typedef struct au_wired_cookie {
	const char	*c_name;
	bus_addr_t	c_start;
	bus_size_t	c_size;
	paddr_t		c_pbase;
	int		c_flags;
	int		c_swswap;
	boolean_t	c_hwswap;
	struct extent	*c_extent;
	long		c_exstore[AU_WIRED_EXTENT_SZ/sizeof (long)];
} au_wired_cookie_t;

int au_wired_map(void *, bus_addr_t, bus_size_t, int,
    bus_space_handle_t *, int);
void au_wired_unmap(void *, bus_space_handle_t, bus_size_t, int);
void *au_wired_vaddr(void *, bus_space_handle_t);
int au_wired_subregion(void *, bus_space_handle_t, bus_size_t, bus_size_t,
    bus_space_handle_t *);
paddr_t au_wired_mmap(void *, bus_addr_t, off_t, int, int);
int au_wired_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t,
    bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void au_wired_free(void *, bus_space_handle_t, bus_size_t);
void au_wired_barrier(void *, bus_space_handle_t, bus_size_t, bus_size_t, int);
uint8_t au_wired_r_1(void *, bus_space_handle_t, bus_size_t);
uint16_t au_wired_r_2(void *, bus_space_handle_t, bus_size_t);
uint32_t au_wired_r_4(void *, bus_space_handle_t, bus_size_t);
uint64_t au_wired_r_8(void *, bus_space_handle_t, bus_size_t);
void au_wired_rm_1(void *, bus_space_handle_t, bus_size_t, uint8_t *,
    bus_size_t);
void au_wired_rm_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_wired_rm_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_wired_rm_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_wired_rr_1(void *, bus_space_handle_t, bus_size_t, uint8_t *,
    bus_size_t);
void au_wired_rr_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_wired_rr_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_wired_rr_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_wired_w_1(void *, bus_space_handle_t, bus_size_t, uint8_t);
void au_wired_w_2(void *, bus_space_handle_t, bus_size_t, uint16_t);
void au_wired_w_4(void *, bus_space_handle_t, bus_size_t, uint32_t);
void au_wired_w_8(void *, bus_space_handle_t, bus_size_t, uint64_t);
void au_wired_wm_1(void *, bus_space_handle_t, bus_size_t, const uint8_t *,
    bus_size_t);
void au_wired_wm_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_wired_wm_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_wired_wm_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);
void au_wired_wr_1(void *, bus_space_handle_t, bus_size_t, const uint8_t *,
    bus_size_t);
void au_wired_wr_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_wired_wr_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_wired_wr_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);
void au_wired_sm_1(void *, bus_space_handle_t, bus_size_t, uint8_t,
    bus_size_t);
void au_wired_sm_2(void *, bus_space_handle_t, bus_size_t, uint16_t,
    bus_size_t);
void au_wired_sm_4(void *, bus_space_handle_t, bus_size_t, uint32_t,
    bus_size_t);
void au_wired_sm_8(void *, bus_space_handle_t, bus_size_t, uint64_t,
    bus_size_t);
void au_wired_sr_1(void *, bus_space_handle_t, bus_size_t, uint8_t,
    bus_size_t);
void au_wired_sr_2(void *, bus_space_handle_t, bus_size_t, uint16_t,
    bus_size_t);
void au_wired_sr_4(void *, bus_space_handle_t, bus_size_t, uint32_t,
    bus_size_t);
void au_wired_sr_8(void *, bus_space_handle_t, bus_size_t, uint64_t,
    bus_size_t);
void au_wired_c_1(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
void au_wired_c_2(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
void au_wired_c_4(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
void au_wired_c_8(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
uint16_t au_wired_rs_2(void *, bus_space_handle_t, bus_size_t);
uint32_t au_wired_rs_4(void *, bus_space_handle_t, bus_size_t);
uint64_t au_wired_rs_8(void *, bus_space_handle_t, bus_size_t);
void au_wired_ws_2(void *, bus_space_handle_t, bus_size_t, uint16_t);
void au_wired_ws_4(void *, bus_space_handle_t, bus_size_t, uint32_t);
void au_wired_ws_8(void *, bus_space_handle_t, bus_size_t, uint64_t);
void au_wired_rms_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_wired_rms_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_wired_rms_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_wired_rrs_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_wired_rrs_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_wired_rrs_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_wired_wms_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_wired_wms_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_wired_wms_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);
void au_wired_wrs_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_wired_wrs_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_wired_wrs_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);

int
au_wired_map(void *cookie, bus_addr_t addr, bus_size_t size,
    int flags, bus_space_handle_t *bshp, int acct)
{
	int			err;
	au_wired_cookie_t	*c = (au_wired_cookie_t *)cookie;
	paddr_t			pa;

	/* make sure we can map this bus address */
	if (addr < c->c_start ||
	    addr + size > c->c_start + c->c_size)
		return EINVAL;

	pa = c->c_pbase + (addr - c->c_start);

	if (!mips3_wired_enter_region(addr, pa, size))
		return ENOMEM;

	/*
	 * bus addresses are taken from virtual address space.
	 */
	if (acct && c->c_extent != NULL) {
		err = extent_alloc_region(c->c_extent, addr, size, EX_NOWAIT);
		if (err)
			return err;
	}

	*bshp = addr;

	return 0;
}

void
au_wired_unmap(void *cookie, bus_space_handle_t bsh, bus_size_t size, int acct)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)cookie;

	if (acct != 0 && c->c_extent != NULL) {
		extent_free(c->c_extent, (vaddr_t)bsh, size, EX_NOWAIT);
	}
}

int
au_wired_subregion(void *cookie, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

void *
au_wired_vaddr(void *cookie, bus_space_handle_t bsh)
{

	return ((void *)bsh);
}

paddr_t
au_wired_mmap(void *cookie, bus_addr_t addr, off_t off, int prot, int flags)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)cookie;

	/* I/O spaces should not be directly mmap'ed */
	if (c->c_flags & AU_WIRED_SPACE_IO)
		return -1;

	if (addr < c->c_start || (addr + off) >= (c->c_start + c->c_size))
		return -1;

	return mips_btop(c->c_pbase + (addr - c->c_start) + off);
}

int
au_wired_alloc(void *cookie, bus_addr_t start, bus_addr_t end,
    bus_size_t size, bus_size_t align, bus_size_t boundary, int flags,
    bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)cookie;
	vaddr_t addr;
	int err;
	paddr_t pa;

	if (c->c_extent == NULL)
		panic("au_wired_alloc: extent map %s not avail", c->c_name);

	if (start < c->c_start || ((start + size) > (c->c_start + c->c_size)))
		return EINVAL;

	err = extent_alloc_subregion(c->c_extent, start, end, size,
	    align, boundary, EX_FAST | EX_NOWAIT, &addr);
	if (err)
		return err;

	pa = c->c_pbase + (addr - c->c_start);

	if (!mips3_wired_enter_region(addr, pa, size))
		return ENOMEM;

	*bshp = addr;
	*addrp = addr;
	return 0;
}

void
au_wired_free(void *cookie, bus_space_handle_t bsh, bus_size_t size)
{

	/* unmap takes care of it all */
	au_wired_unmap(cookie, bsh, size, 1);
}

inline void
au_wired_barrier(void *cookie, bus_space_handle_t bsh, bus_size_t o,
    bus_size_t l, int f)
{

	if (f & BUS_SPACE_BARRIER_WRITE)
		wbflush();
}

inline uint8_t
au_wired_r_1(void *v, bus_space_handle_t h, bus_size_t o)
{

	return (*(volatile uint8_t *)(h + o));
}

inline uint16_t
au_wired_r_2(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint16_t		val = (*(volatile uint16_t *)(h + o));
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	return (c->c_swswap ? bswap16(val) : val);
}

inline uint32_t
au_wired_r_4(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint32_t		val = (*(volatile uint32_t *)(h + o));
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	return (c->c_swswap ? bswap32(val) : val);
}

inline uint64_t
au_wired_r_8(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint64_t		val = (*(volatile uint64_t *)(h + o));
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	return (c->c_swswap ? bswap64(val) : val);
}

inline void
au_wired_w_1(void *v, bus_space_handle_t h, bus_size_t o, uint8_t val)
{

	*(volatile uint8_t *)(h + o) = val;
}

inline void
au_wired_w_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t val)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	*(volatile uint16_t *)(h + o) = c->c_swswap ? bswap16(val) : val;
}

inline void
au_wired_w_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t val)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	*(volatile uint32_t *)(h + o) = c->c_swswap ? bswap32(val) : val;
}

inline void
au_wired_w_8(void *v, bus_space_handle_t h, bus_size_t o, uint64_t val)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	*(volatile uint64_t *)(h + o) = c->c_swswap ? bswap64(val) : val;
}

inline uint16_t
au_wired_rs_2(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint16_t		val = (*(volatile uint16_t *)(h + o));
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	return (c->c_hwswap ? bswap16(val) : val);
}

inline uint32_t
au_wired_rs_4(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint32_t		val = (*(volatile uint32_t *)(h + o));
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	return (c->c_hwswap ? bswap32(val) : val);
}

inline uint64_t
au_wired_rs_8(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint64_t		val = (*(volatile uint64_t *)(h + o));
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	return (c->c_hwswap ? bswap64(val) : val);
}

inline void
au_wired_ws_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t val)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	*(volatile uint16_t *)(h + o) = c->c_hwswap ? bswap16(val) : val;
}

inline void
au_wired_ws_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t val)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	*(volatile uint32_t *)(h + o) = c->c_hwswap ? bswap32(val) : val;
}

inline void
au_wired_ws_8(void *v, bus_space_handle_t h, bus_size_t o, uint64_t val)
{
	au_wired_cookie_t	*c = (au_wired_cookie_t *)v;

	*(volatile uint64_t *)(h + o) = c->c_hwswap ? bswap64(val) : val;
}

#define	AU_WIRED_RM(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_rm_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0)						\
		*dst ++ = __CONCAT(au_wired_r_,BYTES)(v, h, o);		\
}
AU_WIRED_RM(uint8_t,1)
AU_WIRED_RM(uint16_t,2)
AU_WIRED_RM(uint32_t,4)
AU_WIRED_RM(uint64_t,8)

#define	AU_WIRED_RMS(TYPE,BYTES)					\
void									\
__CONCAT(au_wired_rms_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0) {						\
		wbflush();						\
		*dst++ = __CONCAT(au_wired_rs_,BYTES)(v, h, o);		\
	}								\
}
AU_WIRED_RMS(uint16_t,2)
AU_WIRED_RMS(uint32_t,4)
AU_WIRED_RMS(uint64_t,8)

#define AU_WIRED_RR(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_rr_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0) {						\
		*dst++ = __CONCAT(au_wired_r_,BYTES)(v, h, o);		\
		o += BYTES;						\
	}								\
}
AU_WIRED_RR(uint8_t,1)
AU_WIRED_RR(uint16_t,2)
AU_WIRED_RR(uint32_t,4)
AU_WIRED_RR(uint64_t,8)

#define AU_WIRED_RRS(TYPE,BYTES)					\
void									\
__CONCAT(au_wired_rrs_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0) {						\
		*dst++ = __CONCAT(au_wired_rs_,BYTES)(v, h, o);		\
		o += BYTES;						\
	}								\
}
AU_WIRED_RRS(uint16_t,2)
AU_WIRED_RRS(uint32_t,4)
AU_WIRED_RRS(uint64_t,8)

#define	AU_WIRED_WM(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_wm_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_wired_w_,BYTES)(v, h, o, *src++);		\
		wbflush();						\
	}								\
}
AU_WIRED_WM(uint8_t,1)
AU_WIRED_WM(uint16_t,2)
AU_WIRED_WM(uint32_t,4)
AU_WIRED_WM(uint64_t,8)

#define	AU_WIRED_WMS(TYPE,BYTES)					\
void									\
__CONCAT(au_wired_wms_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_wired_ws_,BYTES)(v, h, o, *src++);		\
		wbflush();						\
	}								\
}
AU_WIRED_WMS(uint16_t,2)
AU_WIRED_WMS(uint32_t,4)
AU_WIRED_WMS(uint64_t,8)

#define	AU_WIRED_WR(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_wr_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_wired_w_,BYTES)(v, h, o, *src++);		\
		o += BYTES;						\
	}								\
}
AU_WIRED_WR(uint8_t,1)
AU_WIRED_WR(uint16_t,2)
AU_WIRED_WR(uint32_t,4)
AU_WIRED_WR(uint64_t,8)

#define	AU_WIRED_WRS(TYPE,BYTES)					\
void									\
__CONCAT(au_wired_wrs_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_wired_ws_,BYTES)(v, h, o, *src++);		\
		o += BYTES;						\
	}								\
}
AU_WIRED_WRS(uint16_t,2)
AU_WIRED_WRS(uint32_t,4)
AU_WIRED_WRS(uint64_t,8)

#define	AU_WIRED_SM(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_sm_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE val,			\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_wired_w_,BYTES)(v, h, o, val);		\
		wbflush();						\
	}								\
}
AU_WIRED_SM(uint8_t,1)
AU_WIRED_SM(uint16_t,2)
AU_WIRED_SM(uint32_t,4)
AU_WIRED_SM(uint64_t,8)

#define	AU_WIRED_SR(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_sr_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE val,			\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_wired_w_,BYTES)(v, h, o, val);		\
		o += BYTES;						\
	}								\
}
AU_WIRED_SR(uint8_t,1)
AU_WIRED_SR(uint16_t,2)
AU_WIRED_SR(uint32_t,4)
AU_WIRED_SR(uint64_t,8)


#define	AU_WIRED_C(TYPE,BYTES)						\
void									\
__CONCAT(au_wired_c_,BYTES)(void *v,					\
    bus_space_handle_t h1, bus_size_t o1, bus_space_handle_t h2,	\
    bus_space_handle_t o2, bus_size_t cnt)				\
{									\
	volatile TYPE *src, *dst;					\
	src = (volatile TYPE *)(h1 + o1);				\
	dst = (volatile TYPE *)(h2 + o2);				\
									\
	if (src >= dst) {						\
		while (cnt-- > 0)					\
			*dst++ = *src++;				\
	} else {							\
		src += cnt - 1;						\
		dst += cnt - 1;						\
		while (cnt-- > 0)					\
			*dst-- = *src--;				\
	}								\
}
AU_WIRED_C(uint8_t,1)
AU_WIRED_C(uint16_t,2)
AU_WIRED_C(uint32_t,4)
AU_WIRED_C(uint64_t,8)


void
au_wired_space_init(bus_space_tag_t bst, const char *name,
    paddr_t paddr, bus_addr_t start, bus_size_t size, int flags)
{
	au_wired_cookie_t	*c;

	c = malloc(sizeof (struct au_wired_cookie), M_DEVBUF,
	    M_NOWAIT | M_ZERO);

	c->c_pbase = paddr;
	c->c_name = name;
	c->c_start = start;
	c->c_size = size;

	/* allocate extent manager */
	c->c_extent = extent_create(name, start, start + size, M_DEVBUF,
	    (caddr_t)c->c_exstore, sizeof (c->c_exstore), EX_NOWAIT);
	if (c->c_extent == NULL)
		panic("au_wired_space_init: %s: cannot create extent", name);

#if	_BYTE_ORDER == _BIG_ENDIAN
	if (flags & AU_WIRED_SPACE_LITTLE_ENDIAN) {
		if (flags & AU_WIRED_SPACE_SWAP_HW)
			c->c_hwswap = 1;
		else
			c->c_swswap = 1;
	}

#elif	_BYTE_ORDER == _LITTLE_ENDIAN
	if (flags & AU_WIRED_SPACE_BIG_ENDIAN) {
		if (flags & AU_WIRED_SPACE_SWAP_HW)
			c->c_hwswap = 1;
		else
			c->c_swswap = 1;
	}
#endif

	bst->bs_cookie = c;
	bst->bs_map = au_wired_map;
	bst->bs_unmap = au_wired_unmap;
	bst->bs_subregion = au_wired_subregion;
	bst->bs_translate = NULL;	/* we don't use these */
	bst->bs_get_window = NULL;	/* we don't use these */
	bst->bs_alloc = au_wired_alloc;
	bst->bs_free = au_wired_free;
	bst->bs_vaddr = au_wired_vaddr;
	bst->bs_mmap = au_wired_mmap;
	bst->bs_barrier = au_wired_barrier;
	bst->bs_r_1 = au_wired_r_1;
	bst->bs_w_1 = au_wired_w_1;
	bst->bs_r_2 = au_wired_r_2;
	bst->bs_r_4 = au_wired_r_4;
	bst->bs_r_8 = au_wired_r_8;
	bst->bs_w_2 = au_wired_w_2;
	bst->bs_w_4 = au_wired_w_4;
	bst->bs_w_8 = au_wired_w_8;
	bst->bs_rm_1 = au_wired_rm_1;
	bst->bs_rm_2 = au_wired_rm_2;
	bst->bs_rm_4 = au_wired_rm_4;
	bst->bs_rm_8 = au_wired_rm_8;
	bst->bs_rr_1 = au_wired_rr_1;
	bst->bs_rr_2 = au_wired_rr_2;
	bst->bs_rr_4 = au_wired_rr_4;
	bst->bs_rr_8 = au_wired_rr_8;
	bst->bs_wm_1 = au_wired_wm_1;
	bst->bs_wm_2 = au_wired_wm_2;
	bst->bs_wm_4 = au_wired_wm_4;
	bst->bs_wm_8 = au_wired_wm_8;
	bst->bs_wr_1 = au_wired_wr_1;
	bst->bs_wr_2 = au_wired_wr_2;
	bst->bs_wr_4 = au_wired_wr_4;
	bst->bs_wr_8 = au_wired_wr_8;
	bst->bs_sm_1 = au_wired_sm_1;
	bst->bs_sm_2 = au_wired_sm_2;
	bst->bs_sm_4 = au_wired_sm_4;
	bst->bs_sm_8 = au_wired_sm_8;
	bst->bs_sr_1 = au_wired_sr_1;
	bst->bs_sr_2 = au_wired_sr_2;
	bst->bs_sr_4 = au_wired_sr_4;
	bst->bs_sr_8 = au_wired_sr_8;
	bst->bs_c_1 = au_wired_c_1;
	bst->bs_c_2 = au_wired_c_2;
	bst->bs_c_4 = au_wired_c_4;
	bst->bs_c_8 = au_wired_c_8;

	bst->bs_rs_1 = au_wired_r_1;
	bst->bs_rs_2 = au_wired_rs_2;
	bst->bs_rs_4 = au_wired_rs_4;
	bst->bs_rs_8 = au_wired_rs_8;
	bst->bs_rms_1 = au_wired_rm_1;
	bst->bs_rms_2 = au_wired_rms_2;
	bst->bs_rms_4 = au_wired_rms_4;
	bst->bs_rms_8 = au_wired_rms_8;
	bst->bs_rrs_1 = au_wired_rr_1;
	bst->bs_rrs_2 = au_wired_rrs_2;
	bst->bs_rrs_4 = au_wired_rrs_4;
	bst->bs_rrs_8 = au_wired_rrs_8;
	bst->bs_ws_1 = au_wired_w_1;
	bst->bs_ws_2 = au_wired_ws_2;
	bst->bs_ws_4 = au_wired_ws_4;
	bst->bs_ws_8 = au_wired_ws_8;
	bst->bs_wms_1 = au_wired_wm_1;
	bst->bs_wms_2 = au_wired_wms_2;
	bst->bs_wms_4 = au_wired_wms_4;
	bst->bs_wms_8 = au_wired_wms_8;
	bst->bs_wrs_1 = au_wired_wr_1;
	bst->bs_wrs_2 = au_wired_wrs_2;
	bst->bs_wrs_4 = au_wired_wrs_4;
	bst->bs_wrs_8 = au_wired_wrs_8;
}
