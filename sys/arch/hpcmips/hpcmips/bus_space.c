/*	$NetBSD: bus_space.c,v 1.21 2003/04/02 03:58:11 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <mips/cache.h>
#include <mips/pte.h>
#include <machine/bus.h>
#include <machine/bus_space_hpcmips.h>

#ifdef BUS_SPACE_DEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

#define MAX_BUSSPACE_TAG 10

/* proto types */
bus_space_handle_t __hpcmips_cacheable(struct bus_space_tag_hpcmips*,
    bus_addr_t, bus_size_t, int);
bus_space_protos(_);
bus_space_protos(bs_notimpl);

/* variables */
static  struct bus_space_tag_hpcmips __bus_space[MAX_BUSSPACE_TAG];
static int __bus_space_index;
static struct bus_space_tag_hpcmips __sys_bus_space = {
	{
		NULL,
		{
			/* mapping/unmapping */
			__bs_map,
			__bs_unmap,
			__bs_subregion,

			/* allocation/deallocation */
			__bs_alloc,
			__bs_free,

			/* get kernel virtual address */
			bs_notimpl_bs_vaddr, /* there is no linear mapping */

			/* Mmap bus space for user */
			bs_notimpl_bs_mmap,

			/* barrier */
			__bs_barrier,

			/* probe */
			__bs_peek,
			__bs_poke,

			/* read (single) */
			__bs_r_1,
			__bs_r_2,
			__bs_r_4,
			bs_notimpl_bs_r_8,

			/* read multiple */
			__bs_rm_1,
			__bs_rm_2,
			__bs_rm_4,
			bs_notimpl_bs_rm_8,

			/* read region */
			__bs_rr_1,
			__bs_rr_2,
			__bs_rr_4,
			bs_notimpl_bs_rr_8,

			/* write (single) */
			__bs_w_1,
			__bs_w_2,
			__bs_w_4,
			bs_notimpl_bs_w_8,

			/* write multiple */
			__bs_wm_1,
			__bs_wm_2,
			__bs_wm_4,
			bs_notimpl_bs_wm_8,

			/* write region */
			__bs_wr_1,
			__bs_wr_2,
			__bs_wr_4,
			bs_notimpl_bs_wr_8,

			/* set multi */
			__bs_sm_1,
			__bs_sm_2,
			__bs_sm_4,
			bs_notimpl_bs_sm_8,

			/* set region */
			__bs_sr_1,
			__bs_sr_2,
			__bs_sr_4,
			bs_notimpl_bs_sr_8,

			/* copy */
			__bs_c_1,
			__bs_c_2,
			__bs_c_4,
			bs_notimpl_bs_c_8,
		},
	},

	"whole bus space",	/* bus name */
	0,			/* extent base */
	0xffffffff,		/* extent size */
	NULL,			/* pointer for extent structure */
};
static bus_space_tag_t __sys_bus_space_tag = &__sys_bus_space.bst;

bus_space_tag_t
hpcmips_system_bus_space()
{

	return (__sys_bus_space_tag);
}

struct bus_space_tag_hpcmips *
hpcmips_system_bus_space_hpcmips()
{

	return (&__sys_bus_space);
}

struct bus_space_tag_hpcmips *
hpcmips_alloc_bus_space_tag()
{

	if (__bus_space_index >= MAX_BUSSPACE_TAG) {
		panic("hpcmips_internal_alloc_bus_space_tag: tag full.");
	}

	return (&__bus_space[__bus_space_index++]);
}

void
hpcmips_init_bus_space(struct bus_space_tag_hpcmips *t,
    struct bus_space_tag_hpcmips *basetag,
    char *name, u_int32_t base, u_int32_t size)
{
	u_int32_t pa, endpa;
	vaddr_t va;

	if (basetag != NULL)
		memcpy(t, basetag, sizeof(struct bus_space_tag_hpcmips));
	strncpy(t->name, name, sizeof(t->name));
	t->name[sizeof(t->name) - 1] = '\0';
	t->base = base;
	t->size = size;
	
	/* 
	 * If request physical address is greater than 512MByte,
	 * mapping it to kseg2.
	 */
	if (t->base >= 0x20000000) {
		pa = mips_trunc_page(t->base);
		endpa = mips_round_page(t->base + t->size);

		if (!(va = uvm_km_valloc(kernel_map, endpa - pa))) {
			panic("hpcmips_init_bus_space_extent:"
			    "can't allocate kernel virtual");
		}
		DPRINTF(("pa:0x%08x -> kv:0x%08x+0x%08x",
		    (unsigned int)t->base, (unsigned int)va, t->size));
		t->base = va; /* kseg2 addr */
				
		for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
			pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		}
		pmap_update(pmap_kernel());
	}

	t->extent = (void*)extent_create(t->name, t->base, 
	    t->base + t->size, M_DEVBUF,
	    0, 0, EX_NOWAIT);
	if (!t->extent) {
		panic("hpcmips_init_bus_space_extent:"
		    "unable to allocate %s map", t->name);
	}
}

bus_space_handle_t
__hpcmips_cacheable(struct bus_space_tag_hpcmips *t, bus_addr_t bpa,
    bus_size_t size, int cacheable)
{
	vaddr_t va, endva;
	pt_entry_t *pte;
	u_int32_t opte, npte;

	if (t->base >= MIPS_KSEG2_START) {
		va = mips_trunc_page(bpa);
		endva = mips_round_page(bpa + size);
		npte = CPUISMIPS3 ? MIPS3_PG_UNCACHED : MIPS1_PG_N;
		
		mips_dcache_wbinv_range(va, endva - va);

		for (; va < endva; va += PAGE_SIZE) {
			pte = kvtopte(va);
			opte = pte->pt_entry;
			if (cacheable) {
				opte &= ~npte;
			} else {
				opte |= npte;
			}
			pte->pt_entry = opte;
			/*
			 * Update the same virtual address entry.
			 */
			MachTLBUpdate(va, opte);
		}
		return (bpa);
	}

	return (cacheable ? MIPS_PHYS_TO_KSEG0(bpa) : MIPS_PHYS_TO_KSEG1(bpa));
}

/* ARGSUSED */
int
__bs_map(bus_space_tag_t tx, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp)
{
	struct bus_space_tag_hpcmips *t = (struct bus_space_tag_hpcmips *)tx;
	int err;
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;

	DPRINTF(("\tbus_space_map:%#lx(%#lx)+%#lx\n",
	    bpa, bpa + t->base, size));

	if (!t->extent) { /* Before autoconfiguration, can't use extent */
		DPRINTF(("bus_space_map: map temporary region:"
		    "0x%08lx-0x%08lx\n", bpa, bpa+size));
		bpa += t->base;
	} else {
		bpa += t->base;
		if ((err = extent_alloc_region(t->extent, bpa, size, 
		    EX_NOWAIT|EX_MALLOCOK))) {
			DPRINTF(("\tbus_space_map: "
			    "extent_alloc_regiion() failed\n"));
			return (err);
		}
	}
	*bshp = __hpcmips_cacheable(t, bpa, size, cacheable);

	return (0);
}

/* ARGSUSED */
void
__bs_unmap(bus_space_tag_t tx, bus_space_handle_t bsh, bus_size_t size)
{
	struct bus_space_tag_hpcmips *t = (struct bus_space_tag_hpcmips *)tx;
	int err;
	u_int32_t addr;

	if (!t->extent) {
		return; /* Before autoconfiguration, can't use extent */
	}

	if (t->base < MIPS_KSEG2_START) {
		addr = MIPS_KSEG1_TO_PHYS(bsh);
	} else {
		addr = bsh;
	}

	if ((err = extent_free(t->extent, addr, size, EX_NOWAIT))) {
		DPRINTF(("warning: %#lx-%#lx of %s space lost\n",
		    bsh, bsh+size, t->name));
	}
}

/* ARGSUSED */
int
__bs_subregion(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;

	return (0);
}

/* ARGSUSED */
int
__bs_alloc(bus_space_tag_t tx, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	struct bus_space_tag_hpcmips *t = (struct bus_space_tag_hpcmips *)tx;
	int cacheable = flags & BUS_SPACE_MAP_CACHEABLE;
	u_long bpa;
	int err;

	if (!t->extent)
		panic("bus_space_alloc: no extent");

	DPRINTF(("\tbus_space_alloc:%#lx(%#lx)+%#lx\n", bpa,
	    bpa - t->base, size));

	rstart += t->base;
	rend += t->base;
	if ((err = extent_alloc_subregion(t->extent, rstart, rend, size,
	    alignment, boundary, EX_FAST|EX_NOWAIT|EX_MALLOCOK, &bpa))) {
		return (err);
	}

	*bshp = __hpcmips_cacheable(t, bpa, size, cacheable);

	if (bpap) {
		*bpap = bpa;
	}

	return (0);
}

/* ARGSUSED */
void
__bs_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	/* bus_space_unmap() does all that we need to do. */
	bus_space_unmap(t, bsh, size);
}

void
__bs_barrier(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t len, int flags)
{
	wbflush();
}

int
__bs_peek(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    size_t size, void *ptr)
{
	u_int32_t tmp;

	if (badaddr((void *)(bsh + offset), size))
		return (-1);

	if (ptr == NULL)
		ptr = &tmp;

	switch(size) {
	case 1:
		*((u_int8_t *)ptr) = bus_space_read_1(t, bsh, offset);
		break;
	case 2:
		*((u_int16_t *)ptr) = bus_space_read_2(t, bsh, offset);
		break;
	case 4:
		*((u_int32_t *)ptr) = bus_space_read_4(t, bsh, offset);
		break;
	default:
		panic("bus_space_peek: bad size, %d", size);
	}

	return (0);
}

int
__bs_poke(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    size_t size, u_int32_t val)
{

	if (badaddr((void *)(bsh + offset), size))
		return (-1);

	switch(size) {
	case 1:
		bus_space_write_1(t, bsh, offset, val);
		break;
	case 2:
		bus_space_write_2(t, bsh, offset, val);
		break;
	case 4:
		bus_space_write_4(t, bsh, offset, val);
		break;
	default:
		panic("bus_space_poke: bad size, %d", size);
	}

	return (0);
}

u_int8_t
__bs_r_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	wbflush();
	return (*(volatile u_int8_t *)(bsh + offset));
}

u_int16_t
__bs_r_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	wbflush();
	return (*(volatile u_int16_t *)(bsh + offset));
}

u_int32_t
__bs_r_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{
	wbflush();
	return (*(volatile u_int32_t *)(bsh + offset));
}

void
__bs_rm_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int8_t *addr, bus_size_t count) {
	while (count--)
		*addr++ = bus_space_read_1(t, bsh, offset);
}

void
__bs_rm_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int16_t *addr, bus_size_t count)
{
	while (count--)
		*addr++ = bus_space_read_2(t, bsh, offset);
}

void
__bs_rm_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int32_t *addr, bus_size_t count)
{
	while (count--)
		*addr++ = bus_space_read_4(t, bsh, offset);
}

void
__bs_rr_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int8_t *addr, bus_size_t count)
{
	while (count--) {
		*addr++ = bus_space_read_1(t, bsh, offset);
		offset += sizeof(*addr);
	}
}

void
__bs_rr_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int16_t *addr, bus_size_t count)
{
	while (count--) {
		*addr++ = bus_space_read_2(t, bsh, offset);
		offset += sizeof(*addr);
	}
}

void
__bs_rr_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int32_t *addr, bus_size_t count)
{
	while (count--) {
		*addr++ = bus_space_read_4(t, bsh, offset);
		offset += sizeof(*addr);
	}
}

void
__bs_w_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int8_t value)
{
	*(volatile u_int8_t *)(bsh + offset) = value;
	wbflush();
}

void
__bs_w_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int16_t value)
{
	*(volatile u_int16_t *)(bsh + offset) = value;
	wbflush();
}

void
__bs_w_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    u_int32_t value)
{
	*(volatile u_int32_t *)(bsh + offset) = value;
	wbflush();
}

void
__bs_wm_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    const u_int8_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_1(t, bsh, offset, *addr++);
}

void
__bs_wm_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    const u_int16_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_2(t, bsh, offset, *addr++);
}

void
__bs_wm_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    const u_int32_t *addr, bus_size_t count)
{
	while (count--)
		bus_space_write_4(t, bsh, offset, *addr++);
}

void
__bs_wr_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    const u_int8_t *addr, bus_size_t count)
{
	while (count--) {
		bus_space_write_1(t, bsh, offset, *addr++);
		offset += sizeof(*addr);
	}
}

void
__bs_wr_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    const u_int16_t *addr, bus_size_t count)
{
	while (count--) {
		bus_space_write_2(t, bsh, offset, *addr++);
		offset += sizeof(*addr);
	}
}

void
__bs_wr_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
    const u_int32_t *addr, bus_size_t count)
{
	while (count--) {
		bus_space_write_4(t, bsh, offset, *addr++);
		offset += sizeof(*addr);
	}
}

void
__bs_sm_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value, bus_size_t count)
{
	while (count--)
		bus_space_write_1(t, bsh, offset, value);
}

void
__bs_sm_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t value, bus_size_t count)
{
	while (count--)
		bus_space_write_2(t, bsh, offset, value);
}

void
__bs_sm_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t value, bus_size_t count)
{
	while (count--)
		bus_space_write_4(t, bsh, offset, value);
}


void
__bs_sr_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int8_t value, bus_size_t count)
{
	while (count--) {
		bus_space_write_1(t, bsh, offset, value);
		offset += (value);
	}
}

void
__bs_sr_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int16_t value, bus_size_t count)
{
	while (count--) {
		bus_space_write_2(t, bsh, offset, value);
		offset += (value);
	}
}

void
__bs_sr_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, u_int32_t value, bus_size_t count)
{
	while (count--) {
		bus_space_write_4(t, bsh, offset, value);
		offset += (value);
	}
}

#define __bs_c_n(n)							\
void __CONCAT(__bs_c_,n)(bus_space_tag_t t, bus_space_handle_t h1,	\
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)	\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += n)			\
			__CONCAT(bus_space_write_,n)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,n)(t, h1, o1 + o));\
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * n; c != 0; c--, o -= n)		\
			__CONCAT(bus_space_write_,n)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,n)(t, h1, o1 + o));\
	}								\
}

__bs_c_n(1)
__bs_c_n(2)
__bs_c_n(4)
