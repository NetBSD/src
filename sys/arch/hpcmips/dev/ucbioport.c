/*	$NetBSD: ucbioport.c,v 1.1 2000/02/27 16:34:13 uch Exp $	*/

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Device driver for PHILIPS UCB1200 Advanced modem/audio analog front-end
 *	General Purpose I/O part. platform dependent port hook.
 */
#define UCBIOPORTDEBUG

#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/config_hook.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

int	ucbioport_match	__P((struct device*, struct cfdata*, void*));
void	ucbioport_attach __P((struct device*, struct device*, void*));

struct ucbioport_softc {
	struct device	sc_dev;
	tx_chipset_tag_t sc_tc;
	int sc_port;
};

struct cfattach ucbioport_ca = {
	sizeof(struct ucbioport_softc), ucbioport_match, ucbioport_attach
};

int	ucbioport_hook __P((void*, int, long, void*));

int
ucbioport_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return (1);
}

void
ucbioport_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ucbioport_softc *sc = (void*)self;
	struct ucbio_attach_args *uia = aux;

	printf("\n");
	sc->sc_tc = uia->uia_tc;
	sc->sc_port = uia->uia_port;
	
	config_hook(CONFIG_HOOK_BUTTONEVENT, uia->uia_id, 
		    CONFIG_HOOK_SHARE, /* btnmgr */
		    ucbioport_hook, sc);
}

int
ucbioport_hook(arg, type, id, msg)
	void*	arg;
	int	type;
	long	id;
	void*	msg;
{
	struct ucbioport_softc *sc = arg;
	txreg_t reg, mask, port;

	reg = txsibsf0_reg_read(sc->sc_tc, UCB1200_IO_DATA_REG);
	
	port = 1 << sc->sc_port;
	mask = reg & port;
	mask ^= port;
	reg &= ~port;
	reg |= mask;
	
	txsibsf0_reg_write(sc->sc_tc, UCB1200_IO_DATA_REG, reg);

	return (0);
}
