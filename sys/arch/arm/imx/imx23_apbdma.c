/* $Id: imx23_apbdma.c,v 1.2.6.4 2017/12/03 11:35:53 jdolecek Exp $ */

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
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <arm/imx/imx23_apbdma.h>
#include <arm/imx/imx23_apbdmareg.h>
#include <arm/imx/imx23_apbdmavar.h>
#include <arm/imx/imx23_apbhdmareg.h>
#include <arm/imx/imx23_apbxdmareg.h>
#include <arm/imx/imx23var.h>

static int	apbdma_match(device_t, cfdata_t, void *);
static void	apbdma_attach(device_t, device_t, void *);
static int	apbdma_activate(device_t, enum devact);

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
static void	apbdma_init(struct apbdma_softc *);

#define DMA_RD(sc, reg)							\
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))
#define DMA_WR(sc, reg, val)						\
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))

#define APBDMA_SOFT_RST_LOOP 455 /* At least 1 us ... */

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
	struct apb_softc *sc_parent = device_private(parent);
	static u_int apbdma_attached = 0;

	if ((strncmp(device_xname(parent), "apbh", 4) == 0) &&
	    (apbdma_attached & F_APBH_DMA))
		return;
	if ((strncmp(device_xname(parent), "apbx", 4) == 0) &&
	    (apbdma_attached & F_APBX_DMA))
		return;

	sc->sc_dev = self;
	sc->sc_iot = aa->aa_iot;
	sc->sc_dmat = aa->aa_dmat;

	if (bus_space_map(sc->sc_iot,
	    aa->aa_addr, aa->aa_size, 0, &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map bus space\n");
		return;
	}

	if (strncmp(device_xname(parent), "apbh", 4) == 0)
		sc->flags = F_APBH_DMA;

	if (strncmp(device_xname(parent), "apbx", 4) == 0)
		sc->flags = F_APBX_DMA;

	apbdma_reset(sc);
	apbdma_init(sc);

	if (sc->flags & F_APBH_DMA)
		apbdma_attached |= F_APBH_DMA;
	if (sc->flags & F_APBX_DMA)
		apbdma_attached |= F_APBX_DMA;

	sc_parent->dmac = self;

	/* Initialize mutex to control concurrent access from the drivers. */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	if (sc->flags & F_APBH_DMA)
		aprint_normal(": APBH DMA\n");
	else if (sc->flags & F_APBX_DMA)
		aprint_normal(": APBX DMA\n");
	else
		panic("dma flag missing!\n");

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
	DMA_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_SFTRST) ||
	    (loop < APBDMA_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	DMA_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_CLKGATE);

	/* Soft-reset the block. */
	DMA_WR(sc, HW_APB_CTRL0_SET, HW_APB_CTRL0_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_CLKGATE));

	/* Bring block out of reset. */
	DMA_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_SFTRST);

	loop = 0;
	while ((DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_SFTRST) ||
	    (loop < APBDMA_SOFT_RST_LOOP))
		loop++;

	DMA_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_CLKGATE);

	/* Wait until clock is in the NON-gated state. */
	while (DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_CLKGATE);

	return;
}

/*
 * Initialize APB{H,X}DMA block.
 */
static void
apbdma_init(struct apbdma_softc *sc)
{

	if (sc->flags & F_APBH_DMA) {
		DMA_WR(sc, HW_APBH_CTRL0_SET, HW_APBH_CTRL0_AHB_BURST8_EN);
		DMA_WR(sc, HW_APBH_CTRL0_SET, HW_APBH_CTRL0_APB_BURST4_EN);
	}
	return;
}

/* 
 * Chain DMA commands together.
 *
 * Set src->next point to trg's physical DMA mapped address.
 */
void
apbdma_cmd_chain(apbdma_command_t src, apbdma_command_t trg, void *buf,
    bus_dmamap_t dmap)
{
	int i; 
	bus_size_t daddr;
	bus_addr_t trg_offset;

	trg_offset = (bus_addr_t)trg - (bus_addr_t)buf;
	daddr = 0;

	for (i = 0; i < dmap->dm_nsegs; i++) {
		daddr += dmap->dm_segs[i].ds_len;
		if (trg_offset < daddr) {
			src->next = (void *)(dmap->dm_segs[i].ds_addr +
			    (trg_offset - (daddr - dmap->dm_segs[i].ds_len)));
			break;
		}
	}

	return;
}

/*
 * Set DMA command buffer.
 *
 * Set cmd->buffer point to physical DMA address at offset in DMA map.
 */
void
apbdma_cmd_buf(apbdma_command_t cmd, bus_addr_t offset, bus_dmamap_t dmap)
{
	int i;
	bus_size_t daddr;

	daddr = 0;

	for (i = 0; i < dmap->dm_nsegs; i++) {
		daddr += dmap->dm_segs[i].ds_len;
		if (offset < daddr) {
			cmd->buffer = (void *)(dmap->dm_segs[i].ds_addr +
			    (offset - (daddr - dmap->dm_segs[i].ds_len)));
			break;
		}
	}

	return;
}

/*
 * Initialize DMA channel.
 */
void
apbdma_chan_init(struct apbdma_softc *sc, unsigned int channel)
{

	mutex_enter(&sc->sc_lock);

	/* Enable CMDCMPLT_IRQ. */
	DMA_WR(sc, HW_APB_CTRL1_SET, (1<<channel)<<16);

	mutex_exit(&sc->sc_lock);

	return;
}

/*
 * Set command chain for DMA channel.
 */
#define HW_APB_CHN_NXTCMDAR(base, channel)	(base + (0x70 * channel))
void
apbdma_chan_set_chain(struct apbdma_softc *sc, unsigned int channel,
	bus_dmamap_t dmap)
{
	uint32_t reg;

	if (sc->flags & F_APBH_DMA)
		reg = HW_APB_CHN_NXTCMDAR(HW_APBH_CH0_NXTCMDAR, channel);
	else
		reg = HW_APB_CHN_NXTCMDAR(HW_APBX_CH0_NXTCMDAR, channel);
	
	mutex_enter(&sc->sc_lock);
	DMA_WR(sc, reg, dmap->dm_segs[0].ds_addr);
	mutex_exit(&sc->sc_lock);

	return;
}

/*
 * Initiate DMA transfer.
 */
#define HW_APB_CHN_SEMA(base, channel)	(base + (0x70 * channel))
void
apbdma_run(struct apbdma_softc *sc, unsigned int channel)
{
	uint32_t reg;
	uint8_t val;

	if (sc->flags & F_APBH_DMA) {
		reg = HW_APB_CHN_SEMA(HW_APBH_CH0_SEMA, channel);
		val = __SHIFTIN(1, HW_APBH_CH0_SEMA_INCREMENT_SEMA);
	 } else {
		reg = HW_APB_CHN_SEMA(HW_APBX_CH0_SEMA, channel);
		val = __SHIFTIN(1, HW_APBX_CH0_SEMA_INCREMENT_SEMA);
	}

	mutex_enter(&sc->sc_lock);
	DMA_WR(sc, reg, val);
	mutex_exit(&sc->sc_lock);

	return;
}

/*
 * Acknowledge command complete IRQ.
 */
void
apbdma_ack_intr(struct apbdma_softc *sc, unsigned int channel)
{

	mutex_enter(&sc->sc_lock);
	if (sc->flags & F_APBH_DMA) {
		DMA_WR(sc, HW_APB_CTRL1_CLR, (1<<channel));
	} else {
		DMA_WR(sc, HW_APB_CTRL1_CLR, (1<<channel));
	}
	mutex_exit(&sc->sc_lock);

	return;
}

/*
 * Acknowledge error IRQ.
 */
void
apbdma_ack_error_intr(struct apbdma_softc *sc, unsigned int channel)
{

	mutex_enter(&sc->sc_lock);
	DMA_WR(sc, HW_APB_CTRL2_CLR, (1<<channel));
	mutex_exit(&sc->sc_lock);

	return;
}

/*
 * Return reason for the IRQ.
 */
unsigned int
apbdma_intr_status(struct apbdma_softc *sc, unsigned int channel)
{
	unsigned int reason;

	reason = 0;

	mutex_enter(&sc->sc_lock);

	/* Check if this was command complete IRQ. */
	if (DMA_RD(sc, HW_APB_CTRL1) & (1<<channel))
		reason = DMA_IRQ_CMDCMPLT;

	/* Check if error was set. */
	if (DMA_RD(sc, HW_APB_CTRL2) & (1<<channel)) {
		if (DMA_RD(sc, HW_APB_CTRL2) & (1<<channel)<<16)
			reason = DMA_IRQ_BUS_ERROR;
		else
			reason = DMA_IRQ_TERM;
	}

	mutex_exit(&sc->sc_lock);

	return reason;
}

/*
 * Reset DMA channel.
 * Use only for devices on APBH bus.
 */
void
apbdma_chan_reset(struct apbdma_softc *sc, unsigned int channel)
{
	
	mutex_enter(&sc->sc_lock);

	if (sc->flags & F_APBH_DMA) {
		DMA_WR(sc, HW_APB_CTRL0_SET,
		    __SHIFTIN((1<<channel), HW_APBH_CTRL0_RESET_CHANNEL));
		while(DMA_RD(sc, HW_APB_CTRL0) & HW_APBH_CTRL0_RESET_CHANNEL);
	} else {
		DMA_WR(sc, HW_APBX_CHANNEL_CTRL_SET,
			__SHIFTIN((1<<channel), HW_APBH_CTRL0_RESET_CHANNEL));
		while(DMA_RD(sc, HW_APBX_CHANNEL_CTRL) & (1<<channel));
	}

	mutex_exit(&sc->sc_lock);

	return;
}

void
apbdma_wait(struct apbdma_softc *sc, unsigned int channel)
{

	mutex_enter(&sc->sc_lock);
	
	if (sc->flags & F_APBH_DMA) {
		while (DMA_RD(sc, HW_APB_CHN_SEMA(HW_APBH_CH0_SEMA, channel)) & HW_APBH_CH0_SEMA_PHORE)
			;
	 } else {
		while (DMA_RD(sc, HW_APB_CHN_SEMA(HW_APBX_CH0_SEMA, channel)) & HW_APBX_CH0_SEMA_PHORE)
			;
	}

	mutex_exit(&sc->sc_lock);
}
