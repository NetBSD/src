/*	$NetBSD: m38813c.c,v 1.5.4.2 2002/02/28 04:09:54 nathanw Exp $ */

/*-
 * Copyright (c) 1999, 2001 The NetBSD Foundation, Inc.
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
 * Device driver for MITUBISHI M38813 controller
 */

#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/hpc/hpckbdvar.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>
#include <hpcmips/dev/m38813cvar.h>

struct m38813c_chip {
	bus_space_tag_t		scc_cst;
	bus_space_handle_t	scc_csh;
	int			scc_enabled;
	int t_lastchar;
	int t_extended;
	int t_extended1;

	struct hpckbd_ic_if	scc_if;
	struct hpckbd_if	*scc_hpckbd;
};

struct m38813c_softc {
	struct	device		sc_dev;
	struct m38813c_chip	*sc_chip;
	tx_chipset_tag_t	sc_tc;
	void			*sc_ih;
};

int	m38813c_match(struct device *, struct cfdata *, void *);
void	m38813c_attach(struct device *, struct device *, void *);
int	m38813c_intr(void *);
int	m38813c_poll(void *);
void	m38813c_ifsetup(struct m38813c_chip *);
int	m38813c_input_establish(void *, struct hpckbd_if *);

struct m38813c_chip m38813c_chip;

struct cfattach m38813c_ca = {
	sizeof(struct m38813c_softc), m38813c_match, m38813c_attach
};

int
m38813c_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

void
m38813c_attach(struct device *parent, struct device *self, void *aux)
{
	struct cs_attach_args *ca = aux;
	struct m38813c_softc *sc = (void*)self;
	struct hpckbd_attach_args haa;

	sc->sc_tc = ca->ca_tc;
	sc->sc_chip = &m38813c_chip;
	sc->sc_chip->scc_cst = ca->ca_csio.cstag;

	if (bus_space_map(sc->sc_chip->scc_cst, ca->ca_csio.csbase, 
	    ca->ca_csio.cssize, 0, &sc->sc_chip->scc_csh)) {
		printf(": can't map i/o space\n");
	}

#ifndef USE_POLL
#error options USE_POLL requied.
#endif
	if (!(sc->sc_ih = tx39_poll_establish(sc->sc_tc, 1,
	    IPL_TTY, m38813c_intr,
	    sc))) {
		printf(": can't establish interrupt\n");
	}

	printf("\n");

	/* setup upper interface */
	m38813c_ifsetup(sc->sc_chip);

	haa.haa_ic = &sc->sc_chip->scc_if;

	config_found(self, &haa, hpckbd_print);
}

void
m38813c_ifsetup(struct m38813c_chip *scc)
{

	scc->scc_if.hii_ctx		= scc;

	scc->scc_if.hii_establish	= m38813c_input_establish;
	scc->scc_if.hii_poll		= m38813c_intr;
}

int
m38813c_cnattach(paddr_t addr)
{
	struct m38813c_chip *scc = &m38813c_chip;
	
	scc->scc_csh = MIPS_PHYS_TO_KSEG1(addr);

	m38813c_ifsetup(scc);

	hpckbd_cnattach(&scc->scc_if);

	return (0);
}

int
m38813c_input_establish(void *ic, struct hpckbd_if *kbdif)
{
	struct m38813c_chip *scc = ic;

	/* save lower interface */
	scc->scc_hpckbd = kbdif;

	scc->scc_enabled = 1;

	return (0);
}

#define	KBR_EXTENDED0	0xE0	/* extended key sequence */
#define	KBR_EXTENDED1	0xE1	/* extended key sequence */

int
m38813c_intr(void *arg)
{
	struct m38813c_softc *sc = arg;

	return (m38813c_poll(sc->sc_chip));
}

int
m38813c_poll(void *arg)
{
	struct m38813c_chip *scc = arg;
	bus_space_tag_t t = scc->scc_cst;
	bus_space_handle_t h = scc->scc_csh;
	int datain, type, key;

	if (!scc->scc_enabled) {
		return (0);
	}

	datain= bus_space_read_1(t, h, 0);
	
	hpckbd_input_hook(scc->scc_hpckbd);

	if (datain == KBR_EXTENDED0) {
		scc->t_extended = 1;
		return (0);
	} else if (datain == KBR_EXTENDED1) {
		scc->t_extended1 = 2;
		return (0);
	}

 	/* map extended keys to (unused) codes 128-254 */
	key = (datain & 0x7f) | (scc->t_extended ? 0x80 : 0);
	scc->t_extended = 0;

	/*
	 * process BREAK key (EXT1 1D 45  EXT1 9D C5):
	 * map to (unused) code 7F
	 */
	if (scc->t_extended1 == 2 && (datain == 0x1d || datain == 0x9d)) {
		scc->t_extended1 = 1;
		return (0);
	} else if (scc->t_extended1 == 1 &&
	    (datain == 0x45 || datain == 0xc5)) {
		scc->t_extended1 = 0;
		key = 0x7f;
	} else if (scc->t_extended1 > 0) {
		scc->t_extended1 = 0;
	}

	if (datain & 0x80) {
		scc->t_lastchar = 0;
		type = 0;
	} else {
		/* Always ignore typematic keys */
		if (key == scc->t_lastchar)
			return 0;
		scc->t_lastchar = key;
		type = 1;
	}

	hpckbd_input(scc->scc_hpckbd, type, key);

	return (0);
}
