/*	$NetBSD: ite.c,v 1.25 2001/05/02 10:32:21 scw Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: ite.c 1.1 90/07/09$
 *
 *	@(#)ite.c	7.6 (Berkeley) 5/16/91
 */

/*
 * ite - bitmaped terminal.
 * Supports VT200, a few terminal features will be unavailable until
 * the system actually probes the device (i.e. not after consinit())
 */

#include "ite.h"
#if NITE > 0

#include "bell.h"
#include "kbd.h"

#include "opt_ite.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/kbio.h>
#include <machine/bus.h>
#include <machine/grfioctl.h>
#include <machine/iteioctl.h>

#include <arch/x68k/dev/grfvar.h>
#include <arch/x68k/dev/itevar.h>
#include <arch/x68k/dev/kbdmap.h>
#include <arch/x68k/dev/mfp.h>
#if NBELL > 0
void opm_bell __P((void));
#endif

#define SUBR_CNPROBE(min)	itesw[min].ite_cnprobe(min)
#define SUBR_INIT(ip)		ip->isw->ite_init(ip)
#define SUBR_DEINIT(ip)		ip->isw->ite_deinit(ip)
#define SUBR_PUTC(ip,c,dy,dx,m)	ip->isw->ite_putc(ip,c,dy,dx,m)
#define SUBR_CURSOR(ip,flg)	ip->isw->ite_cursor(ip,flg)
#define SUBR_CLEAR(ip,sy,sx,h,w)	ip->isw->ite_clear(ip,sy,sx,h,w)
#define SUBR_SCROLL(ip,sy,sx,count,dir)	\
    ip->isw->ite_scroll(ip,sy,sx,count,dir)

struct consdev;

__inline static void itesendch __P((int));
__inline static void alignment_display __P((struct ite_softc *));
__inline static void snap_cury __P((struct ite_softc *));
__inline static void ite_dnchar __P((struct ite_softc *, int));
static void ite_inchar __P((struct ite_softc *,	int));
__inline static void ite_clrtoeol __P((struct ite_softc *));
__inline static void ite_clrtobol __P((struct ite_softc *));
__inline static void ite_clrline __P((struct ite_softc *));
__inline static void ite_clrtoeos __P((struct ite_softc *));
__inline static void ite_clrtobos __P((struct ite_softc *));
__inline static void ite_clrscreen __P((struct ite_softc *));
__inline static void ite_dnline __P((struct ite_softc *, int));
__inline static void ite_inline __P((struct ite_softc *, int));
__inline static void ite_index __P((struct ite_softc *));
__inline static void ite_lf __P((struct ite_softc *));
__inline static void ite_crlf __P((struct ite_softc *));
__inline static void ite_cr __P((struct ite_softc *));
__inline static void ite_rlf __P((struct ite_softc *));
static void iteprecheckwrap __P((struct ite_softc *ip));
static void itecheckwrap __P((struct ite_softc *ip));
static int ite_argnum __P((struct ite_softc *ip));
static int ite_zargnum __P((struct ite_softc *ip));
static void ite_sendstr __P((struct ite_softc *ip, char *str));
__inline static int atoi __P((const char *cp));
__inline static char *index __P((const char *cp, char ch));
void ite_reset __P((struct ite_softc *ip));
struct ite_softc *getitesp __P((dev_t));
int iteon __P((dev_t, int));
void iteoff __P((dev_t, int));

struct itesw itesw[] = {
	{0,	tv_init,	tv_deinit,	0,
	 0,	0,		0}
};
int	nitesw = sizeof(itesw) / sizeof(itesw[0]);

/*
 * # of chars are output in a single itestart() call.
 * If this is too big, user processes will be blocked out for
 * long periods of time while we are emptying the queue in itestart().
 * If it is too small, console output will be very ragged.
 */
#define ITEBURST 64

int	nite = NITE;
struct	tty *ite_tty[NITE];
struct	ite_softc *kbd_ite = NULL;
struct  ite_softc con_itesoftc;

struct  tty *kbd_tty = NULL;

int	start_repeat_timeo = 20; /* /100: initial timeout till pressed key repeats */
int	next_repeat_timeo  = 3;  /* /100: timeout when repeating for next char */

u_char	cons_tabs[MAX_TABS];

cdev_decl(ite);

void	itestart __P((struct tty *tp));

void iteputchar __P((int c, struct ite_softc *ip));
void ite_putstr __P((const u_char * s, int len, dev_t dev));

void iteattach __P((struct device *, struct device *, void *));
int itematch __P((struct device *, struct cfdata *, void *));

struct cfattach ite_ca = {
	sizeof(struct ite_softc), itematch, iteattach
};

extern struct cfdriver ite_cd;

int
itematch(pdp, cdp, auxp)
	struct device *pdp;
	struct cfdata *cdp;
	void *auxp;
{
	struct grf_softc *gp;
#if 0
	int maj;
#endif
	
	gp = auxp;

	/* ite0 should be at grf0 XXX */
	if(cdp->cf_unit != gp->g_device.dv_unit)
		return(0);

#if 0
	/*
	 * all that our mask allows (more than enough no one 
	 * has > 32 monitors for text consoles on one machine)
	 */
	if (cdp->cf_unit >= sizeof(ite_confunits) * NBBY)
		return(0);
	/*
	 * XXX
	 * normally this would be done in attach, however
	 * during early init we do not have a device pointer
	 * and thus no unit number.
	 */
	for(maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == iteopen)
			break;
	gp->g_itedev = makedev(maj, cdp->cf_unit);
#endif
	return(1);
}

/* 
 * iteinit() is the standard entry point for initialization of
 * an ite device, it is also called from ite_cninit().
 */
void
iteattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct ite_softc *ip;
	struct grf_softc *gp;

	gp = (struct grf_softc *)auxp;
	if (dp) {
		ip = (struct ite_softc *)dp;
		if(con_itesoftc.grf != NULL
			/*&& con_itesoftc.grf->g_unit == gp->g_unit*/) {
			/*
			 * console reinit copy params over.
			 * and console always gets keyboard
			 */
			bcopy(&con_itesoftc.grf, &ip->grf,
			    (char *)&ip[1] - (char *)&ip->grf);
			con_itesoftc.grf = NULL;
			kbd_ite = ip;
		}
		ip->grf = gp;
		iteinit(ip->device.dv_unit); /* XXX */
		printf(": rows %d cols %d", ip->rows, ip->cols);
		if (kbd_ite == NULL)
			kbd_ite = ip;
		printf("\n");
	} else {
		if (con_itesoftc.grf != NULL) 
			return;
		con_itesoftc.grf = gp;
		con_itesoftc.tabs = cons_tabs;
	}
}

struct ite_softc *
getitesp(dev)
	dev_t dev;
{
	extern int x68k_realconfig;

	if (x68k_realconfig && con_itesoftc.grf == NULL)
		return(ite_cd.cd_devs[UNIT(dev)]);

	if (con_itesoftc.grf == NULL)
		panic("no ite_softc for console");
	return(&con_itesoftc);
}

void
iteinit(dev)
	dev_t dev;
{
	struct ite_softc *ip;

	ip = getitesp(dev);

	if (ip->flags & ITE_INITED)
		return;
	bcopy(&ascii_kbdmap, &kbdmap, sizeof(struct kbdmap));

	ip->curx = 0;
	ip->cury = 0;
	ip->cursorx = 0;
	ip->cursory = 0;

	ip->isw = &itesw[ip->device.dv_unit]; /* XXX */
	SUBR_INIT(ip);
	SUBR_CURSOR(ip, DRAW_CURSOR);
	if (!ip->tabs)
		ip->tabs = malloc(MAX_TABS*sizeof(u_char), M_DEVBUF, M_WAITOK);
	ite_reset(ip);
	ip->flags |= ITE_INITED;
}

/*
 * Perform functions necessary to setup device as a terminal emulator.
 */
int
iteon(dev, flag)
	dev_t dev;
	int flag;
{
	int unit = UNIT(dev);
	struct ite_softc *ip;

	if (unit < 0 || unit >= ite_cd.cd_ndevs ||
	    (ip = getitesp(unit)) == NULL || (ip->flags&ITE_ALIVE) == 0)
		return(ENXIO);
	/* force ite active, overriding graphics mode */
	if (flag & 1) {
		ip->flags |= ITE_ACTIVE;
		ip->flags &= ~(ITE_INGRF|ITE_INITED);
	}
	/* leave graphics mode */
	if (flag & 2) {
		ip->flags &= ~ITE_INGRF;
		if ((ip->flags & ITE_ACTIVE) == 0)
			return(0);
	}
	ip->flags |= ITE_ACTIVE;
	if (ip->flags & ITE_INGRF)
		return(0);
	iteinit(dev);
	if (flag & 2)
		ite_reset(ip);
#if NKBD > 0
	mfp_send_usart (0x49);	/* XXX */
#endif
	return(0);
}

/*
 * "Shut down" device as terminal emulator.
 * Note that we do not deinit the console device unless forced.
 * Deinit'ing the console every time leads to a very active
 * screen when processing /etc/rc.
 */
void
iteoff(dev, flag)
	dev_t dev;
	int flag;
{
	int unit = UNIT(dev);
	register struct ite_softc *ip;

	/* XXX check whether when call from grf.c */
	if (unit < 0 || unit >= ite_cd.cd_ndevs ||
	    (ip = getitesp(unit)) == NULL || (ip->flags&ITE_ALIVE) == 0)
		return;
	if (flag & 2)
		ip->flags |= ITE_INGRF;

	if ((ip->flags & ITE_ACTIVE) == 0)
		return;
	if ((flag & 1) ||
	    (ip->flags & (ITE_INGRF|ITE_ISCONS|ITE_INITED)) == ITE_INITED)
		SUBR_DEINIT(ip);

	/*
	 * XXX When the system is rebooted with "reboot", init(8)
	 * kills the last process to have the console open.
	 * If we don't revent the the ITE_ACTIVE bit from being
	 * cleared, we will never see messages printed during
	 * the process of rebooting.
	 */
	if ((flag & 2) == 0 && (ip->flags & ITE_ISCONS) == 0) {
		ip->flags &= ~ITE_ACTIVE;
#if NKBD > 0
		mfp_send_usart (0x48);	/* XXX */
#endif
	}
}

/*
 * standard entry points to the device.
 */

/* ARGSUSED */
int
iteopen(dev, mode, devtype, p)
	dev_t dev;
	int mode, devtype;
	struct proc *p;
{
	int unit = UNIT(dev);
	register struct tty *tp;
	register struct ite_softc *ip;
	register int error;
	int first = 0;

	if (unit >= ite_cd.cd_ndevs || (ip = getitesp(dev)) == NULL)
		return (ENXIO);
	if (!ite_tty[unit]) {
		tp = ite_tty[unit] = ttymalloc();
		tty_attach(tp);
	} else
		tp = ite_tty[unit];
	if ((tp->t_state&(TS_ISOPEN|TS_XCLUDE)) == (TS_ISOPEN|TS_XCLUDE)
	    && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	if ((ip->flags & ITE_ACTIVE) == 0) {
		error = iteon(dev, 0);
		if (error)
			return (error);
		first = 1;
	}
	tp->t_oproc = itestart;
	tp->t_param = NULL;
	tp->t_dev = dev;
	if ((tp->t_state&TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		tp->t_state = TS_ISOPEN|TS_CARR_ON;
		ttsetwater(tp);
	}
	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error == 0) {
		tp->t_winsize.ws_row = ip->rows;
		tp->t_winsize.ws_col = ip->cols;
	} else if (first)
		iteoff(dev, 0);
	return (error);
}

/*ARGSUSED*/
int
iteclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = ite_tty[UNIT(dev)];

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	iteoff(dev, 0);
#if 0
	ttyfree(tp);
	ite_tty[UNIT(dev)] = (struct tty *)0;
#endif
	return(0);
}

int
iteread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = ite_tty[UNIT(dev)];

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
itewrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = ite_tty[UNIT(dev)];

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
itepoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	register struct tty *tp = ite_tty[UNIT(dev)];
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

struct tty *
itetty(dev)
	dev_t dev;
{

	return (ite_tty[UNIT(dev)]);
}

int
iteioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct iterepeat *irp;
	register struct tty *tp = ite_tty[UNIT(dev)];
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, addr, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {
	case ITEIOCSKMAP:
		if (addr == 0)
			return(EFAULT);
		bcopy(addr, &kbdmap, sizeof(struct kbdmap));
		return(0);

	case ITEIOCGKMAP:
		if (addr == NULL)
			return(EFAULT);
		bcopy(&kbdmap, addr, sizeof(struct kbdmap));
		return(0);

	case ITEIOCGREPT:
		irp = (struct iterepeat *)addr;
		irp->start = start_repeat_timeo;
		irp->next = next_repeat_timeo;

	case ITEIOCSREPT:
		irp = (struct iterepeat *)addr;
		if (irp->start < ITEMINREPEAT && irp->next < ITEMINREPEAT)
			return(EINVAL);
		start_repeat_timeo = irp->start;
		next_repeat_timeo = irp->next;
#if x68k
	case ITELOADFONT:
		if (addr) {
			bcopy(addr, kern_font, 4096 /*sizeof (kernel_font)*/);
			return 0;
		} else
			return EFAULT;

	case ITETVCTRL:
		if (addr && *(u_int8_t *)addr < 0x40) {
			return mfp_send_usart (* (u_int8_t *)addr);
		} else {
			return EFAULT;
		}
#endif
	}
	return (ENOTTY);
}

void
itestart(tp)
	register struct tty *tp;
{
	struct clist *rbp;
	struct ite_softc *ip;
	u_char buf[ITEBURST];
	int s, len;

	ip = getitesp(tp->t_dev);
	/*
	 * (Potentially) lower priority.  We only need to protect ourselves
	 * from keyboard interrupts since that is all that can affect the
	 * state of our tty (kernel printf doesn't go through this routine).
	 */
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	rbp = &tp->t_outq;
	len = q_to_b(rbp, buf, ITEBURST);
	/*splx(s);*/

	/* Here is a really good place to implement pre/jumpscroll() */
	ite_putstr(buf, len, tp->t_dev);

	/*s = spltty();*/
	tp->t_state &= ~TS_BUSY;
	/* we have characters remaining. */
	if (rbp->c_cc) {
		tp->t_state |= TS_TIMEOUT;
		callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, tp);
	}
	/* wakeup we are below */
	if (rbp->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)rbp);
		}
		selwakeup(&tp->t_wsel);
	}
out:
	splx(s);
}

/* XXX called after changes made in underlying grf layer. */
/* I want to nuke this */
void
ite_reinit(dev)
	dev_t dev;
{
	struct ite_softc *ip;
	int unit = UNIT(dev);

	/* XXX check whether when call from grf.c */
	if (unit < 0 || unit >= ite_cd.cd_ndevs ||
	    (ip = getitesp(unit)) == NULL)
		return;

	ip->flags &= ~ITE_INITED;
	iteinit(dev);
}

void
ite_reset(ip)
    struct ite_softc *ip;
{
	int i;

	ip->curx = 0;
	ip->cury = 0;
	ip->attribute = 0;
	ip->save_curx = 0;
	ip->save_cury = 0;
	ip->save_attribute = 0;
	ip->ap = ip->argbuf;
	ip->emul_level = EMUL_VT300_8;
	ip->eightbit_C1 = 0;
	ip->top_margin = 0;
	ip->bottom_margin = ip->rows - 1;
	ip->inside_margins = 0; /* origin mode == absolute */
	ip->linefeed_newline = 0;
	ip->auto_wrap = 1;
	ip->cursor_appmode = 0;
	ip->keypad_appmode = 0;
	ip->imode = 0;
	ip->key_repeat = 1;
	ip->G0 = CSET_ASCII;
	ip->G1 = CSET_JIS1983;
	ip->G2 = CSET_JISKANA;
	ip->G3 = CSET_JIS1990;
	ip->GL = &ip->G0;
	ip->GR = &ip->G1;
	ip->save_GL = 0;
	ip->save_char = 0;
	ip->fgcolor = 7;
	ip->bgcolor = 0;
	for (i = 0; i < ip->cols; i++)
		ip->tabs[i] = ((i & 7) == 0);
	/* XXX clear screen */
	SUBR_CLEAR(ip, 0, 0, ip->rows, ip->cols);
	attrclr(ip, 0, 0, ip->rows, ip->cols);
}

/* Used in console at startup only */
int
ite_cnfilter(c)
	u_char c;
{
	static u_char mod = 0;
	struct key key;
	u_char code, up, mask;
	int s;

	up = c & 0x80 ? 1 : 0;
	c &= 0x7f;
	code = 0;

	s = spltty();

	mask = 0;
	if (c >= KBD_LEFT_ALT && !(c >= 0x63 && c <= 0x6c)) {	/* 0x63: F1, 0x6c:F10 */
		switch (c) {
		case KBD_LEFT_SHIFT:
			mask = KBD_MOD_SHIFT;
			break;

		case KBD_LEFT_ALT:
			mask = KBD_MOD_LALT;
			break;

		case KBD_RIGHT_ALT:
			mask = KBD_MOD_RALT;
			break;

		case KBD_LEFT_META:
			mask = KBD_MOD_LMETA;
			break;

		case KBD_RIGHT_META:
			mask = KBD_MOD_RMETA;
			break;

		case KBD_CAPS_LOCK:    
			/*
			 * capslock already behaves `right', don't need to
			 * keep track of the state in here.
			 */
			mask = KBD_MOD_CAPS;
			break;

		case KBD_CTRL:
			mask = KBD_MOD_CTRL;
			break;

		case KBD_RECONNECT:
			/* ite got 0xff */
			if (up)
				kbd_setLED();
			break;
		}
		if (mask & KBD_MOD_CAPS) {
			if (!up) {
				mod ^= KBD_MOD_CAPS;
				kbdled ^= LED_CAPS_LOCK;
				kbd_setLED();
			}
		} else if (up)
			mod &= ~mask;
		else mod |= mask;
		splx (s);
		return -1;
	}

	if (up) {
		splx(s);
		return -1;
	}

	/* translate modifiers */
	if (mod & KBD_MOD_SHIFT) {
		if (mod & KBD_MOD_ALT)
			key = kbdmap.alt_shift_keys[c];
		else 
			key = kbdmap.shift_keys[c];
	} else if (mod & KBD_MOD_ALT)
		key = kbdmap.alt_keys[c];
	else {
		key = kbdmap.keys[c];
		/* if CAPS and key is CAPable (no pun intended) */
		if ((mod & KBD_MOD_CAPS) && (key.mode & KBD_MODE_CAPS))
			key = kbdmap.shift_keys[c];
	}
	code = key.code;

	/* if string return */
	if (key.mode & (KBD_MODE_STRING | KBD_MODE_KPAD)) {
		splx(s);
		return -1;
	}
	/* handle dead keys */
	if (key.mode & KBD_MODE_DEAD) {
		splx(s);
		return -1;
	}
	if (mod & KBD_MOD_CTRL)
		code &= 0x1f;
	if (mod & KBD_MOD_META)
		code |= 0x80;

	/* do console mapping. */
	code = code == '\r' ? '\n' : code;

	splx(s);
	return (code);
}

/* And now the old stuff. */
__inline static void
itesendch (ch)
	int ch;
{
	(*kbd_tty->t_linesw->l_rint)(ch, kbd_tty);
}


void
ite_filter(c)
	u_char c;
{
	static u_short mod = 0;
	register unsigned char code, *str;
	u_short up, mask;
	struct key key;
	int s, i;

	if (!kbd_ite || !(kbd_tty = ite_tty[kbd_ite->device.dv_unit]))
		return;

	/* have to make sure we're at spltty in here */
	s = spltty ();

	up = c & 0x80 ? 1 : 0;
	c &= 0x7f;
	code = 0;
  
	mask = 0;
	if (c >= KBD_LEFT_ALT &&
	    !(c >= 0x63 && c <= 0x6c)) {	/* 0x63: F1, 0x6c:F10 */
		switch (c) {
		case KBD_LEFT_SHIFT:
			mask = KBD_MOD_SHIFT;
			break;

		case KBD_LEFT_ALT:
			mask = KBD_MOD_LALT;
			break;
    
		case KBD_RIGHT_ALT:
			mask = KBD_MOD_RALT;
			break;
      
		case KBD_LEFT_META:
			mask = KBD_MOD_LMETA;
			break;

		case KBD_RIGHT_META:
			mask = KBD_MOD_RMETA;
			break;

		case KBD_CAPS_LOCK:
			/*
			 * capslock already behaves `right', don't need to keep
			 * track of the state in here.
			 */
			mask = KBD_MOD_CAPS;
			break;

		case KBD_CTRL:
			mask = KBD_MOD_CTRL;
			break;

		case KBD_OPT1:
			mask = KBD_MOD_OPT1;
			break;

		case KBD_OPT2:
			mask = KBD_MOD_OPT2;
			break;

		case KBD_RECONNECT:
			if (up) { /* ite got 0xff */
				kbd_setLED();
			}
			break;
		}

		if (mask & KBD_MOD_CAPS) {
			if (!up) {
				mod ^= KBD_MOD_CAPS;
				kbdled ^= LED_CAPS_LOCK;
				kbd_setLED();
			}
		} else if (up) {
			mod &= ~mask;
		} else mod |= mask;

		/*
		 * return even if it wasn't a modifier key, the other
		 * codes up here are either special (like reset warning),
		 * or not yet defined
		 */
		splx (s);
		return;
	}

	if (up) {
		splx (s);
		return;
	}

	/*
	 * intercept LAlt-LMeta-F1 here to switch back to original ascii-keymap.
	 * this should probably be configurable..
	 */
	if (mod == (KBD_MOD_LALT|KBD_MOD_LMETA) && c == 0x63) {
		bcopy (&ascii_kbdmap, &kbdmap, sizeof (struct kbdmap));
		splx (s);
		return;
	}

	/* translate modifiers */
	if (mod & KBD_MOD_SHIFT) {
		if (mod & KBD_MOD_ALT)
			key = kbdmap.alt_shift_keys[c];
		else 
			key = kbdmap.shift_keys[c];
	} else if (mod & KBD_MOD_ALT)
		key = kbdmap.alt_keys[c];
	else {
		key = kbdmap.keys[c];
		/* if CAPS and key is CAPable (no pun intended) */
		if ((mod & KBD_MOD_CAPS) && (key.mode & KBD_MODE_CAPS))
			key = kbdmap.shift_keys[c];
		else if ((mod & KBD_MOD_OPT2) && (key.mode & KBD_MODE_KPAD))
			key = kbdmap.shift_keys[c];
	}
	code = key.code;
 
	/* handle dead keys */
	if (key.mode & KBD_MODE_DEAD) {
		splx (s);
		return;
	}
  	/* if not string, apply META and CTRL modifiers */
	if (! (key.mode & KBD_MODE_STRING) 
	    && (!(key.mode & KBD_MODE_KPAD) ||
		(kbd_ite && !kbd_ite->keypad_appmode))) {
		if ((mod & KBD_MOD_CTRL) &&
		    (code == ' ' || (code >= '@' && code <= 'z')))
			code &= 0x1f;
		if (mod & KBD_MOD_META)
			code |= 0x80;
	} else if ((key.mode & KBD_MODE_KPAD) &&
	       (kbd_ite && kbd_ite->keypad_appmode)) {
		static char *in = "0123456789-+.\r()/*";
		static char *out = "pqrstuvwxymlnMPQRS";
		char *cp = index (in, code);

		/* 
		 * keypad-appmode sends SS3 followed by the above
		 * translated character
		 */
		(*kbd_tty->t_linesw->l_rint) (27, kbd_tty);
		(*kbd_tty->t_linesw->l_rint) ('O', kbd_tty);
		(*kbd_tty->t_linesw->l_rint) (out[cp - in], kbd_tty);
		splx(s);
		return;
	} else {
		/* *NO* I don't like this.... */
		static u_char app_cursor[] =
		{
		  3, 27, 'O', 'A',
		  3, 27, 'O', 'B',
		  3, 27, 'O', 'C',
		  3, 27, 'O', 'D'};

		str = kbdmap.strings + code;
		/* 
		 * if this is a cursor key, AND it has the default
		 * keymap setting, AND we're in app-cursor mode, switch
		 * to the above table. This is *nasty* !
		 */
		if (c >= 0x3b && c <= 0x3e && kbd_ite->cursor_appmode
		    && !bcmp(str, "\x03\x1b[", 3) &&
		    index("ABCD", str[3]))
			str = app_cursor + 4 * (str[3] - 'A');

		/* 
		 * using a length-byte instead of 0-termination allows
		 * to embed \0 into strings, although this is not used
		 * in the default keymap
		 */
		for (i = *str++; i; i--)
			(*kbd_tty->t_linesw->l_rint) (*str++, kbd_tty);
		splx(s);
		return;
	}
	(*kbd_tty->t_linesw->l_rint)(code, kbd_tty);

	splx(s);
	return;
}

/* helper functions, makes the code below more readable */
__inline static void
ite_sendstr (ip, str)
	struct ite_softc *ip;
	char *str;
{
	while (*str)
		itesendch (*str++);
}

__inline static void
alignment_display(ip)
	struct ite_softc *ip;
{
	int i, j;

	for (j = 0; j < ip->rows; j++)
		for (i = 0; i < ip->cols; i++)
			SUBR_PUTC(ip, 'E', j, i, ATTR_NOR);
	attrclr(ip, 0, 0, ip->rows, ip->cols);
}

__inline static void
snap_cury(ip)
	struct ite_softc *ip;
{
	if (ip->inside_margins) {
		if (ip->cury < ip->top_margin)
			ip->cury = ip->top_margin;
		if (ip->cury > ip->bottom_margin)
			ip->cury = ip->bottom_margin;
	}
}

__inline static void
ite_dnchar(ip, n)
	struct ite_softc *ip;
	int n;
{
	n = min(n, ip->cols - ip->curx);
	if (n < ip->cols - ip->curx) {
		SUBR_SCROLL(ip, ip->cury, ip->curx + n, n, SCROLL_LEFT);
		attrmov(ip, ip->cury, ip->curx + n, ip->cury, ip->curx,
			1, ip->cols - ip->curx - n);
		attrclr(ip, ip->cury, ip->cols - n, 1, n);
	}
	while (n-- > 0)
		SUBR_PUTC(ip, ' ', ip->cury, ip->cols - n - 1, ATTR_NOR);
}

static void
ite_inchar(ip, n)
	struct ite_softc *ip;
	int n;
{
	int c = ip->save_char;
	ip->save_char = 0;
	n = min(n, ip->cols - ip->curx);
	if (n < ip->cols - ip->curx) {
		SUBR_SCROLL(ip, ip->cury, ip->curx, n, SCROLL_RIGHT);
		attrmov(ip, ip->cury, ip->curx, ip->cury, ip->curx + n,
			1, ip->cols - ip->curx - n);
		attrclr(ip, ip->cury, ip->curx, 1, n);
	}
	while (n--)
		SUBR_PUTC(ip, ' ', ip->cury, ip->curx + n, ATTR_NOR);
	ip->save_char = c;
}

__inline static void
ite_clrtoeol(ip)
	struct ite_softc *ip;
{
	int y = ip->cury, x = ip->curx;
	if (ip->cols - x > 0) {
		SUBR_CLEAR(ip, y, x, 1, ip->cols - x);
		attrclr(ip, y, x, 1, ip->cols - x);
	}
}

__inline static void
ite_clrtobol(ip)
	struct ite_softc *ip;
{
	int y = ip->cury, x = min(ip->curx + 1, ip->cols);
	SUBR_CLEAR(ip, y, 0, 1, x);
	attrclr(ip, y, 0, 1, x);
}

__inline static void
ite_clrline(ip)
	struct ite_softc *ip;
{
	int y = ip->cury;
	SUBR_CLEAR(ip, y, 0, 1, ip->cols);
	attrclr(ip, y, 0, 1, ip->cols);
}

__inline static void
ite_clrtoeos(ip)
	struct ite_softc *ip;
{
	ite_clrtoeol(ip);
	if (ip->cury < ip->rows - 1) {
		SUBR_CLEAR(ip, ip->cury + 1, 0, ip->rows - 1 - ip->cury, ip->cols);
		attrclr(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
	}
}

__inline static void
ite_clrtobos(ip)
	struct ite_softc *ip;
{
	ite_clrtobol(ip);
	if (ip->cury > 0) {
		SUBR_CLEAR(ip, 0, 0, ip->cury, ip->cols);
		attrclr(ip, 0, 0, ip->cury, ip->cols);
	}
}

__inline static void
ite_clrscreen(ip)
	struct ite_softc *ip;
{
	SUBR_CLEAR(ip, 0, 0, ip->rows, ip->cols);
	attrclr(ip, 0, 0, ip->rows, ip->cols);
}



__inline static void
ite_dnline(ip, n)
	struct ite_softc *ip;
	int n;
{
	/*
	 * interesting.. if the cursor is outside the scrolling
	 * region, this command is simply ignored..
	 */
	if (ip->cury < ip->top_margin || ip->cury > ip->bottom_margin)
		return;

	n = min(n, ip->bottom_margin + 1 - ip->cury);
	if (n <= ip->bottom_margin - ip->cury) {
		SUBR_SCROLL(ip, ip->cury + n, 0, n, SCROLL_UP);
		attrmov(ip, ip->cury + n, 0, ip->cury, 0,
			ip->bottom_margin + 1 - ip->cury - n, ip->cols);
	}
	SUBR_CLEAR(ip, ip->bottom_margin - n + 1, 0, n, ip->cols);
	attrclr(ip, ip->bottom_margin - n + 1, 0, n, ip->cols);
}

__inline static void
ite_inline(ip, n)
	struct ite_softc *ip;
	int n;
{
	/*
	 * interesting.. if the cursor is outside the scrolling
	 * region, this command is simply ignored..
	 */
	if (ip->cury < ip->top_margin || ip->cury > ip->bottom_margin)
		return;

	if (n <= 0)
		n = 1;
	else n = min(n, ip->bottom_margin + 1 - ip->cury);
	if (n <= ip->bottom_margin  - ip->cury) {
		SUBR_SCROLL(ip, ip->cury, 0, n, SCROLL_DOWN);
		attrmov(ip, ip->cury, 0, ip->cury + n, 0,
			ip->bottom_margin + 1 - ip->cury - n, ip->cols);
	}
	SUBR_CLEAR(ip, ip->cury, 0, n, ip->cols);
	attrclr(ip, ip->cury, 0, n, ip->cols);
	ip->curx = 0;
}

__inline static void
ite_index (ip)
	struct ite_softc *ip;
{
	++ip->cury;
	if ((ip->cury == ip->bottom_margin+1) || (ip->cury == ip->rows)) {
		ip->cury--;
		SUBR_SCROLL(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
		ite_clrline(ip);
	}
	/*clr_attr(ip, ATTR_INV);*/
}

__inline static void
ite_lf (ip)
	struct ite_softc *ip;
{
	++ip->cury;
	if (ip->cury > ip->bottom_margin) {
		ip->cury--;
		SUBR_SCROLL(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
		ite_clrline(ip);
	}
/*	SUBR_CURSOR(ip, MOVE_CURSOR);*/
	/*clr_attr(ip, ATTR_INV);*/
	/* reset character set ... thanks for mohta. */
	ip->G0 = CSET_ASCII;
	ip->G1 = CSET_JIS1983;
	ip->G2 = CSET_JISKANA;
	ip->G3 = CSET_JIS1990;
	ip->GL = &ip->G0;
	ip->GR = &ip->G1;
	ip->save_GL = 0;
	ip->save_char = 0;
}

__inline static void
ite_crlf (ip)
	struct ite_softc *ip;
{
	ip->curx = 0;
	ite_lf (ip);
}

__inline static void
ite_cr (ip)
	struct ite_softc *ip;
{
	if (ip->curx) {
		ip->curx = 0;
	}
}

__inline static void
ite_rlf (ip)
	struct ite_softc *ip;
{
	ip->cury--;
	if ((ip->cury < 0) || (ip->cury == ip->top_margin - 1)) {
		ip->cury++;
		SUBR_SCROLL(ip, ip->top_margin, 0, 1, SCROLL_DOWN);
		ite_clrline(ip);
	}
	clr_attr(ip, ATTR_INV);
}

__inline static int
atoi (cp)
    const char *cp;
{
	int n;

	for (n = 0; *cp && *cp >= '0' && *cp <= '9'; cp++)
		n = n * 10 + *cp - '0';
	return n;
}

__inline static char *
index(cp, ch)
	const char *cp;
	char ch;
{
	while (*cp && *cp != ch)
		cp++;
	return *cp ? (char *) cp : 0;
}

__inline static int
ite_argnum (ip)
	struct ite_softc *ip;
{
	char ch;
	int n;

	/* convert argument string into number */
	if (ip->ap == ip->argbuf)
		return 1;
	ch = *ip->ap;
	*ip->ap = 0;
	n = atoi (ip->argbuf);
	*ip->ap = ch;

	return n;
}

__inline static int
ite_zargnum (ip)
	struct ite_softc *ip;
{
	char ch;
	int n;

	/* convert argument string into number */
	if (ip->ap == ip->argbuf)
		return 0;
	ch = *ip->ap;
	*ip->ap = 0;	/* terminate string */
	n = atoi (ip->argbuf);
	*ip->ap = ch;
  
	return n;	/* don't "n ? n : 1" here, <CSI>0m != <CSI>1m ! */
}

void
ite_putstr(s, len, dev)
	const u_char *s;
	int len;
	dev_t dev;
{
	struct ite_softc *ip;
	int i;

	ip = getitesp(dev);

	/* XXX avoid problems */
	if ((ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE)
	  	return;

	SUBR_CURSOR(ip, START_CURSOROPT);
	for (i = 0; i < len; i++)
		if (s[i])
			iteputchar(s[i], ip);
	SUBR_CURSOR(ip, END_CURSOROPT);
}

void
iteputchar(c, ip)
	register int c;
	struct ite_softc *ip;
{
	int n, x, y;
	char *cp;

	if (c >= 0x20 && ip->escape) {
		switch (ip->escape) {

		case ESC:
			switch (c) {
				/* first 7bit equivalents for the 8bit control characters */
		  
			case 'D':
				c = IND;
				ip->escape = 0;
				break; /* and fall into the next switch below (same for all `break') */

			case 'E':
				/* next line */
				c = NEL;
				ip->escape = 0;
				break;

			case 'H':
				/* set TAB at current col */
				c = HTS;
				ip->escape = 0;
				break;

			case 'M':
				/* reverse index */
				c = RI;
				ip->escape = 0;
				break;

			case 'N':
				/* single shift G2 */
				c = SS2;
				ip->escape = 0;
				break;

			case 'O':
				/* single shift G3 */
				c = SS3;
				ip->escape = 0;
				break;

			case 'P':
				/* DCS detected */
				c = DCS;
				ip->escape = 0;
				break;

			case '[':
				c = CSI;
				ip->escape = 0;
				break;

			case '\\':
				/* String Terminator */
				c = ST;
				ip->escape = 0;
				break;

			case ']':
				c = OSC;
				ip->escape = 0;
				break;

			case '^':
				c = PM;
				ip->escape = 0;
				break;

			case '_':
				c = APC;
				ip->escape = 0;
				break;


			/* introduces 7/8bit control */
			case ' ':
				/* can be followed by either F or G */
				ip->escape = ' ';
				break;


			/* a lot of character set selections, not yet used... 
			   94-character sets: */
			case '(':	/* G0 */
			case ')':	/* G1 */
				ip->escape = c;
				return;

			case '*':	/* G2 */
			case '+':	/* G3 */
			case 'B':	/* ASCII */
			case 'A':	/* ISO latin 1 */
			case '<':	/* user preferred suplemental */
			case '0':	/* dec special graphics */

			/* 96-character sets: */
			case '-':	/* G1 */
			case '.':	/* G2 */
			case '/':	/* G3 */

			/* national character sets: */
			case '4':	/* dutch */
			case '5':
			case 'C':	/* finnish */
			case 'R':	/* french */
			case 'Q':	/* french canadian */
			case 'K':	/* german */
			case 'Y':	/* italian */
			case '6':	/* norwegian/danish */
			/* note: %5 and %6 are not supported (two chars..) */

				ip->escape = 0;
				/* just ignore for now */
				return;

			/* 94-multibyte character sets designate */
			case '$':
				ip->escape = '$';
				return;

			/* locking shift modes */
			case '`':
				ip->GR = &ip->G1;
				ip->escape = 0;
				return;

			case 'n':
				ip->GL = &ip->G2;
				ip->escape = 0;
				return;

			case '}':
				ip->GR = &ip->G2;
				ip->escape = 0;
				return;

			case 'o':
				ip->GL = &ip->G3;
				ip->escape = 0;
				return;

			case '|':
				ip->GR = &ip->G3;
				ip->escape = 0;
				return;

			case '~':
				ip->GR = &ip->G1;
				ip->escape = 0;
				return;

			/* font width/height control */
			case '#':
				ip->escape = '#';
				return;

			case 'c':
				/* hard terminal reset .. */
				ite_reset (ip);
				SUBR_CURSOR(ip, MOVE_CURSOR);
				ip->escape = 0;
				return;


			case '7':
				/* save cursor */
				ip->save_curx = ip->curx;
				ip->save_cury = ip->cury;
				ip->save_attribute = ip->attribute;
				ip->sc_om = ip->inside_margins;
				ip->sc_G0 = ip->G0;
				ip->sc_G1 = ip->G1;
				ip->sc_G2 = ip->G2;
				ip->sc_G3 = ip->G3;
				ip->sc_GL = ip->GL;
				ip->sc_GR = ip->GR;
				ip->escape = 0;
				return;

			case '8':
				/* restore cursor */
				ip->curx = ip->save_curx;
				ip->cury = ip->save_cury;
				ip->attribute = ip->save_attribute;
				ip->inside_margins = ip->sc_om;
				ip->G0 = ip->sc_G0;
				ip->G1 = ip->sc_G1;
				ip->G2 = ip->sc_G2;
				ip->G3 = ip->sc_G3;
				ip->GL = ip->sc_GL;
				ip->GR = ip->sc_GR;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				ip->escape = 0;
				return;

			case '=':
				/* keypad application mode */
				ip->keypad_appmode = 1;
				ip->escape = 0;
				return;

			case '>':
				/* keypad numeric mode */
				ip->keypad_appmode = 0;
				ip->escape = 0;
				return;

			case 'Z':	/* request ID */
				if (ip->emul_level == EMUL_VT100)
					ite_sendstr (ip, "\033[61;0c"); /* XXX not clean */
				else
					ite_sendstr (ip, "\033[63;0c"); /* XXX not clean */
				ip->escape = 0;
				return;

			/* default catch all for not recognized ESC sequences */
			default:
				ip->escape = 0;
				return;
			}
			break;


		case '(': /* designate G0 */
			switch (c) {
			case 'B': /* USASCII */
				ip->G0 = CSET_ASCII;
				ip->escape = 0;
				return;
			case 'I':
				ip->G0 = CSET_JISKANA;
				ip->escape = 0;
				return;
			case 'J':
				ip->G0 = CSET_JISROMA;
				ip->escape = 0;
				return;
			case 'A': /* British or ISO-Latin-1 */
			case 'H': /* Swedish */
			case 'K': /* German */
			case 'R': /* French */
			case 'Y': /* Italian */
			case 'Z': /* Spanish */
			default:
				/* not supported */
				ip->escape = 0;
				return;
			}

		case ')': /* designate G1 */
			ip->escape = 0;
			return;

		case '$': /* 94-multibyte character set */
			switch (c) {
			case '@':
				ip->G0 = CSET_JIS1978;
				ip->escape = 0;
				return;
			case 'B':
				ip->G0 = CSET_JIS1983;
				ip->escape = 0;
				return;
			case 'D':
				ip->G0 = CSET_JIS1990;
				ip->escape = 0;
				return;
			default:
				/* not supported */
				ip->escape = 0;
				return;
			}

		case ' ':
			switch (c) {
			case 'F':
				ip->eightbit_C1 = 0;
				ip->escape = 0;
				return;

			case 'G':
				ip->eightbit_C1 = 1;
				ip->escape = 0;
				return;

			default:
				/* not supported */
				ip->escape = 0;
				return;
			}
			break;

		case '#':
			switch (c) {
			case '5':
				/* single height, single width */
				ip->escape = 0;
				return;

			case '6':
				/* double width, single height */
				ip->escape = 0;
				return;

			case '3':
				/* top half */
				ip->escape = 0;
				return;

			case '4':
				/* bottom half */
				ip->escape = 0;
				return;

			case '8':
				/* screen alignment pattern... */
				alignment_display (ip);
				ip->escape = 0;
				return;

			default:
				ip->escape = 0;
				return;
			}
			break;



		case CSI:
			/* the biggie... */
			switch (c) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case ';': case '\"': case '$': case '>':
				if (ip->ap < ip->argbuf + MAX_ARGSIZE)
					*ip->ap++ = c;
				return;

			case 'p':
				*ip->ap = 0;
				if (!strncmp(ip->argbuf, "61\"", 3))
					ip->emul_level = EMUL_VT100;
				else if (!strncmp(ip->argbuf, "63;1\"", 5)
					 || !strncmp(ip->argbuf, "62;1\"", 5))
					ip->emul_level = EMUL_VT300_7;
				else
					ip->emul_level = EMUL_VT300_8;
				ip->escape = 0;
				return;


			case '?':
				*ip->ap = 0;
				ip->escape = '?';
				ip->ap = ip->argbuf;
				return;


			case 'c':
				/* device attributes */
				*ip->ap = 0;
				if (ip->argbuf[0] == '>') {
					ite_sendstr (ip, "\033[>24;0;0;0c");
				} else
					switch (ite_zargnum(ip)) {
					case 0:
						/* primary DA request, send primary DA response */
						if (ip->emul_level == EMUL_VT100)
							ite_sendstr (ip, "\033[?1;1c");
						else
							ite_sendstr (ip, "\033[63;0c");
						break;
					}
				ip->escape = 0;
				return;

			case 'n':
				switch (ite_zargnum(ip)) {
				case 5:
					ite_sendstr (ip, "\033[0n");	/* no malfunction */
					break;
				case 6:
					/* cursor position report */
					sprintf (ip->argbuf, "\033[%d;%dR", 
						 ip->cury + 1, ip->curx + 1);
					ite_sendstr (ip, ip->argbuf);
					break;
				}
				ip->escape = 0;
				return;


			case 'x':
				switch (ite_zargnum(ip)) {
				case 0:
					/* Fake some terminal parameters.  */
					ite_sendstr (ip, "\033[2;1;1;112;112;1;0x");
					break;
				case 1:
					ite_sendstr (ip, "\033[3;1;1;112;112;1;0x");
					break;
				}
				ip->escape = 0;
				return;


			case 'g':
				/* clear tabs */
				switch (ite_zargnum(ip)) {
				case 0:
					if (ip->curx < ip->cols)
						ip->tabs[ip->curx] = 0;
					break;
				case 3:
					for (n = 0; n < ip->cols; n++)
						ip->tabs[n] = 0;
					break;

				default:
					/* ignore */
					break;
				}
				ip->escape = 0;
				return;


			case 'h': /* set mode */
			case 'l': /* reset mode */
				n = ite_zargnum (ip);
				switch (n) {
				case 4:
					ip->imode = (c == 'h');	/* insert/replace mode */
					break;
				case 20:
					ip->linefeed_newline = (c == 'h');
					break;
				}
				ip->escape = 0;
				return;


			case 'M':
				/* delete line */
				ite_dnline (ip, ite_argnum (ip));
				ip->escape = 0;
				return;


			case 'L':
				/* insert line */
				ite_inline (ip, ite_argnum (ip));
				ip->escape = 0;
				return;


			case 'P':
				/* delete char */
				ite_dnchar (ip, ite_argnum (ip));
				ip->escape = 0;
				return;


			case '@':
				/* insert char(s) */
				ite_inchar (ip, ite_argnum (ip));
				ip->escape = 0;
				return;

			case '!':
				/* soft terminal reset */
				ip->escape = 0; /* XXX */
				return;

			case 'G':
				/* this one was *not* in my vt320 manual but in 
				   a vt320 termcap entry.. who is right?
				   It's supposed to set the horizontal cursor position. */
				*ip->ap = 0;
				x = atoi (ip->argbuf);
				if (x) x--;
				ip->curx = min(x, ip->cols - 1);
				ip->escape = 0;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				clr_attr (ip, ATTR_INV);
				return;


			case 'd':
				/* same thing here, this one's for setting the absolute
				   vertical cursor position. Not documented... */
				*ip->ap = 0;
				y = atoi (ip->argbuf);
				if (y) y--;
				if (ip->inside_margins)
					y += ip->top_margin;
				ip->cury = min(y, ip->rows - 1);
				ip->escape = 0;
				snap_cury(ip);
				SUBR_CURSOR(ip, MOVE_CURSOR);
				clr_attr (ip, ATTR_INV);
				return;


			case 'H':
			case 'f':
				*ip->ap = 0;
				y = atoi (ip->argbuf);
				x = 0;
				cp = index (ip->argbuf, ';');
				if (cp)
					x = atoi (cp + 1);
				if (x) x--;
				if (y) y--;
				if (ip->inside_margins)
					y += ip->top_margin;
				ip->cury = min(y, ip->rows - 1);
				ip->curx = min(x, ip->cols - 1);
				ip->escape = 0;
				snap_cury(ip);
				SUBR_CURSOR(ip, MOVE_CURSOR);
				/*clr_attr (ip, ATTR_INV);*/
				return;

			case 'A':
				/* cursor up */
				n = ite_argnum (ip);
				n = ip->cury - (n ? n : 1);
				if (n < 0) n = 0;
				if (ip->inside_margins)
					n = max(ip->top_margin, n);
				else if (n == ip->top_margin - 1)
					/* allow scrolling outside region, but don't scroll out
					   of active region without explicit CUP */
					n = ip->top_margin;
				ip->cury = n;
				ip->escape = 0;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				clr_attr (ip, ATTR_INV);
				return;

			case 'B':
				/* cursor down */
				n = ite_argnum (ip);
				n = ip->cury + (n ? n : 1);
				n = min(ip->rows - 1, n);
#if 0
				if (ip->inside_margins)
#endif
					n = min(ip->bottom_margin, n);
#if 0
				else if (n == ip->bottom_margin + 1)
					/* allow scrolling outside region, but don't scroll out
					   of active region without explicit CUP */
					n = ip->bottom_margin;
#endif
				ip->cury = n;
				ip->escape = 0;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				clr_attr (ip, ATTR_INV);
				return;

			case 'C':
				/* cursor forward */
				n = ite_argnum (ip);
				n = n ? n : 1;
				ip->curx = min(ip->curx + n, ip->cols - 1);
				ip->escape = 0;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				clr_attr (ip, ATTR_INV);
				return;

			case 'D':
				/* cursor backward */
				n = ite_argnum (ip);
				n = n ? n : 1;
				n = ip->curx - n;
				ip->curx = n >= 0 ? n : 0;
				ip->escape = 0;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				clr_attr (ip, ATTR_INV);
				return;


			case 'J':
				/* erase screen */
				*ip->ap = 0;
				n = ite_zargnum (ip);
				if (n == 0)
					ite_clrtoeos(ip);
				else if (n == 1)
					ite_clrtobos(ip);
				else if (n == 2)
					ite_clrscreen(ip);
				ip->escape = 0;
				return;


			case 'K':
				/* erase line */
				n = ite_zargnum (ip);
				if (n == 0)
					ite_clrtoeol(ip);
				else if (n == 1)
					ite_clrtobol(ip);
				else if (n == 2)
					ite_clrline(ip);
				ip->escape = 0;
				return;

			case 'S':
				/* scroll up */
				n = ite_zargnum (ip);
				if (n <= 0)
					n = 1;
				else if (n > ip->rows-1)
					n = ip->rows-1;
				SUBR_SCROLL(ip, ip->rows-1, 0, n, SCROLL_UP);
				ip->escape = 0;
				return;

			case 'T':
				/* scroll down */
				n = ite_zargnum (ip);
				if (n <= 0)
					n = 1;
				else if (n > ip->rows-1)
					n = ip->rows-1;
				SUBR_SCROLL(ip, 0, 0, n, SCROLL_DOWN);
				ip->escape = 0;
				return;

			case 'X':
				/* erase character */
				n = ite_argnum(ip) - 1;
				n = min(n, ip->cols - 1 - ip->curx);
				for (; n >= 0; n--) {
					attrclr(ip, ip->cury, ip->curx + n, 1, 1);
					SUBR_PUTC(ip, ' ', ip->cury, ip->curx + n, ATTR_NOR);
				}
				ip->escape = 0;
				return;


			case '}': case '`':
				/* status line control */
				ip->escape = 0;
				return;

			case 'r':
				/* set scrolling region */
				ip->escape = 0;
				*ip->ap = 0;
				x = atoi (ip->argbuf);
				x = x ? x : 1;
				y = ip->rows;
				cp = index (ip->argbuf, ';');
				if (cp) {
					y = atoi (cp + 1);
					y = y ? y : ip->rows;
				}
				if (y <= x)
					return;
				x--;
				y--;
				ip->top_margin = min(x, ip->rows - 2);
				ip->bottom_margin = min(y, ip->rows - 1);
				if (ip->inside_margins) {
					ip->cury = ip->top_margin;
				} else
					ip->cury = 0;
				ip->curx = 0;
				return;


			case 'm':
				/* big attribute setter/resetter */
			{
				char *cp;
				*ip->ap = 0;
				/* kludge to make CSIm work (== CSI0m) */
				if (ip->ap == ip->argbuf)
					ip->ap++;
				for (cp = ip->argbuf; cp < ip->ap; ) {
					switch (*cp) {
					case 0:
					case '0':
						clr_attr (ip, ATTR_ALL);
						ip->fgcolor = 7;
						ip->bgcolor = 0;
						cp++;
						break;

					case '1':
						set_attr (ip, ATTR_BOLD);
						cp++;
						break;

					case '2':
						switch (cp[1]) {
						case '2':
							clr_attr (ip, ATTR_BOLD);
							cp += 2;
							break;

						case '4':
							clr_attr (ip, ATTR_UL);
							cp += 2;
							break;

						case '5':
							clr_attr (ip, ATTR_BLINK);
							cp += 2;
							break;

						case '7':
							clr_attr (ip, ATTR_INV);
							cp += 2;
							break;

						default:
							cp++;
							break;
						}
						break;

					case '3':
						switch (cp[1]) {
						case '0': case '1': case '2': case '3':
						case '4': case '5': case '6': case '7':
							/* foreground colors */
							ip->fgcolor = cp[1] - '0';
							cp += 2;
							break;
						default:
							cp++;
							break;
						}
						break;

					case '4':
						switch (cp[1]) {
						case '0': case '1': case '2': case '3':
						case '4': case '5': case '6': case '7':
							/* background colors */
							ip->bgcolor = cp[1] - '0';
							cp += 2;
							break;
						default:
							set_attr (ip, ATTR_UL);
							cp++;
							break;
						}
						break;

					case '5':
						set_attr (ip, ATTR_BLINK);
						cp++;
						break;

					case '7':
						set_attr (ip, ATTR_INV);
						cp++;
						break;

					default:
						cp++;
						break;
					}
				}

			}
				ip->escape = 0;
				return;


			case 'u':
				/* DECRQTSR */
				ite_sendstr (ip, "\033P\033\\");
				ip->escape = 0;
				return;

			default:
				ip->escape = 0;
				return;
			}
			break;



		case '?':	/* CSI ? */
			switch (c) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case ';': case '\"': case '$':
				/* Don't fill the last character; it's needed.  */
				/* XXX yeah, where ?? */
				if (ip->ap < ip->argbuf + MAX_ARGSIZE - 1)
					*ip->ap++ = c;
				return;


			case 'n':
				/* Terminal Reports */
				*ip->ap = 0;
				if (ip->ap == &ip->argbuf[2]) {
					if (!strncmp(ip->argbuf, "15", 2))
						/* printer status: no printer */
						ite_sendstr (ip, "\033[13n");

					else if (!strncmp(ip->argbuf, "25", 2))
						/* udk status */
						ite_sendstr (ip, "\033[20n");

					else if (!strncmp(ip->argbuf, "26", 2))
						/* keyboard dialect: US */
						ite_sendstr (ip, "\033[27;1n");
				}
				ip->escape = 0;
				return;


			case 'h': /* set dec private modes */
			case 'l': /* reset dec private modes */
				n = ite_zargnum (ip);
				switch (n) {
				case 1:
					/* CKM - cursor key mode */
					ip->cursor_appmode = (c == 'h');
					break;

				case 3:
					/* 132/80 columns (132 == 'h') */
					break;

				case 4: /* smooth scroll */
					break;

				case 5:
					/* light background (=='h') /dark background(=='l') */
					break;

				case 6: /* origin mode */
					ip->inside_margins = (c == 'h');
#if 0
					ip->curx = 0;
					ip->cury = ip->inside_margins ? ip->top_margin : 0;
					SUBR_CURSOR(ip, MOVE_CURSOR);
#endif
					break;

				case 7: /* auto wraparound */
					ip->auto_wrap = (c == 'h');
					break;

				case 8: /* keyboard repeat */
					ip->key_repeat = (c == 'h');
					break;

				case 20: /* newline mode */
					ip->linefeed_newline = (c == 'h');
					break;

				case 25: /* cursor on/off */
					SUBR_CURSOR(ip, (c == 'h') ? DRAW_CURSOR : ERASE_CURSOR);
					break;
				}
				ip->escape = 0;
				return;

			case 'K':
				/* selective erase in line */
			case 'J':
				/* selective erase in display */

			default:
				ip->escape = 0;
				return;
			}
			break;


		default:
			ip->escape = 0;
			return;
		}
	}

	switch (c) {
	case 0x00:	/* NUL */
	case 0x01:	/* SOH */
	case 0x02:	/* STX */
	case 0x03:	/* ETX */
	case 0x04:	/* EOT */
	case 0x05:	/* ENQ */
	case 0x06:	/* ACK */
		break;

	case BEL:
#if NBELL > 0
		if (kbd_ite && ite_tty[kbd_ite->device.dv_unit])
			opm_bell();
#endif
		break;

	case BS:
		if (--ip->curx < 0)
			ip->curx = 0;
		else
			SUBR_CURSOR(ip, MOVE_CURSOR);
		break;

	case HT:
		for (n = ip->curx + 1; n < ip->cols; n++) {
			if (ip->tabs[n]) {
				ip->curx = n;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				break;
			}
		}
		break;

	case VT:	/* VT is treated like LF */
	case FF:	/* so is FF */
	case LF:
		/* cr->crlf distinction is done here, on output, 
		   not on input! */
		if (ip->linefeed_newline)
			ite_crlf (ip);
		else
			ite_lf (ip);
		break;

	case CR:
		ite_cr (ip);
		break;


	case SO:
		ip->GL = &ip->G1;
		break;
		
	case SI:
		ip->GL = &ip->G0;
		break;

	case 0x10:	/* DLE */
	case 0x11:	/* DC1/XON */
	case 0x12:	/* DC2 */
	case 0x13:	/* DC3/XOFF */
	case 0x14:	/* DC4 */
	case 0x15:	/* NAK */
	case 0x16:	/* SYN */
	case 0x17:	/* ETB */		
		break;

	case CAN:
		ip->escape = 0;	/* cancel any escape sequence in progress */
		break;
		
	case 0x19:	/* EM */
		break;
					
	case SUB:
		ip->escape = 0;	/* dito, but see below */
		/* should also display a reverse question mark!! */
		break;

	case ESC:
		ip->escape = ESC;
		break;

	case 0x1c:	/* FS */
	case 0x1d:	/* GS */
	case 0x1e:	/* RS */
	case 0x1f:	/* US */
		break;

	/* now it gets weird.. 8bit control sequences.. */
	case IND:	/* index: move cursor down, scroll */
		ite_index (ip);
		break;
		
	case NEL:	/* next line. next line, first pos. */
		ite_crlf (ip);
		break;

	case HTS:	/* set horizontal tab */
		if (ip->curx < ip->cols)
			ip->tabs[ip->curx] = 1;
		break;
		
	case RI:	/* reverse index */
		ite_rlf (ip);
		break;

	case SS2:	/* go into G2 for one character */
		ip->save_GL = ip->GR;	/* GL XXX EUC */
		ip->GR = &ip->G2;	/* GL XXX */
		break;
		
	case SS3:	/* go into G3 for one character */
		ip->save_GL = ip->GR;	/* GL XXX EUC */
		ip->GR = &ip->G3;	/* GL XXX */
		break;
		
	case DCS:	/* device control string introducer */
		ip->escape = DCS;
		ip->ap = ip->argbuf;
		break;
		
	case CSI:	/* control sequence introducer */
		ip->escape = CSI;
		ip->ap = ip->argbuf;
		break;
		
	case ST:	/* string terminator */
		/* ignore, if not used as terminator */
		break;
		
	case OSC:	/* introduces OS command. Ignore everything upto ST */
		ip->escape = OSC;
		break;

	case PM:	/* privacy message, ignore everything upto ST */
		ip->escape = PM;
		break;
		
	case APC:	/* application program command, ignore everything upto ST */
		ip->escape = APC;
		break;

	case DEL:
		break;

	default:
		if (!ip->save_char && (*((c & 0x80) ? ip->GR : ip->GL) & CSET_MULTI)) {
			ip->save_char = c;
			break;
		}
		if (ip->imode)
			ite_inchar(ip, ip->save_char ? 2 : 1);
		iteprecheckwrap(ip);
#ifdef DO_WEIRD_ATTRIBUTES
		if ((ip->attribute & ATTR_INV) || attrtest(ip, ATTR_INV)) {
			attrset(ip, ATTR_INV);
			SUBR_PUTC(ip, c, ip->cury, ip->curx, ATTR_INV);
		}
		else
			SUBR_PUTC(ip, c, ip->cury, ip->curx, ATTR_NOR);
#else
		SUBR_PUTC(ip, c, ip->cury, ip->curx, ip->attribute);
#endif
/*		SUBR_CURSOR(ip, DRAW_CURSOR);*/
		itecheckwrap(ip);
		if (ip->save_char) {
			itecheckwrap(ip);
			ip->save_char = 0;
		}
		if (ip->save_GL) {
			/*
			 * reset single shift
			 */
			ip->GR = ip->save_GL;
			ip->save_GL = 0;
		}
		break;
	}
}

static void
iteprecheckwrap(ip)
	struct ite_softc *ip;
{
	if (ip->auto_wrap && ip->curx + (ip->save_char ? 1 : 0) == ip->cols) {
		ip->curx = 0;
		clr_attr(ip, ATTR_INV);
		if (++ip->cury >= ip->bottom_margin + 1) {
			ip->cury = ip->bottom_margin;
			/*SUBR_CURSOR(ip, MOVE_CURSOR);*/
			SUBR_SCROLL(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip);
		} /*else
			SUBR_CURSOR(ip, MOVE_CURSOR);*/
	}
}

static void
itecheckwrap(ip)
     struct ite_softc *ip;
{
#if 0
	if (++ip->curx == ip->cols) {
		if (ip->auto_wrap) {
			ip->curx = 0;
			clr_attr(ip, ATTR_INV);
			if (++ip->cury >= ip->bottom_margin + 1) {
				ip->cury = ip->bottom_margin;
				SUBR_CURSOR(ip, MOVE_CURSOR);
				SUBR_SCROLL(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
				ite_clrtoeol(ip);
				return;
			}
		} else
			/* stay there if no autowrap.. */
			ip->curx--;
	}
#else
	if (ip->curx < ip->cols) {
		ip->curx++;
		/*SUBR_CURSOR(ip, MOVE_CURSOR);*/
	}
#endif
}

#endif

#if NITE > 0 && NKBD > 0

/*
 * Console functions
 */
#include <dev/cons.h>
extern void kbdenable __P((int));
extern int kbdcngetc __P((void));

/*
 * Return a priority in consdev->cn_pri field highest wins.  This function
 * is called before any devices have been probed.
 */
void
itecnprobe(cd)
	struct consdev *cd;
{
	int maj;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == iteopen)
			break;

	/* 
	 * return priority of the best ite (already picked from attach)
	 * or CN_DEAD.
	 */
	if (con_itesoftc.grf == NULL)
		cd->cn_pri = CN_DEAD;
	else {
		con_itesoftc.flags = (ITE_ALIVE|ITE_CONSOLE);
		/*
		 * hardcode the minor number.
		 * currently we support only one ITE, it is enough for now.
		 */
		con_itesoftc.isw = &itesw[0];
		cd->cn_pri = CN_INTERNAL;
		cd->cn_dev = makedev(maj, 0);
	}

}

void
itecninit(cd)
	struct consdev *cd;
{
	struct ite_softc *ip;

	ip = getitesp(cd->cn_dev);
	iteinit(cd->cn_dev);	       /* init console unit */
	ip->flags |= ITE_ACTIVE | ITE_ISCONS;
	kbdenable(0);
	mfp_send_usart(0x49);
}

/*
 * itecnfinish() is called in ite_init() when the device is
 * being probed in the normal fasion, thus we can finish setting
 * up this ite now that the system is more functional.
 */
void
itecnfinish(ip)
	struct ite_softc *ip;
{
	static int done;

	if (done)
		return;
	done = 1;
}

/*ARGSUSED*/
int
itecngetc(dev)
	dev_t dev;
{
	register int c;

	do {
		c = kbdcngetc();
		c = ite_cnfilter(c);
	} while (c == -1);
	return (c);
}

void
itecnputc(dev, c)
	dev_t dev;
	int c;
{
	static int paniced = 0;
	struct ite_softc *ip = getitesp(dev);
	char ch = c;
#ifdef ITE_KERNEL_ATTR
	short save_attribute;
#endif
	
	if (panicstr && !paniced &&
	    (ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE) {
		(void) iteon(dev, 3);
		paniced = 1;
	}
#ifdef ITE_KERNEL_ATTR
	save_attribute = ip->attribute;
	ip->attribute = ITE_KERNEL_ATTR;
#endif
	ite_putstr(&ch, 1, dev);
#ifdef ITE_KERNEL_ATTR
	ip->attribute = save_attribute;
#endif
}
#endif
