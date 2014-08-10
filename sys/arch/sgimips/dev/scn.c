/*	$NetBSD: scn.c,v 1.6.2.1 2014/08/10 06:54:07 tls Exp $ */

/*
 * Resurrected from the old pc532 port 1/18/2009.
 *
 * XXX- The locking in this is probably totally broken. I haven't attempted
 *      to get it right, but it seems to work okay anyhow.
 */

/*
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
 * 3. Neither the name of the University nor the names of its contributors
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

/*
 * Copyright (c) 1996, 1997 Philip L. Budne.
 * Copyright (c) 1993 Philip A. Nelson.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scn.c,v 1.6.2.1 2014/08/10 06:54:07 tls Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "scn.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/intr.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif
#include <sys/kauth.h>

#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <sgimips/dev/scnreg.h>
#include <sgimips/dev/scnvar.h>

int     scn_match(device_t, struct cfdata *, void *);
void    scn_attach(device_t, device_t, void *);
int     scnparam(struct tty *, struct termios *);
void    scnstart(struct tty *);
int     scnhwiflow(struct tty *, int);

void	scncnprobe(struct consdev *);
void	scncninit(struct consdev *);
int     scncngetc(dev_t);
void    scncnputc(dev_t, int);
void	scncnpollc(dev_t, int);
int	scninit(dev_t, int);
void	scncnreinit(void *);

CFATTACH_DECL_NEW(scn, sizeof(struct scn_softc),
    scn_match, scn_attach, NULL, NULL);

extern struct cfdriver scn_cd;

dev_type_open(scnopen);
dev_type_close(scnclose);
dev_type_read(scnread);
dev_type_write(scnwrite);
dev_type_ioctl(scnioctl);
dev_type_stop(scnstop);
dev_type_tty(scntty);
dev_type_poll(scnpoll);

const struct cdevsw scn_cdevsw = {
	.d_open = scnopen,
	.d_close = scnclose,
	.d_read = scnread,
	.d_write = scnwrite,
	.d_ioctl = scnioctl,
	.d_stop = scnstop,
	.d_tty = scntty,
	.d_poll = scnpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

struct consdev scn_cn = {
	scncnprobe,
	scncninit,
	scncngetc,
	scncnputc,
	scncnpollc,
	NULL,
	NULL,
	NULL,
	NODEV,
	CN_NORMAL
};

#ifndef CONSOLE_SPEED
#define CONSOLE_SPEED TTYDEF_SPEED
#endif

#ifndef SCNDEF_CFLAG
#define SCNDEF_CFLAG TTYDEF_CFLAG
#endif

#ifdef CPU30MHZ
#define RECOVER()	__asm volatile("bispsrw 0x800" : : : "cc")
#else
#define RECOVER()
#endif

int     scndefaultrate = TTYDEF_SPEED;
int     scnconsrate = CONSOLE_SPEED;

static inline struct scn_softc *
SOFTC(int unit)
{
	if (unit < 0 || unit >= scn_cd.cd_ndevs)
		return (NULL);
	return device_private(scn_cd.cd_devs[unit]);
}

static int	scnintr(void *);
static void	scnrxintr(void *);
static int	scn_rxintr(struct scn_softc *);
static void	scnsoft(void *);
static void	scn_setchip(struct scn_softc *sc);
static int	scniter(int *, int, int*, int*, struct chan *, int);
static int	scn_config(int, int, int, int, u_char, u_char);
static void	scn_rxenable(struct scn_softc *);
static void	scn_rxdisable(struct scn_softc *);
static void	dcd_int(struct scn_softc *, struct tty *, u_char);
static void	scnoverrun(int, long *, const char *);
static u_char	opbits(struct scn_softc *, int);

static void *scnsir = NULL;		/* s/w intr cookie */
#define setsoftscn()	softint_schedule(scnsir)

#ifdef SCN_TIMING
/*
 * Keep timing info on latency of software interrupt used by
 * the ringbuf code to empty ring buffer.
 * "getinfo" program reads data from /dev/kmem.
 */
static struct timeval tstart;
#define NJITTER 100
int     scn_njitter = NJITTER;
int     scn_jitter[NJITTER];
#endif

#define SCN_CLOCK	3686400		/* input clock */

/* speed table groups ACR[7] */
#define GRP_A	0
#define GRP_B	ACR_BRG

/* combo of MR0[2:0] and ACR[7] */
#define MODE0A	MR0_MODE_0
#define MODE0B	(MR0_MODE_0|ACR_BRG)
#define MODE1A	MR0_MODE_1
#define MODE1B	(MR0_MODE_1|ACR_BRG)
#define MODE2A	MR0_MODE_2
#define MODE2B	(MR0_MODE_2|ACR_BRG)

#define ANYMODE	-1
#define DEFMODE(C92) MODE0A		/* use MODE4A if 26c92? */

/* speed code for Counter/Timer (all modes, groups) */
#define USE_CT 0xd

/*
 * Rate table, ordered by speed, then mode.
 * NOTE: ordering of modes must be done carefully!
 */
struct tabent {
	int32_t speed;
	int16_t code;
	int16_t mode;
} table[] = {
	{     50, 0x0, MODE0A },
	{     75, 0x0, MODE0B },
	{    110, 0x1, MODE0A },
	{    110, 0x1, MODE0B },
	{    110, 0x1, MODE1A },
	{    110, 0x1, MODE1B },
	{    134, 0x2, MODE0A },	/* 134.5 */
	{    134, 0x2, MODE0B },	/* 134.5 */
	{    134, 0x2, MODE1A },	/* 134.5 */
	{    134, 0x2, MODE1B },	/* 134.5 */
	{    150, 0x3, MODE0A },
	{    150, 0x3, MODE0A },
	{    200, 0x3, MODE0A },
	{    300, 0x4, MODE0A },
	{    300, 0x4, MODE0B },
	{    300, 0x0, MODE1A },
	{    450, 0x0, MODE1B },
	{    600, 0x5, MODE0A },
	{    600, 0x5, MODE0B },
	{    880, 0x1, MODE2A },
	{    880, 0x1, MODE2B },
	{    900, 0x3, MODE1B },
	{   1050, 0x7, MODE0A },
	{   1050, 0x7, MODE1A },
	{   1076, 0x2, MODE2A },
	{   1076, 0x2, MODE2B },
	{   1200, 0x6, MODE0A },
	{   1200, 0x6, MODE0B },
	{   1200, 0x3, MODE1A },
	{   1800, 0xa, MODE0B },
	{   1800, 0x4, MODE1A },
	{   1800, 0x4, MODE1B },
	{   2000, 0x7, MODE0B },
	{   2000, 0x7, MODE1B },
	{   2400, 0x8, MODE0A },
	{   2400, 0x8, MODE0B },
	{   3600, 0x5, MODE1A },
	{   3600, 0x5, MODE1B },
	{   4800, 0x9, MODE2A },
	{   4800, 0x9, MODE2B },
	{   4800, 0x9, MODE0A },
	{   4800, 0x9, MODE0B },
	{   7200, 0xa, MODE0A },
	{   7200, 0x0, MODE2B },
	{   7200, 0x6, MODE1A },
	{   7200, 0x6, MODE1B },
	{   9600, 0xb, MODE2A },
	{   9600, 0xb, MODE2B },
	{   9600, 0xb, MODE0A },
	{   9600, 0xb, MODE0B },
	{   9600, 0xd, MODE1A },	/* use C/T as entre' to mode1 */
	{   9600, 0xd, MODE1B },	/* use C/T as entre' to mode1 */
	{  14400, 0x3, MODE2B },
	{  14400, 0x8, MODE1A },
	{  14400, 0x8, MODE1B },
	{  19200, 0x3, MODE2A },
	{  19200, 0xc, MODE2B },
	{  19200, 0xc, MODE0B },
	{  19200, 0xd, MODE1A },	/* use C/T as entre' to mode1 */
	{  19200, 0xd, MODE1B },	/* use C/T as entre' to mode1 */
	{  28800, 0x4, MODE2A },
	{  28800, 0x4, MODE2B },
	{  28800, 0x9, MODE1A },
	{  28800, 0x9, MODE1B },
	{  38400, 0xc, MODE2A },
	{  38400, 0xc, MODE0A },
	{  57600, 0x5, MODE2A },
	{  57600, 0x5, MODE2B },
	{  57600, 0xb, MODE1A },
	{  57600, 0xb, MODE1B },
	{ 115200, 0x6, MODE2A },
	{ 115200, 0x6, MODE2B },
	{ 115200, 0xc, MODE1B },
	{ 230400, 0xc, MODE1A }
};
#define TABENTRIES (sizeof(table)/sizeof(table[0]))

/*
 * boolean for speed codes which are identical in both A/B BRG groups
 * in all modes
 */
static u_char bothgroups[16] = {
	0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1
};

/*
 * Manually constructed divisors table
 * for minimum error (from some of Dave Rand's code)
 */
const struct {
	uint16_t speed;
	uint16_t div;
} divs[] = {
	{    50, 2303 },	/* 2304 is exact?? */
	{   110, 1047 },	/* Should be 1047.27 */
	{   134, 857 },		/* Should be 856.505576 */
	{  1050, 110 },		/* Should be 109.7142857 */
	{  2000, 57 }		/* Should be 57.6 */
};
#define DIVS (sizeof(divs)/sizeof(divs[0]))

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

#define SCN_MAXDUART 4
static struct duart scn_duart[SCN_MAXDUART];

#ifdef KGDB
extern int kgdb_dev;
extern int kgdb_rate;
extern int kgdb_debug_init;
#endif

/* XXXXX - fix this */
#define splrtty() spltty()

/* RS-232 configuration routines */

/*
 * set chip parameters, or mark for delayed change.
 * called at spltty() or on TxEMPTY interrupt.
 *
 * Reads current values to avoid glitches from redundant sets.
 * Perhaps should save last value set to avoid read/write?  NOTE:
 * Would still need to do read if write not needed to advance MR
 * pointer.
 *
 * new 2/97 -plb
 */

static void
scn_setchip(struct scn_softc *sc)
{
	struct duart *dp;
	u_char acr, csr, mr1, mr2;
	int chan;

	if (sc->sc_tty && (sc->sc_tty->t_state & TS_BUSY)) {
		sc->sc_heldchanges = 1;
		return;
	}

	chan = sc->sc_channel;
	dp = sc->sc_duart;
	if (dp->type == SC26C92) {
		u_char nmr0a, mr0a;

		/* input rate high enough so 64 bit time watchdog not
		 * onerous? */
		if (dp->chan[chan].ispeed >= 1200) {
			/* set FIFO threshold at 6 for other
			 * thresholds we could have to set MR1_FFULL
			 */
			dp->chan[chan].mr0 |= MR0_RXWD | MR0_RXINT;
		} else {
			dp->chan[chan].mr0 &= ~(MR0_RXWD | MR0_RXINT);
		}

		/* select BRG mode (MR0A only) */
		nmr0a = dp->chan[0].mr0 | (dp->mode & MR0_MODE);

		dp->base[CH_CR] = CR_CMD_MR0;
		RECOVER();

		mr0a = dp->base[CH_MR];
		if (mr0a != nmr0a) {
			dp->base[CH_CR] = CR_CMD_MR0;
			RECOVER();
			dp->base[CH_MR] = nmr0a;
		}

		if (chan) {	 /* channel B? */
			u_char mr0b;

			sc->sc_chbase[CH_CR] = CR_CMD_MR0;
			RECOVER();
			mr0b = dp->base[CH_MR];

			if (dp->chan[chan].mr0 != mr0b) {
				sc->sc_chbase[CH_CR] = CR_CMD_MR0;
				RECOVER();
				sc->sc_chbase[CH_MR] = dp->chan[chan].mr0;
			}
		}
	} else {
		sc->sc_chbase[CH_CR] = CR_CMD_MR1;
		RECOVER();
	}

	mr1 = sc->sc_chbase[CH_MR];
	mr2 = sc->sc_chbase[CH_MR];
	if (mr1 != dp->chan[chan].new_mr1 ||
	    mr2 != dp->chan[chan].new_mr2) {
		sc->sc_chbase[CH_CR] = CR_CMD_MR1;
		RECOVER();
		sc->sc_chbase[CH_MR] = dp->chan[chan].new_mr1;
		sc->sc_chbase[CH_MR] = dp->chan[chan].new_mr2;
	}

	acr = dp->acr | (dp->mode & ACR_BRG);
	dp->base[DU_ACR] = acr;		/* write-only reg! */

	/* set speed codes */
	csr = (dp->chan[chan].icode<<4) | dp->chan[chan].ocode;
	if (sc->sc_chbase[CH_CSR] != csr) {
		sc->sc_chbase[CH_CSR] = csr;
	}

	/* see if counter/timer in use */
	if (dp->counter &&
	    (dp->chan[0].icode == USE_CT || dp->chan[0].ocode == USE_CT ||
	     dp->chan[1].icode == USE_CT || dp->chan[1].ocode == USE_CT)) {

		/* program counter/timer only if necessary */
		if (dp->counter != dp->ocounter) {
			uint16_t div;
#ifdef DIVS
			int i;

			/* look for precalculated rate, for minimum error */
			for (i = 0; i < DIVS && divs[i].speed <= dp->counter; i++) {
				if (divs[i].speed == dp->counter) {
					div = divs[i].div;
					goto found;
				}
			}
#endif

			/* not found in table; calculate a value (rounding up) */
			div = ((long)SCN_CLOCK/16/2 + dp->counter/2) / dp->counter;

		found:
			/* halt before loading? may ALWAYS glitch?
			 * reload race may only sometimes glitch??
			 */
			dp->base[DU_CTUR] = div >> 8;
			dp->base[DU_CTLR] = div & 255;
			if (dp->ocounter == 0) {
				/* not previously used? */
				u_char temp;
				/* start C/T running */
				temp = dp->base[DU_CSTRT];
				__USE(temp);
			}
			dp->ocounter = dp->counter;
		}
	} else {
		/* counter not in use; mark as free */
		dp->counter = 0;
	}
	sc->sc_heldchanges = 0;

	/*
	 * delay a tiny bit to try and avoid tx glitching.
	 * I know we're at spltty(), but this is much better than the
	 * old version used DELAY((96000 / out_speed) * 10000)
	 * -plb
	 */
	DELAY(10);
}

/*
 * iterator function for speeds.
 * (could be called "findnextcode")
 * Returns sequence of possible speed codes for a given rate.
 * should set index to zero before first call.
 *
 * Could be implemented as a "checkspeed()" function called
 * to evaluate table entries, BUT this allows more variety in
 * use of C/T with fewer table entries.
 */

static int
scniter(int *index, int wanted, int *counter, int *mode, struct chan *other,
    int c92)
{

	while (*index < TABENTRIES) {
		struct tabent *tp;

		tp = table + (*index)++;
		if (tp->speed != wanted)
			continue;

		/* if not a 26C92 only look at MODE0 entries */
		if (!c92 && (tp->mode & MR0_MODE) != MR0_MODE_0)
			continue;

		/*
		 * check mode;
		 * OK if this table entry for current mode, or mode not
		 * yet set, or other channel's rates are available in both
		 * A and B groups.
		 */

		if (tp->mode == *mode || *mode == ANYMODE ||
		    (other != NULL && (tp->mode & MR0_MODE) == (*mode & MR0_MODE) &&
		     bothgroups[other->icode] && bothgroups[other->ocode])) {
			/*
			 * for future table entries specifying
			 * use of counter/timer
			 */
			if (tp->code == USE_CT) {
				if (*counter != wanted && *counter != 0)
					continue;	/* counter busy */
				*counter = wanted;
			}
			*mode = tp->mode;
			return tp->code;
		}
	}

	/* here after returning all applicable table entries */
	/* XXX return sequence of USE_CT with all possible modes?? */
	if ((*index)++ == TABENTRIES) {
		/* Max C/T rate (even on 26C92?) is 57600 */
		if (wanted <= 57600 && (*counter == wanted || *counter == 0)) {
			*counter = wanted;
			return USE_CT;
		}
	}

	return -1;			/* FAIL */
}

/*
 * calculate configuration
 * rewritten 2/97 -plb
 */
static int
scn_config(int unit, int chan, int ispeed, int ospeed, u_char mr1, u_char mr2)
{
	struct scn_softc *sc;
	struct duart *dp;
	int other;		/* opposite of chan */
	int mode;
	int counter;
	int i, o;		/* input, output iterator indexes */
	int ic, oc;		/* input, output codes */
	struct chan *ocp;	/* other duart channel */
	struct tty *otp;	/* other channel tty struct */
	int c92;		/* true if duart is sc26c92 */
	int s;

	/* Set up softc pointer. */
	if (unit >= scn_cd.cd_ndevs)
		return ENXIO;
	sc = SOFTC(unit);
	chan = sc->sc_channel;
	other = chan ^ 1;
	dp = sc->sc_duart;
	ocp = &dp->chan[other];
	otp = ocp->tty;
	c92 = (dp->type == SC26C92);

	/*
	 * Right now the first combination that works is used.
	 * Perhaps it should search entire solution space for "best"
	 * combination. For example, use heuristic weighting of mode
	 * preferences, and use of counter timer?
	 *
	 * For example right now with 2681/2692 when default rate is
	 * 9600 and other channel is closed setting 19200 will pick
	 * mode 0a and use counter/timer.  Better solution might be
	 * mode 0b, leaving counter/timer free!
	 *
	 * When other channel is open might want to prefer
	 * leaving counter timer free, or not flipping A/B group?
	 */
	if (otp && (otp->t_state & TS_ISOPEN)) {

		/*
		 * Other channel open;
		 * Find speed codes compatible with current mode/counter.
		 */

		i = 0;
		for (;;) {
			mode = dp->mode;
			counter = dp->counter;

			/* NOTE: pass other chan pointer to allow group flipping */
			ic = scniter(&i, ispeed, &counter, &mode, ocp, c92);
			if (ic == -1)
				break;

			o = 0;
			if ((oc = scniter(&o, ospeed, &counter,
					  &mode, NULL, c92)) != -1) {
				/*
				 * take first match
				 *
				 * Perhaps calculate heuristic "score",
				 * save score,codes,mode,counter if score
				 * better than previous best?
				 */
				goto gotit;
			}
		}
		/* XXX try looping for ospeed? */
	} else {
		/* other channel closed */
		int oo, oi;	/* other input, output iterators */
		int oic, ooc;	/* other input, output codes */

		/*
		 * Here when other channel closed.  Finds first
		 * combination that will allow other channel to be opened
		 * (with defaults) and fits our needs.
		 */
		oi = 0;
		for (;;) {
			mode = ANYMODE;
			counter = 0;

			oic = scniter(&oi, ocp->ispeed, &counter, &mode, NULL, c92);
			if (oic == -1)
				break;

			oo = 0;
			while ((ooc = scniter(&oo, ocp->ospeed, &counter,
					   &mode, NULL, c92)) != -1) {
				i = 0;
				while ((ic = scniter(&i, ispeed, &counter,
						  &mode, NULL, c92)) != -1) {
					o = 0;
					if ((oc = scniter(&o, ospeed, &counter,
						       &mode, NULL, c92)) != -1) {
						/*
						 * take first match
						 *
						 * Perhaps calculate heuristic
						 * "score", save
						 *     score,codes,mode,counter
						 * if score better than
						 * previous best?
						 */
						s = spltty();
						dp->chan[other].icode = oic;
						dp->chan[other].ocode = ooc;
						goto gotit2;
					}
				}
			}
		}
	}
	return EINVAL;

 gotit:
	s = spltty();
 gotit2:
	dp->chan[chan].new_mr1 = mr1;
	dp->chan[chan].new_mr2 = mr2;
	dp->chan[chan].ispeed = ispeed;
	dp->chan[chan].ospeed = ospeed;
	dp->chan[chan].icode = ic;
	dp->chan[chan].ocode = oc;
	if (mode == ANYMODE)		/* no mode selected?? */
		mode = DEFMODE(c92);
	dp->mode = mode;
	dp->counter = counter;

	scn_setchip(sc);		/* set chip now, if possible */
	splx(s);
	return (0);
}

int
scn_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if ((mach_type == MACH_SGI_IP6 || mach_type == MACH_SGI_IP10) &&
	    ma->ma_addr == 0x1fb80004)
		return (1);

	return (0);
}

/*
 * No need to make scn_rx{en,dis}able too efficient,
 * they're only called on setup, open & close!
 */
static inline void
scn_rxenable(struct scn_softc *sc)
{
	struct duart *dp;
	int channel;

	dp = sc->sc_duart;
	channel = sc->sc_channel;

	/* Outputs wire-ored and connected to ICU input for fast rx interrupt. */
	if (channel == 0)
		dp->opcr |= OPCR_OP4_RXRDYA;
	else
		dp->opcr |= OPCR_OP5_RXRDYB;
	dp->base[DU_OPCR] = dp->opcr;
	dp->imr |= sc->sc_rx_int;
	dp->base[DU_IMR] = dp->imr;
}

static inline void
scn_rxdisable(struct scn_softc *sc)
{
	struct duart *dp;
	int channel;

	dp = sc->sc_duart;
	channel = sc->sc_channel;

	/* Outputs wire-ored and connected to ICU input for fast rx interrupt. */
	if (channel == 0)
		dp->opcr &= ~OPCR_OP4_RXRDYA;
	else
		dp->opcr &= ~OPCR_OP5_RXRDYB;
	dp->base[DU_OPCR] = dp->opcr;
	dp->imr &= ~sc->sc_rx_int;
	dp->base[DU_IMR] = dp->imr;
}

void
scn_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = aux;
	struct scn_softc *sc;
	struct duart *duart;
	volatile u_char *ch_base;
	volatile u_char *duart_base;
	int channel;
	int speed;
	int s;
	int maj __diagused;
	u_char unit;
	u_char duartno;
	u_char delim = ':';
	u_char mr1, mr2;
	enum scntype scntype = SCNUNK;
	const char *duart_type = "Unknown";
	char *intrname;
	bool console, first;
	devmajor_t major;

	(void)major;

	sc = device_private(self);
	unit = device_unit(self);

	/* XXX - hard-coded */
	if (ma->ma_addr == 0x1fb80004)
		duartno = 1;
	else
		duartno = 0;
	channel = 0;
	console = 1;

	duart = sc->sc_duart = &scn_duart[duartno];
	duart->chan[channel].sc = sc;
	first =	(duart->base == NULL);

	if (console) {
		sc->sc_isconsole = 1;
		sc->sc_swflags |= SCN_SW_SOFTCAR;	/* ignore carrier */
	}

	duart_base = (volatile u_char *)MIPS_PHYS_TO_KSEG1(ma->ma_addr);
	ch_base    = duart_base; /* XXX */

	if (first) {
		/* Probe DUART type */
		s = spltty();
		if (console) {
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
				scntype = SC26C92;
			} else {
				/* 2681 treats as MR1 Select */
				ch_base[CH_CR] = CR_CMD_RTS_OFF;
				RECOVER();
				ch_base[CH_MR] = 1;
				ch_base[CH_MR] = 0;
				ch_base[CH_CR] = CR_CMD_RTS_OFF;
				RECOVER();
				if (ch_base[CH_MR] == 1) {
					duart_type = "scn2681";
					scntype = SCN2681;
				} else {
					duart_type = "scn2692";
					scntype = SCN2692;
				}
			}
		}

		/* If a 2681, the CR_CMD_MR0 is interpreted as a TX_RESET */
		if (console) {
			ch_base[CH_CR] = CR_ENA_TX;
			RECOVER();
		}
		ch_base[CH_CR] = CR_CMD_MR1;
		RECOVER();
		ch_base[CH_MR] = mr1;
		ch_base[CH_MR] = mr2;
		splx(s);

		intrname = malloc(sizeof("scnXX"), M_DEVBUF, M_NOWAIT);
		snprintf(intrname, sizeof("scnXX"), "scn%d", unit);

		/*
		 * On IP6 the console chip is duart1. The keyboard/mouse
		 * is duart0. Each chip has two channels and the channels
		 * share an interrupt. Duart0 is interrupt 0, duart1 is
		 * interrupt 1.
		 */
		if (duartno != 0 && duartno != 1)
			panic("scn_attach: bad duartno: %d", duartno);
		cpu_intr_establish(duartno, IPL_TTY, scnintr, duart);

		printf("%c %s", delim, duart_type);
		delim = ',';

		duart->base = duart_base;
		duart->type = scntype;
	}
	/* Record channel, uart */
	sc->sc_channel = channel;
	sc->sc_chbase = ch_base;

	/* Initialize modem/interrupt bit masks */
	if (channel == 0) {
		sc->sc_op_rts = OP_RTSA;
		sc->sc_op_dtr = OP_DTRA;
		sc->sc_ip_cts = IP_CTSA;
		sc->sc_ip_dcd = IP_DCDA;

		sc->sc_tx_int = INT_TXA;
		sc->sc_rx_int = INT_RXA;
	} else {
		sc->sc_op_rts = OP_RTSB;
		sc->sc_op_dtr = OP_DTRB;
		sc->sc_ip_cts = IP_CTSB;
		sc->sc_ip_dcd = IP_DCDB;

		sc->sc_tx_int = INT_TXB;
		sc->sc_rx_int = INT_RXB;
	}

	/* Initialize counters */
	sc->sc_framing_errors = 0;
	sc->sc_fifo_overruns = 0;
	sc->sc_parity_errors = 0;
	sc->sc_breaks = 0;

	if (console) {
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
	if (first) {
		/* Disable all interrupts. */
		duart_base[DU_IMR] = duart->imr = 0;

		/* Output port config */
		duart_base[DU_OPCR] = duart->opcr = 0;

		/* Speeds... */
		duart->mode = 0;

		/*
		 * Set initial speed to an illegal code that can be changed to
		 * any other baud.
		 */
		duart->chan[0].icode = duart->chan[0].ocode = 0x2f;
		duart->chan[1].icode = duart->chan[1].ocode = 0x2f;
		duart->chan[0].ispeed = duart->chan[0].ospeed = 0;
		duart->chan[1].ispeed = duart->chan[1].ospeed = 0;

		duart->acr = 0;
		duart->acr |= ACR_CT_TCLK1;	/* timer mode 1x clk */
	}

	if (channel == 0) {
		duart->acr |= ACR_DELTA_DCDA;	/* Set CD int */
	} else {
		duart->acr |= ACR_DELTA_DCDB;	/* Set CD int */
	}

	if (scnsir == NULL) {
		/* software intr: calls tty code, hence IPL_TTY */
		scnsir = softint_establish(SOFTINT_SERIAL, scnsoft, NULL);
	}

	duart_base[DU_ACR] = (duart->mode & ACR_BRG) | duart->acr;

	if (console)
		speed = scnconsrate;
	else
		speed = scndefaultrate;

	scn_config(unit, channel, speed, speed, MR1_PNONE | MR1_CS8, MR2_STOP1);
	if (console) {
		maj = cdevsw_lookup_major(&scn_cdevsw);
		KASSERT(maj != NODEVMAJOR);
		shutdownhook_establish(scncnreinit, NULL);
		/* Make sure console can do scncngetc */
		duart_base[DU_OPSET] = channel ? (OP_RTSB | OP_DTRB) :
			(OP_RTSA | OP_DTRA);
	}

	/* Turn on the receiver and transmitters */
	ch_base[CH_CR] = CR_ENA_RX | CR_ENA_TX;

	/* Set up the interrupts. */
	duart->imr |= INT_IP;
	scn_rxdisable(sc);
	splx(s);

	if (sc->sc_swflags) {
		printf("%c flags %d", delim, sc->sc_swflags);
		delim = ',';
	}

#ifdef KGDB
	major = cdevsw_lookup_major(&scn_cdevsw);
	KASSERT(major != NODEVMAJOR);
	if (kgdb_dev == makedev(major, unit)) {
		if (console)
			kgdb_dev = NODEV; /* can't debug over console port */
		else {
			scninit(kgdb_dev, kgdb_rate);
			scn_rxenable(sc);
			scn->sc_iskgdb = 1;
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
scnopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct tty *tp;
	int unit = DEV_UNIT(dev);
	struct scn_softc *sc;
	int error = 0;

	if (unit >= scn_cd.cd_ndevs)
		return ENXIO;
	sc = SOFTC(unit);
	if (!sc)
		return ENXIO;

	tp = sc->sc_tty;
	if (!tp) {
		tp = tty_alloc();
		sc->sc_tty = sc->sc_duart->chan[sc->sc_channel].tty = tp;
		tty_attach(tp);
	}

	tp->t_oproc = scnstart;
	tp->t_param = scnparam;
	tp->t_hwiflow = scnhwiflow;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	mutex_spin_enter(&tty_lock);

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = SCNDEF_CFLAG;

		sc->sc_rx_blocked = 0;

		if (sc->sc_swflags & SCN_SW_CLOCAL)
			tp->t_cflag |= CLOCAL;
		if (sc->sc_swflags & SCN_SW_CRTSCTS)
			tp->t_cflag |= CCTS_OFLOW | CRTS_IFLOW;
		tp->t_lflag = TTYDEF_LFLAG;
		if (sc->sc_isconsole)
			tp->t_ispeed = tp->t_ospeed = scnconsrate;
		else
			tp->t_ispeed = tp->t_ospeed = scndefaultrate;
		scnparam(tp, &tp->t_termios);
		ttsetwater(tp);

		/* Turn on DTR and RTS. */
		SCN_OP_BIS(sc, sc->sc_op_rts | sc->sc_op_dtr);

		/* enable receiver interrupts */
		scn_rxenable(sc);

		/* set carrier state; */
		if ((sc->sc_swflags & SCN_SW_SOFTCAR) || /* check ttyflags */
		    SCN_DCD(sc) ||			 /* check h/w */
		    DEV_DIALOUT(dev))
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;
	}

	mutex_spin_exit(&tty_lock);

	error = ttyopen(tp, SCN_DIALOUT(sc), flags & O_NONBLOCK);
if (error) printf("ttyopen failed line %d, error %d\n", __LINE__, error);
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open) (dev, tp);
if (error) printf("l_open failed line %d, error %d\n", __LINE__, error);
	if (error)
		goto bad;

	return (0);

bad:
	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		scn_rxdisable(sc);
		SCN_OP_BIC(sc, sc->sc_op_rts | sc->sc_op_dtr);
	}

	return (error);
}


/*ARGSUSED*/
int
scnclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = DEV_UNIT(dev);
	struct scn_softc *sc = SOFTC(unit);
	struct tty *tp = sc->sc_tty;
	devmajor_t major;

	(void)major;

	if ((tp->t_state & TS_ISOPEN) == 0)
		return 0;

	(*tp->t_linesw->l_close) (tp, flags);

#ifdef KGDB
	/* do not disable interrupts if debugging */
	major = cdevsw_lookup_major(&scn_devsw);
	KASSERT(major != cdevsw_lookup_major);
	if (kgdb_dev != makedev(major, unit))
#endif
		if ((tp->t_state & TS_ISOPEN) == 0) {
			scn_rxdisable(sc);
		}
	if ((tp->t_cflag & HUPCL) && (sc->sc_swflags & SCN_SW_SOFTCAR) == 0) {
		SCN_OP_BIC(sc, sc->sc_op_dtr);
		/* hold low for 1 second */
		tsleep(sc, TTIPRI, ttclos, hz);
	}
	SCN_CLRDIALOUT(sc);
	ttyclose(tp);

#if 0
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tty_free(tp);
		sc->sc_tty = (struct tty *) NULL;
	}
#endif

	return (0);
}

int
scnread(dev_t dev, struct uio *uio, int flags)
{
	struct scn_softc *sc = SOFTC(DEV_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read) (tp, uio, flags));
}

int
scnwrite(dev_t dev, struct uio *uio, int flags)
{
	struct scn_softc *sc = SOFTC(DEV_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write) (tp, uio, flags));
}

int
scnpoll(dev_t dev, int events, struct lwp *l)
{
	struct scn_softc *sc = SOFTC(DEV_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
scntty(dev_t dev)
{
	struct scn_softc *sc = SOFTC(DEV_UNIT(dev));

	return sc->sc_tty;
}

/* Worker routines for interrupt processing */
static inline void
dcd_int(struct scn_softc *sc, struct tty *tp, u_char new)
{

	if (sc->sc_swflags & SCN_SW_SOFTCAR)
		return;

#if 0
	printf("scn%d: dcd_int ip %x SCN_DCD %x new %x ipcr %x\n",
	    sc->unit,
	    sc->sc_duart->base[DU_IP],
	    SCN_DCD(sc),
	    new,
	    sc->sc_duart->base[DU_IPCR]
	    );
#endif

/* XXX set some flag to have some lower (soft) int call line discipline? */
	if (!(*tp->t_linesw->l_modem) (tp, new == 0? 1: 0)) {
		SCN_OP_BIC(sc, sc->sc_op_rts | sc->sc_op_dtr);
	}
}

/*
 * Print out a ring or fifo overrun error message.
 */
static void
scnoverrun(int unit, long *ptime, const char *what)
{

	if (*ptime != time_second) {
		*ptime = time_second;
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
scnhwiflow(struct tty *tp, int stop)
{
	int unit = DEV_UNIT(tp->t_dev);
	struct scn_softc *sc = SOFTC(unit);
	int s;

	s = splrtty();
	if (!stop) {
		if (sc->sc_rbput - sc->sc_rbget - 1) {
			setsoftscn();
		}
	}
	splx(s);
	return 1;
}

static int
scnintr(void *arg)
{
	struct duart *duart = arg;
	struct scn_softc *sc0 = duart->chan[0].sc;
	struct scn_softc *sc1 = duart->chan[1].sc;

	struct tty *tp0 = (sc0 != NULL) ? sc0->sc_tty : NULL;
	struct tty *tp1 = (sc1 != NULL) ? sc1->sc_tty : NULL;

	char rs_work;
	u_char rs_stat;
	u_char rs_ipcr;

	/* Check for RX interrupts first, since we cannot distinguish by irq. */
	scnrxintr(duart);

	do {
		/* Loop to pick up ALL pending interrupts for device. */
		rs_work = false;
		rs_stat = duart->base[DU_ISR];

/* channel a */
		if (tp0 != NULL) {
			if ((rs_stat & INT_TXA) && (tp0->t_state & TS_BUSY)) {
				/* output char done. */
				tp0->t_state &= ~(TS_BUSY | TS_FLUSH);

				/* disable tx ints */
				duart->imr &= ~sc0->sc_tx_int;
				duart->base[DU_IMR] = duart->imr;

				if (sc0->sc_heldchanges) {
					scn_setchip(sc0);
				}

				(*tp0->t_linesw->l_start) (tp0);
				rs_work = true;
			}
		}
		/* channel b */
		if (tp1 != NULL) {
			if ((rs_stat & INT_TXB) && (tp1->t_state & TS_BUSY)) {
				/* output char done. */
				tp1->t_state &= ~(TS_BUSY | TS_FLUSH);

				/* disable tx ints */
				duart->imr &= ~sc1->sc_tx_int;
				duart->base[DU_IMR] = duart->imr;

				if (sc1->sc_heldchanges) {
					scn_setchip(sc1);
				}

				(*tp1->t_linesw->l_start) (tp1);
				rs_work = true;
			}
		}
		if (rs_stat & INT_IP) {
			rs_work = true;
			rs_ipcr = duart->base[DU_IPCR];

			if (rs_ipcr & IPCR_DELTA_DCDA && tp0 != NULL) {
				dcd_int(sc0, tp0, rs_ipcr & IPCR_DCDA);
			}
			if (rs_ipcr & IPCR_DELTA_DCDB && tp1 != NULL) {
				dcd_int(sc1, tp1, rs_ipcr & IPCR_DCDB);
			}
		}
	} while (rs_work);

	return (1);	/* ? */
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
static inline int
scn_rxintr(struct scn_softc *sc)
{
	char sr;
	int i, n;
	int work;

	work = 0;
	i = sc->sc_rbput;
	while (work <= 10) {
#define SCN_GETCH(SC) \
		sr = (SC)->sc_chbase[CH_SR]; \
		if ((sr & SR_RX_RDY) == 0) \
			break; \
		if (sr & (SR_PARITY | SR_FRAME | SR_BREAK | SR_OVERRUN)) \
			goto exception; \
		work++; \
		(SC)->sc_rbuf[i++ & SCN_RING_MASK] = (SC)->sc_chbase[CH_DAT]

		SCN_GETCH(sc); SCN_GETCH(sc); SCN_GETCH(sc);
		/* XXX more here if 26C92? -plb */
		continue;
	exception:
#if defined(DDB)
		if (sc->sc_isconsole && (sr & SR_BREAK)) {
			Debugger();
			sr = sc->sc_chbase[CH_SR];
		}
#endif
#if defined(KGDB)
		if (sc->sc_iskgdb && (sr & SR_RX_RDY)) {
			kgdb_connect(1);
			sr = sc->sc_chbase[CH_SR];
		}
#endif
		work++;
		sc->sc_rbuf[i++ & SCN_RING_MASK] = (sr << 8) | sc->sc_chbase[CH_DAT];
		sc->sc_chbase[CH_CR] = CR_CMD_RESET_ERR;	/* resets break? */
		RECOVER();
	}
	/*
	 * If ring is getting too full, try to block input.
	 */
	n = i - sc->sc_rbget;
	if (sc->sc_rbhiwat && (n > sc->sc_rbhiwat)) {
		/* If not CRTSCTS sc_rbhiwat is such that this
		 *  never happens.
		 * Clear RTS
		 */
		SCN_OP_BIC(sc, sc->sc_op_rts);
		sc->sc_rx_blocked = 1;
	}
	sc->sc_rbput = i;

	return work;
}

static void
scnrxintr(void *arg)
{
	struct duart *duart = arg;
	int work = 0;

	if (duart->chan[0].sc != NULL)
		work += scn_rxintr(duart->chan[0].sc);
	if (duart->chan[1].sc != NULL)
		work += scn_rxintr(duart->chan[1].sc);
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
 * Dave's original scheme was to use the DUART receiver timeout
 * interrupt. This requires 2692's (which my board doesn't have), and
 * I also liked the idea of using the C/T to generate alternate and/or
 * arbitrary bauds. -plb
 *
 * The ringbuffer code comes from Chris Torek's SPARC 44bsd zs driver
 * (hence the LBL notice on top of this file), DOES NOT require
 * interlocking with interrupt levels!
 *
 * The 44bsd sparc/zs driver reads the ring buffer from a separate
 * zssoftint, while the SunOS 4.x zs driver appears to use
 * timeout()'s.  timeouts seem to be too slow to deal with high data
 * rates.  I know, I tried them.
 * -plb.
 */
static void
scnsoft(void *arg)
{
	int s, unit;
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
		struct scn_softc *sc;
		struct tty *tp;
		int n, get;

		sc = SOFTC(unit);
		if (sc == NULL) {
			continue;
		}
		tp = sc->sc_tty;
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
				sc->sc_ring_overruns++;
			}
			while (--n >= 0) {
				int c, sr;

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
					sc->sc_fifo_overruns++;
				}
				if (sr & SR_PARITY) {
					c |= TTY_PE;
					sc->sc_parity_errors++;
				}
				if (sr & SR_FRAME) {
					c |= TTY_FE;
					sc->sc_framing_errors++;
				}
				if (sr & SR_BREAK) {
#if 0
					/*
					 * See DDB_CHECK() comments in
					 * scnrxintr()
					 */
					if (sc->sc_isconsole)
						Debugger();
#endif
					c = TTY_FE | 0;
					sc->sc_breaks++;
				}

				(*tp->t_linesw->l_rint) (c, tp);

				if (sc->sc_rx_blocked && n < SCN_RING_THRESH) {
					s = splrtty();
					sc->sc_rx_blocked = 0;
					SCN_OP_BIS(sc, sc->sc_op_rts);
					splx(s);
				}

			}
			sc->sc_rbget = get;
		}
	done: ;
	}
}

/* Convert TIOCM_xxx bits to output port bits. */
static unsigned char
opbits(struct scn_softc *sc, int tioc_bits)
{

	return ((((tioc_bits) & TIOCM_DTR) ? sc->sc_op_dtr : 0) |
	    (((tioc_bits) & TIOCM_RTS) ? sc->sc_op_rts : 0));
}

int
scnioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	int unit = DEV_UNIT(dev);
	struct scn_softc *sc = SOFTC(unit);
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl) (tp, cmd, data, flags, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flags, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {
	case TIOCSBRK:
		sc->sc_chbase[CH_CR] = CR_CMD_START_BRK;
		break;

	case TIOCCBRK:
		sc->sc_chbase[CH_CR] = CR_CMD_STOP_BRK;
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
			unsigned char ip;

			/* s = spltty(); */
			ip = sc->sc_duart->base[DU_IP];
			/* splx(s); */

			bits = 0;
			if (ip & sc->sc_ip_dcd)
				bits |= TIOCM_CD;
			if (ip & sc->sc_ip_cts)
				bits |= TIOCM_CTS;

#if 0
			/*
			 * XXX sigh; cannot get op current state!! even if
			 * maintained in private, RTS is done in h/w!!
			 */
			unsigned char op = 0;
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

			if (sc->sc_swflags & SCN_SW_SOFTCAR)
				bits |= TIOCFLAG_SOFTCAR;
			if (sc->sc_swflags & SCN_SW_CLOCAL)
				bits |= TIOCFLAG_CLOCAL;
			if (sc->sc_swflags & SCN_SW_CRTSCTS)
				bits |= TIOCFLAG_CRTSCTS;
			if (sc->sc_swflags & SCN_SW_MDMBUF)
				bits |= TIOCFLAG_MDMBUF;

			*(int *) data = bits;
			break;
		}
	case TIOCSFLAGS:{
			int     userbits, driverbits = 0;

			error = kauth_authorize_device_tty(l->l_cred,
			    KAUTH_DEVICE_TTY_PRIVSET, tp);
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

			sc->sc_swflags = driverbits;

			break;
		}

	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

int
scnparam(struct tty *tp, struct termios *t)
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
		mr1 |= MR1_CS5;
		break;
	case CS6:
		mr1 |= MR1_CS6;
		break;
	case CS7:
		mr1 |= MR1_CS7;
		break;
	case CS8:
	default:
		mr1 |= MR1_CS8;
		break;
	}

	if (cflag & CCTS_OFLOW)
		mr2 |= MR2_TXCTS;

	if (cflag & CRTS_IFLOW) {
		mr1 |= MR1_RXRTS;
		sc->sc_rbhiwat = SCN_RING_HIWAT;
	} else {
		sc->sc_rbhiwat = 0;
	}

	error = scn_config(unit, sc->sc_channel, t->c_ispeed,
	    t->c_ospeed, mr1, mr2);

	/* If successful, copy to tty */
	if (!error) {
		tp->t_ispeed = t->c_ispeed;
		tp->t_ospeed = t->c_ospeed;
		tp->t_cflag = cflag;
	}
	return (error);
}

/*
 * Start or restart a transmission.
 */
void
scnstart(struct tty *tp)
{
	int s, c;
	int unit = DEV_UNIT(tp->t_dev);
	struct scn_softc *sc = SOFTC(unit);

	s = spltty();
	if (tp->t_state & (TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (!ttypull(tp))
		goto out;

	tp->t_state |= TS_BUSY;

	while (sc->sc_chbase[CH_SR] & SR_TX_RDY) {
		if ((c = getc(&tp->t_outq)) == -1)
			break;
		sc->sc_chbase[CH_DAT] = c;
	}
	sc->sc_duart->imr |= (sc->sc_tx_int | sc->sc_rx_int);
	sc->sc_duart->base[DU_IMR] = sc->sc_duart->imr;

out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
void
scnstop(struct tty *tp, int flags)
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 * Following are all routines needed for SCN to act as console.
 */

void
scncnprobe(struct consdev *cn)
{
}

void
scncnreinit(void *v)
{
	volatile u_char *du_base =
	    (volatile u_char *)MIPS_PHYS_TO_KSEG1(0x1fb80004);

	du_base[DU_OPSET] =
	    SCN_CONSCHAN ? (OP_RTSB | OP_DTRB) : (OP_RTSA | OP_DTRA);
}

void
scncninit(struct consdev *cn)
{
	devmajor_t major;

	/* initialize required fields */
	major = cdevsw_lookup_major(&scn_cdevsw);
	KASSERT(major != NODEV);
	cn->cn_dev = makedev(major, SCN_CONSOLE);
	cn->cn_pri = CN_REMOTE;

	scninit(cn->cn_dev, scnconsrate);
}

/* Used by scncninit and kgdb startup. */
int
scninit(dev_t dev, int rate)
{
/* XXX - maintain PROM's settings */
#if 0
	volatile u_char *du_base =
	    (volatile u_char *)MIPS_PHYS_TO_KSEG1(0x1fb80004);
	int unit = DEV_UNIT(dev);

	du_base[DU_OPSET] =
	    SCN_CONSCHAN ? (OP_RTSB | OP_DTRB) : (OP_RTSA | OP_DTRA);
	scn_config(unit, SCN_CONSCHAN, rate, rate,
	    MR1_PNONE | MR1_CS8, MR2_STOP1);
#endif
	return (0);
}

/*
 * Console kernel input character routine.
 */
int
scncngetc(dev_t dev)
{
	volatile u_char *ch_base =
	    (volatile u_char *)MIPS_PHYS_TO_KSEG1(0x1fb80004);
	char c;
	int s;

	s = spltty();

	while ((ch_base[CH_SR] & SR_RX_RDY) == 0)
		;
	c = ch_base[CH_DAT];

	splx(s);
	return c;
}

void
scncnpollc(dev_t dev, int on)
{
}

/*
 * Console kernel output character routine.
 */
void
scncnputc(dev_t dev, int c)
{
	volatile u_char *ch_base =
	    (volatile u_char *)MIPS_PHYS_TO_KSEG1(0x1fb80004);
	volatile u_char *du_base =
	    (volatile u_char *)MIPS_PHYS_TO_KSEG1(0x1fb80004);
	int s;

	s = spltty();

	if (c == '\n')
		scncnputc(dev, '\r');

	while ((ch_base[CH_SR] & SR_TX_RDY) == 0)
		;
	ch_base[CH_DAT] = c;
	while ((ch_base[CH_SR] & SR_TX_RDY) == 0)
		;
	du_base[DU_ISR];

	splx(s);
}
