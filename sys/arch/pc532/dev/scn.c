/*	$NetBSD: scn.c,v 1.35 1997/03/01 09:50:44 matthias Exp $ */

/*
 * Copyright (c) 1996 Philip L. Budne.
 * Copyright (c) 1993 Philip A. Nelson.
 * Copyright (c) 1991, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Portions of this software were developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA
 * contract BG 91-66 and contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include "scn.h"

#if NSCN > 0
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/icu.h>

#include "scnreg.h"
#include "scnvar.h"

int scn_soft_rts = 1;
static const char scnints[4] = {IR_TTY0, IR_TTY1, IR_TTY2, IR_TTY3};
static const char rxints[4] = {IR_TTY0RDY, IR_TTY1RDY, IR_TTY2RDY, IR_TTY3RDY};

int     scnprobe __P((struct device *, struct cfdata *, void *));
void    scnattach __P((struct device *, struct device *, void *));
int     scnparam __P((struct tty *, struct termios *));
void    scnstart __P((struct tty *));
int     scnopen __P((dev_t, int, int, struct proc *));
int     scnclose __P((dev_t, int, int, struct proc *));
int	scncnprobe __P((struct consdev *));
void	scncninit __P((struct consdev *));
int     scncngetc __P((dev_t));
void    scncnputc __P((dev_t, int));
void	scncnpollc __P((dev_t, int));
int	scninit __P((dev_t, int));
void	scncnreinit __P((void *));
int     scnhwiflow __P((struct tty *, int));

struct cfattach scn_ca = {sizeof(struct scn_softc), scnprobe, scnattach};
struct cfdriver scn_cd = {NULL, "scn", DV_TTY, NULL, 0};
#ifndef CONSOLE_SPEED
#define CONSOLE_SPEED TTYDEF_SPEED
#endif

#ifndef SCNDEF_CFLAG
#define SCNDEF_CFLAG TTYDEF_CFLAG
#endif

#ifdef CPU30MHZ
#define RECOVER()	__asm __volatile("bispsrw 0x800" : : : "cc")
#else
#define RECOVER()
#endif

int     scnconsole = SCN_CONSOLE;
int     scndefaultrate = TTYDEF_SPEED;
int     scnconsrate = CONSOLE_SPEED;
int     scnmajor;

#define SOFTC(UNIT) scn_cd.cd_devs[(UNIT)]

static void scnintr __P((void *));
static void scnrxintr __P((void *));
static int scn_rxintr __P((struct scn_softc *, int));
static void scnsoft __P((void *));

static int scnsir = -1;		/* s/w intr number */
#define setsoftscn()	softintr(scnsir)

#ifdef SCN_TIMING
/*
 * Keep timing info on latency of software interrupt used by
 * the ringbuf code to empty ring buffer.
 * "getinfo" program reads data from /dev/kemm.
 */
static struct timeval tstart;
#define NJITTER 100
int     scn_njitter = NJITTER;
int     scn_jitter[NJITTER];
#endif

#define SCN_CLOCK	3686400		/* input clock */

/*
 * Make some use of the counter/timer;
 * one speed can be chosen for all ports at compile time,
 * this can either be a rate not in the BRG table, or a rate
 * available on only one group (to make the rate available at
 * all times).  It would be more clever to use the C/T dynamicly
 * to generate rates on request, but this is a start!! -plb 6/2/94
 *
 * Could supply per channel in config file, or via defines
 */
#ifndef SCN_CT_SPEED
#define SCN_CT_SPEED 57600
#endif

/*
 * Timer supplies 16x clock; two timer cycles needed for each clock
 * and minimum counter/timer value for 2681 is 2 (highest possible
 * rate is 57600).  The 2692 is supposed to be able to handle a
 * counter value of 1 (115200 bps).
 */

/*
 * XXX check CT_VALUE, rather than input??
 * XXX check that CT_VALUE multiplied out generates input rate??? (+/- epsilon)
 */

#ifdef SCN_2692
#define SCN_CT_MAXSPEED 115200
#else
#define SCN_CT_MAXSPEED 57600
#endif

#if SCN_CT_SPEED > SCN_CT_MAXSPEED
#undef SCN_CT_SPEED
#define SCN_CT_SPEED SCN_CT_MAXSPEED
#endif

/* round up */
#define CT_VALUE ((SCN_CLOCK/16/2+SCN_CT_SPEED-1)/SCN_CT_SPEED)

struct speedtab scnspeedtab[] = {
	{0, 0x40},		/* code for line-hangup */
#ifdef SCN_CT_SPEED
	/* before any other speeds, since any C/T speed available in both
	 * speed groups!! */
	{SCN_CT_SPEED, SP_BOTH + 13},	/* timer (16x clock) */
#endif
	{50, SP_GRP0 + 0},
	{75, SP_GRP1 + 0},
	{110, SP_BOTH + 1},
	{134, SP_BOTH + 2},		/* 134.5 */
	{150, SP_GRP1 + 3},
	{200, SP_GRP0 + 3},
	{300, SP_BOTH + 4},
	{600, SP_BOTH + 5},
	{1050, SP_GRP0 + 7},
	{1200, SP_BOTH + 6},
	{1800, SP_GRP1 + 10},
	{2000, SP_GRP1 + 7},
	{2400, SP_BOTH + 8},
	{4800, SP_BOTH + 9},
	{7200, SP_GRP0 + 10},
	{9600, SP_BOTH + 11},
	{19200, SP_GRP1 + 12},
	{38400, SP_GRP0 + 12},
/*	{115200, SP_BOTH + 14},		/* 11/4/95; dave says CLK/2 on IP4! */
	{-1, -1}
};

/*
 * minor unit bit decode:
 * CxxxUUU
 *
 * C - carrier
 *	0 - delay open until carrier high
 *	1 - allow open with carrier low
 * UUU - unit 0-7
 */

#define DEV_UNIT(x)	(minor(x) & 0x7)
#define DEV_DIALOUT(x)	(minor(x) & 0x80)

extern struct tty *constty;

#ifdef KGDB
extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
int     scnkgdb = -1;
#endif

#if 0
/* Debug routine to print out the scn line structures. */
void
print_scn(unit)
	int unit;
{
	struct scn_softc *sc = SOFTC(unit);
	int channel = unit & 1;
	printf("\nline frame overrun parity break\n");
	printf("tty%1d f=%2d o=%2d p=%2d b=%2d in=%x, out=%x grp=%d\n",
	    unit, sc->framing_errors, sc->overrun_errors,
	    sc->parity_errors, sc->break_interrupts,
	    sc->duart->i_speed[channel], sc->duart->o_speed[channel],
	    sc->duart->speed_grp);
}
#endif

static struct speedtab *
getspeedcode(sc, speed)
	struct scn_softc *sc;
	int speed;
{
	register struct speedtab *sp;

	/*
	 * XXX could check here if rate matches per duart counter timer rate
	 * (from config, defines or dynamic) and return group code 13
	 */

	for (sp = scnspeedtab; sp->sp_speed != -1; sp++)
		if (sp->sp_speed == speed)
			return sp;
	return NULL;
}
/* Set various line control parameters for RS232 I/O. */
static int
scn_config(unit, in_speed, out_speed, new_mr1, new_mr2)
	int unit;	/* which rs line */
	int in_speed;	/* input speed: 110, 300, 1200, etc */
	int out_speed;	/* output speed: 110, 300, 1200, etc */
	char new_mr1;	/* new bits for MR1 */
	char new_mr2;	/* new bits for MR2 */

{
	register struct scn_softc *sc;
	char mr1_val, mr2_val;
	struct speedtab *sp;
	int sp_grp, sp_both;
	char set_speed;		/* Non zero if we need to set the speed. */
	int channel;		/* Used for ease of access. */
	int in_code;
	int out_code;
	int s;

	/* Get the speed codes. */
	sp = getspeedcode(sc, in_speed);
	/* XXX allow random speeds using C/T!! */
	if (sp == NULL)
		return (EINVAL);
	in_code = sp->sp_code;

	sp = getspeedcode(sc, out_speed);
	/* XXX allow random speeds using C/T!! */
	if (sp == NULL)
		return (EINVAL);
	out_code = sp->sp_code;

	/* Set up softc pointer. */
	if (unit >= scn_cd.cd_ndevs)
		return ENXIO;
	sc = SOFTC(unit);
	channel = unit & 1;

	/*
         * Check out the Speeds;
         *
         * There are two groups of speeds.
         *
         * If the new speeds are not in the same group, or the other line
         * is not the same speed of the other group, do not change the
         * speeds.
         *
         * Also, if the in speed and the out speed are in different
         * groups, use the in speed.
         */

	set_speed = FALSE;
	sp_grp = 0;		/* keep gcc quiet */
	if ((in_code != sc->duart->i_code[channel]) ||
	    (out_code != sc->duart->o_code[channel])) {
		/* We need to set the speeds. */
		set_speed = TRUE;

		if (((in_code & SP_GRP1) != (out_code & SP_GRP1)) &&
		    (((in_code | out_code) & SP_BOTH) != SP_BOTH)) {
			/* Input speed and output speed are different groups. */
			return (EINVAL);
		}
		sp_grp = (in_code | out_code) & SP_GRP1;
		sp_both = in_code & out_code & SP_BOTH;

		/* Check for compatibility and set the uart values */
		if (sp_both) {
			sp_grp = sc->duart->speed_grp;
		} else {
			if ((sp_grp != sc->duart->speed_grp) &&
			    !(sc->duart->i_code[1 - channel] &
				sc->duart->o_code[1 - channel] & SP_BOTH)) {
				/*
				 * Can't change group, don`t change the speed
				 * rates.
				 */
				return (EINVAL);
			}
		}
		sc->duart->i_code[channel] = in_code;
		sc->duart->o_code[channel] = out_code;
		sc->duart->i_speed[channel] = in_speed;
		sc->duart->o_speed[channel] = out_speed;
	}
	/*
	 * Add in any bits to new_mr1/2 that we don't want the caller to have
	 * to know about (ie; interrupt settings) here -plb
	 *
	 * XXX wait for any any output in progress (avoid noise when
	 * reprogramming line (like when logging in)! -plb
	 *
	 * Lock out interrupts while setting the parameters.
	 * (Just for safety.)
	 */
	s = spltty();
	sc->chbase[CH_CR] = CR_CMD_MR1;
	RECOVER();
	mr1_val = sc->chbase[CH_MR];
	mr2_val = sc->chbase[CH_MR];
	if (mr1_val != new_mr1 || mr2_val != new_mr2) {
		sc->chbase[CH_CR] = CR_CMD_MR1;
		RECOVER();
		sc->chbase[CH_MR] = new_mr1;
		sc->chbase[CH_MR] = new_mr2;
	}
	if (set_speed) {
		if (sc->duart->speed_grp != sp_grp) {
			/* Change the group! */
			char    acr;

			sc->duart->speed_grp = sp_grp;
			acr = sc->duart->acr_bits;
			if (sp_grp)	/* group1? */
				acr |= ACR_BRG;	/* yes; select alternate
						 * group!! */
			sc->duart->base[DU_ACR] = acr;
		}
		sc->chbase[CH_CSR] = ((in_code & 0xf) << 4) | (out_code & 0xf);
	}
	DELAY((96000 / out_speed) * 10000);	/* with tty ints off?!!! */
	splx(s);

	return (0);
}

int
scnprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;
	int unit = cf->cf_unit;
	int mr1;
	register volatile u_char *ch_base;

	/* The pc532 doesn't have more then 8 lines. */
	if (unit >= 8) return(0);

	/* Now some black magic that should detect a scc26x2 channel. */
	ch_base = (volatile u_char *) SCN_FIRST_MAP_ADR + CH_SZ * unit;
	ch_base[CH_CR] = CR_CMD_RESET_ERR;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_RESET_BRK;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_MR1;
	RECOVER();
	mr1 = ch_base[CH_MR] ^ 0x80;
	ch_base[CH_CR] = CR_CMD_MR1;
	RECOVER();
	ch_base[CH_MR] = mr1;
	ch_base[CH_CR] = CR_CMD_MR1;
	RECOVER();
	if (ch_base[CH_MR] != mr1)
		return(0);
	if (ch_base[CH_MR] == mr1)
		return(0);

	ca->ca_addr = (int)ch_base;
	if (unit & 1)
		ca->ca_irq = -1;
	else
		ca->ca_irq = scnints[unit >> 1] | (rxints[unit >> 1] << 4);

	return(1);
}

/*
 * No need to make scn_rx{en,dis}able too efficient,
 * they're only called on setup, open & close!
 */
static __inline void
scn_rxenable(sc)
	struct scn_softc *sc;
{
	struct duart_info *dp;
	int channel;

	dp = sc->duart;
	channel = sc->unit & 1;

	/* Outputs wire-ored and connected to ICU input for fast rx interrupt. */
	if (channel == 0)
		dp->opcr_bits |= OPCR_OP4_RXRDYA;
	else
		dp->opcr_bits |= OPCR_OP5_RXRDYB;
	dp->base[DU_OPCR] = dp->opcr_bits;
}

static __inline void
scn_rxdisable(sc)
	struct scn_softc *sc;
{
	struct duart_info *dp;
	int channel;

	dp = sc->duart;
	channel = sc->unit & 1;

	/* Outputs wire-ored and connected to ICU input for fast rx interrupt. */
	if (channel == 0)
		dp->opcr_bits &= ~OPCR_OP4_RXRDYA;
	else
		dp->opcr_bits &= ~OPCR_OP5_RXRDYB;
	dp->base[DU_OPCR] = dp->opcr_bits;
}

void
scnattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void   *aux;
{
	struct confargs *ca = aux;
	register struct scn_softc *sc;
	register volatile u_char *ch_base;
	register volatile u_char *duart_base;
	long scn_first_adr;
	int channel;
	int speed;
	int s;
	u_char unit;
	u_char duart;
	u_char delim = ':';
	u_char duart_flags = 0;

	sc = (void *) self;
	unit = self->dv_unit;	/* sc->scn_dev.dv_unit ??? */

	duart = unit >> 1;	/* get chip number */
	channel = unit & 1;	/* get channel on chip */

	/* pick up "flags" (SCN_xxx) from config file */
	if (self->dv_cfdata)	/* paranoia */
		sc->scn_swflags = ca->ca_flags;
	else
		sc->scn_swflags = 0;

	if (unit == scnconsole)	/* console? */
		sc->scn_swflags |= SCN_SW_SOFTCAR;	/* ignore carrier */

	/*
         * Precalculate port numbers for speed.
         * Magic numbers in the code (once).
         *
         * XXX get physical addr from "cfdata" (put there from config file)
         * and pass to a map_regs() routine which returns virtual (mapped) addr?
         */
	scn_first_adr = SCN_FIRST_MAP_ADR;	/* to get around a gcc bug. */

	ch_base = (volatile u_char *) scn_first_adr + CH_SZ * unit;
	duart_base = (volatile u_char *) scn_first_adr + DUART_SZ * duart;

	if (channel == 0) {
		char *duart_type;
		/* Probe DUART type */
		unsigned char mr1, mr2;
		s = spltty();
		if(unit == scnconsole) {
			ch_base[CH_CR] = CR_DIS_TX;
			delay(5 * 10000);
		}
		ch_base[CH_CR] = CR_CMD_MR1;
		RECOVER();
		mr1 = ch_base[CH_MR];
		mr2 = ch_base[CH_MR];
		ch_base[CH_CR] = CR_CMD_MR1;
		RECOVER();
		ch_base[CH_MR] = 1;
		ch_base[CH_MR] = 0;
		ch_base[CH_CR] = CR_CMD_MR1;
		RECOVER();
		if (ch_base[CH_MR] == 1) {
			/* MR 2 selected */
			ch_base[CH_CR] = CR_CMD_MR0;
			RECOVER();
			/* if 2681, MR2 still selected */
			ch_base[CH_MR] = 1;
			ch_base[CH_CR] = CR_CMD_MR1;
			RECOVER();
			ch_base[CH_MR] = 0; /* MR1 */
			ch_base[CH_MR] = 0; /* MR2 */
			ch_base[CH_CR] = CR_CMD_MR0;
			RECOVER();
			/* if 2681, MR2 still selected */
			if((ch_base[CH_MR] & 1) == 1) {
				duart_type = "sc26c92";
				duart_flags |= SCN_HW_26C92;
			} else {
				ch_base[CH_CR] = CR_CMD_RTS_OFF; /* 2681 treats as MR1 Select */
				RECOVER();
				ch_base[CH_MR] = 1;
				ch_base[CH_MR] = 0;
				ch_base[CH_CR] = CR_CMD_RTS_OFF;
				RECOVER();
				if (ch_base[CH_MR] == 1) {
					duart_type = "scn2681";
				} else {
					duart_type = "scc2692";
					duart_flags |= SCN_HW_2692;
				}
			}
		} else {
			duart_type = "Unknown";
		}

		/* If a 2681, the CR_CMD_MR0 is interpreted as a TX_RESET */
		if(unit == scnconsole) {
			ch_base[CH_CR] = CR_ENA_TX;
			RECOVER();
		}
		ch_base[CH_CR] = CR_CMD_MR1;
		RECOVER();
		ch_base[CH_MR] = mr1;
		ch_base[CH_MR] = mr2;
		splx(s);
	    
		/* Arg 0 is special, so we must pass "unit + 1" */
		intr_establish(scnints[duart], scnintr, (void *) (unit + 1),
			       "scn", IPL_TTY, IPL_ZERO, LOW_LEVEL);

		printf("%c %s", delim, duart_type);
		delim = ',';

		/*
		 * IPL_ZERO is the right priority for the rx interrupt.
		 * Only splhigh() should disable rxints.
		 */
		intr_establish(rxints[duart], scnrxintr, (void *) (unit + 1),
			       "scnrx", IPL_ZERO, IPL_RTTY, LOW_LEVEL);
	}
	/* Record unit number, uart */
	sc->unit = unit;
	sc->chbase = ch_base;

	/* Initialize modem/interrupt bit masks */
	if (channel == 0) {
		sc->duart = malloc(sizeof(struct duart_info), M_DEVBUF, M_NOWAIT);
		if (sc->duart == NULL)
			panic("scn%d: memory allocation for duart structure failed", unit);
  		sc->duart->base = duart_base;
		sc->duart->hwflags = duart_flags;
		sc->sc_op_rts = OP_RTSA;
		sc->sc_op_dtr = OP_DTRA;
		sc->sc_ip_cts = IP_CTSA;
		sc->sc_ip_dcd = IP_DCDA;

		sc->sc_tx_int = INT_TXA;
	} else {
		sc->duart = ((struct scn_softc *)SOFTC(unit - 1))->duart;

		sc->sc_op_rts = OP_RTSB;
		sc->sc_op_dtr = OP_DTRB;
		sc->sc_ip_cts = IP_CTSB;
		sc->sc_ip_dcd = IP_DCDB;

		sc->sc_tx_int = INT_TXB;
	}

	/* Initialize error counts */
	sc->framing_errors = 0;
	sc->overrun_errors = 0;
	sc->parity_errors = 0;
	sc->break_interrupts = 0;

	if (unit == scnconsole) {
		DELAY(5 * 10000);	/* Let the output go out.... */
	}
	/*
         * Set up the hardware to a base state, in particular:
         * o reset transmitter and receiver
         * o set speeds and configurations
         * o receiver interrupts only (RxRDY and BREAK)
         */

	s = spltty();
	/* RTS off... */
	SCN_OP_BIC(sc, sc->sc_op_rts);	/* "istop" */

	ch_base[CH_CR] = CR_DIS_RX | CR_DIS_TX;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_RESET_RX;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_RESET_TX;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_RESET_ERR;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_RESET_BRK;
	RECOVER();
	ch_base[CH_CR] = CR_CMD_MR1;
	RECOVER();

	/* No receiver control of RTS. */
	ch_base[CH_MR] = 0;
	ch_base[CH_MR] = 0;

	/* Initialize the uart structure if this is channel A. */
	if (channel == 0) {
		u_char  temp;

		/* Disable all interrupts. */
		duart_base[DU_IMR] = sc->duart->imr_int_bits = 0;

		/* Output port config */
		duart_base[DU_OPCR] = sc->duart->opcr_bits = 0;

		/* Speeds... */
		sc->duart->speed_grp = 1;
		/* Set initial speed to an illegal code that can be changed to
		 * any other baud. */
		sc->duart->i_code[0] = sc->duart->o_code[0] =
		    sc->duart->i_code[1] = sc->duart->o_code[1] = 0x2f;
		sc->duart->i_speed[0] = sc->duart->o_speed[0] =
		    sc->duart->i_speed[1] = sc->duart->o_speed[1] = 0x0;

		sc->duart->acr_bits = 0;
#ifdef CT_VALUE
		sc->duart->acr_bits |= ACR_CT_TCLK1;	/* timer mode 1x clk */
		duart_base[DU_CTUR] = CT_VALUE >> 8;
		duart_base[DU_CTLR] = CT_VALUE & 255;
		temp = duart_base[DU_CSTRT];	/* start C/T running */
#endif
		sc->duart->acr_bits |= ACR_DELTA_DCDA;	/* Set CD int */
	} else {
		sc->duart->acr_bits |= ACR_DELTA_DCDB;	/* Set CD int */
	}

	if (scnsir == -1) {
		/* software intr: calls tty code, hence IPL_TTY */
		scnsir = intr_establish(SOFTINT, scnsoft, NULL,
				"softscn", IPL_TTY, IPL_TTY, 0);
	}

	duart_base[DU_ACR] =
	    (sc->duart->speed_grp ? ACR_BRG : 0) | sc->duart->acr_bits;

	if (unit == scnconsole)
		speed = scnconsrate;
	else
		speed = scndefaultrate;

	scn_config(unit, speed, speed, MR1_PNONE | MR1_BITS8, MR2_STOP1);
	if (scnconsole == unit)
		shutdownhook_establish(scncnreinit, (void *) makedev(scnmajor, unit));

	/* Turn on the receiver and transmitters */
	ch_base[CH_CR] = CR_ENA_RX | CR_ENA_TX;

	/* Set up the interrupts. */
	sc->duart->imr_int_bits |= INT_IP;
	scn_rxdisable(sc);
	splx(s);

	if (sc->scn_swflags) {
		printf("%c flags %d", delim, sc->scn_swflags);
		delim = ',';
	}
		
#ifdef KGDB
	if (kgdb_dev == makedev(scnmajor, unit)) {
		if (scnconsole == unit)
			kgdb_dev = NODEV;	/* can't debug over console
						 * port */
		else {
			scninit(kgdb_dev, kgdb_rate);
			scn_rxenable(sc);
			scnkgdb = DEV_UNIT(kgdb_dev);
			kgdb_attach(scncngetc, scncnputc, kgdb_dev);
			if (kgdb_debug_init) {
				printf("%c ", delim);
				kgdb_connect(1);
			} else
				printf("%c kgdb enabled", delim);
			delim = ',';
		}
	}
#endif
	printf("\n");
}

/* ARGSUSED */
int
scnopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	register struct tty *tp;
	register int unit = DEV_UNIT(dev);
	register struct scn_softc *sc;
	int error = 0;
	int hwset = 0;
	int s;

	if (unit >= scn_cd.cd_ndevs)
		return ENXIO;
	sc = SOFTC(unit);
	if (!sc)
		return ENXIO;

	s = spltty();
	if (!sc->scn_tty) {
		tp = sc->scn_tty = ttymalloc();
		tty_attach(tp);
	} else
		tp = sc->scn_tty;

	tp->t_oproc = scnstart;
	tp->t_param = scnparam;
	tp->t_hwiflow = scnhwiflow;
	tp->t_dev = dev;

	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = SCNDEF_CFLAG;

		sc->sc_rx_blocked = 0;

		if (sc->scn_swflags & SCN_SW_CLOCAL)
			tp->t_cflag |= CLOCAL;
		if (sc->scn_swflags & SCN_SW_CRTSCTS)
			tp->t_cflag |= CCTS_OFLOW | CRTS_IFLOW;
		tp->t_lflag = TTYDEF_LFLAG;
		if (unit == scnconsole)
			tp->t_ispeed = tp->t_ospeed = scnconsrate;
		else
			tp->t_ispeed = tp->t_ospeed = scndefaultrate;
		scnparam(tp, &tp->t_termios);
		ttsetwater(tp);

		/* Turn on DTR and RTS. */
		SCN_OP_BIS(sc, sc->sc_op_rts | sc->sc_op_dtr);

		/* enable reciever interrupts */
		scn_rxenable(sc);
		hwset = 1;

		/* set carrier state; */
		if ((sc->scn_swflags & SCN_SW_SOFTCAR) ||	/* check ttyflags */
		    SCN_DCD(sc) ||	/* check h/w */
		    DEV_DIALOUT(dev))
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
	} else
		if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0) {
			splx(s);
			return (EBUSY);
		} else
			if (DEV_DIALOUT(dev) && !SCN_DIALOUT(sc)) {
				/* dialout attempt while someone dialed in */
				splx(s);
				return (EBUSY);
			}
	if (DEV_DIALOUT(dev)) {
		/* dialout open and no one dialed in (see above) */
		SCN_SETDIALOUT(sc);	/* mark line as dialed out */
	}
	if (flag & O_NONBLOCK) {
		/* Non-blocking open
		 *
		 * If non-dialout open, and currenly opened for dialout you
		 * lose.
		 */
		if (!DEV_DIALOUT(dev) && SCN_DIALOUT(sc)) {
			splx(s);
			return EBUSY;
		}
	} else {
		/* Blocking open
		 *
		 * This used to be more compact (while loop with lots of nots)
		 * but it was incomprehensible.
		 * 
		 * "getty" is the classic case of a program that waits here...
		 */
		for (;;) {
			if (SCN_DIALOUT(sc)) {
				/* Currently opened for dialout. */
				if (DEV_DIALOUT(dev))
					/* No need to wait if dialout open */
					break;
				/*
				 * "getty" falls thru and waits while line
				 * dialed out.
				 */
			} else {
				/* Not currently opened for dialout. */
				if ((tp->t_cflag & CLOCAL) ||	/* "local" line? */
				    (tp->t_state & TS_CARR_ON))	/* or carrier up */
					break;	/* no (more) waiting */
				/* "getty" on modem falls thru and waits for
				 * carrier up */
			}
			tp->t_state |= TS_WOPEN;
			error = ttysleep(tp, (caddr_t) & tp->t_rawq,
			    TTIPRI | PCATCH, ttopen, 0);
			if (error) {
				/* XXX should turn off chip if we're the only
				 * waiter */

/* XXX drop RTS too? -plb */
/* XXX dialout state??? -plb */
				SCN_OP_BIC(sc, sc->sc_op_dtr);
				splx(s);
				return error;
			}
		}
	}
	splx(s);

	error = (*linesw[tp->t_line].l_open) (dev, tp);
	if (error && hwset) {
		scn_rxdisable(sc);
		SCN_OP_BIC(sc, sc->sc_op_rts | sc->sc_op_dtr);
	}
	return (error);
}


/*ARGSUSED*/
int
scnclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	register int unit = DEV_UNIT(dev);
	register struct scn_softc *sc = SOFTC(unit);
	register struct tty *tp = sc->scn_tty;

	if ((tp->t_state & TS_ISOPEN) == 0)
	        return 0;

	(*linesw[tp->t_line].l_close) (tp, flag);

#ifdef KGDB
	/* do not disable interrupts if debugging */
	if (kgdb_dev != makedev(scnmajor, unit))
#endif
		if ((tp->t_state & TS_ISOPEN) == 0) {
			scn_rxdisable(sc);
		}
	if ((tp->t_cflag & HUPCL) && (sc->scn_swflags & SCN_SW_SOFTCAR) == 0) {
		SCN_OP_BIC(sc, sc->sc_op_dtr);
		/* hold low for 1 second */
		(void) tsleep((caddr_t)sc, TTIPRI, ttclos, hz);
	}
	SCN_CLRDIALOUT(sc);
	ttyclose(tp);

#if 0
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttyfree(tp);
		sc->scn_tty = (struct tty *) NULL;
	}
#endif

	return (0);
}

int
scnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct scn_softc *sc = SOFTC(DEV_UNIT(dev));
	register struct tty *tp = sc->scn_tty;

	return ((*linesw[tp->t_line].l_read) (tp, uio, flag));
}

int
scnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct scn_softc *sc = SOFTC(DEV_UNIT(dev));
	register struct tty *tp = sc->scn_tty;

	return ((*linesw[tp->t_line].l_write) (tp, uio, flag));
}

struct tty *
scntty(dev)
	dev_t dev;
{
	register struct scn_softc *sc = SOFTC(DEV_UNIT(dev));

	return sc->scn_tty;
}

/* Worker routines for interrupt processing */
static __inline void
dcd_int(sc, tp, new)
	struct scn_softc *sc;
	struct tty *tp;
	u_char new;
{
	if (sc->scn_swflags & SCN_SW_SOFTCAR)
		return;

#if 0
	printf("scn%d: dcd_int ip %x SCN_DCD %x new %x ipcr %x\n",
	    sc->unit,
	    sc->duart->base[DU_IP],
	    SCN_DCD(sc),
	    new,
	    sc->duart->base[DU_IPCR]
	    );
#endif

/* XXX set some flag to have some lower (soft) int call line discipline? */
	if (!(*linesw[(u_char) tp->t_line].l_modem) (tp, new == 0? 1: 0)) {
	      SCN_OP_BIC(sc, sc->sc_op_rts | sc->sc_op_dtr);
	}
}

/*
 * Print out a ring or fifo overrun error message.
 */
static void
scnoverrun(unit, ptime, what)
	int unit;
	long *ptime;
	char *what;
{
	if (*ptime != time.tv_sec) {
		*ptime = time.tv_sec;
		log(LOG_WARNING, "scn%d: %s overrun\n", unit, what);
	}
}

/*
 * Try to block or unblock input using hardware flow-control.
 * This is called by kern/tty.c if MDMBUF|CRTSCTS is set, and
 * if this function returns non-zero, the TS_TBLOCK flag will
 * be set or cleared according to the "stop" arg passed.
 */
int
scnhwiflow(tp, stop)
	struct tty *tp;
	int stop;
{
	int unit = DEV_UNIT(tp->t_dev);
	struct scn_softc *sc = SOFTC(unit);
	int s = splrtty();

	if (!stop) {
		if (sc->sc_rbput - sc->sc_rbget - 1) {
			setsoftscn();
		}
	}
	splx(s);
	return 1;
}

static void
scnintr(arg)
	void *arg;
{
	int line1 = (int)arg;	/* NOTE: line _ONE_ */
	register struct scn_softc *sc0 = SOFTC(line1 - 1);
	register struct scn_softc *sc1 = SOFTC(line1);

	register struct tty *tp0 = sc0->scn_tty;
	register struct tty *tp1 = sc1->scn_tty;

	register struct duart_info *duart = sc0->duart;

	char rs_work;
	u_char rs_stat;
	u_char rs_ipcr;

	do {
		/* Loop to pick up ALL pending interrupts for device. */
		rs_work = FALSE;
		rs_stat = duart->base[DU_ISR];

/* channel a */
		if (tp0 != NULL) {
			if ((rs_stat & INT_TXA) && (tp0->t_state & TS_BUSY)) {
				/* output char done. */
				tp0->t_state &= ~(TS_BUSY | TS_FLUSH);

				/* disable tx ints */
				sc0->duart->imr_int_bits &= ~sc0->sc_tx_int;
				sc0->duart->base[DU_IMR] = sc0->duart->imr_int_bits;

/* XXX perform any held line changes */

				(*linesw[tp0->t_line].l_start) (tp0);
				rs_work = TRUE;
			}
		}
		/* channel b */
		if (tp1 != NULL) {
			if ((rs_stat & INT_TXB) && (tp1->t_state & TS_BUSY)) {
				/* output char done. */
				tp1->t_state &= ~(TS_BUSY | TS_FLUSH);

				/* disable tx ints */
				sc1->duart->imr_int_bits &= ~sc1->sc_tx_int;
				sc1->duart->base[DU_IMR] = sc1->duart->imr_int_bits;

/* XXX perform any held line changes */

				(*linesw[tp1->t_line].l_start) (tp1);
				rs_work = TRUE;
			}
		}
		if (rs_stat & INT_IP) {
			rs_work = TRUE;
			rs_ipcr = duart->base[DU_IPCR];

			if (rs_ipcr & IPCR_DELTA_DCDA && tp0 != NULL) {
				dcd_int(sc0, tp0, rs_ipcr & IPCR_DCDA);
			}
			if (rs_ipcr & IPCR_DELTA_DCDB && tp1 != NULL) {
				dcd_int(sc1, tp1, rs_ipcr & IPCR_DCDB);
			}
		}
	} while (rs_work);
}

/*
 * Handle rxrdy/ffull interrupt: QUICKLY poll both channels (checking
 * status first) and stash data in a ring buffer.  Ring buffer scheme
 * borowed from sparc/zs.c requires NO interlock on data!
 *
 * This interrupt should NOT be included in spltty() mask since it
 * invokes NO tty code!  The whole point is to allow tty input as much
 * of the time as possible, while deferring "heavy" character
 * processing until later.
 *
 * see scn.hw.README and scnsoft() for more info.
 *
 * THIS ROUTINE SHOULD BE KEPT AS CLEAN AS POSSIBLE!!
 * IT'S A CANDIDATE FOR RECODING IN ASSEMBLER!!
 */
static int
scn_rxintr(sc, line)
     struct scn_softc *sc;
     int line;
{
	register char sr;
	register int i, n, put;
	int work;

	work = 0;
	i = sc->sc_rbput;
	while (work <= 10) {
#define SCN_GETCH(SC) \
		sr = (SC)->chbase[CH_SR]; \
		if ((sr & SR_RX_RDY) == 0) \
			break; \
		if (sr & (SR_PARITY | SR_FRAME | SR_BREAK | SR_OVERRUN)) \
			goto exception; \
		work++; \
		(SC)->sc_rbuf[i++ & SCN_RING_MASK] = (SC)->chbase[CH_DAT]

		SCN_GETCH(sc); SCN_GETCH(sc); SCN_GETCH(sc);
			
		continue;
	exception:
#if defined(DDB)
		if (line == scnconsole && (sr & SR_BREAK)) {
			Debugger();
			sr = sc->chbase[CH_SR];
		}
#endif
#if defined(KGDB)
		if (line == scnkgdb && (sr & SR_RX_RDY)) {
			kgdb_connect(1);
			sr = sc->chbase[CH_SR];
		}
#endif
		work++;
		sc->sc_rbuf[i++ & SCN_RING_MASK] = (sr << 8) | sc->chbase[CH_DAT];
		sc->chbase[CH_CR] = CR_CMD_RESET_ERR;	/* resets break? */
		RECOVER();
	}
	/*
	 * If ring is getting too full, try to block input.
	 */
	n = i - sc->sc_rbget;
	if (sc->sc_rbhiwat && (n > sc->sc_rbhiwat)) {
		/* If not CRTSCTS sc_rbhiwat is such that this
		 *  never happens
		 */
		if (scn_soft_rts)
			SCN_OP_BIC(sc, sc->sc_op_rts);
		else
			scn_rxdisable(sc);
		sc->sc_rx_blocked = 1;
	}
	sc->sc_rbput = i;

	return work;
}

static void
scnrxintr(arg)
	void *arg;
{
	register char sr;
	register int i;
	int work = 0;
	int line1 = (int)arg;	/* NOTE: line _ONE_ */
	int line0 = line1 - 1;
	register struct scn_softc *sc0 = SOFTC(line0);
	register struct scn_softc *sc1 = SOFTC(line1);

	if (scn_soft_rts || !sc0->sc_rx_blocked)
		work = scn_rxintr(sc0, line0);
	if (scn_soft_rts || !sc1->sc_rx_blocked
#ifdef KGDB
	    && line1 < scn_cd.cd_ndevs
#endif
	    )
		work += scn_rxintr(sc1, (int)line1);

	if (work > 0) {
		setsoftscn();	/* trigger s/w intr */
#ifdef SCN_TIMING
		microtime(&tstart);
#endif
	}
}

/*
 * Here on soft interrupt (at spltty) to empty ring buffers.
 *
 * Dave's original scheme was to use the DUART reciever timeout
 * interrupt. This requires 2692's (which my board doesn't have), and
 * I also liked the idea of using the C/T to generate alternate and/or
 * arbitrary bauds. -plb
 *
 * The ringbuffer code comes from Chris Torek's SPARC 44bsd zs driver
 * (hence the LBL notice on top of this file), DOES NOT require
 * interlocking with interrupt levels!
 *
 * The 44bsd sparc/zs driver reads the ring buffer from a seperate
 * zssoftint, while the SunOS 4.x zs driver appears to use
 * timeout()'s.  timeouts seem to be too slow to deal with high data
 * rates.  I know, I tried them.
 * -plb.
 */
static void
scnsoft(arg)
	void *arg;
{
	int unit;
#ifdef SCN_TIMING
	struct timeval tend;
	u_long  t;

	microtime(&tend);
	t = (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);
	t = (t + tick / 20) / (tick / 10);
	if (t >= NJITTER - 1) {
		t = NJITTER - 1;
	}
	scn_jitter[t]++;
#endif

	for (unit = 0; unit < scn_cd.cd_ndevs; unit++) {
		register struct scn_softc *sc;
		register struct tty *tp;
		register int n, get;

		sc = SOFTC(unit);
		if (!sc)
			continue;
		tp = sc->scn_tty;
#ifdef KGDB
		if (tp == NULL) {
			sc->sc_rbget = sc->sc_rbput;
			continue;
		}
#endif
		if (tp == NULL || tp->t_state & TS_TBLOCK) {
			continue;
		}


		get = sc->sc_rbget;

		/* NOTE: fetch from rbput is atomic */
		while (get != (n = sc->sc_rbput)) {
			/*
		         * Compute the number of interrupts in the receive ring.
		         * If the count is overlarge, we lost some events, and
		         * must advance to the first valid one.  It may get
		         * overwritten if more data are arriving, but this is
		         * too expensive to check and gains nothing (we already
		         * lost out; all we can do at this point is trade one
		         * kind of loss for another).
		         */
			n -= get;
			if (n > SCN_RING_SIZE) {
				scnoverrun(unit, &sc->sc_rotime, "ring");
				get += n - SCN_RING_SIZE;
				n = SCN_RING_SIZE;
				sc->ring_overruns++;
			}
			while (--n >= 0) {
				register int c, sr;

				if (tp->t_state & TS_TBLOCK) {
					sc->sc_rbget = get;
					goto done;
				}
				/* Race to keep ahead of incoming interrupts. */
				c = sc->sc_rbuf[get++ & SCN_RING_MASK];

				sr = c >> 8;	/* extract status */
				c &= 0xff;	/* leave just character */

				if (sr & SR_OVERRUN) {
					scnoverrun(unit, &sc->sc_fotime, "fifo");
					sc->overrun_errors++;
				}
				if (sr & SR_PARITY)
					c |= TTY_PE;
				if (sr & SR_FRAME)
					c |= TTY_FE;
				if (sr & SR_BREAK) {
#if 0
					/*
					 * See DDB_CHECK() comments in
					 * scnrxintr()
					 */
					if (unit == scnconsole)
						Debugger();
#endif
					c = TTY_FE | 0;
				}
				(*linesw[tp->t_line].l_rint) (c, tp);

				if (sc->sc_rx_blocked && n < SCN_RING_THRESH) {
					int	s = splrtty();
					sc->sc_rx_blocked = 0;
					if (scn_soft_rts)
						SCN_OP_BIS(sc, sc->sc_op_rts);
					else
						scn_rxenable(sc);
					splx(s);
				}
					
			}
			sc->sc_rbget = get;
		}
	done:
	}
}

/* Convert TIOCM_xxx bits to output port bits. */
static unsigned char
opbits(sc, tioc_bits)
	struct scn_softc *sc;
	int tioc_bits;
{
	return ((((tioc_bits) & TIOCM_DTR) ? sc->sc_op_dtr : 0) |
	    (((tioc_bits) & TIOCM_RTS) ? sc->sc_op_rts : 0));
}

int
scnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t	data;
	int flag;
	struct proc *p;
{
	register int unit = DEV_UNIT(dev);
	register struct scn_softc *sc = SOFTC(unit);
	register struct tty *tp = sc->scn_tty;
	register int error;

	error = (*linesw[tp->t_line].l_ioctl) (tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {
	case TIOCSBRK:
		sc->chbase[CH_CR] = CR_CMD_START_BRK;
		break;

	case TIOCCBRK:
		sc->chbase[CH_CR] = CR_CMD_STOP_BRK;
		break;

	case TIOCSDTR:
		SCN_OP_BIS(sc, sc->sc_op_dtr | sc->sc_op_rts);
		break;

	case TIOCCDTR:
		SCN_OP_BIC(sc, sc->sc_op_dtr | sc->sc_op_rts);
		break;

	case TIOCMSET: {
			int     s;
			unsigned char sbits, cbits;

			/* set bits */
			sbits = opbits(sc, *(int *) data);

			/* get bits to clear */
			cbits = ~sbits & (sc->sc_op_dtr | sc->sc_op_rts);

			s = spltty();
			if (sbits) {
				SCN_OP_BIS(sc, sbits);
			}
			if (cbits) {
				SCN_OP_BIC(sc, cbits);
			}
			splx(s);
			break;
		}

	case TIOCMBIS:
		SCN_OP_BIS(sc, opbits(sc, *(int *) data));
		break;

	case TIOCMBIC:
		SCN_OP_BIC(sc, opbits(sc, *(int *) data));
		break;

	case TIOCMGET: {
			int     bits;
			unsigned char ip, op;

			/* s = spltty(); */
			ip = sc->duart->base[DU_IP];
			/*
			 * XXX sigh; cannot get op current state!! even if
			 * maintained in private, RTS is done in h/w!!
			 */
			op = 0;
			/* splx(s); */

			bits = 0;
			if (ip & sc->sc_ip_dcd)
				bits |= TIOCM_CD;
			if (ip & sc->sc_ip_cts)
				bits |= TIOCM_CTS;

#if 0
			if (op & sc->sc_op_dtr)
				bits |= TIOCM_DTR;
			if (op & sc->sc_op_rts)
				bits |= TIOCM_RTS;
#endif

			*(int *) data = bits;
			break;
		}

	case TIOCGFLAGS:{
			int     bits = 0;

			if (sc->scn_swflags & SCN_SW_SOFTCAR)
				bits |= TIOCFLAG_SOFTCAR;
			if (sc->scn_swflags & SCN_SW_CLOCAL)
				bits |= TIOCFLAG_CLOCAL;
			if (sc->scn_swflags & SCN_SW_CRTSCTS)
				bits |= TIOCFLAG_CRTSCTS;
			if (sc->scn_swflags & SCN_SW_MDMBUF)
				bits |= TIOCFLAG_MDMBUF;

			*(int *) data = bits;
			break;
		}
	case TIOCSFLAGS:{
			int     userbits, driverbits = 0;

			error = suser(p->p_ucred, &p->p_acflag);
			if (error != 0)
				return (EPERM);

			userbits = *(int *) data;
			if (userbits & TIOCFLAG_SOFTCAR)
				driverbits |= SCN_SW_SOFTCAR;
			if (userbits & TIOCFLAG_CLOCAL)
				driverbits |= SCN_SW_CLOCAL;
			if (userbits & TIOCFLAG_CRTSCTS)
				driverbits |= SCN_SW_CRTSCTS;
			if (userbits & TIOCFLAG_MDMBUF)
				driverbits |= SCN_SW_MDMBUF;

			sc->scn_swflags = driverbits;

			break;
		}

	default:
		return (ENOTTY);
	}
	return (0);
}

int
scnparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	int cflag = t->c_cflag;
	int unit = DEV_UNIT(tp->t_dev);
	char mr1, mr2;
	int error;
	struct scn_softc *sc = SOFTC(unit);

	/* Is this a hang up? */
	if (t->c_ospeed == B0) {
		SCN_OP_BIC(sc, sc->sc_op_dtr);
		/* leave DTR down. see comment in scnclose() -plb */
		return (0);
	}
	mr1 = mr2 = 0;

	/* Parity? */
	if (cflag & PARENB) {
		if ((cflag & PARODD) == 0)
			mr1 |= MR1_PEVEN;
		else
			mr1 |= MR1_PODD;
	} else
		mr1 |= MR1_PNONE;

	/* Stop bits. */
	if (cflag & CSTOPB)
		mr2 |= MR2_STOP2;
	else
		mr2 |= MR2_STOP1;

	/* Data bits. */
	switch (cflag & CSIZE) {
	case CS5:
		mr1 |= MR1_BITS5;
		break;
	case CS6:
		mr1 |= MR1_BITS6;
		break;
	case CS7:
		mr1 |= MR1_BITS7;
		break;
	case CS8:
	default:
		mr1 |= MR1_BITS8;
		break;
	}

	if (cflag & CCTS_OFLOW)
		mr2 |= MR2_TXCTS;

	if (cflag & CRTS_IFLOW) {
		if(!scn_soft_rts)
			mr1 |= MR1_RXRTS;
		sc->sc_rbhiwat = SCN_RING_HIWAT;
	} else {
		sc->sc_rbhiwat = 0;
	}

	error = scn_config(unit, t->c_ispeed, t->c_ospeed, mr1, mr2);

	/* If successful, copy to tty */
	if (!error) {
		tp->t_ispeed = t->c_ispeed;
		tp->t_ospeed = t->c_ospeed;
		tp->t_cflag = cflag;
	}
	return (error);
}

void
scnstart(tp)
	struct tty *tp;
{
	int s, c;
	int unit = DEV_UNIT(tp->t_dev);
	struct scn_softc *sc = SOFTC(unit);

	s = spltty();
	if (tp->t_state & (TS_BUSY | TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) & tp->t_outq);
		}
		if (tp->t_outq.c_cc == 0)	/* plb 11/8/95 - from
						 * isa/com.c */
			goto out;
		selwakeup(&tp->t_wsel);
	}
	tp->t_state |= TS_BUSY;
	if (sc->chbase[CH_SR] & SR_TX_RDY) {
		c = getc(&tp->t_outq);
		sc->chbase[CH_DAT] = c;

		/* Enable transmit interrupts. */
		sc->duart->base[DU_IMR] = (sc->duart->imr_int_bits |= sc->sc_tx_int);
	}
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
void
scnstop(tp, flag)
	struct tty *tp;
	int flag;
{
	int s = spltty();

	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 * Following are all routines needed for SCN to act as console.
 */
#ifdef VERYLOWDEBUG
/* So the kernel can write in unmapped mode! */
extern int _mapped;
#define SCN_BASE	(_mapped ? SCN_FIRST_MAP_ADR : SCN_FIRST_ADR)
#else
#define SCN_BASE	(SCN_FIRST_MAP_ADR)
#endif

#define	DUADDR(dev)	(u_char *)(SCN_BASE + (DEV_UNIT(dev) >> 1) * DUART_SZ)
#define	CHADDR(dev)	(u_char *)(SCN_BASE + DEV_UNIT(dev) * CH_SZ)

int
scncnprobe(cp)
	struct consdev *cp;
{
	/* Locate the major number. */
	for (scnmajor = 0; scnmajor < nchrdev; scnmajor++)
		if (cdevsw[scnmajor].d_open == scnopen)
			break;

	/* initialize required fields */
	cp->cn_dev = makedev(scnmajor, SCN_CONSOLE);
	cp->cn_pri = CN_NORMAL;
	return 1;
}

void
scncnreinit(v)
	void *v;
{
	dev_t dev = (dev_t)v;
	int unit = DEV_UNIT(dev);
	volatile u_char *du_base = DUADDR(dev);

	du_base[DU_OPSET] = (unit & 1) ? (OP_RTSB | OP_DTRB) : (OP_RTSA | OP_DTRA);
}

void
scncninit(cp)
	struct consdev *cp;
{
	scninit(cp->cn_dev, scnconsrate);
	scnconsole = DEV_UNIT(cp->cn_dev);
}

/* Used by scncninit and kgdb startup. */
int
scninit(dev, rate)
	dev_t dev;
	int rate;
{
	volatile u_char *du_base = DUADDR(dev);
	int unit = DEV_UNIT(dev);

	du_base[DU_OPSET] = (unit & 1) ? (OP_RTSB | OP_DTRB) : (OP_RTSA | OP_DTRA);
	scn_config(unit, rate, rate, MR1_PNONE | MR1_BITS8, MR2_STOP1);
	return (0);
}

/*
 * Console kernel input character routine.
 */
int
scncngetc(dev)
	dev_t dev;
{
	volatile u_char *ch_base = CHADDR(dev);
	char c;
	int s = spltty();

	while ((ch_base[CH_SR] & SR_RX_RDY) == 0);
	c = ch_base[CH_DAT];

	splx(s);
	return c;
}

/* The pc532 does not turn off console polling. */
void
scncnpollc(dev, on)
	dev_t dev;
	int on;
{
}

/*
 * Console kernel output character routine.
 */
void
scncnputc(dev, c)
	dev_t dev;
	char c;
{
	volatile u_char *ch_base = CHADDR(dev);
	volatile u_char *du_base = DUADDR(dev);
	int s = spltty();

	if (c == '\n')
		scncnputc(dev, '\r');

	while ((ch_base[CH_SR] & SR_TX_RDY) == 0);
	ch_base[CH_DAT] = c;
	while ((ch_base[CH_SR] & SR_TX_RDY) == 0);
	du_base[DU_ISR];

	splx(s);
}

void scn_ei(enabled)
     int enabled;
{
	/* Ideas: to make this go faster, precompute addresses and bits to
	 * set when channel is opened, blocked or unblocked or closed. Do
         * both channels of DUART at once.
	 */
	int unit;
	for (unit = 0; unit < scn_cd.cd_ndevs; unit++) {
		register struct scn_softc *sc;

		sc = SOFTC(unit);
		if (!sc->sc_rx_blocked && sc->scn_swflags & SCN_SW_CRTSCTS) {
			if (enabled) {
				SCN_OP_BIS(sc, sc->sc_op_rts);
			} else {
				SCN_OP_BIC(sc, sc->sc_op_rts);	/* "istop" */
			}
		}
	}
}

#endif
