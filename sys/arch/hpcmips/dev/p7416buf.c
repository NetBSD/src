/*	$NetBSD: p7416buf.c,v 1.3 2000/01/03 18:24:03 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
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
 * Device driver for PHILIPS 74ALVC16241/245 buffer chip
 */

#include "opt_tx39_debug.h"
#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>

#include <hpcmips/dev/p7416bufvar.h>
#include <hpcmips/dev/skbdvar.h>

#define P7416_SERACH_MAX	256
#define P7416_ROW_MAX		16
#define P7416_COLUMN_MAX	8	/* XXX 16? */

struct p7416buf_chip {
	bus_space_tag_t		scc_cst;
	bus_space_handle_t	scc_csh;
	u_int16_t		scc_buf[P7416_ROW_MAX];
	int			scc_enabled;

	struct skbd_controller	scc_controller;
};

struct p7416buf_softc {
	struct device		sc_dev;
	struct p7416buf_chip	*sc_chip;
	tx_chipset_tag_t	sc_tc;
	void			*sc_ih;
};

int	p7416buf_match __P((struct device*, struct cfdata*, void*));
void	p7416buf_attach __P((struct device*, struct device*, void*));
int	p7416buf_intr __P((void*));
int	p7416buf_poll __P((void*));
void	p7416buf_ifsetup __P((struct p7416buf_chip*));

int	p7416buf_input_establish __P((void*, int (*) __P((void*, int, int)),
				      void (*) __P((void*)), void*));

struct p7416buf_chip p7416buf_chip;

struct cfattach p7416buf_ca = {
	sizeof(struct p7416buf_softc), p7416buf_match, p7416buf_attach
};

int
p7416buf_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
p7416buf_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cs_attach_args *ca = aux;
	struct p7416buf_softc *sc = (void*)self;
	struct skbd_attach_args saa;

	sc->sc_tc = ca->ca_tc;
	sc->sc_chip = &p7416buf_chip;
	
	sc->sc_chip->scc_cst = ca->ca_csio.cstag;

	if (bus_space_map(sc->sc_chip->scc_cst, ca->ca_csio.csbase, 
			  ca->ca_csio.cssize, 0, &sc->sc_chip->scc_csh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_chip->scc_enabled = 0;

	/*
	 * The key debounce and scanning are handled by reading memory 
	 * mapped I/O in periodic polling.
	 */
#ifndef USE_POLL
#error options USE_POLL requied.
#endif
	if (!(sc->sc_ih = tx39_poll_establish(sc->sc_tc, 1,
					      IPL_TTY, p7416buf_intr, 
					      sc))) {
		printf(": can't establish interrupt\n");
		return;
	}

	printf("\n");

	/* setup upper interface */
	p7416buf_ifsetup(sc->sc_chip);
	
	saa.saa_ic = &sc->sc_chip->scc_controller;

	config_found(self, &saa, skbd_print);
}

void
p7416buf_ifsetup(scc)
	struct p7416buf_chip *scc;
{
	scc->scc_controller.skif_v		= scc;

	scc->scc_controller.skif_establish	= p7416buf_input_establish;
	scc->scc_controller.skif_poll		= p7416buf_poll;
}

int
p7416buf_cnattach(addr)
	paddr_t addr;
{
	struct p7416buf_chip *scc = &p7416buf_chip;
	
	scc->scc_csh = MIPS_PHYS_TO_KSEG1(addr);

	p7416buf_ifsetup(scc);

	skbd_cnattach(&scc->scc_controller);

	return 0;
}

int
p7416buf_input_establish(ic, inputfunc, inputhookfunc, arg)
	void *ic;
	int (*inputfunc) __P((void*, int, int));
	void (*inputhookfunc) __P((void*));
	void *arg;
{
	struct p7416buf_chip *scc = ic;

	/* setup lower interface */

	scc->scc_controller.sk_v = arg;

	scc->scc_controller.sk_input = inputfunc;
	scc->scc_controller.sk_input_hook = inputhookfunc;
	
	scc->scc_enabled = 1;

	return 0;
}

int
p7416buf_intr(arg)
	void *arg;
{
	struct p7416buf_softc *sc = arg;

	return p7416buf_poll(sc->sc_chip);
}

int
p7416buf_poll(arg)
	void *arg;
{
	struct p7416buf_chip *scc = arg;	
	bus_space_tag_t t = scc->scc_cst;
	bus_space_handle_t h = scc->scc_csh;
	u_int16_t buf[P7416_SERACH_MAX], pattern;
	u_int16_t mask, rpat, edge;
	int type, val;
	int i, j, k;
	skbd_tag_t controller;

	if (!scc->scc_enabled) {
		return 0;
	}

	for (pattern = 0, i = 0; i < P7416_SERACH_MAX; i++) {
		buf[i] = bus_space_read_2(t, h, i * 2);
		pattern |= buf[i];
		bus_space_write_2(t, h, i * 2, 0);
	}

	controller = &scc->scc_controller;

	skbd_input_hook(controller);

	for (i = 0; i < P7416_ROW_MAX; i++) { /* Loop for each bit */
		mask = 1 << i;
		if (pattern & mask) {
			for (j = k = 0; j < P7416_SERACH_MAX; j++) {
				if (buf[j] & mask) {
					k++;
				}
			}

			switch (k) {
			default:
				/* crude pattern */
				rpat = 0;
				break;
			case 256: /* 2/2 */
				rpat = 1 << 0;
				break;
			case 192: /* 3/4 */
				rpat = 1 << 1;
				break;
			case 160: /* 5/8 */
				rpat = 1 << 2;
				break;
			case 144: /* 9/16 */
				rpat = 1 << 3;				
				break;
			case 136: /* 17/32 */
				rpat = 1 << 4;
				break;
			case 132: /* 33/64*/
				rpat = 1 << 5;
				break;
			case 130: /* 65/128 */
				rpat = 1 << 6;
				break;
			case 129: /* 129/256 */
				rpat = 1 << 7;
				break;
			}

		} else {
			rpat = 0;
		}

		edge = scc->scc_buf[i] ^ rpat;
		scc->scc_buf[i] = rpat;

		for (k = 0, mask = 1; k < P7416_COLUMN_MAX; 
		     k++, mask <<= 1) {
			if (mask & edge) {
				type = mask & rpat ? 1 : 0;
				val = i * 8 + k;

				skbd_input(controller, type, val);
			}
		}
	}

	return 0;
}
