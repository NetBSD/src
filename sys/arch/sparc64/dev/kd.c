/*	$NetBSD: kd.c,v 1.53 2014/07/25 08:10:35 dholland Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Keyboard/Display device.
 *
 * This driver exists simply to provide a tty device that
 * the indirect console driver can point to.
 * The kbd driver sends its input here.
 * Output goes to the screen via PROM printf.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kd.c,v 1.53 2014/07/25 08:10:35 dholland Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include <machine/openfirm.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/kbd.h>
#include <machine/autoconf.h>

#include <dev/cons.h>
#include <dev/sun/event_var.h>
#include <dev/sun/kbd_xlate.h>
#include <dev/sun/kbdvar.h>
#include <sparc64/dev/cons.h>

dev_type_open(kdopen);
dev_type_close(kdclose);
dev_type_read(kdread);
dev_type_write(kdwrite);
dev_type_ioctl(kdioctl);
dev_type_tty(kdtty);
dev_type_poll(kdpoll);

const struct cdevsw kd_cdevsw = {
	.d_open = kdopen,
	.d_close = kdclose,
	.d_read = kdread,
	.d_write = kdwrite,
	.d_ioctl = kdioctl,
	.d_stop = nostop,
	.d_tty = kdtty,
	.d_poll = kdpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

struct	tty *fbconstty = 0;	/* tty structure for frame buffer console */

#define PUT_WSIZE	64

struct kd_softc {
	struct  tty *kd_tty;
	int rows, cols;

	/* Console input hook */
	struct cons_channel *kd_in;
};

/*
 * There is no point in pretending there might be
 * more than one keyboard/display device.
 */
static struct kd_softc kd_softc;
static int kd_is_console;

static int kdparam(struct tty *, struct termios *);
static void kdstart(struct tty *);
static void kd_init(struct kd_softc *);
static void kd_cons_input(int);
static int  kdcngetc(dev_t);
static void kd_later(void*);
static void kd_putfb(struct tty *);

int	cons_ocount;		/* output byte count */

/*
 * This is called by kbd_attach() 
 * XXX - Make this a proper child of kbd?
 */
void
kd_init(struct kd_softc *kd)
{
	struct tty *tp;
	char prop[6+1];
	
	kd = &kd_softc; 	/* XXX */

	tp = tty_alloc();
	callout_setfunc(&tp->t_rstrt_ch, kd_later, tp);
	tp->t_oproc = kdstart;
	tp->t_param = kdparam;
	tp->t_dev = makedev(cdevsw_lookup_major(&kd_cdevsw), 0);

	tty_attach(tp);
	kd->kd_tty = tp;

	/*
	 * get the console struct winsize.
	 */
	if (kd_is_console) {
		fbconstty = tp;
	}

	if (kd->rows == 0 &&
	    prom_getoption("screen-#rows", prop, sizeof prop) == 0)
		kd->rows = strtoul(prop, NULL, 10);

	if (kd->cols == 0 &&
	    prom_getoption("screen-#columns", prop, sizeof prop) == 0)
		kd->cols = strtoul(prop, NULL, 10);
}

struct tty *
kdtty(dev_t dev)
{
	struct kd_softc *kd;

	kd = &kd_softc; 	/* XXX */
	return (kd->kd_tty);
}

int
kdopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct kd_softc *kd;
	int error, s, unit;
	struct tty *tp;
static	int firstopen = 1;
	
	unit = minor(dev);
	if (unit != 0)
		return ENXIO;
	kd = &kd_softc; 	/* XXX */

	if (firstopen) {
		kd_init(kd);
		firstopen = 0;
	}
	tp = kd->kd_tty;

	/* It's simpler to do this up here. */
	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	s = spltty();

	if ((tp->t_state & TS_ISOPEN) == 0) {
		/* First open. */

		/* Notify the input device that serves us */
		struct cons_channel *cc = kd->kd_in;
		if (cc != NULL &&
		    (error = (*cc->cc_iopen)(cc)) != 0) {
			splx(s);
			return (error);
		}

		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		(void) kdparam(tp, &tp->t_termios);
		ttsetwater(tp);
		tp->t_winsize.ws_row = kd->rows;
		tp->t_winsize.ws_col = kd->cols;
		/* Flush pending input?  Clear translator? */
		/* This (pseudo)device always has SOFTCAR */
		tp->t_state |= TS_CARR_ON;
	}

	splx(s);

	return ((*tp->t_linesw->l_open)(dev, tp));
}

int
kdclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct kd_softc *kd;
	struct tty *tp;
	struct cons_channel *cc;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	/* XXX This is for cons.c. */
	if ((tp->t_state & TS_ISOPEN) == 0)
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if ((cc = kd->kd_in) != NULL)
		(void)(*cc->cc_iclose)(cc);

	return (0);
}

int
kdread(dev_t dev, struct uio *uio, int flag)
{
	struct kd_softc *kd;
	struct tty *tp;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
kdwrite(dev_t dev, struct uio *uio, int flag)
{
	struct kd_softc *kd;
	struct tty *tp;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
kdpoll(dev_t dev, int events, struct lwp *l)
{
	struct kd_softc *kd;
	struct tty *tp;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
kdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct kd_softc *kd;
	struct tty *tp;
	int error;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;

	/* Handle any ioctl commands specific to kbd/display. */
	/* XXX - Send KB* ioctls to kbd module? */
	/* XXX - Send FB* ioctls to fb module?  */

	return EPASSTHROUGH;
}

static int
kdparam(struct tty *tp, struct termios *t)
{
	/* XXX - These are ignored... */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

static void
kdstart(struct tty *tp)
{
	struct clist *cl;
	int s1, s2;

	s1 = splsoftclock();
	s2 = spltty();
	if (tp->t_state & (TS_BUSY|TS_TTSTOP|TS_TIMEOUT))
		goto out;

	cl = &tp->t_outq;
	if (cl->c_cc) {
		if (kd_is_console) {
			tp->t_state |= TS_BUSY;
			if (s1 == 0) {
				/* called at level zero - update screen now. */
				splx(s2);
				kd_putfb(tp);
				s2 = spltty();
				tp->t_state &= ~TS_BUSY;
			} else {
				/* called at interrupt level - do it later */
				callout_schedule(&tp->t_rstrt_ch, 0);
			}
		} else {
			/*
			 * This driver uses the PROM for writing the screen,
			 * and that only works if this is the console device.
			 * If this is not the console, just flush the output.
			 * Sorry.  (In that case, use xdm instead of getty.)
			 */
			ndflush(cl, cl->c_cc);
		}
	}
	ttypull(tp);
out:
	splx(s2);
	splx(s1);
}

/*
 * Timeout function to do delayed writes to the screen.
 * Called at splsoftclock when requested by kdstart.
 */
static void
kd_later(void *tpaddr)
{
	struct tty *tp = tpaddr;
	register int s;

	kd_putfb(tp);

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	(*tp->t_linesw->l_start)(tp);
	splx(s);
}

/*
 * Put text on the screen using the PROM monitor.
 * This can take a while, so to avoid missing
 * interrupts, this is called at splsoftclock.
 */
static void
kd_putfb(struct tty *tp)
{
	char buf[PUT_WSIZE];
	struct clist *cl = &tp->t_outq;
	char *p, *end;
	int len;

	while ((len = q_to_b(cl, buf, PUT_WSIZE-1)) > 0) {
		/* PROM will barf if high bits are set. */
		p = buf;
		end = buf + len;
		while (p < end)
			*p++ &= 0x7f;
		/* Now let the PROM print it. */
		prom_write(prom_stdout(), buf, len);
	}
}

/*
 * Default PROM-based console input stream
 */
static int kd_rom_iopen(struct cons_channel *);
static int kd_rom_iclose(struct cons_channel *);

static struct cons_channel prom_cons_channel;

int
kd_rom_iopen(struct cons_channel *cc)
{
	return (0);
}

int
kd_rom_iclose(struct cons_channel *cc)
{
	return (0);
}

/*
 * Our "interrupt" routine for input. This is called by
 * the keyboard driver (dev/sun/kbd.c) at spltty.
 */
void
kd_cons_input(int c)
{
	struct kd_softc *kd = &kd_softc;
	struct tty *tp;

	/* XXX: Make sure the device is open. */
	tp = kd->kd_tty;
	if (tp == NULL)
		return;
	if ((tp->t_state & TS_ISOPEN) == 0)
		return;

	(*tp->t_linesw->l_rint)(c, tp);
}


/****************************************************************
 * kd console support
 ****************************************************************/

/* The debugger gets its own key translation state. */
static struct kbd_state *kdcn_state;

static void kdcnprobe(struct consdev *);
static void kdcninit(struct consdev *);
static void kdcnputc(dev_t, int);
static void kdcnpollc(dev_t, int);

/* The keyboard driver uses cn_hw to access the real console driver */
extern struct consdev consdev_prom;
struct consdev consdev_kd = {
	.cn_probe = kdcnprobe,
	.cn_init = kdcninit,
	.cn_getc = kdcngetc,
	.cn_putc = kdcnputc,
	.cn_pollc = kdcnpollc,
};
struct consdev *cn_hw = &consdev_kd;

void
cons_attach_input(struct cons_channel *cc, struct consdev *cn)
{
	struct kd_softc *kd = &kd_softc;
	struct kbd_softc *kds = cc->cc_private;
	struct kbd_state *ks;

	/* Share the keyboard state */
	kdcn_state = ks = &kds->k_state;

	kd->kd_in = cc;
	cc->cc_upstream = kd_cons_input;

	/* Attach lower level. */
	cn_hw->cn_dev = cn->cn_dev;
	cn_hw->cn_pollc = cn->cn_pollc;
	cn_hw->cn_getc = cn->cn_getc;

	/* Attach us as console. */
	cn_tab->cn_dev = makedev(cdevsw_lookup_major(&kd_cdevsw), 0);
	cn_tab->cn_probe = kdcnprobe;
	cn_tab->cn_init = kdcninit;
	cn_tab->cn_getc = kdcngetc;
	cn_tab->cn_pollc = kdcnpollc;
	cn_tab->cn_pri = CN_INTERNAL;

	/* Set up initial PROM input channel for /dev/console */
	prom_cons_channel.cc_private = NULL;
	prom_cons_channel.cc_iopen = kd_rom_iopen;
	prom_cons_channel.cc_iclose = kd_rom_iclose;

	/* Indicate that it is OK to use the PROM fbwrite */
	kd_is_console = 1;
}


void kd_attach_input(struct cons_channel *);
void
kd_attach_input(struct cons_channel *cc)
{
	struct kd_softc *kd = &kd_softc;

	kd->kd_in = cc;
	cc->cc_upstream = kd_cons_input;
}


/* We never call this. */
static void
kdcnprobe(struct consdev *cn)
{
}

static void
kdcninit(struct consdev *cn)
{
#if 0
	struct kbd_state *ks = kdcn_state;

	cn->cn_dev = makedev(cdevsw_lookup_major(&kd_cdevsw), 0);
	cn->cn_pri = CN_INTERNAL;

	/* This prepares kbd_translate() */
	ks->kbd_id = KBD_MIN_TYPE;
	kbd_xlate_init(ks);

	/* Set up initial PROM input channel for /dev/console */
	prom_cons_channel.cc_private = NULL;
	prom_cons_channel.cc_iopen = kd_rom_iopen;
	prom_cons_channel.cc_iclose = kd_rom_iclose;
	cons_attach_input(&prom_cons_channel);

	/* Indicate that it is OK to use the PROM fbwrite */
	kd_is_console = 1;
#endif
}

static int
kdcngetc(dev_t dev)
{
	struct kbd_state *ks = kdcn_state;
	int code, class, data, keysym;
	extern int prom_cngetc(dev_t);


	if (cn_hw->cn_getc == prom_cngetc) return (*cn_hw->cn_getc)(dev);
	for (;;) {
		code = (*cn_hw->cn_getc)(dev);
		keysym = kbd_code_to_keysym(ks, code);
		class = KEYSYM_CLASS(keysym);
		
		switch (class) {
		case KEYSYM_ASCII:
			goto out;
			
		case KEYSYM_CLRMOD:
		case KEYSYM_SETMOD:
			data = (keysym & 0x1F);
			/* Only allow ctrl or shift. */
			if (data > KBMOD_SHIFT_R)
				break;
			data = 1 << data;
			if (class == KEYSYM_SETMOD)
				ks->kbd_modbits |= data;
			else
				ks->kbd_modbits &= ~data;
			break;
			
		case KEYSYM_ALL_UP:
			/* No toggle keys here. */
			ks->kbd_modbits = 0;
			break;
			
		default:	/* ignore all other keysyms */
			break;
		}
	}
out:
	return (keysym);
}

static void
kdcnputc(dev_t dev, int c)
{
	int s;
	char c0 = (c & 0x7f);

	s = splhigh();
	prom_write(prom_stdout(), &c0, 1);
	splx(s);
}

static void
kdcnpollc(dev_t dev, int on)
{
	struct kbd_state *ks = kdcn_state;

	if (on) {
		/* Entering debugger. */
#if NFB > 0
		fb_unblank();
#endif
		/* Clear shift keys too. */
		ks->kbd_modbits = 0;
	} else {
		/* Resuming kernel. */
	}
	(*cn_hw->cn_pollc)(dev, on);
}
