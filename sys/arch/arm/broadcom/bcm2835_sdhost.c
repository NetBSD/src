/* $NetBSD: bcm2835_sdhost.c,v 1.3.4.2 2017/12/03 11:35:52 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_sdhost.c,v 1.3.4.2 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/gpio.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835_dmac.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#define	SDCMD		0x00
#define	 SDCMD_NEW	__BIT(15)
#define	 SDCMD_FAIL	__BIT(14)
#define	 SDCMD_BUSY	__BIT(11)
#define	 SDCMD_NORESP	__BIT(10)
#define	 SDCMD_LONGRESP	__BIT(9)
#define	 SDCMD_WRITE	__BIT(7)
#define	 SDCMD_READ	__BIT(6)
#define	SDARG		0x04
#define	SDTOUT		0x08
#define	 SDTOUT_DEFAULT	0xf00000
#define	SDCDIV		0x0c
#define	 SDCDIV_MASK	__BITS(10,0)
#define	SDRSP0		0x10
#define	SDRSP1		0x14
#define	SDRSP2		0x18
#define	SDRSP3		0x1c
#define	SDHSTS		0x20
#define	 SDHSTS_BUSY	__BIT(10)
#define	 SDHSTS_BLOCK	__BIT(9)
#define	 SDHSTS_SDIO	__BIT(8)
#define	 SDHSTS_REW_TO	__BIT(7)
#define	 SDHSTS_CMD_TO	__BIT(6)
#define	 SDHSTS_CRC16_E	__BIT(5)
#define	 SDHSTS_CRC7_E	__BIT(4)
#define	 SDHSTS_FIFO_E	__BIT(3)
#define	 SDHSTS_DATA	__BIT(0)
#define	SDVDD		0x30
#define	 SDVDD_POWER	__BIT(0)
#define	SDEDM		0x34
#define	 SDEDM_RD_FIFO	__BITS(18,14)
#define	 SDEDM_WR_FIFO	__BITS(13,9)
#define	SDHCFG		0x38
#define	 SDHCFG_BUSY_EN	__BIT(10)
#define	 SDHCFG_BLOCK_EN __BIT(8)
#define	 SDHCFG_SDIO_EN	__BIT(5)
#define	 SDHCFG_DATA_EN	__BIT(4)
#define	 SDHCFG_SLOW	__BIT(3)
#define	 SDHCFG_WIDE_EXT __BIT(2)
#define	 SDHCFG_WIDE_INT __BIT(1)
#define	 SDHCFG_REL_CMD	__BIT(0)
#define	SDHBCT		0x3c
#define	SDDATA		0x40
#define	SDHBLC		0x50

struct sdhost_softc;

static int	sdhost_match(device_t, cfdata_t, void *);
static void	sdhost_attach(device_t, device_t, void *);
static void	sdhost_attach_i(device_t);

static int	sdhost_intr(void *);
static int	sdhost_dma_setup(struct sdhost_softc *);
static void	sdhost_dma_done(uint32_t, uint32_t, void *);

static int	sdhost_host_reset(sdmmc_chipset_handle_t);
static uint32_t	sdhost_host_ocr(sdmmc_chipset_handle_t);
static int	sdhost_host_maxblklen(sdmmc_chipset_handle_t);
static int	sdhost_card_detect(sdmmc_chipset_handle_t);
static int	sdhost_write_protect(sdmmc_chipset_handle_t);
static int	sdhost_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	sdhost_bus_clock(sdmmc_chipset_handle_t, int, bool);
static int	sdhost_bus_width(sdmmc_chipset_handle_t, int);
static int	sdhost_bus_rod(sdmmc_chipset_handle_t, int);
static void	sdhost_exec_command(sdmmc_chipset_handle_t,
				      struct sdmmc_command *);
static void	sdhost_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	sdhost_card_intr_ack(sdmmc_chipset_handle_t);

static struct sdmmc_chip_functions sdhost_chip_functions = {
	.host_reset = sdhost_host_reset,
	.host_ocr = sdhost_host_ocr,
	.host_maxblklen = sdhost_host_maxblklen,
	.card_detect = sdhost_card_detect,
	.write_protect = sdhost_write_protect,
	.bus_power = sdhost_bus_power,
	.bus_clock_ddr = sdhost_bus_clock,
	.bus_width = sdhost_bus_width,
	.bus_rod = sdhost_bus_rod,
	.exec_command = sdhost_exec_command,
	.card_enable_intr = sdhost_card_enable_intr,
	.card_intr_ack = sdhost_card_intr_ack,
};

struct sdhost_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;

	bus_addr_t sc_addr;

	void *sc_ih;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_intr_cv;
	kcondvar_t sc_dma_cv;

	u_int sc_rate;

	int sc_mmc_width;
	int sc_mmc_present;

	device_t sc_sdmmc_dev;

	struct bcm_dmac_channel *sc_dmac;

	bus_dmamap_t sc_dmamap;
	bus_dma_segment_t sc_segs[1];
	struct bcm_dmac_conblk *sc_cblk;

	uint32_t sc_intr_hsts;

	uint32_t sc_dma_status;
	uint32_t sc_dma_error;
};

CFATTACH_DECL_NEW(bcmsdhost, sizeof(struct sdhost_softc),
	sdhost_match, sdhost_attach, NULL, NULL);

#define SDHOST_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define SDHOST_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
sdhost_match(device_t parent, cfdata_t cf, void *aux)
{
	struct amba_attach_args * const aaa = aux;

	return strcmp(aaa->aaa_name, "sdhost") == 0;
}

static void
sdhost_attach(device_t parent, device_t self, void *aux)
{
	struct sdhost_softc * const sc = device_private(self);
	struct amba_attach_args * const aaa = aux;
	prop_dictionary_t dict = device_properties(self);
	bool disable = false;

	sc->sc_dev = self;
	sc->sc_bst = aaa->aaa_iot;
	sc->sc_dmat = aaa->aaa_dmat;
	sc->sc_addr = aaa->aaa_addr;
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "sdhostintr");
	cv_init(&sc->sc_dma_cv, "sdhostdma");

	if (bus_space_map(sc->sc_bst, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SD HOST controller\n");

	prop_dictionary_get_bool(dict, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}

	prop_dictionary_get_uint32(dict, "frequency", &sc->sc_rate);
	if (sc->sc_rate == 0) {
		aprint_error_dev(self, "couldn't get clock frequency\n");
		return;
	}

	aprint_debug_dev(self, "ref freq %u Hz\n", sc->sc_rate);

	if (sdhost_dma_setup(sc) != 0) {
		aprint_error_dev(self, "failed to setup DMA\n");
		return;
	}

	sc->sc_ih = intr_establish(aaa->aaa_intr, IPL_SDMMC, IST_LEVEL,
	    sdhost_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    aaa->aaa_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on intr %d\n", aaa->aaa_intr);

	config_interrupts(self, sdhost_attach_i);
}

static int
sdhost_dma_setup(struct sdhost_softc *sc)
{
	int error, rseg;

	sc->sc_dmac = bcm_dmac_alloc(BCM_DMAC_TYPE_NORMAL, IPL_SDMMC,
	    sdhost_dma_done, sc);
	if (sc->sc_dmac == NULL)
		return ENXIO;

	error = bus_dmamem_alloc(sc->sc_dmat, PAGE_SIZE, PAGE_SIZE,
	    PAGE_SIZE, sc->sc_segs, 1, &rseg, BUS_DMA_WAITOK);
	if (error)
		return error;

	error = bus_dmamem_map(sc->sc_dmat, sc->sc_segs, rseg, PAGE_SIZE,
	    (void **)&sc->sc_cblk, BUS_DMA_WAITOK);
	if (error)
		return error;

	memset(sc->sc_cblk, 0, PAGE_SIZE);

	error = bus_dmamap_create(sc->sc_dmat, PAGE_SIZE, 1, PAGE_SIZE, 0,
	    BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error)
		return error;

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap, sc->sc_cblk,
	    PAGE_SIZE, NULL, BUS_DMA_WAITOK|BUS_DMA_WRITE);
	if (error)
		return error;

	return 0;
}

static void
sdhost_attach_i(device_t self)
{
	struct sdhost_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	sdhost_host_reset(sc);
	sdhost_bus_width(sc, 1);
	sdhost_bus_clock(sc, 400, false);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &sdhost_chip_functions;
	saa.saa_sch = sc;
	saa.saa_dmat = sc->sc_dmat;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = 50000;
	saa.saa_caps = SMC_CAPS_DMA |
		       SMC_CAPS_MULTI_SEG_DMA |
		       SMC_CAPS_SD_HIGHSPEED |
		       SMC_CAPS_MMC_HIGHSPEED |
		       SMC_CAPS_4BIT_MODE;

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
sdhost_intr(void *priv)
{
	struct sdhost_softc * const sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	const uint32_t hsts = SDHOST_READ(sc, SDHSTS);
	if (!hsts) {
		mutex_exit(&sc->sc_intr_lock);
		return 0;
	}
	SDHOST_WRITE(sc, SDHSTS, hsts);

#ifdef SDHOST_DEBUG
	device_printf(sc->sc_dev, "mmc intr hsts %#x\n", hsts);
#endif

	if (hsts) {
		sc->sc_intr_hsts |= hsts;
		cv_broadcast(&sc->sc_intr_cv);
	}

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static int
sdhost_dma_transfer(struct sdhost_softc *sc, struct sdmmc_command *cmd)
{
	size_t seg;
	int error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	for (seg = 0; seg < cmd->c_dmamap->dm_nsegs; seg++) {
		sc->sc_cblk[seg].cb_ti =
		    __SHIFTIN(13, DMAC_TI_PERMAP); /* SD HOST */
		sc->sc_cblk[seg].cb_txfr_len =
		    cmd->c_dmamap->dm_segs[seg].ds_len;
		/*
		 * All transfers are assumed to be multiples of 32-bits.
		 */
		KASSERTMSG((sc->sc_cblk[seg].cb_txfr_len & 0x3) == 0,
		    "seg %zu len %d", seg, sc->sc_cblk[seg].cb_txfr_len);
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_INC;
			/*
			 * Use 128-bit mode if transfer is a multiple of
			 * 16-bytes.
			 */
			if ((sc->sc_cblk[seg].cb_txfr_len & 0xf) == 0)
				sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_WIDTH;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_DREQ;
			sc->sc_cblk[seg].cb_source_ad =
			    sc->sc_addr + SDDATA;
			sc->sc_cblk[seg].cb_dest_ad =
			    cmd->c_dmamap->dm_segs[seg].ds_addr;
		} else {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_INC;
			/*
			 * Use 128-bit mode if transfer is a multiple of
			 * 16-bytes.
			 */
			if ((sc->sc_cblk[seg].cb_txfr_len & 0xf) == 0)
				sc->sc_cblk[seg].cb_ti |= DMAC_TI_SRC_WIDTH;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_DEST_DREQ;
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_WAIT_RESP;
			sc->sc_cblk[seg].cb_source_ad =
			    cmd->c_dmamap->dm_segs[seg].ds_addr;
			sc->sc_cblk[seg].cb_dest_ad =
			    sc->sc_addr + SDDATA;
		}
		sc->sc_cblk[seg].cb_stride = 0;
		if (seg == cmd->c_dmamap->dm_nsegs - 1) {
			sc->sc_cblk[seg].cb_ti |= DMAC_TI_INTEN;
			sc->sc_cblk[seg].cb_nextconbk = 0;
		} else {
			sc->sc_cblk[seg].cb_nextconbk =
			    sc->sc_dmamap->dm_segs[0].ds_addr +
			    sizeof(struct bcm_dmac_conblk) * (seg+1);
		}
		sc->sc_cblk[seg].cb_padding[0] = 0;
		sc->sc_cblk[seg].cb_padding[1] = 0;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
	    sc->sc_dmamap->dm_mapsize, BUS_DMASYNC_PREWRITE);

	error = 0;

	sc->sc_dma_status = 0;
	sc->sc_dma_error = 0;

	bcm_dmac_set_conblk_addr(sc->sc_dmac,
	    sc->sc_dmamap->dm_segs[0].ds_addr);
	error = bcm_dmac_transfer(sc->sc_dmac);
	if (error)
		return error;

	return 0;
}

static int
sdhost_dma_wait(struct sdhost_softc *sc, struct sdmmc_command *cmd)
{
	int error = 0;

	while (sc->sc_dma_status == 0 && sc->sc_dma_error == 0) {
		error = cv_timedwait(&sc->sc_dma_cv, &sc->sc_intr_lock, hz*5);
		if (error == EWOULDBLOCK) {
			device_printf(sc->sc_dev, "transfer timeout!\n");
			bcm_dmac_halt(sc->sc_dmac);
			error = ETIMEDOUT;
			break;
		}
	}

	if (sc->sc_dma_status & DMAC_CS_END) {
		cmd->c_resid = 0;
		error = 0;
	} else {
		error = EIO;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
	    sc->sc_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);

	return error;
}

static void
sdhost_dma_done(uint32_t status, uint32_t error, void *arg)
{
	struct sdhost_softc * const sc = arg;

	if (status != (DMAC_CS_INT|DMAC_CS_END))
		device_printf(sc->sc_dev, "dma status %#x error %#x\n",
		    status, error);

	mutex_enter(&sc->sc_intr_lock);
	sc->sc_dma_status = status;
	sc->sc_dma_error = error;
	cv_broadcast(&sc->sc_dma_cv);
	mutex_exit(&sc->sc_intr_lock);
}

static int
sdhost_wait_idle(struct sdhost_softc *sc, int timeout)
{
	int retry;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	retry = timeout * 1000;

	while (--retry > 0) {
		const uint32_t cmd = SDHOST_READ(sc, SDCMD);
		if ((cmd & SDCMD_NEW) == 0)
			return 0;
		delay(1);
	}

	return ETIMEDOUT;
}

static int
sdhost_host_reset(sdmmc_chipset_handle_t sch)
{
	struct sdhost_softc * const sc = sch;
	uint32_t edm;

	SDHOST_WRITE(sc, SDVDD, 0);
	SDHOST_WRITE(sc, SDCMD, 0);
	SDHOST_WRITE(sc, SDARG, 0);
	SDHOST_WRITE(sc, SDTOUT, SDTOUT_DEFAULT);
	SDHOST_WRITE(sc, SDCDIV, 0);
	SDHOST_WRITE(sc, SDHSTS, SDHOST_READ(sc, SDHSTS));
	SDHOST_WRITE(sc, SDHCFG, 0);
	SDHOST_WRITE(sc, SDHBCT, 0);
	SDHOST_WRITE(sc, SDHBLC, 0);

	edm = SDHOST_READ(sc, SDEDM);
	edm &= ~(SDEDM_RD_FIFO|SDEDM_WR_FIFO);
	edm |= __SHIFTIN(4, SDEDM_RD_FIFO);
	edm |= __SHIFTIN(4, SDEDM_WR_FIFO);
	SDHOST_WRITE(sc, SDEDM, edm);
	delay(20000);
	SDHOST_WRITE(sc, SDVDD, SDVDD_POWER);
	delay(20000);

	SDHOST_WRITE(sc, SDHCFG, 0);
	SDHOST_WRITE(sc, SDCDIV, SDCDIV_MASK);

	return 0;
}

static uint32_t
sdhost_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V | MMC_OCR_HCS;
}

static int
sdhost_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 8192;
}

static int
sdhost_card_detect(sdmmc_chipset_handle_t sch)
{
	return 1;	/* XXX */
}

static int
sdhost_write_protect(sdmmc_chipset_handle_t sch)
{
	return 0;	/* no write protect pin, assume rw */
}

static int
sdhost_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
sdhost_bus_clock(sdmmc_chipset_handle_t sch, int freq, bool ddr)
{
	struct sdhost_softc * const sc = sch;
	u_int target_rate = freq * 1000;
	int div;

	if (freq == 0)
		div = SDCDIV_MASK;
	else {
		div = sc->sc_rate / target_rate;
		if (div < 2)
			div = 2;
		if ((sc->sc_rate / div) > target_rate)
			div++;
		div -= 2;
		if (div > SDCDIV_MASK)
			div = SDCDIV_MASK;
	}

	SDHOST_WRITE(sc, SDCDIV, div);

	return 0;
}

static int
sdhost_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct sdhost_softc * const sc = sch;
	uint32_t hcfg;

#ifdef SDHOST_DEBUG
	aprint_normal_dev(sc->sc_dev, "width = %d\n", width);
#endif

	hcfg = SDHOST_READ(sc, SDHCFG);
	if (width == 4)
		hcfg |= SDHCFG_WIDE_EXT;
	else
		hcfg &= ~SDHCFG_WIDE_EXT;
	hcfg |= (SDHCFG_WIDE_INT | SDHCFG_SLOW);
	SDHOST_WRITE(sc, SDHCFG, hcfg);

	return 0;
}

static int
sdhost_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return -1;
}

static void
sdhost_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct sdhost_softc * const sc = sch;
	uint32_t cmdval, hcfg;
	u_int nblks;

#ifdef SDHOST_DEBUG
	aprint_normal_dev(sc->sc_dev,
	    "opcode %d flags 0x%x data %p datalen %d blklen %d\n",
	    cmd->c_opcode, cmd->c_flags, cmd->c_data, cmd->c_datalen,
	    cmd->c_blklen);
#endif

	mutex_enter(&sc->sc_intr_lock);

	hcfg = SDHOST_READ(sc, SDHCFG);
	SDHOST_WRITE(sc, SDHCFG, hcfg | SDHCFG_BUSY_EN);

	sc->sc_intr_hsts = 0;

	cmd->c_error = sdhost_wait_idle(sc, 5000);
	if (cmd->c_error != 0) {
#ifdef SDHOST_DEBUG
		device_printf(sc->sc_dev, "device is busy\n");
#endif
		goto done;
	}

	cmdval = SDCMD_NEW;
	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		cmdval |= SDCMD_NORESP;
	if (ISSET(cmd->c_flags, SCF_RSP_136))
		cmdval |= SDCMD_LONGRESP;
	if (ISSET(cmd->c_flags, SCF_RSP_BSY))
		cmdval |= SDCMD_BUSY;

	if (cmd->c_datalen > 0) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ))
			cmdval |= SDCMD_READ;
		else
			cmdval |= SDCMD_WRITE;

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;

		SDHOST_WRITE(sc, SDHBCT, cmd->c_blklen);
		SDHOST_WRITE(sc, SDHBLC, nblks);

		cmd->c_resid = cmd->c_datalen;
		cmd->c_error = sdhost_dma_transfer(sc, cmd);
		if (cmd->c_error != 0) {
#ifdef SDHOST_DEBUG
			device_printf(sc->sc_dev, "dma transfer failed: %d\n",
			    cmd->c_error);
#endif
			goto done;
		}
	}

	SDHOST_WRITE(sc, SDARG, cmd->c_arg);
	SDHOST_WRITE(sc, SDCMD, cmdval | cmd->c_opcode);

	if (cmd->c_datalen > 0) {
		cmd->c_error = sdhost_dma_wait(sc, cmd);
		if (cmd->c_error != 0) {
#ifdef SDHOST_DEBUG
			device_printf(sc->sc_dev,
			    "wait dma failed: %d\n", cmd->c_error);
#endif
			goto done;
		}
	}

	cmd->c_error = sdhost_wait_idle(sc, 5000);
	if (cmd->c_error != 0) {
#ifdef SDHOST_DEBUG
		device_printf(sc->sc_dev,
		    "wait cmd idle (%#x) failed: %d\n",
		    SDHOST_READ(sc, SDCMD), cmd->c_error);
#endif
	}

	if ((SDHOST_READ(sc, SDCMD) & SDCMD_FAIL) != 0) {
#ifdef SDHOST_DEBUG
		device_printf(sc->sc_dev, "SDCMD: %#x\n",
		    SDHOST_READ(sc, SDCMD));
#endif
		cmd->c_error = EIO;
		goto done;
	}

	if (ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			cmd->c_resp[0] = SDHOST_READ(sc, SDRSP0);
			cmd->c_resp[1] = SDHOST_READ(sc, SDRSP1);
			cmd->c_resp[2] = SDHOST_READ(sc, SDRSP2);
			cmd->c_resp[3] = SDHOST_READ(sc, SDRSP3);
			if (ISSET(cmd->c_flags, SCF_RSP_CRC)) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) |
				    (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) |
				    (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) |
				    (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		} else {
			cmd->c_resp[0] = SDHOST_READ(sc, SDRSP0);
		}
	}

done:
	cmd->c_flags |= SCF_ITSDONE;
	SDHOST_WRITE(sc, SDHCFG, hcfg);
	SDHOST_WRITE(sc, SDHSTS, SDHOST_READ(sc, SDHSTS));
	mutex_exit(&sc->sc_intr_lock);

#ifdef SDHOST_DEBUG
	if (cmd->c_error != 0)
		device_printf(sc->sc_dev, "command failed with error %d\n",
		    cmd->c_error);
#endif
}

static void
sdhost_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
}

static void
sdhost_card_intr_ack(sdmmc_chipset_handle_t sch)
{
}
