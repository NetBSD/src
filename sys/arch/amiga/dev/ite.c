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
 *	from: @(#)ite.c	7.6 (Berkeley) 5/16/91
 *	$Id: ite.c,v 1.1.1.1 1993/07/05 19:19:44 mw Exp $
 */

/*
 * Bit-mapped display terminal emulator machine independent code.
 * This is a very rudimentary.  Much more can be abstracted out of
 * the hardware dependent routines.
 */
#include "ite.h"
#if NITE > 0

#include "grf.h"

#undef NITE
#define NITE	NGRF

#include "param.h"
#include "conf.h"
#include "proc.h"
#include "ioctl.h"
#include "tty.h"
#include "systm.h"
#include "malloc.h"

#include "itevar.h"
#include "iteioctl.h"
#include "kbdmap.h"

#include "machine/cpu.h"

#ifdef __STDC__
/* automatically generated, as you might guess:-) */

struct consdev;
struct itesw;

extern int iteon (dev_t dev, int flag);
extern int iteinit (dev_t dev);
extern int iteoff (dev_t dev, int flag);
extern int iteopen (dev_t dev, int mode, int devtype, struct proc *p);
extern int iteclose (dev_t dev, int flag, int mode, struct proc *p);
extern int iteread (dev_t dev, struct uio *uio, int flag);
extern int itewrite (dev_t dev, struct uio *uio, int flag);
extern int iteioctl (dev_t dev, int cmd, caddr_t addr, int flag);
extern int itestart (register struct tty *tp);
extern int itefilter (register u_char c, enum caller caller);
extern int iteputchar (register int c, dev_t dev);
extern int itecheckwrap (register struct ite_softc *ip, register struct itesw *sp);
extern int itecnprobe (struct consdev *cp);
extern int itecninit (struct consdev *cp);
extern int itecngetc (dev_t dev);
extern int itecnputc (dev_t dev, int c);
static void repeat_handler (int a0, int a1);
static void ite_dnchar (struct ite_softc *ip, struct itesw *sp, int n);
static void ite_inchar (struct ite_softc *ip, struct itesw *sp, int n);
static void ite_clrtoeol (struct ite_softc *ip, struct itesw *sp, int y, int x);
static void ite_clrtobol (struct ite_softc *ip, struct itesw *sp, int y, int x);
static void ite_clrline (struct ite_softc *ip, struct itesw *sp, int y, int x);
static void ite_clrtoeos (struct ite_softc *ip, struct itesw *sp);
static void ite_clrtobos (struct ite_softc *ip, struct itesw *sp);
static void ite_clrscreen (struct ite_softc *ip, struct itesw *sp);
static void ite_dnline (struct ite_softc *ip, struct itesw *sp, int n);
static void ite_inline (struct ite_softc *ip, struct itesw *sp, int n);
static void ite_lf (struct ite_softc *ip, struct itesw *sp);
static void ite_crlf (struct ite_softc *ip, struct itesw *sp);
static void ite_cr (struct ite_softc *ip, struct itesw *sp);
static void ite_rlf (struct ite_softc *ip, struct itesw *sp);
static int atoi (const char *cp);
static char *index (const char *cp, char ch);
static int ite_argnum (struct ite_softc *ip);
static int ite_zargnum (struct ite_softc *ip);
static void ite_sendstr (struct ite_softc *ip, char *str);
static int strncmp (const char *a, const char *b, int l);
#endif


#define set_attr(ip, attr)	((ip)->attribute |= (attr))
#define clr_attr(ip, attr)	((ip)->attribute &= ~(attr))

extern  int nodev();

int customc_scroll(),	customc_init(),		customc_deinit();
int customc_clear(),	customc_putc(),		customc_cursor();

int tiga_scroll(),	tiga_init(),		tiga_deinit();
int tiga_clear(),	tiga_putc(),		tiga_cursor();


struct itesw itesw[] =
{
	customc_init,		customc_deinit,		customc_clear,
	customc_putc,		customc_cursor,		customc_scroll,
	
	tiga_init,		tiga_deinit,		tiga_clear,
	tiga_putc,		tiga_cursor,		tiga_scroll,
};

/*
 * # of chars are output in a single itestart() call.
 * If this is too big, user processes will be blocked out for
 * long periods of time while we are emptying the queue in itestart().
 * If it is too small, console output will be very ragged.
 */
int	iteburst = 64;

int	nite = NITE;
struct  tty *kbd_tty = NULL;
struct	tty ite_cons;
struct	tty *ite_tty[NITE] = { &ite_cons };
struct  ite_softc ite_softc[NITE];

int	itestart();
extern	int ttrstrt();
extern	struct tty *constty;

/* These are (later..) settable via an ioctl */
int	start_repeat_timo = 30;	/* /100: initial timeout till pressed key repeats */
int	next_repeat_timo  = 5;  /* /100: timeout when repeating for next char */

/*
 * Primary attribute buffer to be used by the first bitmapped console
 * found. Secondary displays alloc the attribute buffer as needed.
 * Size is based on a 68x128 display, which is currently our largest.
 */
u_char  console_attributes[0x2200];

/*
 * Perform functions necessary to setup device as a terminal emulator.
 */
iteon(dev, flag)
	dev_t dev;
{
	int unit = UNIT(dev);
	struct tty *tp = ite_tty[unit];
	struct ite_softc *ip = &ite_softc[unit];

	if (unit < 0 || unit >= NITE || (ip->flags&ITE_ALIVE) == 0)
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
	if (kbd_tty == NULL || kbd_tty == tp) {
		kbd_tty = tp;
		kbdenable();
	}
	iteinit(dev);
	return(0);
}

/* used by the grf layer to reinitialize ite after changing fb parameters */
itereinit(dev)
     dev_t dev;
{
  int unit = UNIT(dev);
  struct ite_softc *ip = &ite_softc[unit];

  ip->flags &= ~ITE_INITED;
  iteinit (dev);
}

iteinit(dev)
     dev_t dev;
{
	int unit = UNIT(dev);
	struct ite_softc *ip = &ite_softc[unit];

	if (ip->flags & ITE_INITED)
		return;
	
	ip->curx = 0;
	ip->cury = 0;
	ip->cursorx = 0;
	ip->cursory = 0;
	ip->save_curx = 0;
	ip->save_cury = 0;
	ip->ap = ip->argbuf;
	ip->emul_level = EMUL_VT300_7;
	ip->eightbit_C1 = 0;
	ip->inside_margins = 0;
	ip->linefeed_newline = 0;

	(*itesw[ip->type].ite_init)(ip);
	(*itesw[ip->type].ite_cursor)(ip, DRAW_CURSOR);

	/* ip->rows initialized by ite_init above */
	ip->top_margin = 0; ip->bottom_margin = ip->rows - 1;

	ip->attribute = 0;
	if (ip->attrbuf == NULL)
		ip->attrbuf = (u_char *)
			malloc(ip->rows * ip->cols, M_DEVBUF, M_WAITOK);
	bzero(ip->attrbuf, (ip->rows * ip->cols));

	ip->imode = 0;
	ip->flags |= ITE_INITED;
}

/*
 * "Shut down" device as terminal emulator.
 * Note that we do not deinit the console device unless forced.
 * Deinit'ing the console every time leads to a very active
 * screen when processing /etc/rc.
 */
iteoff(dev, flag)
	dev_t dev;
{
	register struct ite_softc *ip = &ite_softc[UNIT(dev)];

	if (flag & 2)
		ip->flags |= ITE_INGRF;
	if ((ip->flags & ITE_ACTIVE) == 0)
		return;
	if ((flag & 1) ||
	    (ip->flags & (ITE_INGRF|ITE_ISCONS|ITE_INITED)) == ITE_INITED)
		(*itesw[ip->type].ite_deinit)(ip);
	if ((flag & 2) == 0)
		ip->flags &= ~ITE_ACTIVE;
}

/* ARGSUSED */
#ifdef __STDC__
iteopen(dev_t dev, int mode, int devtype, struct proc *p)
#else
iteopen(dev, mode, devtype, p)
	dev_t dev;
	int mode, devtype;
	struct proc *p;
#endif
{
	int unit = UNIT(dev);
	register struct tty *tp;
	register struct ite_softc *ip = &ite_softc[unit];
	register int error;
	int first = 0;

	if(!ite_tty[unit]) {
		MALLOC(tp, struct tty *, sizeof(struct tty), M_TTYS, M_WAITOK);
		bzero(tp, sizeof(struct tty));
		ite_tty[unit] = tp;
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
		tp->t_cflag = CS8|CREAD;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		/* don't set TS_ISOPEN  here, or the tty queues won't
		   be initialized in ttyopen()! */
		tp->t_state = TS_CARR_ON;
		ttsetwater(tp);
	} 
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	if (error == 0) {
		tp->t_winsize.ws_row = ip->rows;
		tp->t_winsize.ws_col = ip->cols;
	} else if (first)
		iteoff(dev, 0);
	return (error);
}

/*ARGSUSED*/
iteclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = ite_tty[UNIT(dev)];

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	iteoff(dev, 0);
	if (tp != &ite_cons)
	  {
	    FREE(tp, M_TTYS);
	    ite_tty[UNIT(dev)] = (struct tty *)NULL;
	  }
	return(0);
}

iteread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = ite_tty[UNIT(dev)];
	int rc;

	rc = ((*linesw[tp->t_line].l_read)(tp, uio, flag));
	return rc;
}

itewrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	int unit = UNIT(dev);
	register struct tty *tp = ite_tty[unit];

	if ((ite_softc[unit].flags & ITE_ISCONS) && constty &&
	    (constty->t_state&(TS_CARR_ON|TS_ISOPEN))==(TS_CARR_ON|TS_ISOPEN))
		tp = constty;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

iteioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register struct tty *tp = ite_tty[UNIT(dev)];
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error >= 0)
		return (error);
	return (ENOTTY);
}

itestart(tp)
	register struct tty *tp;
{
	register int cc, s;
	int hiwat = 0;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	cc = RB_LEN (&tp->t_out);
	if (cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(&tp->t_out);
		}
		selwakeup(&tp->t_wsel);
	}
	/*
	 * Limit the amount of output we do in one burst
	 * to prevent hogging the CPU.
	 */
	if (cc > iteburst) {
		hiwat++;
		cc = iteburst;
	}
	while (--cc >= 0) {
		register int c;

		c = rbgetc(&tp->t_out);
		/*
		 * iteputchar() may take a long time and we don't want to
		 * block all interrupts for long periods of time.  Since
		 * there is no need to stay at high priority while outputing
		 * the character (since we don't have to worry about
		 * interrupts), we don't.  We just need to make sure that
		 * we don't reenter iteputchar, which is guarenteed by the
		 * earlier setting of TS_BUSY.
		 */
		splx(s);
		iteputchar(c, tp->t_dev);
		spltty();
	}
	if (hiwat) {
		tp->t_state |= TS_TIMEOUT;
		timeout(ttrstrt, tp, 1);
	}
	tp->t_state &= ~TS_BUSY;
	splx(s);
}

/* these are used to implement repeating keys.. */
static u_char last_char = 0;
static u_char tout_pending = 0;

static void
repeat_handler (a0, a1)
    int a0, a1;
{
  tout_pending = 0;
  /* leave it up to itefilter() to possible install a new callout entry
     to reinvoke repeat_handler() */
  itefilter (last_char, ITEFILT_REPEATER);
}


int
itefilter(c, caller)
     register u_char c;
     enum caller caller;
{
  static u_char mod = 0;
  static u_char last_dead = 0;
  register char code, *str;
  u_char up, mask, i;
  struct key key;
  
  if (caller != ITEFILT_CONSOLE && kbd_tty == NULL)
     return;

  up = c & 0x80 ? 1 : 0;
  c &= 0x7f;
  code = 0;
  
  mask = 0;
  if (c >= KBD_LEFT_SHIFT)
    {
      switch (c)
        {
        case KBD_LEFT_SHIFT:
          mask = KBD_MOD_LSHIFT;
          break;

        case KBD_RIGHT_SHIFT:
          mask = KBD_MOD_RSHIFT;
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
          /* capslock already behaves `right', don't need to keep track of the
             state in here. */
          mask = KBD_MOD_CAPS;
          break;
 
        case KBD_CTRL:
          mask = KBD_MOD_CTRL;
          break;
        }

      if (mask)
        {
          if (up)
            mod &= ~mask;
          else
            mod |= mask;
        }

      /* these keys should not repeat, so it's the Right Thing dealing with
         repeaters only after this block. */

      /* return even if it wasn't a modifier key, the other codes up here
         are either special (like reset warning), or not yet defined */
      return -1;
    }  

  /* no matter which character we're repeating, stop it if we get a key-up
     event. I think this is the same thing amigados does. */
  if (up)
    {
      if (tout_pending)
        {
          untimeout (repeat_handler, 0);
          tout_pending = 0;
        }
      return -1;
    }


  switch (mod & (KBD_MOD_ALT | KBD_MOD_SHIFT))
    {
    case 0:
	key = kbdmap.keys[c];
	if (!(mod & KBD_MOD_CAPS) || !(key.mode & KBD_MODE_CAPS))
	  break;
	/* FALL INTO */
	
    case KBD_MOD_LSHIFT:
    case KBD_MOD_RSHIFT:
    case KBD_MOD_SHIFT:
        key = kbdmap.shift_keys[c];
        break;
        
    case KBD_MOD_LALT:
    case KBD_MOD_RALT:
    case KBD_MOD_ALT:
	key = kbdmap.alt_keys[c];
	if (!(mod & KBD_MOD_CAPS) || !(key.mode & KBD_MODE_CAPS))
	  break;
	/* FALL INTO */

    case KBD_MOD_LALT|KBD_MOD_LSHIFT:
    case KBD_MOD_LALT|KBD_MOD_RSHIFT:
    case KBD_MOD_LALT|KBD_MOD_SHIFT:
    case KBD_MOD_RALT|KBD_MOD_RSHIFT:
    case KBD_MOD_RALT|KBD_MOD_SHIFT:
    case KBD_MOD_ALT|KBD_MOD_RSHIFT:
	key = kbdmap.alt_shift_keys[c];
	break;
    }

  code = key.code;

  /* arrange to repeat the keystroke. By doing this at the level of scan-codes,
     we can have function keys, and keys that send strings, repeat too. This
     also entitles an additional overhead, since we have to do the conversion
     each time, but I guess that's ok. */
  if (!tout_pending && caller == ITEFILT_TTY)
    {
      tout_pending = 1;
      last_char = c;
      timeout (repeat_handler, 0, start_repeat_timo);
    }
  else if (!tout_pending && caller == ITEFILT_REPEATER)
    {
      tout_pending = 1;
      last_char = c;
      timeout (repeat_handler, 0, next_repeat_timo);
    }
 
  /* handle dead keys */
  if (key.mode & KBD_MODE_DEAD)
    {
      /* if entered twice, send accent itself */
      if (last_dead == key.mode & KBD_MODE_ACCMASK)
        last_dead = 0;
      else
	{
	  last_dead = key.mode & KBD_MODE_ACCMASK;
	  return -1;
	}
    }
  if (last_dead)
    {
      /* can't apply dead flag to string-keys */
      if (! (key.mode & KBD_MODE_STRING) && code >= '@' && code < 0x7f)
        code = acctable[KBD_MODE_ACCENT (last_dead)][code - '@'];

      last_dead = 0;
    }
  
  /* if not string, apply META and CTRL modifiers */
  if (! (key.mode & KBD_MODE_STRING))
    {
      if (mod & KBD_MOD_CTRL)
        code &= 0x1f;

      if (mod & KBD_MOD_META)
        code |= 0x80;
    }
  else
    {
      /* strings are only supported in normal tty mode, not in console mode */
      if (caller != ITEFILT_CONSOLE)
        {
          str = kbdmap.strings + code;
          /* using a length-byte instead of 0-termination allows to embed \0 into
             strings, although this is not used in the default keymap */
          for (i = *str++; i; i--)
            (*linesw[kbd_tty->t_line].l_rint)(*str++, kbd_tty);
        }
      return -1;
    }

  if (caller == ITEFILT_CONSOLE)
    return code;
  else
    (*linesw[kbd_tty->t_line].l_rint)(code, kbd_tty);

  return -1;
}


/* helper functions, makes the code below more readable */
static void inline
ite_dnchar(ip, sp, n)
     struct ite_softc *ip;
     struct itesw *sp;
     int n;
{
  (*sp->ite_scroll)(ip, ip->cury, ip->curx + n, n, SCROLL_LEFT);
  attrmov(ip, ip->cury, ip->curx + n, ip->cury, ip->curx,
	  1, ip->cols - ip->curx - n);
  attrclr(ip, ip->cury, ip->cols - n, 1, n);
  while (n--)
    (*sp->ite_putc)(ip, ' ', ip->cury, ip->cols - n - 1, ATTR_NOR);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_inchar(ip, sp, n)
     struct ite_softc *ip;
     struct itesw *sp;
     int n;
{
  (*sp->ite_scroll)(ip, ip->cury, ip->curx, n, SCROLL_RIGHT);
  attrmov(ip, ip->cury, ip->curx, ip->cury, ip->curx + n,
	  1, ip->cols - ip->curx - n);
  attrclr(ip, ip->cury, ip->curx, 1, n);
  while (n--)
    (*sp->ite_putc)(ip, ' ', ip->cury, ip->curx + n, ATTR_NOR);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrtoeol(ip, sp, y, x)
     struct ite_softc *ip;
     struct itesw *sp;
     int y, x;
{
  (*sp->ite_clear)(ip, y, x, 1, ip->cols - x);
  attrclr(ip, y, x, 1, ip->cols - x);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrtobol(ip, sp, y, x)
     struct ite_softc *ip;
     struct itesw *sp;
     int y, x;
{
  (*sp->ite_clear)(ip, y, 0, 1, x);
  attrclr(ip, y, 0, 1, x);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrline(ip, sp, y, x)
     struct ite_softc *ip;
     struct itesw *sp;
     int y, x;
{
  (*sp->ite_clear)(ip, y, 0, 1, ip->cols);
  attrclr(ip, y, 0, 1, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}



static void inline
ite_clrtoeos(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  (*sp->ite_clear)(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
  attrclr(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrtobos(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  (*sp->ite_clear)(ip, 0, 0, ip->cury, ip->cols);
  attrclr(ip, 0, 0, ip->cury, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrscreen(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  (*sp->ite_clear)(ip, 0, 0, ip->rows, ip->cols);
  attrclr(ip, 0, 0, ip->rows, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}



static void inline
ite_dnline(ip, sp, n)
     struct ite_softc *ip;
     struct itesw *sp;
     int n;
{
  (*sp->ite_scroll)(ip, ip->cury + n, 0, n, SCROLL_UP);
  attrmov(ip, ip->cury + n, 0, ip->cury, 0,
	  ip->bottom_margin + 1 - ip->cury - n, ip->cols);
  (*sp->ite_clear)(ip, ip->bottom_margin - n + 1, 0, n, ip->cols);
  attrclr(ip, ip->bottom_margin - n + 1, 0, n, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_inline(ip, sp, n)
     struct ite_softc *ip;
     struct itesw *sp;
     int n;
{
  (*sp->ite_scroll)(ip, ip->cury, 0, n, SCROLL_DOWN);
  attrmov(ip, ip->cury, 0, ip->cury + n, 0,
	  ip->bottom_margin + 1 - ip->cury - n, ip->cols);
  (*sp->ite_clear)(ip, ip->cury, 0, n, ip->cols);
  attrclr(ip, ip->cury, 0, n, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_lf (ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
#if 0
  if (ip->inside_margins)
    {
#endif
      if (++ip->cury >= ip->bottom_margin + 1) 
        {
          ip->cury = ip->bottom_margin;
          (*sp->ite_scroll)(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
          ite_clrtoeol(ip, sp, ip->cury, 0);
        }
      else
        (*sp->ite_cursor)(ip, MOVE_CURSOR);
#if 0
    }
  else
    {
      if (++ip->cury == ip->rows) 
        {
          --ip->cury;
          (*sp->ite_scroll)(ip, 1, 0, 1, SCROLL_UP);
          ite_clrtoeol(ip, sp, ip->cury, 0);
        }
      else
        (*sp->ite_cursor)(ip, MOVE_CURSOR);
    }
#endif
  clr_attr(ip, ATTR_INV);
}

static void inline 
ite_crlf (ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  ip->curx = 0;
  ite_lf (ip, sp);
}

static void inline
ite_cr (ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  if (ip->curx) 
    {
      ip->curx = 0;
      (*sp->ite_cursor)(ip, MOVE_CURSOR);
    }
}

static void inline
ite_rlf (ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  if (ip->cury > 0) 
    {
      ip->cury--;
      (*sp->ite_cursor)(ip, MOVE_CURSOR);
    }
  else
    ite_inline (ip, sp, 1);
  clr_attr(ip, ATTR_INV);
}

static int inline
atoi (cp)
    const char *cp;
{
  int n;

  for (n = 0; *cp && *cp >= '0' && *cp <= '9'; cp++)
    n = n * 10 + *cp - '0';

  return n;
}

static char *
index (cp, ch)
    const char *cp;
    char ch;
{
  while (*cp && *cp != ch) cp++;
  return *cp ? cp : 0;
}



static int inline
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

static int inline
ite_zargnum (ip)
    struct ite_softc *ip;
{
  char ch, *cp;
  int n;

  /* convert argument string into number */
  if (ip->ap == ip->argbuf)
    return 0;
  ch = *ip->ap;
  *ip->ap = 0;
  n = atoi (ip->argbuf);
  *ip->ap = ch;
  
  return n;
}

static void inline
ite_sendstr (ip, str)
    struct ite_softc *ip;
    char *str;
{
  while (*str)
    (*linesw[kbd_tty->t_line].l_rint)(*str++, kbd_tty);
}

static int inline
strncmp (a, b, l)
    const char *a, *b;
    int l;
{
  for (;l--; a++, b++)
    if (*a != *b)
      return *a - *b;
  return 0;
}

static void inline
ite_reinit(ip)
    struct ite_softc *ip;
{
  ip->curx = 0;
  ip->cury = 0;
  ip->cursorx = 0;
  ip->cursory = 0;
  ip->save_curx = 0;
  ip->save_cury = 0;
  ip->ap = ip->argbuf;
  ip->emul_level = EMUL_VT300_7;
  ip->eightbit_C1 = 0;
  ip->top_margin = 0; ip->bottom_margin = 23;
  ip->inside_margins = 0;
  ip->linefeed_newline = 0;

  (*itesw[ip->type].ite_cursor)(ip, DRAW_CURSOR);

  ip->attribute = 0;
  bzero(ip->attrbuf, (ip->rows * ip->cols));

  ip->imode = 0;
}



iteputchar(c, dev)
	register int c;
	dev_t dev;  
{
	int unit = UNIT(dev);
	register struct ite_softc *ip = &ite_softc[unit];
	register struct itesw *sp = &itesw[ip->type];
	register int n;
	int x, y;
	char *cp;

	if ((ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE)
	  	return;

	if ((ite_tty[unit]->t_cflag & CSIZE) == CS7
	    || (ite_tty[unit]->t_cflag & PARENB))
	  c &= 0x7f;

	if (ip->escape) 
	  {
doesc:
	    switch (ip->escape) 
	      {
	      case ESC:
	        switch (c)
	          {
		  /* first 7bit equivalents for the 8bit control characters */
		  
	          case 'D':
		    c = IND;
		    ip->escape = 0;
		    break;
		    
		  case 'E':
		    c = NEL;
		    ip->escape = 0;
		    break;
		    
		  case 'H':
		    c = HTS;
		    ip->escape = 0;
		    break;
		    
		  case 'M':
		    c = RI;
		    ip->escape = 0;
		    break;
		    
		  case 'N':
		    c = SS2;
		    ip->escape = 0;
		    break;
		  
		  case 'O':
		    c = SS3;
		    ip->escape = 0;
		    break;
		    
		  case 'P':
		    c = DCS;
		    ip->escape = 0;
		    break;
		    
		  case '[':
		    c = CSI;
		    ip->escape = 0;
		    break;
		    
		  case '\\':
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
		  case 'Z':	/* spanish */
		  case '=':	/* swiss */
		  /* note: %5 and %6 are not supported (two chars..) */
		    
		    ip->escape = 0;
		    /* just ignore for now */
		    return -1;
		    
		  
		  /* locking shift modes (as you might guess, not yet supported..) */
		  case '`':
		    ip->GR = ip->G1;
		    ip->escape = 0;
		    return -1;
		    
		  case 'n':
		    ip->GL = ip->G2;
		    ip->escape = 0;
		    return -1;
		    
		  case '}':
		    ip->GR = ip->G2;
		    ip->escape = 0;
		    return -1;
		    
		  case 'o':
		    ip->GL = ip->G3;
		    ip->escape = 0;
		    return -1;
		    
		  case '|':
		    ip->GR = ip->G3;
		    ip->escape = 0;
		    return -1;
		    
		  
		  /* font width/height control */
		  case '#':
		    ip->escape = '#';
		    return -1;
		    
		    
		  /* hard terminal reset .. */
		  case 'c':
		    ite_reinit (ip);
		    ip->escape = 0;
		    return -1;


		  case '7':
		    ip->save_curx = ip->curx;
		    ip->save_cury = ip->cury;
		    ip->escape = 0;
		    return -1;
		    
		  case '8':
		    ip->curx = ip->save_curx;
		    ip->cury = ip->save_cury;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    ip->escape = 0;
		    return -1;
		    
		  
		  /* default catch all for not recognized ESC sequences */
		  default:
		    ip->escape = 0;
		    return -1;
		  }
		break;


	      case ' ':
	        switch (c)
	          {
	          case 'F':
		    ip->eightbit_C1 = 0;
		    ip->escape = 0;
		    return -1;
		    
		  case 'G':
		    ip->eightbit_C1 = 1;
		    ip->escape = 0;
		    return -1;
		    
		  default:
		    /* not supported */
		    ip->escape = 0;
		    return -1;
		  }
		break;
		
		
	      case '#':
		switch (c)
		  {
		  case '5':
		    /* single height, single width */
		    ip->escape = 0;
		    return -1;
		    
		  case '6':
		    /* double width, single height */
		    ip->escape = 0;
		    return -1;
		    
		  case '3':
		    /* top half */
		    ip->escape = 0;
		    return -1;
		    
		  case '4':
		    /* bottom half */
		    ip->escape = 0;
		    return -1;
		    
		  case '8':
		    /* screen alignment pattern... */
		    ip->escape = 0;
		    return -1;
		    
		  default:
		    ip->escape = 0;
		    return -1;
		  }
		break;
		  


#if 0
	      case DCS:
	      case PM:
	      case APC:
	      case OSC:
		switch (c)
		  {
		  case ST:
		    ip->escape = 0;
		    return -1;
		    
		  default:
		    return -1;
		  }
		break;
#endif


	      case CSI:
	        /* the biggie... */
	        switch (c)
	          {
	          case '0': case '1': case '2': case '3': case '4':
	          case '5': case '6': case '7': case '8': case '9':
	          case ';': case '\"': case '$': case '>':
	            if (ip->ap < ip->argbuf + ARGBUF_SIZE)
	              *ip->ap++ = c;
	            return -1;

		  case CAN:
    		    ip->escape = 0;
		    return -1;

	            
	          case 'p':
		    *ip->ap = 0;
	            if (! strncmp (ip->argbuf, "61\"", 3))
	              ip->emul_level = EMUL_VT100;
	            else if (! strncmp (ip->argbuf, "63;1\"", 5)
	            	     || ! strncmp (ip->argbuf, "62;1\"", 5))
	              ip->emul_level = EMUL_VT300_7;
	            else
	              ip->emul_level = EMUL_VT300_8;
	            ip->escape = 0;
	            return -1;
	            
	          
	          case '?':
		    *ip->ap = 0;
	            ip->escape = '?';
	            ip->ap = ip->argbuf;
	            return -1;


		  case 'c':
		    *ip->ap = 0;
		    if (ip->ap == ip->argbuf
		        || (ip->ap == &ip->argbuf[1] && ip->argbuf[0] == '0'))
		      {
			/* primary DA request, send primary DA response */
			if (ip->emul_level == EMUL_VT100)
		          ite_sendstr (ip, "\033[61;0c");
		        else
		          ite_sendstr (ip, "\033[63;0c");
		      }

		    else if ((ip->ap == &ip->argbuf[1] && ip->argbuf[0] == '>')
		        || (ip->ap == &ip->argbuf[2] 
			    && !strncmp (ip->argbuf, ">0", 2)))
		      {
		        ite_sendstr (ip, "\033[>24;0;0;0c");
		      }

		    ip->escape = 0;
		    return -1;


		  case 'n':
		    *ip->ap = 0;
		    if (ip->ap == &ip->argbuf[1])
		      {
		        if (ip->argbuf[0] == '5')
		          ite_sendstr (ip, "\033[0n");	/* no malfunction */
		          
		        else if (ip->argbuf[0] == '6')
		          {
		            sprintf (ip->argbuf, "\033[%d;%dR", 
				     ip->cury + 1, ip->curx + 1);
			    /* cursor position report */
			    ite_sendstr (ip, ip->argbuf);
		          }
		                
		      }
		    ip->escape = 0;
		    return -1;
	          
	          
	          case 'h': case 'l':
		    *ip->ap = 0;
		    if (ip->ap == &ip->argbuf[1] && ip->argbuf[0] == '4')
		      ip->imode = (c == 'h');	/* insert/replace mode */

		    else if (ip->ap == &ip->argbuf[2] 
			     && !strncmp (ip->argbuf, "20", 2))
		      ip->linefeed_newline = (c == 'h');


	            /* various not supported commands */
	            ip->escape = 0;
	            return -1;


		  case 'M':
		    *ip->ap = 0;
		    ite_dnline (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return -1;

		  
		  case 'L':
		    *ip->ap = 0;
		    ite_inline (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return -1;


		  case 'P':
		    *ip->ap = 0;
		    ite_dnchar (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return -1;
		    

		  case '@':
		    *ip->ap = 0;
		    ite_inchar (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return -1;


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
		      {
			y += ip->top_margin;
			ip->cury = MIN(y, ip->bottom_margin);
		      }
		    else
		      {
			ip->cury = MIN(y, ip->rows - 1);
		      }
		    ip->curx = MIN(x, ip->cols - 1);
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return -1;
		    
		  case 'A':		    
		    *ip->ap = 0;
		    n = ip->cury - ite_argnum (ip);
		    ip->cury = n >= 0 ? n : 0;
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return -1;
		  
		  case 'B':
		    *ip->ap = 0;
		    n = ite_argnum (ip) + ip->cury;
		    ip->cury = MIN(n, ip->rows - 1);
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return -1;
		  
		  case 'C':
		    *ip->ap = 0;
		    n = ite_argnum (ip) + ip->curx;
		    ip->curx = MIN(n, ip->cols - 1);
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return -1;
		  
		  case 'D':
		    *ip->ap = 0;
		    n = ip->curx - ite_argnum (ip);
		    ip->curx = n >= 0 ? n : 0;
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return -1;
		  
		    


		  case 'J':
		    *ip->ap = 0;
		    n = ite_zargnum (ip);
		    if (n == 0)
	              ite_clrtoeos(ip, sp);
		    else if (n == 1)
		      ite_clrtobos(ip, sp);
		    else if (n == 2)
		      ite_clrscreen(ip, sp);
	            ip->escape = 0;
	            return -1;


		  case 'K':
		    *ip->ap = 0;
		    n = ite_zargnum (ip);
		    if (n == 0)
		      ite_clrtoeol(ip, sp, ip->cury, ip->curx);
		    else if (n == 1)
		      ite_clrtobol(ip, sp, ip->cury, ip->curx);
		    else if (n == 2)
		      ite_clrline(ip, sp, ip->cury, ip->curx);
		    ip->escape = 0;
		    return -1;


		  case 'X':
		    *ip->ap = 0;
		    for (n = ite_argnum (ip); n > 0; n--)
		      {
			attrclr(ip, ip->cury, ip->curx + n - 1, 1, 1);
			(*sp->ite_putc)(ip, ' ', ip->cury, ip->curx + n - 1, ATTR_NOR);
		      }
		    ip->escape = 0;
		    return -1;

	          
	          case '}': case '`':
	            /* status line control */
	            ip->escape = 0;
	            return -1;


		  case 'r':
		    *ip->ap = 0;
		    x = atoi (ip->argbuf);
		    y = 0;
		    cp = index (ip->argbuf, ';');
		    if (cp)
		      y = atoi (cp + 1);
		    if (x) x--;
		    if (y) y--;
		    ip->top_margin = MIN(x, ip->rows - 1);
		    ip->bottom_margin = MIN(y, ip->rows - 1);
		    ip->escape = 0;
		    return -1;
		    
		  
		  case 'm':
		    /* big attribute setter/resetter */
		    {
		      char *cp;
		      *ip->ap = 0;
		      for (cp = ip->argbuf; cp < ip->ap; )
		        {
			  switch (*cp)
			    {
			    case '0':
			      clr_attr (ip, ATTR_ALL);
			      cp++;
			      break;
			      
			    case '1':
			      set_attr (ip, ATTR_BOLD);
			      cp++;
			      break;
			      
			    case '2':
			      switch (cp[1])
			        {
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
			      
			    case '4':
			      set_attr (ip, ATTR_UL);
			      cp++;
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
		    return -1;


		  case 'u':
		    /* DECRQTSR */
		    ite_sendstr (ip, "\033P\033\\");
		    ip->escape = 0;
		    return -1;

		  
		  
		  default:
		    ip->escape = 0;
		    return -1;
		  }
		break;



	      case '?':	/* CSI ? */
	      	switch (c)
	      	  {
	          case '0': case '1': case '2': case '3': case '4':
	          case '5': case '6': case '7': case '8': case '9':
	          case ';': case '\"': case '$':
	            if (ip->ap < ip->argbuf + ARGBUF_SIZE)
	              *ip->ap++ = c;
	            return -1;


		  case CAN:
    		    ip->escape = 0;
		    return -1;

	            
		  case 'n':
		    *ip->ap = 0;
		    if (ip->ap == &ip->argbuf[2])
		      {
		        if (! strncmp (ip->argbuf, "15", 2))
		          /* printer status: no printer */
		          ite_sendstr (ip, "\033[13n");
		          
		        else if (! strncmp (ip->argbuf, "25", 2))
		          /* udk status */
		          ite_sendstr (ip, "\033[20n");
		          
		        else if (! strncmp (ip->argbuf, "26", 2))
		          /* keyboard dialect: US */
		          ite_sendstr (ip, "\033[27;1n");
		      }
		    ip->escape = 0;
		    return -1;


		  case 'h': case 'l':
		    *ip->ap = 0;
		    if (ip->ap[0] == '6')
		      ip->inside_margins = (c == 'h');

		    else if (! strncmp (ip->ap, "25", 2))
		      (*itesw[ip->type].ite_cursor)(ip, 
						    (c == 'h') ? DRAW_CURSOR : ERASE_CURSOR);

		    /* others.. */
		    
		  default:
		    ip->escape = 0;
		    return -1;
		  }
		break;

	      
	      default:
	        ip->escape = 0;
	        return -1;
	      }
          }


	switch (c) {

	case VT:	/* VT is treated like LF */
	case FF:	/* so is FF */
	case LF:
		if (ip->linefeed_newline)
		  ite_crlf (ip, sp);
		else
		  ite_lf (ip, sp);
		break;

	case CR:
		ite_cr (ip, sp);
		break;
	
	case BS:
		if (--ip->curx < 0)
			ip->curx = 0;
		else
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
		break;

	case HT:
		if (ip->curx < TABEND(unit)) {
			n = TABSIZE - (ip->curx & (TABSIZE - 1));
			ip->curx += n;
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
		} else
			itecheckwrap(ip, sp);
		break;

	case BEL:
		if (ite_tty[unit] == kbd_tty)
			kbdbell();
		break;

	case SO:
		ip->GL = ip->G1;
		break;
		
	case SI:
		ip->GL = ip->G0;
		break;

	case ENQ:
		/* send answer-back message !! */
		break;

	case CAN:
		ip->escape = 0;	/* cancel any escape sequence in progress */
		break;
		
	case SUB:
		ip->escape = 0;	/* dito, but see below */
		/* should also display a reverse question mark!! */
		break;

	case ESC:
		ip->escape = ESC;
		break;


	/* now it gets weird.. 8bit control sequences.. */
	case IND:	/* index: move cursor down, scroll */
		ite_lf (ip, sp);
		break;
		
	case NEL:	/* next line. next line, first pos. */
		ite_crlf (ip, sp);
		break;

	case HTS:	/* set horizontal tab */
		/* not yet supported */
		break;
		
	case RI:	/* reverse index */
		ite_rlf (ip, sp);
		break;

	case SS2:	/* go into G2 for one character */
		/* not yet supported */
		break;
		
	case SS3:	/* go into G3 for one character */
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

	default:
		if (c < ' ' || c == DEL)
			break;
		if (ip->imode)
			ite_inchar(ip, sp, 1);
		if ((ip->attribute & ATTR_INV) || attrtest(ip, ATTR_INV)) {
			attrset(ip, ATTR_INV);
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_INV);
		}			
		else
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_NOR);
		(*sp->ite_cursor)(ip, DRAW_CURSOR);
		itecheckwrap(ip, sp);
		break;
	}
}

itecheckwrap(ip, sp)
     register struct ite_softc *ip;
     register struct itesw *sp;
{
	if (++ip->curx == ip->cols) {
		ip->curx = 0;
		clr_attr(ip, ATTR_INV);
		if (++ip->cury >= ip->bottom_margin + 1) {
			ip->cury = ip->bottom_margin;
			(*sp->ite_scroll)(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip, sp, ip->cury, 0);
			return;
		}
	}
	(*sp->ite_cursor)(ip, MOVE_CURSOR);
}

/*
 * Console functions
 */
#include "../amiga/cons.h"
#include "grfioctl.h"
#include "grfvar.h"

#ifdef DEBUG
/*
 * Minimum ITE number at which to start looking for a console.
 * Setting to 0 will do normal search, 1 will skip first ITE device,
 * NITE will skip ITEs and use serial port.
 */
int	whichconsole = 0;
#endif

itecnprobe(cp)
	struct consdev *cp;
{
	register struct ite_softc *ip;
	int i, maj, unit, pri;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == iteopen)
			break;

	/* urk! */
	grfconfig();

	/* check all the individual displays and find the best */
	unit = -1;
	pri = CN_DEAD;
	for (i = 0; i < NITE; i++) {
		struct grf_softc *gp = &grf_softc[i];

		ip = &ite_softc[i];
		if ((gp->g_flags & GF_ALIVE) == 0)
			continue;
		ip->flags = (ITE_ALIVE|ITE_CONSOLE);

		/* XXX - we need to do something about mapping these */
		switch (gp->g_type) {
		case GT_CUSTOMCHIPS:
		        ip->type = ITE_CUSTOMCHIPS;
		        break;
		        
		case GT_TIGA_A2410:
		        ip->type = ITE_TIGA_A2410;
		        break;
		}
#ifdef DEBUG
		if (i < whichconsole)
			continue;
#endif
		if ((int)gp->g_type == GT_CUSTOMCHIPS) {
			pri = CN_INTERNAL;
			unit = i;
		} else if (unit < 0) {
			pri = CN_NORMAL;
			unit = i;
		}
	}

	/* initialize required fields */
	cp->cn_dev = makedev(maj, unit);
	cp->cn_tp = ite_tty[unit];
	cp->cn_pri = pri;
}

itecninit(cp)
	struct consdev *cp;
{
	int unit = UNIT(cp->cn_dev);
	struct ite_softc *ip = &ite_softc[unit];
	ip->attrbuf = console_attributes;
	iteinit(cp->cn_dev);
	ip->flags |= (ITE_ACTIVE|ITE_ISCONS);
	kbd_tty = ite_tty[unit];
	kbdenable();
}

/*ARGSUSED*/
itecngetc(dev)
	dev_t dev;
{
	register int c;

        do
          {
            c = kbdgetcn ();
            c = itefilter (c, ITEFILT_CONSOLE);
          }
        while (c == -1);

	return(c);
}

itecnputc(dev, c)
	dev_t dev;
	int c;
{
	static int paniced = 0;
	struct ite_softc *ip = &ite_softc[UNIT(dev)];
	extern char *panicstr;

	if (panicstr && !paniced &&
	    (ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE) {
		(void) iteon(dev, 3);
		paniced = 1;
	}
	iteputchar(c, dev);
}
#endif
