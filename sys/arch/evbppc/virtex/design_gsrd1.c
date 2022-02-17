/* 	$NetBSD: design_gsrd1.c,v 1.8 2022/02/17 00:54:51 riastradh Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: design_gsrd1.c,v 1.8 2022/02/17 00:54:51 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/dev/plbvar.h>

#include <evbppc/virtex/dev/xcvbusvar.h>

#include <evbppc/virtex/dev/xlcomreg.h>
#include <evbppc/virtex/dev/cdmacreg.h>
#include <evbppc/virtex/dev/temacreg.h>
#include <evbppc/virtex/dev/tftreg.h>

#include <evbppc/virtex/virtex.h>
#include <evbppc/virtex/dcr.h>


#define DCR_CDMAC_BASE 		0x0140
#define DCR_XLCOM_BASE 		0x0000
#define DCR_TEMAC_BASE 		0x0030
#define DCR_LLFB_BASE 		0x0080

#define CDMAC_TX0_STAT 		CDMAC_STAT_BASE(0)
#define CDMAC_RX0_STAT 		CDMAC_STAT_BASE(1)
#define CDMAC_TX1_STAT 		CDMAC_STAT_BASE(2)
#define CDMAC_RX1_STAT 		CDMAC_STAT_BASE(3)

#define CDMAC_TX0_BASE 		CDMAC_CTRL_BASE(0)
#define CDMAC_RX0_BASE 		CDMAC_CTRL_BASE(1)
#define CDMAC_TX1_BASE 		CDMAC_CTRL_BASE(2)
#define CDMAC_RX1_BASE 		CDMAC_CTRL_BASE(3)

#define CDMAC_INTR_LINE 	2
#define CDMAC_NCHAN 		4

#define IPL_CDMAC 		IPL_NET
#define splcdmac() 		splnet()


/*
 * CDMAC per-channel interrupt handler. CDMAC has only one interrupt signal
 * shared by all channels on GSRD, so we have to dispatch channels manually.
 *
 * Note: we hardwire priority to IPL_NET, temac(4) is the only device that
 * needs to service DMA interrupts anyway.
 */
struct cdmac_intr_handle {
	void 			(*cih_func)(void *);
	void 			*cih_arg;
};

static void 			*cdmac_ih = NULL; 	/* real CDMAC intr */
static struct cdmac_intr_handle *cdmac_intrs[CDMAC_NCHAN];


/*
 * DCR bus space leaf access routines.
 */

static void
xlcom0_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_XLCOM_BASE, XLCOM_TX_FIFO);
	WCASE(DCR_XLCOM_BASE, XLCOM_STAT);
	WCASE(DCR_XLCOM_BASE, XLCOM_CNTL);
	WDEAD(addr);
	}
}

static uint32_t
xlcom0_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_XLCOM_BASE, XLCOM_RX_FIFO);
	RCASE(DCR_XLCOM_BASE, XLCOM_STAT);
	RCASE(DCR_XLCOM_BASE, XLCOM_CNTL);
	RDEAD(addr);
	}

	return (val);
}

static void
tft0_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_LLFB_BASE, TFT_CTRL);
	WDEAD(addr);
	}
}

static uint32_t
tft0_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_LLFB_BASE, TFT_CTRL);
	RDEAD(addr);
	}

	return (val);
}

#define DOCHAN(op, channel) \
	op(DCR_CDMAC_BASE, channel + CDMAC_NEXT); 	\
	op(DCR_CDMAC_BASE, channel + CDMAC_CURADDR); 	\
	op(DCR_CDMAC_BASE, channel + CDMAC_CURSIZE); 	\
	op(DCR_CDMAC_BASE, channel + CDMAC_CURDESC)

static void
cdmac0_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_CDMAC_BASE, CDMAC_INTR);
	WCASE(DCR_CDMAC_BASE, CDMAC_TX0_STAT);
	WCASE(DCR_CDMAC_BASE, CDMAC_RX0_STAT);
	WCASE(DCR_CDMAC_BASE, CDMAC_TX1_STAT);
	WCASE(DCR_CDMAC_BASE, CDMAC_RX1_STAT);
	DOCHAN(WCASE, CDMAC_TX0_BASE);
	DOCHAN(WCASE, CDMAC_RX0_BASE);
	DOCHAN(WCASE, CDMAC_TX1_BASE);
	DOCHAN(WCASE, CDMAC_RX1_BASE);
	WDEAD(addr);
	}
}

static uint32_t
cdmac0_read_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr)
{
	uint32_t 		val;

	addr += h;

	switch (addr) {
	RCASE(DCR_CDMAC_BASE, CDMAC_INTR);
	RCASE(DCR_CDMAC_BASE, CDMAC_TX0_STAT);
	RCASE(DCR_CDMAC_BASE, CDMAC_RX0_STAT);
	RCASE(DCR_CDMAC_BASE, CDMAC_TX1_STAT);
	RCASE(DCR_CDMAC_BASE, CDMAC_RX1_STAT);
	DOCHAN(RCASE, CDMAC_TX0_BASE);
	DOCHAN(RCASE, CDMAC_RX0_BASE);
	DOCHAN(RCASE, CDMAC_TX1_BASE);
	DOCHAN(RCASE, CDMAC_RX1_BASE);
	RDEAD(addr);
	}

	return (val);
}

#undef DOCHAN

static void
temac0_write_4(bus_space_tag_t t, bus_space_handle_t h, uint32_t addr,
    uint32_t val)
{
	addr += h;

	switch (addr) {
	WCASE(DCR_TEMAC_BASE, TEMAC_RESET);
	WDEAD(addr);
	}
}

static const struct powerpc_bus_space xlcom_bst = {
	DCR_BST_BODY(DCR_XLCOM_BASE, xlcom0_read_4, xlcom0_write_4)
};

static const struct powerpc_bus_space cdmac_bst = {
	DCR_BST_BODY(DCR_CDMAC_BASE, cdmac0_read_4, cdmac0_write_4)
};

static const struct powerpc_bus_space temac_bst = {
	DCR_BST_BODY(DCR_TEMAC_BASE, NULL, temac0_write_4)
};

static const struct powerpc_bus_space tft_bst = {
	DCR_BST_BODY(DCR_LLFB_BASE, tft0_read_4, tft0_write_4)
};

/*
 * Master device configuration table for GSRD design.
 */
static const struct gsrddev {
	const char 		*gdv_name;
	const char 		*gdv_attr;
	bus_space_tag_t 	gdv_bst;
	bus_addr_t 		gdv_addr;
	int 			gdv_intr;
	int 			gdv_rx_dma;
	int 			gdv_tx_dma;
} gsrd_devices[] = {
	{			/* gsrd_devices[0] */
		.gdv_name 	= "xlcom",
		.gdv_attr 	= "xcvbus",
		.gdv_bst 	= &xlcom_bst,
		.gdv_addr 	= 0,
		.gdv_intr 	= 0,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= -1,
	},
	{			/* gsrd_devices[1] */
		.gdv_name 	= "temac",
		.gdv_attr 	= "xcvbus",
		.gdv_bst 	= &temac_bst,
		.gdv_addr 	= 0,
		.gdv_intr 	= 1,
		.gdv_rx_dma 	= 3,
		.gdv_tx_dma 	= 2,
	},
	{			/* gsrd_devices[2] */
		.gdv_name 	= "tft",
		.gdv_attr 	= "llbus",
		.gdv_bst 	= &tft_bst,
		.gdv_addr 	= 0,
		.gdv_intr 	= -1,
		.gdv_rx_dma 	= -1,
		.gdv_tx_dma 	= 0,
	}
};

static struct ll_dmac *
virtex_mpmc_mapdma(int n, struct ll_dmac *chan)
{
	if (n == -1)
		return (NULL);

	chan->dmac_iot = &cdmac_bst;
	chan->dmac_ctrl_addr = CDMAC_CTRL_BASE(n);
	chan->dmac_stat_addr = CDMAC_STAT_BASE(n);
	chan->dmac_chan = n;

	return (chan);
}

static int
cdmac_intr(void *arg)
{
	uint32_t 		isr;
	int 			i;
	int 			did = 0;

	isr = bus_space_read_4(&cdmac_bst, 0, CDMAC_INTR);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, isr); 	/* ack */

	for (i = 0; i < CDMAC_NCHAN; i++)
		if (ISSET(isr, CDMAC_CHAN_INTR(i)) &&
		    cdmac_intrs[i] != NULL) {
			(cdmac_intrs[i]->cih_func)(cdmac_intrs[i]->cih_arg);
			did++;
		}

	/* XXX: This happens all the time under load... bug? */
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

	/* Reset all CDMAC engines, disable interrupt. */
	bus_space_write_4(&cdmac_bst, 0, CDMAC_STAT_BASE(0), CDMAC_STAT_RESET);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_STAT_BASE(1), CDMAC_STAT_RESET);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_STAT_BASE(2), CDMAC_STAT_RESET);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_STAT_BASE(3), CDMAC_STAT_RESET);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, 0);

	vaa.vaa_dmat = paa->plb_dmat;
	vaa._vaa_is_dcr = 1; 		/* XXX bst flag */

	/* Attach all we have. */
	for (i = 0; i < __arraycount(gsrd_devices); i++) {
		const struct gsrddev 	*g = &gsrd_devices[i];

		vaa.vaa_name 	= g->gdv_name;
		vaa.vaa_addr 	= g->gdv_addr;
		vaa.vaa_intr 	= g->gdv_intr;
		vaa.vaa_iot 	= g->gdv_bst;

		vaa.vaa_rx_dmac = virtex_mpmc_mapdma(g->gdv_rx_dma, &rx);
		vaa.vaa_tx_dmac = virtex_mpmc_mapdma(g->gdv_tx_dma, &tx);

		config_found(self, &vaa, xcvbus_print,
		    CFARGS(.iattr = g->gdv_attr));
	}

	/* Setup the dispatch handler. */
	cdmac_ih = intr_establish(CDMAC_INTR_LINE, IST_LEVEL, IPL_CDMAC,
	    cdmac_intr, NULL);
	if (cdmac_ih == NULL)
		panic("virtex_autoconf: could not establish cdmac intr");

	/* Enable CDMAC interrupt. */
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, ~CDMAC_INTR_MIE);
	bus_space_write_4(&cdmac_bst, 0, CDMAC_INTR, CDMAC_INTR_MIE);
}

void *
ll_dmac_intr_establish(int chan, void (*func)(void *), void *arg)
{
	struct cdmac_intr_handle *ih;

	KASSERT(chan > 0 && chan < CDMAC_NCHAN);

	/* We only allow one handler per channel, somewhat arbitrarily. */
	if (cdmac_intrs[chan] != NULL)
		return (NULL);

	ih = kmem_alloc(sizeof(*ih), KM_SLEEP);
	ih->cih_func = func;
	ih->cih_arg = arg;

	return (cdmac_intrs[chan] = ih);
}

void
ll_dmac_intr_disestablish(int chan, void *handle)
{
	struct cdmac_intr_handle *ih = handle;
	int 			s;

	KASSERT(chan > 0 && chan < CDMAC_NCHAN);
	KASSERT(cdmac_intrs[chan] == handle);

	s = splcdmac();
	cdmac_intrs[chan] = NULL;
	splx(s);

	kmem_free(ih, sizeof(*ih));
}

int
virtex_bus_space_tag(const char *xname, bus_space_tag_t *bst)
{
	if (strncmp(xname, "xlcom", 5) == 0) {
		*bst = &xlcom_bst;
		return (0);
	}

	return (ENODEV);
}

void
virtex_machdep_init(vaddr_t endva, vsize_t maxsz, struct mem_region *phys,
    struct mem_region *avail)
{
	/* Nothing to do -- no memory-mapped devices. */
}

void
device_register(device_t dev, void *aux)
{
	/* Nothing to do -- no property hacks needed. */
}
