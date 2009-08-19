/*	$NetBSD: gxiic.c,v 1.2.16.3 2009/08/19 18:46:06 yamt Exp $ */
/*
 * Copyright (c) 2007 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gxiic.c,v 1.2.16.3 2009/08/19 18:46:06 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>

#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>

#include <evbarm/gumstix/gumstixvar.h>

#include <dev/i2c/i2cvar.h>


struct gxiic_softc {
	struct pxa2x0_i2c_softc sc_pxa_i2c;

	struct i2c_controller sc_i2c;
	kmutex_t sc_lock;
};


static int gxiicmatch(device_t, cfdata_t, void *);
static void gxiicattach(device_t, device_t, void *);

/* fuctions for i2c_controller */
static int gxiic_acquire_bus(void *, int);
static void gxiic_release_bus(void *, int);
static int gxiic_exec(void *cookie, i2c_op_t, i2c_addr_t, const void *, size_t,
    void *, size_t, int);


CFATTACH_DECL(gxiic, sizeof(struct gxiic_softc),
    gxiicmatch, gxiicattach, NULL, NULL);


/* ARGSUSED */
static int
gxiicmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0)
		 return 0;

	pxa->pxa_size = PXA2X0_I2C_SIZE;
	return 1;
}

/* ARGSUSED */
static void
gxiicattach(device_t parent, device_t self, void *aux)
{
	struct pxaip_attach_args *pxa = aux;
	struct gxiic_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_pxa_i2c.sc_iot = pxa->pxa_iot;
	sc->sc_pxa_i2c.sc_size = pxa->pxa_size;
	if (pxa2x0_i2c_attach_sub(&sc->sc_pxa_i2c)) {
		aprint_error_dev(self, "unable to attach PXA I2C\n");
		return;
	}

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	/* Initialize i2c_controller  */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = gxiic_acquire_bus;
	sc->sc_i2c.ic_release_bus = gxiic_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = gxiic_exec;

	iba.iba_tag = &sc->sc_i2c;
	pxa2x0_i2c_open(&sc->sc_pxa_i2c);
	config_found_ia(&sc->sc_pxa_i2c.sc_dev, "i2cbus", &iba, iicbus_print);
	pxa2x0_i2c_close(&sc->sc_pxa_i2c);
}

static int
gxiic_acquire_bus(void *cookie, int flags)
{
	struct gxiic_softc *sc = cookie;

	mutex_enter(&sc->sc_lock);
	pxa2x0_i2c_open(&sc->sc_pxa_i2c);

	return 0;
}

static void
gxiic_release_bus(void *cookie, int flags)
{
	struct gxiic_softc *sc = cookie;

	pxa2x0_i2c_close(&sc->sc_pxa_i2c);
	mutex_exit(&sc->sc_lock);
	return;
}

static int
gxiic_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
	   size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct gxiic_softc *sc = cookie;
	int rv = -1;

	if (I2C_OP_READ_P(op) && (cmdlen == 0) && (buflen == 1))
		rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c, addr, (u_char *)vbuf);

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, *(u_char *)vbuf);
		if (rv == 0)
			rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c,
			    addr, (u_char *)vbuf);
	}

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 2)) {
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, *(u_char *)vbuf);
		if (rv == 0)
			rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c,
			    addr, (u_char *)vbuf);
		if (rv == 0)
			rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c,
			    addr, (u_char *)(vbuf) + 1);
	}

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 1))
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, *(u_char *)vbuf);

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c,
		    addr, *(const u_char *)vcmd);
		if (rv == 0)
			rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c,
			    addr, *(u_char *)vbuf);
	}

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 2)) {
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c,
		    addr, *(const u_char *)vcmd);
		if (rv == 0)
			rv = pxa2x0_i2c_write_2(&sc->sc_pxa_i2c,
			    addr, *(u_short *)vbuf);
	}

	/* Handle quick_read/quick_write ops - XXX Untested XXX */
	if ((cmdlen == 0) && (buflen == 0))
		rv = pxa2x0_i2c_quick(&sc->sc_pxa_i2c, addr,
			I2C_OP_READ_P(op)?1:0);

	return rv;
}
