/*	$NetBSD: plumicu.c,v 1.2 1999/12/07 17:53:04 uch Exp $ */

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

#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#define TAILQ_FOREACH(var, head, field)					\
	for (var = TAILQ_FIRST(head); var; var = TAILQ_NEXT(var, field))
#define	TAILQ_EMPTY(head) ((head)->tqh_first == NULL)

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumicureg.h>

#define PLUMICUDEBUG
#ifdef PLUMICUDEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

int	plumicu_match __P((struct device*, struct cfdata*, void*));
void	plumicu_attach __P((struct device*, struct device*, void*));
int	plumicu_intr __P((void*));

struct plum_intr_ctrl {
	plumreg_t	ic_ackpat1;
	plumreg_t	ic_ackpat2;	int	ic_ackreg2;
	plumreg_t	ic_ienpat;	int	ic_ienreg;
	plumreg_t	ic_senpat;	int	ic_senreg;
} pi_ctrl[PLUM_INTR_MAX] = {
	[PLUM_INT_C1IO]	= {PLUM_INT_INTSTA_PCCINT,	
			   PLUM_INT_PCCINTS_C1IO,	PLUM_INT_PCCINTS_REG,
			   PLUM_INT_PCCIEN_IENC1IO,	PLUM_INT_PCCIEN_REG,		
			   PLUM_INT_PCCIEN_SENC1IO,	PLUM_INT_PCCIEN_REG
	},
	[PLUM_INT_C1RI] = {PLUM_INT_INTSTA_PCCINT,	
			   PLUM_INT_PCCINTS_C1RI,	PLUM_INT_PCCINTS_REG,
			   PLUM_INT_PCCIEN_IENC1RI,	PLUM_INT_PCCIEN_REG,
			   PLUM_INT_PCCIEN_SENC1RI,	PLUM_INT_PCCIEN_REG
	},
	[PLUM_INT_C1SC] = {PLUM_INT_INTSTA_C1SCINT,	0, 0, 0, 0, 0, 0},
	[PLUM_INT_C2IO] = {PLUM_INT_INTSTA_PCCINT,	
			   PLUM_INT_PCCINTS_C2IO,	PLUM_INT_PCCINTS_REG,
			   PLUM_INT_PCCIEN_IENC2IO,	PLUM_INT_PCCIEN_REG,
			   PLUM_INT_PCCIEN_SENC2IO,	PLUM_INT_PCCIEN_REG
	},
	[PLUM_INT_C2RI] = {PLUM_INT_INTSTA_PCCINT,	
			   PLUM_INT_PCCINTS_C2RI,	PLUM_INT_PCCINTS_REG,
			   PLUM_INT_PCCIEN_IENC2RI,	PLUM_INT_PCCIEN_REG,
			   PLUM_INT_PCCIEN_SENC2RI,	PLUM_INT_PCCIEN_REG
	},
	[PLUM_INT_C2SC] = {PLUM_INT_INTSTA_C2SCINT,	0, 0, 0, 0, 0, 0},
	[PLUM_INT_DISP] = {PLUM_INT_INTSTA_DISPINT,	0, 0, 0, 0, 0, 0},
	[PLUM_INT_USB]  = {PLUM_INT_INTSTA_USBINT,
			   0, 0,
			   PLUM_INT_USBINTEN_IEN,	PLUM_INT_USBINTEN_REG,
			   0, 0
	},
	[PLUM_INT_USBWAKE] = {PLUM_INT_INTSTA_USBWAKE,
			   0, 0,
			   PLUM_INT_USBINTEN_WIEN,	PLUM_INT_USBINTEN_REG,
			   0, 0
	},
	[PLUM_INT_SM]	= {PLUM_INT_INTSTA_SMINT,
			   0, 0,
			   PLUM_INT_SMIEN,		PLUM_INT_SMIEN_REG,
			   0, 0
	},
	[PLUM_INT_EXT5IO0] = {PLUM_INT_INTSTA_EXTINT,	
			   PLUM_INT_EXTINTS_IO5INT0,	PLUM_INT_EXTINTS_REG,
			   PLUM_INT_EXTIEN_IENIO5INT0,	PLUM_INT_EXTIEN_REG,
			   PLUM_INT_EXTIEN_SENIO5INT0,	PLUM_INT_EXTIEN_REG,
	},
	[PLUM_INT_EXT5IO1] = {PLUM_INT_INTSTA_EXTINT,
			   PLUM_INT_EXTINTS_IO5INT1,	PLUM_INT_EXTINTS_REG,
			   PLUM_INT_EXTIEN_IENIO5INT1,	PLUM_INT_EXTIEN_REG,
			   PLUM_INT_EXTIEN_SENIO5INT1,	PLUM_INT_EXTIEN_REG,
	},
	[PLUM_INT_EXT5IO2] = {PLUM_INT_INTSTA_EXTINT,
			   PLUM_INT_EXTINTS_IO5INT2,	PLUM_INT_EXTINTS_REG,
			   PLUM_INT_EXTIEN_IENIO5INT2,	PLUM_INT_EXTIEN_REG,
			   PLUM_INT_EXTIEN_SENIO5INT2,	PLUM_INT_EXTIEN_REG,
	},
	[PLUM_INT_EXT5IO3] = {PLUM_INT_INTSTA_EXTINT,
			   PLUM_INT_EXTINTS_IO5INT3,	PLUM_INT_EXTINTS_REG,
			   PLUM_INT_EXTIEN_IENIO5INT0,	PLUM_INT_EXTIEN_REG,
			   PLUM_INT_EXTIEN_SENIO5INT0,	PLUM_INT_EXTIEN_REG,
	},
	[PLUM_INT_EXT3IO0] = {PLUM_INT_INTSTA_EXTINT,
			   PLUM_INT_EXTINTS_IO3INT0,	PLUM_INT_EXTINTS_REG,
			   PLUM_INT_EXTIEN_IENIO3INT0,	PLUM_INT_EXTIEN_REG,
			   PLUM_INT_EXTIEN_SENIO3INT0,	PLUM_INT_EXTIEN_REG,
	},
	[PLUM_INT_EXT3IO1] = {PLUM_INT_INTSTA_EXTINT,
			   PLUM_INT_EXTINTS_IO3INT1,	PLUM_INT_EXTINTS_REG,
			   PLUM_INT_EXTIEN_IENIO3INT1,	PLUM_INT_EXTIEN_REG,
			   PLUM_INT_EXTIEN_SENIO3INT1,	PLUM_INT_EXTIEN_REG,
	}
};

struct plum_intr_entry {
	int pi_line;
	int (*pi_fun) __P((void*));
	void *pi_arg;
	struct plum_intr_ctrl *pi_ctrl;
	TAILQ_ENTRY(plum_intr_entry) pi_link;
};

struct plumicu_softc {
	struct	device		sc_dev;
	plum_chipset_tag_t	sc_pc;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	void			*sc_ih;
	int			sc_enable_count;
	TAILQ_HEAD(, plum_intr_entry) sc_pi_head[PLUM_INTR_MAX];
};

struct cfattach plumicu_ca = {
	sizeof(struct plumicu_softc), plumicu_match, plumicu_attach
};

void	plumicu_dump __P((struct plumicu_softc*));

int
plumicu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group */
}

void
plumicu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct plum_attach_args *pa = aux;
	struct plumicu_softc *sc = (void*)self;
	struct plum_intr_ctrl *pic;
	bus_space_tag_t regt;
	bus_space_handle_t regh;
	plumreg_t reg;
	int i;

	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;
	
	/* map plum2 interrupt controller register space */
	if (bus_space_map(sc->sc_regt, PLUM_INT_REGBASE, 
			  PLUM_INT_REGSIZE, 0, &sc->sc_regh)) {
		printf(":interrupt register map failed\n");
		return;
	}

	/* disable all interrupt */
	regt = sc->sc_regt;
	regh = sc->sc_regh;
	for (i = 0; i < PLUM_INTR_MAX; i++) {
		pic = &pi_ctrl[i];
		if (pic->ic_ienreg) {
			reg = plum_conf_read(regt, regh, pic->ic_ienreg);
			reg &= ~pic->ic_ienpat;
			plum_conf_write(regt, regh, pic->ic_ienreg, reg);
		}
		if (pic->ic_senreg) {
			reg = plum_conf_read(regt, regh, pic->ic_senreg);
			reg &= ~pic->ic_senpat;
			plum_conf_write(regt, regh, pic->ic_senreg, reg);
		}
	}
	
	for (i = 0; i < PLUM_INTR_MAX; i++) {
		TAILQ_INIT(&sc->sc_pi_head[i]);		
	}

	/* register handle to plum_chipset_tag */
	plum_conf_register_intr(sc->sc_pc, (void*)sc);
	
	/* disable interrupt redirect to TX39 core */
	plum_conf_write(sc->sc_regt, sc->sc_regh, PLUM_INT_INTIEN_REG, 0);

	if (!(sc->sc_ih = tx_intr_establish(sc->sc_pc->pc_tc, pa->pa_irq,
					      IST_EDGE, IPL_BIO, plumicu_intr, sc))) {
		printf(": can't establish interrupt\n");
	}
	printf("\n");

	plumicu_dump(sc);
}

void*
plum_intr_establish(pc, line, mode, level, ih_fun, ih_arg)
	plum_chipset_tag_t pc;
	int line;
	int mode;  /* no meaning */
	int level; /* XXX not yet */
	int (*ih_fun) __P((void*));
	void *ih_arg;
{
	struct plumicu_softc *sc = pc->pc_intrt;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;
	struct plum_intr_entry *pi;

	if (!LEGAL_PRUM_INTR(line)) {
		panic("plum_intr_establish: bogus interrupt line");
	}

	if (!(pi = malloc(sizeof(struct plum_intr_entry), 
			  M_DEVBUF, M_NOWAIT))) {
		panic ("plum_intr_establish: no memory.");
	}

	memset(pi, 0, sizeof(struct plum_intr_entry));
	pi->pi_line = line;
	pi->pi_fun  = ih_fun;
	pi->pi_arg  = ih_arg;
	pi->pi_ctrl = &pi_ctrl[line];
	TAILQ_INSERT_TAIL(&sc->sc_pi_head[line], pi, pi_link);
	
	/* Enable interrupt */
	/* status enable */
	if (pi->pi_ctrl->ic_senreg) {
		reg = plum_conf_read(regt, regh, pi->pi_ctrl->ic_senreg);
		reg |= pi->pi_ctrl->ic_senpat;
		plum_conf_write(regt, regh, pi->pi_ctrl->ic_senreg, reg);
	}
	/* interrupt enable */
	if (pi->pi_ctrl->ic_ienreg) {
		reg = plum_conf_read(regt, regh, pi->pi_ctrl->ic_ienreg);
		reg |= pi->pi_ctrl->ic_ienpat;
		plum_conf_write(regt, regh, pi->pi_ctrl->ic_ienreg, reg);
	}

	/* Enable redirect to TX39 core */
	DPRINTF(("plum_intr_establish: %d (count=%d)\n", line, sc->sc_enable_count));

	if (!sc->sc_enable_count++) {
		plum_conf_write(regt, regh, PLUM_INT_INTIEN_REG, PLUM_INT_INTIEN);
	}

	return ih_fun;
}

void
plum_intr_disestablish(pc, arg)
	plum_chipset_tag_t pc;
	void *arg;
{
	struct plumicu_softc *sc = pc->pc_intrt;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;
	struct plum_intr_entry *pi;
	int i;

	sc = pc->pc_intrt;
	for (i = 0; i < PLUM_INTR_MAX; i++) {
		TAILQ_FOREACH(pi, &sc->sc_pi_head[i], pi_link) {
			if (pi->pi_fun == arg) {
				TAILQ_REMOVE(&sc->sc_pi_head[i], pi, pi_link);
				DPRINTF(("plum_intr_disestablish: %d (count=%d)\n",  
					 pi->pi_line, sc->sc_enable_count - 1));
				goto found;
			}
		}
	}
	panic("plum_intr_disestablish: can't find entry.");
 found:
	/* Disable interrupt */
	if (pi->pi_ctrl->ic_ienreg) {
		reg = plum_conf_read(regt, regh, pi->pi_ctrl->ic_ienreg);
		reg &= ~(pi->pi_ctrl->ic_ienpat);
		plum_conf_write(regt, regh, pi->pi_ctrl->ic_ienreg, reg);
	}
	if (pi->pi_ctrl->ic_senreg) {
		reg = plum_conf_read(regt, regh, pi->pi_ctrl->ic_senreg);
		reg &= ~(pi->pi_ctrl->ic_senpat);
		plum_conf_write(regt, regh, pi->pi_ctrl->ic_senreg, reg);
	}
	free(pi, M_DEVBUF);
	
	/* Disable redirect to TX39 core */
	if (--sc->sc_enable_count == 0) {
		/* Disable redirect to TX39 core */
		plum_conf_write(regt, regh, PLUM_INT_INTIEN_REG, 0);
	}
}

int
plumicu_intr(arg)
	void *arg;
{
	struct plumicu_softc *sc = arg;
	struct plum_intr_entry *pi;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg1, reg2;
	int i;
	
	reg1 = plum_conf_read(regt, regh, PLUM_INT_INTSTA_REG);

	for (i = 0; i < PLUM_INTR_MAX; i++) {
		struct plum_intr_ctrl *pic = &pi_ctrl[i];
		if (pic->ic_ackpat1 & reg1) {
			if (pic->ic_ackpat2) {
				reg2 = plum_conf_read(regt, regh, 
						      pic->ic_ackreg2);
				if (pic->ic_ackpat2 & reg2) {
					plum_conf_write(
						regt, regh,
						pic->ic_ackreg2,
						pic->ic_ackpat2);
					TAILQ_FOREACH(pi, 
						      &sc->sc_pi_head[i], 
						      pi_link) {
						(*pi->pi_fun)(pi->pi_arg);
					}
				} 
			} else {
				TAILQ_FOREACH(pi, &sc->sc_pi_head[i], 
					      pi_link) {
					(*pi->pi_fun)(pi->pi_arg);
					printf("INT(2) %d:", i);
				} 
			}
		}
	}

	return 0;
}

void
plumicu_dump(sc)
	struct plumicu_softc *sc;
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;
	
	printf("status:");
	reg = plum_conf_read(regt, regh, PLUM_INT_INTSTA_REG);
	bitdisp(reg);
	printf("ExtIO\n");
	printf("status:");
	reg = plum_conf_read(regt, regh, PLUM_INT_EXTINTS_REG);
	bitdisp(reg);
	printf("enable:");
	reg = plum_conf_read(regt, regh, PLUM_INT_EXTIEN_REG);
 	bitdisp(reg);

}
