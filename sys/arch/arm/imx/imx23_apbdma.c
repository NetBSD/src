/* $NetBSD: imx23_apbdma.c,v 1.10 2026/02/08 22:19:05 yurix Exp $ */

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
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/imx/imx23_apbdma.h>
#include <arm/imx/imx23_apbdmareg.h>
#include <arm/imx/imx23_apbhdmareg.h>
#include <arm/imx/imx23_apbxdmareg.h>
#include <arm/imx/imx23var.h>

/* DMA command control register bits. */
#define APBDMA_CMD_XFER_COUNT		__BITS(31, 16)
#define APBDMA_CMD_CMDPIOWORDS		__BITS(15, 12)
#define APBDMA_CMD_RESERVED		__BITS(11, 9)
#define APBDMA_CMD_HALTONTERMINATE	__BIT(8)
#define APBDMA_CMD_WAIT4ENDCMD		__BIT(7)
#define APBDMA_CMD_SEMAPHORE		__BIT(6)
#define APBDMA_CMD_NANDWAIT4READY	__BIT(5)
#define APBDMA_CMD_NANDLOCK		__BIT(4)
#define APBDMA_CMD_IRQONCMPLT		__BIT(3)
#define APBDMA_CMD_CHAIN		__BIT(2)
#define APBDMA_CMD_COMMAND		__BITS(1, 0)

#define APBDMA_CMD_XFER_MAX_BYTES	65535

/* DMA command types. */
#define APBDMA_CMD_NO_DMA_XFER		0
#define APBDMA_CMD_DMA_WRITE		1
#define APBDMA_CMD_DMA_READ		2
#define APBDMA_CMD_DMA_SENSE		3

/* Flags. */
#define F_APBH_DMA			__BIT(0)
#define F_APBX_DMA			__BIT(1)

/* Number of channels. */
#define AHB_MAX_DMA_CHANNELS		16

/* Return codes for apbdma_intr_status() */
#define DMA_IRQ_CMDCMPLT		0
#define DMA_IRQ_TERM			1
#define DMA_IRQ_BUS_ERROR		2

/* Supported number of PIO words */
#define MAX_PIO_WORDS 3

/* Memory allocated for per-segment command buffer */
#define IMX23_DMA_MAX_SEGMENTS 16

#define HW_APB_CHN_NXTCMDAR(base, channel)	(base + (0x70 * channel))
#define HW_APB_CHN_SEMA(base, channel)	(base + (0x70 * channel))

#define DMA_RD(sc, reg)							\
		bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))
#define DMA_WR(sc, reg, val)						\
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))

#define APBDMA_SOFT_RST_LOOP 455 /* At least 1 us ... */

struct imx23_apbdma_fdt_config {
	u_int flags;
	u_int num_channels;
};

struct imx23_apbdma_command {
	void *next;		/* Physical address. */
	uint32_t control;
	void *buffer;		/* Physical address. */
	uint32_t pio_words[MAX_PIO_WORDS];
};

struct imx23_apbdma_softc;
struct imx23_apbdma_channel {
	u_int chan_index;
	void (*chan_cb)(void *);
	void *chan_cbarg;
	struct imx23_apbdma_softc *chan_parent;
	bus_dmamap_t chan_dmamp;
	bus_dma_segment_t chan_ds[1];
	bus_size_t chan_cmd_sz;
	struct imx23_apbdma_command *chan_cmds;
};

struct imx23_apbdma_softc {
	device_t sc_dev;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_ioh;
	bus_space_tag_t sc_iot;
	u_int flags;
	struct imx23_apbdma_channel sc_chan[AHB_MAX_DMA_CHANNELS];
};

static int	imx23_apbdma_match(device_t, cfdata_t, void *);
static void	imx23_apbdma_attach(device_t, device_t, void *);
static void 	imx23_apbdma_reset(struct imx23_apbdma_softc *);
static void	imx23_apbdma_init(struct imx23_apbdma_softc *);
static void *	imx23_apbdma_fdt_acquire(device_t, const void *, size_t,
					 void (*)(void *), void *);
static void 	imx23_apbdma_fdt_release(device_t, void *);
static int 	imx23_apbdma_fdt_transfer(device_t, void *,
					  struct fdtbus_dma_req *);
static void 	imx23_apbdma_fdt_halt(device_t, void *);
static int	imx23_apbdma_intr(void *);
static int 	imx23_apbdma_pio_transfer(struct imx23_apbdma_softc *sc,
					  struct imx23_apbdma_channel *chan,
					  struct fdtbus_dma_req *req);
static int 	imx23_apbdma_data_transfer(struct imx23_apbdma_softc *sc,
					   struct imx23_apbdma_channel *chan,
					   struct fdtbus_dma_req *req);

CFATTACH_DECL_NEW(imx23apbdma, sizeof(struct imx23_apbdma_softc),
		  imx23_apbdma_match, imx23_apbdma_attach, NULL, NULL);

static const struct fdtbus_dma_controller_func imx23_apbdma_fdt_dma_funcs = {
	.acquire = imx23_apbdma_fdt_acquire,
	.release = imx23_apbdma_fdt_release,
	.transfer = imx23_apbdma_fdt_transfer,
	.halt = imx23_apbdma_fdt_halt,
};

static const struct imx23_apbdma_fdt_config imx23_apbh_config = {
	.flags = F_APBH_DMA,
	.num_channels = 8,
};
static const struct imx23_apbdma_fdt_config imx23_apbx_config = {
	.flags = F_APBX_DMA,
	.num_channels = 16,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "fsl,imx23-dma-apbh", .data = &imx23_apbh_config },
	{ .compat = "fsl,imx23-dma-apbx", .data = &imx23_apbx_config },
	DEVICE_COMPAT_EOL
};

static void *
imx23_apbdma_fdt_acquire(device_t dev, const void *data, size_t len,
		   void (*cb)(void *), void *cbarg)
{
	struct imx23_apbdma_softc *sc = device_private(dev);
	struct imx23_apbdma_channel *chan;
	const uint32_t *specifier = data;
	int rsegs;

	/* get channel index */
	if (len != 4) {
		return NULL;
	}
	const u_int chan_index = be32toh(specifier[0]);

	chan = &sc->sc_chan[chan_index];

	chan->chan_cb = cb;
	chan->chan_cbarg = cbarg;

	/* Enable CMDCMPLT_IRQ. */
	DMA_WR(sc, HW_APB_CTRL1_SET, (1<<chan->chan_index)<<16);

	/* Prepare DMA command buffers for channel */
	chan->chan_cmd_sz = IMX23_DMA_MAX_SEGMENTS *
			    sizeof(struct imx23_apbdma_command);
	if (bus_dmamem_alloc(sc->sc_dmat, chan->chan_cmd_sz, PAGE_SIZE, 0,
			     chan->chan_ds, 1, &rsegs, BUS_DMA_WAITOK)) {
		return NULL;
	}
	if (bus_dmamem_map(sc->sc_dmat, chan->chan_ds, rsegs, chan->chan_cmd_sz,
			   (void **)&chan->chan_cmds, BUS_DMA_WAITOK)) {
		return NULL;
	}
	if (bus_dmamap_create(sc->sc_dmat, MAXPHYS, 1, MAXPHYS, 0,
			      BUS_DMA_WAITOK, &chan->chan_dmamp)) {
		return NULL;
	}
	if (bus_dmamap_load(sc->sc_dmat, chan->chan_dmamp, chan->chan_cmds,
			    chan->chan_cmd_sz, NULL, BUS_DMA_WAITOK)) {
		return NULL;
	}

	/* link next address field of commands */
	struct imx23_apbdma_command *pa_cmd = (void *) chan->chan_ds[0].ds_addr;
	for (int i = 0; i < IMX23_DMA_MAX_SEGMENTS - 1; i++) {
		chan->chan_cmds[i].next = &pa_cmd[i + 1].next;
	}

	return chan;
}

static void
imx23_apbdma_fdt_release(device_t dev, void *priv)
{
	struct imx23_apbdma_softc *sc = device_private(dev);
	struct imx23_apbdma_channel *chan = priv;

	bus_dmamap_unload(sc->sc_dmat, chan->chan_dmamp);
	bus_dmamap_destroy(sc->sc_dmat, chan->chan_dmamp);
	bus_dmamem_unmap(sc->sc_dmat, chan->chan_cmds, chan->chan_cmd_sz);
	bus_dmamem_free(sc->sc_dmat, chan->chan_ds, 1);
}

static int imx23_apbdma_pio_transfer(struct imx23_apbdma_softc *sc,
				     struct imx23_apbdma_channel *chan,
				     struct fdtbus_dma_req *req)
{
	/* some sanity checks */
	if (req->dreq_nsegs != 0) {
		return EINVAL;
	}
	if (req->dreq_datalen > MAX_PIO_WORDS) {
		return EINVAL;
	}

	/* prepare command */
	chan->chan_cmds[0].control = __SHIFTIN(0, APBDMA_CMD_XFER_COUNT) |
		       __SHIFTIN(req->dreq_datalen, APBDMA_CMD_CMDPIOWORDS) |
		       __SHIFTIN(APBDMA_CMD_NO_DMA_XFER, APBDMA_CMD_COMMAND) |
		       APBDMA_CMD_HALTONTERMINATE | APBDMA_CMD_SEMAPHORE |
		       APBDMA_CMD_IRQONCMPLT;
	if (req->dreq_block_irq) {
		chan->chan_cmds[0].control |= APBDMA_CMD_WAIT4ENDCMD;
	}
	/* copy PIO words */
	uint32_t *pio_words = req->dreq_data;
	for (int i = 0; i < req->dreq_datalen; i++) {
		chan->chan_cmds[0].pio_words[i] = pio_words[i];
	}

	return 0;
}

static int imx23_apbdma_data_transfer(struct imx23_apbdma_softc *sc,
				      struct imx23_apbdma_channel *chan,
				      struct fdtbus_dma_req *req)
{
	/* some sanity checks */
	if (req->dreq_nsegs < 0 || req->dreq_nsegs > IMX23_DMA_MAX_SEGMENTS) {
		return EINVAL;
	}
	for (int i = 0; i < req->dreq_nsegs; i++) {
		if (req->dreq_segs[i].ds_len > APBDMA_CMD_XFER_MAX_BYTES) {
			return EINVAL;
		}
	}
	if (req->dreq_datalen > MAX_PIO_WORDS) {
		return EINVAL;
	}

	/* The fdt subsystem and the imx23 documentation use opposite naming */
	uint32_t dma_dir;
	if (req->dreq_dir == FDT_DMA_WRITE) {
		dma_dir = __SHIFTIN(APBDMA_CMD_DMA_READ, APBDMA_CMD_COMMAND);
	} else {
		dma_dir = __SHIFTIN(APBDMA_CMD_DMA_WRITE, APBDMA_CMD_COMMAND);
	}

	/* prepare commands */
	for (int i = 0; i < req->dreq_nsegs; i++) {
		chan->chan_cmds[i].buffer = (void *)req->dreq_segs[i].ds_addr;
		chan->chan_cmds[i].control =
		    __SHIFTIN(req->dreq_segs[i].ds_len, APBDMA_CMD_XFER_COUNT) |
		    __SHIFTIN(0, APBDMA_CMD_CMDPIOWORDS) | dma_dir |
		    APBDMA_CMD_HALTONTERMINATE | APBDMA_CMD_CHAIN;
	}

	/* special case: first command does PIO */
	uint32_t *pio_words = req->dreq_data;
	for (int i = 0; i < req->dreq_datalen; i++) {
		chan->chan_cmds[0].pio_words[i] = pio_words[i];
	}
	chan->chan_cmds[0].control &= ~APBDMA_CMD_CMDPIOWORDS;
	chan->chan_cmds[0].control |=
		__SHIFTIN(req->dreq_datalen, APBDMA_CMD_CMDPIOWORDS);

	/* special case: last command needs to raise interrupt */
	int tail_index = req->dreq_nsegs - 1;
	chan->chan_cmds[tail_index].control &= ~APBDMA_CMD_CHAIN;
	chan->chan_cmds[tail_index].control |=
		APBDMA_CMD_SEMAPHORE | APBDMA_CMD_IRQONCMPLT;
	if (req->dreq_block_irq) {
		chan->chan_cmds[tail_index].control |= APBDMA_CMD_WAIT4ENDCMD;
	}

	return 0;
}

static int
imx23_apbdma_fdt_transfer(device_t dev, void *priv, struct fdtbus_dma_req *req)
{
	struct imx23_apbdma_softc *sc = device_private(dev);
	struct imx23_apbdma_channel *chan = priv;
	int error = 0;
	uint32_t reg;
	uint8_t val;

	if (req->dreq_dir == FDT_DMA_NO_XFER) {
		error = imx23_apbdma_pio_transfer(sc, chan, req);
		if (error)
			return error;
	} else {
		error = imx23_apbdma_data_transfer(sc, chan, req);
		if (error)
			return error;
	}

	bus_dmamap_sync(sc->sc_dmat, chan->chan_dmamp, 0, chan->chan_cmd_sz,
			BUS_DMASYNC_PREWRITE);

	/* set the address of the dma command */
	if (sc->flags & F_APBH_DMA)
		reg =
		    HW_APB_CHN_NXTCMDAR(HW_APBH_CH0_NXTCMDAR, chan->chan_index);
	else
		reg =
		    HW_APB_CHN_NXTCMDAR(HW_APBX_CH0_NXTCMDAR, chan->chan_index);
	DMA_WR(sc, reg, chan->chan_ds[0].ds_addr);

	/* increase semaphore to start dma transfer */
	if (sc->flags & F_APBH_DMA) {
		reg = HW_APB_CHN_SEMA(HW_APBH_CH0_SEMA, chan->chan_index);
		val = __SHIFTIN(1, HW_APBH_CH0_SEMA_INCREMENT_SEMA);
	} else {
		reg = HW_APB_CHN_SEMA(HW_APBX_CH0_SEMA, chan->chan_index);
		val = __SHIFTIN(1, HW_APBX_CH0_SEMA_INCREMENT_SEMA);
	}
	DMA_WR(sc, reg, val);

	return 0;
}

static void
imx23_apbdma_fdt_halt(device_t dev, void *priv)
{
	/* do nothing */
}

static int
imx23_apbdma_intr(void *frame)
{
	struct imx23_apbdma_channel *chan = frame;
	struct imx23_apbdma_softc *sc = chan->chan_parent;
	unsigned int reason = 0;

	/* Check if this was command complete IRQ. */
	if (DMA_RD(sc, HW_APB_CTRL1) & (1<<chan->chan_index))
		reason = DMA_IRQ_CMDCMPLT;

	/* Check if error was set. */
	if (DMA_RD(sc, HW_APB_CTRL2) & (1<<chan->chan_index)) {
		if (DMA_RD(sc, HW_APB_CTRL2) & (1<<chan->chan_index)<<16)
			reason = DMA_IRQ_BUS_ERROR;
		else
			reason = DMA_IRQ_TERM;
	}

	if (reason) {
		/* acknowledge error */
		DMA_WR(sc, HW_APB_CTRL2_CLR, (1<<chan->chan_index));
	} else {
		/* acknowledge completion */
		if (sc->flags & F_APBH_DMA) {
			DMA_WR(sc, HW_APB_CTRL1_CLR, (1<<chan->chan_index));
		} else {
			DMA_WR(sc, HW_APB_CTRL1_CLR, (1<<chan->chan_index));
		}
	}

	bus_dmamap_sync(sc->sc_dmat, chan->chan_dmamp, 0, chan->chan_cmd_sz,
			BUS_DMASYNC_POSTWRITE);

	if (chan->chan_cb != NULL) {
		chan->chan_cb(chan->chan_cbarg);
	}

	if (reason == DMA_IRQ_TERM) {
		/* reset the dma channel */
		if (sc->flags & F_APBH_DMA) {
			DMA_WR(sc, HW_APB_CTRL0_SET,
			       __SHIFTIN((1 << chan->chan_index),
					 HW_APBH_CTRL0_RESET_CHANNEL));
			while (DMA_RD(sc, HW_APB_CTRL0) & HW_APBH_CTRL0_RESET_CHANNEL)
				continue;
		} else {
			DMA_WR(sc, HW_APBX_CHANNEL_CTRL_SET,
			       __SHIFTIN((1 << chan->chan_index),
					 HW_APBH_CTRL0_RESET_CHANNEL));
			while (DMA_RD(sc, HW_APBX_CHANNEL_CTRL) & (1 << chan->chan_index))
				continue;
		}
	}

	return 1;
}

static int
imx23_apbdma_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args *const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
imx23_apbdma_attach(device_t parent, device_t self, void *aux)
{
	struct imx23_apbdma_softc *const sc = device_private(self);
	struct fdt_attach_args *const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct imx23_apbdma_fdt_config *config;
	int len;
	char intrstr[128];

	// find out if we are for APBH or APBX
	config = of_compatible_lookup(phandle, compat_data)->data;
	sc->flags = config->flags;

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;

	/* Map control registers */
	bus_addr_t addr;
	bus_size_t size;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}
	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	/* establish interrupts */
	const u_int *specifier = fdtbus_get_prop(phandle, "interrupts", &len);
	KASSERT(len == sizeof(u_int)*config->num_channels);
	for (int i = 0; i < config->num_channels; i++) {
		u_int irq = be32toh(specifier[i]);

		/* irq = 0 means channel is unused*/
		if (irq == 0)
			continue;

		/* Some channels share an irq. Only establish it once. */
		bool skip = false;
		for (int j = 0; j<i;j++) {
			if (be32toh(specifier[j]) == irq) {
				skip = true;
				break;
			}
		}
		if (skip) {
			continue;
		}

		if (!fdtbus_intr_str(phandle, i, intrstr, sizeof(intrstr))) {
			aprint_error(": failed to decode interrupt\n");
			return;
		}
		void *ih = fdtbus_intr_establish_xname(
		    phandle, i, IPL_SCHED, FDT_INTR_MPSAFE, imx23_apbdma_intr,
		    &sc->sc_chan[i], device_xname(self));
		if (ih == NULL) {
			aprint_error_dev(
			    self, ": couldn't install interrupt handler %s\n",
			    intrstr);
			return;
		}
	}

	/* initialize our controller */
	for (int i = 0; i < AHB_MAX_DMA_CHANNELS; i++) {
		sc->sc_chan[i].chan_index = i;
		sc->sc_chan[i].chan_parent = sc;
	}
	imx23_apbdma_reset(sc);
	imx23_apbdma_init(sc);

	fdtbus_register_dma_controller(self, phandle,
				       &imx23_apbdma_fdt_dma_funcs);

	if (sc->flags & F_APBH_DMA) {
		aprint_normal(": type=apbh\n");
	} else if (sc->flags & F_APBX_DMA) {
		aprint_normal(": type=apbx\n");
	}
}

/*
 * Reset the APB{H,X}DMA block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
void
imx23_apbdma_reset(struct imx23_apbdma_softc *sc)
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
	while (!(DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_CLKGATE))
		continue;

	/* Bring block out of reset. */
	DMA_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_SFTRST);

	loop = 0;
	while ((DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_SFTRST) ||
	    (loop < APBDMA_SOFT_RST_LOOP))
		loop++;

	DMA_WR(sc, HW_APB_CTRL0_CLR, HW_APB_CTRL0_CLKGATE);

	/* Wait until clock is in the NON-gated state. */
	while (DMA_RD(sc, HW_APB_CTRL0) & HW_APB_CTRL0_CLKGATE)
		continue;
}

/*
 * Initialize APB{H,X}DMA block.
 */
static void
imx23_apbdma_init(struct imx23_apbdma_softc *sc)
{

	if (sc->flags & F_APBH_DMA) {
		DMA_WR(sc, HW_APBH_CTRL0_SET, HW_APBH_CTRL0_AHB_BURST8_EN);
		DMA_WR(sc, HW_APBH_CTRL0_SET, HW_APBH_CTRL0_APB_BURST4_EN);
	}
}
