/*
 * Copyright (c) 1993 Paul Mackerras.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: gsp.c,v 1.2 1994/06/18 12:10:14 paulus Exp $
 */

#include "gsp.h"
#if NGSP > 0

/*
 * Code to drive keyboard & GSP-controlled display for DA30
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <dev/cons.h>
#include <da30/dev/kbd.h>
#include <da30/da30/iio.h>

#define NGSP	1

struct gsp_regs {
    u_long	adrs;		/* address register */
    u_short	ctrl;		/* control/status */
    u_short	data;		/* memory R/W data register */
};

typedef volatile struct gsp_regs *gsp_ptr;

/* Bits in ctrl */
#define LBL	0x2000		/* lower byte last */
#define INCR	0x1000		/* increment address on read */
#define INCW	0x0800		/* increment address on write */

/* Standard addresses in GSP memory */
#define PUT_PTR_ADRS	0x320000	/* put pointer in ring buffer */
#define PUT_HI_PTR_ADRS	0x320010	/* put pointer high word */
#define GET_PTR_ADRS	0x320020	/* get pointer (low word) */
#define GSP_MODE_ADRS	0x320040	/* GSP mode word */
#define KBD_MODE_ADRS	0x320050	/* keyboard mode word */
#define LCL_MODE_ADRS	0x320060	/* GSP local modes */
#define HIST_REQ_ADRS	0x320070	/* history scrollback request */
#define GSP_CMD_ADRS	0x320080	/* GSP command code */
#define MAX_COL_ADRS	0x320140	/* # columns */
#define MAX_ROW_ADRS	0x320150	/* # rows */
#define SCR_WID_ADRS	0x320180	/* screen pixel width */
#define SCR_HT_ADRS	0x320190	/* screen pixel height */

/* Bits in GSP mode word */
#define GMODE_HOLD	1		/* hold screen */
#define GMODE_FLUSH	2		/* flush GSP input queue */
#define GMODE_ALTSCRN	4		/* use alternate screen */
#define GMODE_DISPCTRL	8		/* display control chars */

struct kbd {
    short	prefix;		/* 0 => seen 0x55, 0x100 => 0xAA */
    short	last;		/* last char seen, for autorepeat */
};

/*
 * State.
 */
struct tty *gsp_tty[NGSP];

struct gspsoftc {
    struct device	dev;
    gsp_ptr		GSP;
    struct tty		*tty;
    int			open_as_console;
    int			open;
    int			kbd_dev;
    struct kbd		kbd;
};

/*
 * We use an SCC channel connected to the keyboard as the input
 * side of this device.
 */
int kbd_zsunit = 3;		/* minor no. of keyboard SCC channel */

/* Termios values for keyboard */
struct termios kbdcn_termios = {
    IGNBRK,			/* iflag */
    0,				/* oflag */
    CREAD | CS8 | CLOCAL,	/* cflag */
    0,				/* lflag */
    {0},
    B4800, B4800,		/* speeds */
};

void	gspstart __P((struct tty *));

/*
 * Autoconfiguration stuff.
 */
void gspattach __P((struct device *, struct device *, void *));
int  gspmatch __P((struct device *, struct cfdata *, void *));

struct cfdriver gspcd = {
    NULL, "gsp", gspmatch, gspattach, DV_DULL, sizeof(struct gspsoftc), 0
};

int
gspmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    caddr_t virt;

    virt = (caddr_t) IIO_CFLOC_ADDR(cf);
    return !badbaddr(virt);
}

void
gspattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct gspsoftc *dv;

    iio_print(self->dv_cfdata);

    /* save the address */
    dv = (struct gspsoftc *) self;
    dv->GSP = (volatile struct gsp_regs *) IIO_CFLOC_ADDR(self->dv_cfdata);
    dv->kbd.prefix = -1;
    dv->kbd_dev = kbd_zsunit;
}

/*
 * Major entry points referenced from cdevsw.
 */
gspopen(dev_t dev, int flag, int mode, struct proc *p)
{
    register struct tty *tp;
    int error, unit;
    struct gspsoftc *dv;

    unit = minor(dev);
    if( unit >= gspcd.cd_ndevs
       || (dv = (struct gspsoftc *) gspcd.cd_devs[unit]) == NULL )
	return ENODEV;

    error = zs_kbdopen(dv->kbd_dev, unit, &kbdcn_termios, p);
    if( error )
	return error;
    if( dv->tty == NULL ){
	dv->tty = ttymalloc();
	if( unit < NGSP )
	    gsp_tty[unit] = dv->tty;
    }
    tp = dv->tty;
    tp->t_oproc = gspstart;
    tp->t_param = NULL;
    tp->t_dev = dev;
    tp->t_sc = (caddr_t) dv;
    if ((tp->t_state & TS_ISOPEN) == 0) {
	tp->t_state |= TS_WOPEN;
	ttychars(tp);
	tp->t_iflag = TTYDEF_IFLAG;
	tp->t_oflag = TTYDEF_OFLAG;
	tp->t_cflag = TTYDEF_CFLAG;
	tp->t_lflag = TTYDEF_LFLAG;
	tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
	ttsetwater(tp);
    } else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0) {
	return (EBUSY);
    }
    tp->t_state |= TS_CARR_ON;
    error = ((*linesw[tp->t_line].l_open)(dev, tp));
    if( error ){
	zs_kbdclose(dv->kbd_dev);
	return error;
    }
    if( cdevsw[major(dev)].d_open == gspopen )
        dv->open = 1;
    else
        dv->open_as_console = 1;
    tp->t_winsize.ws_row = gsp_read_word(dv->GSP, MAX_ROW_ADRS);
    tp->t_winsize.ws_col = gsp_read_word(dv->GSP, MAX_COL_ADRS);
    return 0;
}

gspclose(dev, flag, mode, p)
    dev_t dev;
    int flag, mode;
    struct proc *p;
{
    int unit;
    struct gspsoftc *dv;

    unit = minor(dev);
    dv = (struct gspsoftc *) gspcd.cd_devs[unit];
    if( cdevsw[major(dev)].d_open == gspopen )
        dv->open = 0;
    else
        dv->open_as_console = 0;
    if( !dv->open && !dv->open_as_console ){
	(*linesw[dv->tty->t_line].l_close)(dv->tty, flag);
	ttyclose(dv->tty);
    }
    zs_kbdclose(dv->kbd_dev);
    return(0);
}

/*ARGSUSED*/
gspread(dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    struct tty *tp = gsp_tty[minor(dev)];

    return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
gspwrite(dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    struct tty *tp = gsp_tty[minor(dev)];

    return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

gspioctl(dev, cmd, data, flag, p)
    dev_t dev;
    caddr_t data;
    int cmd, flag;
    struct proc *p;
{
    register struct tty *tp = gsp_tty[minor(dev)];
    register error;
 
    error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
    if (error >= 0)
	return (error);
    error = ttioctl(tp, cmd, data, flag, p);
    if (error >= 0)
	return (error);
    return (ENOTTY);
}

void
gspstart(tp)
    register struct tty *tp;
{
    register int s, nc, ncput;
    struct gspsoftc *dv;
    unsigned char buf[256];

    dv = (struct gspsoftc *) tp->t_sc;
    s = spltty();
    if (tp->t_state & (TS_TIMEOUT|TS_TTSTOP|TS_BUSY))
	goto out;
    do {
	if (tp->t_outq.c_cc <= tp->t_lowat) {
	    if (tp->t_state&TS_ASLEEP) {
		tp->t_state &= ~TS_ASLEEP;
		wakeup((caddr_t)&tp->t_outq);
	    }
	    selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc == 0)
	    goto out;
	nc = q_to_b(&tp->t_outq, buf, sizeof(buf));
	tp->t_state |= TS_BUSY;
	splx(s);
	ncput = gsp_write(dv->GSP, buf, (size_t) nc);
	s = spltty();
	tp->t_state &= ~TS_BUSY;
    } while( ncput == nc );

    /* some characters remain; start a timeout to push them out later */
    tp->t_state |= TS_TIMEOUT;
    timeout(ttrstrt, (void *) tp, 1);

out:
    splx(s);
}

/*
 * Stop output to the screen.
 */
gspstop(tp, flag)
    register struct tty *tp;
    int flag;
{
    register int s;
    register struct gspsoftc *dv;
    register gsp_ptr gsp;
    register int mode;
    register u_short oc;
    register u_long oa;

    dv = (struct gspsoftc *) tp->t_sc;
    gsp = dv->GSP;
    s = spltty();
    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl |= LBL;
    gsp->adrs = GSP_MODE_ADRS;
    mode = gsp->data;

    if ((tp->t_state&TS_TTSTOP)==0) {
	/* flush output */
	tp->t_state |= TS_FLUSH;
	mode |= GMODE_FLUSH;
    } else {
	/* suspending output */
	mode |= GMODE_HOLD;
    }

    gsp->adrs = GSP_MODE_ADRS;
    gsp->data = mode;
    gsp->adrs = oa;
    gsp->ctrl = oc;
    splx(s);
}

int
gspmap(dev_t dev, long off, int prot)
{
    struct gspsoftc *dv;

    if( off != 0 )
	return -1;
    dv = (struct gspsoftc *) gspcd.cd_devs[minor(dev)];
    return btoc(dv->GSP);
}
 
int
gsp_write(register gsp_ptr gsp, unsigned char *ptr, size_t nb)
{
    size_t n, na;
    register int c;
    register unsigned short put, oc;
    register unsigned long next, put_hi, oa;

    if( nb == 0 )
	return 0;
    n = nb;
    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl = (gsp->ctrl | LBL | INCW) & ~INCR;

    gsp->adrs = GSP_MODE_ADRS;
    gsp->data &= ~GMODE_FLUSH;

    gsp->adrs = PUT_HI_PTR_ADRS;
    put_hi = gsp->data << 16;

    while( n != 0 ){
	gsp->adrs = GET_PTR_ADRS;
	next = (gsp->data - 8) & 0xFFF0;
	gsp->adrs = PUT_PTR_ADRS;
	put = gsp->data & ~7;
	/* can't put a byte in location `next' */
	if( next == put )
	    break;
	if( (put & 8) != 0 ){
	    /* put a single byte in a word */
	    gsp->adrs = put | put_hi;
	    gsp->data = (*ptr++ << 8) | (gsp->data & 0xFF);
	    --n;
	    gsp->adrs = PUT_PTR_ADRS;
	    gsp->data = put + 8;
	    continue;
	}
	if( next < put )
	    next = 0x10000;
	na = (next - put) >> 3;
	if( na > n )
	    na = n;
	gsp->adrs = put | put_hi;
	n -= na;
	put += na << 3;
	for( ; na > 1; na -= 2 ){
	    c = *ptr++;
	    gsp->data = (*ptr++ << 8) | c;
	}
	if( na != 0 )
	    gsp->data = *ptr++;
	gsp->adrs = PUT_PTR_ADRS;
	gsp->data = put;
    }
    gsp->adrs = oa;
    gsp->ctrl = oc;
    return nb - n;
}

int
gsp_read_word(register gsp_ptr gsp, unsigned long adrs)
{
    register u_short oc, val;
    register u_long oa;

    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl |= LBL;
    gsp->adrs = adrs;
    val = gsp->data;
    gsp->adrs = oa;
    gsp->ctrl = oc;
    return val;
}

int
gsp_get_kmode(register gsp_ptr gsp)
{
    return gsp_read_word(gsp, KBD_MODE_ADRS);
}

int
gsp_toggle_hold(register gsp_ptr gsp)
{
    register int mode;
    register u_short oc;
    register u_long oa;

    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl |= LBL;
    gsp->adrs = GSP_MODE_ADRS;
    mode = gsp->data ^ GMODE_HOLD;
    gsp->adrs = GSP_MODE_ADRS;
    gsp->data = mode;
    gsp->adrs = oa;
    gsp->ctrl = oc;
    return mode & GMODE_HOLD;
}

gsp_set_history(register gsp_ptr gsp, int n)
{
    register u_short oc;
    register u_long oa;

    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl |= LBL;
    gsp->adrs = HIST_REQ_ADRS;
    gsp->data = n;
    gsp->adrs = oa;
    gsp->ctrl = oc;
}

gsp_incr_history(register gsp_ptr gsp, int n)
{
    register u_short oc;
    register u_long oa;

    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl |= LBL;
    gsp->adrs = HIST_REQ_ADRS;
    n += gsp->data;
    if( n < 0 )
	n = 0;
    gsp->adrs = HIST_REQ_ADRS;
    gsp->data = n;
    gsp->adrs = oa;
    gsp->ctrl = oc;
}

gsp_issue_cmd(register gsp_ptr gsp, int cmd)
{
    register u_short oc;
    register u_long oa;

    oc = gsp->ctrl;
    oa = gsp->adrs;
    gsp->ctrl |= LBL;
    gsp->adrs = GSP_CMD_ADRS;
    gsp->data = cmd;
    gsp->adrs = oa;
    gsp->ctrl = oc;
}

/*
 * Routines for using a keyboard attached to an SCC as the
 * input side of /dev/gsp.
 */

int
kbd_newchar(int unit, register int c)
{
    int n, kmode;
    struct gspsoftc *dv = (struct gspsoftc *) gspcd.cd_devs[unit];
    struct tty *tp = dv->tty;
    u_char kbstr[5];

    if( dv->kbd.prefix < 0 ){
	if( c == 0x55 )
	    dv->kbd.prefix = 0;
	else if( c == 0xAA )
	    dv->kbd.prefix = 0x100;
	return;
    }
    c += dv->kbd.prefix;
    dv->kbd.prefix = -1;
    kmode = gsp_get_kmode(dv->GSP);

    if( c == 0x8F ){
	if( (kmode & KBD_AUTO_RPT) == 0 )
	    return;
	c = dv->kbd.last;
    } else
	dv->kbd.last = c;

    /* got a single-character code for the key; take action or
       turn it into an escape sequence */
    if( c <= 0x80 ){
	if( (kmode & KBD_CLICK) != 0 )
	    zs_kbdput(dv->kbd_dev, KC_CLICK);
	(*linesw[tp->t_line].l_rint)(c, tp);
	return;
    }
    if( c < 0x100 ){
	/* compose, f1-f5 */
	if( c == KEY_SCRL ){
	    /* Hold screen */
	    n = gsp_toggle_hold(dv->GSP);
	    zs_kbdput(dv->kbd_dev, n? 0x38: 0x28);
	    if( (kmode & KBD_CLICK) != 0 )
		zs_kbdput(dv->kbd_dev, KC_CLICK);
	} else if( c == KEY_BREAK ){
	    gsp_issue_cmd(dv->GSP, GSP_CMD_REFRESH);
	} else if( c == KEY_PRINT ){
	    /* print */
	    /* print_something(); */
#if 0
	} else if( c == KEY_C_PRINT ){
	    vm_check();
#endif
	} else if( c == KEY_C_BREAK ){
	    /* ctrl-break */
	    asm(" trap #14");
	}
	return;
    }

    /* control of scrolling back through history */
    switch( c ){
    case 0x145:		/* shift up-arrow */
	gsp_incr_history(dv->GSP, 1);
	return;
    case 0x146:		/* shift down-arrow */
	gsp_incr_history(dv->GSP, -1);
	return;
    case 0x1B5:		/* shift prev-screen */
	gsp_incr_history(dv->GSP, 24);
	return;
    case 0x1B6:		/* shift next-screen */
	gsp_incr_history(dv->GSP, -24);
	return;
    case 0x1B4:		/* shift select */
	gsp_set_history(dv->GSP, 0);
	return;
    }

    for( n = 0; n < 5; ++n )
	kbstr[n] = 0;
    if( c < 0x130 ){
	/* top row, f6 - f20, normal, shifted and ctrled */
	n = c & 0xF;
	if( n >= 12 )	++n;
	if( n >= 10 )	++n;
	if( n >= 6 )	++n;
	if( c >= 0x110 )
	    n += 20;
	if( c >= 0x120 )
	    n += 20;
	n += 16;
	kbstr[0] = CSI;
	kbstr[1] = n / 10 + '0';
	kbstr[2] = n % 10 + '0';
	kbstr[3] = '~';
    } else if( c <= 0x15F || (kmode & KBD_APPL_KPAD) != 0 && c <= 0x17E ){
	/* editing and numeric keypads */
	if( c <= 0x139 || (kmode & KBD_APPL_CURS) == 0 && c <= 0x144 )
	    kbstr[0] = CSI;
	else
	    kbstr[0] = SS3;
	kbstr[1] = c;
	if( c <= 0x139 )
	    kbstr[2] = '~';
    } else if( c <= 0x17E ){
	/* numeric keypad, not in application mode */
	kbstr[0] = c - 0x140;
    } else if( c <= 0x18A ){
	/* delete, backspace, line feed */
	kbstr[0] = c & 0x7F;
    } else
	/* shifted keypad keys */
	return;

    if( (kmode & KBD_CLICK) != 0 )
	zs_kbdput(dv->kbd_dev, KC_CLICK);
    for( n = 0; (c = kbstr[n]) != 0; ++n ){
	if( c >= 0x80 && (kmode & KBD_7_BIT) != 0 ){
	    (*linesw[tp->t_line].l_rint)(ESC, tp);
	    c -= 0x40;
	}
	(*linesw[tp->t_line].l_rint)(c, tp);
    }
}

/*
 * Routines to interface to the virtual console code.
 */
int	gsp_cons_addr = 0xE70000;
int	gsp_cons_kbd = 3;
struct kbd gsp_cons_kstate = {-1, 0};
gsp_ptr	gsp_cons_ptr;

gspcnprobe(cp)
    struct consdev *cp;
{
    int maj;

    /* check the keyboard */
    if( !zs_kbdcnprobe(cp, gsp_cons_kbd) )
	return;

    /* locate the major number */
    for (maj = 0; maj < nchrdev; maj++)
	if (cdevsw[maj].d_open == gspopen)
	    break;

    /* initialize required fields */
    cp->cn_dev = makedev(maj, 0);
    cp->cn_pri = CN_INTERNAL;
}

/* ARGSUSED */
gspcninit(cp)
    struct consdev *cp;
{
    gsp_cons_ptr = (gsp_ptr) IIOV(gsp_cons_addr);
    zs_cnsetup(gsp_cons_kbd, &kbdcn_termios);	/* initialize the keyboard */
    gsp_cons_kstate.prefix = -1;
}

/* ARGSUSED */
gspcnputc(dev, c)
    dev_t dev;
    u_char c;
{
    int s;

    s = spltty();
    while( gsp_write(gsp_cons_ptr, &c, 1) == 0 )
	DELAY(100)
    splx(s);
    return 0;
}

/* ARGSUSED */
gspcngetc(dev)
    dev_t dev;
{
    register int c;
    int kmode;

    for(;;){
	c = zscngetc(gsp_cons_kbd);
	if( gsp_cons_kstate.prefix < 0 ){
	    if( c == 0x55 )
		gsp_cons_kstate.prefix = 0;
	    else if( c == 0xAA )
		gsp_cons_kstate.prefix = 0x100;
	    continue;
	}
	c += gsp_cons_kstate.prefix;
	gsp_cons_kstate.prefix = -1;
	kmode = gsp_get_kmode(gsp_cons_ptr);

	if( c == 0x8F ){
	    if( (kmode & KBD_AUTO_RPT) == 0 )
		continue;
	    c = gsp_cons_kstate.last;
	} else
	    gsp_cons_kstate.last = c;

	if( c <= 0x80 ){
	    /* an ordinary char - that's what we want */
	    if( (kmode & KBD_CLICK) != 0 )
		zscnputc(gsp_cons_kbd, KC_CLICK);
	    break;
	}

	/* we take action on some other keys, but we don't return
	   escape sequences. */
	switch( c ){
	case KEY_SCRL:		/* Hold screen */
	    c = gsp_toggle_hold(gsp_cons_ptr);
	    zscnputc(gsp_cons_kbd, c? 0x38: 0x28);
	    break;
	case 0x145:		/* shift up-arrow */
	    gsp_incr_history(gsp_cons_ptr, 1);
	    break;
	case 0x146:		/* shift down-arrow */
	    gsp_incr_history(gsp_cons_ptr, -1);
	    break;
	case 0x1B5:		/* shift prev-screen */
	    gsp_incr_history(gsp_cons_ptr, 24);
	    break;
	case 0x1B6:		/* shift next-screen */
	    gsp_incr_history(gsp_cons_ptr, -24);
	    break;
	case 0x1B4:		/* shift select */
	    gsp_set_history(gsp_cons_ptr, 0);
	    break;
	}
	/* ignore function keys, etc. */
    }
    if (c == '\r')
	c = '\n';
    return c;
}

#endif	/* NGSP > 0 */
