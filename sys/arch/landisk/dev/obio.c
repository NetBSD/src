/*	$NetBSD: obio.c,v 1.6.10.1 2009/10/04 00:45:35 snj Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.6.10.1 2009/10/04 00:45:35 snj Exp $");

#include "btn_obio.h"
#include "pwrsw_obio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <sh3/devreg.h>
#include <sh3/mmu.h>
#include <sh3/pmap.h>
#include <sh3/pte.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <landisk/dev/obiovar.h>

#if (NPWRSW_OBIO > 0) || (NBTN_OBIO > 0)
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>
#endif

#include "locators.h"


struct obio_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;		/* io space tag */
	bus_space_tag_t sc_memt;	/* mem space tag */
};

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_print(void *, const char *);
static int	obio_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(obio, sizeof(struct obio_softc),
    obio_match, obio_attach, NULL, NULL);

static int
obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct obiobus_attach_args *oba = aux;

	if (strcmp(oba->oba_busname, cf->cf_name))
		return (0);

	return (1);
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	struct obiobus_attach_args *oba = aux;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	sc->sc_iot = oba->oba_iot;
	sc->sc_memt = oba->oba_memt;

#if (NPWRSW_OBIO > 0) || (NBTN_OBIO > 0)
	sysmon_power_settype("landisk");
	sysmon_task_queue_init();
#endif

	config_search_ia(obio_search, self, "obio", NULL);
}

static int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_io res_io[1];
	struct obio_iomem res_mem[1];
	struct obio_irq res_irq[1];
	struct obio_softc *sc = device_private(parent);
	struct obio_attach_args oa;
	int tryagain;

	do {
		oa.oa_iot = sc->sc_iot;
		oa.oa_memt = sc->sc_memt;

		res_io[0].or_addr = cf->cf_iobase;
		res_io[0].or_size = cf->cf_iosize;

		res_mem[0].or_addr = cf->cf_maddr;
		res_mem[0].or_size = cf->cf_msize;

		res_irq[0].or_irq = cf->cf_irq;

		oa.oa_io = res_io;
		oa.oa_nio = 1;

		oa.oa_iomem = res_mem;
		oa.oa_niomem = 1;

		oa.oa_irq = res_irq;
		oa.oa_nirq = 1;

		tryagain = 0;
		if (config_match(parent, cf, &oa) > 0) {
			config_attach(parent, cf, &oa, obio_print);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return (0);
}

static int
obio_print(void *args, const char *name)
{
	struct obio_attach_args *oa = args;
	const char *sep;
	int i;

	if (oa->oa_nio) {
		sep = "";
		aprint_normal(" port ");
		for (i = 0; i < oa->oa_nio; i++) {
			if (oa->oa_io[i].or_size == 0)
				continue;
			aprint_normal("%s0x%x", sep, oa->oa_io[i].or_addr);
			if (oa->oa_io[i].or_size > 1)
				aprint_normal("-0x%x", oa->oa_io[i].or_addr +
				    oa->oa_io[i].or_size - 1);
			sep = ",";
		}
	}

	if (oa->oa_niomem) {
		sep = "";
		aprint_normal(" iomem ");
		for (i = 0; i < oa->oa_niomem; i++) {
			if (oa->oa_iomem[i].or_size == 0)
				continue;
			aprint_normal("%s0x%x", sep, oa->oa_iomem[i].or_addr);
			if (oa->oa_iomem[i].or_size > 1)
				aprint_normal("-0x%x", oa->oa_iomem[i].or_addr +
				    oa->oa_iomem[i].or_size - 1);
			sep = ",";
		}
	}

	if (oa->oa_nirq) {
		sep = "";
		aprint_normal(" irq ");
		for (i = 0; i < oa->oa_nirq; i++) {
			if (oa->oa_irq[i].or_irq == IRQUNK)
				continue;
			aprint_normal("%s%d", sep, oa->oa_irq[i].or_irq);
			sep = ",";
		}
	}

	return (UNCONF);
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
obio_intr_establish(int irq, int level, int (*ih_fun)(void *), void *ih_arg)
{

	return extintr_establish(irq, level, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
obio_intr_disestablish(void *arg)
{

	extintr_disestablish(arg);
}

/*
 * on-board I/O bus space
 */
#define	OBIO_IOMEM_IO		0	/* space is i/o space */
#define	OBIO_IOMEM_MEM		1	/* space is mem space */
#define	OBIO_IOMEM_PCMCIA_IO	2	/* PCMCIA IO space */
#define	OBIO_IOMEM_PCMCIA_MEM	3	/* PCMCIA Mem space */
#define	OBIO_IOMEM_PCMCIA_ATT	4	/* PCMCIA Attr space */
#define	OBIO_IOMEM_PCMCIA_8BIT	0x8000	/* PCMCIA BUS 8 BIT WIDTH */
#define	OBIO_IOMEM_PCMCIA_IO8 \
	    (OBIO_IOMEM_PCMCIA_IO|OBIO_IOMEM_PCMCIA_8BIT)
#define	OBIO_IOMEM_PCMCIA_MEM8 \
	    (OBIO_IOMEM_PCMCIA_MEM|OBIO_IOMEM_PCMCIA_8BIT)
#define	OBIO_IOMEM_PCMCIA_ATT8 \
	    (OBIO_IOMEM_PCMCIA_ATT|OBIO_IOMEM_PCMCIA_8BIT)

int obio_iomem_map(void *v, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp);
void obio_iomem_unmap(void *v, bus_space_handle_t bsh, bus_size_t size);
int obio_iomem_subregion(void *v, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp);
int obio_iomem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp);
void obio_iomem_free(void *v, bus_space_handle_t bsh, bus_size_t size);
paddr_t obio_iomem_mmap(void *v, bus_addr_t addr, off_t off, int prot,
    int flags);

static int obio_iomem_add_mapping(bus_addr_t, bus_size_t, int,
    bus_space_handle_t *);

static int
obio_iomem_add_mapping(bus_addr_t bpa, bus_size_t size, int type,
    bus_space_handle_t *bshp)
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;
	unsigned int m = 0;
	int io_type = type & ~OBIO_IOMEM_PCMCIA_8BIT;

	pa = sh3_trunc_page(bpa);
	endpa = sh3_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("obio_iomem_add_mapping: overflow");
#endif

	va = uvm_km_alloc(kernel_map, endpa - pa, 0, UVM_KMF_VAONLY);
	if (va == 0){
		printf("obio_iomem_add_mapping: nomem\n");
		return (ENOMEM);
	}

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

#define MODE(t, s)							\
	((t) & OBIO_IOMEM_PCMCIA_8BIT) ?				\
		_PG_PCMCIA_ ## s ## 8 :					\
		_PG_PCMCIA_ ## s ## 16
	switch (io_type) {
	default:
		panic("unknown pcmcia space.");
		/* NOTREACHED */
	case OBIO_IOMEM_PCMCIA_IO:
		m = MODE(type, IO);
		break;
	case OBIO_IOMEM_PCMCIA_MEM:
		m = MODE(type, MEM);
		break;
	case OBIO_IOMEM_PCMCIA_ATT:
		m = MODE(type, ATTR);
		break;
	}
#undef MODE

	for (; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE) {
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE);
		pte = __pmap_kpte_lookup(va);
		KDASSERT(pte);
		*pte |= m;  /* PTEA PCMCIA assistant bit */
		sh_tlb_update(0, va, *pte);
	}

	return (0);
}

int
obio_iomem_map(void *v, bus_addr_t bpa, bus_size_t size,
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
	error = obio_iomem_add_mapping(addr, size, (int)(u_long)v, bshp);

	return (error);
}

void
obio_iomem_unmap(void *v, bus_space_handle_t bsh, bus_size_t size)
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
		panic("obio_io_unmap: overflow");
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
obio_iomem_subregion(void *v, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;

	return (0);
}

int
obio_iomem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	*bshp = *bpap = rstart;

	return (0);
}

void
obio_iomem_free(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	obio_iomem_unmap(v, bsh, size);
}

paddr_t
obio_iomem_mmap(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{

	return (paddr_t)-1;
}

/*
 * on-board I/O bus space read/write
 */
uint8_t obio_iomem_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint16_t obio_iomem_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint32_t obio_iomem_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset);
void obio_iomem_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void obio_iomem_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void obio_iomem_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);
void obio_iomem_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void obio_iomem_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void obio_iomem_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);
void obio_iomem_write_1(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t value);
void obio_iomem_write_2(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t value);
void obio_iomem_write_4(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t value);
void obio_iomem_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void obio_iomem_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void obio_iomem_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);
void obio_iomem_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void obio_iomem_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void obio_iomem_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);
void obio_iomem_set_multi_1(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t val, bus_size_t count);
void obio_iomem_set_multi_2(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t val, bus_size_t count);
void obio_iomem_set_multi_4(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t val, bus_size_t count);
void obio_iomem_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count);
void obio_iomem_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count);
void obio_iomem_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count);
void obio_iomem_copy_region_1(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count);
void obio_iomem_copy_region_2(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count);
void obio_iomem_copy_region_4(void *v, bus_space_handle_t h1, bus_size_t o1,
    bus_space_handle_t h2, bus_size_t o2, bus_size_t count);

struct _bus_space obio_bus_io =
{
	.bs_cookie = (void *)OBIO_IOMEM_PCMCIA_IO,

	.bs_map = obio_iomem_map,
	.bs_unmap = obio_iomem_unmap,
	.bs_subregion = obio_iomem_subregion,

	.bs_alloc = obio_iomem_alloc,
	.bs_free = obio_iomem_free,

	.bs_mmap = obio_iomem_mmap,

	.bs_r_1 = obio_iomem_read_1,
	.bs_r_2 = obio_iomem_read_2,
	.bs_r_4 = obio_iomem_read_4,

	.bs_rm_1 = obio_iomem_read_multi_1,
	.bs_rm_2 = obio_iomem_read_multi_2,
	.bs_rm_4 = obio_iomem_read_multi_4,

	.bs_rr_1 = obio_iomem_read_region_1,
	.bs_rr_2 = obio_iomem_read_region_2,
	.bs_rr_4 = obio_iomem_read_region_4,

	.bs_w_1 = obio_iomem_write_1,
	.bs_w_2 = obio_iomem_write_2,
	.bs_w_4 = obio_iomem_write_4,

	.bs_wm_1 = obio_iomem_write_multi_1,
	.bs_wm_2 = obio_iomem_write_multi_2,
	.bs_wm_4 = obio_iomem_write_multi_4,

	.bs_wr_1 = obio_iomem_write_region_1,
	.bs_wr_2 = obio_iomem_write_region_2,
	.bs_wr_4 = obio_iomem_write_region_4,

	.bs_sm_1 = obio_iomem_set_multi_1,
	.bs_sm_2 = obio_iomem_set_multi_2,
	.bs_sm_4 = obio_iomem_set_multi_4,

	.bs_sr_1 = obio_iomem_set_region_1,
	.bs_sr_2 = obio_iomem_set_region_2,
	.bs_sr_4 = obio_iomem_set_region_4,

	.bs_c_1 = obio_iomem_copy_region_1,
	.bs_c_2 = obio_iomem_copy_region_2,
	.bs_c_4 = obio_iomem_copy_region_4,
};

struct _bus_space obio_bus_mem =
{
	.bs_cookie = (void *)OBIO_IOMEM_PCMCIA_MEM,

	.bs_map = obio_iomem_map,
	.bs_unmap = obio_iomem_unmap,
	.bs_subregion = obio_iomem_subregion,

	.bs_alloc = obio_iomem_alloc,
	.bs_free = obio_iomem_free,

	.bs_r_1 = obio_iomem_read_1,
	.bs_r_2 = obio_iomem_read_2,
	.bs_r_4 = obio_iomem_read_4,

	.bs_rm_1 = obio_iomem_read_multi_1,
	.bs_rm_2 = obio_iomem_read_multi_2,
	.bs_rm_4 = obio_iomem_read_multi_4,

	.bs_rr_1 = obio_iomem_read_region_1,
	.bs_rr_2 = obio_iomem_read_region_2,
	.bs_rr_4 = obio_iomem_read_region_4,

	.bs_w_1 = obio_iomem_write_1,
	.bs_w_2 = obio_iomem_write_2,
	.bs_w_4 = obio_iomem_write_4,

	.bs_wm_1 = obio_iomem_write_multi_1,
	.bs_wm_2 = obio_iomem_write_multi_2,
	.bs_wm_4 = obio_iomem_write_multi_4,

	.bs_wr_1 = obio_iomem_write_region_1,
	.bs_wr_2 = obio_iomem_write_region_2,
	.bs_wr_4 = obio_iomem_write_region_4,

	.bs_sm_1 = obio_iomem_set_multi_1,
	.bs_sm_2 = obio_iomem_set_multi_2,
	.bs_sm_4 = obio_iomem_set_multi_4,

	.bs_sr_1 = obio_iomem_set_region_1,
	.bs_sr_2 = obio_iomem_set_region_2,
	.bs_sr_4 = obio_iomem_set_region_4,

	.bs_c_1 = obio_iomem_copy_region_1,
	.bs_c_2 = obio_iomem_copy_region_2,
	.bs_c_4 = obio_iomem_copy_region_4,
};

/* read */
uint8_t
obio_iomem_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset)
{

	return *(volatile uint8_t *)(bsh + offset);
}

uint16_t
obio_iomem_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset)
{

	return *(volatile uint16_t *)(bsh + offset);
}

uint32_t
obio_iomem_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset)
{

	return *(volatile uint32_t *)(bsh + offset);
}

void
obio_iomem_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p;
	}
}

void
obio_iomem_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p;
	}
}

void
obio_iomem_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p;
	}
}

void
obio_iomem_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p++;
	}
}

void
obio_iomem_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p++;
	}
}

void
obio_iomem_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*addr++ = *p++;
	}
}

/* write */
void
obio_iomem_write_1(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint8_t value)
{

	*(volatile uint8_t *)(bsh + offset) = value;
}

void
obio_iomem_write_2(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint16_t value)
{

	*(volatile uint16_t *)(bsh + offset) = value;
}

void
obio_iomem_write_4(void *v, bus_space_handle_t bsh, bus_size_t offset,
    uint32_t value)
{

	*(volatile uint32_t *)(bsh + offset) = value;
}

void
obio_iomem_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = *addr++;
	}
}

void
obio_iomem_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = *addr++;
	}
}

void
obio_iomem_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = *addr++;
	}
}

void
obio_iomem_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*p++ = *addr++;
	}
}

void
obio_iomem_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*p++ = *addr++;
	}
}

void
obio_iomem_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*p++ = *addr++;
	}
}

void
obio_iomem_set_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = val;
	}
}

void
obio_iomem_set_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = val;
	}
}

void
obio_iomem_set_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *p = (void *)(bsh + offset);

	while (count--) {
		*p = val;
	}
}

void
obio_iomem_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count)
{
	volatile uint8_t *addr = (void *)(bsh + offset);

	while (count--) {
		*addr++ = val;
	}
}

void
obio_iomem_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count)
{
	volatile uint16_t *addr = (void *)(bsh + offset);

	while (count--) {
		*addr++ = val;
	}
}

void
obio_iomem_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count)
{
	volatile uint32_t *addr = (void *)(bsh + offset);

	while (count--) {
		*addr++ = val;
	}
}

void
obio_iomem_copy_region_1(void *v, bus_space_handle_t h1, bus_size_t o1,
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
obio_iomem_copy_region_2(void *v, bus_space_handle_t h1, bus_size_t o1,
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
obio_iomem_copy_region_4(void *v, bus_space_handle_t h1, bus_size_t o1,
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
