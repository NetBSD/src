/*	$NetBSD: scc.c,v 1.78 2003/02/23 03:37:40 simonb Exp $	*/

/*
 * Copyright (c) 1991,1990,1989,1994,1995,1996 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Author: Chris G. Demetriou and Jonathan Stone
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)scc.c	8.2 (Berkeley) 11/30/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: scc.c,v 1.78 2003/02/23 03:37:40 simonb Exp $");

/*
 * Intel 82530 dual usart chip driver. Supports the serial port(s) on the
 * Personal DECstation 5000/xx and DECstation 5000/1xx, plus the keyboard
 * and mouse on the 5000/1xx. (Don't ask me where the A channel signals
 * are on the 5000/xx.)
 *
 * See: Intel MicroCommunications Handbook, Section 2, pg. 155-173, 1992.
 */

#include "opt_ddb.h"
#include "rasterconsole.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <dev/cons.h>
#include <dev/dec/lk201.h>
#include <dev/ic/z8530reg.h>

#include <machine/pmioctl.h>		/* XXX for pmEventQueue typedef */

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/maxine.h>
#include <pmax/dev/sccreg.h>
#include <pmax/dev/rconsvar.h>
#include <pmax/dev/pdma.h>		/* XXXXXX */
#include <pmax/dev/lk201var.h>
#include <pmax/dev/qvssvar.h>		/* XXX mouseInput() */
#include <pmax/tc/sccvar.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>

void	ttrstrt __P((void *));


/*
 * True iff the console unit is diverted through this SCC device.
 * (used to just test if cn_tab->cn_getc was sccGetc, but that
 * breaks with the new-style glass-tty framebuffer console input.
 */

#define CONSOLE_ON_UNIT(unit) \
  (major(cn_tab->cn_dev) == cdevsw_lookup_major(&scc_cdevsw) && \
   SCCUNIT(cn_tab->cn_dev) == (unit))


/*
 * Extract unit (scc chip), channel on chip, and dialin/dialout unit.
 */
#define	SCCUNIT(dev)	((minor(dev) & 0x7ffff) >> 1)
#define	SCCLINE(dev)	(minor(dev) & 0x1)
#define	SCCDIALOUT(x)	(minor(x) & 0x80000)

#ifdef DEBUG
int	debugChar;
#endif

struct scc_softc {
	struct device sc_dv;
	struct pdma scc_pdma[2];
	struct {
		u_char	wr1;
		u_char	wr3;
		u_char	wr4;
		u_char	wr5;
		u_char	wr14;
	} scc_wreg[2];
	struct tty *scc_tty[2];
	int	scc_softCAR;

	int scc_flags[2];
#define SCC_CHAN_NEEDSDELAY	0x01	/* sw must delay 1.6us between output*/
#define SCC_CHAN_NOMODEM	0x02	/* don't touch modem ctl lines (may
					   be left floating or x-wired */
#define SCC_CHAN_MODEM_CROSSED	0x04	/* modem lines wired to	other channel*/
#define SCC_CHAN_KBDLINE	0x08	/* XXX special-case keyboard lines */
	int scc_unitflags;	/* flags for both channels, e.g. */
#define	SCC_PREFERRED_CONSOLE	0x01
};

/*
 * BRG formula is:
 *				ClockFrequency
 *	BRGconstant =	---------------------------  -  2
 *			2 * BaudRate * ClockDivider
 *
 * Speed selections with Pclk=7.3728Mhz, clock x16
 */
struct speedtab sccspeedtab[] = {
	{ 0,		0,	},
	{ 50,		4606,	},
	{ 75,		3070,	},
	{ 110,		2093,	},
	{ 134.5,	1711,	},
	{ 150,		1534,	},
	{ 200,		1150,	},
	{ 300,		766,	},
	{ 600,		382,	},
	{ 1200,		190,	},
	{ 1800,		126,	},
	{ 2400,		94,	},
	{ 4800,		46,	},
	{ 7200,		30,	},	/* non-POSIX */
	{ 9600,		22,	},
	{ 14400,	14,	},	/* non-POSIX */
	{ 19200,	10,	},
	{ 28800,	6,	},	/* non-POSIX */
	{ 38400,	4,	},
	{ 57600,	2,	},	/* non-POSIX */
#ifndef SCC_NO_HIGHSPEED
	{ 76800,	1,	},	/* non-POSIX, doesn't work reliably */
	{ 115200, 	0	},	/* non-POSIX, doesn't work reliably */
#endif
	{ -1,		-1,	},
};

#ifndef	PORTSELECTOR
#define	ISPEED	TTYDEF_SPEED
#define	LFLAG	TTYDEF_LFLAG
#else
#define	ISPEED	B4800
#define	LFLAG	(TTYDEF_LFLAG & ~ECHO)
#endif

/* Definition of the driver for autoconfig. */
static int	sccmatch  __P((struct device *parent, struct cfdata *cf,
		    void *aux));
static void	sccattach __P((struct device *parent, struct device *self,
		    void *aux));

CFATTACH_DECL(scc, sizeof (struct scc_softc),
    sccmatch, sccattach, NULL, NULL);

extern struct cfdriver scc_cd;

dev_type_open(sccopen);
dev_type_close(sccclose);
dev_type_read(sccread);
dev_type_write(sccwrite);
dev_type_ioctl(sccioctl);
dev_type_stop(sccstop);
dev_type_tty(scctty);
dev_type_poll(sccpoll);

const struct cdevsw scc_cdevsw = {
	sccopen, sccclose, sccread, sccwrite, sccioctl,
	sccstop, scctty, sccpoll, nommap, ttykqfilter, D_TTY
};

/* QVSS-compatible in-kernel X input event parser, pointer tracker */
void	(*sccDivertXInput) __P((int));
void	(*sccMouseEvent) __P((void *));
void	(*sccMouseButtons) __P((void *));

static void	sccPollc __P((dev_t, int));
static int	sccparam __P((struct tty *, struct termios *));
static void	sccstart __P((struct tty *));
static int	sccmctl __P((dev_t, int, int));
static int	cold_sccparam __P((struct tty *, struct termios *,
		    struct scc_softc *sc));

#ifdef SCC_DEBUG
static void	rr __P((char *, scc_regmap_t *));
#endif
static void	scc_modem_intr __P((dev_t));
static void	sccreset __P((struct scc_softc *));

#if NRASTERCONSOLE > 0
static void	scc_kbd_init __P((struct scc_softc *sc, dev_t dev));
static void	scc_mouse_init __P((struct scc_softc *sc, dev_t dev));
#endif
static void	scc_tty_init __P((struct scc_softc *sc, dev_t dev));

static void	scc_txintr __P((struct scc_softc *, int, scc_regmap_t *));
static void	scc_rxintr __P((struct scc_softc *, int, scc_regmap_t *, int));
static void	scc_stintr __P((struct scc_softc *, int, scc_regmap_t *, int));
static int	sccintr __P((void *));

/*
 * console variables, for using serial console while still cold and
 * autoconfig has not attached the scc device.
 */
scc_regmap_t *scc_cons_addr = 0;
static struct scc_softc coldcons_softc;
static struct consdev scccons = {
	NULL, NULL, sccGetc, sccPutc, sccPollc, NULL, NODEV, 0
};

void
scc_cnattach(base, offset)
	u_int32_t	base;
	u_int32_t	offset;
{
	scc_regmap_t *sccaddr;
	struct scc_softc *sc;
	int dev;

	/* XXX XXX XXX */
	dev = 0;
	if (systype == DS_3MIN || systype == DS_3MAXPLUS)
		dev = SCCCOMM3_PORT;
	else if (systype == DS_MAXINE)
		dev = SCCCOMM2_PORT;
	/* XXX XXX XXX */

	sccaddr = (void *)(base + offset);
	/* Save address in case we're cold. */
	if (cold && scc_cons_addr == 0) {
		scc_cons_addr = sccaddr;
		sc = &coldcons_softc;
		coldcons_softc.scc_pdma[0].p_addr = sccaddr;
		coldcons_softc.scc_pdma[1].p_addr = sccaddr;
	} else {
		/* being called from sccattach() to reset console */
		sc = scc_cd.cd_devs[SCCUNIT(dev)];
	}

	/* Reset chip. */
	/* XXX make sure sccreset() called only once for this chip? */
	sccreset(sc);

	cn_tab = &scccons;
	cn_tab->cn_dev = makedev(cdevsw_lookup_major(&scc_cdevsw), dev);
	cn_tab->cn_pri = CN_NORMAL;
	sc->scc_softCAR |= 1 << SCCLINE(cn_tab->cn_dev);
	scc_tty_init(sc, cn_tab->cn_dev);
}

#if NRASTERCONSOLE > 0
void
scc_lk201_cnattach(base, offset)
	u_int32_t	base;
	u_int32_t	offset;
{
	dev_t dev;

	dev = makedev(cdevsw_lookup_major(&scc_cdevsw), SCCKBD_PORT);
	lk_divert(sccGetc, dev);

	cn_tab = &scccons;
	cn_tab->cn_pri = CN_NORMAL;
	cn_tab->cn_getc = lk_getc;
	rcons_indev(cn_tab); /* cn_dev & cn_putc */
}
#endif

/*
 * Test to see if device is present.
 * Return true if found.
 */
static int
sccmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ioasicdev_attach_args *d = aux;
	void *sccaddr;

	/* Make sure that we're looking for this type of device. */
	if ((strncmp(d->iada_modname, "z8530   ", TC_ROM_LLEN) != 0) &&
	    (strncmp(d->iada_modname, "scc", TC_ROM_LLEN)!= 0))
	    return (0);

	/*
	 * Check user-specified offset against the ioasic offset.
	 * Allow it to be wildcarded.
	 */
	if (cf->cf_loc[IOASICCF_OFFSET] != IOASICCF_OFFSET_DEFAULT &&
	    cf->cf_loc[IOASICCF_OFFSET] != d->iada_offset)
		return (0);

	/* Get the address, and check it for validity. */
	sccaddr = (void *)d->iada_addr;
#ifdef SPARSE
	sccaddr = (void *)TC_DENSE_TO_SPARSE((tc_addr_t)sccaddr);
#endif
	if (badaddr(sccaddr, 2))
		return (0);

	return (1);
}

static void
sccattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct scc_softc *sc = (struct scc_softc *)self;
	struct ioasicdev_attach_args *d = aux;
	struct pdma *pdp;
	struct tty *tp;
	void *sccaddr;
	int cntr;
#if 0
	struct termios cterm;
	struct tty ctty;
	int s;
#endif
	int unit;

	unit = sc->sc_dv.dv_unit;

	sccaddr = (void*)MIPS_PHYS_TO_KSEG1(d->iada_addr);

	/* Register the interrupt handler. */
	ioasic_intr_establish(parent, d->iada_cookie, TC_IPL_TTY,
		sccintr, (void *)sc);

	/*
	 * If this is the console, wait a while for any previous
	 *  output to complete.
	 */
	if (CONSOLE_ON_UNIT(unit))
		DELAY(10000);

	pdp = &sc->scc_pdma[0];

	/* init pseudo DMA structures */
	for (cntr = 0; cntr < 2; cntr++) {
		pdp->p_addr = (void *)sccaddr;
		tp = sc->scc_tty[cntr] = ttymalloc();
		if (systype == DS_MAXINE || cntr == 0)
			tty_attach(tp);	/* XXX */
		pdp->p_arg = (long)tp;
		pdp->p_fcn = NULL;
		tp->t_dev = (dev_t)((unit << 1) | cntr);
		pdp++;
	}
	/* What's the warning here? Defaulting to softCAR on line 2? */
	sc->scc_softCAR = sc->sc_dv.dv_cfdata->cf_flags;

	/*
	 * Reset chip, initialize  register-copies in softc.
	 * XXX if this was console, no more printf()s until after
	 * scc_tty_init()!
	 */
	sccreset(sc);


	/*
	 * Special handling for consoles.
	 */
	if (CONSOLE_ON_UNIT(unit)) {
		/*
		 * We were using PROM callbacks for console I/O,
		 * and we just reset the chip under the console.
		 * Re-wire  this unit up as console ASAP.
		 */
		sc->scc_softCAR |= 1 << SCCLINE(cn_tab->cn_dev);
		scc_tty_init(sc, cn_tab->cn_dev);

		DELAY(20000);

		printf(": console");
	}
	printf("\n");


	/* Wire up any childre, like keyboards or mice. */
#if NRASTERCONSOLE > 0
	if (systype != DS_MAXINE) {
		int maj;
		maj = cdevsw_lookup_major(&scc_cdevsw);
		if (unit == 1) {
			scc_kbd_init(sc, makedev(maj, SCCKBD_PORT));
		} else if (unit == 0) {
			scc_mouse_init(sc, makedev(maj, SCCMOUSE_PORT));
		}
	}
#endif /* NRASTERCONSOLE > 0 */

}


/*
 * Initialize line parameters for a serial console.
 */
static void
scc_tty_init(sc, dev)
	struct scc_softc *sc;
	dev_t dev;
{
	struct termios cterm;
	struct tty ctty;
	int s;

	s = spltty();
	ctty.t_dev = dev;
	cterm.c_cflag = (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8;
	/* XXX -- why on pmax, not on Alpha? */
	cterm.c_cflag  |= CLOCAL;
	cterm.c_ospeed = cterm.c_ispeed = 9600;
	/* scc_tty_init() may be called when very cold */
	(void) cold_sccparam(&ctty, &cterm, sc);
	DELAY(1000);
	splx(s);
}

#if NRASTERCONSOLE > 0
static void
scc_kbd_init(sc, dev)
	struct scc_softc *sc;
	dev_t dev;
{
	struct termios cterm;
	struct tty ctty;
	int s;

	s = spltty();
	ctty.t_dev = dev;
	cterm.c_cflag = CS8;
	/* XXX -- why on pmax, not on Alpha? */
	cterm.c_cflag |= CLOCAL;
	cterm.c_ospeed = cterm.c_ispeed = 4800;
	(void) sccparam(&ctty, &cterm);
	DELAY(10000);
#ifdef notyet
	/*
	 * For some reason doing this hangs the 3min
	 * during booting. Fortunately the keyboard
	 * works ok without it.
	 */
	lk_reset(ctty.t_dev, sccPutc);
#endif /* notyet */
	DELAY(10000);
	splx(s);

}

static void
scc_mouse_init(sc, dev)
	struct scc_softc *sc;
	dev_t dev;
{
	struct termios cterm;
	struct tty ctty;
	int s;
#if NRASTERCONSOLE > 0
	extern const struct cdevsw rcons_cdevsw;
#endif

	s = spltty();
	ctty.t_dev = dev;
	cterm.c_cflag = CS8 | PARENB | PARODD;
	cterm.c_ospeed = cterm.c_ispeed = 4800;
	(void) sccparam(&ctty, &cterm);
#if NRASTERCONSOLE > 0
	/*
	 * This is a hack.  As Ted Lemon observed, we want bstreams,
	 * or failing that, a line discipline to do the inkernel DEC
	 * mouse tracking required by Xservers.
	 */
	if (cdevsw_lookup(cn_tab->cn_dev) != &rcons_cdevsw)
		goto done;

	DELAY(10000);
	lk_mouseinit(ctty.t_dev, sccPutc, sccGetc);
	DELAY(10000);

done:
#endif	/* NRASTERCONSOLE > 0 */

	splx(s);
}
#endif


/*
 * Reset the chip and the softc state.
 * Resetting  clobbers chip state and copies of registers for both channels.
 * The driver assumes this is only ever called once per unit.
 */
static void
sccreset(sc)
	struct scc_softc *sc;
{
	scc_regmap_t *regs;
	u_char val;

	regs = (scc_regmap_t *)sc->scc_pdma[0].p_addr;
	/*
	 * Chip once-only initialization
	 *
	 * NOTE: The wiring we assume is the one on the 3min:
	 *
	 *	out	A-TxD	-->	TxD	keybd or mouse
	 *	in	A-RxD	-->	RxD	keybd or mouse
	 *	out	A-DTR~	-->	DTR	comm
	 *	out	A-RTS~	-->	RTS	comm
	 *	in	A-CTS~	-->	SI	comm
	 *	in	A-DCD~	-->	RI	comm
	 *	in	A-SYNCH~-->	DSR	comm
	 *	out	B-TxD	-->	TxD	comm
	 *	in	B-RxD	-->	RxD	comm
	 *	in	B-RxC	-->	TRxCB	comm
	 *	in	B-TxC	-->	RTxCB	comm
	 *	out	B-RTS~	-->	SS	comm
	 *	in	B-CTS~	-->	CTS	comm
	 *	in	B-DCD~	-->	CD	comm
	 */
	SCC_INIT_REG(regs, SCC_CHANNEL_A);
	SCC_INIT_REG(regs, SCC_CHANNEL_B);

	SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_WR9, ZSWR9_HARD_RESET);
	DELAY(50000);	/*enough ? */
	SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_WR9, 0);

	/* program the interrupt vector */
	SCC_WRITE_REG(regs, SCC_CHANNEL_A, ZSWR_IVEC, 0xf0);
	SCC_WRITE_REG(regs, SCC_CHANNEL_B, ZSWR_IVEC, 0xf0);
	SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_WR9, ZSWR9_VECTOR_INCL_STAT);

	/* receive parameters and control */
	sc->scc_wreg[SCC_CHANNEL_A].wr3 = 0;
	sc->scc_wreg[SCC_CHANNEL_B].wr3 = 0;

	/* timing base defaults */
	sc->scc_wreg[SCC_CHANNEL_A].wr4 = ZSWR4_CLK_X16;
	sc->scc_wreg[SCC_CHANNEL_B].wr4 = ZSWR4_CLK_X16;

	/* enable DTR, RTS and SS */
	sc->scc_wreg[SCC_CHANNEL_B].wr5 = ZSWR5_RTS;
	sc->scc_wreg[SCC_CHANNEL_A].wr5 = ZSWR5_RTS | ZSWR5_DTR;

	/* baud rates */
	val = ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK;
	sc->scc_wreg[SCC_CHANNEL_B].wr14 = val;
	sc->scc_wreg[SCC_CHANNEL_A].wr14 = val;

	/* interrupt conditions */
	val =	ZSWR1_RIE | ZSWR1_PE_SC | ZSWR1_SIE | ZSWR1_TIE;
	sc->scc_wreg[SCC_CHANNEL_A].wr1 = val;
	sc->scc_wreg[SCC_CHANNEL_B].wr1 = val;
}

int
sccopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct scc_softc *sc;
	struct tty *tp;
	int unit, line;
	int s, error = 0;

	unit = SCCUNIT(dev);
	if (unit >= scc_cd.cd_ndevs)
		return (ENXIO);
	sc = scc_cd.cd_devs[unit];
	if (!sc)
		return (ENXIO);

	line = SCCLINE(dev);
	if (sc->scc_pdma[line].p_addr == NULL)
		return (ENXIO);
	tp = sc->scc_tty[line];
	if (tp == NULL) {
		tp = sc->scc_tty[line] = ttymalloc();
		tty_attach(tp);
	}
	tp->t_oproc = sccstart;
	tp->t_param = sccparam;
	tp->t_dev = dev;

	/*
	 * Do the following only for first opens.
	 */
	s = spltty();
	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		ttychars(tp);
#ifndef PORTSELECTOR
		if (tp->t_ispeed == 0) {
#endif
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = LFLAG;
			tp->t_ispeed = tp->t_ospeed = ISPEED;
#ifdef PORTSELECTOR
			tp->t_cflag |= HUPCL;
#else
		}
#endif
		(void) sccparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	else if ((tp->t_state & TS_XCLUDE) && curproc->p_ucred->cr_uid != 0) {
		error = EBUSY;
		splx(s);
		goto bad;
	}
	(void) sccmctl(dev, DML_DTR, DMSET);

	while (!(flag & O_NONBLOCK) && !(tp->t_cflag & CLOCAL) &&
	    !(tp->t_state & TS_CARR_ON)) {
		tp->t_wopen++;
		error = ttysleep(tp, &tp->t_rawq, TTIPRI | PCATCH, ttopen, 0);
		tp->t_wopen--;
		if (error != 0)
			break;
	}
	splx(s);

	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);

	if (error)
		goto bad;
bad:
	return (error);
}

/*ARGSUSED*/
int
sccclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct scc_softc *sc = scc_cd.cd_devs[SCCUNIT(dev)];
	struct tty *tp;
	int line;

	line = SCCLINE(dev);
	tp = sc->scc_tty[line];
	if (sc->scc_wreg[line].wr5 & ZSWR5_BREAK) {
		sc->scc_wreg[line].wr5 &= ~ZSWR5_BREAK;
		ttyoutput(0, tp);
	}
	(*tp->t_linesw->l_close)(tp, flag);
	if ((tp->t_cflag & HUPCL) || tp->t_wopen ||
	    !(tp->t_state & TS_ISOPEN))
		(void) sccmctl(dev, 0, DMSET);
	return (ttyclose(tp));
}

int
sccread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct scc_softc *sc;
	struct tty *tp;

	sc = scc_cd.cd_devs[SCCUNIT(dev)];		/* XXX*/
	tp = sc->scc_tty[SCCLINE(dev)];
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
sccwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct scc_softc *sc;
	struct tty *tp;

	sc = scc_cd.cd_devs[SCCUNIT(dev)];	/* XXX*/
	tp = sc->scc_tty[SCCLINE(dev)];
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
sccpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct scc_softc *sc;
	struct tty *tp;

	sc = scc_cd.cd_devs[SCCUNIT(dev)];	/* XXX*/
	tp = sc->scc_tty[SCCLINE(dev)];
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
scctty(dev)
	dev_t dev;
{
	struct scc_softc *sc;
	struct tty *tp;
	int unit = SCCUNIT(dev);

	if ((unit >= scc_cd.cd_ndevs) || (sc = scc_cd.cd_devs[unit]) == 0)
		return (0);
	tp = sc->scc_tty[SCCLINE(dev)];
	return (tp);
}

/*ARGSUSED*/
int
sccioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct scc_softc *sc;
	struct tty *tp;
	int error, line;

	line = SCCLINE(dev);
	sc = scc_cd.cd_devs[SCCUNIT(dev)];
	tp = sc->scc_tty[line];

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {

	case TIOCSBRK:
		sc->scc_wreg[line].wr5 |= ZSWR5_BREAK;
		ttyoutput(0, tp);
		break;

	case TIOCCBRK:
		sc->scc_wreg[line].wr5 &= ~ZSWR5_BREAK;
		ttyoutput(0, tp);
		break;

	case TIOCSDTR:
		(void) sccmctl(dev, DML_DTR|DML_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) sccmctl(dev, DML_DTR|DML_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) sccmctl(dev, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void) sccmctl(dev, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void) sccmctl(dev, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = sccmctl(dev, 0, DMGET);
		break;

	default:
		return (EPASSTHROUGH);
	}
	return (0);
}



/*
 * Set line parameters --  tty t_param entry point.
 */
static int
sccparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	struct scc_softc *sc;

	/* Extract the softc and call cold_sccparam to do all the work. */
	sc = scc_cd.cd_devs[SCCUNIT(tp->t_dev)];
	return cold_sccparam(tp, t, sc);
}


/*
 * Do what sccparam() (t_param entry point) does, but callable when cold.
 */
static int
cold_sccparam(tp, t, sc)
	struct tty *tp;
	struct termios *t;
	struct scc_softc *sc;
{
	scc_regmap_t *regs;
	int line;
	u_char value, wvalue;
	int cflag = t->c_cflag;
	int ospeed;

	/* Check arguments */
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);
	ospeed = ttspeedtab(t->c_ospeed, sccspeedtab);
	if (ospeed < 0)
		return (EINVAL);
	/* and copy to tty */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = cflag;

	if (t->c_ospeed == 0) {
		(void) sccmctl(tp->t_dev, 0, DMSET);	/* hang up line */
		return (0);
	}

	line = SCCLINE(tp->t_dev);
	regs = (scc_regmap_t *)sc->scc_pdma[line].p_addr;

	/*
	 * pmax driver used to reset the SCC here. That reset causes the
	 * other channel on the SCC to drop output chars: at least that's
	 * what CGD reports for the Alpha.  It's a bug.
	 */
#if 0
	/* reset line */
	if (line == SCC_CHANNEL_A)
		value = ZSWR9_A_RESET;
	else
		value = ZSWR9_B_RESET;
	SCC_WRITE_REG(regs, line, SCC_WR9, value);
	DELAY(25);
#endif

	/* stop bits, normally 1 */
	value = sc->scc_wreg[line].wr4 & 0xf0;
	if (cflag & CSTOPB)
		value |= ZSWR4_TWOSB;
	else
		value |= ZSWR4_ONESB;
	if ((cflag & PARODD) == 0)
		value |= ZSWR4_EVENP;
	if (cflag & PARENB)
		value |= ZSWR4_PARENB;

	/* set it now, remember it must be first after reset */
	sc->scc_wreg[line].wr4 = value;
	SCC_WRITE_REG(regs, line, SCC_WR4, value);

	/* vector again */
	SCC_WRITE_REG(regs, line, ZSWR_IVEC, 0xf0);

	/* clear break, keep rts dtr */
	wvalue = sc->scc_wreg[line].wr5 & (ZSWR5_DTR|ZSWR5_RTS);
	switch (cflag & CSIZE) {
	case CS5:
		value = ZSWR3_RX_5;
		wvalue |= ZSWR5_TX_5;
		break;
	case CS6:
		value = ZSWR3_RX_6;
		wvalue |= ZSWR5_TX_6;
		break;
	case CS7:
		value = ZSWR3_RX_7;
		wvalue |= ZSWR5_TX_7;
		break;
	case CS8:
	default:
		value = ZSWR3_RX_8;
		wvalue |= ZSWR5_TX_8;
	};
	sc->scc_wreg[line].wr3 = value;
	SCC_WRITE_REG(regs, line, SCC_WR3, value);

	sc->scc_wreg[line].wr5 = wvalue;
	SCC_WRITE_REG(regs, line, SCC_WR5, wvalue);

	/*
	 * XXX Does the SCC chip require us to refresh the WR5 register
	 * for the other channel after writing the other, or not?
	 */
#ifdef notdef
	/* XXX */
	{
	int otherline = (line + 1) & 1;
	SCC_WRITE_REG(regs, otherline, SCC_WR5, sc->scc_wreg[otherline].wr5);
	}
#endif

	SCC_WRITE_REG(regs, line, ZSWR_SYNCLO, 0);
	SCC_WRITE_REG(regs, line, ZSWR_SYNCHI, 0);
	SCC_WRITE_REG(regs, line, SCC_WR9, ZSWR9_VECTOR_INCL_STAT);
	SCC_WRITE_REG(regs, line, SCC_WR10, 0);
	value = ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD |
		ZSWR11_TRXC_OUT_ENA | ZSWR11_TRXC_BAUD;
	SCC_WRITE_REG(regs, line, SCC_WR11, value);
	SCC_SET_TIMING_BASE(regs, line, ospeed);
	value = sc->scc_wreg[line].wr14;
	SCC_WRITE_REG(regs, line, SCC_WR14, value);

	value = ZSWR15_BREAK_IE | ZSWR15_CTS_IE | ZSWR15_DCD_IE;
	SCC_WRITE_REG(regs, line, SCC_WR15, value);

	/* and now the enables */
	value = sc->scc_wreg[line].wr3 | ZSWR3_RX_ENABLE;
	SCC_WRITE_REG(regs, line, SCC_WR3, value);
	value = sc->scc_wreg[line].wr5 | ZSWR5_TX_ENABLE;
	sc->scc_wreg[line].wr5 = value;
	SCC_WRITE_REG(regs, line, SCC_WR5, value);

	/* master inter enable */
	value = ZSWR9_MASTER_IE | ZSWR9_VECTOR_INCL_STAT;
	SCC_WRITE_REG(regs, line, SCC_WR9, value);
	SCC_WRITE_REG(regs, line, SCC_WR1, sc->scc_wreg[line].wr1);
	tc_mb();

	return (0);
}


/*
 * transmission done interrupts
 */
static void
scc_txintr(sc, chan, regs)
	struct scc_softc *sc;
	int chan;
	scc_regmap_t *regs;
{
	struct tty *tp = sc->scc_tty[chan];
	struct pdma *dp = &sc->scc_pdma[chan];
	int cc;

	tp = sc->scc_tty[chan];
	dp = &sc->scc_pdma[chan];
	if (dp->p_mem < dp->p_end) {
		SCC_WRITE_DATA(regs, chan, *dp->p_mem++);
		/* Alpha handles the 1.6 msec settle time in hardware */
		DELAY(2);
		tc_mb();
	} else {
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else {
			ndflush(&tp->t_outq, dp->p_mem -
				(caddr_t) tp->t_outq.c_cf);
			dp->p_end = dp->p_mem = tp->t_outq.c_cf;
		}
		(*tp->t_linesw->l_start)(tp);
		if (tp->t_outq.c_cc == 0 || !(tp->t_state & TS_BUSY)) {
			SCC_READ_REG(regs, chan, SCC_RR15, cc);
			cc &= ~ZSWR15_TXUEOM_IE;
			SCC_WRITE_REG(regs, chan, SCC_WR15, cc);
			cc = sc->scc_wreg[chan].wr1 & ~ZSWR1_TIE;
			SCC_WRITE_REG(regs, chan, SCC_WR1, cc);
			sc->scc_wreg[chan].wr1 = cc;
			tc_mb();
		}
	}
}

/*
 * receive interrupts
 */
static void
scc_rxintr(sc, chan, regs, unit)
	struct scc_softc *sc;
	int chan;
	scc_regmap_t *regs;
	int unit;
{
	struct tty *tp = sc->scc_tty[chan];
	int cc, rr1 = 0, rr2 = 0;	/* XXX */
#if NRASTERCONSOLE > 0
	char *cp;
	int cl;
#endif

	SCC_READ_DATA(regs, chan, cc);
	if (rr2 == SCC_RR2_A_RECV_SPECIAL ||
		rr2 == SCC_RR2_B_RECV_SPECIAL) {
		SCC_READ_REG(regs, chan, SCC_RR1, rr1);
		SCC_WRITE_REG(regs, chan, SCC_RR0, ZSWR0_RESET_ERRORS);
		if (rr1 & ZSRR1_DO) {
			log(LOG_WARNING, "scc%d,%d: silo overflow\n",
				unit >> 1, chan);
		}
	}

	/*
	 * Keyboard needs special treatment.
	 */
	if (tp == scctty(makedev(cdevsw_lookup_major(&scc_cdevsw),
				 SCCKBD_PORT))) {
#if defined(DDB) && defined(LK_DO)
			if (cc == LK_DO) {
				spl0();
				console_debugger();
				return;
			}
#endif
#ifdef DEBUG
		debugChar = cc;
#endif
		if (sccDivertXInput) {
			(*sccDivertXInput)(cc);
			return;
		}
#if NRASTERCONSOLE > 0
		if ((cp = lk_mapchar(cc, &cl)) == NULL)
			return;

		while (cl--)
			rcons_input(0, *cp++);
#endif
	/*
	 * Now for mousey
	 */
	} else if (tp == scctty(makedev(cdevsw_lookup_major(&scc_cdevsw),
				SCCMOUSE_PORT)) &&
	    sccMouseButtons) {
#if NRASTERCONSOLE > 0
		/*XXX*/
		mouseInput(cc);
#endif
		return;
	}
	if (!(tp->t_state & TS_ISOPEN)) {
		wakeup(&tp->t_rawq);
#ifdef PORTSELECTOR
		if (!(tp->t_state & TS_WOPEN))
#endif
			return;
	}
	if (rr2 == SCC_RR2_A_RECV_SPECIAL ||
		rr2 == SCC_RR2_B_RECV_SPECIAL) {
		if (rr1 & ZSRR1_PE)
			cc |= TTY_PE;
		if (rr1 & ZSRR1_FE)
			cc |= TTY_FE;
	}
	(*tp->t_linesw->l_rint)(cc, tp);
}

/*
 * Modem status interrupts
 */
static void
scc_stintr(sc, chan, regs, unit)
	struct scc_softc *sc;
	int chan;
	scc_regmap_t *regs;
	int unit;
{

	SCC_WRITE_REG(regs, chan, SCC_RR0, ZSWR0_RESET_STATUS);
	scc_modem_intr(unit | chan);
}


/*
 * Check for interrupts from all devices.
 */
static int
sccintr(xxxsc)
	void *xxxsc;
{
	struct scc_softc *sc = (struct scc_softc *)xxxsc;
	int unit = (long)sc->sc_dv.dv_unit;
	scc_regmap_t *regs;
	int rr3;

	regs = (scc_regmap_t *)sc->scc_pdma[0].p_addr;
	unit <<= 1;

	/* Note: only channel A has an RR3 */
	SCC_READ_REG(regs, SCC_CHANNEL_A, ZSRR_IPEND, rr3);

	/*
	 * Clear interrupt first to avoid a race condition.
	 * If a new interrupt condition happens while we are
	 * servicing this one, we will get another interrupt
	 * shortly.  We can NOT just sit here in a loop, or
	 * we will cause horrible latency for other devices
	 * on this interrupt level (i.e. sun3x floppy disk).
	 */

	/*
	 * At least some, maybe all DECstation chips have modem
	 * leads crosswired: the data comes in one channel and the
	 * bulkead modem signals for that port are wired to the
	 * _other_ channel of the chip. Yes, really.
	 * This code cannot get that right.
	 */

	/* First look at channel A. */
	if (rr3 & (ZSRR3_IP_A_RX | ZSRR3_IP_A_TX | ZSRR3_IP_A_STAT)) {
		SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_RR0, ZSWR0_CLR_INTR);
		if (rr3 & ZSRR3_IP_A_RX)
			scc_rxintr(sc, SCC_CHANNEL_A, regs, unit);

		if (rr3 & ZSRR3_IP_A_STAT) {
			/* XXX swapped channels */
			scc_stintr(sc, SCC_CHANNEL_A, regs, unit);
		}

		if (rr3 & ZSRR3_IP_A_TX)
			scc_txintr(sc, SCC_CHANNEL_A, regs);
	}

	/* Now look at channel B. */
	if (rr3 & (ZSRR3_IP_B_RX | ZSRR3_IP_B_TX | ZSRR3_IP_B_STAT)) {
		SCC_WRITE_REG(regs, SCC_CHANNEL_B, SCC_RR0, ZSWR0_CLR_INTR);
		if (rr3 & ZSRR3_IP_B_RX)
			scc_rxintr(sc, SCC_CHANNEL_B, regs, unit);
		if (rr3 & ZSRR3_IP_B_STAT) {
			/* XXX swapped channels */
			scc_stintr(sc, SCC_CHANNEL_B, regs, unit);
		}
		if (rr3 & ZSRR3_IP_B_TX)
			scc_txintr(sc, SCC_CHANNEL_B, regs);
	}

	return 1;
}

static void
sccstart(tp)
	struct tty *tp;
{
	struct pdma *dp;
	scc_regmap_t *regs;
	struct scc_softc *sc;
	int cc, chan;
	u_char temp;
	int s, sendone;

	sc = scc_cd.cd_devs[SCCUNIT(tp->t_dev)];
	dp = &sc->scc_pdma[SCCLINE(tp->t_dev)];
	regs = (scc_regmap_t *)dp->p_addr;
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;

	cc = ndqb(&tp->t_outq, 0);

	tp->t_state |= TS_BUSY;
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;

	/*
	 * Enable transmission and send the first char, as required.
	 */
	chan = SCCLINE(tp->t_dev);
	SCC_READ_REG(regs, chan, SCC_RR0, temp);
	sendone = (temp & ZSRR0_TX_READY);
	SCC_READ_REG(regs, chan, SCC_RR15, temp);
	temp |= ZSWR15_TXUEOM_IE;
	SCC_WRITE_REG(regs, chan, SCC_WR15, temp);
	temp = sc->scc_wreg[chan].wr1 | ZSWR1_TIE;
	SCC_WRITE_REG(regs, chan, SCC_WR1, temp);
	sc->scc_wreg[chan].wr1 = temp;
	if (sendone) {
#ifdef DIAGNOSTIC
		if (cc == 0)
			panic("sccstart: No chars");
#endif
		SCC_WRITE_DATA(regs, chan, *dp->p_mem++);
		/* Alpha handles the 1.6 msec settle time in hardware */
		DELAY(2);
	}
	tc_mb();
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
void
sccstop(tp, flag)
	struct tty *tp;
	int flag;
{
	struct pdma *dp;
	struct scc_softc *sc;
	int s;

	sc = scc_cd.cd_devs[SCCUNIT(tp->t_dev)];
	dp = &sc->scc_pdma[SCCLINE(tp->t_dev)];
	s = spltty();
	if (tp->t_state & TS_BUSY) {
		dp->p_end = dp->p_mem;
		if (!(tp->t_state & TS_TTSTOP))
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

static int
sccmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	struct scc_softc *sc;
	scc_regmap_t *regs;
	int line, mbits;
	u_char value;
	int s;

	sc = scc_cd.cd_devs[SCCUNIT(dev)];
	line = SCCLINE(dev);
	regs = (scc_regmap_t *)sc->scc_pdma[line].p_addr;
	s = spltty();
	/*
	 * only channel B has modem control, however the DTR and RTS
	 * pins on the comm port are wired to the DTR and RTS A channel
	 * signals.
	 */
	mbits = DML_DTR | DML_DSR | DML_CAR;
	if (line == SCC_CHANNEL_B) {
		if (sc->scc_wreg[SCC_CHANNEL_A].wr5 & ZSWR5_DTR)
			mbits = DML_DTR | DML_DSR;
		else
			mbits = 0;
		SCC_READ_REG_ZERO(regs, SCC_CHANNEL_B, value);
		if (value & ZSRR0_DCD)
			mbits |= DML_CAR;
	}
	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return (mbits);
	}
	if (line == SCC_CHANNEL_B) {
		if (mbits & DML_DTR)
			sc->scc_wreg[SCC_CHANNEL_A].wr5 |= ZSWR5_DTR;
		else
			sc->scc_wreg[SCC_CHANNEL_A].wr5 &= ~ZSWR5_DTR;
		SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_WR5,
			sc->scc_wreg[SCC_CHANNEL_A].wr5);
	}
	if ((mbits & DML_DTR) || (sc->scc_softCAR & (1 << line)))
		sc->scc_tty[line]->t_state |= TS_CARR_ON;
	(void) splx(s);
	return (mbits);
}

/*
 * Check for carrier transition.
 */
static void
scc_modem_intr(dev)
	dev_t dev;
{
	scc_regmap_t *regs;
	struct scc_softc *sc;
	struct tty *tp;
	int car, chan;
	u_char value;
	int s;

	chan = SCCLINE(dev);
	sc = scc_cd.cd_devs[SCCUNIT(dev)];
	tp = sc->scc_tty[chan];
	regs = (scc_regmap_t *)sc->scc_pdma[chan].p_addr;

	if (chan == SCC_CHANNEL_A) {
		return;
	}

	s = spltty();
	SCC_READ_REG_ZERO(regs, chan, value);

	if (sc->scc_softCAR & (1 << chan))
		car = 1;
	else {
		car = value & ZSRR0_DCD;
	}

	/* Break on serial console drops into the debugger */
	if ((value & ZSRR0_BREAK) && CONSOLE_ON_UNIT(sc->sc_dv.dv_unit)) {
#ifdef DDB
		splx(s);		/* spl0()? */
		console_debugger();
		return;
#else
		/* XXX maybe fall back to PROM? */
#endif
	}

	/*
	 * The pmax driver follows carrier-detect. The Alpha does not.
	 * On pmax, ignore hups on a console tty.
	 * On alpha, a no-op, for historical reasons.
	 */
	if (!CONSOLE_ON_UNIT(sc->sc_dv.dv_unit)) {
		if (car) {
			/* carrier present */
			if (!(tp->t_state & TS_CARR_ON))
				(void)(*tp->t_linesw->l_modem)(tp, 1);
		} else if (tp->t_state & TS_CARR_ON)
		  (void)(*tp->t_linesw->l_modem)(tp, 0);
	}
	splx(s);
}

/*
 * Get a char off the appropriate line via. a busy wait loop.
 */
int
sccGetc(dev)
	dev_t dev;
{
	scc_regmap_t *regs;
	int c, line;
	u_char value;
	int s;

	line = SCCLINE(dev);
	if (cold && scc_cons_addr) {
		regs = scc_cons_addr;
	} else {
		struct scc_softc *sc;
		sc = scc_cd.cd_devs[SCCUNIT(dev)];
		regs = (scc_regmap_t *)sc->scc_pdma[line].p_addr;
	}

	if (!regs)
		return (0);
	/*s = spltty(); */	/* XXX  why different spls? */
	s = splhigh();
	for (;;) {
		SCC_READ_REG(regs, line, SCC_RR0, value);
		if (value & ZSRR0_RX_READY) {
			SCC_READ_REG(regs, line, SCC_RR1, value);
			SCC_READ_DATA(regs, line, c);
			if (value & (ZSRR1_PE | ZSRR1_DO | ZSRR1_FE)) {
				SCC_WRITE_REG(regs, line, SCC_WR0,
				    ZSWR0_RESET_ERRORS);
				SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_WR0,
				    ZSWR0_CLR_INTR);
			} else {
				SCC_WRITE_REG(regs, SCC_CHANNEL_A, SCC_WR0,
				    ZSWR0_CLR_INTR);
				splx(s);
				return (c & 0xff);
			}
		} else
			DELAY(10);
	}
}

/*
 * Send a char on a port, via a busy wait loop.
 */
void
sccPutc(dev, c)
	dev_t dev;
	int c;
{
	scc_regmap_t *regs;
	int line;
	u_char value;
	int s;

	s = spltty();	/* XXX  why different spls? */
	line = SCCLINE(dev);
	if (cold && scc_cons_addr) {
		regs = scc_cons_addr;
	} else {
		struct scc_softc *sc;
		sc = scc_cd.cd_devs[SCCUNIT(dev)];
		regs = (scc_regmap_t *)sc->scc_pdma[line].p_addr;
	}

	/*
	 * Wait for transmitter to be not busy.
	 */
	do {
		SCC_READ_REG(regs, line, SCC_RR0, value);
		if (value & ZSRR0_TX_READY)
			break;
		DELAY(100);
	} while (1);

	/*
	 * Send the char.
	 */
	SCC_WRITE_DATA(regs, line, c);
	tc_mb();
	splx(s);

	return;
}

/*
 * Enable/disable polling mode
 */
static void
sccPollc(dev, on)
	dev_t dev;
	int on;
{
}

#ifdef	SCC_DEBUG
static void
rr(msg, regs)
	char *msg;
	scc_regmap_t *regs;
{
	u_char value;
	int r0, r1, r2, r3, r10, r15;

	printf("%s: register: %lx\n", msg, regs);
#define	L(reg, r) {							\
	SCC_READ_REG(regs, SCC_CHANNEL_A, reg, value);			\
	r = value;							\
}
	L(SCC_RR0, r0);
	L(SCC_RR1, r1);
	L(ZSRR_IVEC, r2);
	L(ZSRR_IPEND, r3);
	L(SCC_RR10, r10);
	L(SCC_RR15, r15);
	printf("A: 0: %x  1: %x    2(vec): %x  3: %x  10: %x  15: %x\n",
	    r0, r1, r2, r3, r10, r15);
#undef L
#define	L(reg, r) {							\
	SCC_READ_REG(regs, SCC_CHANNEL_B, reg, value);			\
	r = value;							\
}
	L(SCC_RR0, r0);
	L(SCC_RR1, r1);
	L(ZSRR_IVEC, r2);
	L(SCC_RR10, r10);
	L(SCC_RR15, r15);
	printf("B: 0: %x  1: %x  2(state): %x        10: %x  15: %x\n",
	    r0, r1, r2, r10, r15);
}
#endif /* SCC_DEBUG */
