/*	$NetBSD: txioman_in.c,v 1.1 2000/10/22 10:42:33 uch Exp $	*/

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
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <machine/config_hook.h>
#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txiomanvar.h>

int	txin_match(struct device *, struct cfdata *, void *);
void	txin_attach(struct device *, struct device *, void *);
int	txin_intr(void *);
int	txin_intr_p(void *);
int	txin_intr_n(void *);

struct txin_softc {
	struct device sc_dev;
	int sc_type;			/* event type */
	int sc_id;			/* event id */
	int sc_pedge;
	int sc_state;

	void *sc_ih_p;
	void *sc_ih_n;
};

struct cfattach txin_ca = {
	sizeof(struct txin_softc), txin_match, txin_attach
};

int
txin_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
txin_attach(struct device *parent, struct device *self, void *aux)
{
	struct txio_attach_args *taa = aux;
	struct txin_softc *sc = (void *)self;
	struct txio_ops *ops;
	int pe, ne;

	ops = taa->taa_tc->tc_ioops[taa->taa_group];
	if (ops == 0) {
		printf(": I/O module not found.\n");
		return;
	}
	printf("\n");

	sc->sc_type	= taa->taa_type;
	sc->sc_id	= taa->taa_id;

	if (taa->taa_initial != -1) {
		sc->sc_state	= taa->taa_initial;
		/* set initial state */
		config_defer(self, (void (*)(struct device *))txin_intr);
	}

	/* install interrupt handler */
	ops->_intr_map(ops->_v, taa->taa_port, &pe, &ne);
	
	switch (taa->taa_edge) {
	case TXIO_BOTHEDGE:
		sc->sc_ih_p = ops->_intr_establish(ops->_v, pe,
						   txin_intr_p, sc);
		sc->sc_ih_n = ops->_intr_establish(ops->_v, ne,
						   txin_intr_n, sc);
		break;
	case TXIO_POSEDGE:
		sc->sc_ih_p = ops->_intr_establish(ops->_v, pe, txin_intr, sc);
		break;
	case TXIO_NEGEDGE:
		sc->sc_ih_p = ops->_intr_establish(ops->_v, ne, txin_intr, sc);
		break;
	}
}
int
txin_intr(void *arg)
{
	struct txin_softc *sc = arg;

	printf("%s: type=%d, id=%d\n", __FUNCTION__, sc->sc_type, sc->sc_id);
	config_hook_call(sc->sc_type, sc->sc_id, (void *)sc->sc_state);
	sc->sc_state ^= 1;
	printf("done.\n");

	return 0;
}

int
txin_intr_p(void *arg)
{
	struct txin_softc *sc = arg;

	printf("%s(p): type=%d, id=%d\n", __FUNCTION__, sc->sc_type, sc->sc_id);
	config_hook_call(sc->sc_type, sc->sc_id, (void *)1);
	printf("done.\n");

	return 0;
}

int
txin_intr_n(void *arg)
{
	struct txin_softc *sc = arg;

	printf("%s(n): type=%d, id=%d\n", __FUNCTION__, sc->sc_type, sc->sc_id);
	config_hook_call(sc->sc_type, sc->sc_id, (void *)0);
	printf("done.\n");

	return 0;
}
