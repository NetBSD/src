/*	$NetBSD: tx39icu.c,v 1.1 1999/11/20 19:56:33 uch Exp $ */

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
#include "opt_use_poll.h"
#include "opt_tx39icudebug.h"
#include "opt_tx39_watchdogtimer.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#define TAILQ_FOREACH(var, head, field)					\
	for (var = TAILQ_FIRST(head); var; var = TAILQ_NEXT(var, field))
#define	TAILQ_EMPTY(head) ((head)->tqh_first == NULL)

#include <mips/cpuregs.h>
#include <machine/bus.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>

#include <machine/clock_machdep.h>
#include <machine/cpu.h>
#include <dev/dec/clockvar.h>

#undef TX39ICUDEBUG_PRINT_PENDING_INTERRUPT /* For explorer. good luck! */

#ifdef TX39ICUDEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif
u_int32_t tx39intrvec; /* debug use */

/* IRQHIGH lines list */
static struct irqhigh_list {
	int qh_pri; /* IRQHIGH priority */
	int qh_set; /* Register set */
	int qh_bit; /* bit offset in the register set */
} irqhigh_list[] = {
	{15,	5,	25},	/* POSPWROKINT */
	{15,	5,	24},	/* NEGPWROKINT */
	{14,	5,	30},	/* ALARMINT*/
	{13,	5,	29},	/* PERINT */
#ifdef TX391X
	{12,	2,	3},	/* MBUSPOSINT */
	{12,	2,	2},	/* MBUSNEGINT */
	{11,	2,	31},	/* UARTARXINT */
	{10,	2,	21},	/* UARTBRXINT */
	{9,	3,	19},	/* MFIOPOSINT19 */
	{9,	3,	18},	/* MFIOPOSINT18 */
	{9,	3,	17},	/* MFIOPOSINT17 */
	{9,	3,	16},	/* MFIOPOSINT16 */
	{8,	3,	1},	/* MFIOPOSINT1 */
	{8,	3,	0},	/* MFIOPOSINT0 */
	{8,	5,	13},	/* IOPOSINT6 */
	{8,	5,	12},	/* IOPOSINT5 */
	{7,	4,	19},	/* MFIONEGINT19 */
	{7,	4,	18},	/* MFIONEGINT18 */
	{7,	4,	17},	/* MFIONEGINT17 */
	{7,	4,	16},	/* MFIONEGINT16 */
	{6,	4,	1},	/* MFIONEGINT1 */
	{6,	4,	0},	/* MFIONEGINT0 */
	{6,	5,	6},	/* IONEGINT6 */
	{6,	5,	5},	/* IONEGINT5 */
	{5,	2,	5},	/* MBUSDMAFULLINT */
#endif /* TX391X */
#ifdef TX392X
	{12,	2,	31},	/* UARTARXINT */
	{12,	2,	21},	/* UARTBRXINT */
	{11,	3,	19},	/* MFIOPOSINT19 */
	{11,	3,	18},	/* MFIOPOSINT18 */
	{11,	3,	17},	/* MFIOPOSINT17 */
	{11,	3,	16},	/* MFIOPOSINT16 */
	{10,	3,	1},	/* MFIOPOSINT1 */
	{10,	3,	0},	/* MFIOPOSINT0 */
	{10,	5,	13},	/* IOPOSINT6 */
	{10,	5,	12},	/* IOPOSINT5 */
	{9,	4,	19},	/* MFIONEGINT19 */
	{9,	4,	18},	/* MFIONEGINT18 */
	{9,	4,	17},	/* MFIONEGINT17 */
	{9,	4,	16},	/* MFIONEGINT16 */
	{8,	4,	1},	/* MFIONEGINT1 */
	{8,	4,	0},	/* MFIONEGINT0 */
	{8,	5,	6},	/* IONEGINT6 */
	{8,	5,	5},	/* IONEGINT5 */
	{5,	7,	19},	/* IRRXCINT */
	{5,	7,	17},	/* IRRXEINT */
#endif /* TX392X */
	{4,	1,	18},	/* SNDDMACNTINT */
	{3,	1,	17},	/* TELDMACNTINT */
	{2,	1,	27},	/* CHIDMACNTINT */
	{1,	5,	7},	/* IOPOSINT0 */
	{1,	5,	0}	/* IONEGINT0 */
};

struct txintr_high_entry {
	int	he_set;
	txreg_t	he_mask;
	int	(*he_fun) __P((void*));
	void	*he_arg;
	TAILQ_ENTRY(txintr_high_entry) he_link;
};

#ifdef USE_POLL
struct txpoll_entry{
	int	p_cnt; /* dispatch interval */
	int	p_desc;
	int	(*p_fun) __P((void*));
	void	*p_arg;
	TAILQ_ENTRY(txpoll_entry) p_link;	
};
int	tx39_poll_intr __P((void*));
#endif /* USE_POLL */

struct tx39icu_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
	/* IRQLOW */
	txreg_t	sc_le_mask[TX39_INTRSET_MAX + 1];
	int	(*sc_le_fun[TX39_INTRSET_MAX + 1][32]) __P((void*));
	void	*sc_le_arg[TX39_INTRSET_MAX + 1][32];
	/* IRQHIGH */
	TAILQ_HEAD(, txintr_high_entry) sc_he_head[TX39_IRQHIGH_MAX];
	/* Register */
	txreg_t sc_regs[TX39_INTRSET_MAX + 1];
#ifdef USE_POLL
	unsigned sc_pollcnt;
	int	sc_polling;
	void	*sc_poll_ih;
	TAILQ_HEAD(, txpoll_entry) sc_p_head;
#endif /* USE_POLL */
};

int	tx39icu_match __P((struct device*, struct cfdata*, void*));
void	tx39icu_attach __P((struct device*, struct device*, void*));
int	tx39icu_intr __P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));

void	tx39_intr_dump __P((struct tx39icu_softc*));
void	tx39_intr_decode __P((int, int*, int*));
void	tx39_irqhigh_disestablish __P((tx_chipset_tag_t, int, int, int));
void	tx39_irqhigh_establish __P((tx_chipset_tag_t, int, int, int, int (*) __P((void*)), void*));
void	tx39_irqhigh_intr __P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));
int	tx39_irqhigh __P((int, int));

struct cfattach tx39icu_ca = {
	sizeof(struct tx39icu_softc), tx39icu_match, tx39icu_attach
};

int
tx39icu_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group of txsim */
}

void
tx39icu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39icu_softc *sc = (void*)self;
	tx_chipset_tag_t tc = ta->ta_tc;
	txreg_t reg;
	int i;
	
	printf("\n");
	sc->sc_tc = ta->ta_tc;


	printf("\t[Windows CE setting]\n");
	sc->sc_regs[0] = tx_conf_read(tc, TX39_INTRSTATUS6_REG);
	sc->sc_regs[1] = tx_conf_read(tc, TX39_INTRSTATUS1_REG);
	sc->sc_regs[2] = tx_conf_read(tc, TX39_INTRSTATUS2_REG);
	sc->sc_regs[3] = tx_conf_read(tc, TX39_INTRSTATUS3_REG);
	sc->sc_regs[4] = tx_conf_read(tc, TX39_INTRSTATUS4_REG);
	sc->sc_regs[5] = tx_conf_read(tc, TX39_INTRSTATUS5_REG);
#ifdef TX392X
	sc->sc_regs[7] = tx_conf_read(tc, TX39_INTRSTATUS7_REG);
	sc->sc_regs[8] = tx_conf_read(tc, TX39_INTRSTATUS8_REG);
#endif
	tx39_intr_dump(sc);
#ifdef WINCE_DEFAULT_SETTING
#warning WINCE_DEFAULT_SETTING
#else /* WINCE_DEFAULT_SETTING */
	/* Disable IRQLOW */
	tx_conf_write(tc, TX39_INTRENABLE1_REG, 0); 
	tx_conf_write(tc, TX39_INTRENABLE2_REG, 0); 
	tx_conf_write(tc, TX39_INTRENABLE3_REG, 0); 
	tx_conf_write(tc, TX39_INTRENABLE4_REG, 0); 
	tx_conf_write(tc, TX39_INTRENABLE5_REG, 0); 
#ifdef TX392X
	tx_conf_write(tc, TX39_INTRENABLE7_REG, 0); 	
	tx_conf_write(tc, TX39_INTRENABLE8_REG, 0); 
#endif /* TX392X */
	
	/* Disable IRQHIGH */
	reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	reg &= ~TX39_INTRENABLE6_PRIORITYMASK_MASK;
	tx_conf_write(tc, TX39_INTRENABLE6_REG, reg); 
#endif /* WINCE_DEFAULT_SETTING */

	/* Clear all pending interrupts */
	tx_conf_write(tc, TX39_INTRCLEAR1_REG, tx_conf_read(tc, TX39_INTRSTATUS1_REG));
	tx_conf_write(tc, TX39_INTRCLEAR2_REG, tx_conf_read(tc, TX39_INTRSTATUS2_REG));
	tx_conf_write(tc, TX39_INTRCLEAR3_REG, tx_conf_read(tc, TX39_INTRSTATUS3_REG));
	tx_conf_write(tc, TX39_INTRCLEAR4_REG, tx_conf_read(tc, TX39_INTRSTATUS4_REG));
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, tx_conf_read(tc, TX39_INTRSTATUS5_REG));
#ifdef TX392X
	tx_conf_write(tc, TX39_INTRCLEAR7_REG, tx_conf_read(tc, TX39_INTRSTATUS7_REG));
	tx_conf_write(tc, TX39_INTRCLEAR8_REG, tx_conf_read(tc, TX39_INTRSTATUS8_REG));
#endif /* TX392X */

	/* Enable global interrupts */
	reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	reg |= TX39_INTRENABLE6_GLOBALEN;	
	tx_conf_write(tc, TX39_INTRENABLE6_REG, reg);

	/* Initialize IRQHIGH interrupt handler holder*/
	for (i = 0; i < TX39_IRQHIGH_MAX; i++) {
		TAILQ_INIT(&sc->sc_he_head[i]);
	}
#ifdef USE_POLL
	/* Initialize polling handler holder */
	TAILQ_INIT(&sc->sc_p_head);
#endif /* USE_POLL */
	
	/* Register interrupt module myself */
	tx_conf_register_intr(tc, self);
}

int
tx39icu_intr(mask, pc, status, cause)
	u_int32_t mask;
	u_int32_t pc;
	u_int32_t status;
	u_int32_t cause;
{
	struct tx39icu_softc *sc;
	tx_chipset_tag_t tc;
	txreg_t reg, pend;
	int i, j;

	tc = tx_conf_get_tag();
	sc = tc->tc_intrt;
	/*
	 * Read regsiter ASAP
	 */
	sc->sc_regs[0] = tx_conf_read(tc, TX39_INTRSTATUS6_REG);
	sc->sc_regs[1] = tx_conf_read(tc, TX39_INTRSTATUS1_REG);
	sc->sc_regs[2] = tx_conf_read(tc, TX39_INTRSTATUS2_REG);
	sc->sc_regs[3] = tx_conf_read(tc, TX39_INTRSTATUS3_REG);
	sc->sc_regs[4] = tx_conf_read(tc, TX39_INTRSTATUS4_REG);
	sc->sc_regs[5] = tx_conf_read(tc, TX39_INTRSTATUS5_REG);
#ifdef TX392X
	sc->sc_regs[7] = tx_conf_read(tc, TX39_INTRSTATUS7_REG);
	sc->sc_regs[8] = tx_conf_read(tc, TX39_INTRSTATUS8_REG);
#endif

#ifdef TX39ICUDEBUG
	if (!(mask & MIPS_INT_MASK_4) && !(mask & MIPS_INT_MASK_2)) {
		bitdisp(mask);
		panic("bogus HwInt");
	}
#ifdef TX39_DEBUG
	if (tx39debugflag) {
		tx39_intr_dump(sc);
	}
#endif
#endif /* TX39ICUDEBUG */

	/* IRQHIGH */
	if (mask & MIPS_INT_MASK_4) {
		tx39_irqhigh_intr(mask, pc, status, cause);
	}

	/* IRQLOW */
	if (mask & MIPS_INT_MASK_2) {
		for (i = 1; i <= TX39_INTRSET_MAX; i++) {
			int ofs;
#ifdef TX392X
			if (i == 6)
				continue;
#endif /* TX392X */
			ofs = TX39_INTRSTATUS_REG(i);
			pend = sc->sc_regs[i];
			reg = sc->sc_le_mask[i] & pend;
			/* Clear interrupts */
			tx_conf_write(tc, ofs, reg);
			/* Dispatch handler */
			for (j = 0 ; j < 32; j++) {
				if ((reg & (1 << j)) &&
				    sc->sc_le_fun[i][j]) {
#ifdef TX39_DEBUG
					tx39intrvec = (i << 16) | j;
					if (tx39debugflag) {
						DPRINTF(("IRQLOW %d:%d\n", 
							 i, j));
					}
#endif /* TX39_DEBUG */
					(*sc->sc_le_fun[i][j])
						(sc->sc_le_arg[i][j]);

				}
			}
#ifdef TX39ICUDEBUG_PRINT_PENDING_INTERRUPT
			pend &= ~reg;
			if (pend) {
				printf("%d pending:", i);
				__bitdisp(pend, 0, 31, 0, 1);
			}
#endif

		}
	}
#ifdef TX39_WATCHDOGTIMER
	/* Bus error (If watch dog timer is enabled)*/
	if (mask & MIPS_INT_MASK_1) {
		tx39biu_intr(0); /* Clear bus error */
	}
#endif
	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}

int
tx39_irqhigh(set, bit)
	int set, bit;
{
	int i, n;

	n = sizeof irqhigh_list / sizeof (struct irqhigh_list);
	for (i = 0; i < n; i++) {
		if (irqhigh_list[i].qh_set == set &&
		    irqhigh_list[i].qh_bit == bit)
			return irqhigh_list[i].qh_pri;
	}

	return 0;
}

void
tx39_irqhigh_intr(mask, pc, status, cause)
	u_int32_t mask;
	u_int32_t pc;
	u_int32_t status;
	u_int32_t cause;
{
	struct txintr_high_entry *he;
	struct tx39icu_softc *sc;
	struct clockframe cf;
	tx_chipset_tag_t tc;
	int i, pri, ofs, set;
	txreg_t he_mask;

	tc = tx_conf_get_tag();
	sc = tc->tc_intrt;
	pri = TX39_INTRSTATUS6_INTVECT(sc->sc_regs[0]);

	if (pri == TX39_INTRPRI13_TIMER_PERIODIC) {
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		tx_conf_write(tc, TX39_INTRCLEAR5_REG, 
			      TX39_INTRSTATUS5_PERINT);
	}
	/* Handle all pending IRQHIGH interrupts */
	for (i = pri; i > 0; i--) {
		TAILQ_FOREACH(he, &sc->sc_he_head[i], he_link) {
			set = he->he_set;
			he_mask = he->he_mask;
			if (he_mask & (sc->sc_regs[set])) {
				ofs = TX39_INTRSTATUS_REG(set);
				/* Clear interrupt */
				tx_conf_write(tc, ofs, he_mask);
#ifdef TX39_DEBUG
				tx39intrvec = (set << 16) | 
					(ffs(he_mask) - 1);
				if (tx39debugflag) {
					DPRINTF(("IRQHIGH: %d:%d\n", 
						 set, ffs(he_mask) - 1));
				}
#endif /* TX39_DEBUG */
				/* Dispatch handler */
				(*he->he_fun)(he->he_arg);
			}
		}
	}
}

void
tx39_intr_decode(intr, set, bit)
	int intr;
	int *set;
	int *bit;
{
	if (!intr || intr >= (TX39_INTRSET_MAX + 1) * 32
#ifdef TX392X
	    || intr == 6
#endif /* TX392X */
		) {
		panic("tx39icu_decode: bogus intrrupt line. %d", intr);
	}
	*set = intr / 32;
	*bit = intr % 32;
}

void
tx39_irqhigh_establish(tc, set, bit, pri, ih_fun, ih_arg)
	tx_chipset_tag_t tc;
	int set;
	int bit;
	int pri;
	int (*ih_fun) __P((void*));
	void *ih_arg;
{
	struct tx39icu_softc *sc;
	struct txintr_high_entry *he;
	txreg_t reg;

	sc = tc->tc_intrt;
	/*
	 *	Add new entry to `pri' priority.
	 */
	if (!(he = malloc(sizeof(struct txintr_high_entry), 
			  M_DEVBUF, M_NOWAIT))) {
		panic ("tx39_irqhigh_establish: no memory.");
	}
	memset(he, 0, sizeof(struct txintr_high_entry));
	he->he_set = set;
	he->he_mask= (1 << bit);
	he->he_fun = ih_fun;
	he->he_arg = ih_arg;
	TAILQ_INSERT_TAIL(&sc->sc_he_head[pri], he, he_link);
	/*
	 *	Enable interrupt on this priority.
	 */
	reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	reg = TX39_INTRENABLE6_PRIORITYMASK_SET(reg, (1 << pri));
	tx_conf_write(tc, TX39_INTRENABLE6_REG, reg);		
}

void
tx39_irqhigh_disestablish(tc, set, bit, pri)
	tx_chipset_tag_t tc;
	int set, bit, pri;
{
	struct tx39icu_softc *sc;
	struct txintr_high_entry *he;
	txreg_t reg;

	sc = tc->tc_intrt;
	TAILQ_FOREACH(he, &sc->sc_he_head[pri], he_link) {
		if (he->he_set == set && he->he_mask == (1 << bit)) {
			TAILQ_REMOVE(&sc->sc_he_head[pri], he, he_link);
			free(he, M_DEVBUF);
			break;
		}
	}
	
	if (TAILQ_EMPTY(&sc->sc_he_head[pri])) {
		reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
		reg &= ~(1 << pri);
		tx_conf_write(tc, TX39_INTRENABLE6_REG, reg);		
	}
}


void*
tx_intr_establish(tc, line, mode, level, ih_fun, ih_arg)
	tx_chipset_tag_t tc;
	int line;
	int mode;  /* Trigger setting. but TX39 handles edge only. */
	int level; /* XXX not yet */
	int (*ih_fun) __P((void*));
	void *ih_arg;
{
	struct tx39icu_softc *sc;
	txreg_t reg;
	int bit, set, highpri, ofs;

	sc = tc->tc_intrt;

	tx39_intr_decode(line, &set, &bit);

	sc->sc_le_fun[set][bit] = ih_fun;
	sc->sc_le_arg[set][bit] = ih_arg;
	DPRINTF(("tx_intr_establish: %d:%d", set, bit));

	if ((highpri = tx39_irqhigh(set, bit))) {
		tx39_irqhigh_establish(tc, set, bit, highpri, 
				       ih_fun, ih_arg);
		DPRINTF(("(high)\n"));
	} else {
		/* Set mask for acknowledge. */
		sc->sc_le_mask[set] |= (1 << bit);
		/* Enable interrupt */
		ofs = TX39_INTRENABLE_REG(set);
		reg = tx_conf_read(tc, ofs);
		reg |= (1 << bit);
		tx_conf_write(tc, ofs, reg);
		DPRINTF(("(low)\n"));
	}
	
	return (void*)line;
}

void
tx_intr_disestablish(tc, arg)
	tx_chipset_tag_t tc;
	void *arg;
{
	struct tx39icu_softc *sc;
	int set, bit, highpri, ofs;
	txreg_t reg;

	sc = tc->tc_intrt;

	tx39_intr_decode((int)arg, &set, &bit);
	DPRINTF(("tx_intr_disestablish: %d:%d", set, bit));

	if ((highpri = tx39_irqhigh(set, bit))) {
		tx39_irqhigh_disestablish(tc, set, bit, highpri);
		DPRINTF(("(high)\n"));
	} else {
		sc->sc_le_fun[set][bit] = 0;
		sc->sc_le_arg[set][bit] = 0;
		sc->sc_le_mask[set] &= ~(1 << bit);
		ofs = TX39_INTRENABLE_REG(set);
		reg = tx_conf_read(tc, ofs);
		reg &= ~(1 << bit);
		tx_conf_write(tc, ofs, reg);
		DPRINTF(("(low)\n"));
	}
}

void
tx39_intr_dump(sc)
	struct tx39icu_softc *sc;
{
	tx_chipset_tag_t tc = sc->sc_tc;
	int i, j, ofs;
	txreg_t reg;
	char msg[16];

	for (i = 1; i <= TX39_INTRSET_MAX; i++) {
#ifdef TX392X		
		if (i == 6)
			continue;
#endif /* TX392X */
		for (reg = j = 0; j < 32; j++) {
			if (tx39_irqhigh(i, j)) {
				reg |= (1 << j);
			}
		}
		sprintf(msg, "%d high", i);
		__bitdisp(reg, 32, 0, msg, 1);
		sprintf(msg, "%d status", i);
		__bitdisp(sc->sc_regs[i], 0, 31, msg, 1);
		ofs = TX39_INTRENABLE_REG(i);
		reg = tx_conf_read(tc, ofs);
		sprintf(msg, "%d enable", i);
		__bitdisp(reg, 0, 31, msg, 1);
	}
	reg = sc->sc_regs[0];
	printf("<%s><%s> vector=%2d\t\t[6 status]\n",
	       reg & TX39_INTRSTATUS6_IRQHIGH ? "HI" : "--",
	       reg & TX39_INTRSTATUS6_IRQLOW ? "LO" : "--",
	       TX39_INTRSTATUS6_INTVECT(reg));
	reg = tx_conf_read(tc, TX39_INTRENABLE6_REG);
	__bitdisp(reg, 0, 18, "6 enable", 1);

}

#ifdef USE_POLL
void*
tx39_poll_establish(tc, interval, mode, level, ih_fun, ih_arg)
	tx_chipset_tag_t tc;
	int interval;
	int mode;  /* Trigger setting. but TX39 handles edge only. */
	int level; /* XXX not yet */
	int (*ih_fun) __P((void*));
	void *ih_arg;
{
	struct tx39icu_softc *sc;
	struct txpoll_entry *p;

	sc = tc->tc_intrt;

	if (!(p = malloc(sizeof(struct txpoll_entry), 
			 M_DEVBUF, M_NOWAIT))) {
		panic ("tx39_poll_establish: no memory.");
	}
	memset(p, 0, sizeof(struct txpoll_entry));

	p->p_fun = ih_fun;
	p->p_arg = ih_arg;
	p->p_cnt = interval;
	if (!sc->sc_polling) {
		/* Hook VSync : TX39_INTRSTATUS1_LCDINT*/
		if (!(sc->sc_poll_ih = 
		      tx_intr_establish(
			      tc, MAKEINTR(1, TX39_INTRSTATUS1_LCDINT),
			      mode, level, tx39_poll_intr, sc)))  {
			printf("tx39_poll_establish: can't hook\n");
			return 0;
		}
	}
	sc->sc_polling++;
	p->p_desc = sc->sc_polling;
	TAILQ_INSERT_TAIL(&sc->sc_p_head, p, p_link);
	
	return (void*)p->p_desc;
}

void
tx39_poll_disestablish(tc, arg)
	tx_chipset_tag_t tc;
	void *arg;
{
	struct tx39icu_softc *sc;
	struct txpoll_entry *p;
	int desc;
	sc = tc->tc_intrt;

	desc = (int)arg;
	TAILQ_FOREACH(p, &sc->sc_p_head, p_link) {
		if (p->p_desc == desc) {
			TAILQ_REMOVE(&sc->sc_p_head, p, p_link);
			free(p, M_DEVBUF);
			break;
		}
	}
	if (TAILQ_EMPTY(&sc->sc_p_head)) {
		sc->sc_polling = 0;
		tx_intr_disestablish(tc, sc->sc_poll_ih);
	}
}

int
tx39_poll_intr(arg)
	void *arg;
{
	struct tx39icu_softc *sc = arg;
	struct txpoll_entry *p;

	if (!sc->sc_polling) {
		return 0;
	}
	sc->sc_pollcnt++;
	TAILQ_FOREACH(p, &sc->sc_p_head, p_link) {
		if (sc->sc_pollcnt % p->p_cnt == 0) {
			(*p->p_fun)(p->p_arg);
		}
	}
	return 0;
}
#endif /* USE_POLL */
