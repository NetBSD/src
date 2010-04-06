/*	$NetBSD: ap_ms104_sh4_space.c,v 1.1 2010/04/06 15:54:29 nonaka Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
__KERNEL_RCSID(0, "$NetBSD: ap_ms104_sh4_space.c,v 1.1 2010/04/06 15:54:29 nonaka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <sh3/bscreg.h>
#include <sh3/devreg.h>
#include <sh3/mmu.h>
#include <sh3/pmap.h>
#include <sh3/pte.h>

#include <machine/cpu.h>

/*
 * I/O bus space
 */
#define	AP_MS104_SH4_IOMEM_IO		0	/* space is i/o space */
#define	AP_MS104_SH4_IOMEM_MEM		1	/* space is mem space */
#define	AP_MS104_SH4_IOMEM_PCMCIA_IO	2	/* PCMCIA IO space */
#define	AP_MS104_SH4_IOMEM_PCMCIA_MEM	3	/* PCMCIA Mem space */
#define	AP_MS104_SH4_IOMEM_PCMCIA_ATT	4	/* PCMCIA Attr space */
#define	AP_MS104_SH4_IOMEM_PCMCIA_8BIT	0x8000	/* PCMCIA BUS 8 BIT WIDTH */
#define	AP_MS104_SH4_IOMEM_PCMCIA_IO8 \
	    (AP_MS104_SH4_IOMEM_PCMCIA_IO|AP_MS104_SH4_IOMEM_PCMCIA_8BIT)
#define	AP_MS104_SH4_IOMEM_PCMCIA_MEM8 \
	    (AP_MS104_SH4_IOMEM_PCMCIA_MEM|AP_MS104_SH4_IOMEM_PCMCIA_8BIT)
#define	AP_MS104_SH4_IOMEM_PCMCIA_ATT8 \
	    (AP_MS104_SH4_IOMEM_PCMCIA_ATT|AP_MS104_SH4_IOMEM_PCMCIA_8BIT)

int ap_ms104_sh4_iomem_map(void *v, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp);
void ap_ms104_sh4_iomem_unmap(void *v, bus_space_handle_t bsh, bus_size_t size);
int ap_ms104_sh4_iomem_subregion(void *v, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp);
int ap_ms104_sh4_iomem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp);
void ap_ms104_sh4_iomem_free(void *v, bus_space_handle_t bsh, bus_size_t size);

static int ap_ms104_sh4_iomem_add_mapping(bus_addr_t, bus_size_t, int,
    bus_space_handle_t *);

static int
ap_ms104_sh4_iomem_add_mapping(bus_addr_t bpa, bus_size_t size, int type,
    bus_space_handle_t *bshp)
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;
	unsigned int m = 0;
	int io_type = type & ~AP_MS104_SH4_IOMEM_PCMCIA_8BIT;

	pa = sh3_trunc_page(bpa);
	endpa = sh3_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("ap_ms104_sh4_iomem_add_mapping: overflow");
#endif

	va = uvm_km_alloc(kernel_map, endpa - pa, 0, UVM_KMF_VAONLY);
	if (va == 0){
		printf("ap_ms104_sh4_iomem_add_mapping: nomem\n");
		return (ENOMEM);
	}

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

#define MODE(t, s)							\
	((t) & AP_MS104_SH4_IOMEM_PCMCIA_8BIT) ?			\
		_PG_PCMCIA_ ## s ## 8 :					\
		_PG_PCMCIA_ ## s ## 16
	switch (io_type) {
	default:
		panic("unknown pcmcia space.");
		/* NOTREACHED */
	case AP_MS104_SH4_IOMEM_PCMCIA_IO:
		m = MODE(type, IO);
		break;
	case AP_MS104_SH4_IOMEM_PCMCIA_MEM:
		m = MODE(type, MEM);
		break;
	case AP_MS104_SH4_IOMEM_PCMCIA_ATT:
		m = MODE(type, ATTR);
		break;
	}
#undef MODE

	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
		pte = __pmap_kpte_lookup(va);
		KDASSERT(pte);
		*pte |= m;  /* PTEA PCMCIA assistant bit */
		sh_tlb_update(0, va, *pte);
	}

	return (0);
}

int
ap_ms104_sh4_iomem_map(void *v, bus_addr_t bpa, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{
	bus_addr_t addr = SH3_PHYS_TO_P2SEG(bpa);
	int error;

	KASSERT((bpa & SH3_PHYS_MASK) == bpa);

	if (bpa < 0x14000000 || bpa >= 0x1c000000) {
		/* CS0,1,2,3,4,7 */
		*bshp = (bus_space_handle_t)addr;
		return (0);
	}

	/* CS5,6 */
	error = ap_ms104_sh4_iomem_add_mapping(addr, size, (int)(u_long)v, bshp);

	return (error);
}

void
ap_ms104_sh4_iomem_unmap(void *v, bus_space_handle_t bsh, bus_size_t size)
{
	u_long va, endva;
	bus_addr_t bpa;

	if (bsh >= SH3_P2SEG_BASE && bsh <= SH3_P2SEG_END) {
		/* maybe CS0,1,2,3,4,7 */
		return;
	}

	/* CS5,6 */
	va = sh3_trunc_page(bsh);
	endva = sh3_round_page(bsh + size);

#ifdef DIAGNOSTIC
	if (endva <= va)
		panic("ap_ms104_sh4_io_unmap: overflow");
#endif

	pmap_extract(pmap_kernel(), va, &bpa);
	bpa += bsh & PGOFSET;

	pmap_kremove(va, endva - va);

	/*
	 * Free the kernel virtual mapping.
	 */
	uvm_km_free(kernel_map, va, endva - va, UVM_KMF_VAONLY);
}

int
ap_ms104_sh4_iomem_subregion(void *v, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;

	return (0);
}

int
ap_ms104_sh4_iomem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	*bshp = *bpap = rstart;

	return (0);
}

void
ap_ms104_sh4_iomem_free(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	ap_ms104_sh4_iomem_unmap(v, bsh, size);
}

/*
 * on-board I/O bus space read/write
 */
uint8_t ap_ms104_sh4_iomem_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint16_t ap_ms104_sh4_iomem_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint32_t ap_ms104_sh4_iomem_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset);
void ap_ms104_sh4_iomem_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_write_1(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t value);
void ap_ms104_sh4_iomem_write_2(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t value);
void ap_ms104_sh4_iomem_write_4(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t value);
void ap_ms104_sh4_iomem_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);
void ap_ms104_sh4_iomem_set_multi_1(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t val, bus_size_t count);
void ap_ms104_sh4_iomem_set_multi_2(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t val, bus_size_t count);
void ap_ms104_sh4_iomem_set_multi_4(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t val, bus_size_t count);
void ap_ms104_sh4_iomem_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count);
void ap_ms104_sh4_iomem_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count);
void ap_ms104_sh4_iomem_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count);
void ap_ms104_sh4_iomem_copy_region_1(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count);
void ap_ms104_sh4_iomem_copy_region_2(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count);
void ap_ms104_sh4_iomem_copy_region_4(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count);

struct _bus_space ap_ms104_sh4_bus_io =
{
	.bs_cookie = (void *)AP_MS104_SH4_IOMEM_PCMCIA_IO,

	.bs_map = ap_ms104_sh4_iomem_map,
	.bs_unmap = ap_ms104_sh4_iomem_unmap,
	.bs_subregion = ap_ms104_sh4_iomem_subregion,

	.bs_alloc = ap_ms104_sh4_iomem_alloc,
	.bs_free = ap_ms104_sh4_iomem_free,

	.bs_r_1 = ap_ms104_sh4_iomem_read_1,
	.bs_r_2 = ap_ms104_sh4_iomem_read_2,
	.bs_r_4 = ap_ms104_sh4_iomem_read_4,

	.bs_rm_1 = ap_ms104_sh4_iomem_read_multi_1,
	.bs_rm_2 = ap_ms104_sh4_iomem_read_multi_2,
	.bs_rm_4 = ap_ms104_sh4_iomem_read_multi_4,

	.bs_rr_1 = ap_ms104_sh4_iomem_read_region_1,
	.bs_rr_2 = ap_ms104_sh4_iomem_read_region_2,
	.bs_rr_4 = ap_ms104_sh4_iomem_read_region_4,

	.bs_rs_1 = ap_ms104_sh4_iomem_read_1,
	.bs_rs_2 = ap_ms104_sh4_iomem_read_2,
	.bs_rs_4 = ap_ms104_sh4_iomem_read_4,

	.bs_rms_1 = ap_ms104_sh4_iomem_read_multi_1,
	.bs_rms_2 = ap_ms104_sh4_iomem_read_multi_2,
	.bs_rms_4 = ap_ms104_sh4_iomem_read_multi_4,

	.bs_rrs_1 = ap_ms104_sh4_iomem_read_region_1,
	.bs_rrs_2 = ap_ms104_sh4_iomem_read_region_2,
	.bs_rrs_4 = ap_ms104_sh4_iomem_read_region_4,

	.bs_w_1 = ap_ms104_sh4_iomem_write_1,
	.bs_w_2 = ap_ms104_sh4_iomem_write_2,
	.bs_w_4 = ap_ms104_sh4_iomem_write_4,

	.bs_wm_1 = ap_ms104_sh4_iomem_write_multi_1,
	.bs_wm_2 = ap_ms104_sh4_iomem_write_multi_2,
	.bs_wm_4 = ap_ms104_sh4_iomem_write_multi_4,

	.bs_wr_1 = ap_ms104_sh4_iomem_write_region_1,
	.bs_wr_2 = ap_ms104_sh4_iomem_write_region_2,
	.bs_wr_4 = ap_ms104_sh4_iomem_write_region_4,

	.bs_ws_1 = ap_ms104_sh4_iomem_write_1,
	.bs_ws_2 = ap_ms104_sh4_iomem_write_2,
	.bs_ws_4 = ap_ms104_sh4_iomem_write_4,

	.bs_wms_1 = ap_ms104_sh4_iomem_write_multi_1,
	.bs_wms_2 = ap_ms104_sh4_iomem_write_multi_2,
	.bs_wms_4 = ap_ms104_sh4_iomem_write_multi_4,

	.bs_wrs_1 = ap_ms104_sh4_iomem_write_region_1,
	.bs_wrs_2 = ap_ms104_sh4_iomem_write_region_2,
	.bs_wrs_4 = ap_ms104_sh4_iomem_write_region_4,

	.bs_sm_1 = ap_ms104_sh4_iomem_set_multi_1,
	.bs_sm_2 = ap_ms104_sh4_iomem_set_multi_2,
	.bs_sm_4 = ap_ms104_sh4_iomem_set_multi_4,

	.bs_sr_1 = ap_ms104_sh4_iomem_set_region_1,
	.bs_sr_2 = ap_ms104_sh4_iomem_set_region_2,
	.bs_sr_4 = ap_ms104_sh4_iomem_set_region_4,

	.bs_c_1 = ap_ms104_sh4_iomem_copy_region_1,
	.bs_c_2 = ap_ms104_sh4_iomem_copy_region_2,
	.bs_c_4 = ap_ms104_sh4_iomem_copy_region_4,
};

struct _bus_space ap_ms104_sh4_bus_mem =
{
	.bs_cookie = (void *)AP_MS104_SH4_IOMEM_PCMCIA_MEM,

	.bs_map = ap_ms104_sh4_iomem_map,
	.bs_unmap = ap_ms104_sh4_iomem_unmap,
	.bs_subregion = ap_ms104_sh4_iomem_subregion,

	.bs_alloc = ap_ms104_sh4_iomem_alloc,
	.bs_free = ap_ms104_sh4_iomem_free,

	.bs_r_1 = ap_ms104_sh4_iomem_read_1,
	.bs_r_2 = ap_ms104_sh4_iomem_read_2,
	.bs_r_4 = ap_ms104_sh4_iomem_read_4,

	.bs_rm_1 = ap_ms104_sh4_iomem_read_multi_1,
	.bs_rm_2 = ap_ms104_sh4_iomem_read_multi_2,
	.bs_rm_4 = ap_ms104_sh4_iomem_read_multi_4,

	.bs_rr_1 = ap_ms104_sh4_iomem_read_region_1,
	.bs_rr_2 = ap_ms104_sh4_iomem_read_region_2,
	.bs_rr_4 = ap_ms104_sh4_iomem_read_region_4,

	.bs_rs_1 = ap_ms104_sh4_iomem_read_1,
	.bs_rs_2 = ap_ms104_sh4_iomem_read_2,
	.bs_rs_4 = ap_ms104_sh4_iomem_read_4,

	.bs_rms_1 = ap_ms104_sh4_iomem_read_multi_1,
	.bs_rms_2 = ap_ms104_sh4_iomem_read_multi_2,
	.bs_rms_4 = ap_ms104_sh4_iomem_read_multi_4,

	.bs_rrs_1 = ap_ms104_sh4_iomem_read_region_1,
	.bs_rrs_2 = ap_ms104_sh4_iomem_read_region_2,
	.bs_rrs_4 = ap_ms104_sh4_iomem_read_region_4,

	.bs_w_1 = ap_ms104_sh4_iomem_write_1,
	.bs_w_2 = ap_ms104_sh4_iomem_write_2,
	.bs_w_4 = ap_ms104_sh4_iomem_write_4,

	.bs_wm_1 = ap_ms104_sh4_iomem_write_multi_1,
	.bs_wm_2 = ap_ms104_sh4_iomem_write_multi_2,
	.bs_wm_4 = ap_ms104_sh4_iomem_write_multi_4,

	.bs_wr_1 = ap_ms104_sh4_iomem_write_region_1,
	.bs_wr_2 = ap_ms104_sh4_iomem_write_region_2,
	.bs_wr_4 = ap_ms104_sh4_iomem_write_region_4,

	.bs_ws_1 = ap_ms104_sh4_iomem_write_1,
	.bs_ws_2 = ap_ms104_sh4_iomem_write_2,
	.bs_ws_4 = ap_ms104_sh4_iomem_write_4,

	.bs_wms_1 = ap_ms104_sh4_iomem_write_multi_1,
	.bs_wms_2 = ap_ms104_sh4_iomem_write_multi_2,
	.bs_wms_4 = ap_ms104_sh4_iomem_write_multi_4,

	.bs_wrs_1 = ap_ms104_sh4_iomem_write_region_1,
	.bs_wrs_2 = ap_ms104_sh4_iomem_write_region_2,
	.bs_wrs_4 = ap_ms104_sh4_iomem_write_region_4,

	.bs_sm_1 = ap_ms104_sh4_iomem_set_multi_1,
	.bs_sm_2 = ap_ms104_sh4_iomem_set_multi_2,
	.bs_sm_4 = ap_ms104_sh4_iomem_set_multi_4,

	.bs_sr_1 = ap_ms104_sh4_iomem_set_region_1,
	.bs_sr_2 = ap_ms104_sh4_iomem_set_region_2,
	.bs_sr_4 = ap_ms104_sh4_iomem_set_region_4,

	.bs_c_1 = ap_ms104_sh4_iomem_copy_region_1,
	.bs_c_2 = ap_ms104_sh4_iomem_copy_region_2,
	.bs_c_4 = ap_ms104_sh4_iomem_copy_region_4,
};

struct _bus_space ap_ms104_sh4_bus_att =
{
	.bs_cookie = (void *)AP_MS104_SH4_IOMEM_PCMCIA_ATT,

	.bs_map = ap_ms104_sh4_iomem_map,
	.bs_unmap = ap_ms104_sh4_iomem_unmap,
	.bs_subregion = ap_ms104_sh4_iomem_subregion,

	.bs_alloc = ap_ms104_sh4_iomem_alloc,
	.bs_free = ap_ms104_sh4_iomem_free,

	.bs_r_1 = ap_ms104_sh4_iomem_read_1,
	.bs_r_2 = ap_ms104_sh4_iomem_read_2,
	.bs_r_4 = ap_ms104_sh4_iomem_read_4,

	.bs_rm_1 = ap_ms104_sh4_iomem_read_multi_1,
	.bs_rm_2 = ap_ms104_sh4_iomem_read_multi_2,
	.bs_rm_4 = ap_ms104_sh4_iomem_read_multi_4,

	.bs_rr_1 = ap_ms104_sh4_iomem_read_region_1,
	.bs_rr_2 = ap_ms104_sh4_iomem_read_region_2,
	.bs_rr_4 = ap_ms104_sh4_iomem_read_region_4,

	.bs_rs_1 = ap_ms104_sh4_iomem_read_1,
	.bs_rs_2 = ap_ms104_sh4_iomem_read_2,
	.bs_rs_4 = ap_ms104_sh4_iomem_read_4,

	.bs_rms_1 = ap_ms104_sh4_iomem_read_multi_1,
	.bs_rms_2 = ap_ms104_sh4_iomem_read_multi_2,
	.bs_rms_4 = ap_ms104_sh4_iomem_read_multi_4,

	.bs_rrs_1 = ap_ms104_sh4_iomem_read_region_1,
	.bs_rrs_2 = ap_ms104_sh4_iomem_read_region_2,
	.bs_rrs_4 = ap_ms104_sh4_iomem_read_region_4,

	.bs_w_1 = ap_ms104_sh4_iomem_write_1,
	.bs_w_2 = ap_ms104_sh4_iomem_write_2,
	.bs_w_4 = ap_ms104_sh4_iomem_write_4,

	.bs_wm_1 = ap_ms104_sh4_iomem_write_multi_1,
	.bs_wm_2 = ap_ms104_sh4_iomem_write_multi_2,
	.bs_wm_4 = ap_ms104_sh4_iomem_write_multi_4,

	.bs_wr_1 = ap_ms104_sh4_iomem_write_region_1,
	.bs_wr_2 = ap_ms104_sh4_iomem_write_region_2,
	.bs_wr_4 = ap_ms104_sh4_iomem_write_region_4,

	.bs_ws_1 = ap_ms104_sh4_iomem_write_1,
	.bs_ws_2 = ap_ms104_sh4_iomem_write_2,
	.bs_ws_4 = ap_ms104_sh4_iomem_write_4,

	.bs_wms_1 = ap_ms104_sh4_iomem_write_multi_1,
	.bs_wms_2 = ap_ms104_sh4_iomem_write_multi_2,
	.bs_wms_4 = ap_ms104_sh4_iomem_write_multi_4,

	.bs_wrs_1 = ap_ms104_sh4_iomem_write_region_1,
	.bs_wrs_2 = ap_ms104_sh4_iomem_write_region_2,
	.bs_wrs_4 = ap_ms104_sh4_iomem_write_region_4,

	.bs_sm_1 = ap_ms104_sh4_iomem_set_multi_1,
	.bs_sm_2 = ap_ms104_sh4_iomem_set_multi_2,
	.bs_sm_4 = ap_ms104_sh4_iomem_set_multi_4,

	.bs_sr_1 = ap_ms104_sh4_iomem_set_region_1,
	.bs_sr_2 = ap_ms104_sh4_iomem_set_region_2,
	.bs_sr_4 = ap_ms104_sh4_iomem_set_region_4,

	.bs_c_1 = ap_ms104_sh4_iomem_copy_region_1,
	.bs_c_2 = ap_ms104_sh4_iomem_copy_region_2,
	.bs_c_4 = ap_ms104_sh4_iomem_copy_region_4,
};

/* read */
uint8_t
ap_ms104_sh4_iomem_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset)
{

	return *(volatile uint8_t *)(bsh + offset);
}

uint16_t
ap_ms104_sh4_iomem_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset)
{

	return (*(volatile uint16_t *)(bsh + offset));
}

uint32_t
ap_ms104_sh4_iomem_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset)
{

	return (*(volatile uint32_t *)(bsh + offset));
}

void
ap_ms104_sh4_iomem_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p;
	}
}

void
ap_ms104_sh4_iomem_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *src = (void *)(bsh + offset);
	volatile uint16_t *dest = (void *)addr;

	while (count--) {
		*dest++ = *src;
	}
}

void
ap_ms104_sh4_iomem_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *src = (void *)(bsh + offset);
	volatile uint32_t *dest = (void *)addr;

	while (count--) {
		*dest++ = *src;
	}
}

void
ap_ms104_sh4_iomem_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p++;
	}
}

void
ap_ms104_sh4_iomem_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p++;
	}
}

void
ap_ms104_sh4_iomem_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p++;
	}
}

/* write */
void
ap_ms104_sh4_iomem_write_1(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t value)
{

	*(volatile uint8_t *)(bsh + offset) = value;
}

void
ap_ms104_sh4_iomem_write_2(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t value)
{

	*(volatile uint16_t *)(bsh + offset) = value;
}

void
ap_ms104_sh4_iomem_write_4(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t value)
{

	*(volatile uint32_t *)(bsh + offset) = value;
}

void
ap_ms104_sh4_iomem_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = *addr++;
	}
}

void
ap_ms104_sh4_iomem_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *dest = (void *)(bsh + offset);
	volatile const uint16_t *src = (const void *)addr;

	while (count--) {
		*dest = *src++;
	}
}

void
ap_ms104_sh4_iomem_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *dest = (void *)(bsh + offset);
	volatile const uint32_t *src = (const void *)addr;

	while (count--) {
		*dest = *src++;
	}
}

void
ap_ms104_sh4_iomem_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*p++ = *addr++;
	}
}

void
ap_ms104_sh4_iomem_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*p++ = *addr++;
	}
}

void
ap_ms104_sh4_iomem_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*p++ = *addr++;
	}
}

void
ap_ms104_sh4_iomem_set_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = val;
	}
}

void
ap_ms104_sh4_iomem_set_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *dest = (void *)(bsh + offset);

	while (count--) {
		*dest = val;
	}
}

void
ap_ms104_sh4_iomem_set_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *dest = (void *)(bsh + offset);

	while (count--) {
		*dest = val;
	}
}

void
ap_ms104_sh4_iomem_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *addr = (void *)(bsh + offset);

	while (count--) {
		*addr++ = val;
	}
}

void
ap_ms104_sh4_iomem_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *dest = (void *)(bsh + offset);

	while (count--) {
		*dest++ = val;
	}
}

void
ap_ms104_sh4_iomem_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *dest = (void *)(bsh + offset);

	while (count--) {
		*dest++ = val;
	}
}

void
ap_ms104_sh4_iomem_copy_region_1(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count)
{
	volatile uint8_t *addr1 = (void *)(h1 + o1);
	volatile uint8_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			*addr2++ = *addr1++;
		}
	} else {		/* dest after src: copy backwards */
		addr1 += count - 1;
		addr2 += count - 1;
		while (count--) {
			*addr2-- = *addr1--;
		}
	}
}

void
ap_ms104_sh4_iomem_copy_region_2(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count)
{
	volatile uint16_t *addr1 = (void *)(h1 + o1);
	volatile uint16_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			*addr2++ = *addr1++;
		}
	} else {		/* dest after src: copy backwards */
		addr1 += count - 1;
		addr2 += count - 1;
		while (count--) {
			*addr2-- = *addr1--;
		}
	}
}

void
ap_ms104_sh4_iomem_copy_region_4(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count)
{
	volatile uint32_t *addr1 = (void *)(h1 + o1);
	volatile uint32_t *addr2 = (void *)(h2 + o2);

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			*addr2++ = *addr1++;
		}
	} else {		/* dest after src: copy backwards */
		addr1 += count - 1;
		addr2 += count - 1;
		while (count--) {
			*addr2-- = *addr1--;
		}
	}
}
