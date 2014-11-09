/* $NetBSD: awin_hdmi.c,v 1.4 2014/11/09 14:30:55 jmcneill Exp $ */

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

#define AWIN_HDMI_DEBUG

#include "opt_ddb.h"

#define AWIN_HDMI_PLL	3	/* PLL7 or PLL3 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_hdmi.c,v 1.4 2014/11/09 14:30:55 jmcneill Exp $");

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

	uint32_t sc_ver;
	unsigned int sc_i2c_blklen;
};

#define HDMI_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define HDMI_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));

#define HDMI_1_3_P(sc)	((sc)->sc_ver == 0x0103)
#define HDMI_1_4_P(sc)	((sc)->sc_ver == 0x0104)

static int	awin_hdmi_match(device_t, cfdata_t, void *);
static void	awin_hdmi_attach(device_t, device_t, void *);

static void	awin_hdmi_i2c_init(struct awin_hdmi_softc *);
static int	awin_hdmi_i2c_acquire_bus(void *, int);
static void	awin_hdmi_i2c_release_bus(void *, int);
static int	awin_hdmi_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
				   size_t, void *, size_t, int);
static int	awin_hdmi_i2c_xfer(void *, i2c_addr_t, uint8_t,
				   size_t, int, int);
static int	awin_hdmi_i2c_reset(struct awin_hdmi_softc *, int);

static void	awin_hdmi_enable(struct awin_hdmi_softc *);
static void	awin_hdmi_read_edid(struct awin_hdmi_softc *);
static void	awin_hdmi_set_videomode(struct awin_hdmi_softc *,
					const struct videomode *);
static void	awin_hdmi_set_audiomode(struct awin_hdmi_softc *,
					const struct videomode *);
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

#if AWIN_HDMI_PLL == 7
	clk = __SHIFTIN(AWIN_HDMI_CLK_SRC_SEL_PLL7, AWIN_HDMI_CLK_SRC_SEL);
#else
	clk = __SHIFTIN(AWIN_HDMI_CLK_SRC_SEL_PLL3, AWIN_HDMI_CLK_SRC_SEL);
#endif
	clk |= __SHIFTIN(0, AWIN_HDMI_CLK_DIV_RATIO_M);
	clk |= AWIN_CLK_ENABLE;
	if (awin_chip_id() == AWIN_CHIP_ID_A31) {
		clk |= AWIN_A31_HDMI_CLK_DDC_GATING;
	}
	bus_space_write_4(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_HDMI_CLK_REG, clk);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_HDMI, 0);

	ver = HDMI_READ(sc, AWIN_HDMI_VERSION_ID_REG);

	const int vmaj = __SHIFTOUT(ver, AWIN_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(ver, AWIN_HDMI_VERSION_ID_L);

	aprint_naive("\n");
	aprint_normal(": HDMI %d.%d\n", vmaj, vmin);

	sc->sc_ver = ver;
	if (HDMI_1_3_P(sc)) {
		sc->sc_i2c_blklen = 1;
	} else {
		sc->sc_i2c_blklen = 16;
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

	off = *(const uint8_t *)cmdbuf;

	pbuf = buf;
	resid = len;
	while (resid > 0) {
		size_t blklen = min(resid, sc->sc_i2c_blklen);

		err = awin_hdmi_i2c_xfer(sc, addr, off, blklen,
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

		pbuf += blklen;
		off += blklen;
		resid -= blklen;
	}

done:
	return err;
}

static int
awin_hdmi_i2c_xfer_1_3(void *priv, i2c_addr_t addr, uint8_t reg,
    size_t len, int type, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	uint32_t val;
	int retry;

	val = HDMI_READ(sc, AWIN_HDMI_DDC_CTRL_REG);
	val &= ~AWIN_HDMI_DDC_CTRL_FIFO_DIR;
	HDMI_WRITE(sc, AWIN_HDMI_DDC_CTRL_REG, val);

	val = __SHIFTIN(0x60, AWIN_HDMI_DDC_SLAVE_ADDR_1);
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

	val = HDMI_READ(sc, AWIN_HDMI_DDC_FIFO_STATUS_REG);
	if ((val & AWIN_HDMI_DDC_INT_STATUS_TRANSFER_COMPLETE) == 0) {
		device_printf(sc->sc_dev, "xfer failed, status=%08x\n", val);
		return EIO;
	}

	return 0;
}

static int
awin_hdmi_i2c_xfer_1_4(void *priv, i2c_addr_t addr, uint8_t reg,
    size_t len, int type, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	uint32_t val;
	int retry;

	val = HDMI_READ(sc, AWIN_A31_HDMI_DDC_FIFO_CTRL_REG);
	val |= AWIN_A31_HDMI_DDC_FIFO_CTRL_RST;
	HDMI_WRITE(sc, AWIN_A31_HDMI_DDC_FIFO_CTRL_REG, val);

	val = __SHIFTIN(0, AWIN_A31_HDMI_DDC_SLAVE_ADDR_SEG_PTR);
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
awin_hdmi_i2c_xfer(void *priv, i2c_addr_t addr, uint8_t reg,
    size_t len, int type, int flags)
{
	struct awin_hdmi_softc *sc = priv;
	int rv;

	if (HDMI_1_3_P(sc)) {
		rv = awin_hdmi_i2c_xfer_1_3(priv, addr, reg, len, type, flags);
	} else {
		rv = awin_hdmi_i2c_xfer_1_4(priv, addr, reg, len, type, flags);
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

		delay(10000);

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
}

static void
awin_hdmi_read_edid(struct awin_hdmi_softc *sc)
{
	const struct videomode *mode;
	char edid[128];
	struct edid_info ei;
	int retry = 4;

	while (--retry > 0) {
		if (ddc_read_edid(&sc->sc_ic, edid, sizeof(edid)) == 0)
			break;
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "failed to read EDID\n");
		return;
	}

	edid_parse(edid, &ei);
#ifdef AWIN_HDMI_DEBUG
	edid_print(&ei);
#endif

	mode = ei.edid_preferred_mode;
	if (mode == NULL)
		mode = pick_mode_by_ref(640, 480, 60);

	awin_debe_set_videomode(mode);
	awin_tcon_set_videomode(mode);

	if (mode != NULL) {
		delay(10000);
		awin_hdmi_set_videomode(sc, mode);
		awin_hdmi_set_audiomode(sc, mode);
	}
}

static void
awin_hdmi_set_videomode(struct awin_hdmi_softc *sc, const struct videomode *mode)
{
	uint32_t val;
	const u_int dblscan_p = !!(mode->flags & VID_DBLSCAN);
	const u_int interlace_p = !!(mode->flags & VID_INTERLACE);
	const u_int phsync_p = !!(mode->flags & VID_PHSYNC);
	const u_int pvsync_p = !!(mode->flags & VID_PVSYNC);
	const u_int hbp = mode->htotal - mode->hsync_start;
	const u_int hfp = mode->hsync_start - mode->hdisplay;
	const u_int hspw = mode->hsync_end - mode->hsync_start;
	const u_int vbp = mode->vtotal - mode->vsync_start;
	const u_int vfp = mode->vsync_start - mode->vdisplay;
	const u_int vspw = mode->vsync_end - mode->vsync_start;

#ifdef AWIN_HDMI_DEBUG
	device_printf(sc->sc_dev,
	    "dblscan %d, interlace %d, phsync %d, pvsync %d\n",
	    dblscan_p, interlace_p, phsync_p, pvsync_p);
	device_printf(sc->sc_dev, "h: %u %u %u %u\n",
	    mode->hdisplay, hbp, hfp, hspw);
	device_printf(sc->sc_dev, "v: %u %u %u %u\n",
	    mode->vdisplay, vbp, vfp, vspw);
#endif

	HDMI_WRITE(sc, AWIN_HDMI_VID_CTRL_REG, 0);
	HDMI_WRITE(sc, AWIN_HDMI_INT_STATUS_REG, 0xffffffff);

	val = 0;
	if (dblscan_p) {
		val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_REPEATER_SEL_2X,
				 AWIN_HDMI_VID_CTRL_REPEATER_SEL);
	}
	if (interlace_p) {
		val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_OUTPUT_FMT_INTERLACE,
				 AWIN_HDMI_VID_CTRL_OUTPUT_FMT);
	}
	HDMI_WRITE(sc, AWIN_HDMI_VID_CTRL_REG, val);

	val = __SHIFTIN((mode->hdisplay << dblscan_p) - 1,
			AWIN_HDMI_VID_TIMING_0_ACT_H);
	if (interlace_p) {
		val |= __SHIFTIN((mode->vdisplay / 2) - 1,
				 AWIN_HDMI_VID_TIMING_0_ACT_V);
	} else {
		val |= __SHIFTIN(mode->vdisplay - 1,
				 AWIN_HDMI_VID_TIMING_0_ACT_V);
	}
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

	u_int clk_div = awin_tcon_get_clk_div();

#ifdef AWIN_HDMI_DEBUG
	device_printf(sc->sc_dev, "dot_clock: %d\n", mode->dot_clock);
	device_printf(sc->sc_dev, "clkdiv: %d\n", clk_div);
#endif

	if (clk_div == 0) {
		device_printf(sc->sc_dev, "ERROR: TCON clk not configured\n");
		return;
	}

	uint32_t pll_ctrl, pad_ctrl0, pad_ctrl1;
	if (HDMI_1_4_P(sc)) {
		pad_ctrl0 = 0x7e8000ff;
		pad_ctrl1 = 0x01ded030 | 0x40;
		pll_ctrl = 0xba48a308;
		pll_ctrl |= __SHIFTIN(clk_div-1, AWIN_HDMI_PLL_CTRL_PREDIV);
	} else {
		pad_ctrl0 = 0xfe800000;
		pad_ctrl1 = 0x00d8c830 | 0x40;
		pll_ctrl = 0xfa4ef708;
		pll_ctrl |= __SHIFTIN(clk_div, AWIN_HDMI_PLL_CTRL_PREDIV);
	}

	HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL0_REG, pad_ctrl0);
	HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL1_REG, pad_ctrl1);
	HDMI_WRITE(sc, AWIN_HDMI_PLL_CTRL_REG, pll_ctrl);

#if AWIN_HDMI_PLL == 7
	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, (1<<21));
#elif AWIN_HDMI_PLL == 3
	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, (0<<21));
#endif

	delay(50000);

	val = HDMI_READ(sc, AWIN_HDMI_VID_CTRL_REG);
#ifdef AWIN_HDMI_CBGEN
	val |= __SHIFTIN(AWIN_HDMI_VID_CTRL_SRC_SEL_CBGEN,
			 AWIN_HDMI_VID_CTRL_SRC_SEL);
#endif
	val |= AWIN_HDMI_VID_CTRL_VIDEO_EN;
	HDMI_WRITE(sc, AWIN_HDMI_VID_CTRL_REG, val);

#if defined(AWIN_HDMI_DEBUG) && defined(DDB)
	awin_hdmi_dump_regs();
#endif
}

static void
awin_hdmi_set_audiomode(struct awin_hdmi_softc *sc, const struct videomode *mode)
{
	/* TODO */
	HDMI_WRITE(sc, AWIN_HDMI_AUD_CTRL_REG, 0);
}

static void
awin_hdmi_hpd(struct awin_hdmi_softc *sc)
{
	uint32_t hpd = HDMI_READ(sc, AWIN_HDMI_HPD_REG);
	bool con = !!(hpd & AWIN_HDMI_HPD_HOTPLUG_DET);

	if (sc->sc_connected == con)
		return;

	sc->sc_connected = con;
	if (con) {
		device_printf(sc->sc_dev, "display connected\n");
		awin_hdmi_read_edid(sc);
	} else {
		device_printf(sc->sc_dev, "display disconnected\n");
		awin_tcon_set_videomode(NULL);
	}
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
	if (!intsts)
		return 0;

	HDMI_WRITE(sc, AWIN_HDMI_INT_STATUS_REG, intsts);

	device_printf(sc->sc_dev, "INT_STATUS %08X\n", intsts);

	return 1;
}
#endif

#if defined(DDB)
void
awin_hdmi_dump_regs(void)
{
	static const struct {
		const char *name;
		uint16_t reg;
	} regs[] = {
		{ "CTRL", AWIN_HDMI_CTRL_REG },
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
