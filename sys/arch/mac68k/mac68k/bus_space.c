/*	$NetBSD: bus_space.c,v 1.31.44.1 2014/08/20 00:03:11 tls Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
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
 * Implementation of bus_space mapping for mac68k.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.31.44.1 2014/08/20 00:03:11 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <m68k/cacheops.h>

#include <uvm/uvm_extern.h>

int	bus_mem_add_mapping(bus_addr_t, bus_size_t, int, bus_space_handle_t *);

extern struct extent *iomem_ex;
extern int iomem_malloc_safe;
label_t *nofault;

int
bus_mem_add_mapping(bus_addr_t bpa, bus_size_t size, int flags,
		    bus_space_handle_t *hp)
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;

	pa = m68k_trunc_page(bpa);
	endpa = m68k_round_page((bpa + size) - 1);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("bus_mem_add_mapping: overflow");
#endif

	va = uvm_km_alloc(kernel_map, endpa - pa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return (ENOMEM);

	hp->base = (u_long)(va + m68k_page_offset(bpa));
	hp->swapped = 0;
	hp->stride = 1;
	hp->bsr1 = mac68k_bsr1;
	hp->bsr2 = mac68k_bsr2;
	hp->bsr4 = mac68k_bsr4;
	hp->bsrs1 = mac68k_bsr1;
	hp->bsrs2 = mac68k_bsr2;
	hp->bsrs4 = mac68k_bsr4;
	hp->bsrm1 = mac68k_bsrm1;
	hp->bsrm2 = mac68k_bsrm2;
	hp->bsrm4 = mac68k_bsrm4;
	hp->bsrms1 = mac68k_bsrm1;
	hp->bsrms2 = mac68k_bsrm2;
	hp->bsrms4 = mac68k_bsrm4;
	hp->bsrr1 = mac68k_bsrr1;
	hp->bsrr2 = mac68k_bsrr2;
	hp->bsrr4 = mac68k_bsrr4;
	hp->bsrrs1 = mac68k_bsrr1;
	hp->bsrrs2 = mac68k_bsrr2;
	hp->bsrrs4 = mac68k_bsrr4;
	hp->bsw1 = mac68k_bsw1;
	hp->bsw2 = mac68k_bsw2;
	hp->bsw4 = mac68k_bsw4;
	hp->bsws1 = mac68k_bsw1;
	hp->bsws2 = mac68k_bsw2;
	hp->bsws4 = mac68k_bsw4;
	hp->bswm1 = mac68k_bswm1;
	hp->bswm2 = mac68k_bswm2;
	hp->bswm4 = mac68k_bswm4;
	hp->bswms1 = mac68k_bswm1;
	hp->bswms2 = mac68k_bswm2;
	hp->bswms4 = mac68k_bswm4;
	hp->bswr1 = mac68k_bswr1;
	hp->bswr2 = mac68k_bswr2;
	hp->bswr4 = mac68k_bswr4;
	hp->bswrs1 = mac68k_bswr1;
	hp->bswrs2 = mac68k_bswr2;
	hp->bswrs4 = mac68k_bswr4;
	hp->bssm1 = mac68k_bssm1;
	hp->bssm2 = mac68k_bssm2;
	hp->bssm4 = mac68k_bssm4;
	hp->bssr1 = mac68k_bssr1;
	hp->bssr2 = mac68k_bssr2;
	hp->bssr4 = mac68k_bssr4;

	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		pte = kvtopte(va);
		if ((flags & BUS_SPACE_MAP_CACHEABLE))
			*pte &= ~PG_CI;
		else
			*pte |= PG_CI;
		TBIS(va);
	}
	pmap_update(pmap_kernel());
 
	return 0;
}

int
bus_space_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
	      bus_space_handle_t *hp)
{
#ifdef DIAGNOSTIC
	paddr_t pa, endpa;
#endif
	int error;

	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(iomem_ex, bpa, size,
	    EX_NOWAIT | (iomem_malloc_safe ? EX_MALLOCOK : 0));
	if (error)
		return (error);

#ifdef DIAGNOSTIC
	pa = m68k_trunc_page(bpa + t);
	endpa = m68k_round_page((bpa + t + size) - 1);

	if (endpa <= pa)
		panic("bus_space_map: overflow");
#endif

	error = bus_mem_add_mapping(bpa, size, flags, hp);
	if (error) {
		if (extent_free(iomem_ex, bpa, size, EX_NOWAIT |
		    (iomem_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("bus_space_map: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("bus_space_map: can't free region\n");
		}
	}

	return (error);
}

int
bus_space_alloc(bus_space_tag_t t,
		bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
		bus_size_t alignment, bus_size_t boundary, int flags,
		bus_addr_t *bpap, bus_space_handle_t *hp)
{
	u_long bpa;
	int error;

	/*
	 * Sanity check the allocation against the extent's boundaries.
	 */
	if (rstart < iomem_ex->ex_start || rend > iomem_ex->ex_end)
		panic("bus_space_alloc: bad region start/end");

	/*
	 * Do the requested allocation.
	 */
	error = extent_alloc_subregion(iomem_ex, rstart, rend, size, alignment,
	    boundary,
	    EX_FAST | EX_NOWAIT | (iomem_malloc_safe ?  EX_MALLOCOK : 0),
	    &bpa);

	if (error)
		return (error);

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = bus_mem_add_mapping(bpa, size, flags, hp);
	if (error) {
		if (extent_free(iomem_ex, bpa, size, EX_NOWAIT |
		    (iomem_malloc_safe ? EX_MALLOCOK : 0))) {
			printf("bus_space_alloc: pa 0x%lx, size 0x%lx\n",
			    bpa, size);
			printf("bus_space_alloc: can't free region\n");
		}
	}

	*bpap = bpa;

	return (error);
}

void
bus_space_unmap(bus_space_tag_t t, bus_space_handle_t h, bus_size_t size)
{
	vaddr_t va, endva;
	bus_addr_t bpa;

	va = m68k_trunc_page(h.base);
	endva = m68k_round_page((h.base + size) - 1);

#ifdef DIAGNOSTIC
	if (endva <= va)
		panic("bus_space_unmap: overflow");
#endif

	(void) pmap_extract(pmap_kernel(), va, &bpa);
	bpa += m68k_page_offset(h.base);

	/*
	 * Free the kernel virtual mapping.
	 */
	pmap_kremove(va, endva - va);
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);

	if (extent_free(iomem_ex, bpa, size,
	    EX_NOWAIT | (iomem_malloc_safe ? EX_MALLOCOK : 0))) {
		printf("bus_space_unmap: pa 0x%lx, size 0x%lx\n",
		    bpa, size);
		printf("bus_space_unmap: can't free region\n");
	}
}

void    
bus_space_free(bus_space_tag_t t, bus_space_handle_t h, bus_size_t size)
{
	/* bus_space_unmap() does all that we need to do. */
	bus_space_unmap(t, h, size);
}

int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t h, bus_size_t offset,
		    bus_size_t size, bus_space_handle_t *nhp)
{

	*nhp = h;
	nhp->base += offset;
	return (0);
}

int
mac68k_bus_space_probe(bus_space_tag_t t, bus_space_handle_t h,
		       bus_size_t offset, int size)
{
	label_t faultbuf;

	nofault = &faultbuf;
	if (setjmp(nofault)) {
		nofault = (label_t *)0;
		return (0);
	}

	switch (size) {
	case 1:
		bus_space_read_1(t, h, offset);
		break;
	case 2:
		bus_space_read_2(t, h, offset);
		break;
	case 4:
		bus_space_read_4(t, h, offset);
		break;
	case 8:
	default:
		panic("bus_space_probe: unsupported data size %d", size);
		/* NOTREACHED */
	}

	nofault = (label_t *)0;
	return (1);
}

void
mac68k_bus_space_handle_swapped(bus_space_tag_t t, bus_space_handle_t *h)
{
	h->swapped = 1;
	if (h->stride == 1) {
		h->bsr2 = mac68k_bsr2_swap;
		h->bsr4 = mac68k_bsr4_swap;
		h->bsrm2 = mac68k_bsrm2_swap;
		h->bsrm4 = mac68k_bsrm4_swap;
		h->bsrr2 = mac68k_bsrr2_swap;
		h->bsrr4 = mac68k_bsrr4_swap;
		h->bsw2 = mac68k_bsw2_swap;
		h->bsw4 = mac68k_bsw4_swap;
		h->bswm2 = mac68k_bswm2_swap;
		h->bswm4 = mac68k_bswm4_swap;
		h->bswr2 = mac68k_bswr2_swap;
		h->bswr4 = mac68k_bswr4_swap;
		h->bssm2 = mac68k_bssm2_swap;
		h->bssm4 = mac68k_bssm4_swap;
		h->bssr2 = mac68k_bssr2_swap;
		h->bssr4 = mac68k_bssr4_swap;
	}
}

void
mac68k_bus_space_handle_set_stride(bus_space_tag_t t, bus_space_handle_t *h,
				   int stride)
{
	h->stride = stride;
	h->bsr1 = mac68k_bsr1_gen;
	h->bsr2 = mac68k_bsr2_gen;
	h->bsr4 = mac68k_bsr4_gen;
	h->bsrs2 = mac68k_bsrs2_gen;
	h->bsrs4 = mac68k_bsrs4_gen;
	h->bsrm1 = mac68k_bsrm1_gen;
	h->bsrm2 = mac68k_bsrm2_gen;
	h->bsrm4 = mac68k_bsrm4_gen;
	h->bsrms2 = mac68k_bsrms2_gen;
	h->bsrms4 = mac68k_bsrms4_gen;
	h->bsrr1 = mac68k_bsrr1_gen;
	h->bsrr2 = mac68k_bsrr2_gen;
	h->bsrr4 = mac68k_bsrr4_gen;
	h->bsrrs2 = mac68k_bsrrs2_gen;
	h->bsrrs4 = mac68k_bsrrs4_gen;
	h->bsw1 = mac68k_bsw1_gen;
	h->bsw2 = mac68k_bsw2_gen;
	h->bsw4 = mac68k_bsw4_gen;
	h->bsws2 = mac68k_bsws2_gen;
	h->bsws4 = mac68k_bsws4_gen;
	h->bswm2 = mac68k_bswm2_gen;
	h->bswm4 = mac68k_bswm4_gen;
	h->bswms2 = mac68k_bswms2_gen;
	h->bswms4 = mac68k_bswms4_gen;
	h->bswr1 = mac68k_bswr1_gen;
	h->bswr2 = mac68k_bswr2_gen;
	h->bswr4 = mac68k_bswr4_gen;
	h->bswrs2 = mac68k_bswrs2_gen;
	h->bswrs4 = mac68k_bswrs4_gen;
	h->bssm1 = mac68k_bssm1_gen;
	h->bssm2 = mac68k_bssm2_gen;
	h->bssm4 = mac68k_bssm4_gen;
	h->bssr1 = mac68k_bssr1_gen;
	h->bssr2 = mac68k_bssr2_gen;
	h->bssr4 = mac68k_bssr4_gen;
}

u_int8_t
mac68k_bsr1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	return (*(volatile u_int8_t *)(h->base + offset));
}

u_int8_t
mac68k_bsr1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	return (*(volatile u_int8_t *)(h->base + offset * h->stride));
}

u_int16_t
mac68k_bsr2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	return (*(volatile u_int16_t *)(h->base + offset));
}

u_int16_t
mac68k_bsr2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	u_int16_t	v;

	v = (*(volatile u_int16_t *)(h->base + offset));
	return bswap16(v);
}

u_int16_t
mac68k_bsrs2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	u_int16_t	v;

	v = (*(volatile u_int8_t *)(h->base + offset++ * h->stride)) << 8;
	v |= (*(volatile u_int8_t *)(h->base + offset * h->stride));
	return v;
}

u_int16_t
mac68k_bsr2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	u_int16_t	v;

	v = mac68k_bsrs2_gen(t, h, offset);
	if (h->swapped) {
		v = bswap16(v);
	}
	return v;
}

u_int32_t
mac68k_bsr4(bus_space_tag_t tag, bus_space_handle_t *h, bus_size_t offset)
{
	return (*(volatile u_int32_t *)(h->base + offset));
}

u_int32_t
mac68k_bsr4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	u_int32_t	v;

	v = (*(volatile u_int32_t *)(h->base + offset));
	return bswap32(v);
}

u_int32_t
mac68k_bsrs4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	u_int32_t	v;

	v = (*(volatile u_int8_t *)(h->base + offset++ * h->stride));
	v <<= 8;
	v |= (*(volatile u_int8_t *)(h->base + offset++ * h->stride));
	v <<= 8;
	v |= (*(volatile u_int8_t *)(h->base + offset++ * h->stride));
	v <<= 8;
	v |= (*(volatile u_int8_t *)(h->base + offset++ * h->stride));
	return v;
}

u_int32_t
mac68k_bsr4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset)
{
	u_int32_t	v;

	v = mac68k_bsrs4_gen(t, h, offset);
	if (h->swapped) {
		v = bswap32(v);
	}
	return v;
}

void
mac68k_bsrm1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int8_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movb	%%a0@,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrm1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int8_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_1(t, *h, offset);
	}
}

void
mac68k_bsrm2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a0@,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrm2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a0@,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	movw	%%d1,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bsrm2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int16_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_2(t, *h, offset);
	}
}

void
mac68k_bsrms2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int16_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_stream_2(t, *h, offset);
	}
}

void
mac68k_bsrm4(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a0@,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrm4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a0@,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	swap	%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%%d1,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bsrm4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_4(t, *h, offset);
	}
}

void
mac68k_bsrms4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int32_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_stream_4(t, *h, offset);
	}
}

void
mac68k_bsrr1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int8_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movb	%%a0@+,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrr1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int8_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_1(t, *h, offset);
		offset++;
	}
}

void
mac68k_bsrr2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a0@+,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrr2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a0@+,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	movw	%%d1,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bsrr2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int16_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_2(t, *h, offset);
		offset += 2;
	}
}

void
mac68k_bsrrs2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int16_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_stream_2(t, *h, offset);
		offset += 2;
	}
}

void
mac68k_bsrr4(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a0@+,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrr4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a0@+,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	swap	%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%%d1,%%a1@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bsrr4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_4(t, *h, offset);
		offset += 4;
	}
}

void
mac68k_bsrrs4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int32_t *a, size_t c)
{
	while (c--) {
		*a++ = bus_space_read_stream_4(t, *h, offset);
		offset += 4;
	}
}

void
mac68k_bsw1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	    u_int8_t v)
{
	(*(volatile u_int8_t *)(h->base + offset)) = v;
}

void
mac68k_bsw1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		u_int8_t v)
{
	(*(volatile u_int8_t *)(h->base + offset * h->stride)) = v;
}

void
mac68k_bsw2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	    u_int16_t v)
{
	(*(volatile u_int16_t *)(h->base + offset)) = v;
}

void
mac68k_bsw2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int16_t v)
{
	v = bswap16(v);
	(*(volatile u_int16_t *)(h->base + offset)) = v;
}

void
mac68k_bsws2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int16_t v)
{
	u_int8_t	v1;

	v1 = (v & 0xff00) >> 8;
	(*(volatile u_int8_t *)(h->base + offset++ * h->stride)) = v1;
	(*(volatile u_int8_t *)(h->base + offset * h->stride)) = v & 0xff;
}

void
mac68k_bsw2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		u_int16_t v)
{
	if (h->swapped) {
		v = bswap16(v);
	}
	mac68k_bsws2_gen(t, h, offset, v);
}

void
mac68k_bsw4(bus_space_tag_t tag, bus_space_handle_t *h, bus_size_t offset,
	    u_int32_t v)
{
	(*(volatile u_int32_t *)(h->base + offset)) = v;
}

void
mac68k_bsw4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t v)
{
	v = bswap32(v);
	(*(volatile u_int32_t *)(h->base + offset)) = v;
}

void
mac68k_bsws4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t v)
{
	u_int8_t	v1,v2,v3;

	v1 = (v & 0xff000000) >> 24;
	v2 = (v & 0x00ff0000) >> 16;
	v3 = (v & 0x0000ff00) >> 8;
	(*(volatile u_int8_t *)(h->base + offset++ * h->stride)) = v1;
	(*(volatile u_int8_t *)(h->base + offset++ * h->stride)) = v2;
	(*(volatile u_int8_t *)(h->base + offset++ * h->stride)) = v3;
	(*(volatile u_int8_t *)(h->base + offset * h->stride)) = v & 0xff;
}

void
mac68k_bsw4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t v)
{
	if (h->swapped) {
		v = bswap32(v);
	}
	mac68k_bsws4_gen(t, h, offset, v);
}

void
mac68k_bswm1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     const u_int8_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movb	%%a1@+,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bswm1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 const u_int8_t *a, size_t c)
{
	while (c--) {
		bus_space_write_1(t, *h, offset, *a++);
	}
}

void
mac68k_bswm2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     const u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a1@+,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bswm2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a1@+,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	movw	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bswm2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 const u_int16_t *a, size_t c)
{
	while (c--) {
		bus_space_write_2(t, *h, offset, *a++);
	}
}

void
mac68k_bswms2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int16_t *a, size_t c)
{
	while (c--) {
		bus_space_write_stream_2(t, *h, offset, *a++);
	}
}

void
mac68k_bswm4(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     const u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a1@+,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bswm4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a1@+,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	swap	%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bswm4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 const u_int32_t *a, size_t c)
{
	while (c--) {
		bus_space_write_4(t, *h, offset, *a++);
	}
}

void
mac68k_bswms4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int32_t *a, size_t c)
{
	while (c--) {
		bus_space_write_stream_4(t, *h, offset, *a++);
	}
}

void
mac68k_bswr1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     const u_int8_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movb	%%a1@+,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bswr1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 const u_int8_t *a, size_t c)
{
	while (c--) {
		bus_space_write_1(t, *h, offset, *a++);
		offset++;
	}
}

void
mac68k_bswr2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     const u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a1@+,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bswr2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int16_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%a1@+,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	movw	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bswr2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 const u_int16_t *a, size_t c)
{
	while (c--) {
		bus_space_write_2(t, *h, offset, *a++);
		offset += 2;
	}
}

void
mac68k_bswrs2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int16_t *a, size_t c)
{
	while (c--) {
		bus_space_write_stream_2(t, *h, offset, *a++);
		offset += 2;
	}
}

void
mac68k_bswr4(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     const u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a1@+,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0");
}

void
mac68k_bswr4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int32_t *a, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%a1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%a1@+,%%d1	;"
	"	rolw	#8,%%d1		;"
	"	swap	%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"					:
								:
		    "r" (h->base + offset), "g" (a), "g" (c)	:
		    "a0","a1","d0","d1");
}

void
mac68k_bswr4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 const u_int32_t *a, size_t c)
{
	while (c--) {
		bus_space_write_4(t, *h, offset, *a++);
		offset += 4;
	}
}

void
mac68k_bswrs4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  const u_int32_t *a, size_t c)
{
	while (c--) {
		bus_space_write_4(t, *h, offset, *a++);
		offset += 4;
	}
}

void
mac68k_bssm1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int8_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movb	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssm1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int8_t v, size_t c)
{
	while (c--) {
		bus_space_write_1(t, *h, offset, v);
	}
}

void
mac68k_bssm2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int16_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssm2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int16_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssm2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int16_t v, size_t c)
{
	while (c--) {
		bus_space_write_2(t, *h, offset, v);
	}
}

void
mac68k_bssm4(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int32_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssm4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int32_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	rolw	#8,%%d1		;"
	"	swap	%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%d1,%%a0@	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssm4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t v, size_t c)
{
	while (c--) {
		bus_space_write_4(t, *h, offset, v);
	}
}

void
mac68k_bssr1(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int8_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movb	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssr1_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int8_t v, size_t c)
{
	while (c--) {
		bus_space_write_1(t, *h, offset, v);
		offset++;
	}
}

void
mac68k_bssr2(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int16_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssr2_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int16_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movw	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssr2_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int16_t v, size_t c)
{
	while (c--) {
		bus_space_write_2(t, *h, offset, v);
		offset += 2;
	}
}

void
mac68k_bssr4(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
	     u_int32_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssr4_swap(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		  u_int32_t v, size_t c)
{
	__asm volatile (
	"	movl	%0,%%a0		;"
	"	movl	%1,%%d1		;"
	"	rolw	#8,%%d1		;"
	"	swap	%%d1		;"
	"	rolw	#8,%%d1		;"
	"	movl	%2,%%d0		;"
	"1:	movl	%%d1,%%a0@+	;"
	"	subql	#1,%%d0		;"
	"	jne	1b"						:
									:
		    "r" (h->base + offset), "g" ((u_long)v), "g" (c)	:
		    "a0","d0","d1");
}

void
mac68k_bssr4_gen(bus_space_tag_t t, bus_space_handle_t *h, bus_size_t offset,
		 u_int32_t v, size_t c)
{
	while (c--) {
		bus_space_write_4(t, *h, offset, v);
		offset += 4;
	}
}
