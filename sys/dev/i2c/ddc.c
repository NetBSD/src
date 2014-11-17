/* $NetBSD: ddc.c,v 1.4 2014/11/17 00:46:44 jmcneill Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ddc.c,v 1.4 2014/11/17 00:46:44 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <sys/bus.h>
#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcreg.h>
#include <dev/i2c/ddcvar.h>

/*
 * VESA Display Data Channel I2C client, used to access EDID
 * information.
 *
 * Note that this only supports DDC version 2, which uses normal I2C.
 * The older DDCv1 standard used sync signaling to provide the I2C
 * clock, and is typically not used on reasonably recent monitors.
 */

struct ddc_softc {
	i2c_tag_t	sc_tag;
	int		sc_address;
};

static int ddc_match(device_t, cfdata_t, void *);
static void ddc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ddc, sizeof (struct ddc_softc),
    ddc_match, ddc_attach, NULL, NULL);

static int
ddc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ia->ia_addr == DDC_ADDR)
		return 1;
	return 0;
}

static void
ddc_attach(device_t parent, device_t self, void *aux)
{
	struct ddc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_address = ia->ia_addr;

	aprint_naive(": DDC\n");
	aprint_normal(": DDC\n");
}

/* XXX: add cdev ops? */

int
ddc_read_edid(i2c_tag_t tag, uint8_t *dest, size_t len)
{
	return ddc_read_edid_block(tag, dest, len, DDC_EDID_START);
}

int
ddc_read_edid_block(i2c_tag_t tag, uint8_t *dest, size_t len, uint8_t block)
{
	uint8_t		wbuf[2];

	if (iic_acquire_bus(tag, I2C_F_POLL) != 0)
		return -1;

	wbuf[0] = block;	/* start address */

	if (iic_exec(tag, I2C_OP_READ_WITH_STOP, DDC_ADDR, wbuf, 1, dest,
		len, I2C_F_POLL)) {
		iic_release_bus(tag, I2C_F_POLL);
		return -1;
	}
	iic_release_bus(tag, I2C_F_POLL);
	return 0;
}
