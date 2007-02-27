/* 	$NetBSD: design_gsrd2.c,v 1.1.8.1 2007/02/27 16:50:14 yamt Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include "opt_virtex.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: design_gsrd2.c,v 1.1.8.1 2007/02/27 16:50:14 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/powerpc.h>
#include <machine/tlb.h>

#include <powerpc/ibm4xx/dev/plbvar.h>

#include <evbppc/virtex/dev/xcvbusvar.h>
#include <evbppc/virtex/dev/cdmacreg.h>
#include <evbppc/virtex/dev/temacreg.h>
#include <evbppc/virtex/dev/tftreg.h>

#include <evbppc/virtex/virtex.h>
#include <evbppc/virtex/dcr.h>


#define	DCR_TEMAC_BASE 		0x0030
#define	DCR_TFT0_BASE 		0x0082
#define	DCR_TFT1_BASE 		0x0086
#define	DCR_CDMAC_BASE 		0x0140

#define OPB_BASE 		0x80000000 	/* below are offsets in opb */
#define OPB_XLCOM_BASE 		0x010000
#define OPB_GPIO_BASE 		0x020000
#define OPB_PSTWO0_BASE 	0x040000
#define OPB_PSTWO1_BASE 	0x041000
#define CDMAC_NCHAN 		2 	/* cdmac {Tx,Rx} */
#define CDMAC_INTR_LINE 	0

#define	TFT_FB_BASE 		0x3c00000
#define TFT_FB_SIZE 		(2*1024*1024)

/*
 * CDMAC per-channel interrupt handler. CDMAC has one interrupt signal
 * per two channels on mpmc2, so we have to dispatch channels manually.
 *
 * Note: we hardwire priority to IPL_NET, temac(4) is the only device that
 * needs to service DMA interrupts anyway.
 */
typedef struct cdmac_intrhand {
	void 			(*cih_func)(void *);
	void 			*cih_arg;
} *cdmac_intrhand_t;

/* Two instantiated channels, one logical interrupt per direction. */
static struct cdmac_intrhand 	cdmacintr[CDMAC_NCHAN];
static void 			*cdmac_ih;


/*
 * DCR bus space leaf access routines.
 */

static void
tft0_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_TFT0_BASE, TFT_CTRL);
	WCASE(DCR_TFT0_BASE, TFT_ADDR);
	WDEAD(addr);
	}
}

static uint32_t
tft0_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_TFT0_BASE, TFT_CTRL);
	RCASE(DCR_TFT0_BASE, TFT_ADDR);
	RDEAD(addr);
	}

	return (val);
}

static void
tft1_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_TFT1_BASE, TFT_CTRL);
	WCASE(DCR_TFT0_BASE, TFT_ADDR);
	WDEAD(addr);
	}
}

static uint32_t
tft1_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_TFT1_BASE, TFT_CTRL);
	RCASE(DCR_TFT0_BASE, TFT_ADDR);
	RDEAD(addr);
	}

	return (val);
}

#define DOCHAN(op, base, channel) \
	op(base, channel + CDMAC_NEXT); 	\
	op(base, channel + CDMAC_CURADDR); 	\
	op(base, channel + CDMAC_CURSIZE); 	\
	op(base, channel + CDMAC_CURDESC)

static void
cdmac_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_CDMAC_BASE, CDMAC_STAT_BASE(0)); 	/* Tx engine */
	WCASE(DCR_CDMAC_BASE, CDMAC_STAT_BASE(1)); 	/* Rx engine */
	WCASE(DCR_CDMAC_BASE, CDMAC_INTR);
	DOCHAN(WCASE, DCR_CDMAC_BASE, CDMAC_CTRL_BASE(0));
	DOCHAN(WCASE, DCR_CDMAC_BASE, CDMAC_CTRL_BASE(1));
	WDEAD(addr);
	}
}

static uint32_t
cdmac_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_CDMAC_BASE, CDMAC_STAT_BASE(0)); 	/* Tx engine */
	RCASE(DCR_CDMAC_BASE, CDMAC_STAT_BASE(1)); 	/* Rx engine */
	RCASE(DCR_CDMAC_BASE, CDMAC_INTR);
	DOCHAN(RCASE, DCR_CDMAC_BASE, CDMAC_CTRL_BASE(0));
	DOCHAN(RCASE, DCR_CDMAC_BASE, CDMAC_CTRL_BASE(1));
	RDEAD(addr);
	}

	return (val);
}

#undef DOCHAN

static void
temac_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_TEMAC_BASE, TEMAC_RESET);
	WDEAD(addr);
	}
}

static uint32_t
temac_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_TEMAC_BASE, TEMAC_RESET);
	RDEAD(addr);
	}

	return (val);
}

static const struct powerpc_bus_space cdmac_bst = {
	DCR_BST_BODY(DCR_CDMAC_BASE, cdmac_read_4, cdmac_write_4)
};

static const struct powerpc_bus_space temac_bst = {
	DCR_BST_BODY(DCR_TEMAC_BASE, temac_read_4, temac_write_4)
};

static const struct powerpc_bus_space tft0_bst = {
	DCR_BST_BODY(DCR_TFT0_BASE, tft0_read_4, tft0_write_4)
};

static const struct powerpc_bus_space tft1_bst = {
	DCR_BST_BODY(DCR_TFT1_BASE, tft1_read_4, tft1_write_4)
};

static struct powerpc_bus_space opb_bst = {
	.pbs_flags 	= _BUS_SPACE_BIG_ENDIAN|_BUS_SPACE_MEM_TYPE,
	.pbs_base 	= 0 /*OPB_BASE*/,
	.pbs_offset 	= OPB_BASE,
};

static char opb_extent_storage[EXTENT_FIXED_STORAGE_SIZE(8)] __aligned(8);

/*
 * Master device configuration table for GSRD2 design.
 */
static const struct gsrddev {
	const char 		*gdv_name;
	const char 		*gdv_attr;
	bus_space_tag_t 	gdv_bst;
	bus_addr_t 		gdv_addr;
	int 			gdv_intr;
	int 			gdv_rx_dma;
	int 			gdv_tx_dma;
	int 			gdv_dcr; 		/* XXX bst flag */
} gsrd_devices[] = {
	{			/* gsrd_devices[0] */
		.gdv_name 	= "xlcom",
		.gdv_attr 	= "xcvbus",
		.gdv_bst 	= &opb_bst,
		.gdv_addr 	= OPB_XLCOM_BASE,
		.gdv_intr 	= 2,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= -1,
		.gdv_dcr 	= 0,
	},
	{			/* gsrd_devices[1] */
		.gdv_name 	= "temac",
		.gdv_attr 	= "xcvbus",
		.gdv_bst 	= &temac_bst,
		.gdv_addr 	= 0,
		.gdv_intr 	= 1, 		/* unused MII intr */
		.gdv_rx_dma 	= 1, 		/* cdmac Rx */
		.gdv_tx_dma 	= 0, 		/* cdmac Tx */
		.gdv_dcr 	= 1,
	},
#ifndef DESIGN_DFC
	{			/* gsrd_devices[2] */
		.gdv_name 	= "tft",
		.gdv_attr 	= "plbus",
		.gdv_bst 	= &tft0_bst,
		.gdv_addr 	= 0,
		.gdv_intr 	= -1,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= -1,
		.gdv_dcr 	= 1,
	},
#endif
	{			/* gsrd_devices[2] */
		.gdv_name 	= "tft",
		.gdv_attr 	= "plbus",
		.gdv_bst 	= &tft1_bst,
		.gdv_addr 	= 0,
		.gdv_intr 	= -1,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= -1,
		.gdv_dcr 	= 1,
	},
#ifdef DESIGN_DFC
	{			/* gsrd_devices[3] */
		.gdv_name 	= "pstwo",
		.gdv_attr 	= "xcvbus",
		.gdv_bst 	= &opb_bst,
		.gdv_addr 	= OPB_PSTWO0_BASE,
		.gdv_intr 	= 3,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= -1,
		.gdv_dcr 	= 0,
	},
	{			/* gsrd_devices[4] */
		.gdv_name 	= "pstwo",
		.gdv_attr 	= "xcvbus",
		.gdv_bst 	= &opb_bst,
		.gdv_addr 	= OPB_PSTWO1_BASE,
		.gdv_intr 	= 4,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= -1,
		.gdv_dcr 	= 0,
	},
#endif
};

static struct ll_dmac *
virtex_mpmc_mapdma(int idx, struct ll_dmac *chan)
{
	if (idx == -1)
		return (NULL);

	KASSERT(idx >= 0 && idx < CDMAC_NCHAN);

	chan->dmac_iot = &cdmac_bst;
	chan->dmac_ctrl_addr = CDMAC_CTRL_BASE(idx);
	chan->dmac_stat_addr = CDMAC_STAT_BASE(idx);
	chan->dmac_chan = idx;

	return (chan);
}

static int
cdmac_intr(void *arg)
{
	uint32_t 		isr;
	int 			did = 0;

	isr = bus_space_read_4(&cdmac_bst, 0, CDMAC_INTR);

	if (ISSET(isr, CDMAC_INTR_TX0) && cdmacintr[0].cih_func) {
		(cdmacintr[0].cih_func)(cdmacintr[0].cih_arg);
		did++;
	}
	if (ISSET(isr, CDMAC_INTR_RX0) && cdmacintr[1].cih_func) {
		(cdmacintr[1].cih_func)(cdmacintr[1].cih_arg);
		did++;
	}

	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, isr); 	/* ack */

	/* XXX This still happens all the time under load. */
#if 0
	if (did == 0)
		aprint_normal("WARNING: stray cdmac isr 0x%x\n", isr);
#endif
	return (0);
}

/*
 * Public interface.
 */

void
virtex_autoconf(device_t self, struct plb_attach_args *paa)
{

	struct xcvbus_attach_args 	vaa;
	struct ll_dmac 			rx, tx;
	int 				i;

	/* Reset DMA channels. */
	bus_space_write_4(&cdmac_bst, 0, CDMAC_STAT_BASE(0), CDMAC_STAT_RESET);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_STAT_BASE(1), CDMAC_STAT_RESET);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, 0);

	vaa.vaa_dmat = paa->plb_dmat;

	for (i = 0; i < __arraycount(gsrd_devices); i++) {
		const struct gsrddev 	*g = &gsrd_devices[i];

		vaa._vaa_is_dcr = g->gdv_dcr; 	/* XXX bst flag */
		vaa.vaa_name 	= g->gdv_name;
		vaa.vaa_addr 	= g->gdv_addr;
		vaa.vaa_intr 	= g->gdv_intr;
		vaa.vaa_iot 	= g->gdv_bst;

		vaa.vaa_rx_dmac = virtex_mpmc_mapdma(g->gdv_rx_dma, &rx);
		vaa.vaa_tx_dmac = virtex_mpmc_mapdma(g->gdv_tx_dma, &tx);

		config_found_ia(self, g->gdv_attr, &vaa, xcvbus_print);
	}

	/* Setup the dispatch handler. */
	cdmac_ih = intr_establish(CDMAC_INTR_LINE, IST_LEVEL, IPL_NET,
	    cdmac_intr, NULL);
	if (cdmac_ih == NULL)
		panic("virtex_mpmc_done: could not establish cdmac intr");

	/* Clear (XXX?) and enable interrupts. */
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, ~CDMAC_INTR_MIE);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, CDMAC_INTR_MIE);
}

void *
ll_dmac_intr_establish(int chan, void (*handler)(void *), void *arg)
{
	KASSERT(chan >= 0 && chan < CDMAC_NCHAN);
	KASSERT(cdmacintr[chan].cih_func == NULL);
	KASSERT(cdmacintr[chan].cih_arg == NULL);

	cdmacintr[chan].cih_func = handler;
	cdmacintr[chan].cih_arg = arg;

	return (&cdmacintr[chan]);
}

void
ll_dmac_intr_disestablish(int chan, void *handle)
{
	int 			s;

	KASSERT(chan >= 0 && chan < CDMAC_NCHAN);
	KASSERT(&cdmacintr[chan] == handle);

	s = splnet();
	cdmacintr[chan].cih_func = NULL;
	cdmacintr[chan].cih_arg = NULL;
	splx(s);
}

int
virtex_console_tag(const char *xname, bus_space_tag_t *bst)
{
	if (strncmp(xname, "xlcom", 5) == 0) {
		*bst = &opb_bst;
		return (0);
	}

	return (ENODEV);
}

void
virtex_machdep_init(vaddr_t endva, vsize_t maxsz, struct mem_region *phys,
    struct mem_region *avail)
{
	ppc4xx_tlb_reserve(OPB_BASE, endva, maxsz, TLB_I | TLB_G);
	endva += maxsz;

	opb_bst.pbs_limit = maxsz;

	if (bus_space_init(&opb_bst, "opbtag", opb_extent_storage,
	    sizeof(opb_extent_storage)))
		panic("virtex_machdep_init: failed to initialize opb_bst");

	/*
	 * The TFT controller is broken, we can't change FB address.
	 * Hardwire it at predefined base address, create uncached
	 * mapping.
	 */

	avail[0].size = TFT_FB_BASE - avail[0].start;
	ppc4xx_tlb_reserve(TFT_FB_BASE, endva, TFT_FB_SIZE, TLB_I | TLB_G);
}

void
device_register(struct device *dev, void *aux)
{
	prop_number_t 		pn;
	void 			*fb;

	if (strncmp(device_xname(dev), "tft0", 4) == 0) {
		fb = ppc4xx_tlb_mapiodev(TFT_FB_BASE, TFT_FB_SIZE);
		if (fb == NULL)
			panic("device_register: framebuffer mapping gone!\n");

		pn = prop_number_create_unsigned_integer(TFT_FB_BASE);
		if (pn == NULL) {
			printf("WARNING: could not allocate virtex-tft-pa\n");
			return ;
		}
		if (prop_dictionary_set(device_properties(dev),
		    "virtex-tft-pa", pn) != true)
			printf("WARNING: could not set virtex-tft-pa\n");
		prop_object_release(pn);

		pn = prop_number_create_unsigned_integer((uintptr_t)fb);
		if (pn == NULL) {
			printf("WARNING: could not allocate virtex-tft-va\n");
			return ;
		}
		if (prop_dictionary_set(device_properties(dev),
		    "virtex-tft-va", pn) != true)
			printf("WARNING: could not set virtex-tft-va\n");
		prop_object_release(pn);
	}
}
