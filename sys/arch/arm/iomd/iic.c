/*	$NetBSD: iic.c,v 1.8 2003/07/15 00:24:44 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iic.c,v 1.8 2003/07/15 00:24:44 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/event.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arm/cpufunc.h>

#include <arm/iomd/iic.h>
#include <arm/iomd/iicvar.h>

#include "locators.h"

/* Local function prototypes */

static int iic_getack		__P((void));
static void iic_write_bit	__P((int bit));
static int iic_write_byte	__P((u_char value));
static u_char iic_read_byte	__P((void));
static void iic_start_bit	__P((void));
static void iic_stop_bit	__P((void));

static int  iicprint  __P((void *aux, const char *name));

/* External functions that do the bit twiddling */
extern int  iic_getstate		__P((void));
extern void iic_set_state_and_ack	__P((int, int));
extern void iic_set_state		__P((int, int));
extern void iic_delay			__P((int));

extern struct cfdriver iic_cd;

dev_type_open(iicopen);
dev_type_close(iicclose);

const struct cdevsw iic_cdevsw = {
	iicopen, iicclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

/*
 * Main entry to IIC driver.
 */

int
iic_control(address, buffer, count)
	u_char address;
	u_char *buffer;
	int count;
{
	int loop;

	/* Send the start bit */

	iic_start_bit();

	/* Send the address */

	if (!iic_write_byte(address)) {
		iic_stop_bit();
		return(-1);
	}

	/* Read or write the data as required */

	if ((address & 1) == 0) {
		/* Write bytes */
		for (loop = 0; loop < count; ++loop) {
			if (!iic_write_byte(buffer[loop])) {
				iic_stop_bit();
				return(-1);
			}
		}
	}
	else {
		/* Read bytes */
		for (loop = 0; loop < count; ++loop) {
			buffer[loop] = iic_read_byte();

			/* Send final acknowledge */

			if (loop == (count - 1))
				iic_write_bit(1);
			else
				iic_write_bit(0);
		}
	}

	/* Send stop bit */

	iic_stop_bit();

	return(0);
}


static int
iic_getack()
{
	u_int oldirqstate;
	int ack;

	iic_set_state(1, 0);
	oldirqstate = disable_interrupts(I32_bit);
	iic_set_state_and_ack(1, 1);
	ack = iic_getstate();
	iic_set_state(1, 0);
	restore_interrupts(oldirqstate);

	return((ack & 1) == 0);
}


static void
iic_write_bit(bit)
	int bit;
{
	u_int oldirqstate;

	iic_set_state(bit, 0);
	oldirqstate = disable_interrupts(I32_bit);
	iic_set_state_and_ack(bit, 1);
	iic_set_state(bit, 0);
	restore_interrupts(oldirqstate);
}


static int
iic_write_byte(value)
	u_char value;
{
	int loop;
	int bit;

	for (loop = 0x80; loop != 0; loop = loop >> 1) {
		bit = ((value & loop) != 0);
		iic_write_bit(bit);
	}

	return(iic_getack());
}


static u_char
iic_read_byte()
{
	int loop;
	u_char byte;
	u_int oldirqstate;

	iic_set_state(1,0);

	byte = 0;

	for (loop = 0; loop < 8; ++loop) {
		oldirqstate = disable_interrupts(I32_bit);
		iic_set_state_and_ack(1, 1);
		byte = (byte << 1) + (iic_getstate() & 1);
		iic_set_state(1, 0);
		restore_interrupts(oldirqstate);
	}

	return(byte);
}


static void
iic_start_bit()
{
	iic_set_state(1, 1);
	iic_set_state(0, 1);
	iic_delay(10);
	iic_set_state(0, 0);
}


static void
iic_stop_bit()
{
	iic_set_state(0, 1);
	iic_set_state(1, 1);
}

/* driver structures */

/*
 * int iicprint(void *aux, const char *name)
 *
 * print function for child device configuration
 */

static int
iicprint(aux, name)
	void *aux;
	const char *name;
{
	struct iicbus_attach_args *iba = aux;

	if (!name) {
		if (iba->ib_addr)
			aprint_normal(" addr 0x%02x", iba->ib_addr);
	}

	/* XXXX print flags */
	return (QUIET);
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

int
iicsearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct iic_softc *sc = (struct iic_softc *)parent;
	struct iicbus_attach_args iba;
	int tryagain;

	do {
		iba.ib_iic_softc = sc;
		iba.ib_addr = cf->cf_loc[IICCF_ADDR];
		iba.ib_aux = NULL;

		tryagain = 0;
		if (config_match(parent, cf, &iba) > 0) {
			config_attach(parent, cf, &iba, iicprint);
/*			tryagain = (cf->cf_fstate == FSTATE_STAR);*/
		}
	} while (tryagain);

	return (0);
}

/* 
 * Q: Do we really need a device interface ?
 */

int
iicopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct iic_softc *sc;
	int unit = minor(dev);
    
	if (unit >= iic_cd.cd_ndevs)
		return(ENXIO);

	sc = iic_cd.cd_devs[unit];
    
	if (!sc) return(ENXIO);

	if (sc->sc_flags & IIC_OPEN) return(EBUSY);

	sc->sc_flags |= IIC_OPEN;

	return(0);
}


int
iicclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct iic_softc *sc = iic_cd.cd_devs[unit];
    
	sc->sc_flags &= ~IIC_OPEN;

	return(0);
}

/* End of iic.c */
