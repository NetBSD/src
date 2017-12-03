/*	$NetBSD: nbpiic.c,v 1.1.12.1 2017/12/03 11:36:14 jdolecek Exp $ */
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: nbpiic.c,v 1.1.12.1 2017/12/03 11:36:14 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>

#include <hpcarm/dev/nbppconvar.h>

#include <dev/i2c/i2cvar.h>

#include "nbppcon.h"

#define NBPIIC_SLAVE_ADDR	0x70

struct nbpiic_softc {
	struct pxa2x0_i2c_softc sc_pxa_i2c;

	struct i2c_controller sc_i2c;
	kmutex_t sc_lock;

	device_t sc_pcon;
	char sc_buf[16];
	int sc_idx;
};


static int pxaiic_match(device_t, cfdata_t, void *);
static void pxaiic_attach(device_t, device_t, void *);

static int nbpiic_intr(void *);
int nbpiic_poll(void *, int, char *);

/* fuctions for i2c_controller */
static int nbpiic_acquire_bus(void *, int);
static void nbpiic_release_bus(void *, int);
static int nbpiic_exec(void *cookie, i2c_op_t, i2c_addr_t, const void *, size_t,
    void *, size_t, int);

CFATTACH_DECL_NEW(pxaiic, sizeof(struct nbpiic_softc),
    pxaiic_match, pxaiic_attach, NULL, NULL);


/* ARGSUSED */
static int
pxaiic_match(device_t parent, cfdata_t match, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, match->cf_name) != 0 ||
	    !platid_match(&platid, &platid_mask_MACH_PSIONTEKLOGIX_NETBOOK_PRO))
		return 0;

	pxa->pxa_size = PXA2X0_I2C_SIZE;
	return 1;
}

/* ARGSUSED */
static void
pxaiic_attach(device_t parent, device_t self, void *aux)
{
	struct nbpiic_softc *sc = device_private(self);
	struct pxaip_attach_args *pxa = aux;
	struct i2cbus_attach_args iba;
	void *ih;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_pxa_i2c.sc_dev = self;
	sc->sc_pxa_i2c.sc_iot = pxa->pxa_iot;
	sc->sc_pxa_i2c.sc_addr = pxa->pxa_addr;
	sc->sc_pxa_i2c.sc_size = pxa->pxa_size;
	sc->sc_pxa_i2c.sc_isar = NBPIIC_SLAVE_ADDR;
	sc->sc_pxa_i2c.sc_stat = PI2C_STAT_INIT;
	if (pxa2x0_i2c_attach_sub(&sc->sc_pxa_i2c)) {
		aprint_error_dev(self, "unable to attach PXA I2C\n");
		return;
	}

	/* Initialize mutex with IPL_HIGH.  Keyboard was connected to us. */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	ih = pxa2x0_intr_establish(pxa->pxa_intr, IPL_HIGH, nbpiic_intr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "intr_establish failed\n");
		return;
	}

	/* Initialize i2c_controller  */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = nbpiic_acquire_bus;
	sc->sc_i2c.ic_release_bus = nbpiic_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = nbpiic_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	pxa2x0_i2c_open(&sc->sc_pxa_i2c);
	config_found_ia(self, "i2cbus", &iba, iicbus_print);

	sc->sc_pcon = device_find_by_xname("nbppcon0");

	/* Don't close I2C-bus.  We are slave device. */
}

static int
nbpiic_intr(void *arg)
{
	struct nbpiic_softc *sc = arg;
	struct pxa2x0_i2c_softc *pi2c = &sc->sc_pxa_i2c;
	int handled, len;

	handled = pxa2x0_i2c_intr_sub(pi2c, &len, &sc->sc_buf[sc->sc_idx]);
	if (len != 0)
		sc->sc_idx += len;
	if (pi2c->sc_stat == PI2C_STAT_STOP) {
#if NNBPPCON > 0
		nbppcon_intr(sc->sc_pcon, sc->sc_idx, sc->sc_buf);
#endif
		sc->sc_idx = 0;
		pi2c->sc_stat = PI2C_STAT_INIT;
	}

	return handled;
}

int
nbpiic_poll(void *cookie, int buflen, char *buf)
{
	struct nbpiic_softc *sc = cookie;
	int rv;

	if (sc->sc_idx > 0) {
		if (buflen < sc->sc_idx) {
			(void) pxa2x0_i2c_poll(&sc->sc_pxa_i2c,
			    sizeof(sc->sc_buf) - sc->sc_idx, buf + sc->sc_idx,
			    I2C_F_READ);
			sc->sc_idx = 0;
		} else
			memcpy(buf, sc->sc_buf, sc->sc_idx);
	}
	rv = pxa2x0_i2c_poll(&sc->sc_pxa_i2c,
	    buflen - sc->sc_idx, buf + sc->sc_idx, I2C_F_READ);
	sc->sc_idx = 0;

	return rv;
}

static int
nbpiic_acquire_bus(void *cookie, int flags)
{
	struct nbpiic_softc *sc = cookie;

	mutex_enter(&sc->sc_lock);

	return 0;
}

static void
nbpiic_release_bus(void *cookie, int flags)
{
	struct nbpiic_softc *sc = cookie;

	mutex_exit(&sc->sc_lock);

	return;
}

static int
nbpiic_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *vcmd,
	    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
	struct nbpiic_softc *sc = cookie;
	int rv = -1;
	const u_char cmd = *(const u_char *)vcmd; 

	if (I2C_OP_READ_P(op) && (cmdlen == 0) && (buflen == 1))
		rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c, addr, (u_char *)vbuf);

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, cmd);
		if (rv == 0)
			rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c,
			    addr, (u_char *)vbuf);
	}

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 2)) {
printf("%s: read cmdlen=1, buflen=2: Ooops, maybe error...\n", __func__);
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, cmd);
		if (rv == 0)
			rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c,
			    addr, &((u_char *)vbuf)[0]);
		if (rv == 0)
			rv = pxa2x0_i2c_read(&sc->sc_pxa_i2c,
			    addr, &((u_char *)vbuf)[1]);
	}

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 1))
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, *(u_char *)vbuf);

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		u_short v = (cmd << 8) | ((u_char *)vbuf)[0];

		rv = pxa2x0_i2c_write_2(&sc->sc_pxa_i2c, addr, v);
	}

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 2)) {
printf("%s: write cmdlen=1, buflen=2: Ooops, maybe error...\n", __func__);
		rv = pxa2x0_i2c_write(&sc->sc_pxa_i2c, addr, cmd);
		if (rv == 0) {
			u_short v =
			    (((u_char *)vbuf)[0] << 8) | ((u_char *)vbuf)[1];

			rv = pxa2x0_i2c_write_2(&sc->sc_pxa_i2c, addr, v);
		}
	}

	/* Handle quick_read/quick_write ops - XXX Untested XXX */
	if ((cmdlen == 0) && (buflen == 0))
		rv = pxa2x0_i2c_quick(&sc->sc_pxa_i2c, addr,
			I2C_OP_READ_P(op) ? 1 : 0);

	return rv;
}
