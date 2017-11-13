/* $NetBSD: sunxi_nand.c,v 1.3 2017/11/13 14:14:25 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_nand.c,v 1.3 2017/11/13 14:14:25 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>

#define	NDFC_CTL		0x00
#define	 NDFC_CTL_CE_SEL		__BITS(27,24)
#define	 NDFC_CTL_PAGE_SIZE		__BITS(11,8)
#define	 NDFC_CTL_RB_SEL		__BITS(4,3)
#define	 NDFC_CTL_BUS_WIDTH		__BIT(2)
#define	 NDFC_CTL_RESET			__BIT(1)
#define	 NDFC_CTL_EN			__BIT(0)
#define	NDFC_ST			0x04
#define	 NDFC_ST_RB_STATE(n)		__BIT(8 + (n))
#define	 NDFC_ST_CMD_FIFO_STATUS	__BIT(3)
#define	 NDFC_ST_CMD_INT_FLAG		__BIT(1)
#define	NDFC_INT		0x08
#define	NDFC_TIMING_CTL		0x0c
#define	NDFC_TIMING_CFG		0x10
#define	NDFC_ADDR_LOW		0x14
#define	NDFC_ADDR_HIGH		0x18
#define	NDFC_BLOCK_NUM		0x1c
#define	NDFC_CNT		0x20
#define	 NDFC_CNT_DATA_CNT		__BITS(9,0)
#define	NDFC_CMD		0x24
#define	 NDFC_CMD_DATA_METHOD		__BIT(26)
#define	 NDFC_CMD_SEND_FIRST_CMD	__BIT(22)
#define	 NDFC_CMD_DATA_TRANS		__BIT(21)
#define	 NDFC_CMD_ACCESS_DIR		__BIT(20)
#define	 NDFC_CMD_SEND_ADR		__BIT(19)
#define	 NDFC_CMD_ADR_NUM		__BITS(18,16)
#define	NDFC_RCMD_SET		0x28
#define	NDFC_WCMD_SET		0x2c
#define	NDFC_IO_DATA		0x30
#define	NDFC_ECC_CTL		0x34
#define	NDFC_ECC_ST		0x38
#define	NDFC_EFR		0x3c
#define	NDFC_ERR_CNT0		0x40
#define	NDFC_ERR_CNT1		0x44
#define	NDFC_USER_DATA(n)	(0x50 + 4 * (n))
#define	NDFC_EFNAND_STA		0x90
#define	NDFC_SPARE_AREA		0xa0
#define	NDFC_PAT_ID		0xa4
#define	NDFC_RDATA_STA_CTL	0xa8
#define	NDFC_RDATA_STA_0	0xac
#define	NDFC_RDATA_STA_1	0xb0
#define	NDFC_MDMA_ADDR		0xc0
#define	NDFC_MDMA_CNT		0xc4
#define	NDFC_RAM0_BASE		0x400
#define	NDFC_RAM1_BASE		0x800

#define	NDFC_RAM_SIZE		1024

static const char * compatible[] = {
	"allwinner,sun4i-a10-nand",
	NULL
};

struct sunxi_nand_softc;

enum sunxi_nand_eccmode {
	ECC_MODE_UNKNOWN,
	ECC_MODE_HW,
	ECC_MODE_HW_SYNDROME,
	ECC_MODE_SOFT,
	ECC_MODE_SOFT_BCH,
	ECC_MODE_NONE
};

struct sunxi_nand_softc;

struct sunxi_nand_chip {
	struct sunxi_nand_softc		*chip_sc;
	int				chip_phandle;
	device_t			chip_dev;

	u_int				chip_cs;
	enum sunxi_nand_eccmode		chip_eccmode;
	u_int				chip_rb;
	struct fdtbus_gpio_pin		*chip_rb_pin;

	struct nand_interface		chip_nand;

	bool				chip_addr_pending;
	u_int				chip_addr_count;
	uint32_t			chip_addr[2];
};

struct sunxi_nand_softc {
	device_t			sc_dev;
	int				sc_phandle;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;

	struct clk			*sc_clk_mod;
	struct clk			*sc_clk_ahb;
	struct fdtbus_reset		*sc_rst_ahb;

	struct sunxi_nand_chip		sc_chip;
};

#define	NAND_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define NAND_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
sunxi_nand_rb_state(struct sunxi_nand_softc *sc, struct sunxi_nand_chip *chip)
{
	if (chip->chip_rb_pin != NULL)
		return fdtbus_gpio_read(chip->chip_rb_pin);

	const uint32_t status = NAND_READ(sc, NDFC_ST);
	return __SHIFTOUT(status, NDFC_ST_RB_STATE(chip->chip_rb));
}

static int
sunxi_nand_wait_status(struct sunxi_nand_softc *sc, uint32_t mask, uint32_t val)
{
	uint32_t status;
	int retry;

	for (retry = 1000000; retry > 0; retry--) {
		status = NAND_READ(sc, NDFC_ST);
		if ((status & mask) == val)
			break;
		delay(1);
	}
	if (retry == 0) {
#ifdef SUNXI_NAND_DEBUG
		device_printf(sc->sc_dev,
		    "device timeout; status=%x mask=%x val=%x\n",
		    status, mask, val);
#endif
		return ETIMEDOUT;
	}

	if (mask == NDFC_ST_CMD_INT_FLAG)
		NAND_WRITE(sc, NDFC_ST, NDFC_ST_CMD_INT_FLAG);

	return 0;
}

static void
sunxi_nand_send_address(struct sunxi_nand_softc *sc,
    struct sunxi_nand_chip *chip)
{
	if (chip->chip_addr_count == 0)
		return;

	/* Wait for space in the command FIFO */
	sunxi_nand_wait_status(sc, NDFC_ST_CMD_FIFO_STATUS, 0);

	NAND_WRITE(sc, NDFC_ADDR_LOW, chip->chip_addr[0]);
	NAND_WRITE(sc, NDFC_ADDR_HIGH, chip->chip_addr[1]);
	NAND_WRITE(sc, NDFC_CMD,
	    NDFC_CMD_SEND_ADR |
	    __SHIFTIN(chip->chip_addr_count - 1, NDFC_CMD_ADR_NUM));

	/* Wait for the command to finish */
	sunxi_nand_wait_status(sc, NDFC_ST_CMD_INT_FLAG, NDFC_ST_CMD_INT_FLAG);

	chip->chip_addr[0] = 0;
	chip->chip_addr[1] = 0;
	chip->chip_addr_count = 0;
}

static int
sunxi_nand_read_buf_n(device_t dev, void *data, size_t len, int n)
{
	struct sunxi_nand_softc * const sc = device_private(dev);
	struct sunxi_nand_chip * const chip = &sc->sc_chip;
	uint8_t *xfer_buf = data;
	size_t resid = len;
	int error;

	KASSERT(n == 1 || n == 2);
	KASSERT(len % n == 0);

	sunxi_nand_send_address(sc, chip);

	while (resid > 0) {
		const size_t xfer = min(resid, NDFC_RAM_SIZE);

		/* Wait for space in the command FIFO */
		error = sunxi_nand_wait_status(sc, NDFC_ST_CMD_FIFO_STATUS, 0);
		if (error != 0)
			return error;

		/* Start the transfer. */
		NAND_WRITE(sc, NDFC_CNT, xfer);
		NAND_WRITE(sc, NDFC_CMD,
		    NDFC_CMD_DATA_TRANS | NDFC_CMD_DATA_METHOD);

		/* Wait for the command to finish */
		error = sunxi_nand_wait_status(sc, NDFC_ST_CMD_INT_FLAG,
		    NDFC_ST_CMD_INT_FLAG);
		if (error != 0)
			return error;

		/* Transfer data from FIFO RAM to buffer */
		const int count = xfer / n;
		if (n == 1) {
			bus_space_read_region_1(sc->sc_bst, sc->sc_bsh,
			    NDFC_RAM0_BASE, (uint8_t *)xfer_buf, count);
		} else if (n == 2) {
			bus_space_read_region_2(sc->sc_bst, sc->sc_bsh,
			    NDFC_RAM0_BASE, (uint16_t *)xfer_buf, count);
		}

		resid -= xfer;
		xfer_buf += xfer;
	}

	return 0;
}

static int
sunxi_nand_write_buf_n(device_t dev, const void *data, size_t len, int n)
{
	struct sunxi_nand_softc * const sc = device_private(dev);
	struct sunxi_nand_chip * const chip = &sc->sc_chip;
	const uint8_t *xfer_buf = data;
	size_t resid = len;
	int error;

	KASSERT(n == 1 || n == 2);
	KASSERT(len % n == 0);

	sunxi_nand_send_address(sc, chip);

	while (resid > 0) {
		const size_t xfer = min(resid, NDFC_RAM_SIZE);

		/* Wait for space in the command FIFO */
		error = sunxi_nand_wait_status(sc, NDFC_ST_CMD_FIFO_STATUS, 0);
		if (error != 0)
			return error;

		/* Transfer data from buffer to FIFO RAM */
		const int count = xfer / n;
		if (n == 1) {
			bus_space_write_region_1(sc->sc_bst, sc->sc_bsh,
			    NDFC_RAM0_BASE, (const uint8_t *)xfer_buf, count);
		} else if (n == 2) {
			bus_space_write_region_2(sc->sc_bst, sc->sc_bsh,
			    NDFC_RAM0_BASE, (const uint16_t *)xfer_buf, count);
		}

		/* Start the transfer. */
		NAND_WRITE(sc, NDFC_CNT, xfer);
		NAND_WRITE(sc, NDFC_CMD,
		    NDFC_CMD_DATA_TRANS | NDFC_CMD_DATA_METHOD |
		    NDFC_CMD_ACCESS_DIR);

		/* Wait for the command to finish */
		error = sunxi_nand_wait_status(sc, NDFC_ST_CMD_INT_FLAG,
		    NDFC_ST_CMD_INT_FLAG);
		if (error != 0)
			return error;

		resid -= xfer;
		xfer_buf += xfer;
	}

	return 0;
}

/*
 * NAND interface
 */

static void
sunxi_nand_select(device_t dev, bool enable)
{
	struct sunxi_nand_softc * const sc = device_private(dev);
	struct sunxi_nand_chip * const chip = &sc->sc_chip;
	struct nand_softc * const nand_sc = device_private(chip->chip_dev);
	struct nand_chip * const nc = nand_sc ? &nand_sc->sc_chip : NULL;
	uint32_t ctl;

	ctl = NAND_READ(sc, NDFC_CTL);
	if (enable) {
		ctl &= ~NDFC_CTL_CE_SEL;
		ctl |= __SHIFTIN(chip->chip_cs, NDFC_CTL_CE_SEL);
		ctl &= ~NDFC_CTL_RB_SEL;
		ctl |= __SHIFTIN(chip->chip_rb, NDFC_CTL_RB_SEL);
		ctl &= ~NDFC_CTL_PAGE_SIZE;
		ctl &= ~NDFC_CTL_BUS_WIDTH;
		ctl |= NDFC_CTL_EN;

		if (nc) {
			ctl |= __SHIFTIN(__builtin_ffs(nc->nc_page_size) - 11,
					 NDFC_CTL_PAGE_SIZE);
			if (ISSET(nc->nc_flags, NC_BUSWIDTH_16))
				ctl |= NDFC_CTL_BUS_WIDTH;

			NAND_WRITE(sc, NDFC_SPARE_AREA, nc->nc_page_size);
		}
	}
	NAND_WRITE(sc, NDFC_CTL, ctl);
}

static void
sunxi_nand_command(device_t dev, uint8_t command)
{
	struct sunxi_nand_softc * const sc = device_private(dev);
	struct sunxi_nand_chip * const chip = &sc->sc_chip;

	sunxi_nand_send_address(sc, chip);

	/* Wait for space in the command FIFO */
	sunxi_nand_wait_status(sc, NDFC_ST_CMD_FIFO_STATUS, 0);

	NAND_WRITE(sc, NDFC_CMD, NDFC_CMD_SEND_FIRST_CMD | command);

	/* Wait for the command to finish */
	sunxi_nand_wait_status(sc, NDFC_ST_CMD_INT_FLAG, NDFC_ST_CMD_INT_FLAG);
}

static void
sunxi_nand_address(device_t dev, uint8_t address)
{
	struct sunxi_nand_softc * const sc = device_private(dev);
	struct sunxi_nand_chip * const chip = &sc->sc_chip;
	const u_int index = chip->chip_addr_count / 4;
	const u_int shift = (chip->chip_addr_count & 3) * 8;

	KASSERT(index < 2);

	chip->chip_addr[index] |= ((uint32_t)address << shift);
	chip->chip_addr_count++;
}

static void
sunxi_nand_busy(device_t dev)
{
	struct sunxi_nand_softc * const sc = device_private(dev);
	struct sunxi_nand_chip * const chip = &sc->sc_chip;

	while (!sunxi_nand_rb_state(sc, chip))
		delay(1);
}

static void
sunxi_nand_read_1(device_t dev, uint8_t *data)
{
	sunxi_nand_read_buf_n(dev, data, 1, 1);
}

static void
sunxi_nand_read_2(device_t dev, uint16_t *data)
{
	sunxi_nand_read_buf_n(dev, data, 2, 2);
}

static void
sunxi_nand_read_buf_1(device_t dev, void *data, size_t len)
{
	sunxi_nand_read_buf_n(dev, data, len, 1);
}

static void
sunxi_nand_read_buf_2(device_t dev, void *data, size_t len)
{
	sunxi_nand_read_buf_n(dev, data, len, 2);
}

static void
sunxi_nand_write_1(device_t dev, uint8_t data)
{
	sunxi_nand_write_buf_n(dev, &data, 1, 1);
}

static void
sunxi_nand_write_2(device_t dev, uint16_t data)
{
	sunxi_nand_write_buf_n(dev, &data, 2, 2);
}

static void
sunxi_nand_write_buf_1(device_t dev, const void *data, size_t len)
{
	sunxi_nand_write_buf_n(dev, data, len, 1);
}

static void
sunxi_nand_write_buf_2(device_t dev, const void *data, size_t len)
{
	sunxi_nand_write_buf_n(dev, data, len, 2);
}

static void
sunxi_nand_attach_chip(struct sunxi_nand_softc *sc,
    struct sunxi_nand_chip *chip, int phandle)
{
	struct nand_interface *nand = &chip->chip_nand;
	const char *ecc_mode;

	chip->chip_sc = sc;
	chip->chip_phandle = phandle;

	if (of_getprop_uint32(phandle, "reg", &chip->chip_cs) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "missing 'reg' property on chip node\n");
		return;
	}

	if (of_getprop_uint32(phandle, "allwinner,rb", &chip->chip_rb) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "chip #%u: missing 'allwinner,rb' property\n",
		    chip->chip_cs);
		return;
	}

	ecc_mode = fdtbus_get_string(phandle, "nand-ecc-mode");
	if (ecc_mode == NULL)
		ecc_mode = "none";

	if (strcmp(ecc_mode, "none") == 0)
		chip->chip_eccmode = ECC_MODE_NONE;
	else if (strcmp(ecc_mode, "hw") == 0)
		chip->chip_eccmode = ECC_MODE_HW;
	else if (strcmp(ecc_mode, "hw_syndrome") == 0)
		chip->chip_eccmode = ECC_MODE_HW_SYNDROME;
	else if (strcmp(ecc_mode, "soft") == 0)
		chip->chip_eccmode = ECC_MODE_SOFT;
	else if (strcmp(ecc_mode, "soft_bch") == 0)
		chip->chip_eccmode = ECC_MODE_SOFT_BCH;
	else
		chip->chip_eccmode = ECC_MODE_UNKNOWN;

	/* Only HW mode is supported for now */
	switch (chip->chip_eccmode) {
	case ECC_MODE_HW:
		break;
	default:
		return;
	}

	aprint_normal_dev(sc->sc_dev, "chip #%u: RB %u, ECC mode '%s'\n",
	    chip->chip_cs, chip->chip_rb, ecc_mode);

	nand_init_interface(nand);
	nand->select = sunxi_nand_select;
	nand->command = sunxi_nand_command;
	nand->address = sunxi_nand_address;
	nand->read_buf_1 = sunxi_nand_read_buf_1;
	nand->read_buf_2 = sunxi_nand_read_buf_2;
	nand->read_1 = sunxi_nand_read_1;
	nand->read_2 = sunxi_nand_read_2;
	nand->write_buf_1 = sunxi_nand_write_buf_1;
	nand->write_buf_2 = sunxi_nand_write_buf_2;
	nand->write_1 = sunxi_nand_write_1;
	nand->write_2 = sunxi_nand_write_2;
	nand->busy = sunxi_nand_busy;

#if notyet
	switch (chip->chip_eccmode) {
	case ECC_MODE_HW:
		nand->ecc_compute = sunxi_nand_ecc_compute;
		nand->ecc_correct = sunxi_nand_ecc_correct;
		nand->ecc_prepare = sunxi_nand_ecc_prepare;
		nand->ecc.necc_code_size = 3;
		nand->ecc.necc_block_size = 512;
		nand->ecc.necc_type = NAND_ECC_TYPE_HW;
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "chip #%u: ECC mode not supported by driver\n",
		    chip->chip_cs);
		return;
	}
#else
	nand->ecc.necc_code_size = 3;
	nand->ecc.necc_block_size = 256;
#endif

	chip->chip_dev = nand_attach_mi(nand, sc->sc_dev);
}

static int
sunxi_nand_init_resources(struct sunxi_nand_softc *sc)
{
	int error;

	/* Both "mod" and "ahb" clocks are required */
	sc->sc_clk_mod = fdtbus_clock_get(sc->sc_phandle, "mod");
	sc->sc_clk_ahb = fdtbus_clock_get(sc->sc_phandle, "ahb");
	if (sc->sc_clk_mod == NULL || sc->sc_clk_ahb == NULL)
		return ENXIO;

	if ((error = clk_enable(sc->sc_clk_ahb)) != 0)
		return error;
	if ((error = clk_enable(sc->sc_clk_mod)) != 0)
		return error;

	/* Reset is optional */
	sc->sc_rst_ahb = fdtbus_reset_get(sc->sc_phandle, "ahb");
	if (sc->sc_rst_ahb != NULL) {
		if ((error = fdtbus_reset_deassert(sc->sc_rst_ahb)) != 0)
			return error;
	}

	return 0;
}

static int
sunxi_nand_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_nand_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_nand_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int child;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": NAND Flash Controller\n");

	if (sunxi_nand_init_resources(sc) != 0) {
		aprint_error_dev(self, "couldn't initialize resources\n");
		return;
	}

	/* DT bindings allow for multiple chips but we only use the first */
	child = OF_child(phandle);
	if (!child)
		return;

	sunxi_nand_attach_chip(sc, &sc->sc_chip, child);
}
	
CFATTACH_DECL_NEW(sunxi_nand, sizeof(struct sunxi_nand_softc),
	sunxi_nand_match, sunxi_nand_attach, NULL, NULL);

