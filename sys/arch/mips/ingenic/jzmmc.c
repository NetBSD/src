/*	$NetBSD: jzmmc.c,v 1.1 2026/07/26 20:15:17 rkujawa Exp $ */

/*-
 * Copyright (c) 2017, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * Ingenic JZ4780 MMC/SD controller (MSC) driver.
 *
 * The driver has a few peculiarities related to CI20:
 * - only MSC1 is supported
 * - the pins must be muxed here
 * - card detect is a plain GPIO on PF20
 * - card power is a fixed always-on 3.3V rail
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: jzmmc.c,v 1.1 2026/07/26 20:15:17 rkujawa Exp $");

#include "opt_ingenic.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>

#include <mips/ingenic/ingenic_var.h>
#include <mips/ingenic/ingenic_regs.h>

/*
 * SDMA descriptor
 */
#define JZMMC_NDESC		SDMMC_MAXNSEGS
#define JZMMC_DESC_WORDS	4
#define JZMMC_DESC_SIZE		(JZMMC_DESC_WORDS * 4)
#define JZMMC_DESC_NDA		0
#define JZMMC_DESC_DA		1
#define JZMMC_DESC_LEN		2
#define JZMMC_DESC_CMD		3

#ifdef JZMMC_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

/*
 * The CGU MSC1CDR divider runs off the MPLL, which u-boot programs to
 * 1200MHz on the CI20
 */
#define JZMMC_MPLL_KHZ		1200000		/* should be configurable */
#define JZMMC_CDR_DIV		11
#define JZMMC_DEVCLK_KHZ	(JZMMC_MPLL_KHZ / ((JZMMC_CDR_DIV + 1) * 2))

/* SD slot pins: PE20-23 = D0-D3, PE28 = CLK, PE29 = CMD, all function 1 */
static const int jzmmc_sdslot_pins[] = { 20, 21, 22, 23, 28, 29 };
#define JZMMC_GPIO_E		4
#define JZMMC_GPIO_F		5
#define JZMMC_CD_PIN		20	/* PF20, active low */

struct jzmmc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	device_t		sc_sdmmc;
	void			*sc_ih;
	kmutex_t		sc_intr_lock;
	kcondvar_t		sc_intr_cv;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_desc_map;
	bus_dma_segment_t	sc_desc_seg;
	uint32_t		*sc_desc;	/* uncached descriptor ring */
	uint32_t		sc_widthbits;	/* cached CMDAT bus width */
	u_int			sc_dma_fails;	/* silent errors */
	bool			sc_inited;	/* init sent */
	bool			sc_use_intr;	/* handler established */
	bool			sc_use_dma;	/* descriptor ring ready */
};

#define MR4(sc, r)	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (r))
#define MW4(sc, r, v)	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (r), (v))
#define MR2(sc, r)	bus_space_read_2((sc)->sc_bst, (sc)->sc_bsh, (r))
#define MW2(sc, r, v)	bus_space_write_2((sc)->sc_bst, (sc)->sc_bsh, (r), (v))
#define MW1(sc, r, v)	bus_space_write_1((sc)->sc_bst, (sc)->sc_bsh, (r), (v))

#define JZMMC_FIFO_BURST	16

#define JZMMC_REG_SIZE		0x100	/* registers end at RTCNT, 0x5c */

static int	jzmmc_match(device_t, cfdata_t, void *);
static void	jzmmc_attach(device_t, device_t, void *);

static int	jzmmc_host_reset(sdmmc_chipset_handle_t);
static uint32_t	jzmmc_host_ocr(sdmmc_chipset_handle_t);
static int	jzmmc_host_maxblklen(sdmmc_chipset_handle_t);
static int	jzmmc_card_detect(sdmmc_chipset_handle_t);
static int	jzmmc_write_protect(sdmmc_chipset_handle_t);
static int	jzmmc_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	jzmmc_bus_clock(sdmmc_chipset_handle_t, int);
static int	jzmmc_bus_width(sdmmc_chipset_handle_t, int);
static int	jzmmc_bus_rod(sdmmc_chipset_handle_t, int);
static void	jzmmc_exec_command(sdmmc_chipset_handle_t,
		    struct sdmmc_command *);
static void	jzmmc_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	jzmmc_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions jzmmc_chip_functions = {
	.host_reset		= jzmmc_host_reset,
	.host_ocr		= jzmmc_host_ocr,
	.host_maxblklen		= jzmmc_host_maxblklen,
	.card_detect		= jzmmc_card_detect,
	.write_protect		= jzmmc_write_protect,
	.bus_power		= jzmmc_bus_power,
	.bus_clock		= jzmmc_bus_clock,
	.bus_width		= jzmmc_bus_width,
	.bus_rod		= jzmmc_bus_rod,
	.exec_command		= jzmmc_exec_command,
	.card_enable_intr	= jzmmc_card_enable_intr,
	.card_intr_ack		= jzmmc_card_intr_ack,
};

CFATTACH_DECL_NEW(jzmmc, sizeof(struct jzmmc_softc),
    jzmmc_match, jzmmc_attach, NULL, NULL);

/*
 * Polled fallback, used before the interrupt handler is established.
 */
static int
jzmmc_wait_iflg_poll(struct jzmmc_softc *sc, uint32_t mask, int timo_ms)
{
	int us = timo_ms * 1000;
	int spin;

	for (spin = 200; spin > 0 && us > 0; spin--) {
		if (MR4(sc, JZ_MSC_IFLG) & mask)
			return 0;
		delay(5);
		us -= 5;
	}
	while (us > 0) {
		if (MR4(sc, JZ_MSC_IFLG) & mask)
			return 0;
		kpause("jzmmcw", false, mstohz(10) ? mstohz(10) : 1, NULL);
		us -= 10000;
	}
	return ETIMEDOUT;
}

/*
 * Interrupt-driven wait
 */
static int
jzmmc_wait_iflg(struct jzmmc_softc *sc, uint32_t mask, int timo_ms)
{
	int error, timo;

	if (!sc->sc_use_intr || cold) {
		error = jzmmc_wait_iflg_poll(sc, mask, timo_ms);
		goto out;
	}

	mutex_enter(&sc->sc_intr_lock);
	MW4(sc, JZ_MSC_IMASK, MR4(sc, JZ_MSC_IMASK) & ~mask);
	timo = mstohz(timo_ms) ? mstohz(timo_ms) : 1;
	while ((MR4(sc, JZ_MSC_IFLG) & mask) == 0) {
		error = cv_timedwait(&sc->sc_intr_cv, &sc->sc_intr_lock, timo);
		if (error != 0)
			break;
	}
	MW4(sc, JZ_MSC_IMASK, MR4(sc, JZ_MSC_IMASK) | mask);
	error = (MR4(sc, JZ_MSC_IFLG) & mask) ? 0 : ETIMEDOUT;
	mutex_exit(&sc->sc_intr_lock);
out:
	if (error != 0) {
		DPRINTF("%s: timeout waiting for iflg %08x "
		    "(iflg %08x stat %08x)\n",
		    device_xname(sc->sc_dev), mask, MR4(sc, JZ_MSC_IFLG),
		    MR4(sc, JZ_MSC_STAT));
	}
	return error;
}

static int
jzmmc_intr(void *arg)
{
	struct jzmmc_softc *sc = arg;
	uint32_t pend, imask;

	mutex_enter(&sc->sc_intr_lock);
	imask = MR4(sc, JZ_MSC_IMASK);
	pend = MR4(sc, JZ_MSC_IFLG) & ~imask;
	if (pend == 0) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}
	MW4(sc, JZ_MSC_IMASK, imask | pend);
	cv_broadcast(&sc->sc_intr_cv);
	mutex_exit(&sc->sc_intr_lock);
	return 1;
}

static int
jzmmc_controller_reset(struct jzmmc_softc *sc)
{
	int timo;

	MW2(sc, JZ_MSC_CTRL, JZ_RESET);
	delay(20);
	for (timo = 1000; timo > 0; timo--) {
		if ((MR4(sc, JZ_MSC_STAT) & JZ_IS_RESETTING) == 0)
			break;
		delay(10);
	}
	if (timo == 0)
		return ETIMEDOUT;

	/* mask all interrupts (we poll), ack any stale flags */
	MW4(sc, JZ_MSC_IMASK, 0xffffffff);
	MW4(sc, JZ_MSC_IFLG, 0xffffffff);
	/* generous hardware timeouts, in MSC_CLK cycles */
	MW2(sc, JZ_MSC_RESTO, 0xffff);
	MW4(sc, JZ_MSC_RDTO, 0xffffffff);
	/* keep the internal DMA engine off the FIFO between transfers */
	MW4(sc, JZ_MSC_DMAC, 0);
	return 0;
}

/*
 * Allocate the SDMA descriptor ring
 */
static int
jzmmc_dma_init(struct jzmmc_softc *sc)
{
	int error, rseg;
	void *kva;

	error = bus_dmamem_alloc(sc->sc_dmat, JZMMC_NDESC * JZMMC_DESC_SIZE,
	    JZMMC_DESC_SIZE, 0, &sc->sc_desc_seg, 1, &rseg, BUS_DMA_WAITOK);
	if (error != 0)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, &sc->sc_desc_seg, rseg,
	    JZMMC_NDESC * JZMMC_DESC_SIZE, &kva,
	    BUS_DMA_WAITOK | BUS_DMA_COHERENT);
	if (error != 0)
		goto free;
	error = bus_dmamap_create(sc->sc_dmat, JZMMC_NDESC * JZMMC_DESC_SIZE,
	    1, JZMMC_NDESC * JZMMC_DESC_SIZE, 0, BUS_DMA_WAITOK,
	    &sc->sc_desc_map);
	if (error != 0)
		goto unmap;
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_desc_map, kva,
	    JZMMC_NDESC * JZMMC_DESC_SIZE, NULL, BUS_DMA_WAITOK);
	if (error != 0)
		goto destroy;
	sc->sc_desc = kva;
	return 0;

destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_desc_map);
unmap:
	bus_dmamem_unmap(sc->sc_dmat, kva, JZMMC_NDESC * JZMMC_DESC_SIZE);
free:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_desc_seg, rseg);
	return error;
}

/*
 * Build the descriptor chain for cmd's dmamap, one descriptor per
 * segment, and hand it to the engine.
 */
static void
jzmmc_dma_prepare(struct jzmmc_softc *sc, struct sdmmc_command *cmd)
{
	bus_dmamap_t map = cmd->c_dmamap;
	const bus_addr_t ring = sc->sc_desc_map->dm_segs[0].ds_addr;
	bus_size_t resid = cmd->c_datalen, len;
	uint32_t *d;
	int seg;

	KASSERT(map->dm_nsegs > 0 && map->dm_nsegs <= JZMMC_NDESC);
	KASSERT(map->dm_mapsize >= cmd->c_datalen);
	for (seg = 0; resid > 0; seg++) {
		KASSERT(seg < map->dm_nsegs);
		KASSERT(((map->dm_segs[seg].ds_addr |
		    map->dm_segs[seg].ds_len) & 0x3) == 0);
		len = MIN(map->dm_segs[seg].ds_len, resid);
		resid -= len;
		d = &sc->sc_desc[seg * JZMMC_DESC_WORDS];
		d[JZMMC_DESC_NDA] = resid == 0 ? 0 :
		    ring + (seg + 1) * JZMMC_DESC_SIZE;
		d[JZMMC_DESC_DA] = map->dm_segs[seg].ds_addr;
		d[JZMMC_DESC_LEN] = len;
		d[JZMMC_DESC_CMD] = resid == 0 ?
		    (JZ_DMA_ENDI | JZ_DMA_LINK) : 0;
	}
	wbflush();
	MW4(sc, JZ_MSC_DMANDA, ring);
}

static int
jzmmc_match(device_t parent, cfdata_t match, void *aux)
{
	struct apbus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, "jzmmc") != 0)
		return 0;
	if (aa->aa_addr != JZ_MSC1_BASE)
		return 0;
	return 1;
}

static void
jzmmc_attach(device_t parent, device_t self, void *aux)
{
	struct jzmmc_softc *sc = device_private(self);
	struct apbus_attach_args *aa = aux;
	struct sdmmcbus_attach_args saa;
	int error;
	size_t i;

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_bst;
	sc->sc_dmat = aa->aa_dmat;
	sc->sc_widthbits = JZ_BUS_1BIT;
	sc->sc_inited = false;

	aprint_naive("\n");
	aprint_normal(": SD/MMC controller\n");

	error = bus_space_map(aa->aa_bst, aa->aa_addr, JZMMC_REG_SIZE, 0, &sc->sc_bsh);
	if (error != 0) {
		aprint_error_dev(self, "can't map registers: %d\n", error);
		return;
	}

	/* mux the SD-slot pins to MSC1 (port E function 1), CD as input */
	for (i = 0; i < __arraycount(jzmmc_sdslot_pins); i++)
		gpio_as_dev1(JZMMC_GPIO_E, jzmmc_sdslot_pins[i]);
	gpio_as_input(JZMMC_GPIO_F, JZMMC_CD_PIN);

	/* device clock from the CGU: MPLL / ((div+1)*2) */
	error = ingenic_cdr_set(aa->aa_clockreg,
	    MSCCDR_MPLL | JZMMC_CDR_DIV);
	if (error != 0) {
		aprint_error_dev(self, "clock divider stuck busy\n");
		goto fail;
	}
	aprint_normal_dev(self, "device clock %d kHz (MPLL assumed %d kHz)\n",
	    JZMMC_DEVCLK_KHZ, JZMMC_MPLL_KHZ);

	if (jzmmc_controller_reset(sc) != 0) {
		aprint_error_dev(self, "controller stuck in reset, stat %08x\n",
		    MR4(sc, JZ_MSC_STAT));
		goto fail;
	}

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_intr_cv, "jzmmcirq");

	/*
	 * All interrupts are masked (and stay masked outside of waits),
	 * so establishing the handler this early is safe.
	 */
	sc->sc_ih = evbmips_intr_establish(aa->aa_irq, jzmmc_intr, sc);
	if (sc->sc_ih == NULL)
		aprint_error_dev(self, "can't establish interrupt, polling\n");
	sc->sc_use_intr = sc->sc_ih != NULL;

	error = jzmmc_dma_init(sc);
	if (error != 0)
		aprint_error_dev(self,
		    "can't set up DMA descriptors (%d), using PIO\n", error);
	sc->sc_use_dma = error == 0;

	aprint_normal_dev(self, "card %spresent\n",
	    jzmmc_card_detect(sc) ? "" : "not ");

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &jzmmc_chip_functions;
	saa.saa_sch = sc;
	saa.saa_dmat = aa->aa_dmat;
	saa.saa_clkmin = JZMMC_DEVCLK_KHZ / 128;
	/*
	 * 50MHz (CLKRT = 0) is only used for SD high-speed timing
	 */
	saa.saa_clkmax = JZMMC_DEVCLK_KHZ;
	saa.saa_caps = SMC_CAPS_4BIT_MODE | SMC_CAPS_POLL_CARD_DET |
	    SMC_CAPS_SD_HIGHSPEED | SMC_CAPS_AUTO_STOP;
	if (sc->sc_use_dma) {
		saa.saa_caps |= SMC_CAPS_DMA | SMC_CAPS_MULTI_SEG_DMA;
		/* the framework bounces buffers this can't express */
		saa.saa_dma_align_mask = 0x3;
	}

	sc->sc_sdmmc = config_found(self, &saa, NULL, CFARGS_NONE);
	return;

fail:
	bus_space_unmap(aa->aa_bst, sc->sc_bsh, JZMMC_REG_SIZE);
}

static int
jzmmc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct jzmmc_softc *sc = sch;

	sc->sc_inited = false;
	return jzmmc_controller_reset(sc);
}

static uint32_t
jzmmc_host_ocr(sdmmc_chipset_handle_t sch)
{

	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

static int
jzmmc_host_maxblklen(sdmmc_chipset_handle_t sch)
{

	return 512;
}

static int
jzmmc_card_detect(sdmmc_chipset_handle_t sch)
{

	/* PF20, active low */
	return !gpio_get(JZMMC_GPIO_F, JZMMC_CD_PIN);
}

static int
jzmmc_write_protect(sdmmc_chipset_handle_t sch)
{

	return 0;	/* no write-protect switch wired up */
}

static int
jzmmc_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{

	return 0;	/* fixed always-on 3.3V */
}

static int
jzmmc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct jzmmc_softc *sc = sch;
	int n;

	if (freq == 0) {
		MW2(sc, JZ_MSC_CTRL, JZ_CLOCK_STOP);
		return 0;
	}
	/* smallest 2^n divider with devclk/2^n <= freq (in kHz) */
	for (n = 0; n < 7; n++) {
		if ((JZMMC_DEVCLK_KHZ >> n) <= freq)
			break;
	}
	MW2(sc, JZ_MSC_CLKRT, n);
	/*
	 * Above 25MHz the controller MUST drive CMD/DAT a quarter clock 
	 * early and sample on the rising edge... without this every 
	 * transfer fails with CRC errors.
	 */
	if ((JZMMC_DEVCLK_KHZ >> n) > 25000)
		MW4(sc, JZ_MSC_LPM, JZ_RISING_4 | JZ_SMP_SEL | JZ_LPM);
	else
		MW4(sc, JZ_MSC_LPM, 0);
	DPRINTF("%s: clock req %d kHz -> CLKRT %d (%d kHz)\n",
	    device_xname(sc->sc_dev), freq, n, JZMMC_DEVCLK_KHZ >> n);
	return 0;
}

static int
jzmmc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct jzmmc_softc *sc = sch;

	switch (width) {
	case 1:
		sc->sc_widthbits = JZ_BUS_1BIT;
		break;
	case 4:
		sc->sc_widthbits = JZ_BUS_4BIT;
		break;
	default:
		return EINVAL;
	}
	return 0;
}

static int
jzmmc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{

	return ENOTSUP;
}

static void
jzmmc_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	/* no SDIO support */
}

static void
jzmmc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	/* no SDIO support */
}

static void
jzmmc_read_response(struct jzmmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t b[4];
	uint16_t r[9];
	int i;

	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		return;

	if (ISSET(cmd->c_flags, SCF_RSP_136)) {
		bus_space_read_multi_2(sc->sc_bst, sc->sc_bsh, JZ_MSC_RES,
		    r, 9);
		/* byte-carried halfwords: b[i] spans r[2i]..r[2i+2] */
		for (i = 0; i < 4; i++) {
			b[i] = ((uint32_t)r[2 * i] << 24) |
			    ((uint32_t)r[2 * i + 1] << 8) |
			    ((uint32_t)r[2 * i + 2] >> 8);
		}
		/* b[0] = bits 127:96 of the payload ... b[3] = bits 31:0 */
		cmd->c_resp[0] = (b[2] << 24) | (b[3] >> 8);
		cmd->c_resp[1] = (b[1] << 24) | (b[2] >> 8);
		cmd->c_resp[2] = (b[0] << 24) | (b[1] >> 8);
		cmd->c_resp[3] = b[0] >> 8;
		DPRINTF("%s: R2 raw %08x %08x %08x %08x -> resp %08x %08x %08x %08x\n",
		    device_xname(sc->sc_dev), b[0], b[1], b[2], b[3],
		    cmd->c_resp[0], cmd->c_resp[1], cmd->c_resp[2],
		    cmd->c_resp[3]);
	} else {
		bus_space_read_multi_2(sc->sc_bst, sc->sc_bsh, JZ_MSC_RES,
		    r, 3);
		cmd->c_resp[0] = ((uint32_t)r[0] << 24) |
		    ((uint32_t)r[1] << 8) | (r[2] & 0xff);
	}
}

static int
jzmmc_pio_read(struct jzmmc_softc *sc, struct sdmmc_command *cmd)
{
	uint32_t *p = cmd->c_data;
	size_t nwords = cmd->c_datalen / 4;
	uint32_t stat;
	int guard, spin;

	DPRINTF("%s: pio_read %zu words, rtcnt %u iflg %08x stat %08x\n",
	    device_xname(sc->sc_dev), nwords, MR4(sc, JZ_MSC_RTCNT),
	    MR4(sc, JZ_MSC_IFLG), MR4(sc, JZ_MSC_STAT));
	guard = 100;		/* ~1s: 100 x 10ms kpause */
	spin = 2000;
	while (nwords > 0) {
		stat = MR4(sc, JZ_MSC_STAT);
		if (stat & JZ_TIME_OUT_READ)
			return ETIMEDOUT;
		if (stat & JZ_CRC_READ_ERR)
			return EIO;
		/*
		 * the FIFO carries a byte stream, which a byte-swapping 
		 * bus_space must not reorder. Burst when the trigger 
		 * level guarantees a full JZMMC_FIFO_BURST, drain 
		 * word-wise below it.
		 */
		if (nwords >= JZMMC_FIFO_BURST &&
		    (MR4(sc, JZ_MSC_IFLG) & JZ_INT_RXFIFO_RD_REQ)) {
			bus_space_read_multi_stream_4(sc->sc_bst, sc->sc_bsh,
			    JZ_MSC_RXFIFO, p, JZMMC_FIFO_BURST);
			p += JZMMC_FIFO_BURST;
			nwords -= JZMMC_FIFO_BURST;
			spin = 2000;
			continue;
		}
		if ((stat & JZ_DATA_FIFO_EMPTY) == 0) {
			bus_space_read_multi_stream_4(sc->sc_bst, sc->sc_bsh,
			    JZ_MSC_RXFIFO, p, 1);
			p++;
			nwords--;
			spin = 2000;
			continue;
		}
		if (spin > 0) {
			spin--;
			delay(1);
		} else {
			if (--guard == 0)
				return ETIMEDOUT;
			kpause("jzmmcr", false,
			    mstohz(10) ? mstohz(10) : 1, NULL);
		}
	}
	return 0;
}

static int
jzmmc_pio_write(struct jzmmc_softc *sc, struct sdmmc_command *cmd)
{
	const uint32_t *p = cmd->c_data;
	size_t nwords = cmd->c_datalen / 4;
	uint32_t stat;
	int guard, spin;

	guard = 100;
	spin = 2000;
	while (nwords > 0) {
		stat = MR4(sc, JZ_MSC_STAT);
		if ((stat & JZ_CRC_WRITE_ERR_M) != JZ_CRC_WRITE_OK)
			return EIO;
		if (nwords >= JZMMC_FIFO_BURST &&
		    (MR4(sc, JZ_MSC_IFLG) & JZ_INT_TXFIFO_WR_REQ)) {
			bus_space_write_multi_stream_4(sc->sc_bst, sc->sc_bsh,
			    JZ_MSC_TXFIFO, p, JZMMC_FIFO_BURST);
			p += JZMMC_FIFO_BURST;
			nwords -= JZMMC_FIFO_BURST;
			spin = 2000;
			continue;
		}
		if ((stat & JZ_DATA_FIFO_FULL) == 0) {
			bus_space_write_multi_stream_4(sc->sc_bst, sc->sc_bsh,
			    JZ_MSC_TXFIFO, p, 1);
			p++;
			nwords--;
			spin = 2000;
			continue;
		}
		if (spin > 0) {
			spin--;
			delay(1);
		} else {
			if (--guard == 0)
				return ETIMEDOUT;
			kpause("jzmmcw", false,
			    mstohz(10) ? mstohz(10) : 1, NULL);
		}
	}
	return 0;
}

static int
jzmmc_dma_wait_read(struct jzmmc_softc *sc, struct sdmmc_command *cmd)
{
	const uint32_t errbits = JZ_INT_TIMEOUT_READ | JZ_INT_CRC_READ_ERR;
	uint32_t stat;
	int error;

	error = jzmmc_wait_iflg(sc, JZ_INT_DMAEND | errbits, 5000);
	stat = MR4(sc, JZ_MSC_STAT);
	if (error != 0) {
		if ((stat & (JZ_TIME_OUT_READ | JZ_CRC_READ_ERR)) == 0) {
			if (sc->sc_dma_fails++ < 5)
				device_printf(sc->sc_dev,
				    "silent DMA fail: dmanda %08x "
				    "dmada %08x dmalen %08x dmacmd %08x "
				    "snob %d\n",
				    MR4(sc, JZ_MSC_DMANDA),
				    MR4(sc, JZ_MSC_DMADA),
				    MR4(sc, JZ_MSC_DMALEN),
				    MR4(sc, JZ_MSC_DMACMD),
				    MR2(sc, JZ_MSC_SNOB));
		}
		return error;
	}
	MW4(sc, JZ_MSC_IFLG,
	    MR4(sc, JZ_MSC_IFLG) & (JZ_INT_DMAEND | errbits));
	if (stat & JZ_TIME_OUT_READ)
		return ETIMEDOUT;
	if (stat & JZ_CRC_READ_ERR)
		return EIO;
	return 0;
}

static void
jzmmc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct jzmmc_softc *sc = sch;
	uint32_t cmdat, stat, iflg;
	bool dma = false, autostop = false;
	int error = 0;

#ifdef JZMMC_DEBUG_VERBOSE
	printf("%s: CMD%d arg %08x flags %#x datalen %d\n",
	    device_xname(sc->sc_dev), cmd->c_opcode, cmd->c_arg,
	    cmd->c_flags, cmd->c_datalen);
#endif

	cmdat = sc->sc_widthbits;

	/* response format */
	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		cmdat |= JZ_RES_NONE;
	else if (ISSET(cmd->c_flags, SCF_RSP_136))
		cmdat |= JZ_RES_R2;
	else if (!ISSET(cmd->c_flags, SCF_RSP_CRC))
		cmdat |= JZ_RES_R3;
	else {
		cmdat |= JZ_RES_R1;
		if (ISSET(cmd->c_flags, SCF_RSP_BSY))
			cmdat |= JZ_BUSY;
	}

	if (!sc->sc_inited) {
		cmdat |= JZ_INIT;
		sc->sc_inited = true;
	}

	if (cmd->c_datalen > 0) {
		if (cmd->c_blklen == 0 || (cmd->c_datalen % 4) != 0) {
			error = EINVAL;
			goto done;
		}
		cmdat |= JZ_DATA_EN;
		if (!ISSET(cmd->c_flags, SCF_CMD_READ))
			cmdat |= JZ_WRITE;
		dma = sc->sc_use_dma && cmd->c_dmamap != NULL;
		if (dma) {
			jzmmc_dma_prepare(sc, cmd);
		} else {
			/*
			 * PIO: the internal descriptor DMA must be explicitly
			 * disabled or it owns the data FIFO's read port 
			 */
			MW4(sc, JZ_MSC_DMAC, 0);
		}
		MW2(sc, JZ_MSC_BLKLEN, cmd->c_blklen);
		MW2(sc, JZ_MSC_NOB, cmd->c_datalen / cmd->c_blklen);
		/*
		 * Multi-block transfers need STOP_TRANSMISSION
		 */
		if (cmd->c_datalen > cmd->c_blklen) {
			cmdat |= JZ_AUTO_CMD12;
			autostop = true;
		}
	}

	MW4(sc, JZ_MSC_IFLG, 0xffffffff);	/* ack stale flags */
	MW1(sc, JZ_MSC_CMD, cmd->c_opcode);
	MW4(sc, JZ_MSC_ARG, cmd->c_arg);
	MW4(sc, JZ_MSC_CMDAT, cmdat);
	MW2(sc, JZ_MSC_CTRL, JZ_CLOCK_START | JZ_START_OP);
	/*
	 * The engine may be enabled only once the operation has started
	 */
	if (dma)
		MW4(sc, JZ_MSC_DMAC, JZ_INCR_16 | JZ_DMAEN);


	error = jzmmc_wait_iflg(sc, JZ_INT_EMD_CMD_RES, 200);
	if (error != 0)
		goto done;
	/*
	 * Snapshot both status views BEFORE acknowledging anything
	 */
	iflg = MR4(sc, JZ_MSC_IFLG);
	stat = MR4(sc, JZ_MSC_STAT);
	if ((stat & JZ_TIME_OUT_RES) || (iflg & JZ_INT_TIMEOUT_RES)) {
		MW4(sc, JZ_MSC_IFLG,
		    JZ_INT_EMD_CMD_RES | JZ_INT_TIMEOUT_RES);
		/*
		 * ensure that we don't get phantom cards
		 */
		error = ETIMEDOUT;
		goto done;
	}
	MW4(sc, JZ_MSC_IFLG, JZ_INT_EMD_CMD_RES);
	if ((stat & JZ_CRC_RES_ERR) && ISSET(cmd->c_flags, SCF_RSP_CRC)) {
		error = EIO;
		goto done;
	}

	jzmmc_read_response(sc, cmd);

	if (cmd->c_datalen > 0) {
		if (dma) {
			/* DMA writes complete via the common tail below */
			if (ISSET(cmd->c_flags, SCF_CMD_READ))
				error = jzmmc_dma_wait_read(sc, cmd);
		} else if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			error = jzmmc_pio_read(sc, cmd);
		} else {
			error = jzmmc_pio_write(sc, cmd);
		}
#ifdef JZMMC_DEBUG
		if (error == 0 && cmd->c_datalen <= 8 && !dma &&
		    ISSET(cmd->c_flags, SCF_CMD_READ)) {
			const uint8_t *db = cmd->c_data;
			printf("%s: CMD%d data:", device_xname(sc->sc_dev),
			    cmd->c_opcode);
			for (size_t j = 0; j < cmd->c_datalen; j++)
				printf(" %02x", db[j]);
			printf("\n");
		}
#endif
		if (error != 0)
			goto done;

		error = jzmmc_wait_iflg(sc, JZ_INT_DATA_TRAN_DONE, 5000);
		if (error != 0)
			goto done;
		MW4(sc, JZ_MSC_IFLG, JZ_INT_DATA_TRAN_DONE);

		if (autostop) {
			/* controller-generated CMD12 (and its response) */
			error = jzmmc_wait_iflg(sc,
			    JZ_INT_AUTO_CMD12_DONE, 5000);
			if (error != 0)
				goto done;
			MW4(sc, JZ_MSC_IFLG, JZ_INT_AUTO_CMD12_DONE);
		}

		if (!ISSET(cmd->c_flags, SCF_CMD_READ)) {
			/* wait for the card to finish programming */
			error = jzmmc_wait_iflg(sc, JZ_INT_PRG_DONE, 5000);
			if (error != 0)
				goto done;
			MW4(sc, JZ_MSC_IFLG, JZ_INT_PRG_DONE);
			stat = MR4(sc, JZ_MSC_STAT);
			if ((stat & JZ_CRC_WRITE_ERR_M) != JZ_CRC_WRITE_OK)
				error = EIO;
		}
		/*
		 * with the transfer fully over, take the engine off the FIFO
		 */
		if (dma)
			MW4(sc, JZ_MSC_DMAC, 0);
	}

done:
	if (error != 0) {
		DPRINTF("%s: CMD%d error %d (stat %08x iflg %08x snob %d)\n",
		    device_xname(sc->sc_dev), cmd->c_opcode, error,
		    MR4(sc, JZ_MSC_STAT), MR4(sc, JZ_MSC_IFLG),
		    MR2(sc, JZ_MSC_SNOB));
		if (dma)
			MW4(sc, JZ_MSC_DMAC, 0);
		/*
		 * Reset the controller so a failed command doesn't
		 * break the following ones.
		 */
		jzmmc_controller_reset(sc);
	}
	cmd->c_error = error;
	SET(cmd->c_flags, SCF_ITSDONE);
}

