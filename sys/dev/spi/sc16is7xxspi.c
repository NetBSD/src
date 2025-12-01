/*	$NetBSD: sc16is7xxspi.c,v 1.3 2025/12/01 14:56:02 brad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sc16is7xxspi.c,v 1.3 2025/12/01 14:56:02 brad Exp $");

/*
 * SPI frontend driver for the SC16IS7xx UART bridge.
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

#include "opt_sc16is7xx.h"

struct sc16is7xx_spi_softc {
	struct sc16is7xx_sc sc_sc16is7xx;
	spi_handle_t sc_sh;
};

#define	SC16IS7XX_TO_SPI(sc)	\
	container_of((sc), struct sc16is7xx_spi_softc, sc_sc16is7xx)

static int sc16is7xxspi_match(device_t, cfdata_t, void *);
static void sc16is7xxspi_attach(device_t, device_t, void *);
static int sc16is7xxspi_detach(device_t, int);

CFATTACH_DECL_NEW(sc16is7xxspi, sizeof(struct sc16is7xx_spi_softc),
    sc16is7xxspi_match, sc16is7xxspi_attach, sc16is7xxspi_detach, NULL);

static int
sc16is7xxspi_read_register_direct(spi_handle_t sh,
    uint8_t reg, int channel, uint8_t *buf, size_t blen)
{
	int error;
	uint8_t xreg;

	xreg = ((reg << 3) | (channel << 1)) | 0x80;

	error = spi_send_recv(sh, 1, &xreg, blen, buf);
	return error;
}

static int
sc16is7xxspi_write_register_direct(spi_handle_t sh,
    uint8_t reg, int channel, uint8_t *buf, size_t blen)
{
	int error;
	uint8_t xreg;
	struct iovec iov[2];

	xreg = (reg << 3) | (channel << 1);

	KASSERTMSG(!(xreg & 0x80), "xreg=%02x", xreg);	/* panic if this ends up
							 * trying to be a read */

	iov[0].iov_len = 1;
	iov[0].iov_base = &xreg;
	iov[1].iov_len = blen;
	iov[1].iov_base = buf;
	error = spi_sendv(sh, &iov[0], 2);

	return error;
}
/* Use these after the hand off to the general driver happens */

static int
sc16is7xxspi_read_register(struct sc16is7xx_sc *sc, uint8_t reg, int channel,
    uint8_t *buf, size_t blen)
{
	struct sc16is7xx_spi_softc *ssc = SC16IS7XX_TO_SPI(sc);
	int error;

	KASSERT(blen > 0);

	error = sc16is7xxspi_read_register_direct(ssc->sc_sh,
	    reg, channel, buf, blen);

	return error;
}

static int
sc16is7xxspi_write_register(struct sc16is7xx_sc *sc, uint8_t reg, int channel,
    uint8_t *buf, size_t blen)
{
	struct sc16is7xx_spi_softc *ssc = SC16IS7XX_TO_SPI(sc);
	int error;

	KASSERT(blen > 0);

	error = sc16is7xxspi_write_register_direct(ssc->sc_sh,
	    reg, channel, buf, blen);

	return error;
}

static void
sc16is7xxspi_copy_handles(struct sc16is7xx_sc *sc, struct com_regs *regs)
{
	regs->cr_cookie = sc;
}

static const struct sc16is7xx_accessfuncs sc16is7xx_spi_accessfuncs = {
	.read_reg = sc16is7xxspi_read_register,
	.write_reg = sc16is7xxspi_write_register,
	.copy_handles = sc16is7xxspi_copy_handles,
};
/* These will be used by dev/ic/com.c, conform to what is expected */

static uint8_t
sc16is7xx_spi_com_read_1(struct com_regs *regs, u_int reg)
{
	struct sc16is7xx_sc *sc = (struct sc16is7xx_sc *)regs->cr_cookie;
	struct sc16is7xx_spi_softc *isc = SC16IS7XX_TO_SPI(sc);
	uint8_t buf;
	int error;

	if (regs->cr_has_errored)
		return 0;

	error = sc16is7xxspi_read_register_direct(isc->sc_sh,
	    reg, regs->cr_channel, &buf, 1);

	if (!error)
		return buf;

	if (error) {
		device_printf(sc->sc_dev, "sc16is7xx_spi_com_read_1: error=%d\n",error);
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
sc16is7xx_spi_com_write_1(struct com_regs *regs, u_int reg, uint8_t val)
{
	struct sc16is7xx_sc *sc = (struct sc16is7xx_sc *)regs->cr_cookie;
	struct sc16is7xx_spi_softc *isc = SC16IS7XX_TO_SPI(sc);
	int error = 0;

	if (regs->cr_has_errored)
		return;

	error = sc16is7xxspi_write_register_direct(isc->sc_sh,
	    reg, regs->cr_channel, &val, 1);

	if (error) {
		device_printf(sc->sc_dev, "sc16is7xx_spi_com_write_1: error=%d\n",error);
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
sc16is7xx_spi_com_write_multi_1(struct com_regs *regs, u_int reg, const uint8_t *datap,
    bus_size_t count)
{
	struct sc16is7xx_sc *sc = (struct sc16is7xx_sc *)regs->cr_cookie;
	struct sc16is7xx_spi_softc *isc = SC16IS7XX_TO_SPI(sc);
	int error = 0;

	if (regs->cr_has_errored)
		return;

	error = sc16is7xxspi_write_register_direct(isc->sc_sh,
	    reg, regs->cr_channel, __UNCONST(datap), count);
	if (error) {
		device_printf(sc->sc_dev, "sc16is7xx_spi_com_write_multi_1: error=%d\n",error);
		regs->cr_has_errored = true;
		mutex_enter(&sc->sc_thread_mutex);
		if (sc->sc_thread_state != SC16IS7XX_THREAD_GPIO) {
			sc->sc_thread_state = SC16IS7XX_THREAD_STALLED;
			cv_signal(&sc->sc_threadvar);
		}
		mutex_exit(&sc->sc_thread_mutex);
	}
}

static const struct sc16is7xx_accessfuncs sc16is7xx_spi_com_accessfuncs = {
	.com_read_1 = sc16is7xx_spi_com_read_1,
	.com_write_1 = sc16is7xx_spi_com_write_1,
	.com_write_multi_1 = sc16is7xx_spi_com_write_multi_1,
};

static int
sc16is7xxspi_match(device_t parent, cfdata_t match, void *aux)
{
	struct spi_attach_args *sa = aux;
	int match_result;

	if (spi_use_direct_match(sa, sc16is7xx_compat_data, &match_result)) {
		return match_result;
	}
	return SPI_MATCH_DEFAULT;
}

#ifndef SC16IS7XX_SPI_FREQUENCY
#define SC16IS7XX_SPI_FREQUENCY 1
#endif

static void
sc16is7xxspi_attach(device_t parent, device_t self, void *aux)
{
	struct sc16is7xx_spi_softc *ssc = device_private(self);
	struct sc16is7xx_sc *sc = &ssc->sc_sc16is7xx;
	struct spi_attach_args *sa = aux;
	int error;

	sc->sc_dev = self;
	sc->sc_funcs = &sc16is7xx_spi_accessfuncs;
	sc->sc_com_funcs = &sc16is7xx_spi_com_accessfuncs;

	ssc->sc_sh = sa->sa_handle;

	aprint_normal("\n");
	aprint_normal_dev(sc->sc_dev, "SPI frequency %dMhz", SC16IS7XX_SPI_FREQUENCY);

	/* Configure for SPI mode 0 according to the data sheet. The chip will
	 * do up to 4Mhz or 15Mhz depending on the varient and does support
	 * other modes. */
	error = spi_configure(self, sa->sa_handle, SPI_MODE_0, SPI_FREQ_MHz(SC16IS7XX_SPI_FREQUENCY));
	if (error) {
		return;
	}
	sc16is7xx_attach(sc);
}

static int
sc16is7xxspi_detach(device_t self, int flags)
{
	struct sc16is7xx_spi_softc *ssc = device_private(self);

	return sc16is7xx_detach(&ssc->sc_sc16is7xx, flags);
}
