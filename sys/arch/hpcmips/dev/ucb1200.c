/*	$NetBSD: ucb1200.c,v 1.2 2000/01/12 14:56:22 uch Exp $ */

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
 */
#undef UCB1200DEBUG
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

#ifdef UCB1200DEBUG
int	ucb1200_debug = 1;
#define	DPRINTF(arg) if (ucb1200_debug) printf arg;
#define	DPRINTFN(n, arg) if (ucb1200_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

struct ucbchild_state {
	int (*cs_busy) __P((void*));
	void *cs_arg;
};

struct ucb1200_softc {
	struct device		sc_dev;
	struct device		*sc_parent; /* parent (TX39 SIB module) */
	tx_chipset_tag_t	sc_tc;
	
	int		sc_snd_rate; /* passed down from SIB module */
	int		sc_tel_rate;

	/* inquire child module state */
	struct ucbchild_state sc_child[UCB1200_MODULE_MAX];
};

int	ucb1200_match	__P((struct device*, struct cfdata*, void*));
void	ucb1200_attach	__P((struct device*, struct device*, void*));
int	ucb1200_print	__P((void*, const char*));
int	ucb1200_search	__P((struct device*, struct cfdata*, void*));

void	ucb1200_dump	__P((struct ucb1200_softc*));

struct cfattach ucb_ca = {
	sizeof(struct ucb1200_softc), ucb1200_match, ucb1200_attach
};

int
ucb1200_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct txsib_attach_args *sa = aux;
	u_int16_t reg;
	
	if (sa->sa_slot != 0) /* UCB1200 must be subframe 0 */
		return 0;
	reg = txsibsf0_reg_read(sa->sa_tc, UCB1200_ID_REG);
	
	if (reg == UCB1200_ID || reg == TC35413F_ID)
		return 1;
	else 
		return 0;
}

void
ucb1200_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsib_attach_args *sa = aux;
	struct ucb1200_softc *sc = (void*)self;

	sc->sc_tc = sa->sa_tc;
	sc->sc_parent = parent;
	sc->sc_snd_rate = sa->sa_snd_rate;
	sc->sc_tel_rate = sa->sa_tel_rate;

	tx39sib_enable1(sc->sc_parent);
	tx39sib_enable2(sc->sc_parent);

#ifdef UCB1200DEBUG	
	ucb1200_dump(sc);
#endif
	printf("\n");

	config_search(ucb1200_search, self, ucb1200_print);
}

int
ucb1200_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ucb1200_softc *sc = (void*)parent;
	struct ucb1200_attach_args ucba;
	
	ucba.ucba_tc = sc->sc_tc;
	ucba.ucba_snd_rate = sc->sc_snd_rate;
	ucba.ucba_tel_rate = sc->sc_tel_rate;
	ucba.ucba_sib	   = sc->sc_parent;
	ucba.ucba_ucb	   = parent;
	
	if ((*cf->cf_attach->ca_match)(parent, cf, &ucba))
		config_attach(parent, cf, &ucba, ucb1200_print);

	return 0;
}

int
ucb1200_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	return pnp ? QUIET : UNCONF;
}

void
ucb1200_state_install(dev, sfun, sarg, sid)
	struct device *dev;
	int (*sfun) __P((void*));
	void *sarg;
	int sid;
{
	struct ucb1200_softc *sc = (void*)dev;
	
	sc->sc_child[sid].cs_busy = sfun;
	sc->sc_child[sid].cs_arg = sarg;
}

int
ucb1200_state_idle(dev)
	struct device *dev;
{
	struct ucb1200_softc *sc = (void*)dev;
	struct ucbchild_state *cs;
	int i;

	cs = sc->sc_child;
	for (i = 0; i < UCB1200_MODULE_MAX; i++, cs++)
		if (cs->cs_busy)
			if ((*cs->cs_busy)(cs->cs_arg))
				return 0;
		
	return 1; /* idle state */
}

void
ucb1200_dump(sc)
	struct ucb1200_softc *sc;
{
        const char *regname[] = {
                "IO_DATA        ",
                "IO_DIR         ",
                "POSINTEN       ",
                "NEGINTEN       ",
                "INTSTAT        ",
                "TELECOMCTRLA   ",
                "TELECOMCTRLB   ",
                "AUDIOCTRLA     ",
                "AUDIOCTRLB     ",
                "TOUCHSCREENCTRL",
                "ADCCTRL        ",
                "ADCDATA        ",
                "ID             ",
                "MODE           ",
                "RESERVED       ",
                "NULL           "
        };
	tx_chipset_tag_t tc = sc->sc_tc;
	u_int16_t reg;
	int i;

	printf("\n\t[UCB1200 register]\n");
	for (i = 0; i < 16; i++) {
		reg = txsibsf0_reg_read(tc, i);
		printf("%s(%02d) 0x%04x ", regname[i], i, reg);
		bitdisp(reg);
	}
}
