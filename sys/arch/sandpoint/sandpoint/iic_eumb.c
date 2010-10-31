/* $NetBSD: iic_eumb.c,v 1.9 2010/10/31 11:08:06 nisimura Exp $ */

/*-
 * Copyright (c) 2010 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iic_eumb.c,v 1.9 2010/10/31 11:08:06 nisimura Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <dev/i2c/i2cvar.h>

#include <sandpoint/sandpoint/eumbvar.h>

struct iic_eumb_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct i2c_controller	sc_i2c;
	kmutex_t		sc_buslock;
	bool			sc_start;
};

static int  iic_eumb_match(struct device *, struct cfdata *, void *);
static void iic_eumb_attach(struct device *, struct device *, void *);

CFATTACH_DECL(iic_eumb, sizeof(struct iic_eumb_softc),
    iic_eumb_match, iic_eumb_attach, NULL, NULL);

static int  motoi2c_acquire_bus(void *, int);
static void motoi2c_release_bus(void *, int);
static int  motoi2c_send_start(void *, int);
static int  motoi2c_send_stop(void *, int);
static int  motoi2c_initiate_xfer(void *, uint16_t, int);
static int  motoi2c_read_byte(void *, uint8_t *, int);
static int  motoi2c_write_byte(void *, uint8_t, int);
static int  motoi2c_waitxferdone(struct iic_eumb_softc *);

struct iic_eumb_softc *iic_eumb;

static struct i2c_controller motoi2c = {
	.ic_acquire_bus = motoi2c_acquire_bus,
	.ic_release_bus = motoi2c_release_bus,
	.ic_send_start	= motoi2c_send_start,
	.ic_send_stop	= motoi2c_send_stop,
	.ic_initiate_xfer = motoi2c_initiate_xfer,
	.ic_read_byte	= motoi2c_read_byte,
	.ic_write_byte	= motoi2c_write_byte,
};

/*
 * This I2C controller seems to share a common design with
 * i.MX/MC9328.  Different names in bit field definition and
 * not suffered from document error.
 */
#define I2CADR	0x0000	/* my own I2C addr to respond for an external master */
#define I2CFDR	0x0004	/* frequency devider */
#define I2CCR	0x0008	/* control */
#define	 CR_MEN   0x80	/* enable this HW */
#define	 CR_MIEN  0x40	/* enable interrupt */
#define	 CR_MSTA  0x20	/* 0->1 activates START, 1->0 makes STOP condition */
#define	 CR_MTX   0x10	/* 1 for Tx, 0 for Rx */
#define	 CR_TXAK  0x08	/* 1 makes no acknowledge when Rx */
#define	 CR_RSTA  0x04	/* generate repeated START condition */
#define I2CSR	0x000c	/* status */
#define	 SR_MCF   0x80	/* 0 means transfer in progress, 1 when completed */
#define	 SR_MBB   0x20	/* 1 before STOP condition is detected */
#define	 SR_MAL   0x10	/* arbitration was lost */
#define	 SR_MIF   0x02	/* indicates data transter completion */
#define	 SR_RXAK  0x01	/* 1 to indicate receive has completed */
#define I2CDR	0x0010	/* data */

#define	I2C_READ(r)	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (r))
#define	I2C_WRITE(r,v)	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (r), (v))
#define I2C_SETCLR(r, s, c) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (r), \
	    (bus_space_read_4(sc->sc_iot, sc->sc_ioh, (r)) | (s)) & ~(c))

static int found;

static int
iic_eumb_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (found == 0);
}

static void
iic_eumb_attach(struct device *parent, struct device *self, void *aux)
{
	struct iic_eumb_softc *sc;
	struct eumb_attach_args *eaa;
	struct i2cbus_attach_args iba;
	bus_space_handle_t ioh;

	sc = (struct iic_eumb_softc *)self;
	eaa = aux;
	found = 1;
	printf("\n");

	bus_space_map(eaa->eumb_bt, 0x3000, 0x20, 0, &ioh);
	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);

	iic_eumb = sc;
	sc->sc_i2c = motoi2c;
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_iot = eaa->eumb_bt;
	sc->sc_ioh = ioh;
	sc->sc_start = false;
	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;

	I2C_WRITE(I2CCR, 0);		/* reset before changing anything */
	I2C_WRITE(I2CFDR, 0x1031);	/* DFFSR=0x10, divider 3072 (0x31) */
	I2C_WRITE(I2CADR, 0x7f << 1);	/* our slave address is 0x7f */
	I2C_WRITE(I2CSR, 0);		/* clear status flags */
	I2C_WRITE(I2CCR, CR_MEN);	/* enable the I2C module */

	config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);

#if 0
	/* we do not use the I2C interrupt yet */
	intr_establish(16 + 16, IST_LEVEL, IPL_BIO, motoic2_intr, sc);
#endif
}

static int
motoi2c_acquire_bus(void *v, int flags)
{
	struct iic_eumb_softc *sc;

	sc = v;
	mutex_enter(&sc->sc_buslock);
	return 0;
}

static void
motoi2c_release_bus(void *v, int flags)
{
	struct iic_eumb_softc *sc;

	sc = v;
	mutex_exit(&sc->sc_buslock);
}

static int
motoi2c_send_start(void *v, int flags)
{
	struct iic_eumb_softc *sc;
	int timo;

	sc = v;

	if (!sc->sc_start && (I2C_READ(I2CSR) & SR_MBB) != 0) {
		/* wait for bus becoming available */
		timo = 100;
		do
			DELAY(10);
		while (--timo > 0 && (I2C_READ(I2CSR) & SR_MBB) != 0);

		if (timo == 0) {
#ifdef DEBUG
			printf("%s: bus is busy\n", __func__);
#endif
			return -1;
		}
	}
	else if (sc->sc_start &&
	    (I2C_READ(I2CSR) & (SR_MIF | SR_MAL)) == SR_MIF) {
		sc->sc_start = false;
#ifdef DEBUG
		printf("%s: lost the bus\n", __func__);
#endif
		return -1;
	}

	/* reset interrupt and arbitration-lost flags */
	I2C_SETCLR(I2CSR, 0, SR_MIF | SR_MAL);

	if (!sc->sc_start) {
		/* generate start condition */
		I2C_SETCLR(I2CCR, CR_MSTA | CR_MTX, CR_TXAK | CR_RSTA);
		sc->sc_start = true;
	} else	/* repeated start, we still own the bus */
		I2C_SETCLR(I2CCR, CR_RSTA | CR_MTX, CR_TXAK);

	return 0;
}

static int
motoi2c_send_stop(void *v, int flags)
{
	struct iic_eumb_softc *sc;

	sc = v;
	I2C_SETCLR(I2CCR, 0, CR_MSTA | CR_RSTA | CR_TXAK);
	sc->sc_start = false;
	DELAY(100);
	return 0;
}

static int
motoi2c_initiate_xfer(void *v, i2c_addr_t addr, int flags)
{
	struct iic_eumb_softc *sc;

	sc = v;

	/* start condition */
	if (motoi2c_send_start(v, flags) != 0)
		return -1;

	/* send target address and transfer direction */
	I2C_WRITE(I2CDR, (addr << 1) | ((flags & I2C_F_READ) ? 1 : 0));

	if (motoi2c_waitxferdone(sc) == 0) {
		if (flags & I2C_F_READ) {
			/* clear TX mode */
			I2C_SETCLR(I2CCR, 0, CR_MTX);
			/*
			 * A dummy read is required to start with
			 * receiving bytes.
			 */
			(void)I2C_READ(I2CDR);
		}
		if (flags & I2C_F_STOP)
			return motoi2c_send_stop(v, flags);
		return 0;
	}
	return -1;
}

static int
motoi2c_read_byte(void *v, uint8_t *bytep, int flags)
{
	struct iic_eumb_softc *sc;

	sc = v;

	/* wait for next byte */
	if (motoi2c_waitxferdone(sc) != 0)
		return -1;

	/* no TX-acknowledge for next byte */
	if (flags & I2C_F_LAST)
		I2C_SETCLR(I2CCR, CR_TXAK, 0);

	*bytep = I2C_READ(I2CDR);

	/*
	 * To make the TXAK take effect we have to receive another
	 * byte, which will be discarded.
	 */
	if (flags & I2C_F_LAST) {
		(void)motoi2c_waitxferdone(sc);	/* ignore RXAK flag */
		if (flags & I2C_F_STOP)
			(void)motoi2c_send_stop(v, flags);
		(void)I2C_READ(I2CDR);
	} else if (flags & I2C_F_STOP)
		return motoi2c_send_stop(v, flags);

	return 0;
}

static int
motoi2c_write_byte(void *v, uint8_t byte, int flags)
{
	struct iic_eumb_softc *sc;

	sc = v;
	I2C_WRITE(I2CDR, byte);
	DELAY(10);

	if (motoi2c_waitxferdone(sc) != 0)
		return -1;

	if (flags & I2C_F_STOP)
		return motoi2c_send_stop(v, flags);

	return 0;
}

/* busy waiting for byte data transfer completion */
static int
motoi2c_waitxferdone(struct iic_eumb_softc *sc)
{
	uint32_t sr;
	int timo;

	timo = 1000;
	do {
		sr = I2C_READ(I2CSR);
		DELAY(10);
	} while (--timo > 0 && (sr & SR_MIF) == 0);

#ifdef DEBUG
	if (timo == 0)
		printf("%s: timeout\n", __func__);
	if (sr & SR_RXAK)
		printf("%s: missing rx ack\n", __func__);
#endif
	I2C_SETCLR(I2CSR, 0, sr);
	return (timo == 0 || (sr & SR_RXAK) != 0) ? -1 : 0;
}
