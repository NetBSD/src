/* $NetBSD: iic_eumb.c,v 1.6.2.1 2009/05/13 17:18:16 jym Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
__KERNEL_RCSID(0, "$NetBSD: iic_eumb.c,v 1.6.2.1 2009/05/13 17:18:16 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <dev/i2c/i2cvar.h>

#include <sandpoint/sandpoint/eumbvar.h>

void iic_bootstrap_init(void);
int iic_bootstrap_read(int, int, uint8_t *, size_t);

static int  iic_eumb_match(struct device *, struct cfdata *, void *);
static void iic_eumb_attach(struct device *, struct device *, void *);

struct iic_eumb_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct i2c_controller	sc_i2c;
	kmutex_t		sc_buslock;
};

CFATTACH_DECL(iic_eumb, sizeof(struct iic_eumb_softc),
    iic_eumb_match, iic_eumb_attach, NULL, NULL);

static int motoi2c_acquire_bus(void *, int);
static void motoi2c_release_bus(void *, int);
static int motoi2c_send_start(void *, int);
static int motoi2c_send_stop(void *, int);
static int motoi2c_initiate_xfer(void *, uint16_t, int);
static int motoi2c_read_byte(void *, uint8_t *, int);
static int motoi2c_write_byte(void *, uint8_t, int);
static void waitxferdone(int);

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

#define	CSR_READ(r)	in8rb(0xfc003000 + (r))
#define	CSR_WRITE(r,v)	out8rb(0xfc003000 + (r), (v))
#define	CSR_WRITE4(r,v)	out32rb(0xfc003000 + (r), (v))

static int found;

static int
iic_eumb_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (found == 0);
}

static void
iic_eumb_attach(struct device *parent, struct device *self, void *aux)
{
	struct iic_eumb_softc *sc = (void *)self;
	struct eumb_attach_args *eaa = aux;
	struct i2cbus_attach_args iba;
	bus_space_handle_t ioh;

	found = 1;
	printf("\n");

	bus_space_map(eaa->eumb_bt, 0x3000, 0x20, 0, &ioh);
	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_i2c = motoi2c;
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_iot = eaa->eumb_bt;
	sc->sc_ioh = ioh;
	iba.iba_tag = &sc->sc_i2c;

	iic_bootstrap_init();
#if 0
	/* not yet */
	config_found_ia(&sc->sc_dev, "i2cbus", &iba, iicbus_print);

	/* we never attempt to use I2C interrupt */
	intr_establish(16 + 16, IST_LEVEL, IPL_BIO, iic_intr, sc);
#endif
}

void
iic_bootstrap_init(void)
{

	CSR_WRITE(I2CCR, 0);
	CSR_WRITE4(I2CFDR, 0x1031); /* XXX magic XXX */
	CSR_WRITE(I2CADR, 0);
	CSR_WRITE(I2CSR, 0);
	CSR_WRITE(I2CCR, CR_MEN);
}

int
iic_bootstrap_read(int i2caddr, int offset, uint8_t *rvp, size_t len)
{
	i2c_addr_t addr;
	uint8_t cmdbuf[1];

	if (motoi2c_acquire_bus(&motoi2c, I2C_F_POLL) != 0)
		return -1;
	while (len) {
		addr = i2caddr + (offset >> 8);
		cmdbuf[0] = offset & 0xff;
		if (iic_exec(&motoi2c, I2C_OP_READ_WITH_STOP, addr,
			     cmdbuf, 1, rvp, 1, I2C_F_POLL)) {
			motoi2c_release_bus(&motoi2c, I2C_F_POLL);
			return -1;
		}
		len--;
		rvp++;
		offset++;
	}
	motoi2c_release_bus(&motoi2c, I2C_F_POLL);
	return 0;	
}

static int
motoi2c_acquire_bus(void *v, int flags)
{
	struct iic_eumb_softc *sc = v;

	if (flags & I2C_F_POLL)
		return 0;
	mutex_enter(&sc->sc_buslock);
	return 0;
}

static void
motoi2c_release_bus(void *v, int flags)
{
	struct iic_eumb_softc *sc = v;

	if (flags & I2C_F_POLL)
		return;
	mutex_exit(&sc->sc_buslock);
}

static int
motoi2c_send_start(void *v, int flags)
{
	int loop = 10;

	CSR_WRITE(I2CSR, 0);
	CSR_WRITE(I2CCR, CR_MEN);
	do {
		DELAY(1);
	} while (--loop > 0 && (CSR_READ(I2CSR) & SR_MBB));
	return 0;
}

static int
motoi2c_send_stop(void *v, int flags)
{

	CSR_WRITE(I2CCR, CR_MEN);
	return 0;
}

static int
motoi2c_initiate_xfer(void *v, i2c_addr_t addr, int flags)
{
	int rd_req;

	rd_req = !!(flags & I2C_F_READ);
	CSR_WRITE(I2CCR, CR_MIEN | CR_MEN | CR_MSTA | CR_MTX);
	CSR_WRITE(I2CDR, (addr << 1) | rd_req);
	if (flags & I2C_F_STOP)
		CSR_WRITE(I2CCR, CR_MIEN | CR_MEN | CR_TXAK);
	waitxferdone(SR_MIF);
	return 0;
}

static int
motoi2c_read_byte(void *v, uint8_t *bytep, int flags)
{
	int last_byte, send_stop;

	last_byte = (flags & I2C_F_LAST) != 0;
	send_stop = (flags & I2C_F_STOP) != 0;
	if (last_byte)
		CSR_WRITE(I2CCR, CR_MIEN | CR_MEN | CR_MSTA | CR_TXAK);
	if (send_stop)
		CSR_WRITE(I2CCR, CR_MIEN | CR_MEN | CR_TXAK);
	waitxferdone(SR_MIF);
	*bytep = CSR_READ(I2CDR);
	return 0;
}

static int
motoi2c_write_byte(void *v, uint8_t byte, int flags)
{
	int send_stop;

	send_stop = (flags & I2C_F_STOP) != 0;
	if (send_stop)
		CSR_WRITE(I2CCR, CR_MEN);
	CSR_WRITE(I2CDR, byte);
	waitxferdone(SR_MIF);
	return 0;
}

/* busy waiting for byte data transfer completion */
static void
waitxferdone(int cond)
{
	int timo, sr;

	timo = 100;
	do {
		DELAY(1);
		sr = CSR_READ(I2CSR);
	} while (--timo > 0 && (sr & cond) == 0);
	sr &= ~cond;
	CSR_WRITE(I2CSR, sr);
}
