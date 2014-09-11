/* $NetBSD: awin_hdmi.c,v 1.1 2014/09/11 02:21:19 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_hdmi.c,v 1.1 2014/09/11 02:21:19 jmcneill Exp $");

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

/* NB: Should be 16, but it doesn't seem to work */
#define HDMI_I2C_MAX_BLKLEN	1

struct awin_hdmi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;
	lwp_t *sc_thread;

	struct i2c_controller sc_ic;
	kmutex_t sc_ic_lock;

	bool sc_connected;
};

#define HDMI_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define HDMI_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));

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
static void	awin_hdmi_connect(struct awin_hdmi_softc *);
static void	awin_hdmi_read_edid(struct awin_hdmi_softc *);
static void	awin_hdmi_thread(void *);
static int	awin_hdmi_intr(void *);

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
	uint32_t ver;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	awin_pll7_enable();
	bus_space_write_4(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_HDMI_CLK_REG,
	    __SHIFTIN(AWIN_HDMI_CLK_SRC_SEL_PLL7, AWIN_HDMI_CLK_SRC_SEL) |
	    __SHIFTIN(0x0, AWIN_HDMI_CLK_DIV_RATIO_M) |
	    AWIN_CLK_ENABLE);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_HDMI, 0);

	ver = HDMI_READ(sc, AWIN_HDMI_VERSION_ID_REG);

	const int vmaj = __SHIFTOUT(ver, AWIN_HDMI_VERSION_ID_H);
	const int vmin = __SHIFTOUT(ver, AWIN_HDMI_VERSION_ID_L);

	aprint_naive("\n");
	aprint_normal(": HDMI %d.%d\n", vmaj, vmin);

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_hdmi_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	awin_hdmi_i2c_init(sc);

	awin_hdmi_enable(sc);

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
		size_t blklen = min(resid, HDMI_I2C_MAX_BLKLEN);

		err = awin_hdmi_i2c_xfer(sc, addr, off, blklen,
		    AWIN_HDMI_DDC_COMMAND_ACCESS_CMD_EOREAD, flags);
		if (err)
			goto done;

		bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh,
		    AWIN_HDMI_DDC_FIFO_ACCESS_REG, pbuf, blklen);

		pbuf += blklen;
		off += blklen;
		resid -= blklen;
	}

done:
	return err;
}

static int
awin_hdmi_i2c_xfer(void *priv, i2c_addr_t addr, uint8_t reg,
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
awin_hdmi_i2c_reset(struct awin_hdmi_softc *sc, int flags)
{
	uint32_t hpd, ctrl;

	hpd = HDMI_READ(sc, AWIN_HDMI_HPD_REG);
	if ((hpd & AWIN_HDMI_HPD_HOTPLUG_DET) == 0) {
		device_printf(sc->sc_dev, "no device detected\n");
		return ENODEV;	/* no device plugged in */
	}


	HDMI_WRITE(sc, AWIN_HDMI_DDC_FIFO_CTRL_REG, 0);
	HDMI_WRITE(sc, AWIN_HDMI_DDC_CTRL_REG,
	    AWIN_HDMI_DDC_CTRL_EN | AWIN_HDMI_DDC_CTRL_SW_RST); 

	delay(10000);

	ctrl = HDMI_READ(sc, AWIN_HDMI_DDC_CTRL_REG);
	if (ctrl & AWIN_HDMI_DDC_CTRL_SW_RST) {
		device_printf(sc->sc_dev, "reset failed\n");
		return EBUSY;	/* reset failed */
	}

	/* N=5,M=1 */
	HDMI_WRITE(sc, AWIN_HDMI_DDC_CLOCK_REG,
	    __SHIFTIN(5, AWIN_HDMI_DDC_CLOCK_N) |
	    __SHIFTIN(1, AWIN_HDMI_DDC_CLOCK_M));

	HDMI_WRITE(sc, AWIN_HDMI_DDC_DBG_REG, 0x300);

	return 0;
}

static void
awin_hdmi_enable(struct awin_hdmi_softc *sc)
{
	HDMI_WRITE(sc, AWIN_HDMI_CTRL_REG, AWIN_HDMI_CTRL_MODULE_EN);
	if (awin_chip_id() == AWIN_CHIP_ID_A20) {
		HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL0_REG,
		    AWIN_HDMI_PAD_CTRL0_BIAS |
		    AWIN_HDMI_PAD_CTRL0_LDOCEN |
		    AWIN_HDMI_PAD_CTRL0_LD0DEN);
	}
}

static void
awin_hdmi_connect(struct awin_hdmi_softc *sc)
{
	HDMI_WRITE(sc, AWIN_HDMI_CTRL_REG, AWIN_HDMI_CTRL_MODULE_EN);

	HDMI_WRITE(sc, AWIN_HDMI_PLL_CTRL_REG,
	    AWIN_HDMI_PLL_CTRL_PLL_EN |
	    AWIN_HDMI_PLL_CTRL_BWS |
	    AWIN_HDMI_PLL_CTRL_HV_IS_33 |
	    AWIN_HDMI_PLL_CTRL_LDO1_EN |
	    AWIN_HDMI_PLL_CTRL_LDO2_EN |
	    AWIN_HDMI_PLL_CTRL_SDIV2 |
	    __SHIFTIN(0x4, AWIN_HDMI_PLL_CTRL_VCO_GAIN) |
	    __SHIFTIN(0x7, AWIN_HDMI_PLL_CTRL_S) |
	    __SHIFTIN(0xf, AWIN_HDMI_PLL_CTRL_CP_S) |
	    __SHIFTIN(0x7, AWIN_HDMI_PLL_CTRL_CS) |
	    __SHIFTIN(0xf, AWIN_HDMI_PLL_CTRL_PREDIV) |
	    __SHIFTIN(0x8, AWIN_HDMI_PLL_CTRL_VCO_S));

	HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL0_REG,
	    AWIN_HDMI_PAD_CTRL0_BIAS |
	    AWIN_HDMI_PAD_CTRL0_LDOCEN |
	    AWIN_HDMI_PAD_CTRL0_LD0DEN |
	    AWIN_HDMI_PAD_CTRL0_PWENC |
	    AWIN_HDMI_PAD_CTRL0_PWEND |
	    AWIN_HDMI_PAD_CTRL0_PWENG |
	    AWIN_HDMI_PAD_CTRL0_CKEN |
	    AWIN_HDMI_PAD_CTRL0_TXEN);

	HDMI_WRITE(sc, AWIN_HDMI_PAD_CTRL1_REG,
	    AWIN_HDMI_PAD_CTRL1_AMP_OPT |
	    AWIN_HDMI_PAD_CTRL1_AMPCK_OPT |
	    AWIN_HDMI_PAD_CTRL1_EMP_OPT |
	    AWIN_HDMI_PAD_CTRL1_EMPCK_OPT |
	    AWIN_HDMI_PAD_CTRL1_REG_DEN |
	    AWIN_HDMI_PAD_CTRL1_REG_DENCK |
	    __SHIFTIN(0x2, AWIN_HDMI_PAD_CTRL1_REG_EMP) |
	    __SHIFTIN(0x1, AWIN_HDMI_PAD_CTRL1_REG_CKSS) |
	    __SHIFTIN(0x4, AWIN_HDMI_PAD_CTRL1_REG_AMP));

	HDMI_WRITE(sc, AWIN_HDMI_PLL_DBG0_REG, 0);
}

static void
awin_hdmi_read_edid(struct awin_hdmi_softc *sc)
{
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
	edid_print(&ei);
}

static void
awin_hdmi_thread(void *priv)
{
	struct awin_hdmi_softc *sc = priv;

	for (;;) {
		uint32_t hpd = HDMI_READ(sc, AWIN_HDMI_HPD_REG);
		bool con = !!(hpd & AWIN_HDMI_HPD_HOTPLUG_DET);

		if (sc->sc_connected == con)
			goto next;

		sc->sc_connected = con;
		if (con) {
			device_printf(sc->sc_dev, "display connected\n");
			awin_hdmi_connect(sc);
			awin_hdmi_read_edid(sc);
		} else {
			device_printf(sc->sc_dev, "display disconnected\n");
		}

next:
		kpause("hdmihotplug", false, mstohz(1000), NULL);
	}
}

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
