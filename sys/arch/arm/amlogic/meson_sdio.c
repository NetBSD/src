/* $NetBSD: meson_sdio.c,v 1.5 2021/08/07 16:18:43 thorpej Exp $ */

/*-
 * Copyright (c) 2015-2019 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: meson_sdio.c,v 1.5 2021/08/07 16:18:43 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/gpio.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmc_ioreg.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_sdioreg.h>

static int	meson_sdio_match(device_t, cfdata_t, void *);
static void	meson_sdio_attach(device_t, device_t, void *);
static void	meson_sdio_attach_i(device_t);

static int	meson_sdio_intr(void *);

struct meson_sdio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	bus_dma_tag_t		sc_dmat;
	void			*sc_ih;

	int			sc_slot_phandle;

	uint32_t		sc_bus_freq;
	u_int			sc_cur_width;
	int			sc_cur_port;

	struct fdtbus_gpio_pin	*sc_gpio_cd;
	int			sc_gpio_cd_inverted;
	struct fdtbus_gpio_pin	*sc_gpio_wp;
	int			sc_gpio_wp_inverted;

	struct fdtbus_regulator	*sc_reg_vmmc;
	struct fdtbus_regulator	*sc_reg_vqmmc;

	bool			sc_non_removable;
	bool			sc_broken_cd;

	device_t		sc_sdmmc_dev;
	kmutex_t		sc_intr_lock;
	kcondvar_t		sc_intr_cv;

	uint32_t		sc_intr_irqs;

	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_segs[1];
	void			*sc_bbuf;
};

CFATTACH_DECL_NEW(meson_sdio, sizeof(struct meson_sdio_softc),
	meson_sdio_match, meson_sdio_attach, NULL, NULL);

static int	meson_sdio_host_reset(sdmmc_chipset_handle_t);
static uint32_t	meson_sdio_host_ocr(sdmmc_chipset_handle_t);
static int	meson_sdio_host_maxblklen(sdmmc_chipset_handle_t);
static int	meson_sdio_card_detect(sdmmc_chipset_handle_t);
static int	meson_sdio_write_protect(sdmmc_chipset_handle_t);
static int	meson_sdio_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int	meson_sdio_bus_clock(sdmmc_chipset_handle_t, int);
static int	meson_sdio_bus_width(sdmmc_chipset_handle_t, int);
static int	meson_sdio_bus_rod(sdmmc_chipset_handle_t, int);
static int	meson_sdio_signal_voltage(sdmmc_chipset_handle_t, int);
static void	meson_sdio_exec_command(sdmmc_chipset_handle_t,
				     struct sdmmc_command *);
static void	meson_sdio_card_enable_intr(sdmmc_chipset_handle_t, int);
static void	meson_sdio_card_intr_ack(sdmmc_chipset_handle_t);

static int	meson_sdio_set_clock(struct meson_sdio_softc *, u_int);
static int	meson_sdio_wait_irqs(struct meson_sdio_softc *, uint32_t, int);

static void	meson_sdio_dmainit(struct meson_sdio_softc *);

static struct sdmmc_chip_functions meson_sdio_chip_functions = {
	.host_reset = meson_sdio_host_reset,
	.host_ocr = meson_sdio_host_ocr,
	.host_maxblklen = meson_sdio_host_maxblklen,
	.card_detect = meson_sdio_card_detect,
	.write_protect = meson_sdio_write_protect,
	.bus_power = meson_sdio_bus_power,
	.bus_clock = meson_sdio_bus_clock,
	.bus_width = meson_sdio_bus_width,
	.bus_rod = meson_sdio_bus_rod,
	.signal_voltage = meson_sdio_signal_voltage,
	.exec_command = meson_sdio_exec_command,
	.card_enable_intr = meson_sdio_card_enable_intr,
	.card_intr_ack = meson_sdio_card_intr_ack,
};

#define SDIO_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define SDIO_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson8b-sdio" },
	DEVICE_COMPAT_EOL
};

static const struct device_compatible_entry slot_compat_data[] = {
	{ .compat = "mmc-slot" },
	DEVICE_COMPAT_EOL
};

static int
meson_sdio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_sdio_attach(device_t parent, device_t self, void *aux)
{
	struct meson_sdio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	struct clk *clk_clkin, *clk_core;
	bus_addr_t addr, port;
	bus_size_t size;
	int child;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	clk_core = fdtbus_clock_get(phandle, "core");
	if (clk_core == NULL || clk_enable(clk_core) != 0) {
		aprint_error(": failed to enable core clock\n");
		return;
	}

	clk_clkin = fdtbus_clock_get(phandle, "clkin");
	if (clk_clkin == NULL) {
		aprint_error(": failed to get clkin clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": failed to map registers\n");
		return;
	}

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_BIO);
	cv_init(&sc->sc_intr_cv, "sdiointr");

	sc->sc_cur_port = -1;
	for (child = OF_child(phandle); child; child = OF_peer(child))
		if (of_compatible_match(child, slot_compat_data)) {
			if (fdtbus_get_reg(child, 0, &port, NULL) == 0) {
				sc->sc_slot_phandle = child;
				sc->sc_cur_port = port;
			}
			break;
		}
	if (sc->sc_cur_port == -1) {
		aprint_error(": couldn't get mmc slot\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": SDIO controller (port %c)\n", sc->sc_cur_port + 'A');

	sc->sc_reg_vmmc = fdtbus_regulator_acquire(sc->sc_slot_phandle, "vmmc-supply");
	sc->sc_reg_vqmmc = fdtbus_regulator_acquire(sc->sc_slot_phandle, "vqmmc-supply");

	sc->sc_gpio_cd = fdtbus_gpio_acquire(sc->sc_slot_phandle, "cd-gpios",
	    GPIO_PIN_INPUT);
	sc->sc_gpio_wp = fdtbus_gpio_acquire(sc->sc_slot_phandle, "wp-gpios",
	    GPIO_PIN_INPUT);

	sc->sc_gpio_cd_inverted = of_hasprop(sc->sc_slot_phandle, "cd-inverted");
	sc->sc_gpio_wp_inverted = of_hasprop(sc->sc_slot_phandle, "wp-inverted");

	sc->sc_non_removable = of_hasprop(sc->sc_slot_phandle, "non-removable");
	sc->sc_broken_cd = of_hasprop(sc->sc_slot_phandle, "broken-cd");

	sc->sc_ih = fdtbus_intr_establish_xname(phandle, 0, IPL_BIO, 0,
	    meson_sdio_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_bus_freq = clk_get_rate(clk_clkin);

	aprint_normal_dev(self, "core %u Hz, clkin %u Hz\n", clk_get_rate(clk_core), clk_get_rate(clk_clkin));

	meson_sdio_dmainit(sc);

	config_interrupts(self, meson_sdio_attach_i);
}

static void
meson_sdio_attach_i(device_t self)
{
	struct meson_sdio_softc *sc = device_private(self);
	struct sdmmcbus_attach_args saa;

	meson_sdio_signal_voltage(sc, SDMMC_SIGNAL_VOLTAGE_330);
	meson_sdio_host_reset(sc);
	meson_sdio_bus_clock(sc, 400);
	meson_sdio_bus_width(sc, 1);

	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.saa_sct = &meson_sdio_chip_functions;
	saa.saa_dmat = sc->sc_dmat;
	saa.saa_sch = sc;
	saa.saa_clkmin = 400;
	saa.saa_clkmax = sc->sc_bus_freq;
	/* Do not advertise DMA capabilities, we handle DMA ourselves */
	saa.saa_caps = SMC_CAPS_4BIT_MODE|
		       SMC_CAPS_SD_HIGHSPEED|
		       SMC_CAPS_MMC_HIGHSPEED;

	sc->sc_sdmmc_dev = config_found(self, &saa, NULL, CFARGS_NONE);
}

static int
meson_sdio_intr(void *priv)
{
	struct meson_sdio_softc *sc = priv;

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
meson_sdio_dmainit(struct meson_sdio_softc *sc)
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
meson_sdio_set_clock(struct meson_sdio_softc *sc, u_int freq)
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
meson_sdio_wait_irqs(struct meson_sdio_softc *sc, uint32_t mask, int timeout)
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
meson_sdio_host_reset(sdmmc_chipset_handle_t sch)
{
	struct meson_sdio_softc *sc = sch;

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
meson_sdio_host_ocr(sdmmc_chipset_handle_t sch)
{
	return MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V;
}

static int
meson_sdio_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	return 512;
}

static int
meson_sdio_card_detect(sdmmc_chipset_handle_t sch)
{
	struct meson_sdio_softc *sc = sch;
	int val;

	if (sc->sc_non_removable || sc->sc_broken_cd) {
		return 1;
	} else if (sc->sc_gpio_cd != NULL) {
		val = fdtbus_gpio_read(sc->sc_gpio_cd);
		if (sc->sc_gpio_cd_inverted)
			val = !val;
		return val;
	} else {
		return 1;
	}
}

static int
meson_sdio_write_protect(sdmmc_chipset_handle_t sch)
{
	struct meson_sdio_softc *sc = sch;
	int val;

	if (sc->sc_gpio_wp != NULL) {
		val = fdtbus_gpio_read(sc->sc_gpio_wp);
		if (sc->sc_gpio_wp_inverted)
			val = !val;
		return val;
	}

	return 0;
}

static int
meson_sdio_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	return 0;
}

static int
meson_sdio_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct meson_sdio_softc *sc = sch;

	return meson_sdio_set_clock(sc, freq);
}

static int
meson_sdio_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct meson_sdio_softc *sc = sch;
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
meson_sdio_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	return ENOTSUP;
}

static int
meson_sdio_signal_voltage(sdmmc_chipset_handle_t sch, int signal_voltage)
{
	struct meson_sdio_softc *sc = sch;
	u_int uvol;
	int error;

	if (sc->sc_reg_vqmmc == NULL)
		return 0;

	switch (signal_voltage) {
	case SDMMC_SIGNAL_VOLTAGE_330:
		uvol = 3300000;
		break;
	case SDMMC_SIGNAL_VOLTAGE_180:
		uvol = 1800000;
		break;
	default:
		return EINVAL;
	}

	error = fdtbus_regulator_supports_voltage(sc->sc_reg_vqmmc, uvol, uvol);
	if (error != 0)
		return 0;

	error = fdtbus_regulator_set_voltage(sc->sc_reg_vqmmc, uvol, uvol);
	if (error != 0)
		return error;

	return fdtbus_regulator_enable(sc->sc_reg_vqmmc);
}

static void
meson_sdio_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct meson_sdio_softc *sc = sch;
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

	cmd->c_error = meson_sdio_wait_irqs(sc, SDIO_IRQS_CMD_INT, hz * 3);
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
meson_sdio_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct meson_sdio_softc *sc = sch;
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
meson_sdio_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct meson_sdio_softc *sc = sch;

	mutex_enter(&sc->sc_intr_lock);
	SDIO_WRITE(sc, SDIO_IRQS_REG, SDIO_IRQS_IF_INT);
	mutex_exit(&sc->sc_intr_lock);
}
