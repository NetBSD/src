/* $NetBSD: awin_hdmi.c,v 1.15.2.1 2015/09/22 12:05:36 skrll Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#include "opt_allwinner.h"
#include "opt_ddb.h"

#define AWIN_HDMI_PLL	3	/* PLL7 or PLL3 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_hdmi.c,v 1.15.2.1 2015/09/22 12:05:36 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/kthread.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcvar.h>
#include <dev/i2c/ddcreg.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

struct awin_hdmi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;
	lwp_t *sc_thread;

	struct i2c_controller sc_ic;
	kmutex_t sc_ic_lock;

	bool sc_connected;
	char sc_display_vendor[16];
	char sc_display_product[16];

	u_int sc_display_mode;
	u_int sc_current_display_mode;
#define DISPLAY_MODE_AUTO	0
#define DISPLAY_MODE_HDMI	1
#define DISPLAY_MODE_DVI	2

	uint32_t sc_ver;
	unsigned int sc_i2c_blklen;
};

#define HDMI_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define HDMI_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));

#define HDMI_1_3_P(sc)	((sc)->sc_ver == 0x00010003)
#define HDMI_1_4_P(sc)	((sc)->sc_ver == 0x00010004)

static int	awin_hdmi_match(device_t, cfdata_t, void *);
static void	awin_hdmi_attach(device_t, device_t, void *);

static void	awin_hdmi_i2c_init(struct awin_hdmi_softc *);
static int	awin_hdmi_i2c_acquire_bus(void *, int);
static void	awin_hdmi_i2c_release_bus(void *, int);
static int	awin_hdmi_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
				   size_t, void *, size_t, int);
static int	awin_hdmi_i2c_xfer(void *, i2c_addr_t, uint8_t, uint8_t,
				   size_t, int, int);
static int	awin_hdmi_i2c_reset(struct awin_hdmi_softc *, int);

static void	awin_hdmi_enable(struct awin_hdmi_softc *);
static void	awin_hdmi_read_edid(struct awin_hdmi_softc *);
static int	awin_hdmi_read_edid_block(struct awin_hdmi_softc *, uint8_t *,
					  uint8_t);
static u_int	awin_hdmi_get_display_mode(struct awin_hdmi_softc *,
					   const struct edid_info *);
static void	awin_hdmi_video_enable(struct awin_hdmi_softc *, bool);
static void	awin_hdmi_set_videomode(struct awin_hdmi_softc *,
					const struct videomode *, u_int);
static void	awin_hdmi_set_audiomode(struct awin_hdmi_softc *,
					const struct videomode *, u_int);
static void	awin_hdmi_hpd(struct awin_hdmi_softc *);
static void	awin_hdmi_thread(void *);
#if 0
static int	awin_hdmi_intr(void *);
#endif

#if defined(DDB)
void		awin_hdmi_dump_regs(void);
#endif

CFATTACH_DECL_NEW(awin_hdmi, sizeof(struct awin_hdmi_softc),
	awin_hdmi_match, awin_hdmi_attach, NULL, NULL);

static int
awin_hdmi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_hdmi_attach(device_t parent, device_t self, void *aux)
{
	struct awin_hdmi_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	uint32_t ver, clk;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

#if AWIN_HDMI_PLL == 3
	awin_pll3_enable();
#elif AWIN_HDMI_PLL == 7
	awin_pll7_enable();
#else
#error AWIN_HDMI_PLL must be 3 or 7
#endif

	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
		    AWIN_A31_AHB_RESET1_REG, AWIN_A31_AHB_RESET1_HDMI_RST, 0);
	}

#if AWIN_HDMI_PLL == 3
	clk = __SHIFTIN(AWIN_HDMI_CLK_SRC_SEL_PLL3, AWIN_HDMI_CLK_SRC_SEL);
#else
	clk = __SHIFTIN(AWIN_HDMI_CLK_SRC_SEL_PLL7, AWIN_HDMI_CLK_SRC_SEL);
#endif
	clk |= AWIN_CLK_ENABLE;
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		clk |= AWIN_A31_HDMI_CLK_DDC_GATING;
	}
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_HDMI_CLK_REG, clk, AWIN_HDMI_CLK_SRC_SEL);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_HDMI, 0);

	ver = HDMI_READ(sc, AWIN_HDMI_VERSION_ID_REG);

	const int vmaj = __SHIFTOUT(ver, AWIN_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(ver, AWIN_HDMI_VERSION_ID_L);

	aprint_naive("\n");
	aprint_normal(": HDMI %d.%d\n", vmaj, vmin);

	sc->sc_ver = ver;
	sc->sc_i2c_blklen = 16;

	const char *display_mode = NULL;
	prop_dictionary_get_cstring_nocopy(cfg, "display-mode", &display_mode);
	if (display_mode) {
		if (strcasecmp(display_mode, "hdmi") == 0)
			sc->sc_display_mode = DISPLAY_MODE_HDMI;
		else if (strcasecmp(display_mode, "dvi") == 0)
			sc->sc_display_mode = DISPLAY_MODE_DVI;
	}

#if 0
	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_hdmi_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);
#endif

	awin_hdmi_i2c_init(sc);

	awin_hdmi_enable(sc);

	delay(50000);

	awin_hdmi_hpd(sc);

	kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL, awin_hdmi_thread, sc,
	    &sc->sc_thread, "%s", device_xname(sc->sc_dev));
}

static void
awin_hdmi_i2c_init(struct awin_hdmi_softc *sc)
{
	struct i2c_controller *ic = &sc->sc_ic;

	mutex_init(&sc->sc_ic_lock, MUTEX_DEFAULT, IPL_NONE);

	ic->ic_cookie = sc;
	ic->ic_acquire_bus = awin_hdmi_i2c_acquire_bus;
	ic->ic_release_bus = awin_hdmi_i2c_release_bus;
	ic->ic_exec = awin_hdmi_i2c_exec;
}

static int
awin_hdmi_i2c_acquire_bus(void *priv, int flags)
{
	struct awin_hdmi_softc *sc = priv;

	if (flags & I2C_F_POLL) {
		if (!mutex_tryenter(&sc->sc_ic_lock))
			return EBUSY;
	} else {
		mutex_enter(&sc->sc_ic_lock);
	}

	return 0;
}

static void
awin_hdmi_i2c_release_bus(void *priv, int flags)
{
	struct awin_hdmi_softc *sc = priv;

	mutex_exit(&sc->sc_ic_lock);
}

static int
awin_hdmi_i2c_exec(void *priv, i2c_op_t op, i2c_addr_t addr,
    const void *cmdbuf, size_t cmdlen, void *buf, size_t len, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	uint8_t *pbuf;
	uint8_t block;
	int resid;
	off_t off;
	int err;

	KASSERT(mutex_owned(&sc->sc_ic_lock));
	KASSERT(op == I2C_OP_READ_WITH_STOP);
	KASSERT(addr == DDC_ADDR);
	KASSERT(cmdlen > 0);
	KASSERT(buf != NULL);

	err = awin_hdmi_i2c_reset(sc, flags);
	if (err)
		goto done;

	block = *(const uint8_t *)cmdbuf;
	off = (block & 1) ? 128 : 0;

	pbuf = buf;
	resid = len;
	while (resid > 0) {
		size_t blklen = min(resid, sc->sc_i2c_blklen);

		err = awin_hdmi_i2c_xfer(sc, addr, block >> 1, off, blklen,
		      AWIN_HDMI_DDC_COMMAND_ACCESS_CMD_EOREAD, flags);
		if (err)
			goto done;

		if (HDMI_1_3_P(sc)) {
			bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
			    AWIN_HDMI_DDC_FIFO_ACCESS_REG, pbuf, blklen);
		} else {
			bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
			    AWIN_A31_HDMI_DDC_FIFO_ACCESS_REG, pbuf, blklen);
		}

#ifdef AWIN_HDMI_DEBUG
		printf("off=%d:", (int)off);
		for (int i = 0; i < blklen; i++)
			printf(" %02x", pbuf[i]);
		printf("\n");
#endif

		pbuf += blklen;
		off += blklen;
		resid -= blklen;
	}

done:
	return err;
}

static int
awin_hdmi_i2c_xfer_1_3(void *priv, i2c_addr_t addr, uint8_t block, uint8_t reg,
    size_t len, int type, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	uint32_t val;
	int retry;

	val = HDMI_READ(sc, AWIN_HDMI_DDC_CTRL_REG);
	val &= ~AWIN_HDMI_DDC_CTRL_FIFO_DIR;
	HDMI_WRITE(sc, AWIN_HDMI_DDC_CTRL_REG, val);

	val |= __SHIFTIN(block, AWIN_HDMI_DDC_SLAVE_ADDR_0);
	val |= __SHIFTIN(0x60, AWIN_HDMI_DDC_SLAVE_ADDR_1);
	val |= __SHIFTIN(reg, AWIN_HDMI_DDC_SLAVE_ADDR_2);
	val |= __SHIFTIN(addr, AWIN_HDMI_DDC_SLAVE_ADDR_3);
	HDMI_WRITE(sc, AWIN_HDMI_DDC_SLAVE_ADDR_REG, val);

	val = HDMI_READ(sc, AWIN_HDMI_DDC_FIFO_CTRL_REG);
	val |= AWIN_HDMI_DDC_FIFO_CTRL_ADDR_CLEAR;
	HDMI_WRITE(sc, AWIN_HDMI_DDC_FIFO_CTRL_REG, val);

	HDMI_WRITE(sc, AWIN_HDMI_DDC_BYTE_COUNTER_REG, len);

	HDMI_WRITE(sc, AWIN_HDMI_DDC_COMMAND_REG, type);

	val = HDMI_READ(sc, AWIN_HDMI_DDC_CTRL_REG);
	val |= AWIN_HDMI_DDC_CTRL_ACCESS_CMD_START;
	HDMI_WRITE(sc, AWIN_HDMI_DDC_CTRL_REG, val);

	retry = 1000;
	while (--retry > 0) {
		val = HDMI_READ(sc, AWIN_HDMI_DDC_CTRL_REG);
		if ((val & AWIN_HDMI_DDC_CTRL_ACCESS_CMD_START) == 0)
			break;
		delay(1000);
	}
	if (retry == 0)
		return ETIMEDOUT;

	val = HDMI_READ(sc, AWIN_HDMI_DDC_INT_STATUS_REG);
	if ((val & AWIN_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE) == 0) {
		device_printf(sc->sc_dev, "xfer failed, status=%08x\n", val);
		return EIO;
	}

	return 0;
}

static int
awin_hdmi_i2c_xfer_1_4(void *priv, i2c_addr_t addr, uint8_t block, uint8_t reg,
    size_t len, int type, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	uint32_t val;
	int retry;

	val = HDMI_READ(sc, AWIN_A31_HDMI_DDC_FIFO_CTRL_REG);
	val |= AWIN_A31_HDMI_DDC_FIFO_CTRL_RST;
	HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_FIFO_CTRL_REG, val);

	val = __SHIFTIN(block, AWIN_A31_HDMI_DDC_SLAVE_ADDR_SEG_PTR);
	val |= __SHIFTIN(0x60, AWIN_A31_HDMI_DDC_SLAVE_ADDR_DDC_CMD);
	val |= __SHIFTIN(reg, AWIN_A31_HDMI_DDC_SLAVE_ADDR_OFF_ADR);
	val |= __SHIFTIN(addr, AWIN_A31_HDMI_DDC_SLAVE_ADDR_DEV_ADR);
	HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_SLAVE_ADDR_REG, val);

	HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_COMMAND_REG,
	    __SHIFTIN(len, AWIN_A31_HDMI_DDC_COMMAND_DTC) |
	    __SHIFTIN(type, AWIN_A31_HDMI_DDC_COMMAND_CMD));

	val = HDMI_READ(sc, AWIN_A31_HDMI_DDC_CTRL_REG);
	val |= AWIN_A31_HDMI_DDC_CTRL_ACCESS_CMD_START;
	HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_CTRL_REG, val);

	retry = 1000;
	while (--retry > 0) {
		val = HDMI_READ(sc, AWIN_A31_HDMI_DDC_CTRL_REG);
		if ((val & AWIN_A31_HDMI_DDC_CTRL_ACCESS_CMD_START) == 0)
			break;
		if (cold)
			delay(1000);
		else
			kpause("hdmiddc", false, mstohz(10), &sc->sc_ic_lock);
	}
	if (retry == 0)
		return ETIMEDOUT;

	return 0;
}

static int
awin_hdmi_i2c_xfer(void *priv, i2c_addr_t addr, uint8_t block, uint8_t reg,
    size_t len, int type, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	int rv;

	if (HDMI_1_3_P(sc)) {
		rv = awin_hdmi_i2c_xfer_1_3(priv, addr, block, reg, len,
		    type, flags);
	} else {
		rv = awin_hdmi_i2c_xfer_1_4(priv, addr, block, reg, len,
		    type, flags);
	}

	return rv;
}

static int
awin_hdmi_i2c_reset(struct awin_hdmi_softc *sc, int flags)
{
	uint32_t hpd, ctrl;

	hpd = HDMI_READ(sc, AWIN_HDMI_HPD_REG);
	if ((hpd & AWIN_HDMI_HPD_HOTPLUG_DET) == 0) {
		device_printf(sc->sc_dev, "no device detected\n");
		return ENODEV;	/* no device plugged in */
	}

	if (HDMI_1_3_P(sc)) {
		HDMI_WRITE(sc, AWIN_HDMI_DDC_FIFO_CTRL_REG, 0);
		HDMI_WRITE(sc, AWIN_HDMI_DDC_CTRL_REG,
		    AWIN_HDMI_DDC_CTRL_EN | AWIN_HDMI_DDC_CTRL_SW_RST); 

		delay(1000);

		ctrl = HDMI_READ(sc, AWIN_HDMI_DDC_CTRL_REG);
		if (ctrl & AWIN_HDMI_DDC_CTRL_SW_RST) {
			device_printf(sc->sc_dev, "reset failed (1.3)\n");
			return EBUSY;
		}

		/* N=5,M=1 */
		HDMI_WRITE(sc, AWIN_HDMI_DDC_CLOCK_REG,
		    __SHIFTIN(5, AWIN_HDMI_DDC_CLOCK_N) |
		    __SHIFTIN(1, AWIN_HDMI_DDC_CLOCK_M));

		HDMI_WRITE(sc, AWIN_HDMI_DDC_DBG_REG, 0x300);
	} else {
		HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_CTRL_REG,
		    AWIN_A31_HDMI_DDC_CTRL_SW_RST);

		/* N=1,M=12 */
		HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_CLOCK_REG,
		    __SHIFTIN(1, AWIN_HDMI_DDC_CLOCK_N) |
		    __SHIFTIN(12, AWIN_HDMI_DDC_CLOCK_M));

		HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_CTRL_REG,
		    AWIN_A31_HDMI_DDC_CTRL_SDA_PAD_EN |
		    AWIN_A31_HDMI_DDC_CTRL_SCL_PAD_EN |
		    AWIN_A31_HDMI_DDC_CTRL_EN);
	}

	return 0;
}

static void
awin_hdmi_enable(struct awin_hdmi_softc *sc)
{
	HDMI_WRITE(sc, AWIN_HDMI_CTRL_REG, AWIN_HDMI_CTRL_MODULE_EN);
	if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL0_REG, 0xfe800000);
		HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL1_REG, 0x00d8c830);
	} else if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL0_REG, 0x7e80000f);
		HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL1_REG, 0x01ded030);
	}
#if AWIN_HDMI_PLL == 7
	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, (1<<21));
#elif AWIN_HDMI_PLL == 3
	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, (0<<21));
#endif

	delay(1000);
}

static int
awin_hdmi_read_edid_block(struct awin_hdmi_softc *sc, uint8_t *data,
    uint8_t block)
{
	i2c_tag_t tag = &sc->sc_ic;
	uint8_t wbuf[2];
	int error;

	if ((error = iic_acquire_bus(tag, I2C_F_POLL)) != 0)
		return error;

	wbuf[0] = block;	/* start address */

	if ((error = iic_exec(tag, I2C_OP_READ_WITH_STOP, DDC_ADDR, wbuf, 1,
	    data, 128, I2C_F_POLL)) != 0) {
		iic_release_bus(tag, I2C_F_POLL);
		return error;
	}
	iic_release_bus(tag, I2C_F_POLL);

	return 0;
}

static void
awin_hdmi_read_edid(struct awin_hdmi_softc *sc)
{
	const struct videomode *mode;
	char edid[128];
	struct edid_info ei;
	int retry = 4;
	u_int display_mode;

	memset(edid, 0, sizeof(edid));
	memset(&ei, 0, sizeof(ei));

	while (--retry > 0) {
		if (!awin_hdmi_read_edid_block(sc, edid, 0))
			break;
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "failed to read EDID\n");
	} else {
		if (edid_parse(edid, &ei) != 0) {
			device_printf(sc->sc_dev, "failed to parse EDID\n");
		}
#ifdef AWIN_HDMI_DEBUG
		else {
			edid_print(&ei);
		}
#endif
	}

	if (sc->sc_display_mode == DISPLAY_MODE_AUTO)
		display_mode = awin_hdmi_get_display_mode(sc, &ei);
	else
		display_mode = sc->sc_display_mode;

	const char *forced = sc->sc_display_mode == DISPLAY_MODE_AUTO ?
	    "auto-detected" : "forced";
	device_printf(sc->sc_dev, "%s mode (%s)\n",
	    display_mode == DISPLAY_MODE_HDMI ? "HDMI" : "DVI", forced);

	strlcpy(sc->sc_display_vendor, ei.edid_vendorname,
	    sizeof(sc->sc_display_vendor));
	strlcpy(sc->sc_display_product, ei.edid_productname,
	    sizeof(sc->sc_display_product));
	sc->sc_current_display_mode = display_mode;

	mode = ei.edid_preferred_mode;
	if (mode == NULL)
		mode = pick_mode_by_ref(640, 480, 60);

	if (mode != NULL) {
		awin_hdmi_video_enable(sc, false);
		awin_tcon_enable(false);
		delay(20000);

		awin_debe_set_videomode(mode);
		awin_tcon_set_videomode(mode);
		awin_hdmi_set_videomode(sc, mode, display_mode);
		awin_hdmi_set_audiomode(sc, mode, display_mode);
		awin_debe_enable(true);
		delay(20000);
		awin_tcon_enable(true);
		delay(20000);
		awin_hdmi_video_enable(sc, true);
	}
}

static u_int
awin_hdmi_get_display_mode(struct awin_hdmi_softc *sc,
    const struct edid_info *ei)
{
	char edid[128];
	bool found_hdmi = false;
	unsigned int n, p;

	/*
	 * Scan through extension blocks, looking for a CEA-861-D v3
	 * block. If an HDMI Vendor-Specific Data Block (HDMI VSDB) is
	 * found in that, assume HDMI mode.
	 */
	for (n = 1; n <= MIN(ei->edid_ext_block_count, 4); n++) {
		if (awin_hdmi_read_edid_block(sc, edid, n)) {
#ifdef AWIN_HDMI_DEBUG
			device_printf(sc->sc_dev,
			    "Failed to read EDID block %d\n", n);
#endif
			break;
		}

#ifdef AWIN_HDMI_DEBUG
		device_printf(sc->sc_dev, "EDID block #%d:\n", n);
#endif

		const uint8_t tag = edid[0];
		const uint8_t rev = edid[1];
		const uint8_t off = edid[2];

#ifdef AWIN_HDMI_DEBUG
		device_printf(sc->sc_dev, "  Tag %d, Revision %d, Offset %d\n",
		    tag, rev, off);
		device_printf(sc->sc_dev, "  Flags: 0x%02x\n", edid[3]);
#endif

		/* We are looking for a CEA-861-D tag (02h) with revision 3 */
		if (tag != 0x02 || rev != 3)
			continue;
		/*
		 * CEA data block collection starts at byte 4, so the
		 * DTD blocks must start after it.
		 */
		if (off <= 4)
			continue;

		/* Parse the CEA data blocks */
		for (p = 4; p < off;) {
			const uint8_t btag = (edid[p] >> 5) & 0x7;
			const uint8_t blen = edid[p] & 0x1f;

#ifdef AWIN_HDMI_DEBUG
			device_printf(sc->sc_dev, "  CEA data block @ %d\n", p);
			device_printf(sc->sc_dev, "    Tag %d, Length %d\n",
			    btag, blen);
#endif

			/* Make sure the length is sane */
			if (p + blen + 1 > off)
				break;
			/* Looking for a VSDB tag */
			if (btag != 3)
				goto next_block;
			/* HDMI VSDB is at least 5 bytes long */
			if (blen < 5)
				goto next_block;

#ifdef AWIN_HDMI_DEBUG
			device_printf(sc->sc_dev, "    ID: %02x%02x%02x\n",
			    edid[p + 1], edid[p + 2], edid[p + 3]);
#endif

			/* HDMI 24-bit IEEE registration ID is 0x000C03 */
			if (memcmp(&edid[p + 1], "\x03\x0c\x00", 3) == 0)
				found_hdmi = true;

next_block:
			p += (1 + blen);
		}
	}

	return found_hdmi ? DISPLAY_MODE_HDMI : DISPLAY_MODE_DVI;
}

static void
awin_hdmi_video_enable(struct awin_hdmi_softc *sc, bool enable)
{
	uint32_t val;

	val = HDMI_READ(sc, AWIN_HDMI_VID_CTRL_REG);
	val &= ~AWIN_HDMI_VID_CTRL_SRC_SEL;
#ifdef AWIN_HDMI_CBGEN
	val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_SRC_SEL_CBGEN,
			 AWIN_HDMI_VID_CTRL_SRC_SEL);
#else
	val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_SRC_SEL_RGB,
			 AWIN_HDMI_VID_CTRL_SRC_SEL);
#endif
	if (enable) {
		val |= AWIN_HDMI_VID_CTRL_VIDEO_EN;
	} else {
		val &= ~AWIN_HDMI_VID_CTRL_VIDEO_EN;
	}
	HDMI_WRITE(sc, AWIN_HDMI_VID_CTRL_REG, val);

#if defined(AWIN_HDMI_DEBUG) && defined(DDB)
	awin_hdmi_dump_regs();
#endif
}

static void
awin_hdmi_set_videomode(struct awin_hdmi_softc *sc,
    const struct videomode *mode, u_int display_mode)
{
	uint32_t val;
	const u_int dblscan_p = !!(mode->flags & VID_DBLSCAN);
	const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
	const u_int phsync_p = !!(mode->flags & VID_PHSYNC);
	const u_int pvsync_p = !!(mode->flags & VID_PVSYNC);
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int hbp = mode->htotal - mode->hsync_start;
	const u_int vfp = mode->vsync_start - mode->vdisplay;
	const u_int vspw = mode->vsync_end - mode->vsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_start;

#ifdef AWIN_HDMI_DEBUG
	device_printf(sc->sc_dev,
	    "dblscan %d, interlace %d, phsync %d, pvsync %d\n",
	    dblscan_p, interlace_p, phsync_p, pvsync_p);
	device_printf(sc->sc_dev, "h: %u %u %u %u\n",
	    mode->hdisplay, hbp, hfp, hspw);
	device_printf(sc->sc_dev, "v: %u %u %u %u\n",
	    mode->vdisplay, vbp, vfp, vspw);
#endif

	HDMI_WRITE(sc, AWIN_HDMI_INT_STATUS_REG, 0xffffffff);

	u_int clk_div = awin_tcon_get_clk_div();
	bool clk_dbl = awin_tcon_get_clk_dbl();

#ifdef AWIN_HDMI_DEBUG
	device_printf(sc->sc_dev, "dot_clock: %d\n", mode->dot_clock);
	device_printf(sc->sc_dev, "clkdiv: %d\n", clk_div);
	device_printf(sc->sc_dev, "clkdbl: %c\n", clk_dbl ? 'Y' : 'N');
#endif

	if (clk_div == 0) {
		device_printf(sc->sc_dev, "ERROR: TCON clk not configured\n");
		return;
	}

	uint32_t pll_ctrl, pad_ctrl0, pad_ctrl1;
	if (HDMI_1_4_P(sc)) {
		pad_ctrl0 = 0x7e8000ff;
		pad_ctrl1 = 0x01ded030;
		pll_ctrl = 0xba48a308;
		pll_ctrl |= __SHIFTIN(clk_div-1, AWIN_HDMI_PLL_CTRL_PREDIV);
	} else {
		pad_ctrl0 = 0xfe800000;
		pad_ctrl1 = 0x00d8c830;
		pll_ctrl = 0xfa4ef708;
		pll_ctrl |= __SHIFTIN(clk_div, AWIN_HDMI_PLL_CTRL_PREDIV);
	}
	if (clk_dbl == false)
		pad_ctrl1 |= 0x40;

	HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL0_REG, pad_ctrl0);
	HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL1_REG, pad_ctrl1);
	HDMI_WRITE(sc, AWIN_HDMI_PLL_CTRL_REG, pll_ctrl);
#if AWIN_HDMI_PLL == 7
	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, (1<<21));
#elif AWIN_HDMI_PLL == 3
	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, (0<<21));
#endif

	val = HDMI_READ(sc, AWIN_HDMI_VID_CTRL_REG);
	val &= ~AWIN_HDMI_VID_CTRL_HDMI_MODE;
	if (display_mode == DISPLAY_MODE_DVI) {
		val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_HDMI_MODE_DVI,
				 AWIN_HDMI_VID_CTRL_HDMI_MODE);
	} else {
		val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_HDMI_MODE_HDMI,
				 AWIN_HDMI_VID_CTRL_HDMI_MODE);
	}
	val &= ~AWIN_HDMI_VID_CTRL_REPEATER_SEL;
	if (dblscan_p) {
		val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_REPEATER_SEL_2X,
				 AWIN_HDMI_VID_CTRL_REPEATER_SEL);
	}
	val &= ~AWIN_HDMI_VID_CTRL_OUTPUT_FMT;
	if (interlace_p) {
		val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_OUTPUT_FMT_INTERLACE,
				 AWIN_HDMI_VID_CTRL_OUTPUT_FMT);
	}
	HDMI_WRITE(sc, AWIN_HDMI_VID_CTRL_REG, val);

	val = __SHIFTIN((mode->hdisplay << dblscan_p) - 1,
			AWIN_HDMI_VID_TIMING_0_ACT_H);
	val |= __SHIFTIN(mode->vdisplay - 1,
			 AWIN_HDMI_VID_TIMING_0_ACT_V);
	HDMI_WRITE(sc, AWIN_HDMI_VID_TIMING_0_REG, val);

	val = __SHIFTIN((hbp << dblscan_p) - 1,
			AWIN_HDMI_VID_TIMING_1_HBP);
	val |= __SHIFTIN(vbp - 1,
			 AWIN_HDMI_VID_TIMING_1_VBP);
	HDMI_WRITE(sc, AWIN_HDMI_VID_TIMING_1_REG, val);

	val = __SHIFTIN((hfp << dblscan_p) - 1,
			AWIN_HDMI_VID_TIMING_2_HFP);
	val |= __SHIFTIN(vfp - 1,
			 AWIN_HDMI_VID_TIMING_2_VFP);
	HDMI_WRITE(sc, AWIN_HDMI_VID_TIMING_2_REG, val);

	val = __SHIFTIN((hspw << dblscan_p) - 1,
			AWIN_HDMI_VID_TIMING_3_HSPW);
	val |= __SHIFTIN(vspw - 1,
			 AWIN_HDMI_VID_TIMING_3_VSPW);
	HDMI_WRITE(sc, AWIN_HDMI_VID_TIMING_3_REG, val);

	val = 0;
	if (phsync_p) {
		val |= AWIN_HDMI_VID_TIMING_4_HSYNC_ACTIVE_SEL;
	}
	if (pvsync_p) {
		val |= AWIN_HDMI_VID_TIMING_4_VSYNC_ACTIVE_SEL;
	}
	val |= __SHIFTIN(AWIN_HDMI_VID_TIMING_4_TX_CLOCK_NORMAL,
			 AWIN_HDMI_VID_TIMING_4_TX_CLOCK);
	HDMI_WRITE(sc, AWIN_HDMI_VID_TIMING_4_REG, val);

	/* Packet control */
	HDMI_WRITE(sc, AWIN_HDMI_PKT_CTRL0_REG, 0x00005321);
	HDMI_WRITE(sc, AWIN_HDMI_PKT_CTRL1_REG, 0x0000000f);
}

static void
awin_hdmi_set_audiomode(struct awin_hdmi_softc *sc,
    const struct videomode *mode, u_int display_mode)
{
	uint32_t cts, n, val;

	/*
	 * Before changing audio parameters, disable and reset the
	 * audio module. Wait for the soft reset bit to clear before
	 * configuring the audio parameters.
	 */
	val = HDMI_READ(sc, AWIN_HDMI_AUD_CTRL_REG);
	val &= ~AWIN_HDMI_AUD_CTRL_EN;
	val |= AWIN_HDMI_AUD_CTRL_RST;
	HDMI_WRITE(sc, AWIN_HDMI_AUD_CTRL_REG, val);
	do {
		val = HDMI_READ(sc, AWIN_HDMI_AUD_CTRL_REG);
	} while (val & AWIN_HDMI_AUD_CTRL_RST);

	/* No audio support in DVI mode */
	if (display_mode != DISPLAY_MODE_HDMI) {
		return;
	}

	/* DMA & FIFO control */
	val = HDMI_READ(sc, AWIN_HDMI_ADMA_CTRL_REG);
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		val |= AWIN_HDMI_ADMA_CTRL_SRC_DMA_MODE;	/* NDMA */
	} else {
		val &= ~AWIN_HDMI_ADMA_CTRL_SRC_DMA_MODE;	/* DDMA */
	}
	val &= ~AWIN_HDMI_ADMA_CTRL_SRC_DMA_SAMPLE_RATE;
	val &= ~AWIN_HDMI_ADMA_CTRL_SRC_SAMPLE_LAYOUT;
	val &= ~AWIN_HDMI_ADMA_CTRL_SRC_WORD_LEN;
	val &= ~AWIN_HDMI_ADMA_CTRL_DATA_SEL;
	HDMI_WRITE(sc, AWIN_HDMI_ADMA_CTRL_REG, val);

	/* Audio format control */
	val = HDMI_READ(sc, AWIN_HDMI_AUD_FMT_REG);
	val &= ~AWIN_HDMI_AUD_FMT_SRC_SEL;
	val &= ~AWIN_HDMI_AUD_FMT_SEL;
	val &= ~AWIN_HDMI_AUD_FMT_DSD_FMT;
	val &= ~AWIN_HDMI_AUD_FMT_LAYOUT;
	val &= ~AWIN_HDMI_AUD_FMT_SRC_CH_CFG;
	val |= __SHIFTIN(1, AWIN_HDMI_AUD_FMT_SRC_CH_CFG);
	HDMI_WRITE(sc, AWIN_HDMI_AUD_FMT_REG, val);

	/* PCM control (channel map) */
	HDMI_WRITE(sc, AWIN_HDMI_AUD_PCM_CTRL_REG, 0x76543210);

	/* Clock setup */
	n = 6144;	/* 48 kHz */
	cts = ((mode->dot_clock * 10) * (n / 128)) / 480;
	HDMI_WRITE(sc, AWIN_HDMI_AUD_CTS_REG, cts);
	HDMI_WRITE(sc, AWIN_HDMI_AUD_N_REG, n);

	/* Audio PCM channel status 0 */
	val = __SHIFTIN(AWIN_HDMI_AUD_CH_STATUS0_FS_FREQ_48,
			AWIN_HDMI_AUD_CH_STATUS0_FS_FREQ);
	HDMI_WRITE(sc, AWIN_HDMI_AUD_CH_STATUS0_REG, val);

	/* Audio PCM channel status 1 */
	val = HDMI_READ(sc, AWIN_HDMI_AUD_CH_STATUS1_REG);
	val &= ~AWIN_HDMI_AUD_CH_STATUS1_CGMS_A;
	val &= ~AWIN_HDMI_AUD_CH_STATUS1_ORIGINAL_FS;
	val &= ~AWIN_HDMI_AUD_CH_STATUS1_WORD_LEN;
	val |= __SHIFTIN(5, AWIN_HDMI_AUD_CH_STATUS1_WORD_LEN);
	val |= AWIN_HDMI_AUD_CH_STATUS1_WORD_LEN_MAX;
	HDMI_WRITE(sc, AWIN_HDMI_AUD_CH_STATUS1_REG, val);

	/* Re-enable */
	val = HDMI_READ(sc, AWIN_HDMI_AUD_CTRL_REG);
	val |= AWIN_HDMI_AUD_CTRL_EN;
	HDMI_WRITE(sc, AWIN_HDMI_AUD_CTRL_REG, val);

#if defined(AWIN_HDMI_DEBUG) && defined(DDB)
	awin_hdmi_dump_regs();
#endif
}

static void
awin_hdmi_hpd(struct awin_hdmi_softc *sc)
{
	uint32_t hpd = HDMI_READ(sc, AWIN_HDMI_HPD_REG);
	bool con = !!(hpd & AWIN_HDMI_HPD_HOTPLUG_DET);

	if (sc->sc_connected == con)
		return;

	if (con) {
		device_printf(sc->sc_dev, "display connected\n");
		awin_hdmi_read_edid(sc);
	} else {
		device_printf(sc->sc_dev, "display disconnected\n");
		awin_tcon_set_videomode(NULL);
	}

	sc->sc_connected = con;
}

static void
awin_hdmi_thread(void *priv)
{
	struct awin_hdmi_softc *sc = priv;

	for (;;) {
		awin_hdmi_hpd(sc);
		kpause("hdmihotplug", false, mstohz(1000), NULL);
	}
}

#if 0
static int
awin_hdmi_intr(void *priv)
{
	struct awin_hdmi_softc *sc = priv;
	uint32_t intsts;

	intsts = HDMI_READ(sc, AWIN_HDMI_INT_STATUS_REG);
	if (!(intsts & 0x73))
		return 0;

	HDMI_WRITE(sc, AWIN_HDMI_INT_STATUS_REG, intsts);

	device_printf(sc->sc_dev, "INT_STATUS %08X\n", intsts);

	return 1;
}
#endif

void
awin_hdmi_get_info(struct awin_hdmi_info *info)
{
	struct awin_hdmi_softc *sc;
	device_t dev;

	memset(info, 0, sizeof(*info));

	dev = device_find_by_driver_unit("awinhdmi", 0);
	if (dev == NULL) {
		info->display_connected = false;
		return;
	}
	sc = device_private(dev);

	info->display_connected = sc->sc_connected;
	if (info->display_connected) {
		strlcpy(info->display_vendor, sc->sc_display_vendor,
		    sizeof(info->display_vendor));
		strlcpy(info->display_product, sc->sc_display_product,
		    sizeof(info->display_product));
		info->display_hdmimode =
		    sc->sc_current_display_mode == DISPLAY_MODE_HDMI;
	}
}

#if defined(DDB)
void
awin_hdmi_dump_regs(void)
{
	static const struct {
		const char *name;
		uint16_t reg;
	} regs[] = {
		{ "CTRL", AWIN_HDMI_CTRL_REG },
		{ "INT_STATUS", AWIN_HDMI_INT_STATUS_REG },
		{ "VID_CTRL", AWIN_HDMI_VID_CTRL_REG },
		{ "VID_TIMING_0", AWIN_HDMI_VID_TIMING_0_REG },
		{ "VID_TIMING_1", AWIN_HDMI_VID_TIMING_1_REG },
		{ "VID_TIMING_2", AWIN_HDMI_VID_TIMING_2_REG },
		{ "VID_TIMING_3", AWIN_HDMI_VID_TIMING_3_REG },
		{ "VID_TIMING_4", AWIN_HDMI_VID_TIMING_4_REG },
		{ "PAD_CTRL0", AWIN_HDMI_PAD_CTRL0_REG },
		{ "PAD_CTRL1", AWIN_HDMI_PAD_CTRL1_REG },
		{ "PLL_CTRL", AWIN_HDMI_PLL_CTRL_REG },
		{ "PLL_DBG0", AWIN_HDMI_PLL_DBG0_REG },
		{ "PLL_DBG1", AWIN_HDMI_PLL_DBG1_REG },
	};
	struct awin_hdmi_softc *sc;
	device_t dev;

	dev = device_find_by_driver_unit("awinhdmi", 0);
	if (dev == NULL)
		return;
	sc = device_private(dev);

	for (int i = 0; i < __arraycount(regs); i++) {
		printf("%s: 0x%08x\n", regs[i].name,
		    HDMI_READ(sc, regs[i].reg));
	}
}
#endif
