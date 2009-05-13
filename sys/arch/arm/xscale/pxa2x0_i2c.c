/*	$NetBSD: pxa2x0_i2c.c,v 1.3.34.1 2009/05/13 17:16:18 jym Exp $	*/
/*	$OpenBSD: pxa2x0_i2c.c,v 1.2 2005/05/26 03:52:07 pascoe Exp $	*/

/*
 * Copyright (c) 2005 Christopher Pascoe <pascoe@openbsd.org>
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
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_i2c.c,v 1.3.34.1 2009/05/13 17:16:18 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>
#include <arm/xscale/pxa2x0_gpio.h>

#define I2C_RETRY_COUNT	10

int
pxa2x0_i2c_attach_sub(struct pxa2x0_i2c_softc *sc)
{

	if (bus_space_map(sc->sc_iot, PXA2X0_I2C_BASE,
	    PXA2X0_I2C_SIZE, 0, &sc->sc_ioh)) {
		sc->sc_size = 0;
		return EIO;
	}
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);

	pxa2x0_i2c_init(sc);

	return 0;
}

int
pxa2x0_i2c_detach_sub(struct pxa2x0_i2c_softc *sc)
{

	if (sc->sc_size) {
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_size);
		sc->sc_size = 0;
	}
	pxa2x0_clkman_config(CKEN_I2C, 0);

	return 0;
}

void
pxa2x0_i2c_init(struct pxa2x0_i2c_softc *sc)
{

	pxa2x0_i2c_open(sc);
	pxa2x0_i2c_close(sc);
}

void
pxa2x0_i2c_open(struct pxa2x0_i2c_softc *sc)
{

	/* Enable the clock to the standard I2C unit. */
	pxa2x0_clkman_config(CKEN_I2C, 1);
}

void
pxa2x0_i2c_close(struct pxa2x0_i2c_softc *sc)
{

	/* Reset and disable the standard I2C unit. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, I2C_ISAR, 0);
	delay(1);
	pxa2x0_clkman_config(CKEN_I2C, 0);
}

int
pxa2x0_i2c_read(struct pxa2x0_i2c_softc *sc, u_char slave, u_char *valuep)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	int tries = I2C_RETRY_COUNT;
	uint32_t rv;

retry:
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE | ISR_IRF);
	delay(1);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	/* Write slave device address. */
	bus_space_write_4(iot, ioh, I2C_IDBR, (slave<<1) | 0x1);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_START);

	/* Read data value. */
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv |
	    (ICR_STOP | ICR_ACKNAK));
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_IRF) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_IRF);

	rv = bus_space_read_4(iot, ioh, I2C_IDBR);
	*valuep = (u_char)rv;
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~(ICR_STOP | ICR_ACKNAK));

	return 0;

err:
	if (tries-- >= 0)
		goto retry;

	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE | ISR_IRF);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	return EIO;
}

int
pxa2x0_i2c_write(struct pxa2x0_i2c_softc *sc, u_char slave, u_char value)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	int tries = I2C_RETRY_COUNT;
	uint32_t rv;

retry:
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);
	delay(1);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	/* Write slave device address. */
	bus_space_write_4(iot, ioh, I2C_IDBR, (slave<<1));
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}
	if ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ACKNAK) != 0)
		goto err;

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	/* Write data. */
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_STOP);
	bus_space_write_4(iot, ioh, I2C_IDBR, value);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}
	if ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ACKNAK) != 0)
		goto err;

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);

	return 0;

err:
	if (tries-- >= 0)
		goto retry;

	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	return EIO;
}

/*
 * XXX The quick_{read,write} opertions are untested!
 */
int
pxa2x0_i2c_quick(struct pxa2x0_i2c_softc *sc, u_char slave, u_char rw)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	int tries = I2C_RETRY_COUNT;
	uint32_t rv;

retry:
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);
	delay(1);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	/* Write slave device address. */
	bus_space_write_4(iot, ioh, I2C_IDBR, (slave<<1) | (rw & 1));
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_STOP);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}
	if ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ACKNAK) != 0)
		goto err;

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);

	return 0;

err:
	if (tries-- >= 0)
		goto retry;

	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	return EIO;
}

int
pxa2x0_i2c_write_2(struct pxa2x0_i2c_softc *sc, u_char slave, u_short value)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	int tries = I2C_RETRY_COUNT;
	uint32_t rv;

retry:
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);
	delay(1);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	/* Write slave device address. */
	bus_space_write_4(iot, ioh, I2C_IDBR, (slave<<1));
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}
	if ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ACKNAK) != 0)
		goto err;

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	/* Write upper 8 bits of data. */
	bus_space_write_4(iot, ioh, I2C_IDBR, (value >> 8) & 0xff);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}
	if ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ACKNAK) != 0)
		goto err;

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	/* Write lower 8 bits of data. */
	bus_space_write_4(iot, ioh, I2C_IDBR, value & 0xff);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_START);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_STOP);
	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv | ICR_TB);

	timeout = 10000;
	while ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ITE) == 0) {
		if (timeout-- == 0)
			goto err;
		delay(1);
	}
	if ((bus_space_read_4(iot, ioh, I2C_ISR) & ISR_ACKNAK) != 0)
		goto err;

	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);

	rv = bus_space_read_4(iot, ioh, I2C_ICR);
	bus_space_write_4(iot, ioh, I2C_ICR, rv & ~ICR_STOP);

	return 0;

err:
	if (tries-- >= 0)
		goto retry;

	bus_space_write_4(iot, ioh, I2C_ICR, ICR_UR);
	bus_space_write_4(iot, ioh, I2C_ISAR, 0x00);
	bus_space_write_4(iot, ioh, I2C_ISR, ISR_ITE);
	bus_space_write_4(iot, ioh, I2C_ICR, ICR_IUE | ICR_SCLE);

	return EIO;
}
