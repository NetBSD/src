/*	$NetBSD: ziic.c,v 1.2.14.1 2017/12/03 11:36:52 jdolecek Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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
__KERNEL_RCSID(0, "$NetBSD: ziic.c,v 1.2.14.1 2017/12/03 11:36:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <dev/i2c/i2cvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2c.h>

#ifdef PXAIIC_DEBUG
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)	do { } while (/*CONSTCOND*/0)
#endif

struct pxaiic_softc {
	struct pxa2x0_i2c_softc	sc_pxa_i2c;
	void *			sc_ih;

	struct i2c_controller	sc_i2c;
	kmutex_t		sc_buslock;
};

static int pxaiic_match(device_t, cfdata_t, void *);
static void pxaiic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pxaiic, sizeof(struct pxaiic_softc),
    pxaiic_match, pxaiic_attach, NULL, NULL);

static int pxaiic_acquire_bus(void *, int);
static void pxaiic_release_bus(void *, int);
static int pxaiic_send_start(void *, int);
static int pxaiic_send_stop(void *, int);
static int pxaiic_initiate_xfer(void *, uint16_t, int);
static int pxaiic_read_byte(void *, uint8_t *, int);
static int pxaiic_write_byte(void *, uint8_t, int);

static int
pxaiic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(cf->cf_name, pxa->pxa_name))
		return 0;

	pxa->pxa_addr = PXA2X0_I2C_BASE;
	pxa->pxa_size = PXA2X0_I2C_SIZE;
	return 1;
}

static void
pxaiic_attach(device_t parent, device_t self, void *aux)
{
	struct pxaiic_softc *sc = device_private(self);
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;
	struct pxaip_attach_args *pxa = aux;
	struct i2cbus_attach_args iba;

	aprint_normal(": I2C controller\n");
	aprint_naive("\n");

	psc->sc_dev = self;
	psc->sc_iot = pxa->pxa_iot;
	psc->sc_addr = pxa->pxa_addr;
	psc->sc_size = pxa->pxa_size;
	psc->sc_flags = 0;
	if (pxa2x0_i2c_attach_sub(psc)) {
		aprint_error_dev(self, "unable to attach PXA I2C controller\n");
		return;
	}

	mutex_init(&sc->sc_buslock, MUTEX_DEFAULT, IPL_TTY);

#if 0
	sc->sc_ih = pxa2x0_intr_establish(PXA2X0_INT_I2C, IPL_TTY,
	    pxa2x0_i2c_intr, &psc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish intr\n");
		return;	/* XXX: mutex_destroy, bus_space_unmap */
	}
#endif

	/* Initialize i2c_controller */
	sc->sc_i2c.ic_cookie = sc;
	sc->sc_i2c.ic_acquire_bus = pxaiic_acquire_bus;
	sc->sc_i2c.ic_release_bus = pxaiic_release_bus;
	sc->sc_i2c.ic_send_start = pxaiic_send_start;
	sc->sc_i2c.ic_send_stop = pxaiic_send_stop;
	sc->sc_i2c.ic_initiate_xfer = pxaiic_initiate_xfer;
	sc->sc_i2c.ic_read_byte = pxaiic_read_byte;
	sc->sc_i2c.ic_write_byte = pxaiic_write_byte;
	sc->sc_i2c.ic_exec = NULL;

	memset(&iba, 0, sizeof(iba));
	iba.iba_tag = &sc->sc_i2c;
	(void)config_found_ia(psc->sc_dev, "i2cbus", &iba, iicbus_print);
}

static int
pxaiic_acquire_bus(void *cookie, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	mutex_enter(&sc->sc_buslock);
	pxa2x0_i2c_open(psc);

	return 0;
}

static void
pxaiic_release_bus(void *cookie, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	pxa2x0_i2c_close(psc);
	mutex_exit(&sc->sc_buslock);
}

static int
pxaiic_send_start(void *cookie, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	return pxa2x0_i2c_send_start(psc, flags);
}

static int
pxaiic_send_stop(void *cookie, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	return pxa2x0_i2c_send_stop(psc, flags);
}

static int
pxaiic_initiate_xfer(void *cookie, uint16_t addr, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	return pxa2x0_i2c_initiate_xfer(psc, addr, flags);
}

static int
pxaiic_read_byte(void *cookie, uint8_t *bytep, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	return pxa2x0_i2c_read_byte(psc, bytep, flags);
}

static int
pxaiic_write_byte(void *cookie, uint8_t byte, int flags)
{
	struct pxaiic_softc *sc = cookie;
	struct pxa2x0_i2c_softc *psc = &sc->sc_pxa_i2c;

	return pxa2x0_i2c_write_byte(psc, byte, flags);
}
