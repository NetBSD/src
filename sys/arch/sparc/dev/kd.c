/*	$NetBSD: kd.c,v 1.19 2001/06/11 13:34:51 pk Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Console driver based on PROM primitives.
 *
 * This driver exists to provide a tty device that the indirect
 * console driver can point to. It also provides a hook that
 * allows another device to serve console input. This will normally
 * be a keyboard driver (see sys/dev/sun/kbd.c)
 */

#include "opt_kgdb.h"
#include "fb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/bsd_openprom.h>
#include <machine/promlib.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/kbd.h>
#include <machine/autoconf.h>
#include <machine/conf.h>

#if defined(RASTERCONSOLE) && NFB > 0
#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#endif

#include <dev/cons.h>
#include <sparc/dev/cons.h>

#include <dev/sun/event_var.h>
#include <dev/sun/kbd_xlate.h>
#include <dev/sun/kbdvar.h>

#define	KDMAJOR 1
#define PUT_WSIZE	64

struct kd_softc {
	struct	device kd_dev;		/* required first: base device */
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

static int kdparam(struct tty *, struct termios *);
static void kdstart(struct tty *);
static void kd_init __P((struct kd_softc *));
static void kd_cons_input __P((int));

/*
 * Prepare the console tty; called on first open of /dev/console
 */
void
kd_init(kd)
	struct kd_softc *kd;
{
	struct tty *tp;

	tp = ttymalloc();
	tp->t_oproc = kdstart;
	tp->t_param = kdparam;
	tp->t_dev = makedev(KDMAJOR, 0);

	tty_attach(tp);
	kd->kd_tty = tp;

	/*
	 * Get the console struct winsize.
	 */
#if defined(RASTERCONSOLE) && NFB > 0
	/* If the raster console driver is attached, copy its size */
	kd->rows = fbrcons_rows();
	kd->cols = fbrcons_cols();
	rcons_ttyinit(tp);
#endif

	/* else, consult the PROM */
	switch (prom_version()) {
	char *prop;
	struct eeprom *ep;
	case PROM_OLDMON:
		if ((ep = (struct eeprom *)eeprom_va) == NULL)
			break;
		if (kd->rows == 0)
			kd->rows = (u_short)ep->eeTtyRows;
		if (kd->cols == 0)
			kd->cols = (u_short)ep->eeTtyCols;
		break;
	case PROM_OBP_V0:
	case PROM_OBP_V2:
	case PROM_OBP_V3:
	case PROM_OPENFIRM:

		if (kd->rows == 0 &&
		    (prop = getpropstring(optionsnode, "screen-#rows"))) {
			int i = 0;

			while (*prop != '\0')
				i = i * 10 + *prop++ - '0';
			kd->rows = (unsigned short)i;
		}
		if (kd->cols == 0 &&
		    (prop = getpropstring(optionsnode, "screen-#columns"))) {
			int i = 0;

			while (*prop != '\0')
				i = i * 10 + *prop++ - '0';
			kd->cols = (unsigned short)i;
		}
		break;
	}

	return;
}

struct tty *
kdtty(dev)
	dev_t dev;
{
	struct kd_softc *kd;

	kd = &kd_softc; 	/* XXX */
	return (kd->kd_tty);
}

int
kdopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
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
	if (((tp->t_state & (TS_ISOPEN | TS_XCLUDE))
	     ==             (TS_ISOPEN | TS_XCLUDE))
	    && (p->p_ucred->cr_uid != 0) )
	{
		return (EBUSY);
	}

	s = spltty();
	if ((tp->t_state & TS_ISOPEN) == 0) {
		/* First open. */

		/* Notify the input device that serves us */
		struct cons_channel *cc = kd->kd_in;
		if (cc != NULL &&
		    (error = (*cc->cc_iopen)(cc)) != 0) {
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
kdclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
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
kdread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct kd_softc *kd;
	struct tty *tp;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
kdwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct kd_softc *kd;
	struct tty *tp;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
kdpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct kd_softc *kd;
	struct tty *tp;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

int
kdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct kd_softc *kd;
	struct tty *tp;
	int error;

	kd = &kd_softc; 	/* XXX */
	tp = kd->kd_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	/* Handle any ioctl commands specific to kbd/display. */
	/* XXX - Send KB* ioctls to kbd module? */
	/* XXX - Send FB* ioctls to fb module?  */

	return ENOTTY;
}

void
kdstop(tp, flag)
	struct tty *tp;
	int flag;
{

}


static int
kdparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	/* XXX - These are ignored... */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}


static void kd_later(void*);
static void kd_putfb(struct tty *);

static void
kdstart(tp)
	struct tty *tp;
{
	struct clist *cl;
	int s;

	s = spltty();
	if (tp->t_state & (TS_BUSY|TS_TTSTOP|TS_TIMEOUT))
		goto out;

	cl = &tp->t_outq;
	if (cl->c_cc) {
		tp->t_state |= TS_BUSY;
		if ((s & PSR_PIL) == 0) {
			/* called at level zero - update screen now. */
			(void) spllowersoftclock();
			kd_putfb(tp);
			(void) spltty();
			tp->t_state &= ~TS_BUSY;
		} else {
			/* called at interrupt level - do it later */
			callout_reset(&tp->t_rstrt_ch, 0, kd_later, tp);
		}
	}
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)cl);
		}
		selwakeup(&tp->t_wsel);
	}
out:
	splx(s);
}

/*
 * Timeout function to do delayed writes to the screen.
 * Called at splsoftclock when requested by kdstart.
 */
static void
kd_later(arg)
	void *arg;
{
	struct tty *tp = arg;
	int s;

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
kd_putfb(tp)
	struct tty *tp;
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
		prom_putstr(buf, len);
	}
}

/*
 * Our "interrupt" routine for input. This is called by
 * the keyboard driver (dev/sun/kbd.c) at spltty.
 */
void
kd_cons_input(c)
	int c;
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

void
cons_attach_input(cc, cn)
	struct cons_channel *cc;
	struct consdev *cn;
{
	struct kd_softc *kd = &kd_softc;

	kd->kd_in = cc;
	cc->cc_upstream = kd_cons_input;
}

void kd_attach_input(struct cons_channel *);
void
kd_attach_input(cc)
	struct cons_channel *cc;
{
	struct kd_softc *kd = &kd_softc;

	kd->kd_in = cc;
	cc->cc_upstream = kd_cons_input;
}

/*
 * Default PROM-based console input stream
 * Since the PROM does not notify us when data is available on the
 * input channel these functions periodically poll the PROM.
 */
static int kd_rom_iopen __P((struct cons_channel *));
static int kd_rom_iclose __P((struct cons_channel *));
static void kd_rom_intr __P((void *));

static struct cons_channel prom_cons_channel;

int
kd_rom_iopen(cc)
	struct cons_channel *cc;
{
	/* Poll for ROM input 4 times per second */
	callout_reset(&cc->cc_callout, hz / 4, kd_rom_intr, cc);
	return (0);
}

int
kd_rom_iclose(cc)
	struct cons_channel *cc;
{

	callout_stop(&cc->cc_callout);
	return (0);
}

/*
 * "Interrupt" routine for input through ROM vectors
 */
void
kd_rom_intr(arg)
	void *arg;
{
	struct cons_channel *cc = arg;
	int s, c;

	/* Re-schedule */
	callout_reset(&cc->cc_callout, hz / 4, kd_rom_intr, cc);

	s = spltty();

	while ((c = prom_peekchar()) >= 0)
		(*cc->cc_upstream)(c);

	splx(s);
}

/*****************************************************************/

int prom_stdin_node;
int prom_stdout_node;
char prom_stdin_args[16];
char prom_stdout_args[16];

extern void prom_cnprobe __P((struct consdev *));
static void prom_cninit __P((struct consdev *));
static int  prom_cngetc __P((dev_t));
static void prom_cnputc __P((dev_t, int));
extern void prom_cnpollc __P((dev_t, int));

/*
 * The console is set to this one initially,
 * which lets us use the PROM until consinit()
 * is called to select a real console.
 */
struct consdev consdev_prom = {
	prom_cnprobe,
	prom_cninit,
	prom_cngetc,
	prom_cnputc,
	prom_cnpollc,
	NULL,
};

/*
 * The console table pointer is statically initialized
 * to point to the PROM table, so that early calls to printf will work.
 */
struct consdev *cn_tab = &consdev_prom;

void
prom_cnprobe(cn)
	struct consdev *cn;
{
}

static void
prom_cninit(cn)
	struct consdev *cn;
{
}

void
prom_cnpollc(dev, on)
	dev_t dev;
	int on;
{

	if (on) {
		/* Entering debugger. */
#if NFB > 0
		fb_unblank();
#endif
	} else {
		/* Resuming kernel. */
	}
}


/*
 * PROM console input putchar.
 */
static int
prom_cngetc(dev)
	dev_t dev;
{
	int s, c;

	s = splhigh();
	c = prom_getchar();
	splx(s);
	return (c);
}

/*
 * PROM console output putchar.
 */
static void
prom_cnputc(dev, c)
	dev_t dev;
	int c;
{

	prom_putchar(c);
}


/*****************************************************************/

static void prom_get_device_args __P((const char *, char *, unsigned int));

void
prom_get_device_args(prop, args, sz)
	const char *prop;
	char *args;
	unsigned int sz;
{
	char *cp, buffer[128];

	cp = getpropstringA(findroot(), (char *)prop, buffer, sizeof buffer);

	/*
	 * Extract device-specific arguments from a PROM device path (if any)
	 */
	cp = buffer + strlen(buffer);
	while (cp >= buffer) {
		if (*cp == ':') {
			strncpy(args, cp+1, sz);
			break;
		}
		cp--;
	}
}

/*
 *
 */
void
consinit()
{
	int inSource, outSink;

	switch (prom_version()) {
	case PROM_OLDMON:
	case PROM_OBP_V0:
		/* The stdio handles identify the device type */
		inSource = prom_stdin();
		outSink  = prom_stdout();
		break;

	case PROM_OBP_V2:
	case PROM_OBP_V3:
	case PROM_OPENFIRM:

		/* Save PROM arguments for device matching */
		prom_get_device_args("stdin-path", prom_stdin_args,
				     sizeof(prom_stdin_args));
		prom_get_device_args("stdout-path", prom_stdout_args,
				    sizeof(prom_stdout_args));

		/*
		 * Translate the STDIO package instance (`ihandle') -- that
		 * the PROM has already opened for us -- to a device tree
		 * node (i.e. a `phandle'). 
		 */

		prom_stdin_node = prom_instance_to_package(prom_stdin());
		if (prom_stdin_node == 0)
			printf("consinit: cannot convert stdin ihandle\n");

		prom_stdout_node = prom_instance_to_package(prom_stdout());
		if (prom_stdout_node == 0) {
			printf("consinit: cannot convert stdout ihandle\n");
			break;
		}

		break;

	default:
		break;
	}

	/* Wire up /dev/console */
	cn_tab->cn_dev = makedev(KDMAJOR, 0);
	cn_tab->cn_pri = CN_INTERNAL;

	/* Set up initial PROM input channel for /dev/console */
	prom_cons_channel.cc_dev = NULL;
	prom_cons_channel.cc_iopen = kd_rom_iopen;
	prom_cons_channel.cc_iclose = kd_rom_iclose;
	cons_attach_input(&prom_cons_channel, cn_tab);

#ifdef	KGDB
	zs_kgdb_init();	/* XXX */
#endif
}
