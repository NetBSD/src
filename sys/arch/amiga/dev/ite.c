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
 *	$Id: ite.c,v 1.4 1993/10/30 23:41:17 mw Exp $
 *
 * Original author: unknown
 * Amiga author:: Markus Wild
 * Other contributors: Bryan Ford (improved vt100 compability)
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
#include "../amiga/cons.h"

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
extern void iteputchar (register int c, dev_t dev);
extern int iteprecheckwrap (register struct ite_softc *ip, register struct itesw *sp);
extern int itecheckwrap (register struct ite_softc *ip, register struct itesw *sp);
extern int itecnprobe (struct consdev *cp);
extern int itecninit (struct consdev *cp);
extern int itecngetc (dev_t dev);
extern int itecnputc (dev_t dev, int c);
static void repeat_handler (int a0, int a1);
static void inline ite_reset(struct ite_softc *ip);
static void ite_dnchar (struct ite_softc *ip, struct itesw *sp, int n);
static void ite_inchar (struct ite_softc *ip, struct itesw *sp, int n);
static void ite_clrtoeol (struct ite_softc *ip, struct itesw *sp);
static void ite_clrtobol (struct ite_softc *ip, struct itesw *sp);
static void ite_clrline (struct ite_softc *ip, struct itesw *sp);
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

int retina_scroll(),	retina_init(),		retina_deinit();
int retina_clear(),	retina_putc(),		retina_cursor();

struct itesw itesw[] =
{
	customc_init,		customc_deinit,		0,
	0,			0,			0,
	
	tiga_init,		tiga_deinit,		tiga_clear,
	tiga_putc,		tiga_cursor,		tiga_scroll,
	
	retina_init,		retina_deinit,		retina_clear,
	retina_putc,		retina_cursor,		retina_scroll,
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
struct	ite_softc *kbd_ip = NULL;

struct	tty ite_cons1, ite_cons2;
#if 0
struct	tty *ite_tty[NITE] = { &ite_cons1, &ite_cons2 };
#else
struct	tty *ite_tty[NITE];
/* ugh.. */
static int delayed_con_tty = -1;  /* if >= 0 set cn_tp later to that tty.. */
#endif
struct  ite_softc ite_softc[NITE];

int	itestart();
extern	int ttrstrt();
extern	struct tty *constty;

/* These are (later..) settable via an ioctl */
int	start_repeat_timo = 20;	/* /100: initial timeout till pressed key repeats */
int	next_repeat_timo  = 3;  /* /100: timeout when repeating for next char */

#ifdef DO_WEIRD_ATTRIBUTES
/*
 * Primary attribute buffer to be used by the first bitmapped console
 * found. Secondary displays alloc the attribute buffer as needed.
 * Size is based on a 68x128 display, which is currently our largest.
 */
u_char  console_attributes[0x2200];
#endif
u_char	console_tabs[256 * NITE];

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
	if (tp && (kbd_tty == NULL || kbd_tty == tp)) {
		kbd_tty = tp;
		kbd_ip = ip;
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

	bcopy (&ascii_kbdmap, &kbdmap, sizeof (struct kbdmap));
	
	ip->cursorx = 0;
	ip->cursory = 0;
	(*itesw[ip->type].ite_init)(ip);

#ifdef DO_WEIRD_ATTRIBUTES
	if (ip->attrbuf == NULL)
		ip->attrbuf = (u_char *)
			malloc(ip->rows * ip->cols, M_DEVBUF, M_WAITOK);
	bzero(ip->attrbuf, (ip->rows * ip->cols));
#endif
	if (! ip->tabs)
	  {
	    /* can't use malloc, as this buffer might be used before
	       the allocators are initialized (console!) */
	    ip->tabs = &console_tabs[unit * 256];
	  }

	ite_reset (ip);

	(*itesw[ip->type].ite_cursor)(ip, DRAW_CURSOR);
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

	if (unit >= NITE)
	  return ENXIO;

	if(!ite_tty[unit]) {
#if 0
		MALLOC(tp, struct tty *, sizeof(struct tty), M_TTYS, M_WAITOK);
		bzero(tp, sizeof(struct tty));
		ite_tty[unit] = tp;
#else
		tp = ite_tty[unit] = ttymalloc();
		/* HA! caught it finally... */
		if (unit == delayed_con_tty)
		  {
		    extern struct consdev *cn_tab;
		    extern struct tty *cn_tty;

		    kbd_tty = tp;
		    kbd_ip = ip;
		    kbdenable ();
		    cn_tty = cn_tab->cn_tp = tp;
		    delayed_con_tty = -1;
		  }
#endif
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
		ip->open_cnt++;
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
	int unit = UNIT(dev);
	register struct tty *tp = ite_tty[unit];
	register struct ite_softc *ip = &ite_softc[unit];

#if 0
	/* aliasing with /dev/console can lead to such weird problems... */
	if (ip->open_cnt-- > 0)
	  return 0;
#endif

	if (tp)
	  {
	    (*linesw[tp->t_line].l_close)(tp, flag);
	    ttyclose(tp);
	  }
	iteoff(dev, 0);
#if 0
	if (tp != &ite_cons)
	  {
#if 0
	    FREE(tp, M_TTYS);
#else
	    ttyfree (tp);
#endif
	    ite_tty[UNIT(dev)] = (struct tty *)NULL;
	  }
#endif
	return(0);
}

iteread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = ite_tty[UNIT(dev)];
	int rc;

	if (! tp)
	  return ENXIO;

	rc = ((*linesw[tp->t_line].l_read)(tp, uio, flag));
	return rc;
}

itewrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
{
	int unit = UNIT(dev);
	register struct tty *tp = ite_tty[unit];

	if (! tp)
	  return ENXIO;

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

	if (! tp)
	  return ENXIO;


	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, addr, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, addr, flag);
	if (error >= 0)
		return (error);

	switch (cmd)
	  {
	  case ITELOADKMAP:
	    if (addr)
	      {
		bcopy (addr, &kbdmap, sizeof (struct kbdmap));
		return 0;
	      }
	    else
	      return EFAULT;

	  case ITEGETKMAP:
	    if (addr)
	      {
		bcopy (&kbdmap, addr, sizeof (struct kbdmap));
		return 0;
	      }
	    else
	      return EFAULT;
	  }


	return (ENOTTY);
}

itestart(tp)
	register struct tty *tp;
{
	register int cc, s;
	int unit = UNIT(tp->t_dev);
	register struct ite_softc *ip = &ite_softc[unit];
	int hiwat = 0;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY;
	cc = tp->t_outq.c_cc;
	if (cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) &tp->t_outq);
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
	(*itesw[ip->type].ite_cursor)(ip, START_CURSOROPT);
	while (--cc >= 0) {
		register int c;

		c = getc(&tp->t_outq);
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
	(*itesw[ip->type].ite_cursor)(ip, END_CURSOROPT);
	if (hiwat) {
		tp->t_state |= TS_TIMEOUT;
		timeout((timeout_t) ttrstrt, (caddr_t) tp, 1);
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
  /* leave it up to itefilter() to possibly install a new callout entry
     to reinvoke repeat_handler() */
  if (last_char)
    {
      /* don't call itefilter directly, we're at spl6 here, and have 
	 blocked out way too much stuff. Besides, the keyboard wouldn't
	 even have a chance to tell us about key-up events if we
	 did.. */
      add_sicallback (itefilter, last_char, ITEFILT_REPEATER);

      /* itefilter (last_char, ITEFILT_REPEATER); */
    }
}

static void inline
itesendch (int ch)
{
  (*linesw[kbd_tty->t_line].l_rint)(ch, kbd_tty);
}


int
itefilter(c, caller)
     register u_char c;
     enum caller caller;
{
  static u_char mod = 0;
  static u_char last_dead = 0;
  register unsigned char code, *str;
  u_char up, mask, i;
  struct key key;
  int s;
  
  if (caller != ITEFILT_CONSOLE && kbd_tty == NULL)
     return;

  /* have to make sure we're at spltty in here */
  s = spltty ();

  /* keyboard interrupts come at priority 2, while softint-
     generated keyboard-repeat interrupts come at level 1.
     So, to not allow a key-up event to get thru before
     a repeat for the key-down, we remove any outstanding
     callout requests.. */
  rem_sicallback (itefilter);

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
      splx (s);
      return -1;
    }  

  /* no matter which character we're repeating, stop it if we get a key-up
     event. I think this is the same thing amigados does. */
  if (up)
    {
      if (tout_pending)
        {
          untimeout ((timeout_t) repeat_handler, 0);
          tout_pending = 0;
	  last_char = 0;
        }
      splx (s);
      return -1;
    }


  /* intercept LAlt-LMeta-F1 here to switch back to original ascii-keymap.
     this should probably be configurable.. */
  if (mod == (KBD_MOD_LALT|KBD_MOD_LMETA) && c == 0x50)
    {
      bcopy (&ascii_kbdmap, &kbdmap, sizeof (struct kbdmap));
      splx (s);
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
  if (!tout_pending && caller == ITEFILT_TTY && kbd_ip->key_repeat)
    {
      tout_pending = 1;
      last_char = c;
      timeout ((timeout_t) repeat_handler, 0, start_repeat_timo);
    }
  else if (!tout_pending && caller == ITEFILT_REPEATER && kbd_ip->key_repeat)
    {
      tout_pending = 1;
      last_char = c;
      timeout ((timeout_t) repeat_handler, 0, next_repeat_timo);
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
	  splx (s);
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
  if (! (key.mode & KBD_MODE_STRING) 
      && (!(key.mode & KBD_MODE_KPAD) || !kbd_ip->keypad_appmode))
    {
      if (mod & KBD_MOD_CTRL)
        code &= 0x1f;

      if (mod & KBD_MOD_META)
        code |= 0x80;
    }
  else if ((key.mode & KBD_MODE_KPAD) && kbd_ip->keypad_appmode)
    {
      static char *in  = "0123456789-+.\r()/*";
      static char *out = "pqrstuvwxymlnMPQRS";
      char *cp;
      
      if (caller != ITEFILT_CONSOLE && (cp = index (in, code)))
	{
	  /* keypad-appmode sends SS3 followed by the above translated
	     character */
	  itesendch (27); itesendch ('O');
	  itesendch (out[cp - in]);
	  splx (s);
	  return -1;
	}
    }
  else
    {
      /* strings are only supported in normal tty mode, not in console mode */
      if (caller != ITEFILT_CONSOLE)
        {
	  /* *NO* I don't like this.... */
	  static u_char app_cursor[] = {
	    3, 27, 'O', 'A',
	    3, 27, 'O', 'B',
	    3, 27, 'O', 'C',
	    3, 27, 'O', 'D'};

          str = kbdmap.strings + code;
	  /* if this is a cursor key, AND it has the default keymap setting,
	     AND we're in app-cursor mode, switch to the above table. This
	     is *nasty* ! */
	  if (c >= 0x4c && c <= 0x4f && kbd_ip->cursor_appmode 
	      && !bcmp (str, "\x03\x1b[", 3) && index ("ABCD", str[3]))
	    str = app_cursor + 4 * (str[3] - 'A');

          /* using a length-byte instead of 0-termination allows to embed \0 into
             strings, although this is not used in the default keymap */
          for (i = *str++; i; i--)
            itesendch(*str++);
        }
      splx (s);
      return -1;
    }

  if (caller == ITEFILT_CONSOLE)
    {
      /* do the conversion here because raw console input doesn't go thru 
	 tty conversions */
      code = code == '\r' ? '\n' : code;
      splx (s);
      return code;
    }
  else
    /* NOTE: *don't* do any cr->crlf conversion here, this is input 
	     processing, the mentioned conversion should only be 
	     done for output processing (for input, it is not 
	     terminal-specific but depends on tty-settings!) */
    (*linesw[kbd_tty->t_line].l_rint)(code, kbd_tty);

  splx (s);
  return -1;
}


/* helper functions, makes the code below more readable */
static void inline
ite_sendstr (ip, str)
    struct ite_softc *ip;
    char *str;
{
  while (*str)
    itesendch (*str++);
}

static void
alignment_display(struct ite_softc *ip, struct itesw *sp)
{
  int i, j;

  for (j = 0; j < ip->rows; j++)
    for (i = 0; i < ip->cols; i++)
      (*sp->ite_putc)(ip, 'E', j, i, ATTR_NOR);
  attrclr(ip, 0, 0, ip->rows, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
snap_cury(struct ite_softc *ip, struct itesw *sp)
{
  if (ip->inside_margins)
    {
      if (ip->cury < ip->top_margin)
	ip->cury = ip->top_margin;
      if (ip->cury > ip->bottom_margin)
	ip->cury = ip->bottom_margin;
    }
}

static void inline
ite_dnchar(ip, sp, n)
     struct ite_softc *ip;
     struct itesw *sp;
     int n;
{
  n = MIN(n, ip->cols - ip->curx);
  if (n < ip->cols - ip->curx)
    {
      (*sp->ite_scroll)(ip, ip->cury, ip->curx + n, n, SCROLL_LEFT);
      attrmov(ip, ip->cury, ip->curx + n, ip->cury, ip->curx,
	      1, ip->cols - ip->curx - n);
      attrclr(ip, ip->cury, ip->cols - n, 1, n);
    }
  while (n-- > 0)
    (*sp->ite_putc)(ip, ' ', ip->cury, ip->cols - n - 1, ATTR_NOR);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_inchar(ip, sp, n)
     struct ite_softc *ip;
     struct itesw *sp;
     int n;
{
  n = MIN(n, ip->cols - ip->curx);
  if (n < ip->cols - ip->curx)
    {
      (*sp->ite_scroll)(ip, ip->cury, ip->curx, n, SCROLL_RIGHT);
      attrmov(ip, ip->cury, ip->curx, ip->cury, ip->curx + n,
	      1, ip->cols - ip->curx - n);
      attrclr(ip, ip->cury, ip->curx, 1, n);
    }
  while (n--)
    (*sp->ite_putc)(ip, ' ', ip->cury, ip->curx + n, ATTR_NOR);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrtoeol(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  int y = ip->cury, x = ip->curx;
  if (ip->cols - x > 0)
    {
      (*sp->ite_clear)(ip, y, x, 1, ip->cols - x);
      attrclr(ip, y, x, 1, ip->cols - x);
      (*sp->ite_cursor)(ip, DRAW_CURSOR);
    }
}

static void inline
ite_clrtobol(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  int y = ip->cury, x = MIN(ip->curx + 1, ip->cols);
  (*sp->ite_clear)(ip, y, 0, 1, x);
  attrclr(ip, y, 0, 1, x);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_clrline(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  int y = ip->cury;
  (*sp->ite_clear)(ip, y, 0, 1, ip->cols);
  attrclr(ip, y, 0, 1, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}



static void inline
ite_clrtoeos(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  ite_clrtoeol(ip, sp);
  if (ip->cury < ip->rows - 1)
    {
      (*sp->ite_clear)(ip, ip->cury + 1, 0, ip->rows - 1 - ip->cury, ip->cols);
      attrclr(ip, ip->cury, 0, ip->rows - ip->cury, ip->cols);
      (*sp->ite_cursor)(ip, DRAW_CURSOR);
    }
}

static void inline
ite_clrtobos(ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  ite_clrtobol(ip, sp);
  if (ip->cury > 0)
    {
      (*sp->ite_clear)(ip, 0, 0, ip->cury, ip->cols);
      attrclr(ip, 0, 0, ip->cury, ip->cols);
      (*sp->ite_cursor)(ip, DRAW_CURSOR);
    }
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
  /* interesting.. if the cursor is outside the scrolling
     region, this command is simply ignored.. */
  if (ip->cury < ip->top_margin || ip->cury > ip->bottom_margin)
    return;

  n = MIN(n, ip->bottom_margin + 1 - ip->cury);
  if (n <= ip->bottom_margin - ip->cury)
    {
      (*sp->ite_scroll)(ip, ip->cury + n, 0, n, SCROLL_UP);
      attrmov(ip, ip->cury + n, 0, ip->cury, 0,
	      ip->bottom_margin + 1 - ip->cury - n, ip->cols);
    }
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
  /* interesting.. if the cursor is outside the scrolling
     region, this command is simply ignored.. */
  if (ip->cury < ip->top_margin || ip->cury > ip->bottom_margin)
    return;

  n = MIN(n, ip->bottom_margin + 1 - ip->cury);
  if (n <= ip->bottom_margin - ip->cury)
    {
      (*sp->ite_scroll)(ip, ip->cury, 0, n, SCROLL_DOWN);
      attrmov(ip, ip->cury, 0, ip->cury + n, 0,
	      ip->bottom_margin + 1 - ip->cury - n, ip->cols);
    }
  (*sp->ite_clear)(ip, ip->cury, 0, n, ip->cols);
  attrclr(ip, ip->cury, 0, n, ip->cols);
  (*sp->ite_cursor)(ip, DRAW_CURSOR);
}

static void inline
ite_lf (ip, sp)
     struct ite_softc *ip;
     struct itesw *sp;
{
  ++ip->cury;
  if ((ip->cury == ip->bottom_margin+1) || (ip->cury == ip->rows))
    {
      ip->cury--;
      (*sp->ite_scroll)(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
      ite_clrline(ip, sp);
    }
  (*sp->ite_cursor)(ip, MOVE_CURSOR);
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
  ip->cury--;
  if ((ip->cury < 0) || (ip->cury == ip->top_margin - 1))
    {
      ip->cury++;
      (*sp->ite_scroll)(ip, ip->top_margin, 0, 1, SCROLL_DOWN);
      ite_clrline(ip, sp);
    }
  (*sp->ite_cursor)(ip, MOVE_CURSOR);
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
  return *cp ? (char *) cp : 0;
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
  
  return n;	/* don't "n ? n : 1" here, <CSI>0m != <CSI>1m ! */
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
  ip->emul_level = EMUL_VT300_7;
  ip->eightbit_C1 = 0;
  ip->top_margin = 0; ip->bottom_margin = ip->rows - 1;
  ip->inside_margins = 0;
  ip->linefeed_newline = 0;
  ip->auto_wrap = 0;
  ip->cursor_appmode = 0;
  ip->keypad_appmode = 0;
  ip->imode = 0;
  ip->key_repeat = 1;

#ifdef DO_WEIRD_ATTRIBUTES
  bzero(ip->attrbuf, (ip->rows * ip->cols));
#endif
  bzero (ip->tabs, ip->cols);
  for (i = 0; i < ip->cols; i++)
    ip->tabs[i] = (i & 7) == 0;
}


void
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

#if 0
	if (ite_tty[unit])
	  if ((ite_tty[unit]->t_cflag & CSIZE) == CS7
	      || (ite_tty[unit]->t_cflag & PARENB))
	    c &= 0x7f;
#endif

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
		    break; /* and fall into the next switch below (same for all `break') */
		    
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
		    
		  
		  /* locking shift modes (as you might guess, not yet supported..) */
		  case '`':
		    ip->GR = ip->G1;
		    ip->escape = 0;
		    return;
		    
		  case 'n':
		    ip->GL = ip->G2;
		    ip->escape = 0;
		    return;
		    
		  case '}':
		    ip->GR = ip->G2;
		    ip->escape = 0;
		    return;
		    
		  case 'o':
		    ip->GL = ip->G3;
		    ip->escape = 0;
		    return;
		    
		  case '|':
		    ip->GR = ip->G3;
		    ip->escape = 0;
		    return;
		    
		  
		  /* font width/height control */
		  case '#':
		    ip->escape = '#';
		    return;
		    
		    
		  /* hard terminal reset .. */
		  case 'c':
		    ite_reset (ip);
		    (*itesw[ip->type].ite_cursor)(ip, MOVE_CURSOR);
		    ip->escape = 0;
		    return;


		  case '7':
		    ip->save_curx = ip->curx;
		    ip->save_cury = ip->cury;
		    ip->save_attribute = ip->attribute;
		    ip->escape = 0;
		    return;
		    
		  case '8':
		    ip->curx = ip->save_curx;
		    ip->cury = ip->save_cury;
		    ip->attribute = ip->save_attribute;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    ip->escape = 0;
		    return;
		    
		  case '=':
		    ip->keypad_appmode = 1;
		    ip->escape = 0;
		    return;
		    
		  case '>':
		    ip->keypad_appmode = 0;
		    ip->escape = 0;
		    return;
		  
		  case 'Z':	/* request ID */
		    if (ip->emul_level == EMUL_VT100)
		      ite_sendstr (ip, "\033[61;0c");
		    else
		      ite_sendstr (ip, "\033[63;0c");
		    ip->escape = 0;
		    return;

		  /* default catch all for not recognized ESC sequences */
		  default:
		    ip->escape = 0;
		    return;
		  }
		break;


	      case '(':
	      case ')':
		ip->escape = 0;
		return;


	      case ' ':
	        switch (c)
	          {
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
		switch (c)
		  {
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
		    alignment_display (ip, sp);
		    ip->escape = 0;
		    return;
		    
		  default:
		    ip->escape = 0;
		    return;
		  }
		break;
		  


	      case CSI:
	        /* the biggie... */
	        switch (c)
	          {
	          case '0': case '1': case '2': case '3': case '4':
	          case '5': case '6': case '7': case '8': case '9':
	          case ';': case '\"': case '$': case '>':
	            if (ip->ap < ip->argbuf + ARGBUF_SIZE)
	              *ip->ap++ = c;
	            return;

		  case BS:
		    /* you wouldn't believe such perversion is possible?
		       it is.. BS is allowed in between cursor sequences
		       (at least), according to vttest.. */
		    if (--ip->curx < 0)
		      ip->curx = 0;
		    else
		      (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    break;

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
	            return;
	            
	          
	          case '?':
		    *ip->ap = 0;
	            ip->escape = '?';
	            ip->ap = ip->argbuf;
	            return;


		  case 'c':
  		    *ip->ap = 0;
		    if (ip->argbuf[0] == '>')
		      {
		        ite_sendstr (ip, "\033[>24;0;0;0c");
		      }
		    else switch (ite_zargnum(ip))
		      {
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
		    switch (ite_zargnum(ip))
		      {
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
		    switch (ite_zargnum(ip))
		      {
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
		    switch (ite_zargnum(ip))
		      {
		      case 0:
			if (ip->curx < ip->cols)
			  ip->tabs[ip->curx] = 0;
			break;
		      case 3:
		        for (n = 0; n < ip->cols; n++)
		          ip->tabs[n] = 0;
			break;
		      }
		    ip->escape = 0;
		    return;

	          
  	          case 'h': case 'l':
		    n = ite_zargnum (ip);
		    switch (n)
		      {
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
		    ite_dnline (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return;

		  
		  case 'L':
		    ite_inline (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return;


		  case 'P':
		    ite_dnchar (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return;
		    

		  case '@':
		    ite_inchar (ip, sp, ite_argnum (ip));
	            ip->escape = 0;
	            return;


		  case 'G':
		    /* this one was *not* in my vt320 manual but in 
		       a vt320 termcap entry.. who is right?
		       It's supposed to set the horizontal cursor position. */
		    *ip->ap = 0;
		    x = atoi (ip->argbuf);
		    if (x) x--;
		    ip->curx = MIN(x, ip->cols - 1);
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
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
		    ip->cury = MIN(y, ip->rows - 1);
		    ip->escape = 0;
		    snap_cury(ip, sp);
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
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
		    ip->cury = MIN(y, ip->rows - 1);
		    ip->curx = MIN(x, ip->cols - 1);
		    ip->escape = 0;
		    snap_cury(ip, sp);
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return;
		    
		  case 'A':		    
		    n = ite_argnum (ip);
		    n = ip->cury - (n ? n : 1);
		    if (n < 0) n = 0;
		    if (ip->inside_margins)
		      n = MAX(ip->top_margin, n);
		    else if (n == ip->top_margin - 1)
		      /* allow scrolling outside region, but don't scroll out
			 of active region without explicit CUP */
		      n = ip->top_margin;
		    ip->cury = n;
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return;
		  
		  case 'B':
		    n = ite_argnum (ip);
		    n = ip->cury + (n ? n : 1);
		    n = MIN(ip->rows - 1, n);
		    if (ip->inside_margins)
		      n = MIN(ip->bottom_margin, n);
		    else if (n == ip->bottom_margin + 1)
		      /* allow scrolling outside region, but don't scroll out
			 of active region without explicit CUP */
		      n = ip->bottom_margin;
		    ip->cury = n;
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return;
		  
		  case 'C':
		    n = ite_argnum (ip);
		    n = n ? n : 1;
		    ip->curx = MIN(ip->curx + n, ip->cols - 1);
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return;
		  
		  case 'D':
		    n = ite_argnum (ip);
		    n = n ? n : 1;
		    n = ip->curx - n;
		    ip->curx = n >= 0 ? n : 0;
		    ip->escape = 0;
		    (*sp->ite_cursor)(ip, MOVE_CURSOR);
		    clr_attr (ip, ATTR_INV);
		    return;
		  
		    


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
	            return;


		  case 'K':
		    n = ite_zargnum (ip);
		    if (n == 0)
		      ite_clrtoeol(ip, sp);
		    else if (n == 1)
		      ite_clrtobol(ip, sp);
		    else if (n == 2)
		      ite_clrline(ip, sp);
		    ip->escape = 0;
		    return;


		  case 'X':
		    n = ite_argnum(ip) - 1;
		    n = MIN(n, ip->cols - 1 - ip->curx);
		    for (; n >= 0; n--)
		      {
			attrclr(ip, ip->cury, ip->curx + n, 1, 1);
			(*sp->ite_putc)(ip, ' ', ip->cury, ip->curx + n, ATTR_NOR);
		      }
		    ip->escape = 0;
		    return;

	          
	          case '}': case '`':
	            /* status line control */
	            ip->escape = 0;
	            return;


		  case 'r':
		    *ip->ap = 0;
		    x = atoi (ip->argbuf);
		    x = x ? x : 1;
		    y = ip->rows;
		    cp = index (ip->argbuf, ';');
		    if (cp)
		      {
			y = atoi (cp + 1);
			y = y ? y : ip->rows;
		      }
		    if (y - x < 2)
		      {
			/* if illegal scrolling region, reset to defaults */
			x = 1;
			y = ip->rows;
		      }
		    x--;
		    y--;
		    ip->top_margin = MIN(x, ip->rows - 1);
		    ip->bottom_margin = MIN(y, ip->rows - 1);
		    if (ip->inside_margins)
		      {
			ip->cury = ip->top_margin;
			ip->curx = 0;
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
		      }
		    ip->escape = 0;
		    return;
		    
		  
		  case 'm':
		    /* big attribute setter/resetter */
		    {
		      char *cp;
		      *ip->ap = 0;
		      /* kludge to make CSIm work (== CSI0m) */
		      if (ip->ap == ip->argbuf)
		        ip->ap++;
		      for (cp = ip->argbuf; cp < ip->ap; )
		        {
			  switch (*cp)
			    {
			    case 0:
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
	      	switch (c)
	      	  {
	          case '0': case '1': case '2': case '3': case '4':
	          case '5': case '6': case '7': case '8': case '9':
	          case ';': case '\"': case '$':
		    /* Don't fill the last character; it's needed.  */
		    /* XXX yeah, where ?? */
	            if (ip->ap < ip->argbuf + ARGBUF_SIZE - 1)
	              *ip->ap++ = c;
	            return;


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
		    return;


  		  case 'h': case 'l':
		    n = ite_zargnum (ip);
		    switch (n)
		      {
		      case 1:
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
			ip->curx = 0;
			ip->cury = ip->inside_margins ? ip->top_margin : 0;
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
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
			(*itesw[ip->type].ite_cursor)(ip, (c == 'h') ? DRAW_CURSOR : ERASE_CURSOR);
			break;
		      }
		    ip->escape = 0;
		    return;
		    
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

	case VT:	/* VT is treated like LF */
	case FF:	/* so is FF */
	case LF:
		/* cr->crlf distinction is done here, on output, 
		   not on input! */
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
		for (n = ip->curx + 1; n < ip->cols; n++) {
			if (ip->tabs[n]) {
				ip->curx = n;
				(*sp->ite_cursor)(ip, MOVE_CURSOR);
				break;
			}
		}
		break;

	case BEL:
		if (kbd_tty && ite_tty[unit] == kbd_tty)
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
		if (ip->curx < ip->cols)
		  ip->tabs[ip->curx] = 1;
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
		iteprecheckwrap(ip, sp);
#ifdef DO_WEIRD_ATTRIBUTES
		if ((ip->attribute & ATTR_INV) || attrtest(ip, ATTR_INV)) {
			attrset(ip, ATTR_INV);
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_INV);
		}			
		else
			(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ATTR_NOR);
#else
		(*sp->ite_putc)(ip, c, ip->cury, ip->curx, ip->attribute);
#endif
		(*sp->ite_cursor)(ip, DRAW_CURSOR);
		itecheckwrap(ip, sp);
		break;
	}
}

iteprecheckwrap(ip, sp)
     register struct ite_softc *ip;
     register struct itesw *sp;
{
	if (ip->auto_wrap && ip->curx == ip->cols) {
		ip->curx = 0;
		clr_attr(ip, ATTR_INV);
		if (++ip->cury >= ip->bottom_margin + 1) {
			ip->cury = ip->bottom_margin;
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
			(*sp->ite_scroll)(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(ip, sp);
		} else
			(*sp->ite_cursor)(ip, MOVE_CURSOR);
	}
}

itecheckwrap(ip, sp)
     register struct ite_softc *ip;
     register struct itesw *sp;
{
#if 0
	if (++ip->curx == ip->cols) {
		if (ip->auto_wrap) {
			ip->curx = 0;
			clr_attr(ip, ATTR_INV);
			if (++ip->cury >= ip->bottom_margin + 1) {
				ip->cury = ip->bottom_margin;
				(*sp->ite_cursor)(ip, MOVE_CURSOR);
				(*sp->ite_scroll)(ip, ip->top_margin + 1, 0, 1, SCROLL_UP);
				ite_clrtoeol(ip, sp);
				return;
			}
		} else
			/* stay there if no autowrap.. */
			ip->curx--;
	}
#else
	if (ip->curx < ip->cols) {
		ip->curx++;
		(*sp->ite_cursor)(ip, MOVE_CURSOR);
	}
#endif
}

/*
 * Console functions
 */
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
		        
		case GT_RETINA:
			ip->type = ITE_RETINA;
			break;
		}
#ifdef DEBUG
		if (i < whichconsole)
			continue;
#endif
		if ((int)gp->g_type == GT_CUSTOMCHIPS) {
			pri = CN_INTERNAL;
			unit = i;
		} else /* if (unit < 0) */ {
			pri = CN_NORMAL;
			unit = i;
		}

	}

	/* initialize required fields */
	cp->cn_dev = makedev(maj, unit);
#if 0
	cp->cn_tp = ite_tty[unit];
#else
	delayed_con_tty = unit;
#endif
	cp->cn_pri = pri;
}

itecninit(cp)
	struct consdev *cp;
{
	int unit;
	struct ite_softc *ip;

	iteinit(cp->cn_dev);
	unit = UNIT(cp->cn_dev);
	ip = &ite_softc[unit];

#ifdef DO_WEIRD_ATTRIBUTES
	ip->attrbuf = console_attributes;
#endif
	ip->flags |= (ITE_ACTIVE|ITE_ISCONS);
	/* if this is the console, NEVER close the device! */
	ip->open_cnt++;
#if 0
	/* have to delay this too.. sigh.. */
	kbd_tty = ite_tty[unit];
	kbd_ip = ip;
	kbdenable();
#endif
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

	if (panicstr && !paniced &&
	    (ip->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE) {
		(void) iteon(dev, 3);
		paniced = 1;
	}
	iteputchar(c, dev);
}
#endif
