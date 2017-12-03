/* $NetBSD: amlogic_sdio.c,v 1.5.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amlogic_sdio.c,v 1.5.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_sdioreg.h>
#include <arm/amlogic/amlogic_var.h>

static int	amlogic_sdio_match(device_t, cfdata_t, void *);
static void	amlogic_sdio_attach(device_t, device_t, void *);
static void	amlogic_sdio_attach_i(device_t);

static int	amlogic_sdio_intr(void *);

struct amlogic_sdio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	void			*sc_ih;

	uint32_t		sc_bus_freq;
	u_int			sc_cur_width;
	int			sc_cur_port;

	device_t		sc_sdmmc_dev;
	kmutex_t		sc_intr_lock;
	kcondvar_t		sc_intr_cv;

	uint32_t		sc_intr_irqs;

	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_segs[1];
	void			*sc_bbuf;
};

CFATTACH_DECL_NEW(amlogic_sdio, sizeof(struct amlogic_sdio_softc),
	amlogic_sdio_match, amlogic_sdio_attach, NULL, NULL);

static int	amlogic_sdio_host_reset(sdmmc_chipset_handle_t);
static uint32_t	amlogic_sdio_host_ocr(sdmmc_chipset_handle_t);
static int	amlogic_sdio_host_maxblklen(sdmmc_chipset_handle_t);
static int	amlogic_sdio_card_detect(sdmmc_chipset_handle_t);
static int	amlogic_sdio_write_protect(sdmmc_chipset_handle_t);
static int	amlogic_sdio_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	amlogic_sdio_bus_clock(sdmmc_chipset_handle_t, int);
static int	amlogic_sdio_bus_width(sdmmc_chipset_handle_t, int);
static int	amlogic_sdio_bus_rod(sdmmc_chipset_handle_t, int);
static void	amlogic_sdio_exec_command(sdmmc_chipset_handle_t,
				     struct sdmmc_command *);
static void	amlogic_sdio_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	amlogic_sdio_card_intr_ack(sdmmc_chipset_handle_t);

static int	amlogic_sdio_set_clock(struct amlogic_sdio_softc *, u_int);
static int	amlogic_sdio_wait_irqs(struct amlogic_sdio_softc *, uint32_t, int);

static void	amlogic_sdio_dmainit(struct amlogic_sdio_softc *);

static struct sdmmc_chip_functions amlogic_sdio_chip_functions = {
	.host_reset = amlogic_sdio_host_reset,
	.host_ocr = amlogic_sdio_host_ocr,
	.host_maxblklen = amlogic_sdio_host_maxblklen,
	.card_detect = amlogic_sdio_card_detect,
	.write_protect = amlogic_sdio_write_protect,
	.bus_power = amlogic_sdio_bus_power,
	.bus_clock = amlogic_sdio_bus_clock,
	.bus_width = amlogic_sdio_bus_width,
	.bus_rod = amlogic_sdio_bus_rod,
	.exec_command = amlogic_sdio_exec_command,
	.card_enable_intr = amlogic_sdio_card_enable_intr,
	.card_intr_ack = amlogic_sdio_card_intr_ack,
};

#define SDIO_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define SDIO_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static int
amlogic_sdio_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_sdio_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_sdio_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	uint32_t boot_id;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	sc->sc_cur_port = loc->loc_port;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "sdiointr");

	if (sc->sc_cur_port == AMLOGICIOCF_PORT_DEFAULT) {
		if (!prop_dictionary_get_uint32(cfg, "boot_id", &boot_id)) {
			aprint_error(": no port selected\n");
			return;
		}
		/* Non-booted device goes on SDIO controller */
		if (boot_id == 0) {
			sc->sc_cur_port = AMLOGIC_SDIO_PORT_B;	/* SD card */
		} else {
			sc->sc_cur_port = AMLOGIC_SDIO_PORT_C;	/* eMMC */
		}
	}

	amlogic_sdio_init();
	if (amlogic_sdio_select_port(sc->sc_cur_port) != 0) {
		aprint_error(": couldn't select port %d\n", sc->sc_cur_port);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SDIO controller (port %c)\n", sc->sc_cur_port + 'A');

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_BIO, IST_EDGE,
	    amlogic_sdio_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	sc->sc_bus_freq = amlogic_get_rate_clk81();
	aprint_debug_dev(self, "CLK81 rate: %u Hz\n", sc->sc_bus_freq);

	amlogic_sdio_dmainit(sc);

	config_interrupts(self, amlogic_sdio_attach_i);
}

static void
amlogic_sdio_attach_i(device_t self)
{
	struct amlogic_sdio_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	amlogic_sdio_host_reset(sc);
	amlogic_sdio_bus_clock(sc, 400);
	amlogic_sdio_bus_width(sc, 1);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &amlogic_sdio_chip_functions;
	saa.saa_dmat = sc->sc_dmat;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = sc->sc_bus_freq;
	/* Do not advertise DMA capabilities, we handle DMA ourselves */
	saa.saa_caps = SMC_CAPS_4BIT_MODE|
		       SMC_CAPS_SD_HIGHSPEED|
		       SMC_CAPS_MMC_HIGHSPEED;

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL);
}

static int
amlogic_sdio_intr(void *priv)
{
	struct amlogic_sdio_softc *sc = priv;

	mutex_enter(&sc->sc_intr_lock);
	const u_int irqs = SDIO_READ(sc, SDIO_IRQS_REG);
	if (irqs & SDIO_IRQS_CLEAR) {
		SDIO_WRITE(sc, SDIO_IRQS_REG, irqs);
		sc->sc_intr_irqs |= irqs;
		cv_broadcast(&sc->sc_intr_cv);
	}
	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

static void
amlogic_sdio_dmainit(struct amlogic_sdio_softc *sc)
{
	int error, rseg;

	error = bus_dmamem_alloc(sc->sc_dmat, MAXPHYS, PAGE_SIZE, MAXPHYS,
	    sc->sc_segs, 1, &rseg, BUS_DMA_WAITOK);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamem_alloc failed\n");
		return;
	}
	KASSERT(rseg == 1);

	error = bus_dmamem_map(sc->sc_dmat, sc->sc_segs, rseg, MAXPHYS,
	    &sc->sc_bbuf, BUS_DMA_WAITOK);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamem_map failed\n");
		return;
	}

	error = bus_dmamap_create(sc->sc_dmat, MAXPHYS, 1, MAXPHYS, 0,
	    BUS_DMA_WAITOK, &sc->sc_dmamap);
	if (error) {
		device_printf(sc->sc_dev, "bus_dmamap_create failed\n");
		return;
	}
}

static int
amlogic_sdio_set_clock(struct amlogic_sdio_softc *sc, u_int freq)
{
	const u_int pll_freq = sc->sc_bus_freq / 2000;
	uint32_t conf;
	int clk_div;

	if (freq == 0)
		return 0;

	clk_div = howmany(pll_freq, freq);

	conf = SDIO_READ(sc, SDIO_CONF_REG);
	conf &= ~SDIO_CONF_COMMAND_CLK_DIV;
	conf |= __SHIFTIN(clk_div - 1, SDIO_CONF_COMMAND_CLK_DIV);
	SDIO_WRITE(sc, SDIO_CONF_REG, conf);

	return 0;
}

static int
amlogic_sdio_wait_irqs(struct amlogic_sdio_softc *sc, uint32_t mask, int timeout)
{
	int retry, error;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (sc->sc_intr_irqs & mask)
		return 0;

	retry = timeout / hz;

	while (retry > 0) {
		error = cv_timedwait(&sc->sc_intr_cv, &sc->sc_intr_lock, hz);
		if (error && error != EWOULDBLOCK)
			return error;
		if (sc->sc_intr_irqs & mask)
			return 0;
		--retry;
	}

	return ETIMEDOUT;
}

static int
amlogic_sdio_host_reset(sdmmc_chipset_handle_t sch)
{
	struct amlogic_sdio_softc *sc = sch;

	SDIO_WRITE(sc, SDIO_IRQC_REG, SDIO_IRQC_SOFT_RESET);

	delay(2);

	SDIO_WRITE(sc, SDIO_IRQS_REG, SDIO_IRQS_CLEAR);
	SDIO_WRITE(sc, SDIO_CONF_REG,
	    __SHIFTIN(2, SDIO_CONF_WRITE_CRC_OK_STATUS) |
	    __SHIFTIN(2, SDIO_CONF_WRITE_NWR) |
	    __SHIFTIN(3, SDIO_CONF_M_ENDIAN) |
	    __SHIFTIN(39, SDIO_CONF_COMMAND_ARG_BITS) |
	    __SHIFTIN(0x1f4, SDIO_CONF_COMMAND_CLK_DIV));

	SDIO_WRITE(sc, SDIO_MULT_REG,
	    __SHIFTIN(sc->sc_cur_port, SDIO_MULT_PORT_SEL));

	return 0;
}

static uint32_t
amlogic_sdio_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

static int
amlogic_sdio_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 512;
}

static int
amlogic_sdio_card_detect(sdmmc_chipset_handle_t sch)
{
	struct amlogic_sdio_softc *sc = sch;

	return amlogic_sdhc_is_card_present(sc->sc_cur_port);
}

static int
amlogic_sdio_write_protect(sdmmc_chipset_handle_t sch)
{
	return 0;
}

static int
amlogic_sdio_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
amlogic_sdio_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct amlogic_sdio_softc *sc = sch;

	return amlogic_sdio_set_clock(sc, freq);
}

static int
amlogic_sdio_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct amlogic_sdio_softc *sc = sch;
	uint32_t conf;

	conf = SDIO_READ(sc, SDIO_CONF_REG);
	if (width == 1) {
		conf &= ~SDIO_CONF_BUS_WIDTH;
	} else if (width == 4) {
		conf |= SDIO_CONF_BUS_WIDTH;
	} else {
		return EINVAL;
	}
	SDIO_WRITE(sc, SDIO_CONF_REG, conf);

	sc->sc_cur_width = width;

	return 0;
}

static int
amlogic_sdio_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return ENOTSUP;
}

static void
amlogic_sdio_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct amlogic_sdio_softc *sc = sch;
	uint32_t send, ext, mult, addr;
	bool use_bbuf = false;
	int i;

	KASSERT(cmd->c_blklen <= 512);

	send = ext = mult = addr = 0;

	mutex_enter(&sc->sc_intr_lock);

	if (cmd->c_opcode == SD_IO_SEND_OP_COND ||
	    cmd->c_opcode == SD_IO_RW_DIRECT ||
	    cmd->c_opcode == SD_IO_RW_EXTENDED) {
		cmd->c_error = EINVAL;
		goto done;
	}

	sc->sc_intr_irqs = 0;

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136) {
			send |= __SHIFTIN(133, SDIO_SEND_RESPONSE_BITS);
			send |= SDIO_SEND_RESPONSE_CRC7_FROM_8;
		} else {
			send |= __SHIFTIN(45, SDIO_SEND_RESPONSE_BITS);
		}
	}
	if ((cmd->c_flags & SCF_RSP_CRC) == 0) {
		send |= SDIO_SEND_RESPONSE_NO_CRC;
	}
	if (cmd->c_flags & SCF_RSP_BSY) {
		send |= SDIO_SEND_CHECK_BUSY_DAT0;
	}

	if (cmd->c_datalen > 0) {
		unsigned int nblks, packlen;

		nblks = cmd->c_datalen / cmd->c_blklen;
		if (nblks == 0 || (cmd->c_datalen % cmd->c_blklen) != 0)
			++nblks;
		packlen = (cmd->c_blklen * 8) + (0xf * sc->sc_cur_width);

		send |= __SHIFTIN(nblks - 1, SDIO_SEND_REPEAT_PACKAGE);
		ext |= __SHIFTIN(packlen, SDIO_EXT_DATA_RW_NUMBER);

		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			send |= SDIO_SEND_RESPONSE_DATA;
		} else {
			send |= SDIO_SEND_COMMAND_HAS_DATA;
		}

		cmd->c_error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
		    sc->sc_bbuf, MAXPHYS, NULL, BUS_DMA_WAITOK);
		if (cmd->c_error) {
			device_printf(sc->sc_dev, "bus_dmamap_load failed\n");
			goto done;
		}
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_PREREAD);
		} else {
			memcpy(sc->sc_bbuf, cmd->c_data, cmd->c_datalen);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_PREWRITE);
		}
		addr = sc->sc_dmamap->dm_segs[0].ds_addr;
		use_bbuf = true;
	}
	send |= __SHIFTIN(cmd->c_opcode | 0x40, SDIO_SEND_COMMAND_INDEX);

	mult |= __SHIFTIN(sc->sc_cur_port, SDIO_MULT_PORT_SEL);

	SDIO_WRITE(sc, SDIO_IRQC_REG, SDIO_IRQC_SOFT_RESET);
	delay(2);

	SDIO_WRITE(sc, SDIO_IRQC_REG, SDIO_IRQC_ARC_CMD_INTEN);
	SDIO_WRITE(sc, SDIO_IRQS_REG, SDIO_IRQS_CLEAR);

	SDIO_WRITE(sc, SDIO_ARGU_REG, cmd->c_arg);
	SDIO_WRITE(sc, SDIO_MULT_REG, mult);
	SDIO_WRITE(sc, SDIO_EXT_REG, ext);
	SDIO_WRITE(sc, SDIO_ADDR_REG, addr);
	SDIO_WRITE(sc, SDIO_SEND_REG, send);

	cmd->c_error = amlogic_sdio_wait_irqs(sc, SDIO_IRQS_CMD_INT, hz * 3);
	if (cmd->c_error) {
		goto done;
	}

	if (SDIO_READ(sc, SDIO_IRQS_REG) & SDIO_IRQS_CMD_BUSY) {
		int retry;
		for (retry = 10000; retry > 0; retry--) {
			const uint32_t irqs = SDIO_READ(sc, SDIO_IRQS_REG);
			if ((irqs & SDIO_IRQS_CMD_BUSY) == 0)
				break;
			delay(100);
		}
		if (retry == 0) {
			aprint_debug_dev(sc->sc_dev,
			    "busy timeout, opcode %d flags %#x datalen %d\n",
			    cmd->c_opcode, cmd->c_flags, cmd->c_datalen);
			cmd->c_error = ETIMEDOUT;
			goto done;
		}
	}

	const uint32_t irqs = SDIO_READ(sc, SDIO_IRQS_REG);
	if (cmd->c_flags & SCF_RSP_CRC) {
		if ((irqs & SDIO_IRQS_RESPONSE_CRC7_OK) == 0) {
			device_printf(sc->sc_dev, "response crc error\n");
			cmd->c_error = EIO;
			goto done;
		}
	}
	if (cmd->c_datalen > 0) {
		uint32_t crcmask = SDIO_IRQS_DATA_READ_CRC16_OK|
				   SDIO_IRQS_DATA_WRITE_CRC16_OK;
		if ((irqs & crcmask) == 0) {
			device_printf(sc->sc_dev, "data crc error\n");
			cmd->c_error = EIO;
			goto done;
		}
	}

	if (cmd->c_flags & SCF_RSP_PRESENT) {
		mult |= SDIO_MULT_WRITE_READ_OUT_INDEX;
		mult &= ~SDIO_MULT_RESPONSE_READ_INDEX;
		SDIO_WRITE(sc, SDIO_MULT_REG, mult);

		if (cmd->c_flags & SCF_RSP_136) {
			for (i = 0; i < 4; i++) {
				cmd->c_resp[i] = SDIO_READ(sc, SDIO_ARGU_REG);
			}
		} else {
			cmd->c_resp[0] = SDIO_READ(sc, SDIO_ARGU_REG);
		}
	}

done:
	if (use_bbuf) {
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_POSTREAD);
		} else {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmamap, 0,
			    MAXPHYS, BUS_DMASYNC_POSTWRITE);
		}
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmamap);
		if (ISSET(cmd->c_flags, SCF_CMD_READ)) {
			memcpy(cmd->c_data, sc->sc_bbuf, cmd->c_datalen);
		}
	}
	cmd->c_flags |= SCF_ITSDONE;

	SDIO_WRITE(sc, SDIO_IRQC_REG, 0);
	SDIO_WRITE(sc, SDIO_IRQS_REG, SDIO_IRQS_CLEAR);

	mutex_exit(&sc->sc_intr_lock);
}

static void
amlogic_sdio_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct amlogic_sdio_softc *sc = sch;
	uint32_t irqc;

	mutex_enter(&sc->sc_intr_lock);
	irqc = SDIO_READ(sc, SDIO_IRQC_REG);
	if (enable) {
		irqc |= SDIO_IRQC_ARC_IF_INTEN;
	} else {
		irqc &= ~SDIO_IRQC_ARC_IF_INTEN;
	}
	SDIO_WRITE(sc, SDIO_IRQC_REG, irqc);
	mutex_exit(&sc->sc_intr_lock);
}

static void
amlogic_sdio_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct amlogic_sdio_softc *sc = sch;

	mutex_enter(&sc->sc_intr_lock);
	SDIO_WRITE(sc, SDIO_IRQS_REG, SDIO_IRQS_IF_INT);
	mutex_exit(&sc->sc_intr_lock);
}
