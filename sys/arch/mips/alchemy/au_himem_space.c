/* $NetBSD: au_himem_space.c,v 1.9 2009/11/07 07:27:45 cegger Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: au_himem_space.c,v 1.9 2009/11/07 07:27:45 cegger Exp $");

/*
 * This provides mappings for the upper I/O regions used on some
 * Alchemy parts, e.g. PCI, PCMCIA, and LCD.  The mappings do not use
 * wired TLB entries, but instead rely on wiring entries in the kernel
 * pmap.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/endian.h>
#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/locore.h>
#include <mips/alchemy/include/au_himem_space.h>

#define	TRUNC_PAGE(x)	((x) & ~(PAGE_SIZE - 1))
#define	ROUND_PAGE(x)	TRUNC_PAGE((x) + (PAGE_SIZE - 1))

typedef struct au_himem_cookie {
	const char	*c_name;
	bus_addr_t	c_start;
	bus_addr_t	c_end;
	paddr_t		c_physoff;
	int		c_flags;
	int		c_swswap;
	bool		c_hwswap;
	struct extent	*c_extent;
} au_himem_cookie_t;

int au_himem_map(void *, bus_addr_t, bus_size_t, int,
    bus_space_handle_t *, int);	
void au_himem_unmap(void *, bus_space_handle_t, bus_size_t, int);
void *au_himem_vaddr(void *, bus_space_handle_t);
int au_himem_subregion(void *, bus_space_handle_t, bus_size_t, bus_size_t,
    bus_space_handle_t *);
paddr_t au_himem_mmap(void *, bus_addr_t, off_t, int, int);
int au_himem_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t,
    bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void au_himem_free(void *, bus_space_handle_t, bus_size_t);
void au_himem_barrier(void *, bus_space_handle_t, bus_size_t, bus_size_t, int);
uint8_t au_himem_r_1(void *, bus_space_handle_t, bus_size_t);
uint16_t au_himem_r_2(void *, bus_space_handle_t, bus_size_t);
uint32_t au_himem_r_4(void *, bus_space_handle_t, bus_size_t);
uint64_t au_himem_r_8(void *, bus_space_handle_t, bus_size_t);
void au_himem_rm_1(void *, bus_space_handle_t, bus_size_t, uint8_t *,
    bus_size_t);
void au_himem_rm_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_himem_rm_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_himem_rm_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_himem_rr_1(void *, bus_space_handle_t, bus_size_t, uint8_t *,
    bus_size_t);
void au_himem_rr_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_himem_rr_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_himem_rr_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_himem_w_1(void *, bus_space_handle_t, bus_size_t, uint8_t);
void au_himem_w_2(void *, bus_space_handle_t, bus_size_t, uint16_t);
void au_himem_w_4(void *, bus_space_handle_t, bus_size_t, uint32_t);
void au_himem_w_8(void *, bus_space_handle_t, bus_size_t, uint64_t);
void au_himem_wm_1(void *, bus_space_handle_t, bus_size_t, const uint8_t *,
    bus_size_t);
void au_himem_wm_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_himem_wm_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_himem_wm_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);
void au_himem_wr_1(void *, bus_space_handle_t, bus_size_t, const uint8_t *,
    bus_size_t);
void au_himem_wr_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_himem_wr_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_himem_wr_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);
void au_himem_sm_1(void *, bus_space_handle_t, bus_size_t, uint8_t,
    bus_size_t);
void au_himem_sm_2(void *, bus_space_handle_t, bus_size_t, uint16_t,
    bus_size_t);
void au_himem_sm_4(void *, bus_space_handle_t, bus_size_t, uint32_t,
    bus_size_t);
void au_himem_sm_8(void *, bus_space_handle_t, bus_size_t, uint64_t,
    bus_size_t);
void au_himem_sr_1(void *, bus_space_handle_t, bus_size_t, uint8_t,
    bus_size_t);
void au_himem_sr_2(void *, bus_space_handle_t, bus_size_t, uint16_t,
    bus_size_t);
void au_himem_sr_4(void *, bus_space_handle_t, bus_size_t, uint32_t,
    bus_size_t);
void au_himem_sr_8(void *, bus_space_handle_t, bus_size_t, uint64_t,
    bus_size_t);
void au_himem_c_1(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
void au_himem_c_2(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
void au_himem_c_4(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
void au_himem_c_8(void *, bus_space_handle_t, bus_size_t,
    bus_space_handle_t, bus_size_t, bus_size_t);
uint16_t au_himem_rs_2(void *, bus_space_handle_t, bus_size_t);
uint32_t au_himem_rs_4(void *, bus_space_handle_t, bus_size_t);
uint64_t au_himem_rs_8(void *, bus_space_handle_t, bus_size_t);
void au_himem_ws_2(void *, bus_space_handle_t, bus_size_t, uint16_t);
void au_himem_ws_4(void *, bus_space_handle_t, bus_size_t, uint32_t);
void au_himem_ws_8(void *, bus_space_handle_t, bus_size_t, uint64_t);
void au_himem_rms_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_himem_rms_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_himem_rms_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_himem_rrs_2(void *, bus_space_handle_t, bus_size_t, uint16_t *,
    bus_size_t);
void au_himem_rrs_4(void *, bus_space_handle_t, bus_size_t, uint32_t *,
    bus_size_t);
void au_himem_rrs_8(void *, bus_space_handle_t, bus_size_t, uint64_t *,
    bus_size_t);
void au_himem_wms_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_himem_wms_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_himem_wms_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);
void au_himem_wrs_2(void *, bus_space_handle_t, bus_size_t, const uint16_t *,
    bus_size_t);
void au_himem_wrs_4(void *, bus_space_handle_t, bus_size_t, const uint32_t *,
    bus_size_t);
void au_himem_wrs_8(void *, bus_space_handle_t, bus_size_t, const uint64_t *,
    bus_size_t);

int
au_himem_map(void *cookie, bus_addr_t addr, bus_size_t size,
    int flags, bus_space_handle_t *bshp, int acct)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)cookie;
	int			err;
	paddr_t			pa;
	vaddr_t			va;
	vsize_t			realsz;
	int			s;

	/* make sure we can map this bus address */
	if (addr < c->c_start || (addr + size) > c->c_end) {
		return EINVAL;
	}

	/* physical address, page aligned */
	pa = TRUNC_PAGE(c->c_physoff + addr);

	/*
	 * we are only going to work with whole pages.  the
	 * calculation is the offset into the first page, plus the
	 * intended size, rounded up to a whole number of pages.
	 */
	realsz = ROUND_PAGE((addr % PAGE_SIZE) + size);

	va = uvm_km_alloc(kernel_map,
	    realsz, PAGE_SIZE, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0) {
		return ENOMEM;
	}

	/* virtual address in handle (offset appropriately) */
	*bshp = va + (addr % PAGE_SIZE);

	/* map the pages in the kernel pmap */
	s = splhigh();
	while (realsz) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
		pa += PAGE_SIZE;
		va += PAGE_SIZE;
		realsz -= PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	splx(s);

	/* record our allocated range of bus addresses */
	if (acct && c->c_extent != NULL) {
		err = extent_alloc_region(c->c_extent, addr, size, EX_NOWAIT);
		if (err) {
			au_himem_unmap(cookie, *bshp, size, 0);
			return err;
		}
	}

	return 0;
}

void
au_himem_unmap(void *cookie, bus_space_handle_t bsh, bus_size_t size, int acct)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)cookie;
	vaddr_t			va;
	vsize_t			realsz;
	paddr_t			pa;
	int			s;

	va = (vaddr_t)TRUNC_PAGE(bsh);
	realsz = (vsize_t)ROUND_PAGE((bsh % PAGE_SIZE) + size);

	s = splhigh();

	/* make sure that any pending writes are flushed */
	wbflush();

	/*
	 * we have to get the bus address, so that we can free it in the
	 * extent manager.  this is the unfortunate thing about using
	 * virtual memory instead of just a 1:1 mapping scheme.
	 */
	if (pmap_extract(pmap_kernel(), va, &pa) == false)
		panic("au_himem_unmap: virtual address invalid!");

	/* now remove it from the pmap */
	pmap_kremove(va, realsz);
	pmap_update(pmap_kernel());
	splx(s);

	/* finally we can release both virtual and bus address ranges */
	uvm_km_free(kernel_map, va, realsz, UVM_KMF_VAONLY);

	if (acct) {
		bus_addr_t		addr;
		addr = ((pa - c->c_physoff) + (bsh % PAGE_SIZE));
		extent_free(c->c_extent, addr, size, EX_NOWAIT);
	}
}

int
au_himem_subregion(void *cookie, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;
	return 0;
}

void *
au_himem_vaddr(void *cookie, bus_space_handle_t bsh)
{

	return ((void *)bsh);
}

paddr_t
au_himem_mmap(void *cookie, bus_addr_t addr, off_t off, int prot, int flags)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)cookie;

	/* I/O spaces should not be directly mmap'ed */
	if (c->c_flags & AU_HIMEM_SPACE_IO)
		return -1;

	if (addr < c->c_start || (addr + off) >= c->c_end)
		return -1;

	return mips_btop(c->c_physoff + addr + off);
}

int
au_himem_alloc(void *cookie, bus_addr_t start, bus_addr_t end,
    bus_size_t size, bus_size_t align, bus_size_t boundary, int flags,
    bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)cookie;
	int			err;

	err = extent_alloc_subregion(c->c_extent, start, end, size,
	    align, boundary, EX_FAST | EX_NOWAIT, addrp);
	if (err) {
		return err;
	}
	err = au_himem_map(cookie, *addrp, size, flags, bshp, 0);
	if (err)
		extent_free(c->c_extent, *addrp, size, EX_NOWAIT);
	return err;
}

void
au_himem_free(void *cookie, bus_space_handle_t bsh, bus_size_t size)
{

	/* unmap takes care of it all */
	au_himem_unmap(cookie, bsh, size, 1);
}

inline void
au_himem_barrier(void *cookie, bus_space_handle_t bsh, bus_size_t o,
    bus_size_t l, int f)
{

	if (f & BUS_SPACE_BARRIER_WRITE)
		wbflush();
}

inline uint8_t
au_himem_r_1(void *v, bus_space_handle_t h, bus_size_t o)
{
	wbflush();
	return (*(volatile uint8_t *)(h + o));
}

inline uint16_t
au_himem_r_2(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint16_t		val;
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	wbflush();
	val = (*(volatile uint16_t *)(h + o));
	return (c->c_swswap ? bswap16(val) : val);
}

inline uint32_t
au_himem_r_4(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint32_t		val;
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	wbflush();
	val = (*(volatile uint32_t *)(h + o));
	return (c->c_swswap ? bswap32(val) : val);
}

inline uint64_t
au_himem_r_8(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint64_t		val;
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	wbflush();
	val = (*(volatile uint64_t *)(h + o));
	return (c->c_swswap ? bswap64(val) : val);
}

inline void
au_himem_w_1(void *v, bus_space_handle_t h, bus_size_t o, uint8_t val)
{

	*(volatile uint8_t *)(h + o) = val;
	wbflush();
}

inline void
au_himem_w_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t val)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	*(volatile uint16_t *)(h + o) = c->c_swswap ? bswap16(val) : val;
	wbflush();
}

inline void
au_himem_w_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t val)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	*(volatile uint32_t *)(h + o) = c->c_swswap ? bswap32(val) : val;
	wbflush();
}

inline void
au_himem_w_8(void *v, bus_space_handle_t h, bus_size_t o, uint64_t val)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	*(volatile uint64_t *)(h + o) = c->c_swswap ? bswap64(val) : val;
	wbflush();
}

inline uint16_t
au_himem_rs_2(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint16_t		val;
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	wbflush();
	val = (*(volatile uint16_t *)(h + o));
	return (c->c_hwswap ? bswap16(val) : val);
}

inline uint32_t
au_himem_rs_4(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint32_t		val;
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	wbflush();
	val = (*(volatile uint32_t *)(h + o));
	return (c->c_hwswap ? bswap32(val) : val);
}

inline uint64_t
au_himem_rs_8(void *v, bus_space_handle_t h, bus_size_t o)
{
	uint64_t		val;
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	wbflush();
	val = (*(volatile uint64_t *)(h + o));
	return (c->c_hwswap ? bswap64(val) : val);
}

inline void
au_himem_ws_2(void *v, bus_space_handle_t h, bus_size_t o, uint16_t val)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	*(volatile uint16_t *)(h + o) = c->c_hwswap ? bswap16(val) : val;
	wbflush();
}

inline void
au_himem_ws_4(void *v, bus_space_handle_t h, bus_size_t o, uint32_t val)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	*(volatile uint32_t *)(h + o) = c->c_hwswap ? bswap32(val) : val;
	wbflush();
}

inline void
au_himem_ws_8(void *v, bus_space_handle_t h, bus_size_t o, uint64_t val)
{
	au_himem_cookie_t	*c = (au_himem_cookie_t *)v;

	*(volatile uint64_t *)(h + o) = c->c_hwswap ? bswap64(val) : val;
	wbflush();
}

#define	AU_HIMEM_RM(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_rm_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0)						\
		*dst ++ = __CONCAT(au_himem_r_,BYTES)(v, h, o);		\
}
AU_HIMEM_RM(uint8_t,1)
AU_HIMEM_RM(uint16_t,2)
AU_HIMEM_RM(uint32_t,4)
AU_HIMEM_RM(uint64_t,8)

#define	AU_HIMEM_RMS(TYPE,BYTES)					\
void									\
__CONCAT(au_himem_rms_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0) {						\
		*dst++ = __CONCAT(au_himem_rs_,BYTES)(v, h, o);		\
	}								\
}
AU_HIMEM_RMS(uint16_t,2)
AU_HIMEM_RMS(uint32_t,4)
AU_HIMEM_RMS(uint64_t,8)

#define AU_HIMEM_RR(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_rr_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0) {						\
		*dst++ = __CONCAT(au_himem_r_,BYTES)(v, h, o);		\
		o += BYTES;						\
	}								\
}
AU_HIMEM_RR(uint8_t,1)
AU_HIMEM_RR(uint16_t,2)
AU_HIMEM_RR(uint32_t,4)
AU_HIMEM_RR(uint64_t,8)

#define AU_HIMEM_RRS(TYPE,BYTES)					\
void									\
__CONCAT(au_himem_rrs_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE *dst, bus_size_t cnt)	\
{									\
									\
	while (cnt-- > 0) {						\
		*dst++ = __CONCAT(au_himem_rs_,BYTES)(v, h, o);		\
		o += BYTES;						\
	}								\
}
AU_HIMEM_RRS(uint16_t,2)
AU_HIMEM_RRS(uint32_t,4)
AU_HIMEM_RRS(uint64_t,8)

#define	AU_HIMEM_WM(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_wm_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_himem_w_,BYTES)(v, h, o, *src++);		\
	}								\
}
AU_HIMEM_WM(uint8_t,1)
AU_HIMEM_WM(uint16_t,2)
AU_HIMEM_WM(uint32_t,4)
AU_HIMEM_WM(uint64_t,8)

#define	AU_HIMEM_WMS(TYPE,BYTES)					\
void									\
__CONCAT(au_himem_wms_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_himem_ws_,BYTES)(v, h, o, *src++);		\
	}								\
}
AU_HIMEM_WMS(uint16_t,2)
AU_HIMEM_WMS(uint32_t,4)
AU_HIMEM_WMS(uint64_t,8)

#define	AU_HIMEM_WR(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_wr_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_himem_w_,BYTES)(v, h, o, *src++);		\
		o += BYTES;						\
	}								\
}
AU_HIMEM_WR(uint8_t,1)
AU_HIMEM_WR(uint16_t,2)
AU_HIMEM_WR(uint32_t,4)
AU_HIMEM_WR(uint64_t,8)

#define	AU_HIMEM_WRS(TYPE,BYTES)					\
void									\
__CONCAT(au_himem_wrs_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, const TYPE *src,		\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_himem_ws_,BYTES)(v, h, o, *src++);		\
		o += BYTES;						\
	}								\
}
AU_HIMEM_WRS(uint16_t,2)
AU_HIMEM_WRS(uint32_t,4)
AU_HIMEM_WRS(uint64_t,8)

#define	AU_HIMEM_SM(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_sm_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE val,			\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_himem_w_,BYTES)(v, h, o, val);		\
	}								\
}
AU_HIMEM_SM(uint8_t,1)
AU_HIMEM_SM(uint16_t,2)
AU_HIMEM_SM(uint32_t,4)
AU_HIMEM_SM(uint64_t,8)

#define	AU_HIMEM_SR(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_sr_,BYTES)(void *v,					\
    bus_space_handle_t h, bus_size_t o, TYPE val,			\
    bus_size_t cnt)							\
{									\
									\
	while (cnt-- > 0) {						\
		__CONCAT(au_himem_w_,BYTES)(v, h, o, val);		\
		o += BYTES;						\
	}								\
}
AU_HIMEM_SR(uint8_t,1)
AU_HIMEM_SR(uint16_t,2)
AU_HIMEM_SR(uint32_t,4)
AU_HIMEM_SR(uint64_t,8)


#define	AU_HIMEM_C(TYPE,BYTES)						\
void									\
__CONCAT(au_himem_c_,BYTES)(void *v,					\
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
AU_HIMEM_C(uint8_t,1)
AU_HIMEM_C(uint16_t,2)
AU_HIMEM_C(uint32_t,4)
AU_HIMEM_C(uint64_t,8)


void
au_himem_space_init(bus_space_tag_t bst, const char *name,
    paddr_t physoff, bus_addr_t start, bus_addr_t end, int flags)
{
	au_himem_cookie_t	*c;

	c = malloc(sizeof (struct au_himem_cookie), M_DEVBUF,
	    M_NOWAIT | M_ZERO);

	c->c_name = name;
	c->c_start = start;
	c->c_end = end;
	c->c_physoff = physoff;

	/* allocate extent manager */
	c->c_extent = extent_create(name, start, end, M_DEVBUF,
	    NULL, 0, EX_NOWAIT);
	if (c->c_extent == NULL)
		panic("au_himem_space_init: %s: cannot create extent", name);

#if	_BYTE_ORDER == _BIG_ENDIAN
	if (flags & AU_HIMEM_SPACE_LITTLE_ENDIAN) {
		if (flags & AU_HIMEM_SPACE_SWAP_HW)
			c->c_hwswap = 1;
		else
			c->c_swswap = 1;
	}

#elif	_BYTE_ORDER == _LITTLE_ENDIAN
	if (flags & AU_HIMEM_SPACE_BIG_ENDIAN) {
		if (flags & AU_HIMEM_SPACE_SWAP_HW)
			c->c_hwswap = 1;
		else
			c->c_swswap = 1;
	}
#endif

	bst->bs_cookie = c;
	bst->bs_map = au_himem_map;
	bst->bs_unmap = au_himem_unmap;
	bst->bs_subregion = au_himem_subregion;
	bst->bs_translate = NULL;	/* we don't use these */
	bst->bs_get_window = NULL;	/* we don't use these */
	bst->bs_alloc = au_himem_alloc;
	bst->bs_free = au_himem_free;
	bst->bs_vaddr = au_himem_vaddr;
	bst->bs_mmap = au_himem_mmap;
	bst->bs_barrier = au_himem_barrier;
	bst->bs_r_1 = au_himem_r_1;
	bst->bs_w_1 = au_himem_w_1;
	bst->bs_r_2 = au_himem_r_2;
	bst->bs_r_4 = au_himem_r_4;
	bst->bs_r_8 = au_himem_r_8;
	bst->bs_w_2 = au_himem_w_2;
	bst->bs_w_4 = au_himem_w_4;
	bst->bs_w_8 = au_himem_w_8;
	bst->bs_rm_1 = au_himem_rm_1;
	bst->bs_rm_2 = au_himem_rm_2;
	bst->bs_rm_4 = au_himem_rm_4;
	bst->bs_rm_8 = au_himem_rm_8;
	bst->bs_rr_1 = au_himem_rr_1;
	bst->bs_rr_2 = au_himem_rr_2;
	bst->bs_rr_4 = au_himem_rr_4;
	bst->bs_rr_8 = au_himem_rr_8;
	bst->bs_wm_1 = au_himem_wm_1;
	bst->bs_wm_2 = au_himem_wm_2;
	bst->bs_wm_4 = au_himem_wm_4;
	bst->bs_wm_8 = au_himem_wm_8;
	bst->bs_wr_1 = au_himem_wr_1;
	bst->bs_wr_2 = au_himem_wr_2;
	bst->bs_wr_4 = au_himem_wr_4;
	bst->bs_wr_8 = au_himem_wr_8;
	bst->bs_sm_1 = au_himem_sm_1;
	bst->bs_sm_2 = au_himem_sm_2;
	bst->bs_sm_4 = au_himem_sm_4;
	bst->bs_sm_8 = au_himem_sm_8;
	bst->bs_sr_1 = au_himem_sr_1;
	bst->bs_sr_2 = au_himem_sr_2;
	bst->bs_sr_4 = au_himem_sr_4;
	bst->bs_sr_8 = au_himem_sr_8;
	bst->bs_c_1 = au_himem_c_1;
	bst->bs_c_2 = au_himem_c_2;
	bst->bs_c_4 = au_himem_c_4;
	bst->bs_c_8 = au_himem_c_8;

	bst->bs_rs_1 = au_himem_r_1;
	bst->bs_rs_2 = au_himem_rs_2;
	bst->bs_rs_4 = au_himem_rs_4;
	bst->bs_rs_8 = au_himem_rs_8;
	bst->bs_rms_1 = au_himem_rm_1;
	bst->bs_rms_2 = au_himem_rms_2;
	bst->bs_rms_4 = au_himem_rms_4;
	bst->bs_rms_8 = au_himem_rms_8;
	bst->bs_rrs_1 = au_himem_rr_1;
	bst->bs_rrs_2 = au_himem_rrs_2;
	bst->bs_rrs_4 = au_himem_rrs_4;
	bst->bs_rrs_8 = au_himem_rrs_8;
	bst->bs_ws_1 = au_himem_w_1;
	bst->bs_ws_2 = au_himem_ws_2;
	bst->bs_ws_4 = au_himem_ws_4;
	bst->bs_ws_8 = au_himem_ws_8;
	bst->bs_wms_1 = au_himem_wm_1;
	bst->bs_wms_2 = au_himem_wms_2;
	bst->bs_wms_4 = au_himem_wms_4;
	bst->bs_wms_8 = au_himem_wms_8;
	bst->bs_wrs_1 = au_himem_wr_1;
	bst->bs_wrs_2 = au_himem_wrs_2;
	bst->bs_wrs_4 = au_himem_wrs_4;
	bst->bs_wrs_8 = au_himem_wrs_8;
}
