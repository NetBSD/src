/*	$NetBSD: ucbio.c,v 1.11.34.1 2012/10/30 17:19:43 yamt Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * Device driver for PHILIPS UCB1200 Advanced modem/audio analog front-end
 *	General Purpose I/O part.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ucbio.c,v 1.11.34.1 2012/10/30 17:19:43 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

#include <dev/hpc/hpciovar.h>	/* I/O manager */

#include <machine/debug.h>

int ucbio_match(device_t, cfdata_t, void *);
void ucbio_attach(device_t, device_t, void *);

struct betty_port_status {
	u_int16_t dir;
	u_int16_t in, out;
};

struct ucbio_softc {
	device_t sc_dev;
	tx_chipset_tag_t sc_tc;

	struct betty_port_status sc_stat, sc_ostat;
	struct hpcio_chip sc_hc;
};

CFATTACH_DECL_NEW(ucbio, sizeof(struct ucbio_softc),
    ucbio_match, ucbio_attach, NULL, NULL);

/* I/O */
static int betty_in(hpcio_chip_t, int);
static void betty_out(hpcio_chip_t, int, int);
/* interrupt */
static hpcio_intr_handle_t betty_intr_establish(hpcio_chip_t, int, int,
    int (*)(void *), void *);
static void betty_intr_disestablish(hpcio_chip_t, void *);
/* debug */
static void betty_update(hpcio_chip_t);
static void betty_dump(hpcio_chip_t);

int
ucbio_match(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

void
ucbio_attach(device_t parent, device_t self, void *aux)
{
	struct ucb1200_attach_args *ucba = aux;
	struct ucbio_softc *sc = device_private(self);
	struct hpcio_chip *hc = &sc->sc_hc;

	sc->sc_dev = self;
	sc->sc_tc = ucba->ucba_tc;
	printf("\n");

	hc->hc_sc		 = sc;
	hc->hc_chipid		 = 2;
	hc->hc_name		 = "UCB1200";
	hc->hc_portread		 = betty_in;
	hc->hc_portwrite	 = betty_out;
	hc->hc_intr_establish	 = betty_intr_establish;
	hc->hc_intr_disestablish = betty_intr_disestablish;
	hc->hc_update		 = betty_update;
	hc->hc_dump		 = betty_dump;

	tx_conf_register_ioman(sc->sc_tc, hc);

	hpcio_update(hc);
	hpcio_dump(hc);
}

/* basic I/O */
static void
betty_out(hpcio_chip_t hc, int port, int onoff)
{
	struct ucbio_softc *sc = hc->hc_sc;
	tx_chipset_tag_t tc = sc->sc_tc;	
	txreg_t reg, pos;

	pos = 1 << port;
	reg = txsibsf0_reg_read(tc, UCB1200_IO_DATA_REG);
	if (onoff)
		reg |= pos;
	else
		reg &= ~pos;
	txsibsf0_reg_write(tc, UCB1200_IO_DATA_REG, reg);
}

static int
betty_in(hpcio_chip_t hc, int port)
{
	struct ucbio_softc *sc = hc->hc_sc;
	tx_chipset_tag_t tc = sc->sc_tc;	
	txreg_t reg = txsibsf0_reg_read(tc, UCB1200_IO_DATA_REG);

	return (reg & (1 << port));
}

/* interrupt method not implemented */
static hpcio_intr_handle_t
betty_intr_establish(hpcio_chip_t hc, int port, int mode, int (*func)(void *),
    void *func_arg)
{
	struct ucbio_softc *sc = hc->hc_sc;

	printf("%s: %s not implemented.\n", device_xname(sc->sc_dev),
	    __func__);

	return (0);
}

static void
betty_intr_disestablish(hpcio_chip_t hc, void *ih)
{
	struct ucbio_softc *sc = hc->hc_sc;

	printf("%s: %s not implemented.\n", device_xname(sc->sc_dev),
	    __func__);
}

/* debug */
static void
betty_update(hpcio_chip_t hc)
{
	struct ucbio_softc *sc = hc->hc_sc;
	tx_chipset_tag_t tc = sc->sc_tc;	
	struct betty_port_status *stat = &sc->sc_stat;
	u_int16_t dir, data;
	
	sc->sc_ostat = *stat; /* save old status */
	dir = stat->dir = txsibsf0_reg_read(tc, UCB1200_IO_DIR_REG);
	data = txsibsf0_reg_read(tc, UCB1200_IO_DATA_REG);
	stat->out = data & dir;
	stat->in = data & ~dir;
}

static void
betty_dump(hpcio_chip_t hc)
{
#ifdef UCBIO_DEBUG
	struct ucbio_softc *sc = hc->hc_sc;
	struct betty_port_status *stat = &sc->sc_stat;

	printf("[BETTY I/O]\n");
	printf("IN  ");
	dbg_bit_print(stat->in);
	printf("OUT ");
	dbg_bit_print(stat->out);
	printf("DIR ");
	dbg_bit_print(stat->dir);
#endif /* UCBIO_DEBUG */
}
