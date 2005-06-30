/*	$NetBSD: ucb1200.c,v 1.14 2005/06/30 17:03:53 drochner Exp $ */

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ucb1200.c,v 1.14 2005/06/30 17:03:53 drochner Exp $");

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

#ifdef	UCB1200_DEBUG
#define DPRINTF_ENABLE
#define DPRINTF_DEBUG	ucb1200_debug
#endif
#include <machine/debug.h>

struct ucbchild_state {
	int (*cs_busy)(void *);
	void *cs_arg;
};

struct ucb1200_softc {
	struct device	sc_dev;
	struct device	*sc_parent; /* parent (TX39 SIB module) */
	tx_chipset_tag_t sc_tc;
	
	int		sc_snd_rate; /* passed down from SIB module */
	int		sc_tel_rate;

	/* inquire child module state */
	struct ucbchild_state sc_child[UCB1200_MODULE_MAX];
};

int	ucb1200_match(struct device *, struct cfdata *, void *);
void	ucb1200_attach(struct device *, struct device *, void *);
int	ucb1200_print(void *, const char *);
int	ucb1200_search(struct device *, struct cfdata *,
		       const locdesc_t *, void *);
int	ucb1200_check_id(u_int16_t, int);

#ifdef UCB1200_DEBUG
void	ucb1200_dump(struct ucb1200_softc *);
#endif

CFATTACH_DECL(ucb, sizeof(struct ucb1200_softc),
    ucb1200_match, ucb1200_attach, NULL, NULL);

const struct ucb_id {
	u_int16_t	id;
	const char	*product;
} ucb_id[] = {
	{ UCB1100_ID,	"PHILIPS UCB1100" },
	{ UCB1200_ID,	"PHILIPS UCB1200" },
	{ UCB1300_ID,	"PHILIPS UCB1300" },
	{ TC35413F_ID,	"TOSHIBA TC35413F" },
	{ 0, 0 }
};

int
ucb1200_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct txsib_attach_args *sa = aux;
	u_int16_t reg;
	
	if (sa->sa_slot != 0) /* UCB1200 must be subframe 0 */
		return (0);
	reg = txsibsf0_reg_read(sa->sa_tc, UCB1200_ID_REG);
	
	return (ucb1200_check_id(reg, 0));
}

void
ucb1200_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsib_attach_args *sa = aux;
	struct ucb1200_softc *sc = (void*)self;
	u_int16_t reg;

	printf(": ");
	sc->sc_tc = sa->sa_tc;
	sc->sc_parent = parent;
	sc->sc_snd_rate = sa->sa_snd_rate;
	sc->sc_tel_rate = sa->sa_tel_rate;

	tx39sib_enable1(sc->sc_parent);
	tx39sib_enable2(sc->sc_parent);

#ifdef UCB1200_DEBUG	
	if (ucb1200_debug)
		ucb1200_dump(sc);
#endif
	reg = txsibsf0_reg_read(sa->sa_tc, UCB1200_ID_REG);
	(void)ucb1200_check_id(reg, 1);
	printf("\n");

	config_search_ia(ucb1200_search, self, "ucbif", ucb1200_print);
}

int
ucb1200_search(struct device *parent, struct cfdata *cf,
	       const locdesc_t *ldesc, void *aux)
{
	struct ucb1200_softc *sc = (void*)parent;
	struct ucb1200_attach_args ucba;
	
	ucba.ucba_tc = sc->sc_tc;
	ucba.ucba_snd_rate = sc->sc_snd_rate;
	ucba.ucba_tel_rate = sc->sc_tel_rate;
	ucba.ucba_sib	   = sc->sc_parent;
	ucba.ucba_ucb	   = parent;
	
	if (config_match(parent, cf, &ucba))
		config_attach(parent, cf, &ucba, ucb1200_print);

	return (0);
}

int
ucb1200_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}

int
ucb1200_check_id(u_int16_t idreg, int print)
{
	int i;

	for (i = 0; ucb_id[i].product; i++) {
		if (ucb_id[i].id == idreg) {
			if (print) {
				printf("%s", ucb_id[i].product);
			}
			
			return (1);
		}
	}
	
	return (0);
}

void
ucb1200_state_install(struct device *dev, int (*sfun)(void *), void *sarg,
    int sid)
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
				return (0);
		
	return (1); /* idle state */
}

#ifdef UCB1200_DEBUG
void
ucb1200_dump(struct ucb1200_softc *sc)
{
	static const char *const regname[] = {
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
	tx_chipset_tag_t tc;
	u_int16_t reg;
	int i;

	tc = sc->sc_tc;

	printf("\n\t[UCB1200 register]\n");
	for (i = 0; i < 16; i++) {
		reg = txsibsf0_reg_read(tc, i);
		printf("%s(%02d) 0x%04x ", regname[i], i, reg);
		dbg_bit_print(reg);
	}
}
#endif /* UCB1200_DEBUG */
