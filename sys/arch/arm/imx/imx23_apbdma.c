/* $Id: imx23_apbdma.c,v 1.2.2.3 2013/01/23 00:05:41 yamt Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <arm/imx/imx23_apbdmareg.h>
#include <arm/imx/imx23_apbhdmareg.h>
#include <arm/imx/imx23_apbxdmareg.h>
#include <arm/imx/imx23_apbdma.h>
#include <arm/imx/imx23var.h>

static int	apbdma_match(device_t, cfdata_t, void *);
static void	apbdma_attach(device_t, device_t, void *);
static int	apbdma_activate(device_t, enum devact);

#define APBDMA_SOFT_RST_LOOP 455 /* At least 1 us ... */
#define DMACTRL_RD(sc, reg)						\
		bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define DMACTRL_WR(sc, reg, val)					\
		bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))

struct apbdma_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_hdl;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmamp;
	struct imx23_dma_channel *sc_channel;
	int n_channel;
};

struct imx23_dma_cmd {
	uint32_t next_cmd;
	uint32_t cmd;
	uint32_t buffer;
	uint32_t pio[CMDPIOWORDS_MAX];
	SIMPLEQ_ENTRY(imx23_dma_cmd) entries;
};

struct imx23_dma_channel {
	SIMPLEQ_HEAD(simplehead, imx23_dma_cmd) head;
	struct simplehead *headp;
	struct apbdma_softc *sc;
};

CFATTACH_DECL3_NEW(apbdma,
	sizeof(struct apbdma_softc),
	apbdma_match,
	apbdma_attach,
	NULL,
	apbdma_activate,
	NULL,
	NULL,
	0);

static void	apbdma_reset(struct apbdma_softc *);

static int
apbdma_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if (aa->aa_addr == HW_APBHDMA_BASE && aa->aa_size == HW_APBHDMA_SIZE)
			return 1;

	if (aa->aa_addr == HW_APBXDMA_BASE && aa->aa_size == HW_APBXDMA_SIZE)
			return 1;

	return 0;
}

static void
apbdma_attach(device_t parent, device_t self, void *aux)
{
	struct apb_attach_args *aa = aux;
	struct apbdma_softc *sc = device_private(self);
	//struct apb_softc *scp = device_private(parent);

//	static int apbdma_attached = 0;
//	struct imx23_dma_channel *chan;
//	int i;
	int error;

//	if (apbdma_attached)
//		return;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	/*
 	 * Parent bus softc has a pointer to DMA controller device_t for
	 * specific bus. As different busses need different instances of the
	 * DMA driver. The apb_softc.dmac is set up here. Now device drivers
	 * which use DMA can pass apb_softc.dmac from their parent to apbdma
	 * functions.
	 */
	if (bus_space_map(sc->sc_iot,
	    aa->aa_addr, aa->aa_size, 0, &(sc->sc_hdl))) {
		aprint_error_dev(sc->sc_dev, "unable to map bus space\n");
		return;
	}

	error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1,
	    PAGE_SIZE, 0, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW, &sc->sc_dmamp);
	if (error) {
		aprint_error_dev(sc->sc_dev,
			"couldn't create dma map. (error=%d)\n", error);
		return;
	}
#ifdef notyet	
	if (aa->aa_addr == HW_APBHDMA_BASE && aa->aa_size == HW_APBHDMA_SIZE) {
		sc->sc_channel = kmem_alloc(sizeof(struct imx23_dma_channel)
		    * APBH_DMA_N_CHANNELS, KM_SLEEP);
		sc->n_channel = APBH_DMA_N_CHANNELS;
	}

	if (aa->aa_addr == HW_APBXDMA_BASE && aa->aa_size == HW_APBXDMA_SIZE) {
		sc->sc_channel = kmem_alloc(sizeof(struct imx23_dma_channel)
		    * APBX_DMA_N_CHANNELS, KM_SLEEP);
		sc->n_channel = APBX_DMA_N_CHANNELS;
	}

	if (sc->sc_channel == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to allocate memory for"
		    " DMA channel structures\n");
		return;
	}

	for (i=0; i < sc->n_channel; i++) {
		chan = (struct imx23_dma_channel *)sc->sc_channel+i;
		chan->sc = sc;
		SIMPLEQ_INIT(&chan->head);
	}
#endif
	apbdma_reset(sc);
//	apbdma_attached = 1;

	aprint_normal("\n");

	return;
}

static int
apbdma_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * Reset the APB{H,X}DMA block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
apbdma_reset(struct apbdma_softc *sc)
{
	unsigned int loop;

	/*
	 * Prepare for soft-reset by making sure that SFTRST is not currently
	 * asserted. Also clear CLKGATE so we can wait for its assertion below.
	 */
	DMACTRL_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((DMACTRL_RD(sc, HW_APB_CTRL0) &  HW_APB_CTRL0_SFTRST) ||
		(loop < APBDMA_SOFT_RST_LOOP))
	{
		loop++;
	}

	/* Clear CLKGATE so we can wait for its assertion below. */
	DMACTRL_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_CLKGATE);

	/* Soft-reset the block. */
	DMACTRL_WR(sc, HW_APB_CTRL0_SET, HW_APB_CTRL0_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(DMACTRL_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_CLKGATE));

	/* Bring block out of reset. */
	DMACTRL_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_SFTRST);

	loop = 0;
	while ((DMACTRL_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_SFTRST) ||
		(loop < APBDMA_SOFT_RST_LOOP))
	{
		loop++;
	}
         
	DMACTRL_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_CLKGATE);

        /* Wait until clock is in the NON-gated state. */
	while (DMACTRL_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_CLKGATE);

        return;
}

/*
 * Allocate DMA safe memory for commands.
 */
void *
apbdma_dmamem_alloc(device_t dmac, int channel, bus_size_t size)
{
	struct apbdma_softc *sc = device_private(dmac);
	bus_dma_segment_t segs[1];	/* bus_dmamem_free needs. */
	int rsegs;
	int error;
	void *ptr = NULL;	/* bus_dmamem_unmap needs (size also) */

	if (size > PAGE_SIZE)
		return NULL;

	error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, segs, 1,
			&rsegs, BUS_DMA_NOWAIT);
	if (error)
		goto out;
//XXX:
	printf("segs[0].ds_addr=%lx, segs[0].ds_len=%lx, rsegs=%d\n", segs[0].ds_addr, segs[0].ds_len, rsegs);

	error = bus_dmamem_map(sc->sc_dmat, segs, 1, size, &ptr,
			BUS_DMA_NOWAIT);
	if (error)
		goto free;
//XXX:
	printf("segs[0].ds_addr=%lx, segs[0].ds_len=%lx, ptr=%p\n", segs[0].ds_addr, segs[0].ds_len, ptr);

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamp, ptr, size, NULL,
			BUS_DMA_NOWAIT | BUS_DMA_WRITE);
	if (error)
		goto unmap;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamp, 0, size,
		BUS_DMASYNC_PREWRITE);

	// return usable memory
unmap:
	bus_dmamem_unmap(sc->sc_dmat, ptr, size);
free:
	bus_dmamem_free(sc->sc_dmat, segs, 1);
out:
	return NULL;
}
