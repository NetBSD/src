/* $NetBSD: tcakp.c,v 1.5.2.2 2018/06/25 07:25:50 pgoyette Exp $ */

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

#include "opt_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tcakp.c,v 1.5.2.2 2018/06/25 07:25:50 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/bitops.h>

#include <dev/i2c/i2cvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/linux_keymap.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

#define	TCA_MAX_ROWS		8
#define	TCA_MAX_COLS		10

#define	TCA_CFG			0x01
#define	 CFG_AI			__BIT(7)
#define	 CFG_GPI_E_CFG		__BIT(6)
#define	 CFG_OVR_FLOW_M		__BIT(5)
#define	 CFG_INT_CFG		__BIT(4)
#define	 CFG_OVR_FLOW_IEN	__BIT(3)
#define	 CFG_K_LCK_IEN		__BIT(2)
#define	 CFG_GPI_IEN		__BIT(1)
#define	 CFG_KE_IEN		__BIT(0)
#define	TCA_INT_STAT		0x02
#define	 INT_STAT_CAD_INT	__BIT(4)
#define	 INT_STAT_OVR_FLOW_INT	__BIT(3)
#define	 INT_STAT_K_LCD_INT	__BIT(2)
#define	 INT_STAT_GPI_INT	__BIT(1)
#define	 INT_STAT_K_INT		__BIT(0)
#define	TCA_KEY_LCK_EC		0x03
#define	 KEY_LCK_EC_K_LCK_EN	__BIT(6)
#define	 KEY_LCK_EC_LCK2	__BIT(5)
#define	 KEY_LCK_EC_LCK1	__BIT(4)
#define	 KEY_LCK_EC_KEC		__BITS(3,0)
#define	TCA_EVENT(c)		(0x04 + (c) - 'A')
#define	 TCA_EVENT_STATE	__BIT(7)
#define	 TCA_EVENT_CODE		__BITS(6,0)
#define	TCA_KP_GPIO1		0x1d
#define	TCA_KP_GPIO2		0x1e
#define	TCA_KP_GPIO3		0x1f
#define	TCA_DEBOUNCE_DIS1	0x29
#define	TCA_DEBOUNCE_DIS2	0x2a
#define	TCA_DEBOUNCE_DIS3	0x2b

struct tcakp_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;
	int		sc_phandle;

	u_int		sc_rows;
	u_int		sc_cols;
	bool		sc_autorepeat;
	u_int		sc_row_shift;

	uint16_t	sc_keymap[128];

	void		*sc_ih;

	int		sc_enabled;
	device_t	sc_wskbddev;
};

static int	tcakp_match(device_t, cfdata_t, void *);
static void	tcakp_attach(device_t, device_t, void *);

static int	tcakp_read(struct tcakp_softc *, uint8_t, uint8_t *);
static int	tcakp_write(struct tcakp_softc *, uint8_t, uint8_t);

CFATTACH_DECL_NEW(tcakp, sizeof(struct tcakp_softc),
    tcakp_match, tcakp_attach, NULL, NULL);

static const char * tcakp_compats[] = {
	"ti,tca8418",
	NULL
};

static const struct device_compatible_entry tcakp_compat_data[] = {
	DEVICE_COMPAT_ENTRY(tcakp_compats),
	DEVICE_COMPAT_TERMINATOR
};

static u_int
tcakp_decode(struct tcakp_softc *sc, uint8_t code)
{
	u_int row = code / TCA_MAX_COLS;
	u_int col = code % TCA_MAX_COLS;
	if (col == 0) {
		row = row - 1;
		col = TCA_MAX_COLS - 1;
	} else {
		col = col - 1;
	}

	return (row << sc->sc_row_shift) + col;
}

static int
tcakp_intr(void *priv)
{
	struct tcakp_softc * const sc = priv;
	uint8_t stat, ev;
	int ret = 0;

	tcakp_read(sc, TCA_INT_STAT, &stat);
	if (stat & INT_STAT_K_INT) {
		tcakp_read(sc, TCA_EVENT('A'), &ev);
		while (ev != 0) {
			const bool pressed = __SHIFTOUT(ev, TCA_EVENT_STATE);
			const uint8_t code = __SHIFTOUT(ev, TCA_EVENT_CODE);

			/* Translate raw code to keymap index */
			const u_int index = tcakp_decode(sc, code);

			u_int type = pressed ? WSCONS_EVENT_KEY_DOWN :
					       WSCONS_EVENT_KEY_UP;
			int key = linux_key_to_usb(sc->sc_keymap[index]);

			if (sc->sc_wskbddev)
				wskbd_input(sc->sc_wskbddev, type, key);

			tcakp_read(sc, TCA_EVENT('A'), &ev);
		}
		ret = 1;
	}
	tcakp_write(sc, TCA_INT_STAT, stat);

	return ret;
}

static int
tcakp_init(struct tcakp_softc *sc)
{
	uint32_t mask;
	uint8_t val;

	if (sc->sc_rows == 0 || sc->sc_cols == 0) {
		aprint_error_dev(sc->sc_dev, "not configured\n");
		return ENXIO;
	}

	mask = __BITS(sc->sc_rows - 1, 0);
	mask += __BITS(sc->sc_cols - 1, 0) << 8;

	/* Lock the keyboard */
	tcakp_write(sc, TCA_KEY_LCK_EC, KEY_LCK_EC_K_LCK_EN);
	/* Select keyboard mode */
	tcakp_write(sc, TCA_KP_GPIO1, (mask >> 0) & 0xff);
	tcakp_write(sc, TCA_KP_GPIO2, (mask >> 8) & 0xff);
	tcakp_write(sc, TCA_KP_GPIO3, (mask >> 16) & 0xff);
	/* Disable debounce */
	tcakp_write(sc, TCA_DEBOUNCE_DIS1, (mask >> 0) & 0xff);
	tcakp_write(sc, TCA_DEBOUNCE_DIS2, (mask >> 8) & 0xff);
	tcakp_write(sc, TCA_DEBOUNCE_DIS3, (mask >> 16) & 0xff);
	/* Enable key event interrupts */
	tcakp_write(sc, TCA_CFG, CFG_INT_CFG | CFG_KE_IEN);
	/* Clear interrupts */
	tcakp_read(sc, TCA_INT_STAT, &val);
	tcakp_write(sc, TCA_INT_STAT, val);

	return 0;
}

static void
tcakp_configure_fdt(struct tcakp_softc *sc)
{
	const uint32_t *keymap;
	int len;

	of_getprop_uint32(sc->sc_phandle, "keypad,num-rows", &sc->sc_rows);
	of_getprop_uint32(sc->sc_phandle, "keypad,num-columns", &sc->sc_cols);
	sc->sc_autorepeat = of_getprop_bool(sc->sc_phandle, "keypad,autorepeat");

	keymap = fdtbus_get_prop(sc->sc_phandle, "linux,keymap", &len);
	if (keymap == NULL || len <= 0)
		return;

	sc->sc_row_shift = fls32(sc->sc_cols) - 1;
	if (sc->sc_row_shift & (sc->sc_cols - 1))
		sc->sc_row_shift++;

	while (len >= 4) {
		const uint32_t e = be32toh(*keymap);
		const u_int row = (e >> 24) & 0xff;
		const u_int col = (e >> 16) & 0xff;
		const u_int index = (row << sc->sc_row_shift) + col;

		sc->sc_keymap[index] = e & 0xffff;

		len -= 4;
		keymap++;
	}
}

static int
tcakp_enable(void *v, int on)
{
	struct tcakp_softc * const sc = v;

	if (on) {
		/* Disable key lock */
		tcakp_write(sc, TCA_KEY_LCK_EC, 0);
	} else {
		/* Enable key lock */
		tcakp_write(sc, TCA_KEY_LCK_EC, KEY_LCK_EC_K_LCK_EN);
	}

	return 0;
}

static void
tcakp_set_leds(void *v, int leds)
{
}

static int
tcakp_ioctl(void *v, u_long cmd, void *data, int flag, lwp_t *l)
{
	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_USB;
		return 0;
	}

	return EPASSTHROUGH;
}

static const struct wskbd_accessops tcakp_accessops = {
	tcakp_enable,
	tcakp_set_leds,
	tcakp_ioctl,
};

#if notyet
static void
tcakp_cngetc(void *v, u_int *type, int *data)
{
	struct tcakp_softc * const sc = v;
	uint8_t ev = 0;

	do {
		tcakp_read(sc, TCA_EVENT('A'), &ev);
	} while (ev == 0);

	const bool pressed = __SHIFTOUT(ev, TCA_EVENT_STATE);
	const uint8_t code = __SHIFTOUT(ev, TCA_EVENT_CODE);
	const u_int index = tcakp_decode(sc, code);

	*type = pressed ? WSCONS_EVENT_KEY_DOWN :
			  WSCONS_EVENT_KEY_UP;
	*data = sc->sc_keymap[index];
}

static void
tcakp_cnpollc(void *v, int on)
{
}

static void
tcakp_cnbell(void *v, u_int pitch, u_int period, u_int volume)
{
}

static const struct wskbd_consops tcakp_consops = {
	tcakp_cngetc,
	tcakp_cnpollc,
	tcakp_cnbell,
};
#endif

extern const struct wscons_keydesc hidkbd_keydesctab[];
static const struct wskbd_mapdata tcakp_keymapdata = {
	hidkbd_keydesctab,
	KB_US,
};

static int
tcakp_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, match, tcakp_compat_data, &match_result))
		return match_result;

	if (ia->ia_addr == 0x34)
		return I2C_MATCH_ADDRESS_ONLY;
	
	return 0;
}

static void
tcakp_attach(device_t parent, device_t self, void *aux)
{
	struct tcakp_softc * const sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	struct wskbddev_attach_args a;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_phandle = ia->ia_cookie;

	aprint_naive("\n");
	aprint_normal(": TCA8418\n");

#ifdef FDT
	sc->sc_ih = fdtbus_intr_establish(sc->sc_phandle, 0, IPL_VM, 0,
	    tcakp_intr, sc);

	tcakp_configure_fdt(sc);
#endif

	if (tcakp_init(sc) != 0)
		return;

	memset(&a, 0, sizeof(a));
	a.console = false;	/* XXX */
	a.keymap = &tcakp_keymapdata;
	a.accessops = &tcakp_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found_ia(self, "wskbddev", &a, wskbddevprint);
}

static int
tcakp_read(struct tcakp_softc *sc, uint8_t reg, uint8_t *val)
{
	return iic_exec(sc->sc_i2c, I2C_OP_READ_WITH_STOP, sc->sc_addr,
	    &reg, 1, val, 1, I2C_F_POLL);
}

static int
tcakp_write(struct tcakp_softc *sc, uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = { reg, val };
	return iic_exec(sc->sc_i2c, I2C_OP_WRITE_WITH_STOP, sc->sc_addr,
	    NULL, 0, buf, 2, I2C_F_POLL);
}
