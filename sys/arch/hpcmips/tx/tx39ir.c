/*	$NetBSD: tx39ir.c,v 1.3 2001/08/02 04:30:01 shin Exp $ */

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
 * TX39 IR module (connected to UARTB)
 */
#undef TX39IRDEBUG
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39irvar.h>
#include <hpcmips/tx/tx39irreg.h>

#include <hpcmips/tx/tx39clockreg.h> /* XXX */

#ifdef TX39IRDEBUG
int	tx39ir_debug = 1;
#define	DPRINTF(arg) if (vrpiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

int	tx39ir_match(struct device *, struct cfdata *, void *);
void	tx39ir_attach(struct device *, struct device *, void *);

struct tx39ir_softc {
	struct	device sc_dev;
	struct	device *sc_parent;
	tx_chipset_tag_t sc_tc;
};

#ifdef TX39IRDEBUG
static void	tx39ir_dump(struct tx39ir_softc *);
#endif
#if not_required_yet
static int	tx39ir_intr(void *);
#endif

struct cfattach tx39ir_ca = {
	sizeof(struct tx39ir_softc), tx39ir_match, tx39ir_attach
};

int
tx39ir_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (ATTACH_NORMAL);
}

void
tx39ir_attach(struct device *parent, struct device *self, void *aux)
{
	struct txcom_attach_args *tca = aux;
	struct tx39ir_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	txreg_t reg;

	sc->sc_tc = tc = tca->tca_tc;
	sc->sc_parent = tca->tca_parent;

	printf("\n");

	/* setup IR module */
	reg = tx_conf_read(tc, TX39_IRCTRL1_REG);
	reg |= TX39_IRCTRL1_RXPWR;
	tx_conf_write(tc, TX39_IRCTRL1_REG, reg);

	/* power up IR module */
	reg = tx_conf_read(tc, TX39_CLOCKCTRL_REG);
	reg |= TX39_CLOCK_ENIRCLK | TX39_CLOCK_ENUARTBCLK;
	tx_conf_write(tc, TX39_CLOCKCTRL_REG, reg);

	/* turn to pulse mode UARTB */
	txcom_pulse_mode(sc->sc_parent);

#if not_required_yet
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_CARSTINT),
	    IST_EDGE, IPL_TTY, tx39ir_intr, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_POSCARINT),
	    IST_EDGE, IPL_TTY, tx39ir_intr, sc);
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_NEGCARINT),
	    IST_EDGE, IPL_TTY, tx39ir_intr, sc);
#endif

#ifdef TX39IRDEBUG
	tx39ir_dump(sc);
#endif	
}

#ifdef TX39IRDEBUG
void
tx39ir_dump(struct tx39ir_softc *sc)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	reg = tx_conf_read(tc, TX39_IRCTRL1_REG);
#define ISSETPRINT(r, m) __is_set_print((u_int32_t)(r),			\
	TX39_IRCTRL1_##m, #m)
	ISSETPRINT(reg, CARDET);
	ISSETPRINT(reg, TESTIR);
	ISSETPRINT(reg, DTINVERT);
	ISSETPRINT(reg, RXPWR);
	ISSETPRINT(reg, ENSTATE);
	ISSETPRINT(reg, ENCOMSM);
#undef	ISSETPRINT
	printf("baudval %d\n", TX39_IRCTRL1_BAUDVAL(reg));
}
#endif

#if not_required_yet
int
tx39ir_intr(void *arg)
{
	return (0);
}
#endif
