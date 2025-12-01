/*	$NetBSD: sc16is7xxi2c.c,v 1.3 2025/12/01 14:56:02 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sc16is7xxi2c.c,v 1.3 2025/12/01 14:56:02 brad Exp $");

/*
 * I2C frontend driver for the SC16IS7xx UART bridge.
 * The heavy lifting is done by the general sc16is7xx(4)
 * driver and the com(4) backend.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/pool.h>
#include <sys/kmem.h>

#include <dev/i2c/i2cvar.h>
#include <dev/spi/spivar.h>
#include <dev/ic/sc16is7xxreg.h>
#include <dev/ic/sc16is7xxvar.h>

struct sc16is7xx_i2c_softc {
	struct sc16is7xx_sc sc_sc16is7xx;
	i2c_tag_t sc_tag;
	i2c_addr_t sc_addr;
};

#define	SC16IS7XX_TO_I2C(sc)	\
	container_of((sc), struct sc16is7xx_i2c_softc, sc_sc16is7xx)

static int sc16is7xxi2c_poke(i2c_tag_t, i2c_addr_t, bool);
static int sc16is7xxi2c_match(device_t, cfdata_t, void *);
static void sc16is7xxi2c_attach(device_t, device_t, void *);
static int sc16is7xxi2c_detach(device_t, int);

CFATTACH_DECL_NEW(sc16is7xxi2c, sizeof(struct sc16is7xx_i2c_softc),
    sc16is7xxi2c_match, sc16is7xxi2c_attach, sc16is7xxi2c_detach, NULL);

static int
sc16is7xxi2c_read_register_direct(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    int channel, uint8_t *buf, size_t blen)
{
	int error;
	uint8_t xreg;

	xreg = (reg << 3) | (channel << 1);

	error = iic_acquire_bus(tag, 0);
	if (error == 0) {
		error = iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, &xreg, 1,
		    buf, blen, 0);
	}
	iic_release_bus(tag, 0);

	return error;
}

static int
sc16is7xxi2c_write_register_direct(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg,
    int channel, uint8_t *buf, size_t blen)
{
	int error;
	uint8_t xreg;

	xreg = (reg << 3) | (channel << 1);

	error = iic_acquire_bus(tag, 0);
	if (error == 0) {
		error = iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &xreg, 1,
		    buf, blen, 0);
	}
	iic_release_bus(tag, 0);

	return error;
}
/* Use these after the hand off to the general driver happens */

static int
sc16is7xxi2c_read_register(struct sc16is7xx_sc *sc, uint8_t reg, int channel,
    uint8_t *buf, size_t blen)
{
	struct sc16is7xx_i2c_softc *isc = SC16IS7XX_TO_I2C(sc);
	int error;

	KASSERT(blen > 0);

	error = sc16is7xxi2c_read_register_direct(isc->sc_tag, isc->sc_addr,
	    reg, channel, buf, blen);

	return error;
}

static int
sc16is7xxi2c_write_register(struct sc16is7xx_sc *sc, uint8_t reg, int channel,
    uint8_t *buf, size_t blen)
{
	struct sc16is7xx_i2c_softc *isc = SC16IS7XX_TO_I2C(sc);
	int error;

	KASSERT(blen > 0);

	error = sc16is7xxi2c_write_register_direct(isc->sc_tag, isc->sc_addr,
	    reg, channel, buf, blen);

	return error;
}

static void
sc16is7xxi2c_copy_handles(struct sc16is7xx_sc *sc, struct com_regs *regs)
{
	regs->cr_cookie = sc;
}

static const struct sc16is7xx_accessfuncs sc16is7xx_i2c_accessfuncs = {
	.read_reg = sc16is7xxi2c_read_register,
	.write_reg = sc16is7xxi2c_write_register,
	.copy_handles = sc16is7xxi2c_copy_handles,
};
/* These will be used by dev/ic/com.c, conform to what is expected */

static uint8_t
sc16is7xx_i2c_com_read_1(struct com_regs *regs, u_int reg)
{
	struct sc16is7xx_sc *sc = (struct sc16is7xx_sc *)regs->cr_cookie;
	struct sc16is7xx_i2c_softc *isc = SC16IS7XX_TO_I2C(sc);
	uint8_t buf;
	int error;

	if (regs->cr_has_errored)
		return 0;

	error = sc16is7xxi2c_read_register_direct(isc->sc_tag, isc->sc_addr,
	    reg, regs->cr_channel, &buf, 1);

	if (!error)
		return buf;

	if (error) {
		device_printf(sc->sc_dev, "sc16is7xx_i2c_com_read_1: error=%d\n",error);
		regs->cr_has_errored = true;
		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state != SC16IS7XX_THREAD_GPIO) {
			sc->sc_thread_state = SC16IS7XX_THREAD_STALLED;
			cv_signal(&sc->sc_threadvar);
		}
		mutex_exit(&sc->sc_thread_mutex);
	}

	return 0;
}

static void
sc16is7xx_i2c_com_write_1(struct com_regs *regs, u_int reg, uint8_t val)
{
	struct sc16is7xx_sc *sc = (struct sc16is7xx_sc *)regs->cr_cookie;
	struct sc16is7xx_i2c_softc *isc = SC16IS7XX_TO_I2C(sc);
	int error = 0;

	if (regs->cr_has_errored)
		return;

	error = sc16is7xxi2c_write_register_direct(isc->sc_tag, isc->sc_addr,
	    reg, regs->cr_channel, &val, 1);

	if (error) {
		device_printf(sc->sc_dev, "sc16is7xx_i2c_com_write_1: error=%d\n",error);
		regs->cr_has_errored = true;
		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state != SC16IS7XX_THREAD_GPIO) {
			sc->sc_thread_state = SC16IS7XX_THREAD_STALLED;
			cv_signal(&sc->sc_threadvar);
		}
		mutex_exit(&sc->sc_thread_mutex);
	}
}

static void
sc16is7xx_i2c_com_write_multi_1(struct com_regs *regs, u_int reg, const uint8_t *datap,
    bus_size_t count)
{
	struct sc16is7xx_sc *sc = (struct sc16is7xx_sc *)regs->cr_cookie;
	struct sc16is7xx_i2c_softc *isc = SC16IS7XX_TO_I2C(sc);
	int error = 0;

	if (regs->cr_has_errored)
		return;

	error = sc16is7xxi2c_write_register_direct(isc->sc_tag, isc->sc_addr,
	    reg, regs->cr_channel, __UNCONST(datap), count);

	if (error) {
		device_printf(sc->sc_dev, "sc16is7xx_i2c_com_write_multi_1: error=%d\n",error);
		regs->cr_has_errored = true;
		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state != SC16IS7XX_THREAD_GPIO) {
			sc->sc_thread_state = SC16IS7XX_THREAD_STALLED;
			cv_signal(&sc->sc_threadvar);
		}
		mutex_exit(&sc->sc_thread_mutex);
	}
}

static const struct sc16is7xx_accessfuncs sc16is7xx_i2c_com_accessfuncs = {
	.com_read_1 = sc16is7xx_i2c_com_read_1,
	.com_write_1 = sc16is7xx_i2c_com_write_1,
	.com_write_multi_1 = sc16is7xx_i2c_com_write_multi_1,
};

static int
sc16is7xxi2c_poke(i2c_tag_t tag, i2c_addr_t addr, bool matchdebug)
{
	uint8_t reg = SC16IS7XX_REGISTER_SPR;
	uint8_t buf[1];
	int error;

	error = sc16is7xxi2c_read_register_direct(tag, addr, reg, 0, buf, 1);
	if (matchdebug) {
		printf("poke addr=%02x, error=%d\n", addr, error);
	}
	return error;
}

static int
sc16is7xxi2c_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int error, match_result;
	const bool matchdebug = false;
	bool indirect_found = false;
	int i;

	if (iic_use_direct_match(ia, cf, sc16is7xx_compat_data, &match_result)) {
		return match_result;
	}
	for (i = SC16IS7XX_LOW_I2C_ADDR; i <= SC16IS7XX_HIGH_I2C_ADDR && indirect_found == false; i++) {
		if (ia->ia_addr == i) {
			if (matchdebug) {
				printf("sc16is7xxi2c_match possible indirect: ia_addr=%02x, i=%02x\n", ia->ia_addr, i);
			}
			indirect_found = true;
		}
	}

	if (!indirect_found)
		return 0;

	/* Check to see if something is really at this i2c address. This will
	 * keep phantom devices from appearing */

	error = sc16is7xxi2c_poke(ia->ia_tag, ia->ia_addr, matchdebug);

	return error == 0 ? I2C_MATCH_ADDRESS_AND_PROBE : 0;
}

static void
sc16is7xxi2c_attach(device_t parent, device_t self, void *aux)
{
	struct sc16is7xx_i2c_softc *isc = device_private(self);
	struct sc16is7xx_sc *sc = &isc->sc_sc16is7xx;
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_funcs = &sc16is7xx_i2c_accessfuncs;
	sc->sc_com_funcs = &sc16is7xx_i2c_com_accessfuncs;

	isc->sc_tag = ia->ia_tag;
	isc->sc_addr = ia->ia_addr;

	sc16is7xx_attach(sc);
}

static int
sc16is7xxi2c_detach(device_t self, int flags)
{
	struct sc16is7xx_i2c_softc *isc = device_private(self);

	return sc16is7xx_detach(&isc->sc_sc16is7xx, flags);
}
