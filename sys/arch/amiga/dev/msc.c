/*	$NetBSD: msc.c,v 1.47 2014/07/25 08:10:31 dholland Exp $ */

/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *   - converted from NetBSD Amiga serial driver to A2232 serial driver
 *     by zik 931207
 *   - added ttyflags hooks rfh 940419
 *   - added new style config support rfh 940601
 *   - added code to halt board during memory load so board doesn't flip
 *     out. /dev/reload works now. Also created mschwiflow function so BSD can
 *     attempt to use board RTS flow control now. rfh 950108
 *   - Integrated work from Jukka Marin <jmarin@jmp.fi> and
 *     Timo Rossi <trossi@jyu.fi> The mscmint() code is Jukka's. 950916
 *     Integrated more bug fixes by Jukka Marin <jmarin@jmp.fi> 950918
 *     Also added Jukka's turbo board code. 950918
 *   - Reformatted to NetBSD style format.
 *   - Rewritten the carrier detect system to prevent lock-ups (jm 951029)
 */

/*
 * Copyright (c) 1993 Zik.
 * Copyright (c) 1995 Jukka Marin <jmarin@jmp.fi>.
 * Copyright (c) 1995 Timo Rossi <trossi@jyu.fi>.
 * Copyright (c) 1995 Rob Healey <rhealey@kas.helios.mn.org>.
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
 *   - converted from NetBSD Amiga serial driver to A2232 serial driver
 *     by zik 931207
 *   - added ttyflags hooks rfh 940419
 *   - added new style config support rfh 940601
 *   - added code to halt board during memory load so board doesn't flip
 *     out. /dev/reload works now. Also created mschwiflow function so BSD can
 *     attempt to use board RTS flow control now. rfh 950108
 *   - Integrated work from Jukka Marin <jmarin@jmp.fi> and
 *     Timo Rossi <trossi@jyu.fi> The mscmint() code is Jukka's. 950916
 *     Integrated more bug fixes by Jukka Marin <jmarin@jmp.fi> 950918
 *     Also added Jukka's turbo board code. 950918
 *   - Reformatted to NetBSD style format.
 *   - Rewritten the carrier detect system to prevent lock-ups (jm 951029)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: msc.c,v 1.47 2014/07/25 08:10:31 dholland Exp $");

#include "msc.h"

#if NMSC > 0
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <amiga/amiga/device.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/mscreg.h>
#include <machine/cpu.h>

#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/cc.h>

/* 6502 code for A2232 card */
#include "msc6502.h"

/*
 * Note: These are Zik's original comments:
 * There is a bit of a "gotcha" concerning the numbering
 * of msc devices and the specification of the number of serial
 * lines in the kernel.
 *
 * Each board has seven lines, but for programming convenience
 * and compatibility with Amiga UNIX the boards' minor device
 * numbers are allocated in groups of sixteen:
 *
 *                     minor device numbers
 * First board		0  2  4  6  8 10 12
 * Second board        16 18 20 22 24 26 28
 * Third board	       32 34 36 38 40 42 44
 *
 * The intermediate minor device numbers are dialin versions
 * of the same devices. ie. 10 is dialout line 6 and 11 is
 * the dialin version of line 6.
 *
 * On the other hand, I have made the NMSC config option refer
 * to the total number of a2232 cards, not the maximum
 * minor device number. So you might have NMSC=3, in which case
 * you have three boards with minor device numbers from 0 to 45.
 */

int	mscparam(struct tty *, struct termios *);
void	mscstart(struct tty *);
int	mschwiflow(struct tty *, int);
int	mscinitcard(struct zbus_args *);

int	mscdefaultrate = TTYDEF_SPEED;

struct	mscdevice mscdev[MSCSLOTS];	/* device structs for all lines */
struct	tty *msc_tty[MSCTTYS];		/* ttys for all lines */

struct	vbl_node msc_vbl_node[NMSC];	/* vbl interrupt node per board */

const struct speedtab mscspeedtab_normal[] = {
	{ 0,		0		},
	{ 50,		MSCPARAM_B50	},
	{ 75,		MSCPARAM_B75	},
	{ 110,		MSCPARAM_B110	},
	{ 134,		MSCPARAM_B134	},
	{ 150,		MSCPARAM_B150	},
	{ 300,		MSCPARAM_B300	},
	{ 600,		MSCPARAM_B600	},
	{ 1200,		MSCPARAM_B1200	},
	{ 1800,		MSCPARAM_B1800	},
	{ 2400,		MSCPARAM_B2400	},
	{ 3600,		MSCPARAM_B3600	},
	{ 4800,		MSCPARAM_B4800	},
	{ 7200,		MSCPARAM_B7200	},
	{ 9600,		MSCPARAM_B9600	},
	{ 19200,	MSCPARAM_B19200	},
	{ 115200,	MSCPARAM_B115200 },
	{ -1,		-1		}
};

const struct speedtab mscspeedtab_turbo[] = {
	{ 0,		0		},
	{ 100,		MSCPARAM_B50	},
	{ 150,		MSCPARAM_B75	},
	{ 220,		MSCPARAM_B110	},
	{ 269,		MSCPARAM_B134	},
	{ 300,		MSCPARAM_B150	},
	{ 600,		MSCPARAM_B300	},
	{ 1200,		MSCPARAM_B600	},
	{ 2400,		MSCPARAM_B1200	},
	{ 3600,		MSCPARAM_B1800	},
	{ 4800,		MSCPARAM_B2400	},
	{ 7200,		MSCPARAM_B3600	},
	{ 9600,		MSCPARAM_B4800	},
	{ 14400,	MSCPARAM_B7200	},
	{ 19200,	MSCPARAM_B9600	},
	{ 38400,	MSCPARAM_B19200	},
	{ 230400,	MSCPARAM_B115200 },
	{ -1,		-1		}
};

const struct   speedtab *mscspeedtab;

int mscmctl(dev_t dev, int bits, int howto);
void mscmint(register void *data);

int mscmatch(device_t, cfdata_t, void *);
void mscattach(device_t, device_t, void *);

#define	SWFLAGS(dev)	(msc->openflags | (MSCDIALIN(dev) ? 0 : TIOCFLAG_SOFTCAR))
#define	DEBUG_CD	0

CFATTACH_DECL_NEW(msc, 0,
    mscmatch, mscattach, NULL, NULL);

dev_type_open(mscopen);
dev_type_close(mscclose);
dev_type_read(mscread);
dev_type_write(mscwrite);
dev_type_ioctl(mscioctl);
dev_type_stop(mscstop);
dev_type_tty(msctty);
dev_type_poll(mscpoll);

const struct cdevsw msc_cdevsw = {
	.d_open = mscopen,
	.d_close = mscclose,
	.d_read = mscread,
	.d_write = mscwrite,
	.d_ioctl = mscioctl,
	.d_stop = mscstop,
	.d_tty = msctty,
	.d_poll = mscpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

int
mscmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;
	if (zap->manid == 514 && (zap->prodid == 70 || zap->prodid == 69))
		return(1);

	return (0);
}

void
mscattach(device_t parent, device_t self, void *aux)
{
	volatile struct mscmemory *mscmem;
	struct mscdevice *msc;
	struct zbus_args *zap;
	int unit;
	int Count;

	zap = aux;
	unit = device_unit(self);

	/*
	 * Make config msgs look nicer.
	 */
	printf("\n");

	if (mscinitcard(zap) != 0) {
		printf("msc%d: Board initialize failed, bad download code.\n", unit);
		return;
	}

	printf("msc%d: Board successfully initialized.\n", unit);

	mscmem = (struct mscmemory *) zap->va;

	if (mscmem->Common.Crystal == MSC_UNKNOWN) {
		printf("msc%d: Unable to detect crystal frequency.\n", unit);
		return;
	}

	if (mscmem->Common.Crystal == MSC_TURBO) {
		printf("msc%d: Turbo version detected (%02x%02x:%d)\n", unit,
			mscmem->Common.TimerH, mscmem->Common.TimerL,
			mscmem->Common.Pad_a);
		mscspeedtab = mscspeedtab_turbo;
	} else {
		printf("msc%d: Normal version detected (%02x%02x:%d)\n", unit,
			mscmem->Common.TimerH, mscmem->Common.TimerL,
			mscmem->Common.Pad_a);
		mscspeedtab = mscspeedtab_normal;
	}

	mscmem->Common.CDStatus = 0;	/* common status for all 7 ports */

	/* XXX 8 is a constant */
	for (Count = 0; Count < 8 && MSCSLOTUL(unit, Count) < MSCSLOTS; Count++) {
		msc = &mscdev[MSCSLOTUL(unit, Count)];
		msc->board = mscmem;
		msc->port = Count;
		msc->flags = 0;
		msc->openflags = 0;
		msc->active = 1;
		msc->unit = unit;
		msc->closing = false;
		msc_tty[MSCTTYSLOT(MSCSLOTUL(unit, Count))] = NULL;
		msc_tty[MSCTTYSLOT(MSCSLOTUL(unit, Count)) + 1] = NULL;
	}

	/* disable the non-existent eighth port */
	if (MSCSLOTUL(unit, NUMLINES) < MSCSLOTS)
		mscdev[MSCSLOTUL(unit, NUMLINES)].active = 0;

	msc_vbl_node[unit].function = (void (*) (void *)) mscmint;
	msc_vbl_node[unit].data = (void *) unit;

	add_vbl_function (&msc_vbl_node[unit], MSC_VBL_PRIORITY, (void *)unit);

	return;
}

/* ARGSUSED */
int
mscopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	register struct tty *tp;
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
	int error = 0;
	int s, slot, ttyn;

	/* get the device structure */
	slot = MSCSLOT(dev);
	ttyn = MSCTTY(dev);

	if (slot >= MSCSLOTS)
		return ENXIO;

	if (MSCLINE(dev) >= NUMLINES)
		return ENXIO;

	msc = &mscdev[slot];
	ms = &msc->board->Status[msc->port];

	if (!msc->active)
		return ENXIO;

	/*
	 * RFH: WHY here? Put down by while like other serial drivers
	 *      But if we do that it makes things bomb.
	 */
	s = spltty();

	if (!msc_tty[ttyn]) {

		tp = tty_alloc();
		tty_attach(tp);
		msc_tty[ttyn] = tp;
		msc_tty[ttyn+1] = (struct tty *)NULL;

#if 0
		/* default values are not optimal for this device, increase buffers. */
		clfree(&tp->t_rawq);
		clfree(&tp->t_canq);
		clfree(&tp->t_outq);
		clalloc(&tp->t_rawq, 8192, 1);
		clalloc(&tp->t_canq, 8192, 1);
		clalloc(&tp->t_outq, 8192, 0);
#endif

	} else
		tp = msc_tty[ttyn];

	tp->t_oproc = (void (*) (struct tty *)) mscstart;
	tp->t_param = mscparam;
	tp->t_dev = dev;
	tp->t_hwiflow = mschwiflow;

	/* if port is still closing, just bitbucket remaining characters */
	if (msc->closing) {
		ms->OutFlush = true;
		msc->closing = false;
	}
	splx(s);	

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	mutex_spin_enter(&tty_lock);
	/* initialize tty */
	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		ttychars(tp);
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = mscdefaultrate;
		}

		/* flags changed to be private to every unit by JM */
		if (msc->openflags & TIOCFLAG_CLOCAL)
			tp->t_cflag |= CLOCAL;
		if (msc->openflags & TIOCFLAG_CRTSCTS)
			tp->t_cflag |= CRTSCTS;
		if (msc->openflags & TIOCFLAG_MDMBUF)
			tp->t_cflag |= MDMBUF;

		mscparam(tp, &tp->t_termios);
		ttsetwater(tp);

		(void) mscmctl(dev, TIOCM_DTR | TIOCM_RTS, DMSET);

		if ((SWFLAGS(dev) & TIOCFLAG_SOFTCAR) ||
		    (mscmctl(dev, 0, DMGET) & TIOCM_CD))
			tp->t_state |= TS_CARR_ON;
		else
			tp->t_state &= ~TS_CARR_ON;

	}

	/*
	 * if NONBLOCK requested, ignore carrier
	 */
	if (flag & O_NONBLOCK) {
#if DEBUG_CD
		printf("msc%d: %d open nonblock\n", msc->unit, MSCLINE(dev));
#endif
		goto done;
	}

	/*
	 * s = spltty();
	 *
	 * This causes hangs when put here, like other TTY drivers do, rather than
	 * above, WHY? RFH
	 *
	 */

	while ((tp->t_state & TS_CARR_ON) == 0 && (tp->t_cflag & CLOCAL) == 0) {
		tp->t_wopen++;

#if DEBUG_CD
		printf("msc%d: %d waiting for CD\n", msc->unit, MSCLINE(dev));
#endif
		error = ttysleep(tp, &tp->t_rawcv, true, 0);
		tp->t_wopen--;

		if (error) {
			mutex_spin_exit(&tty_lock);
			return(error);
		}
	}

#if DEBUG_CD
	printf("msc%d: %d got CD\n", msc->unit, MSCLINE(dev));
#endif

	done:
		/* This is a way to handle lost XON characters */
		if ((flag & O_TRUNC) && (tp->t_state & TS_TTSTOP)) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart (tp);
		}

	/*
	 * Reset the tty pointer, as there could have been a dialout
	 * use of the tty with a dialin open waiting.
	 */
	tp->t_dev = dev;
	mutex_spin_exit(&tty_lock);

	return tp->t_linesw->l_open(dev, tp);
}


int
mscclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	register struct tty *tp;
	int slot;
	volatile struct mscstatus *ms;
	struct mscdevice *msc;

	/* get the device structure */
	slot = MSCSLOT(dev);

	if (slot >= MSCSLOTS)
		return ENXIO;

	msc = &mscdev[slot];

	if (!msc->active)
		return ENXIO;

	ms = &msc->board->Status[msc->port];

	tp = msc_tty[MSCTTY(dev)];
	tp->t_linesw->l_close(tp, flag);

	(void) mscmctl(dev, 0, DMSET);

	ttyclose(tp);

	if (msc->flags & TIOCM_DTR)
		msc->closing = true; /* flush remaining characters before dropping DTR */
	else
		ms->OutFlush = true; /* just bitbucket remaining characters */

	return (0);
}


int
mscread(dev_t dev, struct uio *uio, int flag)
{
	register struct tty *tp;

	tp = msc_tty[MSCTTY(dev)];

	if (! tp)
	 return ENXIO;

	return tp->t_linesw->l_read(tp, uio, flag);
}


int
mscwrite(dev_t dev, struct uio *uio, int flag)
{
	register struct tty *tp;

	tp = msc_tty[MSCTTY(dev)];

	if (! tp)
		return ENXIO;

	return tp->t_linesw->l_write(tp, uio, flag);
}

int
mscpoll(dev_t dev, int events, struct lwp *l)
{
	register struct tty *tp;

	tp = msc_tty[MSCTTY(dev)];

	if (! tp)
		return ENXIO;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

/*
 * This interrupt is periodically invoked in the vertical blank
 * interrupt. It's used to keep track of the modem control lines
 * and (new with the fast_int code) to move accumulated data up in
 * to the tty layer.
 *
 * NOTE: MSCCDHACK is an invention of mine for dubious purposes. If you
 *	 want to activate it add
 *	 options MSCCDHACK
 *	 in the kernel conf file. Basically it forces CD->Low transitions
 *	 to ALWAYS send a signal to the process, even if the device is in
 *	 clocal mode or an outdial device. RFH
 */
void
mscmint(register void *data)
{
	register struct tty *tp;
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
	volatile u_char *ibuf, *cbuf;
	unsigned char newhead; /* was int */
	unsigned char bufpos;  /* was int */
	unsigned char ncd, ocd, ccd;
	int unit, slot, maxslot;
	int s, i;

	unit = (int) data;

	/* check each line on this board */
	maxslot = MSCSLOTUL(unit, NUMLINES);
	if (maxslot > MSCSLOTS)
		maxslot = MSCSLOTS;

	msc = &mscdev[MSCSLOTUL(unit, 0)];

	newhead = msc->board->Common.CDHead;
	bufpos  = msc->board->Common.CDTail;
	if (newhead != bufpos) {	/* CD events in queue	*/
	    /* set interrupt priority level */
	    s = spltty();
	    ocd = msc->board->Common.CDStatus;		/* get old status bits	*/
	    while (newhead != bufpos) {			/* read all events	*/
		ncd = msc->board->CDBuf[bufpos++];	/* get one event	*/
		ccd = ncd ^ ocd;			/* mask of changed lines*/
		ocd = ncd;				/* save new status bits	*/
#if DEBUG_CD
		printf("ocd %02x ncd %02x ccd %02x\n", ocd, ncd, ccd);
#endif
		for(i = 0; i < NUMLINES; i++) {		/* do for all lines	*/
		    if (ccd & 1) {			/* this one changed	*/
			msc = &mscdev[MSCSLOTUL(unit, i)];
			if (ncd & 1) {	/* CD is now OFF */
#if DEBUG_CD
			    printf("msc%d: CD OFF %d\n", unit, msc->port);
#endif
			    msc->flags &= ~TIOCM_CD;
			    if ((tp = msc_tty[MSCTTYSLOT(MSCSLOTUL(unit, i))]) &&
				 (tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {

#ifndef MSCCDHACK
				if (MSCDIALIN(tp->t_dev))
#endif
				{
				    if (tp->t_linesw->l_modem(tp, 0) == 0) {
					/* clear RTS and DTR, bitbucket output */
					ms = &msc->board->Status[msc->port];
					ms->Command = (ms->Command & ~MSCCMD_CMask) |
						 MSCCMD_Close;
					ms->Setup = true;
					msc->flags &= ~(TIOCM_DTR | TIOCM_RTS);
					ms->OutFlush = true;
				    }
				}
			    }
			} else {	/* CD is now ON */
#if DEBUG_CD
			    printf("msc%d: CD ON %d\n", unit, msc->port);
#endif
			    msc->flags |= TIOCM_CD;
			    if ((tp = msc_tty[MSCTTYSLOT(MSCSLOTUL(unit, i))]) &&
				(tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
				    if (MSCDIALIN(tp->t_dev))
					tp->t_linesw->l_modem(tp, 1);
			    } /* if tp valid and port open */
			}		/* CD on/off */
		    } /* if CD changed for this line */
		    ccd >>= 1; ncd >>= 1;			/* bit for next line */
		} /* for every line */
	    } /* while events in queue */
	msc->board->Common.CDStatus = ocd;		/* save new status */
	msc->board->Common.CDTail   = bufpos;	/* remove events */
	splx(s);
	}	/* if events in CD queue */

	for (slot = MSCSLOTUL(unit, 0); slot < maxslot; slot++) {
	    msc = &mscdev[slot];

	    if (!msc->active)
		continue;

	    tp = msc_tty[MSCTTYSLOT(slot)];
	    ms = &msc->board->Status[msc->port];

	    newhead = ms->InHead;		/* 65c02 write pointer */

	    /* yoohoo, is the port open? */
	    if (tp && (tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
		/* port is open, handle all type of events */

		/* set interrupt priority level */
		s = spltty();

		/* check for input for this port */
		if (newhead != (bufpos = ms->InTail)) {
		    /* buffer for input chars/events */
		    ibuf = &msc->board->InBuf[msc->port][0];

		    /* data types of bytes in ibuf */
		    cbuf = &msc->board->InCtl[msc->port][0];

		    /* do for all chars, if room */
		    while (bufpos != newhead) {
			/* which type of input data? */
			switch (cbuf[bufpos]) {
			    /* input event (CD, BREAK, etc.) */
			    case MSCINCTL_EVENT:
				switch (ibuf[bufpos++]) {
				    case MSCEVENT_Break:
					tp->t_linesw->l_rint(TTY_FE, tp);
					break;

				    default:
					printf("msc%d: unknown event type %d\n",
					msc->unit, ibuf[(bufpos-1)&0xff]);
				} /* event type switch */
				break;

			    case MSCINCTL_CHAR:
				if (tp->t_state & TS_TBLOCK) {
				    goto NoRoomForYa;
				}
				tp->t_linesw->l_rint((int)ibuf[bufpos++], tp);
				break;

			    default:
				printf("msc%d: unknown data type %d\n",
				msc->unit, cbuf[bufpos]);
				bufpos++;
			} /* switch on input data type */
		    } /* while there's something in the buffer */
NoRoomForYa:
		ms->InTail = bufpos;		/* tell 65C02 what we've read */
	    } /* if there was something in the buffer */

	    /* we get here only when the port is open */
	    /* send output */
	    if (tp->t_state & (TS_BUSY|TS_FLUSH)) {

		bufpos = ms->OutHead - ms->OutTail;

		/* busy and below low water mark? */
		if (tp->t_state & TS_BUSY) {
		    if (bufpos < IOBUFLOWWATER) {
			tp->t_state &= ~TS_BUSY;	/* not busy any more */
			if (tp->t_linesw)
			    tp->t_linesw->l_start(tp);
			else
			    mscstart(tp);
		    }
		}

		/* waiting for flush and buffer empty? */
		if (tp->t_state & TS_FLUSH) {
		    if (bufpos == 0)
			tp->t_state &= ~TS_FLUSH;	/* finished flushing */
		}
	    } /* BUSY or FLUSH */

	    splx(s);

	    } else { /* End of port open */
		/* port is closed, don't pass on the chars from it */

		/* check for input for this port */
		if (newhead != (bufpos = ms->InTail)) {
		    /* buffer for input chars/events */
		    ibuf = &msc->board->InBuf[msc->port][0];

		    /* data types of bytes in ibuf */
		    cbuf = &msc->board->InCtl[msc->port][0];

		    /* do for all chars, if room */
			while (bufpos != newhead) {
			    /* which type of input data? */
			    switch (cbuf[bufpos]) {
			    /* input event (BREAK, etc.) */
				case MSCINCTL_EVENT:
				    switch (ibuf[bufpos++]) {
					default:
					    printf("msc: unknown event type %d\n",
						ibuf[(bufpos-1)&0xff]);
				    } /* event type switch */
				    break;

				default:
				    bufpos++;
			    } /* switch on input data type */
			} /* while there's something in the buffer */

		    ms->InTail = bufpos;		/* tell 65C02 what we've read */
		} /* if there was something in the buffer */
	    } /* End of port open/close */

	    /* is this port closing? */
	    if (msc->closing) {
		/* if DTR is off, just bitbucket remaining characters */
		if ( (msc->flags & TIOCM_DTR) == 0) {
		    ms->OutFlush = true;
		    msc->closing = false;
		}
		/* if output has drained, drop DTR */
		else if (ms->OutHead == ms->OutTail) {
		    (void) mscmctl(tp->t_dev, 0, DMSET);
		    msc->closing = false;
		}
	    }
	}  /* For all ports */
}


int
mscioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	register struct tty *tp;
	register int slot;
	register int error;
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
	int s;

	/* get the device structure */
	slot = MSCSLOT(dev);

	if (slot >= MSCSLOTS)
		return ENXIO;

	msc = &mscdev[slot];

	if (!msc->active)
		return ENXIO;

	ms = &msc->board->Status[msc->port];
	if (!(tp = msc_tty[MSCTTY(dev)]))
		return ENXIO;

	error = tp->t_linesw->l_ioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {

		/* send break */
		case TIOCSBRK:
			s = spltty();
			ms->Command = (ms->Command & (~MSCCMD_RTSMask)) | MSCCMD_Break;
			ms->Setup = true;
			splx(s);
			break;

		/* clear break */
		case TIOCCBRK:
			s = spltty();
			ms->Command = (ms->Command & (~MSCCMD_RTSMask)) | MSCCMD_RTSOn;
			ms->Setup = true;
			splx(s);
			break;

		case TIOCSDTR:
			(void) mscmctl(dev, TIOCM_DTR | TIOCM_RTS, DMBIS);
			break;

		case TIOCCDTR:
			if (!MSCDIALIN(dev))	/* don't let dialins drop DTR */
				(void) mscmctl(dev, TIOCM_DTR | TIOCM_RTS, DMBIC);
			break;

		case TIOCMSET:
			(void) mscmctl(dev, *(int *)data, DMSET);
			break;

		case TIOCMBIS:
			(void) mscmctl(dev, *(int *)data, DMBIS);
			break;

		case TIOCMBIC:
			if (MSCDIALIN(dev))	/* don't let dialins drop DTR */
				(void) mscmctl(dev, *(int *)data & TIOCM_DTR, DMBIC);
			else
				(void) mscmctl(dev, *(int *)data, DMBIC);
			break;

		case TIOCMGET:
			*(int *)data = mscmctl(dev, 0, DMGET);
			break;

		case TIOCGFLAGS:
			*(int *)data = SWFLAGS(dev);
			break;

		case TIOCSFLAGS:
			error = kauth_authorize_device_tty(l->l_cred,
			    KAUTH_DEVICE_TTY_PRIVSET, tp);
			if (error != 0)
				return(EPERM);
			msc->openflags = *(int *)data;
			/* only allow valid flags */
			msc->openflags &=
			     (TIOCFLAG_SOFTCAR | TIOCFLAG_CLOCAL | TIOCFLAG_CRTSCTS);
			break;

		default:
			return (EPASSTHROUGH);
	}

	return (0);
}


int
mscparam(register struct tty *tp, register struct termios *t)
{
	register int cflag = t->c_cflag;
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
	int s, slot;
	int ospeed = ttspeedtab(t->c_ospeed, mscspeedtab);

	/* get the device structure */
	slot = MSCSLOT(tp->t_dev);

	if (slot >= MSCSLOTS)
		return ENXIO;

	msc = &mscdev[slot];

	if (!msc->active)
		return ENXIO;

	ms = &msc->board->Status[msc->port];

	/* check requested parameters */
	if (ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed))
		return (EINVAL);

	/* and copy to tty */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = cflag;

	/* hang up if baud is zero */
	if (t->c_ospeed == 0) {
		if (!MSCDIALIN(tp->t_dev))  /* don't let dialins drop DTR */
			(void) mscmctl(tp->t_dev, 0, DMSET);
	} else {
		/* set the baud rate */
		s = spltty();
		ms->Param = (ms->Param & ~MSCPARAM_BaudMask) | ospeed | MSCPARAM_RcvBaud;

		/*
		 * Make sure any previous hangup is undone, ie.  reenable DTR.
		 * also mscmctl will cause the speed to be set
		 */
		(void) mscmctl (tp->t_dev, TIOCM_DTR | TIOCM_RTS, DMSET);

		splx(s);
	}

	return(0);
}


/*
 *	Jukka's code initializes alot of stuff that other drivers don't
 *	I'm including it here so that this code is a common set of work
 *	done by both of us. rfh
 */
int
mschwiflow(struct tty *tp, int flag)
{

/* Rob's version */
#if 1
	if (flag)
		mscmctl( tp->t_dev, TIOCM_RTS, DMBIC);	/* Clear/Lower RTS */
	else
		mscmctl( tp->t_dev, TIOCM_RTS, DMBIS);	/* Set/Raise RTS */

#else	/* Jukka's version */

	int s, slot;
	struct mscdevice *msc;
	volatile struct mscstatus *ms;

	/* get the device structure */
	slot = MSCSLOT(tp->t_dev);
	if (slot >= MSCSLOTS)
		return ENXIO;
	msc = &mscdev[slot];
	if (!msc->active)
		return ENXIO;
	ms = &msc->board->Status[msc->port];

	/* Well, we should really _do_ something here, but the 65c02 code
	 * manages the RTS signal on its own now, so...  This will probably
	 * change in the future.
	 */
#endif
	return 1;
}


void
mscstart(register struct tty *tp)
{
	register int cc;
	register char *cp;
	register int mhead;
	int s, slot;
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
	volatile char *mob;
	int hiwat = 0;
	int maxout;

	if (! (tp->t_state & TS_ISOPEN))
		return;

	slot = MSCSLOT(tp->t_dev);

#if 0
	printf("starting msc%d\n", slot);
#endif

	s = spltty();

	/* don't start if explicitly stopped */
	if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP))
		goto out;

	/* wake up if below low water */
	cc = tp->t_outq.c_cc;
	if (!ttypull(tp) || (tp->t_state & TS_BUSY))
		goto out;

	/*
	 * Limit the amount of output we do in one burst
	 */
	msc = &mscdev[slot];
	ms = &msc->board->Status[msc->port];
	mhead = ms->OutHead;
	maxout = mhead - ms->OutTail;

	if (maxout < 0)
		maxout += IOBUFLEN;

	maxout = IOBUFLEN - 1 - maxout;

	if (cc >= maxout) {
		hiwat++;
		cc = maxout;
	}

	cc = q_to_b (&tp->t_outq, msc->tmpbuf, cc);

	if (cc > 0) {
		tp->t_state |= TS_BUSY;

		mob = &msc->board->OutBuf[msc->port][0];
		cp = &msc->tmpbuf[0];

		/* enable output */
		ms->OutDisable = false;

#if 0
		msc->tmpbuf[cc] = 0;
		printf("sending '%s'\n", msctmpbuf);
#endif

		/* send the first char across to reduce latency */
		mob[mhead++] = *cp++;
		mhead &= IOBUFLENMASK;
		ms->OutHead = mhead;
		cc--;

		/* copy the rest of the chars across quickly */
		while (cc > 0) {
			mob[mhead++] = *cp++;
			mhead &= IOBUFLENMASK;
			cc--;
		}
		ms->OutHead = mhead;

		/* leave the device busy if we've filled the buffer */
		if (!hiwat)
			tp->t_state &= ~TS_BUSY;
	}

out:
	splx(s);
}


/* XXX */
/*
 * Stop output on a line.
 */
/*ARGSUSED*/
void
mscstop(register struct tty *tp, int flag)
/* flag variable defaulted to int anyway */
{
	register int s;
#if 0
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
#endif

	s = spltty();
	if (tp->t_state & TS_BUSY) {
		if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_FLUSH;
#if 0
			msc = &mscdev[MSCSLOT(tp->t_dev)];
			ms = &msc->board->Status[msc->port];
			printf("stopped output on msc%d\n", MSCSLOT(tp->t_dev));
			ms->OutDisable = true;
#endif
		}
	}
	splx(s);
}


/*
 * bits can be: TIOCM_DTR, TIOCM_RTS, TIOCM_CTS, TIOCM_CD, TIOCM_RI, TIOCM_DSR
 */
int
mscmctl(dev_t dev, int bits, int how)
{
	struct mscdevice *msc;
	volatile struct mscstatus *ms;
	int slot;
	int s;
	u_char newcmd;
	int OldFlags;

	/* get the device structure */
	slot = MSCSLOT(dev);

	if (slot >= MSCSLOTS)
		return ENXIO;

	msc = &mscdev[slot];

	if (!msc->active)
		return ENXIO;

	s = spltty();

	if (how != DMGET) {
		OldFlags = msc->flags;
		bits &= TIOCM_DTR | TIOCM_RTS;	/* can only modify DTR and RTS */

		switch (how) {
		    case DMSET:
			msc->flags = (bits | (msc->flags & ~(TIOCM_DTR | TIOCM_RTS)));
			break;

		    case DMBIC:
			msc->flags &= ~bits;
			break;

		    case DMBIS:
			msc->flags |= bits;
			break;
		}

		/* modify modem control state */
		ms = &msc->board->Status[msc->port];

		if (msc->flags & TIOCM_RTS)	/* was bits & */
			newcmd = MSCCMD_RTSOn;
		else			/* this doesn't actually work now */
			newcmd = MSCCMD_RTSOff;

		if (msc->flags & TIOCM_DTR)	/* was bits & */
			newcmd |= MSCCMD_Enable;

		ms->Command = (ms->Command & (~MSCCMD_RTSMask & ~MSCCMD_Enable)) | newcmd;
		ms->Setup = true;

		/* if we've dropped DTR, bitbucket any pending output */
		if ( (OldFlags & TIOCM_DTR) && ((bits & TIOCM_DTR) == 0))
			ms->OutFlush = true;
	}

	bits = msc->flags;

	(void) splx(s);

	return(bits);
}


struct tty *
msctty(dev_t dev)
{
	return(msc_tty[MSCTTY(dev)]);
}


/*
 * Load JM's freely redistributable A2232 6502c code. Let turbo detector
 * run for a while too.
 */

int
mscinitcard(struct zbus_args *zap)
{
	int bcount;
	short start;
	u_char *from;
	volatile u_char *to;
	volatile struct mscmemory *mlm;

	mlm = (volatile struct mscmemory *)zap->va;
	(void)mlm->Enable6502Reset;

	/* copy the code across to the board */
	to = (volatile u_char *)mlm;
	from = msc6502code; bcount = sizeof(msc6502code) - 2;
	start = *(short *)from; from += sizeof(start);
	to += start;

	while(bcount--) *to++ = *from++;

	mlm->Common.Crystal = MSC_UNKNOWN;	/* use automatic speed check */

	/* start 6502 running */
	(void)mlm->ResetBoard;

	/* wait until speed detector has finished */
	for (bcount = 0; bcount < 2000; bcount++) {
		delay(10000);
		if (mlm->Common.Crystal)
			break;
	}

	return(0);
}

#endif  /* NMSC > 0 */
