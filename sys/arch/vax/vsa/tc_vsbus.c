/*	$NetBSD: tc_vsbus.c,v 1.7.12.1 2017/12/03 11:36:48 jdolecek Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
__KERNEL_RCSID(0, "$NetBSD: tc_vsbus.c,v 1.7.12.1 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/scb.h>
#include <machine/sid.h>
#include <machine/vsbus.h>

#include <dev/tc/tcvar.h>

#define NSLOTS	1

struct tc_vsbus_softc {
	struct tc_softc sc_tc;
	struct tc_slotdesc sc_slots[NSLOTS];
	struct vax_bus_dma_tag sc_dmatag;
	struct vax_sgmap sc_sgmap;
	struct evcnt sc_ev;
	int (*sc_intr_func)(void *);
	void *sc_intr_arg;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh_csr;
	int sc_cvec;
};

static int tc_vsbus_match(device_t, cfdata_t, void *);
static void tc_vsbus_attach(device_t, device_t, void *);

static int tc_vsbus_dma_init(device_t);
static bus_dma_tag_t tc_vsbus_get_dma_tag(int);

static void tc_vsbus_intr(void *);
static void tc_vsbus_intr_establish(device_t, void *, int, int (*)(void *),
    void *);
static void tc_vsbus_intr_disestablish(device_t, void *);
static const struct evcnt *tc_vsbus_intr_evcnt(device_t, void *);

static int vax_tc_bus_space_map(void *, bus_addr_t, bus_size_t, int,
    bus_space_handle_t *, int);
static int vax_tc_bus_space_subregion(void *, bus_space_handle_t, bus_size_t,
    bus_size_t, bus_space_handle_t *);
static void vax_tc_bus_space_unmap(void *, bus_space_handle_t, bus_size_t, int);
static int vax_tc_bus_space_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t,
    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
static void vax_tc_bus_space_free(void *, bus_space_handle_t, bus_size_t);
static paddr_t vax_tc_bus_space_mmap(void *, bus_addr_t, off_t, int, int);

CFATTACH_DECL_NEW(tc_vsbus, sizeof(struct tc_vsbus_softc),
    tc_vsbus_match, tc_vsbus_attach, 0, 0);

static bus_dma_tag_t tc_vsbus_dmat;

struct vax_bus_space vax_tc_bus_space = {
	NULL,
	vax_tc_bus_space_map,
	vax_tc_bus_space_unmap,
	vax_tc_bus_space_subregion,
	vax_tc_bus_space_alloc,
	vax_tc_bus_space_free,
	vax_tc_bus_space_mmap,
};

/*
 * taken from KA46 System Board Specification, KA46-0-DBF, Rev. X.02,
 * 10 October 1990
 */
#define KA46_BWF0			0x20080014
#define   KA46_BWF0_ADP			__BIT(31)
#define KA46_BWF0_SZ			4

/*
 * taken from KA49 Processor Module Specification V 1.1, 20 August 1992
 */
#define KA49_CFG			0x25800000
#define   KA49_CFG_BA			__BIT(0)
#define KA49_CFG_SZ			2

/*
 * taken from Pmariah TURBOchannel Adapter Specification, 29 November 1991
 */
#define KA4x_TCA_BASE			0x30000000
#define KA4x_TCA_DIAG_TRIG		(KA4x_TCA_BASE + 0x4000000)
#define KA4x_TCA_MAP_CHK_SEQ		(KA4x_TCA_BASE + 0x4800000)
#define KA4x_TCA_FIFO_DIAG		(KA4x_TCA_BASE + 0x5000000)
#define KA4x_TCA_SGMAP			(KA4x_TCA_BASE + 0x5800000)
#define KA4x_TCA_DIAG_ROM		(KA4x_TCA_BASE + 0x6000000)
#define KA4x_TCA_CSR			(KA4x_TCA_BASE + 0x6800000)
#define   KA4x_TCA_CSR_BLK_SZ		__BITS(0, 2)	/* 0x00007 RW   */
#define   KA4x_TCA_CSR_SPARE		__BIT(3)	/* 0x00008 RW   */
#define   KA4x_TCA_CSR_BAD_PAR		__BITS(4, 7)	/* 0x000f0 RW   */
#define   KA4x_TCA_CSR_RST_TC		__BIT(8)	/* 0x00100 RW   */
#define   KA4x_TCA_CSR_EN_MAP		__BIT(9)	/* 0x00200 RW   */
#define   KA4x_TCA_CSR_INVAL_REF	__BIT(10)	/* 0x00400 RW1C */
#define   KA4x_TCA_CSR_TC_TMO		__BIT(11)	/* 0x00800 RW   */
#define   KA4x_TCA_CSR_EN_TC_IRQ	__BIT(12)	/* 0x01000 RW   */
#define   KA4x_TCA_CSR_TC_IRQ		__BIT(13)	/* 0x02000 R    */
#define   KA4x_TCA_CSR_ERR		__BIT(14)	/* 0x04000 RW1C */
#define   KA4x_TCA_CSR_ALT_CYC_ST	__BIT(15)	/* 0x08000 RW   */
#define   KA4x_TCA_CSR_EN_PAR		__BIT(16)	/* 0x10000 RW   */
#define   KA4x_TCA_CSR_FIFO_EMPTY	__BIT(17)	/* 0x20000 R    */
#define KA4x_TCA_CSR_SZ			4

static int
tc_vsbus_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct vsbus_attach_args * const va = aux;
	uint32_t *csr;
	bus_space_tag_t bst = va->va_memt;
	bus_space_handle_t bsh;
	int found, rc;

	if (va->va_paddr != KA4x_TCA_CSR)
		return 0;

	/* Bus adaptor present? */
	switch (vax_boardtype) {
	case VAX_BTYP_46:
		if (bus_space_map(bst, KA46_BWF0, KA46_BWF0_SZ, 0, &bsh))
			return 0;
		found = ((bus_space_read_4(bst, bsh, 0) & KA46_BWF0_ADP) != 0);
		bus_space_unmap(bst, bsh, KA46_BWF0_SZ);
		/*
		 * On VS4000/60, although interrupting on a real vector, fool
		 * vsbus interrupt, as no interrupt bit will be set in
		 * vsbus_softc's sc_intreq for TC adaptor.
		 */
		rc = 20;
		break;
	case VAX_BTYP_49:
		if (bus_space_map(bst, KA49_CFG, KA49_CFG_SZ, 0, &bsh))
			return 0;
		found = ((bus_space_read_2(bst, bsh, 0) & KA49_CFG_BA) != 0);
		bus_space_unmap(bst, bsh, KA49_CFG_SZ);
		rc = 10;
		break;
	default:
		return 0;
	}
	if (!found)
		return 0;

	/* XXX Assume a found bus adaptor is the TC bus adaptor. */

	/* Force interrupt. */
	csr = (uint32_t *)va->va_addr;
	*csr |= KA4x_TCA_CSR_TC_TMO;
	DELAY(10000);
	*csr &= ~KA4x_TCA_CSR_TC_TMO;
	DELAY(10000);

	return rc;
}

#define INIT_SLOTSZ	1

static void
tc_vsbus_attach(device_t parent, device_t self, void *aux)
{
	struct tcbus_attach_args tba;
	struct vsbus_attach_args * const va = aux;
	struct tc_vsbus_softc * const sc = device_private(self);
	struct tc_rommap *rommap;
	bus_space_tag_t bst = va->va_memt;
	bus_space_handle_t bsh_csr, bsh_slot;
	const bus_size_t slotb = 4194304;
	uint32_t csr;
	int error, slotsz;

	sc->sc_cvec = va->va_cvec;

	error = bus_space_map(bst, KA4x_TCA_CSR, KA4x_TCA_CSR_SZ, 0, &bsh_csr);
	if (error) {
		aprint_normal(": failed to map TCA CSR: %d\n", error);
		return;
	}
	sc->sc_bst = bst;
	sc->sc_bsh_csr = bsh_csr;

	/* Deassert TC option reset and clean up. */
	csr = bus_space_read_4(bst, bsh_csr, 0);
	csr &= ~(KA4x_TCA_CSR_TC_TMO | KA4x_TCA_CSR_RST_TC);
	csr |= KA4x_TCA_CSR_ERR | KA4x_TCA_CSR_INVAL_REF;
	bus_space_write_4(bst, bsh_csr, 0, csr);

	/*
	 * Map initial number of "slots" (4 MB each) to read the option ROM
	 * header.
	 */
	error = bus_space_map(bst, KA4x_TCA_BASE, INIT_SLOTSZ * slotb,
	    BUS_SPACE_MAP_LINEAR, &bsh_slot);
	if (error) {
		aprint_normal(": failed to map TC slot: %d", error);
		goto fail;
	}
	/* Determine number of slots required from option ROM header. */
	slotsz = 0;
	if (tc_checkslot((tc_addr_t)bus_space_vaddr(bst, bsh_slot), NULL,
	    &rommap))
		slotsz = rommap->tcr_ssize.v;
	if (slotsz == 0) {
		/* Invalid option ROM header or no option present. */
		bus_space_unmap(bst, bsh_slot, INIT_SLOTSZ * slotb);
		goto fail;
	} else if (slotsz > INIT_SLOTSZ) {
		/* Remap with actual slot size required. */
		bus_space_unmap(bst, bsh_slot, INIT_SLOTSZ * slotb);
		error = bus_space_map(bst, KA4x_TCA_BASE, slotsz * slotb,
		    BUS_SPACE_MAP_LINEAR, &bsh_slot);
		if (error) {
			aprint_normal(": failed to map TC slot: %d", error);
			goto fail;
		}
	} else
		slotsz = INIT_SLOTSZ;

	/* Pass pre-mapped space for TC drivers not bus_space'ified yet. */
	sc->sc_slots[0].tcs_addr = (tc_addr_t)bus_space_vaddr(bst, bsh_slot);
	sc->sc_slots[0].tcs_cookie = sc;
	sc->sc_slots[0].tcs_used = 0;

	tba.tba_busname = "tc";
	/* Tag with custom methods for pre-mapped bus_space. */
	tba.tba_memt = &vax_tc_bus_space;
	tba.tba_speed = TC_SPEED_12_5_MHZ;
	tba.tba_nslots = __arraycount(sc->sc_slots);
	tba.tba_slots = sc->sc_slots;
	tba.tba_nbuiltins = 0;
	tba.tba_intr_evcnt = tc_vsbus_intr_evcnt;
	tba.tba_intr_establish = tc_vsbus_intr_establish;
	tba.tba_intr_disestablish = tc_vsbus_intr_disestablish;
	tba.tba_get_dma_tag = tc_vsbus_get_dma_tag;

	error = tc_vsbus_dma_init(self);
	if (error) {
		aprint_normal(": failed to init DMA: %d", error);
		bus_space_unmap(bst, bsh_slot, slotsz * slotb);
		goto fail;
	}

	evcnt_attach_dynamic(&sc->sc_ev, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "intr");

	/* Enable SGDMA and option IRQ now. */
	csr = bus_space_read_4(bst, bsh_csr, 0);
	csr &= ~(KA4x_TCA_CSR_TC_TMO | KA4x_TCA_CSR_RST_TC);
	csr |= KA4x_TCA_CSR_ERR | KA4x_TCA_CSR_EN_TC_IRQ |
	    KA4x_TCA_CSR_INVAL_REF | KA4x_TCA_CSR_EN_MAP;
	bus_space_write_4(bst, bsh_csr, 0, csr);

	/* XXX: why not config_found(9)?? */
	tcattach(parent, self, &tba);

	return;

fail:
	aprint_normal("\n");
	/* Clear possible timeout bit which asserts TC interrupt. */
	csr = bus_space_read_4(bst, bsh_csr, 0);
	csr &= ~KA4x_TCA_CSR_TC_TMO;
	bus_space_write_4(bst, bsh_csr, 0, csr);
	bus_space_unmap(bst, bsh_csr, KA4x_TCA_CSR_SZ);
}

static int
tc_vsbus_dma_init(device_t dev)
{
	struct tc_vsbus_softc * const sc = device_private(dev);
	struct pte *pte;
	bus_dma_tag_t dmat = &sc->sc_dmatag;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh;
	const bus_size_t ptecnt = 8192;
	const bus_size_t mapsize = ptecnt * sizeof(pte[0]);
	int error;

	vax_sgmap_dmatag_init(dmat, sc, ptecnt);

	dmat->_sgmap = &sc->sc_sgmap;

	error = bus_space_map(bst, KA4x_TCA_SGMAP, mapsize,
	    BUS_SPACE_MAP_LINEAR, &bsh);
	if (error)
		return error;
	bus_space_set_region_4(bst, bsh, 0, 0, ptecnt);
	pte = bus_space_vaddr(bst, bsh);

	/* Initialize the SGMAP. */
	vax_sgmap_init(dmat, &sc->sc_sgmap, "tc_sgmap", dmat->_wbase,
	    dmat->_wsize, pte, 0);

	tc_vsbus_dmat = dmat;

	return 0;
}

static bus_dma_tag_t
tc_vsbus_get_dma_tag(int slotno)
{

	return tc_vsbus_dmat;
}

static void
tc_vsbus_intr(void *arg)
{
	struct tc_vsbus_softc * const sc = arg;
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh_csr;
	uint32_t csr;

	sc->sc_ev.ev_count++;

	csr = bus_space_read_4(bst, bsh, 0);
	if (__predict_true((csr & KA4x_TCA_CSR_TC_IRQ) == 0))	/* active low */
		sc->sc_intr_func(sc->sc_intr_arg);

	/* Clear possible timeout bit which asserts TC interrupt. */
	csr = bus_space_read_4(bst, bsh, 0);
	csr &= ~KA4x_TCA_CSR_TC_TMO;
	bus_space_write_4(bst, bsh, 0, csr);
}

static void
tc_vsbus_intr_establish(device_t dv, void *cookie, int level,
    int (*func)(void *), void *arg)
{
	struct tc_vsbus_softc * const sc = cookie;

	sc->sc_intr_func = func;
	sc->sc_intr_arg = arg;

	scb_vecalloc(sc->sc_cvec, tc_vsbus_intr, sc, SCB_ISTACK, &sc->sc_ev);
}

static void
tc_vsbus_intr_disestablish(device_t dv, void *cookie)
{

	/* Do nothing. */
}

static const struct evcnt *
tc_vsbus_intr_evcnt(device_t dv, void *cookie)
{
	struct tc_vsbus_softc * const sc = device_private(dv);

	return &sc->sc_ev;
}

static int
vax_tc_bus_space_map(void *t, bus_addr_t pa, bus_size_t size, int cacheable,
    bus_space_handle_t *bshp, int f2)
{

	/* bus_space is pre-mapped, so "pa" is a virtual address already. */
	*bshp = pa;
	return 0;
}

static int
vax_tc_bus_space_subregion(void *t, bus_space_handle_t h, bus_size_t o,
    bus_size_t s, bus_space_handle_t *hp)
{

	*hp = h + o;
	return 0;
}

static void
vax_tc_bus_space_unmap(void *t, bus_space_handle_t h, bus_size_t size, int f)
{

	/* Do nothing. */
}

static int
vax_tc_bus_space_alloc(void *t, bus_addr_t rs, bus_addr_t re, bus_size_t s,
    bus_size_t a, bus_size_t b, int f, bus_addr_t *ap, bus_space_handle_t *hp)
{

	panic("vax_tc_bus_space_alloc not implemented");
}

static void
vax_tc_bus_space_free(void *t, bus_space_handle_t h, bus_size_t s)
{

	panic("vax_tc_bus_space_free not implemented");
}

static paddr_t
vax_tc_bus_space_mmap(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{
	bus_addr_t rv;

	rv = addr + off;
	return btop(rv);
}
