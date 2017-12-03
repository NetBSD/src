/* $NetBSD: tegra_cec.c,v 1.3.16.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_cec.c,v 1.3.16.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/poll.h>
#include <sys/select.h>

#include <dev/hdmicec/hdmicecio.h>
#include <dev/hdmicec/hdmicec_if.h>

#include <arm/nvidia/tegra_var.h>
#include <arm/nvidia/tegra_pmcreg.h>
#include <arm/nvidia/tegra_cecreg.h>

#include <dev/fdt/fdtvar.h>

#define CEC_VENDORID_NVIDIA	0x00044b

static int	tegra_cec_match(device_t, cfdata_t, void *);
static void	tegra_cec_attach(device_t, device_t, void *);

static int	tegra_cec_intr(void *);

struct tegra_cec_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	void			*sc_ih;
	struct clk		*sc_clk;
	struct fdtbus_reset	*sc_rst;

	kmutex_t		sc_lock;
	kcondvar_t		sc_cv;

	const char		*sc_hdmidevname;
	device_t		sc_cecdev;

	struct selinfo		sc_selinfo;

	uint8_t			sc_rxbuf[16];
	int			sc_rxlen;
	bool			sc_rxdone;

	uint8_t			sc_txbuf[16];
	int			sc_txlen;
	int			sc_txcur;
	int			sc_txerr;
	bool			sc_txdone;
};

static void	tegra_cec_reset(struct tegra_cec_softc *);

static int	tegra_cec_open(void *, int);
static void	tegra_cec_close(void *);
static int	tegra_cec_ioctl(void *, u_long, void *, int, lwp_t *);
static int	tegra_cec_send(void *, const uint8_t *, size_t);
static ssize_t	tegra_cec_recv(void *, uint8_t *, size_t);
static int	tegra_cec_poll(void *, int, lwp_t *);

static const struct hdmicec_hw_if tegra_cec_hw_if = {
	.open = tegra_cec_open,
	.close = tegra_cec_close,
	.ioctl = tegra_cec_ioctl,
	.send = tegra_cec_send,
	.recv = tegra_cec_recv,
	.poll = tegra_cec_poll,
};

CFATTACH_DECL_NEW(tegra_cec, sizeof(struct tegra_cec_softc),
	tegra_cec_match, tegra_cec_attach, NULL, NULL);

#define CEC_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define CEC_WRITE(sc, reg, val)			\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))
#define CEC_SET_CLEAR(sc, reg, set, clr)	\
    tegra_reg_set_clear((sc)->sc_bst, (sc)->sc_bsh, (reg), (set), (clr))

static int
tegra_cec_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "nvidia,tegra124-cec", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_cec_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_cec_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t prop = device_properties(self);
	struct hdmicec_attach_args caa;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_clk = fdtbus_clock_get(faa->faa_phandle, "cec");
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock cec\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get(faa->faa_phandle, "cec");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset cec\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	cv_init(&sc->sc_cv, "tegracec");
	selinit(&sc->sc_selinfo);

	aprint_naive("\n");
	aprint_normal(": HDMI CEC\n");

	if (!fdtbus_intr_str(faa->faa_phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(faa->faa_phandle, 0, IPL_VM,
	    FDT_INTR_MPSAFE, tegra_cec_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	prop_dictionary_get_cstring_nocopy(prop, "hdmi-device",
	    &sc->sc_hdmidevname);

	fdtbus_reset_assert(sc->sc_rst);
	error = clk_enable(sc->sc_clk);
	if (error) {
		aprint_error_dev(self, "couldn't enable cec: %d\n", error);
		return;
	}
	fdtbus_reset_deassert(sc->sc_rst);

	CEC_WRITE(sc, CEC_SW_CONTROL_REG, 0);
	CEC_WRITE(sc, CEC_INPUT_FILTER_REG, 0);
	CEC_WRITE(sc, CEC_HW_CONTROL_REG, 0);
	CEC_WRITE(sc, CEC_INT_MASK_REG, 0);
	CEC_WRITE(sc, CEC_INT_STAT_REG, 0xffffffff);

	memset(&caa, 0, sizeof(caa));
	caa.priv = sc;
	caa.hwif = &tegra_cec_hw_if;
	sc->sc_cecdev = config_found(self, &caa, NULL);
}

static int
tegra_cec_intr(void *priv)
{
	struct tegra_cec_softc * const sc = priv;
	uint32_t val;
	int handled = 0;

	mutex_enter(&sc->sc_lock);
	const uint32_t int_stat = CEC_READ(sc, CEC_INT_STAT_REG);

	if (int_stat & CEC_INT_RX_REGISTER_FULL) {
		val = CEC_READ(sc, CEC_RX_REGISTER_REG);
		sc->sc_rxbuf[sc->sc_rxlen++] =
		    __SHIFTOUT(val, CEC_RX_REGISTER_DATA);
		if ((val & CEC_RX_REGISTER_EOM) != 0 ||
		    sc->sc_rxlen == 16) {
			CEC_SET_CLEAR(sc, CEC_INT_MASK_REG, 0,
			    CEC_INT_RX_REGISTER_FULL);
			sc->sc_rxdone = true;
			cv_broadcast(&sc->sc_cv);
			selnotify(&sc->sc_selinfo, POLLIN|POLLRDNORM,
			    NOTE_SUBMIT);
		}
		CEC_WRITE(sc, CEC_INT_STAT_REG, CEC_INT_RX_REGISTER_FULL);
		++handled;
	}

	if (int_stat & CEC_INT_TX_REGISTER_EMPTY) {
		if (sc->sc_txcur < sc->sc_txlen) {
			const uint8_t destination = sc->sc_txbuf[0] & 0xf;
			val = __SHIFTIN(sc->sc_txbuf[sc->sc_txcur],
			    CEC_TX_REGISTER_DATA);
			if (sc->sc_txcur == 0)
				val |= CEC_TX_REGISTER_GENERATE_START_BIT;
			if (sc->sc_txcur == sc->sc_txlen - 1)
				val |= CEC_TX_REGISTER_EOM;
			if (destination == 0xf)
				val |= CEC_TX_REGISTER_ADDRESS_MODE;

			CEC_WRITE(sc, CEC_TX_REGISTER_REG, val);
			CEC_WRITE(sc, CEC_INT_STAT_REG,
			    CEC_INT_TX_REGISTER_EMPTY);
			++sc->sc_txcur;
		} else {
			CEC_SET_CLEAR(sc, CEC_INT_MASK_REG, 0,
			    CEC_INT_TX_REGISTER_EMPTY);
		}
		++handled;
	}

	if (int_stat & CEC_INT_TX_FRAME_TRANSMITTED) {
		CEC_SET_CLEAR(sc, CEC_INT_MASK_REG, 0,
		    CEC_INT_TX_FRAME_TRANSMITTED |
		    CEC_INT_TX_FRAME_OR_BLOCK_NAKD);
		CEC_WRITE(sc, CEC_INT_STAT_REG, CEC_INT_TX_FRAME_TRANSMITTED);
		if (int_stat & CEC_INT_TX_FRAME_OR_BLOCK_NAKD) {
			CEC_WRITE(sc, CEC_INT_STAT_REG,
			    CEC_INT_TX_FRAME_OR_BLOCK_NAKD);
			sc->sc_txerr = ECONNREFUSED;
			tegra_cec_reset(sc);
		}
		sc->sc_txdone = true;
		cv_broadcast(&sc->sc_cv);
		++handled;
	}

	if (int_stat & CEC_INT_TX_REGISTER_UNDERRUN) {
		tegra_cec_reset(sc);
		cv_broadcast(&sc->sc_cv);
		++handled;
	}

	mutex_exit(&sc->sc_lock);

	return handled;
}

static void
tegra_cec_reset(struct tegra_cec_softc *sc)
{
	uint32_t val;

	KASSERT(mutex_owned(&sc->sc_lock));

	val = CEC_READ(sc, CEC_HW_CONTROL_REG);
	CEC_WRITE(sc, CEC_HW_CONTROL_REG, 0);
	CEC_WRITE(sc, CEC_INT_STAT_REG, 0xffffffff);
	CEC_WRITE(sc, CEC_HW_CONTROL_REG, val);
}

static int
tegra_cec_open(void *priv, int flag)
{
	struct tegra_cec_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);
	sc->sc_rxlen = 0;
	sc->sc_rxdone = false;
	CEC_WRITE(sc, CEC_INT_MASK_REG, CEC_INT_RX_REGISTER_FULL);
	CEC_WRITE(sc, CEC_HW_CONTROL_REG, CEC_HW_CONTROL_TX_RX_MODE);
	mutex_exit(&sc->sc_lock);

	return 0;
}

static void
tegra_cec_close(void *priv)
{
	struct tegra_cec_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);
	CEC_WRITE(sc, CEC_HW_CONTROL_REG, 0);
	CEC_WRITE(sc, CEC_INT_MASK_REG, 0);
	CEC_WRITE(sc, CEC_INT_STAT_REG, 0xffffffff);
	mutex_exit(&sc->sc_lock);
}

static int
tegra_cec_get_phys_addr(struct tegra_cec_softc *sc, uint16_t *phys_addr)
{
	device_t hdmidev;

	if (sc->sc_hdmidevname == NULL)
		return EIO;
	hdmidev = device_find_by_xname(sc->sc_hdmidevname);
	if (hdmidev == NULL)
		return ENXIO;

	const prop_dictionary_t prop = device_properties(hdmidev);
	if (!prop_dictionary_get_uint16(prop, "physical-address", phys_addr))
		return ENOTCONN;

	return 0;
}

static int
tegra_cec_ioctl(void *priv, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct tegra_cec_softc * const sc = priv;
	uint32_t val;

	switch (cmd) {
	case CEC_GET_PHYS_ADDR:
		return tegra_cec_get_phys_addr(sc, data);
	case CEC_GET_LOG_ADDRS:
		val = CEC_READ(sc, CEC_HW_CONTROL_REG);
		*(uint16_t *)data =
		    __SHIFTOUT(val, CEC_HW_CONTROL_RX_LOGICAL_ADDRS);
		return 0;
	case CEC_SET_LOG_ADDRS:
		val = *(uint16_t *)data & 0x7fff;
		CEC_SET_CLEAR(sc, CEC_HW_CONTROL_REG,
		    __SHIFTIN(val, CEC_HW_CONTROL_RX_LOGICAL_ADDRS),
		    CEC_HW_CONTROL_RX_LOGICAL_ADDRS);
		return 0;
	case CEC_GET_VENDOR_ID:
		*(uint32_t *)data = CEC_VENDORID_NVIDIA;
		return 0;
	default:
		return EINVAL;
	}
}

static int
tegra_cec_send(void *priv, const uint8_t *data, size_t len)
{
	struct tegra_cec_softc * const sc = priv;
	int error = 0;

	mutex_enter(&sc->sc_lock);

	sc->sc_txdone = false;
	sc->sc_txcur = 0;
	sc->sc_txerr = 0;
	memcpy(sc->sc_txbuf, data, len);
	sc->sc_txlen = len;

	CEC_SET_CLEAR(sc, CEC_INT_MASK_REG,
	    CEC_INT_TX_REGISTER_EMPTY |
	    CEC_INT_TX_FRAME_TRANSMITTED |
	    CEC_INT_TX_FRAME_OR_BLOCK_NAKD, 0);

	while (sc->sc_txdone == false) {
		error = cv_timedwait_sig(&sc->sc_cv, &sc->sc_lock, hz);
		if (error)
			break;
	}

	if (sc->sc_txdone)
		error = sc->sc_txerr;

	mutex_exit(&sc->sc_lock);

	return error;
}

static ssize_t
tegra_cec_recv(void *priv, uint8_t *data, size_t len)
{
	struct tegra_cec_softc * const sc = priv;
	ssize_t alen = -1;
	int error = 0;

	mutex_enter(&sc->sc_lock);

	while (sc->sc_rxdone == false) {
		error = cv_timedwait_sig(&sc->sc_cv, &sc->sc_lock, hz);
		if (error)
			break;
	}

	if (sc->sc_rxdone) {
		memcpy(data, sc->sc_rxbuf, sc->sc_rxlen);
		alen = sc->sc_rxlen;
		sc->sc_rxlen = 0;
		sc->sc_rxdone = false;
	}

	mutex_exit(&sc->sc_lock);

	return alen;
}

static int
tegra_cec_poll(void *priv, int events, lwp_t *l)
{
	struct tegra_cec_softc * const sc = priv;
	int revents;

	revents = events & (POLLOUT | POLLWRNORM);

	if ((events & (POLLIN | POLLRDNORM)) == 0)
		return revents;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_rxdone) {
		revents = (events & (POLLIN | POLLRDNORM));
	} else {
		selrecord(l, &sc->sc_selinfo);
		revents = 0;
	}
	mutex_exit(&sc->sc_lock);

	return revents;
}
