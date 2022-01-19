
/*	$NetBSD: scmdspi.c,v 1.3 2022/01/19 05:21:44 thorpej Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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
__KERNEL_RCSID(0, "$NetBSD: scmdspi.c,v 1.3 2022/01/19 05:21:44 thorpej Exp $");

/*
 * SPI driver for the Sparkfun Serial motor controller.
 * Uses the common scmd driver to do the real work.
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
#include <dev/ic/scmdreg.h>
#include <dev/ic/scmdvar.h>

extern void	scmd_attach(struct scmd_sc *);

static int 	scmdspi_match(device_t, cfdata_t, void *);
static void 	scmdspi_attach(device_t, device_t, void *);
static int 	scmdspi_detach(device_t, int);
static int	scmdspi_activate(device_t, enum devact);

#define SCMD_DEBUG
#ifdef SCMD_DEBUG
#define DPRINTF(s, l, x) \
    do { \
	if (l <= s->sc_scmddebug) \
	    printf x; \
    } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s, l, x)
#endif

CFATTACH_DECL_NEW(scmdspi, sizeof(struct scmd_sc),
    scmdspi_match, scmdspi_attach, scmdspi_detach, scmdspi_activate);

/* For the SPI interface on this device, the reads are done in an odd
 * manor.  The first part is normal enough, you send the register binary
 * or'ed with 0x80 and then the receive the data.  However, you MUST also
 * then receive a dummy value otherwise, everything gets out of sync and
 * no further reads appear to work unless you do a SPI receive all by itself.
 * This is documented in the data sheet for this device.
 *
 * Please note that the Ardunio code does this a little differently.  What is
 * below works on a Raspberry PI 3 without any apparent problems.
 *
 * The delays are also mentioned in the datasheet as being 20us, however, the
 * Ardunio code does 50us, so do likewise.
 */
static int
scmdspi_read_reg_direct(struct spi_handle *sh, uint8_t reg,
    uint8_t *buf)
{
	int err;
	uint8_t b;
	uint8_t rreg = reg | 0x80;

	err = spi_send(sh, 1, &rreg);
	if (err)
		return err;

	delay(50);

	b = SCMD_HOLE_VALUE;
	err = spi_recv(sh, 1, &b);
	if (err)
		return err;

	*buf = b;

	delay(50);

	b = SCMD_HOLE_VALUE;
	err = spi_recv(sh, 1, &b);
	delay(50);

	return err;
}

static int
scmdspi_read_reg(struct scmd_sc *sc, uint8_t reg, uint8_t *buf)
{
	return scmdspi_read_reg_direct(sc->sc_sh, reg, buf);
}

/* SPI writes to this device are normal enough.  You send the register
 * you want making sure that the high bit, 0x80, is clear and then the
 * data.
 *
 * The rule about waiting between operations appears to not apply, however.
 * This does more or less what the Ardunio code does.
 */
static int
scmdspi_write_reg_direct(struct spi_handle *sh, uint8_t reg,
    uint8_t buf)
{
	uint8_t rreg = reg & 0x7F;
	int err;

	err = spi_send(sh, 1, &rreg);
	if (err)
		return err;

	err = spi_send(sh, 1, &buf);
	if (err)
		return err;

	delay(50);

	return err;
}

static int
scmdspi_write_reg(struct scmd_sc *sc, uint8_t reg, uint8_t buf)
{
	return scmdspi_write_reg_direct(sc->sc_sh, reg, buf);
}

/* These are to satisfy the common code */
static int
scmdspi_acquire_bus(struct scmd_sc *sc)
{
	return 0;
}

static void
scmdspi_release_bus(struct scmd_sc *sc)
{
	return;
}

/* Nothing more is done here. It would be nice if the device was
 * actually checked to make sure it was there, but at least on the
 * Raspberry PI 3 the SPI pins were not set up in ALT0 mode yet and
 * everything acts like it succeeds.  No errors are ever produced while
 * in that state.
 */
static int
scmdspi_match(device_t parent, cfdata_t match, void *aux)
{
	struct spi_attach_args *sa = aux;
	const bool matchdebug = true;

	if (matchdebug) {
		printf("Trying to match\n");
	}

	return 1;
}

static void
scmdspi_attach(device_t parent, device_t self, void *aux)
{
	struct scmd_sc *sc;
	struct spi_attach_args *sa;
	int error;

	sa = aux;
	sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_sh = sa->sa_handle;
	sc->sc_scmddebug = 0;
	sc->sc_topaddr = 0xff;
	sc->sc_opened = false;
	sc->sc_dying = false;
	sc->sc_func_acquire_bus = &scmdspi_acquire_bus;
	sc->sc_func_release_bus = &scmdspi_release_bus;
	sc->sc_func_read_register = &scmdspi_read_reg;
	sc->sc_func_write_register = &scmdspi_write_reg;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_condmutex, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_dying_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condvar, "scmdspicv");
	cv_init(&sc->sc_cond_dying, "scmdspidc");

	/* configure for 1MHz and SPI mode 0 according to the data sheet */
	error = spi_configure(self, sa->sa_handle, SPI_MODE_0, 1000000);
	if (error) {
		return;
	}

	/* Please note that if the pins are not set up for SPI, the attachment
	 * will work, but it will not figure out that there are slave modules.
	 * It is likely required that a re-enumeration be performed after the pins
	 * are set.  This can be done from userland later.
	 */
	scmd_attach(sc);

	return;
}

/* These really do not do a whole lot, as SPI devices do not seem to work
 * as modules.
 */
static int
scmdspi_detach(device_t self, int flags)
{
	struct scmd_sc *sc;

	sc = device_private(self);

	mutex_enter(&sc->sc_mutex);
	sc->sc_dying = true;
	/* If this is true we are still open, destroy the condvar */
	if (sc->sc_opened) {
		mutex_enter(&sc->sc_dying_mutex);
		DPRINTF(sc, 2, ("%s: Will wait for anything to exit\n",
		    device_xname(sc->sc_dev)));
		/* In the worst case this will time out after 5 seconds.
		 * It really should not take that long for the drain / whatever
		 * to happen
		 */
		cv_timedwait_sig(&sc->sc_cond_dying,
		    &sc->sc_dying_mutex, mstohz(5000));
		mutex_exit(&sc->sc_dying_mutex);
		cv_destroy(&sc->sc_cond_dying);
	}
	cv_destroy(&sc->sc_condvar);
	mutex_exit(&sc->sc_mutex);

	mutex_destroy(&sc->sc_mutex);
	mutex_destroy(&sc->sc_condmutex);

	return 0;
}

int
scmdspi_activate(device_t self, enum devact act)
{
	struct scmd_sc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = true;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}
