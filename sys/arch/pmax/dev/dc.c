/*	$NetBSD: dc.c,v 1.22 1996/09/11 06:41:19 jonathan Exp $	*/

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
 *	@(#)dc.c	8.5 (Berkeley) 6/2/95
 */

/*
 * devDC7085.c --
 *
 *     	This file contains machine-dependent routines that handle the
 *	output queue for the serial lines.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/dev/ds3100.md/RCS/devDC7085.c,
 *	v 1.4 89/08/29 11:55:30 nelson Exp  SPRITE (DECWRL)";
 */

#include "dc.h"
#if NDC > 0
/*
 * DC7085 (DZ-11 look alike) Driver
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <machine/conf.h>
#include <sys/device.h>
#include <machine/autoconf.h>
#include <machine/machConst.h>
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>

#include <machine/dc7085cons.h>
#include <machine/pmioctl.h>

#include <pmax/pmax/pmaxtype.h>
#include <pmax/pmax/cons.h>

#include <pmax/dev/pdma.h>
#include <pmax/dev/lk201.h>

#include "dcvar.h"
#include "tc.h"

#include <pmax/dev/lk201var.h>		/* XXX KbdReset band friends */

extern int pmax_boardtype;
extern struct cfdriver mainbus_cd;

struct dc_softc {
	struct device sc_dv;
	struct pdma dc_pdma[4];
};

/*
 * Autoconfiguration data for config.
 * 
 * Use the statically-allocated softc until old autoconfig code and
 * config.old are completely gone.
 */
int	dcmatch  __P((struct device * parent, void *cfdata, void *aux));
void	dcattach __P((struct device *parent, struct device *self, void *aux));

int	dc_doprobe __P((void *addr, int unit, int flags, int pri));
int	dcintr __P((void * xxxunit));

extern struct cfdriver dc_cd;

struct cfattach dc_ca = {
	sizeof(struct dc_softc), dcmatch, dcattach
};

struct  cfdriver dc_cd = {
	NULL, "dc", DV_TTY
};


#define	NDCLINE 	(NDC*4)

void dcstart	__P((struct tty *));
void dcxint	__P((struct tty *));
void dcPutc	__P((dev_t, int));
void dcscan	__P((void *));
extern void ttrstrt __P((void *));
int dcGetc	__P((dev_t));
int dcparam	__P((struct tty *, struct termios *));

struct	tty *dc_tty[NDCLINE];
int	dc_cnt = NDCLINE;
void	(*dcDivertXInput)();	/* X windows keyboard input routine */
void	(*dcMouseEvent)();	/* X windows mouse motion event routine */
void	(*dcMouseButtons)();	/* X windows mouse buttons event routine */
#ifdef DEBUG
int	debugChar;
#endif

/*
 * Software copy of brk register since it isn't readable
 */
int	dc_brk[NDC];
char	dcsoftCAR[NDC];		/* mask of dc's with carrier on (DSR) */

/*
 * The DC7085 doesn't interrupt on carrier transitions, so
 * we have to use a timer to watch it.
 */
int	dc_timer;		/* true if timer started */

/*
 * Pdma structures for fast output code
 */
struct	pdma dcpdma[NDCLINE];

struct speedtab dcspeedtab[] = {
	{ 0,	0,	},
	{ 50,	LPR_B50    },
	{ 75,	LPR_B75    },
	{ 110,	LPR_B110   },
	{ 134,	LPR_B134   },
	{ 150,	LPR_B150   },
	{ 300,	LPR_B300   },
	{ 600,	LPR_B600   },
	{ 1200,	LPR_B1200  },
	{ 1800,	LPR_B1800  },
	{ 2400,	LPR_B2400  },
	{ 4800,	LPR_B4800  },
	{ 9600,	LPR_B9600  },
	{ 19200,LPR_B19200 },
	{ -1,	-1 }
};

#ifndef	PORTSELECTOR
#define	ISPEED	TTYDEF_SPEED
#define	LFLAG	TTYDEF_LFLAG
#else
#define	ISPEED	B4800
#define	LFLAG	(TTYDEF_LFLAG & ~ECHO)
#endif

/*
 * Forward declarations
 */
struct tty *dctty __P((dev_t  dev));
void dcrint __P((int));
int dcmctl __P((dev_t dev, int bits, int how));



/*
 * Match driver based on name
 */
int
dcmatch(parent, match, aux)
	struct device *parent;
	void *match;
	void *aux;
{
	struct confargs *ca = aux;
#if NTC>0
	struct ioasicdev_attach_args *d = aux;
#endif

	static int nunits = 0;

#if NTC > 0
	if (parent->dv_cfdata->cf_driver == &ioasic_cd) {
		if (strcmp(d->iada_modname, "dc") != 0 &&
		    strcmp(d->iada_modname, "dc7085") != 0)
			return (0);
	}
	else
#endif /* NTC */

	if (parent->dv_cfdata->cf_driver == &mainbus_cd) {
		if (strcmp(ca->ca_name, "dc") != 0 &&
		    strcmp(ca->ca_name, "mdc") != 0 &&
		    strcmp(ca->ca_name, "dc7085") != 0)
			return (0);
	}
	else
		return (0);

	/*
	 * Use statically-allocated softc and attach code until
	 * old config is completely gone.  Don't  over-run softc.
	 */
	if (nunits > NDC) {
		printf("dc: too many units for old config\n");
		return (0);
	}
	nunits++;
	return (1);
}

void
dcattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	register struct confargs *ca = aux;
#if NTC > 0
	struct ioasicdev_attach_args *d = aux;
#endif /* NTC */
	caddr_t dcaddr;


#if NTC > 0
	if (parent->dv_cfdata->cf_driver == &ioasic_cd) {
		dcaddr = (caddr_t)d->iada_addr;
		(void) dc_doprobe((void*)MACH_PHYS_TO_UNCACHED(dcaddr),
				  self->dv_unit, self->dv_cfdata->cf_flags,
				  (int)d->iada_cookie);
		/* tie pseudo-slot to device */
		ioasic_intr_establish(parent, d->iada_cookie, TC_IPL_TTY,
		    dcintr, self);
	}
	else
#endif /* NTC */
	if (parent->dv_cfdata->cf_driver == &mainbus_cd) {
		dcaddr = (caddr_t)ca->ca_addr;
		(void) dc_doprobe((void*)MACH_PHYS_TO_UNCACHED(dcaddr),
				  self->dv_unit, self->dv_cfdata->cf_flags,
				  ca->ca_slot);

		/* tie pseudo-slot to device */
		BUS_INTR_ESTABLISH(ca, dcintr, self);
	}
	printf("\n");
}

/*
 * Is there a framebuffer console device using this serial driver?
 * XXX used for ugly special-cased console input that should be redone
 * more cleanly.
 */
static inline int raster_console __P((void));

static inline int
raster_console()
{
	return (cn_tab->cn_pri == CN_INTERNAL ||
		cn_tab->cn_pri == CN_NORMAL);
}


/*
 * DC7085 (dz-11) probe routine from old-style config.
 * This is only here out of intertia.
 */
int
dc_doprobe(addr, unit, flags, priority)
	void *addr;
	int unit, flags, priority;
{
	register dcregs *dcaddr;
	register struct pdma *pdp;
	register struct tty *tp;
	register int cntr;
	int s;

	if (unit >= NDC)
		return (0);
	if (badaddr(addr, 2))
		return (0);

	/*
	 * For a remote console, wait a while for previous output to
	 * complete.
	 */
	if (major(cn_tab->cn_dev) == DCDEV && unit == 0 &&
		cn_tab->cn_pri == CN_REMOTE)
		DELAY(10000);

	/* reset chip */
	dcaddr = (dcregs *)addr;
	dcaddr->dc_csr = CSR_CLR;
	wbflush();
	while (dcaddr->dc_csr & CSR_CLR)
		;
	dcaddr->dc_csr = CSR_MSE | CSR_TIE | CSR_RIE;

	/* init pseudo DMA structures */
	pdp = &dcpdma[unit * 4];
	for (cntr = 0; cntr < 4; cntr++) {
		pdp->p_addr = (void *)dcaddr;
		tp = dc_tty[unit * 4 + cntr] = ttymalloc();
		if (cntr != DCKBD_PORT && cntr != DCMOUSE_PORT)
			tty_attach(tp);
		pdp->p_arg = (int) tp;
		pdp->p_fcn = dcxint;
		pdp++;
	}
	dcsoftCAR[unit] = flags | 0xB;

	if (dc_timer == 0) {
		dc_timer = 1;
		timeout(dcscan, (void *)0, hz);
	}

	/*
	 * Special handling for consoles.
	 */
	if (unit == 0) {
		if (cn_tab->cn_pri == CN_INTERNAL ||
		    cn_tab->cn_pri == CN_NORMAL) {
			s = spltty();
			dcaddr->dc_lpr = LPR_RXENAB | LPR_8_BIT_CHAR |
				LPR_B4800 | DCKBD_PORT;
			wbflush();
			dcaddr->dc_lpr = LPR_RXENAB | LPR_B4800 | LPR_OPAR |
				LPR_PARENB | LPR_8_BIT_CHAR | DCMOUSE_PORT;
			wbflush();
			DELAY(1000);
			KBDReset(makedev(DCDEV, DCKBD_PORT), dcPutc);
			MouseInit(makedev(DCDEV, DCMOUSE_PORT), dcPutc, dcGetc);
			splx(s);
		} else if (major(cn_tab->cn_dev) == DCDEV) {
			s = spltty();
			dcaddr->dc_lpr = LPR_RXENAB | LPR_8_BIT_CHAR |
				LPR_B9600 | minor(cn_tab->cn_dev);
			wbflush();
			DELAY(1000);
			/*cn_tab.cn_disabled = 0;*/ /* FIXME */
			splx(s);
		}
	}

	return (1);
}

int
dcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp;
	register int unit;
	int s, error = 0;

	unit = minor(dev);
	if (unit >= dc_cnt || dcpdma[unit].p_addr == (void *)0)
		return (ENXIO);
	tp = dc_tty[unit];
	if (tp == NULL) {
		tp = dc_tty[unit] = ttymalloc();
		tty_attach(tp);
	}
	tp->t_oproc = dcstart;
	tp->t_param = dcparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
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
		(void) dcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if ((tp->t_state & TS_XCLUDE) && curproc->p_ucred->cr_uid != 0)
		return (EBUSY);
	(void) dcmctl(dev, DML_DTR | DML_RTS, DMSET);
	if ((dcsoftCAR[unit >> 2] & (1 << (unit & 03))) ||
	    (dcmctl(dev, 0, DMGET) & DML_CAR))
		tp->t_state |= TS_CARR_ON;
	s = spltty();
	while (!(flag & O_NONBLOCK) && !(tp->t_cflag & CLOCAL) &&
	       !(tp->t_state & TS_CARR_ON)) {
		tp->t_state |= TS_WOPEN;
		if ((error = ttysleep(tp, (caddr_t)&tp->t_rawq,
				      TTIPRI | PCATCH, ttopen, 0)) != 0)
			break;
	}
	splx(s);
	if (error)
		return (error);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
int
dcclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp;
	register int unit, bit;
	int s;

	unit = minor(dev);
	tp = dc_tty[unit];
	bit = 1 << ((unit & 03) + 8);
	s = spltty();
	/* turn off the break bit if it is set */
	if (dc_brk[unit >> 2] & bit) {
		dc_brk[unit >> 2] &= ~bit;
		ttyoutput(0, tp);
	}
	splx(s);
	(*linesw[tp->t_line].l_close)(tp, flag);
	if ((tp->t_cflag & HUPCL) || (tp->t_state & TS_WOPEN) ||
	    !(tp->t_state & TS_ISOPEN))
		(void) dcmctl(dev, 0, DMSET);
	return (ttyclose(tp));
}

int
dcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = dc_tty[minor(dev)];
	if ((tp->t_cflag & CRTS_IFLOW) && (tp->t_state & TS_TBLOCK) &&
	    tp->t_rawq.c_cc < TTYHOG/5) {
		tp->t_state &= ~TS_TBLOCK;
		(void) dcmctl(dev, DML_RTS, DMBIS);
	}
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
dcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = dc_tty[minor(dev)];
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
dctty(dev)
        dev_t dev;
{
        struct tty *tp = dc_tty [minor (dev)];
        return (tp);
}

/*ARGSUSED*/
int
dcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tty *tp;
	register int unit = minor(dev);
	register int dc = unit >> 2;
	int error;

	tp = dc_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {

	case TIOCSBRK:
		dc_brk[dc] |= 1 << ((unit & 03) + 8);
		ttyoutput(0, tp);
		break;

	case TIOCCBRK:
		dc_brk[dc] &= ~(1 << ((unit & 03) + 8));
		ttyoutput(0, tp);
		break;

	case TIOCSDTR:
		(void) dcmctl(dev, DML_DTR|DML_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dcmctl(dev, DML_DTR|DML_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dcmctl(dev, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void) dcmctl(dev, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void) dcmctl(dev, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dcmctl(dev, 0, DMGET);
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

int
dcparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register dcregs *dcaddr;
	register int lpr;
	register int cflag = t->c_cflag;
	int unit = minor(tp->t_dev);
	int ospeed = ttspeedtab(t->c_ospeed, dcspeedtab);
	int s;

	/* check requested parameters */
        if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed) ||
            (cflag & CSIZE) == CS5 || (cflag & CSIZE) == CS6 ||
	    (pmax_boardtype == DS_PMAX && t->c_ospeed == 19200))
                return (EINVAL);
        /* and copy to tty */
        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	/*
	 * Handle console cases specially.
	 */
	if (raster_console()) {
		if (unit == DCKBD_PORT) {
			lpr = LPR_RXENAB | LPR_8_BIT_CHAR |
				LPR_B4800 | DCKBD_PORT;
			goto out;
		} else if (unit == DCMOUSE_PORT) {
			lpr = LPR_RXENAB | LPR_B4800 | LPR_OPAR |
				LPR_PARENB | LPR_8_BIT_CHAR | DCMOUSE_PORT;
			goto out;
		}
	} else if (tp->t_dev == cn_tab->cn_dev) {
		lpr = LPR_RXENAB | LPR_8_BIT_CHAR | LPR_B9600 | unit;
		goto out;
	}
	if (ospeed == 0) {
		(void) dcmctl(unit, 0, DMSET);	/* hang up line */
		return (0);
	}
	lpr = LPR_RXENAB | ospeed | (unit & 03);
	if ((cflag & CSIZE) == CS7)
		lpr |= LPR_7_BIT_CHAR;
	else
		lpr |= LPR_8_BIT_CHAR;
	if (cflag & PARENB)
		lpr |= LPR_PARENB;
	if (cflag & PARODD)
		lpr |= LPR_OPAR;
	if (cflag & CSTOPB)
		lpr |= LPR_2_STOP;
out:
	dcaddr = (dcregs *)dcpdma[unit].p_addr;
	s = spltty();
	dcaddr->dc_lpr = lpr;
	wbflush();
	splx(s);
	DELAY(10);
	return (0);
}

/*
 * Check for interrupts from all devices.
 */
int
dcintr(xxxunit)
	void *xxxunit;
{
	register struct dc_softc *sc = xxxunit;
	register dcregs *dcaddr;
	register unsigned csr;

	register int unit = sc->sc_dv.dv_unit;

	unit <<= 2;
	dcaddr = (dcregs *)dcpdma[unit].p_addr;
	while ((csr = dcaddr->dc_csr) & (CSR_RDONE | CSR_TRDY)) {
		if (csr & CSR_RDONE)
			dcrint(unit);
		if (csr & CSR_TRDY)
			dcxint(dc_tty[unit + ((csr >> 8) & 03)]);
	}
	/* XXX check for spurious interrupts */
	return 0;
}

void
dcrint(unit)
	register int unit;
{
	register dcregs *dcaddr;
	register struct tty *tp;
	register int c, cc;
	int overrun = 0;

	dcaddr = (dcregs *)dcpdma[unit].p_addr;
	while ((c = dcaddr->dc_rbuf) < 0) {	/* char present */
		cc = c & 0xff;
		tp = dc_tty[unit + ((c >> 8) & 03)];
		if ((c & RBUF_OERR) && overrun == 0) {
			log(LOG_WARNING, "dc%d,%d: silo overflow\n", unit >> 2,
				(c >> 8) & 03);
			overrun = 1;
		}
		/* the keyboard requires special translation */
		if (tp == dc_tty[DCKBD_PORT] && raster_console()) {
#ifdef KADB
			if (cc == LK_DO) {
				spl0();
				kdbpanic();
				return;
			}
#endif
#ifdef DEBUG
			debugChar = cc;
#endif
			if (dcDivertXInput) {
				(*dcDivertXInput)(cc);
				return;
			}
			if ((cc = kbdMapChar(cc)) < 0)
				return;
		} else if (tp == dc_tty[DCMOUSE_PORT] && dcMouseButtons) {
			mouseInput(cc);
			return;
		}
		if (!(tp->t_state & TS_ISOPEN)) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if (!(tp->t_state & TS_WOPEN))
#endif
				return;
		}
		if (c & RBUF_FERR)
			cc |= TTY_FE;
		if (c & RBUF_PERR)
			cc |= TTY_PE;
		if ((tp->t_cflag & CRTS_IFLOW) && !(tp->t_state & TS_TBLOCK) &&
		    tp->t_rawq.c_cc + tp->t_canq.c_cc >= TTYHOG) {
			tp->t_state &= ~TS_TBLOCK;
			(void) dcmctl(tp->t_dev, DML_RTS, DMBIC);
		}
		(*linesw[tp->t_line].l_rint)(cc, tp);
	}
	DELAY(10);
}

void
dcxint(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	register dcregs *dcaddr;
	int unit;

	unit = minor(tp->t_dev);
	dp = &dcpdma[unit];
	if (dp->p_mem < dp->p_end) {
		dcaddr = (dcregs *)dp->p_addr;
		/* check for hardware flow control of output */
		if ((tp->t_cflag & CCTS_OFLOW) && pmax_boardtype != DS_PMAX) {
			switch (unit) {
			case DCCOMM_PORT:
				if (dcaddr->dc_msr & MSR_CTS2)
					break;
				goto stop;

			case DCPRINTER_PORT:
				if (dcaddr->dc_msr & MSR_CTS3)
					break;
			stop:
				tp->t_state &= ~TS_BUSY;
				tp->t_state |= TS_TTSTOP;
				ndflush(&tp->t_outq, dp->p_mem - 
						(caddr_t)tp->t_outq.c_cf);
				dp->p_end = dp->p_mem = tp->t_outq.c_cf;
				dcaddr->dc_tcr &= ~(1 << unit);
				wbflush();
				DELAY(10);
				return;
			}
		}
		dcaddr->dc_tdr = dc_brk[unit >> 2] | *(u_char *)dp->p_mem;
		dp->p_mem++;

		wbflush();
		DELAY(10);
		return;
	}
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else {
		ndflush(&tp->t_outq, dp->p_mem - (caddr_t) tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		dcstart(tp);
	if (tp->t_outq.c_cc == 0 || !(tp->t_state & TS_BUSY)) {
		dcaddr = (dcregs *)dp->p_addr;
		dcaddr->dc_tcr &= ~(1 << (unit & 03));
		wbflush();
		DELAY(10);
	}
}

void
dcstart(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	register dcregs *dcaddr;
	register int cc;
	int unit, s;

	dp = &dcpdma[unit = minor(tp->t_dev)];
	dcaddr = (dcregs *)dp->p_addr;
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	/* handle console specially */
	if (tp == dc_tty[DCKBD_PORT] && raster_console()) {
		while (tp->t_outq.c_cc > 0) {
			cc = getc(&tp->t_outq) & 0x7f;
			cnputc(cc);
		}
		/*
		 * After we flush the output queue we may need to wake
		 * up the process that made the output.
		 */
		if (tp->t_outq.c_cc <= tp->t_lowat) {
			if (tp->t_state & TS_ASLEEP) {
				tp->t_state &= ~TS_ASLEEP;
				wakeup((caddr_t)&tp->t_outq);
			}
			selwakeup(&tp->t_wsel);
		}
		goto out;
	}
  	cc = ndqb(&tp->t_outq, 0);
	tp->t_state |= TS_BUSY;
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	dcaddr->dc_tcr |= 1 << (unit & 03);
	wbflush();
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
void
dcstop(tp, flag)
	register struct tty *tp;
{
	register struct pdma *dp;
	register int s;

	dp = &dcpdma[minor(tp->t_dev)];
	s = spltty();
	if (tp->t_state & TS_BUSY) {
		dp->p_end = dp->p_mem;
		if (!(tp->t_state & TS_TTSTOP))
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

int
dcmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register dcregs *dcaddr;
	register int unit, mbits;
	int b, s;
	register int tcr, msr;

	unit = minor(dev);
	b = 1 << (unit & 03);
	dcaddr = (dcregs *)dcpdma[unit].p_addr;
	s = spltty();
	/* only channel 2 has modem control on a DECstation 2100/3100 */
	mbits = DML_DTR | DML_RTS | DML_DSR | DML_CAR;
	switch (unit & 03) {
	case 2:
		mbits = 0;
		tcr = dcaddr->dc_tcr;
		if (tcr & TCR_DTR2)
			mbits |= DML_DTR;
		if (pmax_boardtype != DS_PMAX && (tcr & TCR_RTS2))
			mbits |= DML_RTS;
		msr = dcaddr->dc_msr;
		if (msr & MSR_CD2)
			mbits |= DML_CAR;
		if (msr & MSR_DSR2) {
			if (pmax_boardtype == DS_PMAX)
				mbits |= DML_CAR | DML_DSR;
			else
				mbits |= DML_DSR;
		}
		break;

	case 3:
		if (pmax_boardtype != DS_PMAX) {
			mbits = 0;
			tcr = dcaddr->dc_tcr;
			if (tcr & TCR_DTR3)
				mbits |= DML_DTR;
			if (tcr & TCR_RTS3)
				mbits |= DML_RTS;
			msr = dcaddr->dc_msr;
			if (msr & MSR_CD3)
				mbits |= DML_CAR;
			if (msr & MSR_DSR3)
				mbits |= DML_DSR;
		}
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
	switch (unit & 03) {
	case 2:
		tcr = dcaddr->dc_tcr;
		if (mbits & DML_DTR)
			tcr |= TCR_DTR2;
		else
			tcr &= ~TCR_DTR2;
		if (pmax_boardtype != DS_PMAX) {
			if (mbits & DML_RTS)
				tcr |= TCR_RTS2;
			else
				tcr &= ~TCR_RTS2;
		}
		dcaddr->dc_tcr = tcr;
		break;

	case 3:
		if (pmax_boardtype != DS_PMAX) {
			tcr = dcaddr->dc_tcr;
			if (mbits & DML_DTR)
				tcr |= TCR_DTR3;
			else
				tcr &= ~TCR_DTR3;
			if (mbits & DML_RTS)
				tcr |= TCR_RTS3;
			else
				tcr &= ~TCR_RTS3;
			dcaddr->dc_tcr = tcr;
		}
	}
	(void) splx(s);
	return (mbits);
}

/*
 * This is called by timeout() periodically.
 * Check to see if modem status bits have changed.
 */
void
dcscan(arg)
	void *arg;
{
	register dcregs *dcaddr;
	register struct tty *tp;
	register int unit, limit, dtr, dsr;
	int s;

	/* only channel 2 has modem control on a DECstation 2100/3100 */
	dtr = TCR_DTR2;
	dsr = MSR_DSR2;
	limit = (pmax_boardtype == DS_PMAX) ? 2 : 3;
	s = spltty();
	for (unit = 2; unit <= limit; unit++, dtr >>= 2, dsr >>= 8) {
		tp = dc_tty[unit];
		dcaddr = (dcregs *)dcpdma[unit].p_addr;
		if ((dcaddr->dc_msr & dsr) || (dcsoftCAR[0] & (1 << unit))) {
			/* carrier present */
			if (!(tp->t_state & TS_CARR_ON))
				(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		} else if ((tp->t_state & TS_CARR_ON) &&
		    (*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			dcaddr->dc_tcr &= ~dtr;
		/*
		 * If we are using hardware flow control and output is stopped,
		 * then resume transmit.
		 */
		if ((tp->t_cflag & CCTS_OFLOW) && (tp->t_state & TS_TTSTOP) &&
		    pmax_boardtype != DS_PMAX) {
			switch (unit) {
			case DCCOMM_PORT:
				if (dcaddr->dc_msr & MSR_CTS2)
					break;
				continue;

			case DCPRINTER_PORT:
				if (dcaddr->dc_msr & MSR_CTS3)
					break;
				continue;
			}
			tp->t_state &= ~TS_TTSTOP;
			dcstart(tp);
		}
	}
	splx(s);
	timeout(dcscan, (void *)0, hz);
}

/*
 * ----------------------------------------------------------------------------
 *
 * dcGetc --
 *
 *	Read a character from a serial line.
 *
 * Results:
 *	A character read from the serial port.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
int
dcGetc(dev)
	dev_t dev;
{
	register dcregs *dcaddr;
	register int c;
	int s;

	dcaddr = (dcregs *)dcpdma[minor(dev)].p_addr;
	if (!dcaddr)
		return (0);
	s = spltty();
	for (;;) {
		if (!(dcaddr->dc_csr & CSR_RDONE))
			continue;
		c = dcaddr->dc_rbuf;
		DELAY(10);
		if (((c >> 8) & 03) == (minor(dev) & 03))
			break;
	}
	splx(s);
	return (c & 0xff);
}

/*
 * Send a char on a port, non interrupt driven.
 */
void
dcPutc(dev, c)
	dev_t dev;
	int c;
{
	register dcregs *dcaddr;
	register u_short tcr;
	register int timeout;
	int s, line;

	s = spltty();

	dcaddr = (dcregs *)dcpdma[minor(dev)].p_addr;
	tcr = dcaddr->dc_tcr;
	dcaddr->dc_tcr = tcr | (1 << minor(dev));
	wbflush();
	DELAY(10);
	while (1) {
		/*
		 * Wait for transmitter to be not busy.
		 */
		timeout = 1000000;
		while (!(dcaddr->dc_csr & CSR_TRDY) && timeout > 0)
			timeout--;
		if (timeout == 0) {
			printf("dcPutc: timeout waiting for CSR_TRDY\n");
			break;
		}
		line = (dcaddr->dc_csr >> 8) & 3;
		/*
		 * Check to be sure its the right port.
		 */
		if (line != minor(dev)) {
			tcr |= 1 << line;
			dcaddr->dc_tcr &= ~(1 << line);
			wbflush();
			DELAY(10);
			continue;
		}
		/*
		 * Start sending the character.
		 */
		dcaddr->dc_tdr = dc_brk[0] | (c & 0xff);
		wbflush();
		DELAY(10);
		/*
		 * Wait for character to be sent.
		 */
		while (1) {
			/*
			 * cc -O bug: this code produces and infinite loop!
			 * while (!(dcaddr->dc_csr & CSR_TRDY))
			 *	;
			 */
			timeout = 1000000;
			while (!(dcaddr->dc_csr & CSR_TRDY) && timeout > 0)
				timeout--;
			line = (dcaddr->dc_csr >> 8) & 3;
			if (line != minor(dev)) {
				tcr |= 1 << line;
				dcaddr->dc_tcr &= ~(1 << line);
				wbflush();
				DELAY(10);
				continue;
			}
			dcaddr->dc_tcr &= ~(1 << minor(dev));
			wbflush();
			DELAY(10);
			break;
		}
		break;
	}
	/*
	 * Enable interrupts for other lines which became ready.
	 */
	if (tcr & 0xF) {
		dcaddr->dc_tcr = tcr;
		wbflush();
		DELAY(10);
	}

	splx(s);
}
#endif /* NDC */
