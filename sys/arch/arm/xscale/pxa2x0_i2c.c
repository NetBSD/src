/*	$NetBSD: pxa2x0_i2c.c,v 1.4.10.1 2011/06/23 14:19:01 cherry Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_i2c.c,v 1.4.10.1 2011/06/23 14:19:01 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/i2c/i2cvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>

#ifdef PXAIIC_DEBUG
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)	do { } while (/*CONSTCOND*/0)
#endif

#define I2C_RETRY_COUNT	10

int
pxa2x0_i2c_attach_sub(struct pxa2x0_i2c_softc *sc)
{
	int error;

	error = bus_space_map(sc->sc_iot, PXA2X0_I2C_BASE, sc->sc_size, 0,
	    &sc->sc_ioh);
	if (error) {
		aprint_error_dev(sc->sc_dev, "unable to map register\n");
		sc->sc_size = 0;
		return error;
	}

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_size,
	    BUS_SPACE_BARRIER_READ|BUS_SPACE_BARRIER_WRITE);

	sc->sc_icr = ICR_GCD | ICR_SCLE | ICR_IUE;
#if 0
	if (ISSET(sc->sc_flags, PI2CF_ENABLE_INTR))
		sc->sc_icr |= ICR_BEIE | ICR_DRFIE | ICR_ITEIE;
#endif
	if (ISSET(sc->sc_flags, PI2CF_FAST_MODE))
		sc->sc_icr |= ICR_FM;

	pxa2x0_i2c_init(sc);

	return 0;
}

int
pxa2x0_i2c_detach_sub(struct pxa2x0_i2c_softc *sc)
{

	if (sc->sc_size != 0) {
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

/* ----------------------------------------------------------------------------
 * for i2c_controller
 */

#define	CSR_READ_4(sc,r)	bus_space_read_4(sc->sc_iot, sc->sc_ioh, r)
#define	CSR_WRITE_4(sc,r,v)	bus_space_write_4(sc->sc_iot, sc->sc_ioh, r, v)

#define	ISR_ALL			(ISR_RWM | ISR_ACKNAK | ISR_UE | ISR_IBB \
				 | ISR_SSD | ISR_ALD | ISR_ITE | ISR_IRF \
				 | ISR_GCAD | ISR_SAD | ISR_BED)

#define	I2C_TIMEOUT		100	/* protocol timeout, in uSecs */

void
pxa2x0_i2c_reset(struct pxa2x0_i2c_softc *sc)
{

	CSR_WRITE_4(sc, I2C_ICR, ICR_UR);
	CSR_WRITE_4(sc, I2C_ISAR, 0);
	CSR_WRITE_4(sc, I2C_ISR, ISR_ALL);
	while (CSR_READ_4(sc, I2C_ICR) & ~ICR_UR)
		continue;
	CSR_WRITE_4(sc, I2C_ICR, sc->sc_icr);
}

int
pxa2x0_i2c_wait(struct pxa2x0_i2c_softc *sc, int bit, int flags)
{
	uint32_t isr;
	int error;
	int i;

	for (i = I2C_TIMEOUT; i >= 0; --i) {
		isr = CSR_READ_4(sc, I2C_ISR);
		if (isr & (bit | ISR_BED))
			break;
		delay(1);
	}

	if (isr & (ISR_BED | (bit & ISR_ALD)))
		error = EIO;
	else if (isr & (bit & ~ISR_ALD))
		error = 0;
	else
		error = ETIMEDOUT;

	CSR_WRITE_4(sc, I2C_ISR, isr);

	return error;
}

int
pxa2x0_i2c_send_start(struct pxa2x0_i2c_softc *sc, int flags)
{

	CSR_WRITE_4(sc, I2C_ICR, sc->sc_icr | ICR_START);
	delay(I2C_TIMEOUT);
	return 0;
}

int
pxa2x0_i2c_send_stop(struct pxa2x0_i2c_softc *sc, int flags)
{

	CSR_WRITE_4(sc, I2C_ICR, sc->sc_icr | ICR_STOP);
	delay(I2C_TIMEOUT);
	return 0;
}

int
pxa2x0_i2c_initiate_xfer(struct pxa2x0_i2c_softc *sc, uint16_t addr, int flags)
{
	int rd_req = (flags & I2C_F_READ) ? 1 : 0;
	int error;

	if ((addr & ~0x7f) != 0) {
		error = EINVAL;
		goto error;
	}

	CSR_WRITE_4(sc, I2C_IDBR, (addr << 1) | rd_req);
	CSR_WRITE_4(sc, I2C_ICR, sc->sc_icr | ICR_START | ICR_TB);

	error = pxa2x0_i2c_wait(sc, ISR_ITE, flags);
error:
	if (error) {
		DPRINTF(("%s: failed to initiate %s xfer (error=%d)\n",
		    device_xname(sc->sc_dev),
		    rd_req ? "read" : "write", error));
		return error;
	}
	return 0;
}

int
pxa2x0_i2c_read_byte(struct pxa2x0_i2c_softc *sc, uint8_t *bytep, int flags)
{
	int last_byte = flags & I2C_F_LAST;
	int send_stop = flags & I2C_F_STOP;
	int error;

	CSR_WRITE_4(sc, I2C_ICR, sc->sc_icr | ICR_TB
	    | (last_byte ? ICR_ACKNAK : 0) | (send_stop ? ICR_STOP : 0));
	error = pxa2x0_i2c_wait(sc, ISR_IRF | ISR_ALD, flags);
	if (error) {
		DPRINTF(("%s: read byte failed\n", device_xname(sc->sc_dev)));
		return error;
	}

	*bytep = CSR_READ_4(sc, I2C_IDBR);
	return 0;
}

int
pxa2x0_i2c_write_byte(struct pxa2x0_i2c_softc *sc, uint8_t byte, int flags)
{
	int send_stop = flags & I2C_F_STOP;
	int error;

	CSR_WRITE_4(sc, I2C_IDBR, byte);
	CSR_WRITE_4(sc, I2C_ICR, sc->sc_icr | ICR_TB
	    | (send_stop ? ICR_STOP : 0));
	error = pxa2x0_i2c_wait(sc, ISR_ITE | ISR_ALD, flags);
	if (error) {
		DPRINTF(("%s: write byte failed\n", device_xname(sc->sc_dev)));
		return error;
	}
	return 0;
}
