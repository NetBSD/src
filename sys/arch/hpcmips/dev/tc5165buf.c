/*	$NetBSD: tc5165buf.c,v 1.14 2007/07/09 20:52:12 ad Exp $ */

/*-
 * Copyright (c) 1999-2001 The NetBSD Foundation, Inc.
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
 * Device driver for TOSHIBA TC5165BFTS, PHILIPS 74ALVC16241/245 
 * buffer chip
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tc5165buf.c,v 1.14 2007/07/09 20:52:12 ad Exp $");

#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/hpc/hpckbdvar.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>
#include <hpcmips/dev/tc5165bufvar.h>

#define TC5165_ROW_MAX		16
#define TC5165_COLUMN_MAX	8

#ifdef TC5165DEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

struct tc5165buf_chip {
	bus_space_tag_t		scc_cst;
	bus_space_handle_t	scc_csh;
	u_int16_t		scc_buf[TC5165_COLUMN_MAX];
	int			scc_enabled;
	int			scc_queued;
	struct callout		scc_soft_ch;
	struct hpckbd_ic_if	scc_if;
	struct hpckbd_if	*scc_hpckbd;
};

struct tc5165buf_softc {
	struct device		sc_dev;
	struct tc5165buf_chip	*sc_chip;
	tx_chipset_tag_t	sc_tc;
	void			*sc_ih;
};

int	tc5165buf_match(struct device *, struct cfdata *, void *);
void	tc5165buf_attach(struct device *, struct device *, void *);
int	tc5165buf_intr(void *);
int	tc5165buf_poll(void *);
void	tc5165buf_soft(void *);
void	tc5165buf_ifsetup(struct tc5165buf_chip *);

int	tc5165buf_input_establish(void *, struct hpckbd_if *);

struct tc5165buf_chip tc5165buf_chip;

CFATTACH_DECL(tc5165buf, sizeof(struct tc5165buf_softc),
    tc5165buf_match, tc5165buf_attach, NULL, NULL);

int
tc5165buf_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

void
tc5165buf_attach(struct device *parent, struct device *self, void *aux)
{
	struct cs_attach_args *ca = aux;
	struct tc5165buf_softc *sc = (void*)self;
	struct hpckbd_attach_args haa;

	printf(": ");
	sc->sc_tc = ca->ca_tc;
	sc->sc_chip = &tc5165buf_chip;

	callout_init(&sc->sc_chip->scc_soft_ch, 0);

	sc->sc_chip->scc_cst = ca->ca_csio.cstag;

	if (bus_space_map(sc->sc_chip->scc_cst, ca->ca_csio.csbase, 
	    ca->ca_csio.cssize, 0, &sc->sc_chip->scc_csh)) {
		printf("can't map i/o space\n");
		return;
	}

	sc->sc_chip->scc_enabled = 0;

	if (ca->ca_irq1 != -1) {
		sc->sc_ih = tx_intr_establish(sc->sc_tc, ca->ca_irq1,
		    IST_EDGE, IPL_TTY, 
		    tc5165buf_intr, sc);
		printf("interrupt mode");
	} else {
		sc->sc_ih = tx39_poll_establish(sc->sc_tc, 1, IPL_TTY, 
		    tc5165buf_intr, sc);
		printf("polling mode");
	}

	if (!sc->sc_ih) {
		printf(" can't establish interrupt\n");
		return;
	}

	printf("\n");

	/* setup upper interface */
	tc5165buf_ifsetup(sc->sc_chip);
	
	haa.haa_ic = &sc->sc_chip->scc_if;

	config_found(self, &haa, hpckbd_print);
}

void
tc5165buf_ifsetup(struct tc5165buf_chip *scc)
{

	scc->scc_if.hii_ctx		= scc;
	scc->scc_if.hii_establish	= tc5165buf_input_establish;
	scc->scc_if.hii_poll		= tc5165buf_poll;
}

int
tc5165buf_cnattach(paddr_t addr)
{
	struct tc5165buf_chip *scc = &tc5165buf_chip;
	
	scc->scc_csh = MIPS_PHYS_TO_KSEG1(addr);

	tc5165buf_ifsetup(scc);

	hpckbd_cnattach(&scc->scc_if);

	return (0);
}

int
tc5165buf_input_establish(void *ic, struct hpckbd_if *kbdif)
{
	struct tc5165buf_chip *scc = ic;

	/* save hpckbd interface */
	scc->scc_hpckbd = kbdif;
	
	scc->scc_enabled = 1;

	return (0);
}

int
tc5165buf_intr(void *arg)
{
	struct tc5165buf_softc *sc = arg;
	struct tc5165buf_chip *scc = sc->sc_chip;

	if (!scc->scc_enabled || scc->scc_queued)
		return (0);
	
	scc->scc_queued = 1;
	callout_reset(&scc->scc_soft_ch, 1, tc5165buf_soft, scc);

	return (0);
}

int
tc5165buf_poll(void *arg)
{
	struct tc5165buf_chip *scc = arg;

	if (!scc->scc_enabled)
		return (POLL_CONT);

	tc5165buf_soft(arg);

	return (POLL_CONT);
}

void
tc5165buf_soft(void *arg)
{
	struct tc5165buf_chip *scc = arg;
	bus_space_tag_t t = scc->scc_cst;
	bus_space_handle_t h = scc->scc_csh;
	u_int16_t mask, rpat, edge;
	int i, j, type, val;
	int s;

	hpckbd_input_hook(scc->scc_hpckbd);

	/* clear scanlines */
	(void)bus_space_read_2(t, h, 0);
	delay(3);

	for (i = 0; i < TC5165_COLUMN_MAX; i++) {
		rpat = bus_space_read_2(t, h, 2 << i);
		delay(3);
		(void)bus_space_read_2(t, h, 0);
		delay(3);
		if ((edge = (rpat ^ scc->scc_buf[i]))) {
			scc->scc_buf[i] = rpat;
			for (j = 0, mask = 1; j < TC5165_ROW_MAX; 
			    j++, mask <<= 1) {
				if (mask & edge) {
					type = mask & rpat ? 1 : 0;
					val = j * TC5165_COLUMN_MAX + i;
					DPRINTF(("%d %d\n", j, i));
					hpckbd_input(scc->scc_hpckbd,
					    type, val);
				}
			}
		}
	}

	s = spltty();
	scc->scc_queued = 0;
	splx(s);
}
