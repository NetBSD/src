/*	$NetBSD: ucbio.c,v 1.2.2.2 2000/11/20 20:46:20 bouyer Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include "opt_tx39_debug.h"

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

int ucbio_match(struct device*, struct cfdata *, void *);
void ucbio_attach(struct device*, struct device *, void *);

struct betty_port_status {
	u_int16_t dir;
	u_int16_t in, out;
};

struct ucbio_softc {
	struct device sc_dev;
	tx_chipset_tag_t sc_tc;

	struct betty_port_status sc_stat, sc_ostat;
	struct txio_ops sc_betty_ops;
};

struct cfattach ucbio_ca = {
	sizeof(struct ucbio_softc), ucbio_match, ucbio_attach
};

/* I/O */
static int betty_in(void *, int);
static void betty_out(void *, int, int);
/* interrupt */
static void betty_intr_map(void *, int, int *, int *);
static void *betty_intr_establish(void *, int, int (*)(void *), void *);
static void betty_intr_disestablish(void *, void *);
/* debug */
static void betty_update(void *);
static void betty_dump(void *);

int
ucbio_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
ucbio_attach(struct device *parent, struct device *self, void *aux)
{
	struct ucb1200_attach_args *ucba = aux;
	struct ucbio_softc *sc = (void *)self;
	struct txio_ops *ops = &sc->sc_betty_ops;

	sc->sc_tc = ucba->ucba_tc;
	printf("\n");

	ops->_v			= sc;
	ops->_group		= BETTY;
	ops->_in		= betty_in;
	ops->_out		= betty_out;
	ops->_intr_map		= betty_intr_map;
	ops->_intr_establish	= betty_intr_establish;
	ops->_intr_disestablish	= betty_intr_disestablish;
	ops->_update		= betty_update;
	ops->_dump		= betty_dump;

	tx_conf_register_ioman(sc->sc_tc, ops);

	tx_ioman_update(BETTY);
	tx_ioman_dump(BETTY);
}

/* basic I/O */
static void
betty_out(void *arg, int port, int onoff)
{
	struct ucbio_softc *sc = arg;
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
betty_in(void *arg, int port)
{
	struct ucbio_softc *sc = arg;
	tx_chipset_tag_t tc = sc->sc_tc;	
	txreg_t reg = txsibsf0_reg_read(tc, UCB1200_IO_DATA_REG);
	return reg & (1 << port);
}

/* interrupt method not implemented */
static void
betty_intr_map(void *arg, int port, int *pedge, int *nedge)
{
}

static void *
betty_intr_establish(void *arg, int edge, int (*func)(void *), void *func_arg)
{
	struct ucbio_softc *sc = arg;
	printf("%s: %s not implemented.\n", sc->sc_dev.dv_xname,
	       __FUNCTION__);
	return 0;
}

static void
betty_intr_disestablish(void *arg, void *ih)
{
	struct ucbio_softc *sc = arg;
	printf("%s: %s not implemented.\n", sc->sc_dev.dv_xname,
	       __FUNCTION__);
}

/* debug */
static void
betty_update(void *arg)
{
	struct ucbio_softc *sc = arg;
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
betty_dump(void *arg)
{
	struct ucbio_softc *sc = arg;
	struct betty_port_status *stat = &sc->sc_stat;

	printf("[BETTY I/O]\n");
	printf("IN  ");
	bitdisp(stat->in);
	printf("OUT ");
	bitdisp(stat->out);
	printf("DIR ");
	bitdisp(stat->dir);
}




