/*	$NetBSD: txioman_out.c,v 1.2 2001/04/24 17:09:54 uch Exp $	*/

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

int	txout_match(struct device *, struct cfdata *, void *);
void	txout_attach(struct device *, struct device *, void *);
int	txout_hook(void *, int, long, void *);

struct txout_softc {
	struct device sc_dev;
	struct txio_ops *sc_ops;
	int sc_port;
};

struct cfattach txout_ca = {
	sizeof(struct txout_softc), txout_match, txout_attach
};

int
txout_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

void
txout_attach(struct device *parent, struct device *self, void *aux)
{
	struct txio_attach_args *taa = aux;
	struct txout_softc *sc = (void *)self;
	struct txio_ops *ops;

	ops = sc->sc_ops = taa->taa_tc->tc_ioops[taa->taa_group];
	if (ops == 0) {
		printf(": I/O module not found.\n");
		return;
	}
	printf("\n");

	sc->sc_port = taa->taa_port;

	if (taa->taa_initial != -1) {
		ops->_out(ops->_v, sc->sc_port, taa->taa_initial);
	}

	config_hook(taa->taa_type, taa->taa_id, CONFIG_HOOK_SHARE,
		    txout_hook, sc);
}

int
txout_hook(void *arg, int type, long id, void *msg)
{
	struct txout_softc *sc = arg;
	struct txio_ops *ops = sc->sc_ops;	

	ops->_out(ops->_v, sc->sc_port, (int)msg);

	return (0);
}
