/*	$NetBSD: iic.c,v 1.1.6.2 2002/06/23 17:33:46 jdolecek Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * iic.c
 *
 * Routines to communicate with IIC devices
 *
 * Created      : 13/10/94
 *
 * Based of kate/display/iiccontrol.c
 */

#include <sys/param.h>

__RCSID("$NetBSD: iic.c,v 1.1.6.2 2002/06/23 17:33:46 jdolecek Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <acorn26/iobus/iocreg.h>
#include <acorn26/iobus/iocvar.h>
#include <acorn26/ioc/iic.h>

#include "locators.h"

struct iic_softc {
	struct device sc_dev;
};

/* Autoconfiguration glue */
static int  iic_match (struct device *, struct cfdata *, void *);
static void iic_attach(struct device *, struct device *, void *);
static int  iicsearch(struct device *, struct cfdata *, void *);

struct cfattach iic_ca = {
	sizeof(struct iic_softc), iic_match, iic_attach
};

/* Local function prototypes */

static int iic_getack(struct device *);
static void iic_write_bit(struct device *, int);
static int iic_write_byte(struct device *, int);
static int iic_read_byte(struct device *);
static void iic_start_bit(struct device *);
static void iic_stop_bit(struct device *);

static int iicprint(void *, const char *);

/* Functions that do the bit twiddling */
static int  iic_get_state(struct device *);
static void iic_set_state_and_ack(struct device *, int, int);
static void iic_set_state(struct device *, int, int);

/*
 * iic device probe function
 */

/* ARGSUSED */
static int
iic_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return(1);
}

/*
 * iic device attach function
 *
 * Initialise the softc structure and do a search for children
 */

static void
iic_attach(struct device *parent, struct device *self, void *aux)
{

	printf("\n");
	config_search(iicsearch, self, NULL);
}

/*
 * Main entry to IIC driver.
 */

int
iic_control(struct device *self, int address, u_char *buffer, int count)
{
	int loop;

	/* Send the start bit */

	iic_start_bit(self);

	/* Send the address */

	if (!iic_write_byte(self, address)) {
		iic_stop_bit(self);
		return -1;
	}

	/* Read or write the data as required */

	if ((address & 1) == 0) {
		/* Write bytes */
		for (loop = 0; loop < count; ++loop) {
			if (!iic_write_byte(self, buffer[loop])) {
				iic_stop_bit(self);
				return -1;
			}
		}
	}
	else {
		/* Read bytes */
		for (loop = 0; loop < count; ++loop) {
			buffer[loop] = iic_read_byte(self);

			/* Send final acknowledge */

			if (loop == (count - 1))
				iic_write_bit(self, 1);
			else
				iic_write_bit(self, 0);
		}
	}

	/* Send stop bit */

	iic_stop_bit(self);

	return 0;
}


static int
iic_getack(struct device *self)
{
	int ack;

	iic_set_state(self, 1, 0);
	delay(5);
	iic_set_state_and_ack(self, 1, 1);
	ack = iic_get_state(self);
	delay(4);
	iic_set_state(self, 1, 0);
	delay(5); /* Clock low time (4.7 us) */

	return (ack & 1) == 0;
}

/*
 * Put the IIC bus lines in the given state (MD)
 */
static void
iic_set_state(struct device *self, int sda, int scl)
{
	int bits;

	bits = 0;
	if (sda) bits |= IOC_CTL_SDA;
	if (scl) bits |= IOC_CTL_SCL;
	ioc_ctl_write(self->dv_parent, bits, IOC_CTL_SDA | IOC_CTL_SCL);
}

/*
 * Put the IIC bus lines in a given state and wait for the SCL line
 * to actually do what we asked (so the slave is ready).
 */
static void
iic_set_state_and_ack(struct device *self, int sda, int scl)
{

	iic_set_state(self, sda, scl);
	while ((ioc_ctl_read(self->dv_parent) & IOC_CTL_SCL) == 0)
		continue;
}

/*
 * Read state of IIC data line
 */
static int
iic_get_state(struct device *self)
{

	return (ioc_ctl_read(self->dv_parent) & IOC_CTL_SDA) != 0;
}

static void
iic_write_bit(struct device *self, int bit)
{

	iic_set_state(self, bit, 0);
	delay(1); /* Data setup time (250 ns) */
	iic_set_state_and_ack(self, bit, 1);
	delay(4); /* Clock high time (4.0 us) */
	iic_set_state(self, bit, 0);
	delay(5); /* Clock low time (4.7 us) */
}


static int
iic_write_byte(struct device *self, int value)
{
	int loop;
	int bit;

	for (loop = 0x80; loop != 0; loop = loop >> 1) {
		bit = ((value & loop) != 0);
		iic_write_bit(self, bit);
	}

	return(iic_getack(self));
}


static int
iic_read_byte(struct device *self)
{
	int loop;
	u_char byte;

	iic_set_state(self, 1, 0);

	byte = 0;

	for (loop = 0; loop < 8; ++loop) {
		iic_set_state_and_ack(self, 1, 1);
		delay(4); /* Clock high time (4.0 us) */
		byte = (byte << 1) + (iic_get_state(self) & 1);
		iic_set_state(self, 1, 0);
		delay(5); /* Clock low time (4.7 us) */
	}

	return(byte);
}


static void
iic_start_bit(struct device *self)
{

	iic_set_state(self, 1, 1);
	delay(5); /* Bus free time (4.7 us) */
	iic_set_state(self, 0, 1);
	delay(4); /* Start hold time (4.0 us) */
	iic_set_state(self, 0, 0);
	delay(5); /* Clock low time (4.7 us) */
}


static void
iic_stop_bit(struct device *self)
{

	iic_set_state(self, 0, 1);
	delay(4); /* Stop setup time (4.0 us) */
	iic_set_state(self, 1, 1);
}

/*
 * int iicprint(void *aux, const char *name)
 *
 * print function for child device configuration
 */

static int
iicprint(void *aux, const char *name)
{
	struct iicbus_attach_args *iba = aux;

	if (!name) {
		if (iba->ib_addr)
			printf(" addr 0x%02x", iba->ib_addr);
	}

	/* XXXX print flags */
	return (UNCONF);
}

/*
 * iic search function
 *
 * search for devices that are children of the iic device
 * fill out the attach arguments and call the probe and
 * attach function (as required).
 *
 * Note: since the offsets of the devices need to be specified in the
 * config file we ignore the FSTAT_STAR.
 */

static int
iicsearch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct iic_softc *sc = (struct iic_softc *)parent;
	struct iicbus_attach_args iba;
	int tryagain;

	do {
		iba.ib_iic_softc = sc;
		iba.ib_addr = cf->cf_loc[IICCF_ADDR];
		iba.ib_aux = NULL;

		tryagain = 0;
		if ((*cf->cf_attach->ca_match)(parent, cf, &iba) > 0) {
			config_attach(parent, cf, &iba, iicprint);
/*			tryagain = (cf->cf_fstate == FSTATE_STAR);*/
		}
	} while (tryagain);

	return (0);
}
