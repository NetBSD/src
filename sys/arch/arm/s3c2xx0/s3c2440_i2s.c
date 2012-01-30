/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kmem.h>

#include <sys/bus.h>

#include <arch/arm/s3c2xx0/s3c2440_dma.h>
#include <arch/arm/s3c2xx0/s3c2xx0var.h>
#include <arch/arm/s3c2xx0/s3c2440reg.h>
#include <arch/arm/s3c2xx0/s3c2440_i2s.h>

/*#define S3C2440_I2S_DEBUG*/

#ifdef S3C2440_I2S_DEBUG
#define DPRINTF(x) do {printf x; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

struct s3c2440_i2s_softc {
	device_t		sc_dev;
	kmutex_t		*sc_intr_lock;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_i2s_ioh;

	int			sc_master_clock;
	int			sc_serial_clock;
	int			sc_dir;
	int			sc_sample_width;
	int			sc_bus_format;

	bus_dma_segment_t	sc_dr;
};

static void	s3c2440_i2s_xfer_complete(dmac_xfer_t, void *);

static int	s3c2440_i2s_match(struct device *, struct cfdata *, void*);
static void	s3c2440_i2s_attach(struct device *, struct device *, void*);
static int	s3c2440_i2s_search(struct device *, struct cfdata *, const int *, void *);
static int	s3c2440_i2s_print(void *aux, const char *name);
static int	s3c2440_i2s_init(struct s3c2440_i2s_softc*);

CFATTACH_DECL_NEW(ssiis, sizeof(struct s3c2440_i2s_softc), s3c2440_i2s_match,
	      s3c2440_i2s_attach, NULL, NULL);

int
s3c2440_i2s_match(struct device *parent, struct cfdata *match, void*aux)
{
	/*struct s3c2xx0_attach_args *sa = aux;*/

	return 1;
}

void
s3c2440_i2s_attach(struct device *parent, struct device *self, void *aux)
{
	struct s3c2440_i2s_softc *sc = device_private(self);
	DPRINTF(("%s\n", __func__));

	sc->sc_dev = self;

	s3c2440_i2s_init(sc);

	printf("\n");

	config_search_ia(s3c2440_i2s_search, self, "ssiis", NULL);
}

static int
s3c2440_i2s_print(void *aux, const char *name)
{
	return UNCONF;
}

static int
s3c2440_i2s_search(struct device *parent, struct cfdata *cf, const int *ldesc,
		   void *aux)
{
	struct s3c2440_i2s_attach_args ia;
	DPRINTF(("%s\n", __func__));

	ia.i2sa_handle = device_private(parent);

	if (config_match(parent, cf, &ia))
		config_attach(parent, cf, &ia, s3c2440_i2s_print);

	return 1;
}

void
s3c2440_i2s_set_intr_lock(void *handle, kmutex_t *sc_intr_lock)
{
	struct s3c2440_i2s_softc *sc = handle;

	sc->sc_intr_lock = sc_intr_lock;
}

int
s3c2440_i2s_init(struct s3c2440_i2s_softc *i2s_sc)
{
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */
	uint32_t reg;

	i2s_sc->sc_iot = sc->sc_iot;

	if (bus_space_map(sc->sc_iot, S3C2440_IIS_BASE, S3C24X0_IIS_SIZE, 0,
			  &i2s_sc->sc_i2s_ioh)) {
		printf("Failed to map I2S registers\n");
		return ENOMEM;
	}

	i2s_sc->sc_master_clock = 0;
	i2s_sc->sc_serial_clock = 48;
	i2s_sc->sc_dir = 0;
	i2s_sc->sc_sample_width = 0;
	i2s_sc->sc_bus_format = 0;

	reg = bus_space_read_4(sc->sc_iot, sc->sc_clkman_ioh, CLKMAN_CLKCON);
	bus_space_write_4(sc->sc_iot, sc->sc_clkman_ioh, CLKMAN_CLKCON, reg | CLKCON_IIS);

	/* Setup GPIO pins to use I2S */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_PECON);
	reg = GPIO_SET_FUNC(reg, 0, 2);
	reg = GPIO_SET_FUNC(reg, 1, 2);
	reg = GPIO_SET_FUNC(reg, 2, 2);
	reg = GPIO_SET_FUNC(reg, 3, 2);
	reg = GPIO_SET_FUNC(reg, 4, 2);
	bus_space_write_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_PECON, reg);

	/* Disable Pull-up resister for all I2S pins */
	reg = bus_space_read_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_PEUP);

	reg = GPIO_SET_DATA(reg, 0, 1);
	reg = GPIO_SET_DATA(reg, 1, 1);
	reg = GPIO_SET_DATA(reg, 2, 1);
	reg = GPIO_SET_DATA(reg, 3, 1);
	reg = GPIO_SET_DATA(reg, 4, 1);

	bus_space_write_4(sc->sc_iot, sc->sc_gpio_ioh, GPIO_PEUP, reg);

	i2s_sc->sc_dr.ds_addr = S3C2440_IIS_BASE + IISFIFO;
	i2s_sc->sc_dr.ds_len = 4;

	return 0;
}

void
s3c2440_i2s_set_direction(void *handle, int direction)
{
	struct s3c2440_i2s_softc *sc = handle;
	sc->sc_dir = direction;
}

void
s3c2440_i2s_set_sample_rate(void *handle, int sample_rate)
{
	struct s3c2440_i2s_softc *sc = handle;
	int codecClock;
	int codecClockPrescaler;
	int pclk = s3c2xx0_softc->sc_pclk; /* Peripherical Clock in Hz*/

	DPRINTF(("%s\n", __func__));

	/* TODO: Add selection of 256fs when needed */
	sc->sc_master_clock = 384;

	codecClock = sample_rate * sc->sc_master_clock;
	codecClockPrescaler = pclk/codecClock;

	DPRINTF(("CODEC Clock: %d Hz\n", codecClock));
	DPRINTF(("Prescaler: %d\n", codecClockPrescaler));
	DPRINTF(("Actual CODEC Clock: %d Hz\n", pclk/(codecClockPrescaler+1)));
	DPRINTF(("Actual Sampling rate: %d Hz\n",
		 (pclk/(codecClockPrescaler+1))/sc->sc_master_clock));

	bus_space_write_4(sc->sc_iot, sc->sc_i2s_ioh, IISPSR,
			  IISPSR_PRESCALER_A(codecClockPrescaler) |
			  IISPSR_PRESCALER_B(codecClockPrescaler));
}

void
s3c2440_i2s_set_sample_width(void *handle, int width)
{
	struct s3c2440_i2s_softc *sc = handle;
	sc->sc_sample_width = width;
}

void
s3c2440_i2s_set_bus_format(void *handle, int format)
{
	struct s3c2440_i2s_softc *sc = handle;

	sc->sc_bus_format = format;
}

int
s3c2440_i2s_commit(void *handle)
{
	uint32_t iisfcon, iiscon, iismod;
	struct s3c2440_i2s_softc *sc = handle;

	DPRINTF(("%s\n", __func__));

	iisfcon = 0;
	iiscon = IISCON_IFACE_EN | IISCON_PRESCALER_EN;
	iismod = 0;

	if ( (sc->sc_dir & S3C2440_I2S_TRANSMIT) ) {
		iisfcon |= IISFCON_TX_DMA_EN | IISFCON_TX_FIFO_EN;
		iiscon |= IISCON_TX_DMA_EN;
		iismod |= IISMOD_MODE_TRANSMIT;
	}

	if ( (sc->sc_dir & S3C2440_I2S_RECEIVE) ) {
		iisfcon |= IISFCON_RX_DMA_EN | IISFCON_RX_FIFO_EN;
		iiscon |= IISCON_RX_DMA_EN;
		iismod |= IISMOD_MODE_RECEIVE;
	}

	if (iisfcon == 0) {
		return EINVAL;
	}


	if (sc->sc_bus_format == S3C2440_I2S_BUS_MSB)
		iismod |= IISMOD_IFACE_MSB;

	switch (sc->sc_master_clock) {
	case 256:
		iismod |= IISMOD_MASTER_FREQ256;
		break;
	case 384:
		iismod |= IISMOD_MASTER_FREQ384;
		break;
	default:
		return EINVAL;

	}

	switch (sc->sc_serial_clock) {
	case 16:
		iismod |= IISMOD_SERIAL_FREQ16;
		break;
	case 32:
		iismod |= IISMOD_SERIAL_FREQ32;
		break;
	case 48:
		iismod |= IISMOD_SERIAL_FREQ48;
		break;
	default:
		return EINVAL;
	}

	if (sc->sc_sample_width == 16)
		iismod |= IISMOD_16BIT;

	bus_space_write_4(sc->sc_iot, sc->sc_i2s_ioh, IISFCON, iisfcon);
	bus_space_write_4(sc->sc_iot, sc->sc_i2s_ioh, IISMOD, iismod);
	bus_space_write_4(sc->sc_iot, sc->sc_i2s_ioh, IISCON, iiscon);

	return 0;
}

int
s3c2440_i2s_disable(void *handle)
{
	return 0;
}

int
s3c2440_i2s_get_master_clock(void *handle)
{
	struct s3c2440_i2s_softc *sc = handle;
	return sc->sc_master_clock;
}

int
s3c2440_i2s_get_serial_clock(void *handle)
{
	struct s3c2440_i2s_softc *sc = handle;

	return sc->sc_serial_clock;
}

int
s3c2440_i2s_alloc(void *handle,
		  int direction, size_t size, int flags,
		  s3c2440_i2s_buf_t *out)
{
	int kalloc_flags = KM_SLEEP;
	int dma_flags = BUS_DMA_WAITOK;
	int retval = 0;
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */
	s3c2440_i2s_buf_t buf;

	DPRINTF(("%s\n", __func__));

	if (flags & M_NOWAIT) {
		kalloc_flags = KM_NOSLEEP;
		dma_flags = BUS_DMA_NOWAIT;
	}

	*out = kmem_alloc(sizeof(struct s3c2440_i2s_buf), kalloc_flags);
	if (*out == NULL) {
		DPRINTF(("Failed to allocate memory\n"));
		return ENOMEM;
	}

	buf = *out;
	buf->i2b_parent = handle;
	buf->i2b_size = size;
	buf->i2b_nsegs = S3C2440_I2S_BUF_MAX_SEGS;
	buf->i2b_xfer = NULL;
	buf->i2b_cb = NULL;
	buf->i2b_cb_cookie = NULL;

	/* We first allocate some DMA-friendly memory for the buffer... */
	retval = bus_dmamem_alloc(sc->sc_dmat, buf->i2b_size, NBPG, 0,
				  buf->i2b_segs, buf->i2b_nsegs, &buf->i2b_nsegs,
				  dma_flags);
	if (retval != 0) {
		printf("%s: Failed to allocate DMA memory\n", __func__);
		goto cleanup_dealloc;
	}

	DPRINTF(("%s: Using %d DMA segments\n", __func__, buf->i2b_nsegs));

	retval = bus_dmamem_map(sc->sc_dmat, buf->i2b_segs, buf->i2b_nsegs,
				buf->i2b_size, &buf->i2b_addr, dma_flags);

	if (retval != 0) {
		printf("%s: Failed to map DMA memory\n", __func__);
		goto cleanup_dealloc_dma;
	}

	DPRINTF(("%s: Playback DMA buffer mapped at %p\n", __func__,
		 buf->i2b_addr));

	/* XXX: Not sure if nsegments is really 1...*/
	retval = bus_dmamap_create(sc->sc_dmat, buf->i2b_size, 1,
				   buf->i2b_size, 0, dma_flags,
				   &buf->i2b_dmamap);
	if (retval != 0) {
		printf("%s: Failed to create DMA map\n", __func__);
		goto cleanup_unmap_dma;
	}

	DPRINTF(("%s: DMA map created successfully\n", __func__));

	buf->i2b_xfer = s3c2440_dmac_allocate_xfer(M_NOWAIT);
	if (buf->i2b_xfer == NULL) {
		retval = ENOMEM;
		goto cleanup_destroy_dmamap;
	}

	return 0;
cleanup_destroy_dmamap:
	bus_dmamap_destroy(sc->sc_dmat, buf->i2b_dmamap);
 cleanup_unmap_dma:
	bus_dmamem_unmap(sc->sc_dmat, &buf->i2b_addr, buf->i2b_size);
 cleanup_dealloc_dma:
	bus_dmamem_free(sc->sc_dmat, buf->i2b_segs, buf->i2b_nsegs);
 cleanup_dealloc:
	kmem_free(*out, sizeof(struct s3c2440_i2s_buf));
	return retval;
}

void
s3c2440_i2s_free(s3c2440_i2s_buf_t buf)
{
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */

	if (buf->i2b_xfer != NULL) {
		s3c2440_dmac_free_xfer(buf->i2b_xfer);
	}

	bus_dmamap_unload(sc->sc_dmat, buf->i2b_dmamap);
	bus_dmamap_destroy(sc->sc_dmat, buf->i2b_dmamap);
	bus_dmamem_unmap(sc->sc_dmat, &buf->i2b_addr, buf->i2b_size);
	bus_dmamem_free(sc->sc_dmat, buf->i2b_segs, buf->i2b_nsegs);
	kmem_free(buf, sizeof(struct s3c2440_i2s_buf));
}

int
s3c2440_i2s_output(s3c2440_i2s_buf_t buf, void *block, int bsize,
		   void (*callback)(void*), void *cb_cookie)
{
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */
	struct s3c2440_i2s_softc *i2s = buf->i2b_parent;
	int retval;
	dmac_xfer_t xfer = buf->i2b_xfer;

	retval = bus_dmamap_load(sc->sc_dmat, buf->i2b_dmamap, block,
				 bsize, NULL, BUS_DMA_NOWAIT);
	if (retval != 0) {
		printf("Failed to load DMA map\n");
		return retval;
	}

	xfer->dx_desc[DMAC_DESC_DST].xd_bus_type = DMAC_BUS_TYPE_PERIPHERAL;
	xfer->dx_desc[DMAC_DESC_DST].xd_increment = FALSE;
	xfer->dx_desc[DMAC_DESC_DST].xd_nsegs = 1;
	xfer->dx_desc[DMAC_DESC_DST].xd_dma_segs = &i2s->sc_dr;

	xfer->dx_desc[DMAC_DESC_SRC].xd_bus_type = DMAC_BUS_TYPE_SYSTEM;
	xfer->dx_desc[DMAC_DESC_SRC].xd_increment = TRUE;
	xfer->dx_desc[DMAC_DESC_SRC].xd_nsegs = buf->i2b_dmamap->dm_nsegs;
	xfer->dx_desc[DMAC_DESC_SRC].xd_dma_segs = buf->i2b_dmamap->dm_segs;

	xfer->dx_peripheral = DMAC_PERIPH_I2SSDO;

	if (i2s->sc_sample_width == 16)
		xfer->dx_xfer_width = DMAC_XFER_WIDTH_16BIT;
	else if (i2s->sc_sample_width == 8)
		xfer->dx_xfer_width = DMAC_XFER_WIDTH_8BIT;

	xfer->dx_done = s3c2440_i2s_xfer_complete;
	xfer->dx_cookie = buf;
	xfer->dx_xfer_mode = DMAC_XFER_MODE_HANDSHAKE;

	buf->i2b_cb = callback;
	buf->i2b_cb_cookie = cb_cookie;

	s3c2440_dmac_start_xfer(buf->i2b_xfer);

	return 0;
}

int
s3c2440_i2s_halt_output(s3c2440_i2s_buf_t buf)
{
	/*int retval;*/
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */

	DPRINTF(("Aborting DMA transfer\n"));
	/*do {
	  retval =*/ s3c2440_dmac_abort_xfer(buf->i2b_xfer);
/*} while(retval != 0);*/
	DPRINTF(("Aborting DMA transfer: SUCCESS\n"));

	bus_dmamap_unload(sc->sc_dmat, buf->i2b_dmamap);

	return 0;
}

int
s3c2440_i2s_input(s3c2440_i2s_buf_t buf, void *block, int bsize,
		   void (*callback)(void*), void *cb_cookie)
{
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */
	struct s3c2440_i2s_softc *i2s = buf->i2b_parent;
	int retval;
	dmac_xfer_t xfer = buf->i2b_xfer;

	retval = bus_dmamap_load(sc->sc_dmat, buf->i2b_dmamap, block,
				 bsize, NULL, BUS_DMA_NOWAIT);
	if (retval != 0) {
		printf("Failed to load DMA map\n");
		return retval;
	}

	xfer->dx_desc[DMAC_DESC_SRC].xd_bus_type = DMAC_BUS_TYPE_PERIPHERAL;
	xfer->dx_desc[DMAC_DESC_SRC].xd_increment = FALSE;
	xfer->dx_desc[DMAC_DESC_SRC].xd_nsegs = 1;
	xfer->dx_desc[DMAC_DESC_SRC].xd_dma_segs = &i2s->sc_dr;

	xfer->dx_desc[DMAC_DESC_DST].xd_bus_type = DMAC_BUS_TYPE_SYSTEM;
	xfer->dx_desc[DMAC_DESC_DST].xd_increment = TRUE;
	xfer->dx_desc[DMAC_DESC_DST].xd_nsegs = buf->i2b_dmamap->dm_nsegs;
	xfer->dx_desc[DMAC_DESC_DST].xd_dma_segs = buf->i2b_dmamap->dm_segs;

	xfer->dx_peripheral = DMAC_PERIPH_I2SSDI;

	if (i2s->sc_sample_width == 16)
		xfer->dx_xfer_width = DMAC_XFER_WIDTH_16BIT;
	else if (i2s->sc_sample_width == 8)
		xfer->dx_xfer_width = DMAC_XFER_WIDTH_8BIT;

	xfer->dx_done = s3c2440_i2s_xfer_complete;
	xfer->dx_cookie = buf;
	xfer->dx_xfer_mode = DMAC_XFER_MODE_HANDSHAKE;

	buf->i2b_cb = callback;
	buf->i2b_cb_cookie = cb_cookie;

	s3c2440_dmac_start_xfer(buf->i2b_xfer);

	return 0;
}

static void
s3c2440_i2s_xfer_complete(dmac_xfer_t xfer, void *cookie)
{
	struct s3c2xx0_softc *sc = s3c2xx0_softc; /* Shortcut */
	s3c2440_i2s_buf_t buf = cookie;
	struct s3c2440_i2s_softc *i2s = buf->i2b_parent;

	bus_dmamap_unload(sc->sc_dmat, buf->i2b_dmamap);

	mutex_spin_enter(i2s->sc_intr_lock);
	(buf->i2b_cb)(buf->i2b_cb_cookie);
	mutex_spin_exit(i2s->sc_intr_lock);
}
