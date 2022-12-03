/*	$NetBSD: bmx280thpspi.c,v 1.1 2022/12/03 01:04:43 brad Exp $	*/

/*
 * Copyright (c) 2022 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: bmx280thpspi.c,v 1.1 2022/12/03 01:04:43 brad Exp $");

/*
 * SPI driver for the Bosch BMP280 / BME280 sensor.
 * Uses the common bmx280thp driver to do the real work.
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

#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>
#include <dev/spi/spivar.h>
#include <dev/ic/bmx280reg.h>
#include <dev/ic/bmx280var.h>

extern void	bmx280_attach(struct bmx280_sc *);

static int 	bmx280thpspi_match(device_t, cfdata_t, void *);
static void 	bmx280thpspi_attach(device_t, device_t, void *);
static int 	bmx280thpspi_detach(device_t, int);

#define BMX280_DEBUG
#ifdef BMX280_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_bmx280debug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(bmx280thpspi, sizeof(struct bmx280_sc),
    bmx280thpspi_match, bmx280thpspi_attach, bmx280thpspi_detach, NULL);

/* The SPI interface of the chip, assuming that it has managed to get into that
 * mode to start with, is pretty simple.  Simply send the register MINUS the 7th
 * bit which will be 1 and then do as many reads as you want.  The chip will
 * auto increment for you.
 *
 * The delays are only hinted at in the data sheet.
 */

static int
bmx280thpspi_read_reg_direct(struct spi_handle *sh, uint8_t reg,
    uint8_t *buf, size_t rlen)
{
	int err = 0;
	uint8_t rreg = reg | 0x80;

	if (buf != NULL) {
		err = spi_send_recv(sh, 1, &rreg,
		    rlen, buf);
	} else {
		err = spi_send(sh, 1, &rreg);
	}

	return err;
}

static int
bmx280thpspi_read_reg(struct bmx280_sc *sc, uint8_t reg, uint8_t *buf, size_t rlen)
{
	return bmx280thpspi_read_reg_direct(sc->sc_sh, reg, buf, rlen);
}

/* SPI writes to this device are normal enough.  You send the register
 * you want making sure that the high bit, 0x80, is clear and then the
 * data.  These pairs can be repeated as many times as you like.
 */
static int
bmx280thpspi_write_reg_direct(struct spi_handle *sh, uint8_t *buf, size_t slen)
{
	int err = 0;
	int i;

	/* XXX -
	   this is probably  BAD thing to do... but we must insure that the
	   registers have a cleared bit.. otherwise it is a read ....
	*/

	for(i = 0; i < slen;i+=2) {
		buf[i] = buf[i] & 0x7F;
	}

	err = spi_send(sh, slen, buf);

	return err;
}

static int
bmx280thpspi_write_reg(struct bmx280_sc *sc, uint8_t *buf, size_t slen)
{
	return bmx280thpspi_write_reg_direct(sc->sc_sh, buf, slen);
}

/* These are to satisfy the common code */
static int
bmx280thpspi_acquire_bus(struct bmx280_sc *sc)
{
	return 0;
}

static void
bmx280thpspi_release_bus(struct bmx280_sc *sc)
{
	return;
}

/* Nothing more is done here.  Assumptions on whether or not
 * the SPI interface is set up may not be proper.... for better
 * or worse... and there is setting that are needed such as the
 * SPI mode and bus speed that really should not be done here, so
 * any active match might not work anyway.
 */
static int
bmx280thpspi_match(device_t parent, cfdata_t match, void *aux)
{
	const bool matchdebug = false;

	if (matchdebug) {
		printf("Trying to match\n");
	}

	return 1;
}

static void
bmx280thpspi_attach(device_t parent, device_t self, void *aux)
{
	struct bmx280_sc *sc;
	struct spi_attach_args *sa;
	int error;

	sa = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_sh = sa->sa_handle;
	sc->sc_bmx280debug = 0;
	sc->sc_func_acquire_bus = &bmx280thpspi_acquire_bus;
	sc->sc_func_release_bus = &bmx280thpspi_release_bus;
	sc->sc_func_read_register = &bmx280thpspi_read_reg;
	sc->sc_func_write_register = &bmx280thpspi_write_reg;

	/* Configure for 1MHz and SPI mode 0 according to the data sheet.
	 * The chip will actually handle a number of different modes and
	 * can go a lot faster, just use this for now...
	 */
	error = spi_configure(self, sa->sa_handle, SPI_MODE_0, 1000000);
	if (error) {
		return;
	}

	/* Please note that if the pins are not set up for SPI, the attachment
	 * will probably not work out.
	 */
	bmx280_attach(sc);

	return;
}

/* These really do not do a whole lot, as SPI devices do not seem to work
 * as modules.
 */
static int
bmx280thpspi_detach(device_t self, int flags)
{
	struct bmx280_sc *sc;

	sc = device_private(self);

	mutex_destroy(&sc->sc_mutex);

	return 0;
}
