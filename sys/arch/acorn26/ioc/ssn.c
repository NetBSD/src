/*	$NetBSD: ssn.c,v 1.2.6.2 2002/06/23 17:33:47 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>

__RCSID("$NetBSD: ssn.c,v 1.2.6.2 2002/06/23 17:33:47 jdolecek Exp $");

#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <acorn26/iobus/iocreg.h>
#include <acorn26/iobus/iocvar.h>

#include <dev/ic/ds.h>

struct ssn_softc {
	struct device sc_dev;
	struct ds_handle sc_dsh;
	struct device *sc_ioc;
	int sc_timebase;
};

static int ssn_match(struct device *, struct cfdata *, void *);
static void ssn_attach(struct device *, struct device *, void *);

struct cfattach ssn_ca = {
	sizeof(struct ssn_softc), ssn_match, ssn_attach
};

static int ds_ioc_read_bit(void *);
static void ds_ioc_write_bit(void *, int);
static void ds_ioc_reset(void *);

static int ds_crc(const u_int8_t *data, size_t len);

static int
ssn_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

static void
ssn_attach(struct device *parent, struct device *self, void *aux)
{
	struct ssn_softc *sc = (void *)self;
	int i;
	u_int8_t rombuf[8];

	sc->sc_ioc = parent;
	sc->sc_dsh.ds_read_bit = ds_ioc_read_bit;
	sc->sc_dsh.ds_write_bit = ds_ioc_write_bit;
	sc->sc_dsh.ds_reset = ds_ioc_reset;
	sc->sc_dsh.ds_hw_handle = sc;

	sc->sc_dsh.ds_reset(sc->sc_dsh.ds_hw_handle);

	ds_write_byte(&sc->sc_dsh, DS_ROM_READ);
	for (i=0; i<8; ++i) 
		rombuf[i] = ds_read_byte(&sc->sc_dsh);

	printf(": ROM %02x %02x%02x%02x%02x%02x%02x %02x CRC %02x\n",
	    rombuf[7], rombuf[6], rombuf[5], rombuf[4], 
	    rombuf[3], rombuf[2], rombuf[1], rombuf[0], ds_crc(rombuf, 8));

}

static int
ds_ioc_read_bit(void *cookie)
{
	struct ssn_softc *sc = cookie;
	int result;

	/*
	 * Read is 1--15us low, >60us high.
	 * Sample 15us after falling edge.
	 */
	ioc_ctl_write(sc->sc_ioc, 0, IOC_CTL_SSN);
	ioc_ctl_write(sc->sc_ioc, IOC_CTL_SSN, IOC_CTL_SSN);
	result = (ioc_ctl_read(sc->sc_ioc) & IOC_CTL_SSN) != 0;
	DELAY(60);
	return result;
}

static void
ds_ioc_write_bit(void *cookie, int bit)
{
	struct ssn_softc *sc = cookie;

	if (!bit) {
		/* Write 0 is 60--120us low, >1us high. */
		ioc_ctl_write(sc->sc_ioc, 0, IOC_CTL_SSN);
		DELAY(60);
		ioc_ctl_write(sc->sc_ioc, IOC_CTL_SSN, IOC_CTL_SSN);
		DELAY(1);
	} else {
		/* Write 1 is 1--15us low, >60us high. */
		ioc_ctl_write(sc->sc_ioc, 0, IOC_CTL_SSN);
		ioc_ctl_write(sc->sc_ioc, IOC_CTL_SSN, IOC_CTL_SSN);
		DELAY(60);
	}
}

static void
ds_ioc_reset(void *cookie)
{
	struct ssn_softc *sc = cookie;
	int t_pdh, t_pdl;

	/* Reset pulse is >480us low, then >480us high. */
	ioc_ctl_write(sc->sc_ioc, 0, IOC_CTL_SSN);
	DELAY(480);
	ioc_ctl_write(sc->sc_ioc, IOC_CTL_SSN, IOC_CTL_SSN);
       	DELAY(60);
	if ((ioc_ctl_read(sc->sc_ioc) & IOC_CTL_SSN) != 0) {
		printf(": No presence pulse\n");
		return;
	}
	DELAY(420);
	/* Reset again, and time it this time. */
	ioc_ctl_write(sc->sc_ioc, 0, IOC_CTL_SSN);
	DELAY(480);
	ioc_ctl_write(sc->sc_ioc, IOC_CTL_SSN, IOC_CTL_SSN);
	t_pdh = 0;
	while ((ioc_ctl_read(sc->sc_ioc) & IOC_CTL_SSN) != 0)
		t_pdh++;
	t_pdl = 0;
	while ((ioc_ctl_read(sc->sc_ioc) & IOC_CTL_SSN) == 0)
		t_pdl++;
	DELAY(480);
	printf(": t_PDH = %d, t_PDL = %d", t_pdh, t_pdl);
	sc->sc_timebase = (t_pdh + t_pdl) / 5 + 1;
}

#define DS_CRC_POLY 0x8c

static int
ds_crc(const u_int8_t *buf, size_t len)
{
	u_int8_t c, crc, carry;
	size_t i, j;

	crc = 0;

	for (i = 0; i < len; i++) {
		c = buf[i];
		for (j = 0; j < 8; j++) {
			carry = (crc ^ c) & 0x01;
			crc >>= 1;
			c >>= 1;
			if (carry)
				crc ^= DS_CRC_POLY;
		}
	}
	return (crc);
}
