/*-
 * Copyright (c) 1993 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	from: @(#)pccons.c	5.11 (Berkeley) 5/21/91
 *	$Id: pccons.c,v 1.31.2.12 1993/10/17 14:04:28 mycroft Exp $
 */

/*
 * code to work keyboard & display for PC-style console
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/pc/display.h>
#include <machine/pccons.h>

#include <i386/i386/cons.h>
#include <i386/isa/isa.h>
#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/kbdreg.h>

#include "pc.h"

#ifndef BEEP_FREQ
#define BEEP_FREQ 1500
#endif
#ifndef BEEP_TIME
#define BEEP_TIME (hz/5)
#endif

#define MONO_BASE	0x3B4
#define MONO_BUF	0xB0000
#define CGA_BASE	0x3D4
#define CGA_BUF		0xB8000

#define	PCUNIT(dev)	(minor(dev))

#define PCBURST 128

struct	pc_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;
};

/*
 * These things really ought to be in pc_softc, but the console code
 * really screws us unless we want to do an awful lot of extra work.
 */
struct	pc_state {
	u_short	ps_iobase;
	u_char	ps_flags;		/* long-term state */
#define	PSF_COLOR	0x1		/* color display? */
#define	PSF_STAND	0x2		/* standout mode? */
#define	PSF_RAW		0x4		/* read raw scan codes */
	u_char	ps_state;		/* parser state */
#define	PSS_ESCAPE	0x1		/* seen escape */
#define	PSS_EBRACE	0x2		/* seen escape bracket */
#define	PSS_EPARAM	0x4		/* seen escape and parameters */
	u_char	ps_at;			/* normal attributes */
	u_char	ps_sat;			/* standout attributes */
	int 	ps_cx, ps_cy;		/* escape parameters */
	int 	ps_col;			/* current cursor position */
}	pc_state[NPC];

struct	tty *pc_tty[NPC];

static int pcprobe __P((struct device *, struct cfdata *, void *));
static void pcforceintr __P((void *));
static void pcattach __P((struct device *, struct device *, void *));
static int pcintr __P((struct pc_softc *));

struct	cfdriver pccd =
{ NULL, "pc", pcprobe, pcattach, DV_TTY, sizeof(struct pc_softc) };

/* block cursor so wfj does not go blind on laptop hunting for
	the verdamnt cursor -wfj */
#define	FAT_CURSOR		/* XXX */

#define	COL		80	/* XXX */
#define	ROW		25	/* XXX */
#define	CHR		2

/* DANGER WIL ROBINSON -- the values of SCROLL, NUM, and CAPS are important. */
#define	SCROLL		0x0001	/* stop output */
#define	NUM		0x0002	/* numeric shift  cursors vs. numeric */
#define	CAPS		0x0004	/* caps shift -- swaps case of letter */
#define	SHIFT		0x0008	/* keyboard shift */
#define	CTL		0x0010	/* control shift  -- allows CTL function */
#define	ASCII		0x0040	/* ascii code for this key */
#define	ALT		0x0080	/* alternate shift -- alternate chars */
#define	FUNC		0x0100	/* function key */
#define	KP		0x0200	/* Keypad keys */
#define	NONE		0x0400	/* no function */

static u_short *Crtat;		/* pointer to backing store */
static u_short *crtat;		/* pointer to current char */
static u_char ack, nak;		/* Don't ask. */

int pcopen __P((dev_t, int, int, struct proc *));
int pcclose __P((dev_t, int));

static void sput __P((struct pc_state *, u_char *, int, int));
static char *sget __P((struct pc_state *, int));
static void cursor __P((struct pc_state *));
static void update_led __P((void));
static void set_typematic __P((u_char));
static void pc_xmode_on __P((struct pc_state *));
static void pc_xmode_off __P((struct pc_state *));
static void pcstart __P((struct tty *));
static int pcparam __P((struct tty *, struct termios *));

static inline int
kbd_wait()
{
	u_int i;

	for (i = 0x10000; i; i--)
		if ((inb(KBSTATP) & KBS_IBF) == 0)
			return 1;

	return 0;
}

/*
 * Pass command to keyboard controller (8042)
 */
static int
kbc_8042cmd(val)
	u_char val;
{

	if (!kbd_wait())
		return 0;
	outb(KBCMDP, val);
	return 1;
}

/*
 * Pass command to keyboard itself
 */
int
kbd_cmd(val, polling)
	u_char val;
	u_char polling;
{
	u_int retries = 3;
	register u_int i;

	do {
		if (!kbd_wait())
			return 0;
		ack = nak = 0;
		outb(KBOUTP, val);
		for (i = 0x10000; i; i--) {
			if (polling && inb(KBSTATP) & KBS_DIB) {
				register u_char c;
				c = inb(KBDATAP);
				if (c == KBR_ACK)
					ack = 1;
				else if (c == KBR_RESEND)
					nak = 1;
			}
			if (ack)
				return 1;
			if (nak)
				break;
		}
		if (!nak)
			return 0;
	} while (--retries);
	return 0;
}

static int
_pcprobe(ps)
	struct pc_state *ps;
{
	u_short volatile *cp;
	u_short was;
	u_short iobase;
	u_int cursorat;

	/*
	 * Set Crtat to MONO_BUF if it exists; otherwise use CGA_BUF.
	 */

	cp = (u_short volatile *)ISA_HOLE_VADDR(MONO_BUF);
	was = *cp;
	*cp = (u_short) 0xA55A;
	if (*cp == 0xA55A) {
		ps->ps_iobase = iobase = MONO_BASE;
		ps->ps_flags &= ~PSF_COLOR;
		Crtat = (u_short *)cp;
		goto found;
	}

	cp = (u_short volatile *)ISA_HOLE_VADDR(CGA_BUF);
	was = *cp;
	*cp = (u_short) 0xA55A;
	if (*cp == 0xA55A) {
		ps->ps_iobase = iobase = CGA_BASE;
		ps->ps_flags |= PSF_COLOR;
		Crtat = (u_short *)cp;
		goto found;
	}

	return 0;

    found:
	/* Extract cursor location */
	outb(iobase, 14);
	cursorat = inb(iobase + 1) << 8;
	outb(iobase, 15);
	cursorat |= inb(iobase + 1);
	crtat = Crtat + cursorat;

#ifdef	FAT_CURSOR
	outb(iobase, 10);
	outb(iobase + 1, 0);
	outb(iobase, 11);
	outb(iobase + 1, 18);
#endif	FAT_CURSOR

	return 1;
}

static int
pcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct pc_state *ps = &pc_state[cf->cf_unit];

	if (cf->cf_unit != 0)
		panic("pcprobe: unit != 0");

	/* Enable interrupts and keyboard, etc. */
	if (!kbc_8042cmd(K_LDCMDBYTE)) {
#ifdef DIAGNOSTIC
		printf("pc: command error 1\n");
#endif
		return 0;
	}
	if (!kbd_cmd(CMDBYTE, 1)) {
#ifdef DIAGNOSTIC
		printf("pc: command error 2\n");
#endif
		return 0;
	}

	/* Start keyboard reset */
	if (!kbd_cmd(KBC_RESET, 1)) {
#ifdef DIAGNOSTIC
		printf("pc: reset error 1\n");
#endif
		return 0;
	}

	while ((inb(KBSTATP) & KBS_DIB) == 0);
	if (inb(KBDATAP) != KBR_RSTDONE) {
#ifdef DIAGNOSTIC
		printf("pc: reset error 2\n");
#endif
		return 0;
	}

	if (!_pcprobe(ps))
		return 0;

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(pcforceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;
	}

	ia->ia_iobase = ps->ps_iobase;
	ia->ia_iosize = 16;
	ia->ia_maddr = ISA_PHYSADDR(Crtat);
	ia->ia_msize = COL * ROW * CHR;
	ia->ia_drq = DRQUNK;
	return 1;
}

static void
pcforceintr(aux)
	void *aux;
{

	(void) kbd_cmd(KBC_ECHO, 1);
}

static void
pcattach(self, parent, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pc_softc *sc = (struct pc_softc *)self;
	struct pc_state *ps = &pc_state[sc->sc_dev.dv_unit];
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;
	u_int cursorat;

	if (ps->ps_flags & PSF_COLOR)
		printf(": color\n");
	else
		printf(": mono\n");
	isa_establish(&sc->sc_id, &sc->sc_dev);

	ps->ps_at = FG_LIGHTGREY | BG_BLACK;
	if (ps->ps_flags & PSF_COLOR)
		ps->ps_sat = FG_YELLOW | BG_BLACK;
	else
		ps->ps_sat = FG_BLACK | BG_LIGHTGREY;

	fillw((ps->ps_at << 8) | ' ', crtat,
	      (Crtat + COL * ROW) - crtat);

	cursor(ps);

	sc->sc_ih.ih_fun = pcintr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_TTY);
}

int
pcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = PCUNIT(dev);
	struct pc_softc *sc;
	struct tty *tp;

	if (unit >= pccd.cd_ndevs)
		return ENXIO;
	sc = pccd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (!pc_tty[unit])
		tp = pc_tty[unit] = ttymalloc();
	else
		tp = pc_tty[unit];

	tp->t_oproc = pcstart;
	tp->t_param = pcparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_WOPEN;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		pcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return EBUSY;
	tp->t_state |= TS_CARR_ON;

	return (*linesw[tp->t_line].l_open)(dev, tp);
}

int
pcclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp = pc_tty[PCUNIT(dev)];

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
#ifdef notyet /* XXXX */
	ttyfree(tp);
#endif
	return 0;
}

int
pcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = pc_tty[PCUNIT(dev)];

	return (*linesw[tp->t_line].l_read)(tp, uio, flag);
}

int
pcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = pc_tty[PCUNIT(dev)];

	return (*linesw[tp->t_line].l_write)(tp, uio, flag);
}

/*
 * Got a console receive interrupt -
 * the console processor wants to give us a character.
 * Catch the character, and see who it goes to.
 */
static int
pcintr(sc)
	struct pc_softc *sc;
{
	int unit = sc->sc_dev.dv_unit;
	struct tty *tp = pc_tty[unit];
	u_char *cp;

	cp = sget(&pc_state[unit], 1);
	if ((tp->t_state & TS_ISOPEN) == 0)
		return 0;
	if (cp)
		do
			(*linesw[tp->t_line].l_rint)(*cp++, tp);
		while (*cp);
	return 1;
}

int
pcioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	int unit = PCUNIT(dev);
	struct tty *tp = pc_tty[unit];
	register error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0)
		return error;

	switch (cmd) {
	    case CONSOLE_X_MODE_ON:
		pc_xmode_on(&pc_state[unit]);
		break;
	    case CONSOLE_X_MODE_OFF:
		pc_xmode_off(&pc_state[unit]);
		break;
	    case CONSOLE_X_BELL:
		/*
		 * If set, data is a pointer to a length 2 array of
		 * integers.  data[0] is the pitch in Hz and data[1]
		 * is the duration in msec.  Otherwise use defaults.
		 */
		if (data)
			sysbeep(((int*)data)[0],
				(((int*)data)[1] * hz) / 1000);
		else
			sysbeep(BEEP_FREQ, BEEP_TIME);
		break;
	    case CONSOLE_SET_TYPEMATIC_RATE: {
		u_char	rate;

		if (!data)
			return EINVAL;
		rate = *((u_char *)data);
		/*
		 * Check that it isn't too big (which would cause it to be
		 * confused with a command).
		 */
		if (rate & 0x80)
			return EINVAL;
		set_typematic(rate);
		break;
	    }
	    default:
		return ENOTTY;
	}
 
	return 0;
}

static void
pcstart(tp)
	struct tty *tp;
{
	struct pc_state *ps = &pc_state[PCUNIT(tp->t_dev)];
	struct clist *cl;
	int s, len, n;
	u_char buf[PCBURST];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	splx(s);
	/*
	 * We need to do this outside spl since it could be fairly
	 * expensive and we don't want our serial ports to overflow.
	 * The tty is locked, so it shouldn't get corrupted.
	 */
	cl = &tp->t_outq;
	len = q_to_b(cl, buf, PCBURST);
	sput(ps, buf, len, 0);
	s = spltty();
	tp->t_state &=~ TS_BUSY;
	if (cl->c_cc) {
		tp->t_state |= TS_TIMEOUT;
		timeout((timeout_t)ttrstrt, (caddr_t)tp, 1);
	}
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &=~ TS_ASLEEP;
			wakeup((caddr_t)cl);
		}
		selwakeup(&tp->t_wsel);
	}
    out:
	splx(s);
}

/*
 * Set line parameters
 */
static int
pcparam(tp, t)
	struct tty *tp;
	struct termios *t;
{

        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = t->c_cflag;
	return 0;
}

/*
 * cursor():
 *   reassigns cursor position, updated by the rescheduling clock
 *   which is a index (0-1999) into the text area. Note that the
 *   cursor is a "foreground" character, it's color determined by
 *   the fg_at attribute. Thus if fg_at is left as 0, (FG_BLACK),
 *   as when a portion of screen memory is 0, the cursor may dissappear.
 */
static void
docursor(ps)
	struct pc_state *ps;
{
 	int pos = crtat - Crtat;
	u_short iobase = ps->ps_iobase;

	outb(iobase, 14);
	outb(iobase + 1, pos>>8);
	outb(iobase, 15);
	outb(iobase + 1, pos);
}

static void
cursor(ps)
	struct pc_state *ps;
{

	docursor(ps);
	timeout((timeout_t)cursor, (caddr_t)ps, hz/10);
}

static	u_char lock_state;

#define	wrtchar(c, at) do { \
	char *cp = (char *)crtat; *cp++ = (c); *cp = (at); crtat++; ps->ps_col++; \
} while (0)


/* translate ANSI color codes to standard pc ones */
static char fgansitopc[] =
{	FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
	FG_MAGENTA, FG_CYAN, FG_LIGHTGREY};

static char bgansitopc[] =
{	BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
	BG_MAGENTA, BG_CYAN, BG_LIGHTGREY};

/*
 *   sput has support for emulation of the 'pc3' termcap entry.
 *   if ka, use kernel attributes.
 */
static void
sput(ps, cp, n, kernel)
	register struct pc_state *ps;
	u_char *cp;
	int n;
	int kernel;
{
#define	state	ps->ps_state
#define	at	ps->ps_at
	u_char	c, scroll;

	if (ps->ps_flags & PSF_RAW)		/* XXX */
		return;

	while (n--) {
		c = *cp++;
		scroll = 1;	/* do scroll check */

		switch (c) {
		    case 0x1B:
			if (state & PSS_ESCAPE) {
				wrtchar(c, ps->ps_sat);
				state = 0;
			} else
				state = PSS_ESCAPE;
			break;

		    case '\t': {
			    int col = ps->ps_col,
			    	inccol = (8 - col % 8);
			    crtat += inccol;
			    ps->ps_col = col + inccol;
			    break;
		    }

		    case '\010':
			--crtat;
			if (--ps->ps_col < 0)
				ps->ps_col += COL;	/* non-destructive backspace */
			break;

		    case '\r':
			crtat -= ps->ps_col;
			ps->ps_col = 0;
			break;

		    case '\n':
			crtat += COL;
			break;

		    default:
		    bypass:
			if (state & PSS_ESCAPE) {
				scroll = 0;
				if (state & PSS_EBRACE) {
					switch(c) {
						int pos;
					    case 'm':
						if (!ps->ps_cx)
							ps->ps_flags &=~ PSF_STAND;
						else
							ps->ps_flags |= PSF_STAND;
						state = 0;
						break;
					    case 'A': { /* up cx rows */
						int	cx = ps->ps_cx;
						if (cx <= 0)
							cx = 1;
						else
							cx %= ROW;
						pos = crtat - Crtat;
						pos -= COL * cx;
						if (pos < 0)
							pos += ROW * COL;
						crtat = Crtat + pos;
						state = 0;
						break;
					    }
					    case 'B': { /* down cx rows */
						int	cx = ps->ps_cx;
						if (cx <= 0)
							cx = 1;
						else
							cx %= ROW;
						pos = crtat - Crtat;
						pos += COL * cx;
						if (pos >= ROW * COL)
							pos -= ROW * COL;
						crtat = Crtat + pos;
						state = 0;
						break;
					    }
					    case 'C': { /* right cursor */
						int	cx = ps->ps_cx,
							col = ps->ps_col;
						if (cx <= 0)
							cx = 1;
						else
							cx %= COL;
						pos = crtat - Crtat;
						pos += cx;
						col += cx;
						if (col >= COL) {
							pos -= COL;
							col -= COL;
						}
						ps->ps_col = col;
						crtat = Crtat + pos;
						state = 0;
						break;
					    }
					    case 'D': { /* left cursor */
						int	cx = ps->ps_cx,
							col = ps->ps_col;
						if (cx <= 0)
							cx = 1;
						else
							cx %= COL;
						pos = crtat - Crtat;
						pos -= cx;
						col -= cx;
						if (col < 0) {
							pos += COL;
							col += COL;
						}
						ps->ps_col = col;
						crtat = Crtat + pos;
						state = 0;
						break;
					    }
					    case 'J': /* Clear ... */
						switch (ps->ps_cx) {
						    case 0:
							/* ... to end of display */
							fillw((at << 8) | ' ',
								crtat,
								Crtat + COL * ROW - crtat);
							break;
						    case 1:
							/* ... to next location */
							fillw((at << 8) | ' ',
								Crtat,
								crtat - Crtat + 1);
							break;
						    case 2:
							/* ... whole display */
							fillw((at << 8) | ' ',
								Crtat, COL * ROW);
							break;
						}
						state = 0;
						break;
					    case 'K': /* Clear line ... */
						switch (ps->ps_cx) {
						    case 0:
							/* ... current to EOL */
							fillw((at << 8) | ' ',
								crtat, COL - ps->ps_col);
							break;
						    case 1: {
							int	col = ps->ps_col;
							/* ... beginning to next */
							fillw((at << 8) | ' ',
								crtat - col,
								col + 1);
							break;
						    }
						    case 2:
							/* ... entire line */
							fillw((at << 8) | ' ',
								crtat - ps->ps_col, COL);
							break;
						}
						state = 0;
						break;
					    case 'f': /* in system V consoles */
					    case 'H': { /* Cursor move */
						int	cx = ps->ps_cx,
							cy = ps->ps_cy;
						if (!cx || !cy) {
							crtat = Crtat;
							ps->ps_col = 0;
						} else {
							if (cx > ROW)
								cx = ROW;
							if (cy > COL)
								cy = COL;
							crtat = Crtat + (cx - 1) * COL + cy - 1;
							ps->ps_col = cy - 1;
						}
						state = 0;
						break;
					    }
					    case 'S': { /* scroll up cx lines */
						int	cx = ps->ps_cx;
						if (cx <= 0)
							cx = 1;
						if (cx > ROW)
							cx = ROW;
						if (cx < ROW)
							bcopy(Crtat+COL*cx, Crtat,
							      COL*(ROW-cx)*CHR);
						fillw((at << 8) | ' ', Crtat+COL*(ROW-cx), COL*cx);
						/* crtat -= COL*cx; /* XXX */
						state = 0;
						break;
					    }
					    case 'T': { /* scroll down cx lines */
						int	cx = ps->ps_cx;
						if (cx <= 0)
							cx = 1;
						if (cx > ROW)
							cx = ROW;
						if (cx < ROW)
							bcopy(Crtat, Crtat+COL*cx,
							      COL*(ROW-cx)*CHR);
						fillw((at << 8) | ' ', Crtat, COL*cx);
						/* crtat += COL*cx; /* XXX */
						state = 0;
						break;
					    }
					    case ';': /* Switch params in cursor def */
						state = state | PSS_EPARAM;
						break;
					    case 'r':
						ps->ps_sat = (ps->ps_cx & FG_MASK) |
							((ps->ps_cy << 4) & BG_MASK);
						state = 0;
						break;
					    case 'x': /* set attributes */
						switch (ps->ps_cx) {
						    case 0:
							/* reset to normal attributes */
							at = FG_LIGHTGREY | BG_BLACK;
						    case 1:
							/* ansi background */
							if (ps->ps_flags & PSF_COLOR) {
								at &= FG_MASK;
								at |= bgansitopc[ps->ps_cy & 7];
							}
							break;
						    case 2:
							/* ansi foreground */
							if (ps->ps_flags & PSF_COLOR) {
								at &= BG_MASK;
								at |= fgansitopc[ps->ps_cy & 7];
							}
							break;
						    case 3:
							/* pc text attribute */
							if (state & PSS_EPARAM)
								at = ps->ps_cy;
							break;
						}
						ps->ps_at = at;
						state = 0;
						break;

					    default: /* Only numbers valid here */
						if ((c >= '0') && (c <= '9')) {
							if (state & PSS_EPARAM) {
								ps->ps_cy *= 10;
								ps->ps_cy += c - '0';
							} else {
								ps->ps_cx *= 10;
								ps->ps_cx += c - '0';
							}
						} else
							state = 0;
						break;
					}
				} else if (c == 'c') { /* Clear screen & home */
					fillw((at << 8) | ' ', Crtat, COL * ROW);
					crtat = Crtat;
					ps->ps_col = 0;
					state = 0;
				} else if (c == '[') { /* Start ESC [ sequence */
					ps->ps_cx = ps->ps_cy = 0;
					state = state | PSS_EBRACE;
				} else { /* Invalid, clear state */
					wrtchar(c, ps->ps_sat); 
					state = 0;
				}
			} else {
				if (c == 7) {
					sysbeep(BEEP_FREQ, BEEP_TIME);
					scroll = 0;
				} else {
					if (ps->ps_flags & PSF_STAND)
						wrtchar(c, ps->ps_sat);
					else
						wrtchar(c, at); 
					if (ps->ps_col >= COL)
						ps->ps_col = 0;
					break;
				}
			}
		}
		if (scroll) {
			if (crtat >= Crtat + COL * ROW) { /* scroll check */
				if (!kernel) {
					int s = spltty();
					if (lock_state & SCROLL)
						tsleep((caddr_t)&lock_state, PUSER,
						       "pcputc", 0);
					splx(s);
				}
				bcopy(Crtat + COL, Crtat, COL*(ROW-1)*CHR);
				fillw((at << 8) | ' ', Crtat + COL*(ROW-1), COL);
				crtat -= COL;
			}
		}
	}
#undef at
#undef state
}


unsigned	__debug = 0; /*0xffe */
static char scantokey[] = {
0,
120,	/* F9 */
0,
116,	/* F5 */
114,	/* F3 */
112,	/* F1 */
113,	/* F2 */
123,	/* F12 */
0,
121,	/* F10 */
119,	/* F8 */
117,	/* F6 */
115,	/* F4 */
16,	/* TAB */
1,	/* ` */
0,
0,
60,	/* ALT (left) */
44,	/* SHIFT (left) */
0,
58,	/* CTL (left) */
17,	/* Q */
2,	/* 1 */
0,
0,
0,
46,	/* Z */
32,	/* S */
31,	/* A */
18,	/* W */
3,	/* 2 */
0,
0,
48,	/* C */
47,	/* X */
33,	/* D */
19,	/* E */
5,	/* 4 */
4,	/* 3 */
0,
0,
61,	/* SPACE */
49,	/* V */
34,	/* F */
21,	/* T */
20,	/* R */
6,	/* 5 */
0,
0,
51,	/* N */
50,	/* B */
36,	/* H */
35,	/* G */
22,	/* Y */
7,	/* 6 */
0,
0,
0,
52,	/* M */
37,	/* J */
23,	/* U */
8,	/* 7 */
9,	/* 8 */
0,
0,
53,	/* , */
38,	/* K */
24,	/* I */
25,	/* O */
11,	/* 0 */
10,	/* 9 */
0,
0,
54,	/* . */
55,	/* / */
39,	/* L */
40,	/* ; */
26,	/* P */
12,	/* - */
0,
0,
0,
41,	/* " */
0,
27,	/* [ */
13,	/* + */
0,
0,
0,
57,	/* SHIFT (right) */
43,	/* ENTER */
28,	/* ] */
0,
29,	/* \ */
0,
0,
0,
45,	/* na*/
0,
0,
0,
0,
15,	/* backspace */
0,
0,				/* keypad */
93,	/* 1 */
0,
92,	/* 4 */
91,	/* 7 */
0,
0,
0,
99,	/* 0 */
104,	/* . */
98,	/* 2 */
97,	/* 5 */
102,	/* 6 */
96,	/* 8 */
110,	/* ESC */
90,	/* Num Lock */
122,	/* F11 */
106,	/* + */
103,	/* 3 */
105,	/* - */
100,	/* * */
101,	/* 9 */
0,
0,
0,
0,
0,
118,	/* F7 */
};
static char extscantokey[] = {
0,
120,	/* F9 */
0,
116,	/* F5 */
114,	/* F3 */
112,	/* F1 */
113,	/* F2 */
123,	/* F12 */
0,
121,	/* F10 */
119,	/* F8 */
117,	/* F6 */
115,	/* F4 */
16,	/* TAB */
1,	/* ` */
0,
0,
62,	/* ALT (right) */
124,	/* Print Screen */
0,
64,	/* CTL (right) */
17,	/* Q */
2,	/* 1 */
0,
0,
0,
46,	/* Z */
32,	/* S */
31,	/* A */
18,	/* W */
3,	/* 2 */
0,
0,
48,	/* C */
47,	/* X */
33,	/* D */
19,	/* E */
5,	/* 4 */
4,	/* 3 */
0,
0,
61,	/* SPACE */
49,	/* V */
34,	/* F */
21,	/* T */
20,	/* R */
6,	/* 5 */
0,
0,
51,	/* N */
50,	/* B */
36,	/* H */
35,	/* G */
22,	/* Y */
7,	/* 6 */
0,
0,
0,
52,	/* M */
37,	/* J */
23,	/* U */
8,	/* 7 */
9,	/* 8 */
0,
0,
53,	/* , */
38,	/* K */
24,	/* I */
25,	/* O */
11,	/* 0 */
10,	/* 9 */
0,
0,
54,	/* . */
95,	/* / */
39,	/* L */
40,	/* ; */
26,	/* P */
12,	/* - */
0,
0,
0,
41,	/* " */
0,
27,	/* [ */
13,	/* + */
0,
0,
0,
57,	/* SHIFT (right) */
108,	/* ENTER */
28,	/* ] */
0,
29,	/* \ */
0,
0,
0,
45,	/* na*/
0,
0,
0,
0,
15,	/* backspace */
0,
0,				/* keypad */
81,	/* end */
0,
79,	/* left arrow */
80,	/* home */
0,
0,
0,
75,	/* ins */
76,	/* del */
84,	/* down arrow */
97,	/* 5 */
89,	/* right arrow */
83,	/* up arrow */
110,	/* ESC */
90,	/* Num Lock */
122,	/* F11 */
106,	/* + */
86,	/* page down */
105,	/* - */
124,	/* print screen */
85,	/* page up */
0,
0,
0,
0,
0,
118,	/* F7 */
};
#define	CODE_SIZE	4		/* Use a max of 4 for now... */
typedef struct
{
	u_short	type;
	char	unshift[CODE_SIZE];
	char	shift[CODE_SIZE];
	char	ctl[CODE_SIZE];
} Scan_def;

static Scan_def	scan_codes[] =
{
	NONE,	"",		"",		"",		/* 0 unused */
	ASCII,	"\033",		"\033",		"\033",		/* 1 ESCape */
	ASCII,	"1",		"!",		"!",		/* 2 1 */
	ASCII,	"2",		"@",		"\000",		/* 3 2 */
	ASCII,	"3",		"#",		"#",		/* 4 3 */
	ASCII,	"4",		"$",		"$",		/* 5 4 */
	ASCII,	"5",		"%",		"%",		/* 6 5 */
	ASCII,	"6",		"^",		"\036",		/* 7 6 */
	ASCII,	"7",		"&",		"&",		/* 8 7 */
	ASCII,	"8",		"*",		"\010",		/* 9 8 */
	ASCII,	"9",		"(",		"(",		/* 10 9 */
	ASCII,	"0",		")",		")",		/* 11 0 */
	ASCII,	"-",		"_",		"\037",		/* 12 - */
	ASCII,	"=",		"+",		"+",		/* 13 = */
	ASCII,	"\177",		"\177",		"\010",		/* 14 backspace */
	ASCII,	"\t",		"\177\t",	"\t",		/* 15 tab */
	ASCII,	"q",		"Q",		"\021",		/* 16 q */
	ASCII,	"w",		"W",		"\027",		/* 17 w */
	ASCII,	"e",		"E",		"\005",		/* 18 e */
	ASCII,	"r",		"R",		"\022",		/* 19 r */
	ASCII,	"t",		"T",		"\024",		/* 20 t */
	ASCII,	"y",		"Y",		"\031",		/* 21 y */
	ASCII,	"u",		"U",		"\025",		/* 22 u */
	ASCII,	"i",		"I",		"\011",		/* 23 i */
	ASCII,	"o",		"O",		"\017",		/* 24 o */
	ASCII,	"p",		"P",		"\020",		/* 25 p */
	ASCII,	"[",		"{",		"\033",		/* 26 [ */
	ASCII,	"]",		"}",		"\035",		/* 27 ] */
	ASCII,	"\r",		"\r",		"\n",		/* 28 return */
	CTL,	"",		"",		"",		/* 29 control */
	ASCII,	"a",		"A",		"\001",		/* 30 a */
	ASCII,	"s",		"S",		"\023",		/* 31 s */
	ASCII,	"d",		"D",		"\004",		/* 32 d */
	ASCII,	"f",		"F",		"\006",		/* 33 f */
	ASCII,	"g",		"G",		"\007",		/* 34 g */
	ASCII,	"h",		"H",		"\010",		/* 35 h */
	ASCII,	"j",		"J",		"\n",		/* 36 j */
	ASCII,	"k",		"K",		"\013",		/* 37 k */
	ASCII,	"l",		"L",		"\014",		/* 38 l */
	ASCII,	";",		":",		";",		/* 39 ; */
	ASCII,	"'",		"\"",		"'",		/* 40 ' */
	ASCII,	"`",		"~",		"`",		/* 41 ` */
	SHIFT,	"",		"",		"",		/* 42 shift */
	ASCII,	"\\",		"|",		"\034",		/* 43 \ */
	ASCII,	"z",		"Z",		"\032",		/* 44 z */
	ASCII,	"x",		"X",		"\030",		/* 45 x */
	ASCII,	"c",		"C",		"\003",		/* 46 c */
	ASCII,	"v",		"V",		"\026",		/* 47 v */
	ASCII,	"b",		"B",		"\002",		/* 48 b */
	ASCII,	"n",		"N",		"\016",		/* 49 n */
	ASCII,	"m",		"M",		"\r",		/* 50 m */
	ASCII,	",",		"<",		"<",		/* 51 , */
	ASCII,	".",		">",		">",		/* 52 . */
	ASCII,	"/",		"?",		"\037",		/* 53 / */
	SHIFT,	"",		"",		"",		/* 54 shift */
	KP,	"*",		"*",		"*",		/* 55 kp * */
	ALT,	"",		"",		"",		/* 56 alt */
	ASCII,	" ",		" ",		"\000",		/* 57 space */
	CAPS,	"",		"",		"",		/* 58 caps */
	FUNC,	"\033[M",	"\033[Y",	"\033[k",	/* 59 f1 */
	FUNC,	"\033[N",	"\033[Z",	"\033[l",	/* 60 f2 */
	FUNC,	"\033[O",	"\033[a",	"\033[m",	/* 61 f3 */
	FUNC,	"\033[P",	"\033[b",	"\033[n",	/* 62 f4 */
	FUNC,	"\033[Q",	"\033[c",	"\033[o",	/* 63 f5 */
	FUNC,	"\033[R",	"\033[d",	"\033[p",	/* 64 f6 */
	FUNC,	"\033[S",	"\033[e",	"\033[q",	/* 65 f7 */
	FUNC,	"\033[T",	"\033[f",	"\033[r",	/* 66 f8 */
	FUNC,	"\033[U",	"\033[g",	"\033[s",	/* 67 f9 */
	FUNC,	"\033[V",	"\033[h",	"\033[t",	/* 68 f10 */
	NUM,	"",		"",		"",		/* 69 num lock */
	SCROLL,	"",		"",		"",		/* 70 scroll lock */
	KP,	"7",		"\033[H",	"7",		/* 71 kp 7 */
	KP,	"8",		"\033[A",	"8",		/* 72 kp 8 */
	KP,	"9",		"\033[I",	"9",		/* 73 kp 9 */
	KP,	"-",		"-",		"-",		/* 74 kp - */
	KP,	"4",		"\033[D",	"4",		/* 75 kp 4 */
	KP,	"5",		"\033[E",	"5",		/* 76 kp 5 */
	KP,	"6",		"\033[C",	"6",		/* 77 kp 6 */
	KP,	"+",		"+",		"+",		/* 78 kp + */
	KP,	"1",		"\033[F",	"1",		/* 79 kp 1 */
	KP,	"2",		"\033[B",	"2",		/* 80 kp 2 */
	KP,	"3",		"\033[G",	"3",		/* 81 kp 3 */
	KP,	"0",		"\033[L",	"0",		/* 82 kp 0 */
	KP,	".",		"\177",		".",		/* 83 kp . */
	NONE,	"",		"",		"",		/* 84 0 */
	NONE,	"100",		"",		"",		/* 85 0 */
	NONE,	"101",		"",		"",		/* 86 0 */
	FUNC,	"\033[W",	"\033[i",	"\033[u",	/* 87 f11 */
	FUNC,	"\033[X",	"\033[j",	"\033[v",	/* 88 f12 */
	NONE,	"102",		"",		"",		/* 89 0 */
	NONE,	"103",		"",		"",		/* 90 0 */
	NONE,	"",		"",		"",		/* 91 0 */
	NONE,	"",		"",		"",		/* 92 0 */
	NONE,	"",		"",		"",		/* 93 0 */
	NONE,	"",		"",		"",		/* 94 0 */
	NONE,	"",		"",		"",		/* 95 0 */
	NONE,	"",		"",		"",		/* 96 0 */
	NONE,	"",		"",		"",		/* 97 0 */
	NONE,	"",		"",		"",		/* 98 0 */
	NONE,	"",		"",		"",		/* 99 0 */
	NONE,	"",		"",		"",		/* 100 */
	NONE,	"",		"",		"",		/* 101 */
	NONE,	"",		"",		"",		/* 102 */
	NONE,	"",		"",		"",		/* 103 */
	NONE,	"",		"",		"",		/* 104 */
	NONE,	"",		"",		"",		/* 105 */
	NONE,	"",		"",		"",		/* 106 */
	NONE,	"",		"",		"",		/* 107 */
	NONE,	"",		"",		"",		/* 108 */
	NONE,	"",		"",		"",		/* 109 */
	NONE,	"",		"",		"",		/* 110 */
	NONE,	"",		"",		"",		/* 111 */
	NONE,	"",		"",		"",		/* 112 */
	NONE,	"",		"",		"",		/* 113 */
	NONE,	"",		"",		"",		/* 114 */
	NONE,	"",		"",		"",		/* 115 */
	NONE,	"",		"",		"",		/* 116 */
	NONE,	"",		"",		"",		/* 117 */
	NONE,	"",		"",		"",		/* 118 */
	NONE,	"",		"",		"",		/* 119 */
	NONE,	"",		"",		"",		/* 120 */
	NONE,	"",		"",		"",		/* 121 */
	NONE,	"",		"",		"",		/* 122 */
	NONE,	"",		"",		"",		/* 123 */
	NONE,	"",		"",		"",		/* 124 */
	NONE,	"",		"",		"",		/* 125 */
	NONE,	"",		"",		"",		/* 126 */
	NONE,	"",		"",		"",		/* 127 */
};

static void
update_led()
{
	u_char new_leds = lock_state & (SCROLL | NUM | CAPS);

	if (kbd_cmd(KBC_MODEIND, 0) || kbd_cmd(new_leds, 0)) {
		printf("pc: timeout updating leds\n");
		(void) kbd_cmd(KBC_ENABLE, 0);
	}
}

static void
set_typematic(u_char new_rate)
{

	if (kbd_cmd(KBC_TYPEMATIC, 0) || kbd_cmd(new_rate, 0)) {
		printf("pc: timeout updating typematic rate\n");
		(void) kbd_cmd(KBC_ENABLE, 0);
	}
}

/*
 *   sget(ps, noblock):  get  characters  from  the  keyboard.  If
 *   noblock  ==  0  wait  until a key is gotten. Otherwise return a
 *    if no characters are present 0.
 */
static char *
sget(ps, noblock)
	struct pc_state *ps;
	int noblock;
{
	u_char dt;
	static u_char extended = 0, lock_down = 0;
	static u_char capchar[2];

	/*
	 *   First see if there is something in the keyboard port
	 */
loop:
	if ((inb(KBSTATP) & KBS_DIB) == 0)
		if (noblock)
			return 0;
		else
			goto loop;

	dt = inb(KBDATAP);

	if (ps->ps_flags & PSF_RAW) {
		capchar[0] = dt;
		capchar[1] = 0;
		/*
		 *   Check for locking keys
		 */
		switch (scan_codes[dt & 0x7f].type)
		{
			case NUM:
				if (dt & 0x80) {
					lock_down &= ~NUM;
					break;
				}
				lock_down |= NUM;
				lock_state ^= NUM;
				break;
			case CAPS:
				if (dt & 0x80) {
					lock_down &= ~CAPS;
					break;
				}
				lock_down |= CAPS;
				lock_state ^= CAPS;
				break;
			case SCROLL:
				if (dt & 0x80) {
					lock_down &= ~SCROLL;
					break;
				}
				lock_down |= SCROLL;
				lock_state ^= SCROLL;
				if ((lock_state & SCROLL) == 0)
					wakeup((caddr_t)&lock_state);
				break;
		}
		return capchar;
	}

	switch (dt) {
	    case KBR_EXTENDED:
		extended = 1;
		goto loop;
	    case KBR_ACK:
		ack = 1;
		goto loop;
	    case KBR_RESEND:
		nak = 1;
		goto loop;
	}

#ifdef DDB
	/*
	 *   Check for cntl-alt-esc
	 */
	if ((dt == 1) && (lock_down & (CTL | ALT)) == (CTL | ALT)) {
		Debugger();
		dt |= 0x80;	/* discard esc (ddb discarded CTL-alt) */
	}
#endif

	/*
	 *   Check for make/break
	 */
	if (dt & 0x80) {
		/*
		 *   break
		 */
		dt &= 0x7f;
		switch (scan_codes[dt].type)
		{
			case NUM:
				lock_down &=~ NUM;
				break;
			case CAPS:
				lock_down &=~ CAPS;
				break;
			case SCROLL:
				lock_down &=~ SCROLL;
				break;
			case SHIFT:
				lock_down &=~ SHIFT;
				break;
			case ALT:
				lock_down &=~ ALT;
				break;
			case CTL:
				lock_down &=~ CTL;
				break;
		}
	} else {
		/*
		 *   Make
		 */
		switch (scan_codes[dt].type)
		{
			/*
			 *   Locking keys
			 */
			case NUM:
				if (lock_down & NUM)
					break;
				lock_down |= NUM;
				lock_state ^= NUM;
				update_led();
				break;
			case CAPS:
				if (lock_down & CAPS)
					break;
				lock_down |= CAPS;
				lock_state ^= CAPS;
				update_led();
				break;
			case SCROLL:
				if (lock_down & SCROLL)
					break;
				lock_down |= SCROLL;
				lock_state ^= SCROLL;
				if ((lock_state & SCROLL) == 0)
					wakeup((caddr_t)&lock_state);
				update_led();
				break;
			/*
			 *   Non-locking keys
			 */
			case SHIFT:
				lock_down |= SHIFT;
				break;
			case ALT:
				lock_down |= ALT;
				break;
			case CTL:
				lock_down |= CTL;
				break;
			case ASCII:
				/* control has highest priority */
				if (lock_down & CTL)
					capchar[0] = scan_codes[dt].ctl[0];
				else if (lock_down & SHIFT)
					capchar[0] = scan_codes[dt].shift[0];
				else
					capchar[0] = scan_codes[dt].unshift[0];
				if ((lock_down & CAPS) && (capchar[0] >= 'a'
					 && capchar[0] <= 'z')) {
					capchar[0] = capchar[0] - ('a' - 'A');
				}
				capchar[0] |= (lock_down & ALT);
				extended = 0;
				return capchar;
			case NONE:
				break;
			case FUNC: {
				char	*more_chars;
				if (lock_down & SHIFT)
					more_chars = scan_codes[dt].shift;
				else if (lock_down & CTL)
					more_chars = scan_codes[dt].ctl;
				else
					more_chars = scan_codes[dt].unshift;
				extended = 0;
				return more_chars;
			}
			case KP: {
				char  	*more_chars;
				if (lock_down & (SHIFT | CTL) ||
				    (lock_state & NUM) == 0 || extended)
					more_chars = scan_codes[dt].shift;
				else
					more_chars = scan_codes[dt].unshift;
				extended = 0;
				return more_chars;
			}
		}
	}

	extended = 0;
	goto loop;
}

int
pcmmap(dev, offset, nprot)
	dev_t dev;
	int offset;
	int nprot;
{
	if (offset > 0x20000)
		return -1;
	return i386_btop(IOM_BEGIN + offset);
}

static void
pc_xmode_on(ps)
	struct pc_state *ps;
{
	struct trapframe *fp;

	if (ps->ps_flags & PSF_RAW)
		return;
	ps->ps_flags |= PSF_RAW;

	untimeout((timeout_t)cursor, (caddr_t)ps);

	fp = (struct trapframe *)curproc->p_regs;
	fp->tf_eflags |= PSL_IOPL;
}

static void
pc_xmode_off(ps)
	struct pc_state *ps;
{
	struct trapframe *fp;

	if ((ps->ps_flags & PSF_RAW) == 0)
		return;
	ps->ps_flags &= ~PSF_RAW;

	cursor(ps);

	fp = (struct trapframe *)curproc->p_regs;
	fp->tf_eflags &= ~PSL_IOPL;

}

void
pccnprobe(cp)
	struct consdev *cp;
{
	int maj;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == pcopen)
			break;

	if (maj >= nchrdev || !_pcprobe(&pc_state[0])) {
		cp->cn_pri = CN_DEAD;
		return;
	}

	/* initialize required fields */
	cp->cn_dev = makedev(maj, 0);
	cp->cn_pri = CN_INTERNAL;
}

void
pccninit(cp)
	struct consdev *cp;
{
	/*
	 * For now, don't screw with it.
	 */
}

int
pccngetc(dev)
	dev_t dev;
{
	struct pc_state *ps = &pc_state[PCUNIT(dev)];
	register int s;
	register char *cp;

	if (ps->ps_flags & PSF_RAW)
		return 0;

	s = spltty();		/* block pcrint while we poll */
	cp = sget(ps, 0);
	splx(s);
	if (*cp == '\r')
		return '\n';
	return *cp;
}

void
pccnputc(dev, c)
	dev_t dev;
	char c;
{
	struct pc_state *ps = &pc_state[PCUNIT(dev)];

	if (c == '\n')
		sput(ps, "\r\n", 2, 1);
	else
		sput(ps, &c, 1, 1);
	docursor(ps);
}
