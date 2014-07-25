/*	$NetBSD: ite.c,v 1.76 2014/07/25 08:10:32 dholland Exp $	*/

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
 *      from: Utah Hdr: ite.c 1.1 90/07/09
 *      from: @(#)ite.c 7.6 (Berkeley) 5/16/91
 */

/*
 * ite - bitmapped terminal.
 * Supports VT200, a few terminal features will be unavailable until
 * the system actually probes the device (i.e. not after consinit())
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ite.c,v 1.76 2014/07/25 08:10:32 dholland Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <dev/cons.h>
#include <sys/kauth.h>

#include <machine/cpu.h>

#include <atari/atari/device.h>
#include <atari/dev/event_var.h>
#include <atari/dev/kbdmap.h>
#include <atari/dev/kbdvar.h>
#include <atari/dev/iteioctl.h>
#include <atari/dev/itevar.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grfvar.h>
#include <atari/dev/viewioctl.h>
#include <atari/dev/viewvar.h>

#include "ioconf.h"

#define ITEUNIT(dev)	(minor(dev))

#define SUBR_INIT(sc)			(sc)->grf->g_iteinit(sc)
#define SUBR_DEINIT(sc)			(sc)->grf->g_itedeinit(sc)
#define SUBR_PUTC(sc,c,dy,dx,m)		(sc)->grf->g_iteputc(sc,c,dy,dx,m)
#define SUBR_CURSOR(sc,flg)		(sc)->grf->g_itecursor(sc,flg)
#define SUBR_CLEAR(sc,sy,sx,h,w)	(sc)->grf->g_iteclear(sc,sy,sx,h,w)
#define SUBR_SCROLL(sc,sy,sx,cnt,dir)	(sc)->grf->g_itescroll(sc,sy,sx,cnt,dir)

int	start_repeat_timeo = 30;	/* first repeat after x s/100 */
int	next_repeat_timeo  = 10;	/* next repeat after x s/100 */

/*
 * Patchable
 */
int ite_default_x      = 0;	/* def leftedge offset			*/
int ite_default_y      = 0;	/* def topedge offset			*/
int ite_default_width  = 640;	/* def width				*/
int ite_default_depth  = 1;	/* def depth				*/
int ite_default_height = 400;	/* def height				*/
int ite_default_wrap   = 1;	/* if you want vtxxx-nam -> binpatch	*/

struct	ite_softc con_itesoftc;
u_char	cons_tabs[MAX_TABS];

struct ite_softc *kbd_ite;
int kbd_init;

static inline int  atoi(const char *);
static inline int  ite_argnum(struct ite_softc *);
static inline int  ite_zargnum(struct ite_softc *);
static inline void ite_cr(struct ite_softc *);
static inline void ite_crlf(struct ite_softc *);
static inline void ite_clrline(struct ite_softc *);
static inline void ite_clrscreen(struct ite_softc *);
static inline void ite_clrtobos(struct ite_softc *);
static inline void ite_clrtobol(struct ite_softc *);
static inline void ite_clrtoeol(struct ite_softc *);
static inline void ite_clrtoeos(struct ite_softc *);
static inline void ite_dnchar(struct ite_softc *, int);
static inline void ite_inchar(struct ite_softc *, int);
static inline void ite_inline(struct ite_softc *, int);
static inline void ite_lf(struct ite_softc *);
static inline void ite_dnline(struct ite_softc *, int);
static inline void ite_rlf(struct ite_softc *);
static inline void ite_sendstr(const char *);
static inline void snap_cury(struct ite_softc *);

static void	alignment_display(struct ite_softc *);
static struct ite_softc *getitesp(dev_t);
static void	itecheckwrap(struct ite_softc *);
static void	iteprecheckwrap(struct ite_softc *);
static void	itestart(struct tty *);
static void	ite_switch(int);
static void	repeat_handler(void *);

void iteputchar(int c, struct ite_softc *sc);
void ite_putstr(const u_char * s, int len, dev_t dev);
void iteattach(device_t, device_t, void *);
int  itematch(device_t, cfdata_t, void *);

/*
 * Console specific types.
 */
dev_type_cnprobe(itecnprobe);
dev_type_cninit(itecninit);
dev_type_cngetc(itecngetc);
dev_type_cnputc(itecnputc);

CFATTACH_DECL_NEW(ite, sizeof(struct ite_softc),
    itematch, iteattach, NULL, NULL);

dev_type_open(iteopen);
dev_type_close(iteclose);
dev_type_read(iteread);
dev_type_write(itewrite);
dev_type_ioctl(iteioctl);
dev_type_tty(itetty);
dev_type_poll(itepoll);

const struct cdevsw ite_cdevsw = {
	.d_open = iteopen,
	.d_close = iteclose,
	.d_read = iteread,
	.d_write = itewrite,
	.d_ioctl = iteioctl,
	.d_stop = nostop,
	.d_tty = itetty,
	.d_poll = itepoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

/*
 * Keep track of the device number of the ite console. Only used in the
 * itematch/iteattach functions.
 */
static int		cons_ite = -1;

int
itematch(device_t parent, cfdata_t cf, void *aux)
{
	
	/*
	 * Handle early console stuff. The first unit number
	 * is the console unit. All other early matches will fail.
	 */
	if (atari_realconfig == 0) {
		if (cons_ite >= 0)
			return 0;
		cons_ite = cf->cf_unit;
		return 1;
	}
	return 1;
}

void
iteattach(device_t parent, device_t self, void *aux)
{
	struct grf_softc	*gsc;
	struct ite_softc	*sc;
	int			s;
	int			maj, unit;

	gsc = device_private(parent);
	sc = device_private(self);

	maj = cdevsw_lookup_major(&ite_cdevsw);
	unit = (self != NULL) ? device_unit(self) : cons_ite;
	gsc->g_itedev = makedev(maj, unit);

	if (self != NULL) {
		s = spltty();
		if (con_itesoftc.grf != NULL &&
		    con_itesoftc.grf->g_unit == gsc->g_unit) {
			/*
			 * console reinit copy params over.
			 * and console always gets keyboard
			 */
			memcpy(&sc->grf, &con_itesoftc.grf,
			    (char *)&sc[1] - (char *)&sc->grf);
			con_itesoftc.grf = NULL;
			kbd_ite = sc;
		}
		sc->grf = gsc;
		splx(s);

		iteinit(gsc->g_itedev);
		printf(": %dx%d", sc->rows, sc->cols);
		printf(" repeat at (%d/100)s next at (%d/100)s",
		    start_repeat_timeo, next_repeat_timeo);

		if (kbd_ite == NULL)
			kbd_ite = sc;
		if (kbd_ite == sc)
			printf(" has keyboard");
		printf("\n");
		sc->flags |= ITE_ATTACHED;
 	} else {
		if (con_itesoftc.grf != NULL &&
		    con_itesoftc.grf->g_conpri > gsc->g_conpri)
			return;
		con_itesoftc.grf = gsc;
		con_itesoftc.tabs = cons_tabs;
	}
}

static struct ite_softc *
getitesp(dev_t dev)
{

	if (atari_realconfig && (con_itesoftc.grf == NULL))
		return(device_lookup_private(&ite_cd, ITEUNIT(dev)));

	if (con_itesoftc.grf == NULL)
		panic("no ite_softc for console");
	return(&con_itesoftc);
}

/*
 * cons.c entry points into ite device.
 */

/*
 * Return a priority in consdev->cn_pri field highest wins.  This function
 * is called before any devices have been probed.
 */
void
itecnprobe(struct consdev *cd)
{
	/* 
	 * return priority of the best ite (already picked from attach)
	 * or CN_DEAD.
	 */
	if (con_itesoftc.grf == NULL)
		cd->cn_pri = CN_DEAD;
	else {
		cd->cn_pri = con_itesoftc.grf->g_conpri;
		cd->cn_dev = con_itesoftc.grf->g_itedev;
	}
}

void
itecninit(struct consdev *cd)
{
	struct ite_softc *sc;

	sc = getitesp(cd->cn_dev);
	sc->flags |= ITE_ISCONS;
	iteinit(cd->cn_dev);
	sc->flags |= ITE_ACTIVE | ITE_ISCONS;
}

/*
 * ite_cnfinish() is called in ite_init() when the device is
 * being probed in the normal fasion, thus we can finish setting
 * up this ite now that the system is more functional.
 */
void
ite_cnfinish(struct ite_softc *sc)
{
	static int done;

	if (done)
		return;
	done = 1;
}

int
itecngetc(dev_t dev)
{
	int c;

	do {
		c = kbdgetcn();
		c = ite_cnfilter(c, ITEFILT_CONSOLE);
	} while (c == -1);
	return (c);
}

void
itecnputc(dev_t dev, int c)
{
	static int paniced;
	struct ite_softc *sc;
	char ch;

	sc = getitesp(dev);
	ch = c;

	if (panicstr && !paniced &&
	    (sc->flags & (ITE_ACTIVE | ITE_INGRF)) != ITE_ACTIVE) {
		ite_on(dev, 3);
		paniced = 1;
	}
	SUBR_CURSOR(sc, START_CURSOROPT);
	iteputchar(ch, sc);
	SUBR_CURSOR(sc, END_CURSOROPT);
}

/*
 * standard entry points to the device.
 */

/* 
 * iteinit() is the standard entry point for initialization of
 * an ite device, it is also called from itecninit().
 *
 */
void
iteinit(dev_t dev)
{
	struct ite_softc	*sc;

	sc = getitesp(dev);
	if (sc->flags & ITE_INITED)
		return;
	if (atari_realconfig) {
		if (sc->kbdmap && sc->kbdmap != &ascii_kbdmap)
			free(sc->kbdmap, M_DEVBUF);
		sc->kbdmap = malloc(sizeof(struct kbdmap), M_DEVBUF, M_WAITOK);
		memcpy(sc->kbdmap, &ascii_kbdmap, sizeof(struct kbdmap));
	}
	else
		sc->kbdmap = &ascii_kbdmap;

	sc->cursorx = 0;
	sc->cursory = 0;
	SUBR_INIT(sc);
	SUBR_CURSOR(sc, DRAW_CURSOR);
	if (sc->tabs == NULL)
		sc->tabs = malloc(MAX_TABS * sizeof(u_char),M_DEVBUF,M_WAITOK);
	ite_reset(sc);
	sc->flags |= ITE_INITED;
}

int
iteopen(dev_t dev, int mode, int devtype, struct lwp *l)
{
	struct ite_softc *sc;
	struct tty *tp;
	int error, first, unit;

	unit = ITEUNIT(dev);
	if (unit >= ite_cd.cd_ndevs)
		return ENXIO;

	first = 0;
	sc = getitesp(dev);
	if (sc == NULL)
		return ENXIO;
	if ((sc->flags & ITE_ATTACHED) == 0)
		return (ENXIO);

	if (sc->tp == NULL) {
		tp = sc->tp = tty_alloc();
		tty_attach(tp);
	}
	else
		tp = sc->tp;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	if ((sc->flags & ITE_ACTIVE) == 0) {
		ite_on(dev, 0);
		first = 1;
	}
	if (!(tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
		tp->t_oproc = itestart;
		tp->t_param = ite_param;
		tp->t_dev = dev;
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		tp->t_state = TS_CARR_ON;
		ttychars(tp);
		ttsetwater(tp);
	}


	error = ttyopen(tp, 0, (mode & O_NONBLOCK) ? 1 : 0);
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open) (dev, tp);
	if (error)
		goto bad;

	tp->t_winsize.ws_row = sc->rows;
	tp->t_winsize.ws_col = sc->cols;
	if (!kbd_init) {
		kbd_init = 1;
		kbdenable();
	}
	return (0);


bad:
	if (first)
		ite_off(dev, 0);

	return (error);
}

int
iteclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tty *tp;

	tp = getitesp(dev)->tp;

	KDASSERT(tp);
	(*tp->t_linesw->l_close) (tp, flag);
	ttyclose(tp);
	ite_off(dev, 0);
	return (0);
}

int
iteread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;

	tp = getitesp(dev)->tp;

	KDASSERT(tp);
	return ((*tp->t_linesw->l_read) (tp, uio, flag));
}

int
itewrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp;

	tp = getitesp(dev)->tp;

	KDASSERT(tp);
	return ((*tp->t_linesw->l_write) (tp, uio, flag));
}

int
itepoll(dev_t dev, int events, struct lwp *l)
{
	struct tty *tp;

	tp = getitesp(dev)->tp;

	KDASSERT(tp);
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
itetty(dev_t dev)
{
	return(getitesp(dev)->tp);
}

int
iteioctl(dev_t dev, u_long cmd, void * addr, int flag, struct lwp *l)
{
	struct iterepeat	*irp;
	struct ite_softc	*sc;
	struct tty		*tp;
	view_t			*view;
	struct itewinsize	*is;
	struct itebell		*ib;
	int error;
	
	sc   = getitesp(dev);
	tp   = sc->tp;
	view = viewview(sc->grf->g_viewdev);

	KDASSERT(tp);

	error = (*tp->t_linesw->l_ioctl) (tp, cmd, addr, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, addr, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {
	case ITEIOCSKMAP:
		if (addr == NULL)
			return(EFAULT);
		memcpy(sc->kbdmap, addr, sizeof(struct kbdmap));
		return 0;
	case ITEIOCSSKMAP:
		if (addr == NULL)
			return(EFAULT);
		memcpy(&ascii_kbdmap, addr, sizeof(struct kbdmap));
		return 0;
	case ITEIOCGKMAP:
		if (addr == NULL)
			return(EFAULT);
		memcpy(addr, sc->kbdmap, sizeof(struct kbdmap));
		return 0;
	case ITEIOCGREPT:
		if (addr == NULL)
			return(EFAULT);
		irp = (struct iterepeat *)addr;
		irp->start = start_repeat_timeo;
		irp->next = next_repeat_timeo;
		return 0;
	case ITEIOCSREPT:
		if (addr == NULL)
			return(EFAULT);
		irp = (struct iterepeat *)addr;
		if (irp->start < ITEMINREPEAT || irp->next < ITEMINREPEAT)
			return(EINVAL);
		start_repeat_timeo = irp->start;
		next_repeat_timeo = irp->next;
		return 0;
	case ITEIOCGWINSZ:
		if (addr == NULL)
			return(EFAULT);
		is         = (struct itewinsize *)addr;
		is->x      = view->display.x;
		is->y      = view->display.y;
		is->width  = view->display.width;
		is->height = view->display.height;
		is->depth  = view->bitmap->depth;
		return 0;
	case ITEIOCDSPWIN:
		sc->grf->g_mode(sc->grf, GM_GRFON, NULL, 0, 0);
		return 0;
	case ITEIOCREMWIN:
		sc->grf->g_mode(sc->grf, GM_GRFOFF, NULL, 0, 0);
		return 0;
	case ITEIOCSBELL:
		if (addr == NULL)
			return(EFAULT);
		ib = (struct itebell *)addr;
		kbd_bell_sparms(ib->volume, ib->pitch, ib->msec);
		return 0;
	case ITEIOCGBELL:
		if (addr == NULL)
			return(EFAULT);
		ib = (struct itebell *)addr;
		kbd_bell_gparms(&ib->volume, &ib->pitch, &ib->msec);
		return 0;
	}
	return (sc->itexx_ioctl)(sc, cmd, addr, flag, l);
}

void
itestart(struct tty *tp)
{
	struct clist *rbp;
	u_char buf[ITEBURST];
	int s, len;

	KDASSERT(tp);

	s = spltty(); {
		if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
			goto out;

		tp->t_state |= TS_BUSY;
		rbp = &tp->t_outq;
		
		len = q_to_b(rbp, buf, ITEBURST);
	} splx(s);

	/* Here is a really good place to implement pre/jumpscroll() */
	ite_putstr((char *)buf, len, tp->t_dev);

	s = spltty(); {
		tp->t_state &= ~TS_BUSY;
		/* we have characters remaining. */
		if (ttypull(tp)) {
			tp->t_state |= TS_TIMEOUT;
			callout_schedule(&tp->t_rstrt_ch, 1);
		}
	      out: ;
	} splx(s);
}

void
ite_on(dev_t dev, int flag)
{
	struct ite_softc *sc;
	sc = getitesp(dev); 

	/* force ite active, overriding graphics mode */
	if (flag & 1) {
		sc->flags |= ITE_ACTIVE;
		sc->flags &= ~(ITE_INGRF | ITE_INITED);
	}
	/* leave graphics mode */
	if (flag & 2) {
		sc->flags &= ~ITE_INGRF;
		if ((sc->flags & ITE_ACTIVE) == 0)
			return;
	}
	sc->flags |= ITE_ACTIVE;
	if (sc->flags & ITE_INGRF)
		return;
	iteinit(dev);
}

void
ite_off(dev_t dev, int flag)
{
	struct ite_softc *sc;

	sc = getitesp(dev);
	if (flag & 2)
		sc->flags |= ITE_INGRF;
	if ((sc->flags & ITE_ACTIVE) == 0)
		return;
	if ((flag & 1) ||
	    (sc->flags & (ITE_INGRF | ITE_ISCONS | ITE_INITED)) == ITE_INITED)
		SUBR_DEINIT(sc);
	if ((flag & 2) == 0)	/* XXX hmm grfon() I think wants this to  go inactive. */
		sc->flags &= ~ITE_ACTIVE;
}

static void
ite_switch(int unit)
{
	struct ite_softc	*sc;
	extern const struct cdevsw view_cdevsw;

	sc = device_lookup_private(&ite_cd, unit);
	if (sc == NULL || (sc->flags & (ITE_ATTACHED | ITE_INITED)) == 0)
		return;

	/*
	 * If switching to an active ite, also switch the keyboard.
	 */
	if (sc->flags & ITE_ACTIVE)
		kbd_ite = sc;

	/*
	 * Now make it visible
	 */
	(*view_cdevsw.d_ioctl)(sc->grf->g_viewdev, VIOCDISPLAY, NULL,
			       0, NOLWP);

	/*
	 * Make sure the cursor's there too....
	 */
  	SUBR_CURSOR(sc, DRAW_CURSOR);
}

/* XXX called after changes made in underlying grf layer. */
/* I want to nuke this */
void
ite_reinit(dev_t dev)
{
	struct ite_softc *sc;

	sc = getitesp(dev);
	sc->flags &= ~ITE_INITED;
	iteinit(dev);
}

int
ite_param(struct tty *tp, struct termios *t)
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return (0);
}

void
ite_reset(struct ite_softc *sc)
{
	int i;

	sc->curx = 0;
	sc->cury = 0;
	sc->attribute = ATTR_NOR;
	sc->save_curx = 0;
	sc->save_cury = 0;
	sc->save_attribute = ATTR_NOR;
	sc->ap = sc->argbuf;
	sc->emul_level = 0;
	sc->eightbit_C1 = 0;
	sc->top_margin = 0;
	sc->bottom_margin = sc->rows - 1;
	sc->inside_margins = 0;
	sc->linefeed_newline = 0;
	sc->auto_wrap = ite_default_wrap;
	sc->cursor_appmode = 0;
	sc->keypad_appmode = 0;
	sc->imode = 0;
	sc->key_repeat = 1;
	memset(sc->tabs, 0, sc->cols);
	for (i = 0; i < sc->cols; i++)
		sc->tabs[i] = ((i & 7) == 0);
}

/*
 * has to be global because of the shared filters.
 */
static u_char last_dead;

/*
 * Used in console at startup only and for DDB.
 */
int
ite_cnfilter(u_int c, enum caller caller)
{
	struct key	key;
	struct kbdmap	*kbdmap;
	u_char		code, up;
	int		s;

	up   = KBD_RELEASED(c);
	c    = KBD_SCANCODE(c);
	code = 0;
	kbdmap = (kbd_ite == NULL) ? &ascii_kbdmap : kbd_ite->kbdmap;

	s = spltty();

	/*
	 * No special action if key released
	 */
	if (up) {
		splx(s);
		return -1;
	}
	
	/* translate modifiers */
	if (kbd_modifier & KBD_MOD_SHIFT) {
		if (kbd_modifier & KBD_MOD_ALT)
			key = kbdmap->alt_shift_keys[c];
		else
			key = kbdmap->shift_keys[c];
	}
	else if (kbd_modifier & KBD_MOD_ALT)
			key = kbdmap->alt_keys[c];
	else {
		key = kbdmap->keys[c];
		/*
		 * If CAPS and key is CAPable (no pun intended)
		 */
		if ((kbd_modifier & KBD_MOD_CAPS) && (key.mode & KBD_MODE_CAPS))
			key = kbdmap->shift_keys[c];
	}
	code = key.code;

#ifdef notyet /* LWP: Didn't have time to look at this yet */
	/*
	 * If string return simple console filter
	 */
	if (key->mode & (KBD_MODE_STRING | KBD_MODE_KPAD)) {
		splx(s);
		return -1;
	}
	/* handle dead keys */
	if (key->mode & KBD_MODE_DEAD) {
		/* if entered twice, send accent itself */
		if (last_dead == key->mode & KBD_MODE_ACCMASK)
			last_dead = 0;
		else {
			last_dead = key->mode & KBD_MODE_ACCMASK;
			splx(s);
			return -1;
		}
	}
	if (last_dead) {
		/* can't apply dead flag to string-keys */
		if (code >= '@' && code < 0x7f)
			code =
			    acctable[KBD_MODE_ACCENT(last_dead)][code - '@'];
		last_dead = 0;
	}
#endif
	if (kbd_modifier & KBD_MOD_CTRL)
		code &= 0x1f;

	/*
	 * Do console mapping.
	 */
	code = code == '\r' ? '\n' : code;

	splx(s);
	return (code);
}

/* And now the old stuff. */

/* these are used to implement repeating keys.. */
static u_int last_char;
static u_char tout_pending;

static callout_t repeat_ch;

/*ARGSUSED*/
static void
repeat_handler(void *arg)
{
	tout_pending = 0;
	if (last_char) 
		add_sicallback((si_farg)ite_filter, (void *)last_char,
						    (void *)ITEFILT_REPEATER);
}

void
ite_filter(u_int c, enum caller caller)
{
	struct tty	*kbd_tty;
	struct kbdmap	*kbdmap;
	u_char		code, *str, up;
	struct key	key;
	int		s, i;
	static bool	again;

	if (!again) {
		/* XXX */
		callout_init(&repeat_ch, 0);
		again = true;
	}

	if (kbd_ite == NULL)
		return;

	kbd_tty = kbd_ite->tp;
	kbdmap  = kbd_ite->kbdmap;

	up   = KBD_RELEASED(c);
	c    = KBD_SCANCODE(c);
	code = 0;

	/* have to make sure we're at spltty in here */
	s = spltty();

	/* 
	 * keyboard interrupts come at priority 2, while softint
	 * generated keyboard-repeat interrupts come at level 1.  So,
	 * to not allow a key-up event to get thru before a repeat for
	 * the key-down, we remove any outstanding callout requests..
	 */
	rem_sicallback((si_farg)ite_filter);

	/*
	 * Stop repeating on up event
	 */
	if (up) {
		if (tout_pending) {
			callout_stop(&repeat_ch);
			tout_pending = 0;
			last_char    = 0;
		}
		splx(s);
		return;
	}
	else if (tout_pending && last_char != c) {
		/*
		 * Different character, stop also
		 */
		callout_stop(&repeat_ch);
		tout_pending = 0;
		last_char    = 0;
	}

	/*
	 * Handle ite-switching ALT + Fx
	 */
	if ((kbd_modifier == KBD_MOD_ALT) && (c >= 0x3b) && (c <= 0x44)) {
		ite_switch(c - 0x3b);
		splx(s);
		return;
	}
	/*
	 * Safety button, switch back to ascii keymap.
	 */
	if (kbd_modifier == (KBD_MOD_ALT | KBD_MOD_LSHIFT) && c == 0x3b) {
		/* ALT + LSHIFT + F1 */
		memcpy(kbdmap, &ascii_kbdmap, sizeof(struct kbdmap));
		splx(s);
		return;
#ifdef DDB
	} else if (kbd_modifier == (KBD_MOD_ALT | KBD_MOD_LSHIFT) &&
	    c == 0x43) {
		/*
		 * ALT + LSHIFT + F9 -> Debugger!
		 */
		Debugger();
		splx(s);
		return;
#endif
	}

	/*
	 * The rest of the code is senseless when the device is not open.
	 */
	if (kbd_tty == NULL) {
		splx(s);
		return;
	}

	/*
	 * Translate modifiers
	 */
	if (kbd_modifier & KBD_MOD_SHIFT) {
		if (kbd_modifier & KBD_MOD_ALT)
			key = kbdmap->alt_shift_keys[c];
		else
			key = kbdmap->shift_keys[c];
	}
	else if (kbd_modifier & KBD_MOD_ALT)
		key = kbdmap->alt_keys[c];
	else {
		key = kbdmap->keys[c];
		/*
		 * If CAPS and key is CAPable (no pun intended)
		 */
		if ((kbd_modifier & KBD_MOD_CAPS) && (key.mode & KBD_MODE_CAPS))
			key = kbdmap->shift_keys[c];
	}
	code = key.code;

	/* 
	 * Arrange to repeat the keystroke. By doing this at the level
	 * of scan-codes, we can have function keys, and keys that
	 * send strings, repeat too. This also entitles an additional
	 * overhead, since we have to do the conversion each time, but
	 * I guess that's ok.
	 */
	if (!tout_pending && caller == ITEFILT_TTY && kbd_ite->key_repeat) {
		tout_pending = 1;
		last_char    = c;
		callout_reset(&repeat_ch, start_repeat_timeo * hz / 100,
		    repeat_handler, NULL);
	} else if (!tout_pending && caller==ITEFILT_REPEATER &&
	    kbd_ite->key_repeat) {
		tout_pending = 1;
		last_char    = c;
		callout_reset(&repeat_ch, next_repeat_timeo * hz / 100,
		    repeat_handler, NULL);
	}
	/* handle dead keys */
	if (key.mode & KBD_MODE_DEAD) {
		/* if entered twice, send accent itself */
		if (last_dead == (key.mode & KBD_MODE_ACCMASK))
			last_dead = 0;
		else {
			last_dead = key.mode & KBD_MODE_ACCMASK;
			splx(s);
			return;
		}
	}
	if (last_dead) {
		/* can't apply dead flag to string-keys */
		if (!(key.mode & KBD_MODE_STRING) && code >= '@' &&
		    code < 0x7f)
			code = acctable[KBD_MODE_ACCENT(last_dead)][code - '@'];
		last_dead = 0;
	}

	/*
	 * If not string, apply CTRL modifiers
	 */
	if (!(key.mode & KBD_MODE_STRING) &&
	    (!(key.mode & KBD_MODE_KPAD) ||
	     (kbd_ite && !kbd_ite->keypad_appmode))) {
		if (kbd_modifier & KBD_MOD_CTRL)
			code &= 0x1f;
	} else if ((key.mode & KBD_MODE_KPAD) &&
	    (kbd_ite && kbd_ite->keypad_appmode)) {
		static const char * const in  = "0123456789-+.\r()/*";
		static const char * const out = "pqrstuvwxymlnMPQRS";
			   char *cp  = strchr(in, code);

		/* 
		 * keypad-appmode sends SS3 followed by the above
		 * translated character
		 */
		kbd_tty->t_linesw->l_rint(27, kbd_tty);
		kbd_tty->t_linesw->l_rint('O', kbd_tty);
		kbd_tty->t_linesw->l_rint(out[cp - in], kbd_tty);
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

		str = kbdmap->strings + code;
		/* 
		 * if this is a cursor key, AND it has the default
		 * keymap setting, AND we're in app-cursor mode, switch
		 * to the above table. This is *nasty* !
		 */
		if (((c == 0x48) || (c == 0x4b) ||(c == 0x4d) || (c == 0x50)) &&
		     kbd_ite->cursor_appmode &&
		    !memcmp(str, "\x03\x1b[", 3) &&
		    strchr("ABCD", str[3]))
			str = app_cursor + 4 * (str[3] - 'A');

		/* 
		 * using a length-byte instead of 0-termination allows
		 * to embed \0 into strings, although this is not used
		 * in the default keymap
		 */
		for (i = *str++; i; i--)
			kbd_tty->t_linesw->l_rint(*str++, kbd_tty);
		splx(s);
		return;
	}
	kbd_tty->t_linesw->l_rint(code, kbd_tty);

	splx(s);
	return;
}

/* helper functions, makes the code below more readable */
static inline void
ite_sendstr(const char *str)
{
	struct tty *kbd_tty;

	kbd_tty = kbd_ite->tp;
	KDASSERT(kbd_tty);
	while (*str)
		kbd_tty->t_linesw->l_rint(*str++, kbd_tty);
}

static void
alignment_display(struct ite_softc *sc)
{
  int i, j;

  for (j = 0; j < sc->rows; j++)
    for (i = 0; i < sc->cols; i++)
      SUBR_PUTC(sc, 'E', j, i, ATTR_NOR);
  attrclr(sc, 0, 0, sc->rows, sc->cols);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}

static inline void
snap_cury(struct ite_softc *sc)
{
  if (sc->inside_margins)
    {
      if (sc->cury < sc->top_margin)
	sc->cury = sc->top_margin;
      if (sc->cury > sc->bottom_margin)
	sc->cury = sc->bottom_margin;
    }
}

static inline void
ite_dnchar(struct ite_softc *sc, int n)
{
  n = min(n, sc->cols - sc->curx);
  if (n < sc->cols - sc->curx)
    {
      SUBR_SCROLL(sc, sc->cury, sc->curx + n, n, SCROLL_LEFT);
      attrmov(sc, sc->cury, sc->curx + n, sc->cury, sc->curx,
	      1, sc->cols - sc->curx - n);
      attrclr(sc, sc->cury, sc->cols - n, 1, n);
    }
  while (n-- > 0)
    SUBR_PUTC(sc, ' ', sc->cury, sc->cols - n - 1, ATTR_NOR);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}

static inline void
ite_inchar(struct ite_softc *sc, int n)
{
  n = min(n, sc->cols - sc->curx);
  if (n < sc->cols - sc->curx)
    {
      SUBR_SCROLL(sc, sc->cury, sc->curx, n, SCROLL_RIGHT);
      attrmov(sc, sc->cury, sc->curx, sc->cury, sc->curx + n,
	      1, sc->cols - sc->curx - n);
      attrclr(sc, sc->cury, sc->curx, 1, n);
    }
  while (n--)
    SUBR_PUTC(sc, ' ', sc->cury, sc->curx + n, ATTR_NOR);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}

static inline void
ite_clrtoeol(struct ite_softc *sc)
{
  int y = sc->cury, x = sc->curx;
  if (sc->cols - x > 0)
    {
      SUBR_CLEAR(sc, y, x, 1, sc->cols - x);
      attrclr(sc, y, x, 1, sc->cols - x);
      SUBR_CURSOR(sc, DRAW_CURSOR);
    }
}

static inline void
ite_clrtobol(struct ite_softc *sc)
{
  int y = sc->cury, x = min(sc->curx + 1, sc->cols);
  SUBR_CLEAR(sc, y, 0, 1, x);
  attrclr(sc, y, 0, 1, x);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}

static inline void
ite_clrline(struct ite_softc *sc)
{
  int y = sc->cury;
  SUBR_CLEAR(sc, y, 0, 1, sc->cols);
  attrclr(sc, y, 0, 1, sc->cols);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}



static inline void
ite_clrtoeos(struct ite_softc *sc)
{
  ite_clrtoeol(sc);
  if (sc->cury < sc->rows - 1)
    {
      SUBR_CLEAR(sc, sc->cury + 1, 0, sc->rows - 1 - sc->cury, sc->cols);
      attrclr(sc, sc->cury, 0, sc->rows - sc->cury, sc->cols);
      SUBR_CURSOR(sc, DRAW_CURSOR);
    }
}

static inline void
ite_clrtobos(struct ite_softc *sc)
{
  ite_clrtobol(sc);
  if (sc->cury > 0)
    {
      SUBR_CLEAR(sc, 0, 0, sc->cury, sc->cols);
      attrclr(sc, 0, 0, sc->cury, sc->cols);
      SUBR_CURSOR(sc, DRAW_CURSOR);
    }
}

static inline void
ite_clrscreen(struct ite_softc *sc)
{
  SUBR_CLEAR(sc, 0, 0, sc->rows, sc->cols);
  attrclr(sc, 0, 0, sc->rows, sc->cols);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}



static inline void
ite_dnline(struct ite_softc *sc, int n)
{
  /* interesting.. if the cursor is outside the scrolling
     region, this command is simply ignored.. */
  if (sc->cury < sc->top_margin || sc->cury > sc->bottom_margin)
    return;

  n = min(n, sc->bottom_margin + 1 - sc->cury);
  if (n <= sc->bottom_margin - sc->cury)
    {
      SUBR_SCROLL(sc, sc->cury + n, 0, n, SCROLL_UP);
      attrmov(sc, sc->cury + n, 0, sc->cury, 0,
	      sc->bottom_margin + 1 - sc->cury - n, sc->cols);
    }
  SUBR_CLEAR(sc, sc->bottom_margin - n + 1, 0, n, sc->cols);
  attrclr(sc, sc->bottom_margin - n + 1, 0, n, sc->cols);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}

static inline void
ite_inline(struct ite_softc *sc, int n)
{
  /* interesting.. if the cursor is outside the scrolling
     region, this command is simply ignored.. */
  if (sc->cury < sc->top_margin || sc->cury > sc->bottom_margin)
    return;

  n = min(n, sc->bottom_margin + 1 - sc->cury);
  if (n <= sc->bottom_margin - sc->cury)
    {
      SUBR_SCROLL(sc, sc->cury, 0, n, SCROLL_DOWN);
      attrmov(sc, sc->cury, 0, sc->cury + n, 0,
	      sc->bottom_margin + 1 - sc->cury - n, sc->cols);
    }
  SUBR_CLEAR(sc, sc->cury, 0, n, sc->cols);
  attrclr(sc, sc->cury, 0, n, sc->cols);
  SUBR_CURSOR(sc, DRAW_CURSOR);
}

static inline void
ite_lf (struct ite_softc *sc)
{
  ++sc->cury;
  if ((sc->cury == sc->bottom_margin+1) || (sc->cury == sc->rows))
    {
      sc->cury--;
      SUBR_SCROLL(sc, sc->top_margin + 1, 0, 1, SCROLL_UP);
      ite_clrline(sc);
    }
  SUBR_CURSOR(sc, MOVE_CURSOR);
  clr_attr(sc, ATTR_INV);
}

static inline void
ite_crlf (struct ite_softc *sc)
{
  sc->curx = 0;
  ite_lf (sc);
}

static inline void
ite_cr (struct ite_softc *sc)
{
  if (sc->curx)
    {
      sc->curx = 0;
      SUBR_CURSOR(sc, MOVE_CURSOR);
    }
}

static inline void
ite_rlf (struct ite_softc *sc)
{
  sc->cury--;
  if ((sc->cury < 0) || (sc->cury == sc->top_margin - 1))
    {
      sc->cury++;
      SUBR_SCROLL(sc, sc->top_margin, 0, 1, SCROLL_DOWN);
      ite_clrline(sc);
    }
  SUBR_CURSOR(sc, MOVE_CURSOR);
  clr_attr(sc, ATTR_INV);
}

static inline int
atoi (const char *cp)
{
  int n;

  for (n = 0; *cp && *cp >= '0' && *cp <= '9'; cp++)
    n = n * 10 + *cp - '0';

  return n;
}

static inline int
ite_argnum (struct ite_softc *sc)
{
  char ch;
  int n;

  /* convert argument string into number */
  if (sc->ap == sc->argbuf)
    return 1;
  ch = *sc->ap;
  *sc->ap = 0;
  n = atoi (sc->argbuf);
  *sc->ap = ch;
  
  return n;
}

static inline int
ite_zargnum (struct ite_softc *sc)
{
  char ch;
  int n;

  /* convert argument string into number */
  if (sc->ap == sc->argbuf)
    return 0;
  ch = *sc->ap;
  *sc->ap = 0;
  n = atoi (sc->argbuf);
  *sc->ap = ch;
  
  return n;	/* don't "n ? n : 1" here, <CSI>0m != <CSI>1m ! */
}

void
ite_putstr(const u_char *s, int len, dev_t dev)
{
	struct ite_softc *sc;
	int i;
	
	sc = getitesp(dev);

	/* XXX avoid problems */
	if ((sc->flags & (ITE_ACTIVE|ITE_INGRF)) != ITE_ACTIVE)
	  	return;

	SUBR_CURSOR(sc, START_CURSOROPT);
	for (i = 0; i < len; i++)
		if (s[i])
			iteputchar(s[i], sc);
	SUBR_CURSOR(sc, END_CURSOROPT);
}


void
iteputchar(register int c, struct ite_softc *sc)
{
	struct tty *kbd_tty;
	int n, x, y;
	char *cp;

	if (kbd_ite == NULL)
		kbd_tty = NULL;
	else
		kbd_tty = kbd_ite->tp;

	if (sc->escape) 
	  {
	    switch (sc->escape) 
	      {
	      case ESC:
	        switch (c)
	          {
		  /* first 7bit equivalents for the 8bit control characters */
		  
	          case 'D':
		    c = IND;
		    sc->escape = 0;
		    break; /* and fall into the next switch below (same for all `break') */
		    
		  case 'E':
		    c = NEL;
		    sc->escape = 0;
		    break;
		    
		  case 'H':
		    c = HTS;
		    sc->escape = 0;
		    break;
		    
		  case 'M':
		    c = RI;
		    sc->escape = 0;
		    break;
		    
		  case 'N':
		    c = SS2;
		    sc->escape = 0;
		    break;
		  
		  case 'O':
		    c = SS3;
		    sc->escape = 0;
		    break;
		    
		  case 'P':
		    c = DCS;
		    sc->escape = 0;
		    break;
		    
		  case '[':
		    c = CSI;
		    sc->escape = 0;
		    break;
		    
		  case '\\':
		    c = ST;
		    sc->escape = 0;
		    break;
		    
		  case ']':
		    c = OSC;
		    sc->escape = 0;
		    break;
		    
		  case '^':
		    c = PM;
		    sc->escape = 0;
		    break;
		    
		  case '_':
		    c = APC;
		    sc->escape = 0;
		    break;


		  /* introduces 7/8bit control */
		  case ' ':
		     /* can be followed by either F or G */
		     sc->escape = ' ';
		     break;

		  
		  /* a lot of character set selections, not yet used... 
		     94-character sets: */
		  case '(':	/* G0 */
		  case ')':	/* G1 */
		    sc->escape = c;
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
		    
		    sc->escape = 0;
		    /* just ignore for now */
		    return;
		    
		  
		  /* locking shift modes (as you might guess, not yet supported..) */
		  case '`':
		    sc->GR = sc->G1;
		    sc->escape = 0;
		    return;
		    
		  case 'n':
		    sc->GL = sc->G2;
		    sc->escape = 0;
		    return;
		    
		  case '}':
		    sc->GR = sc->G2;
		    sc->escape = 0;
		    return;
		    
		  case 'o':
		    sc->GL = sc->G3;
		    sc->escape = 0;
		    return;
		    
		  case '|':
		    sc->GR = sc->G3;
		    sc->escape = 0;
		    return;
		    
		  
		  /* font width/height control */
		  case '#':
		    sc->escape = '#';
		    return;
		    
		    
		  /* hard terminal reset .. */
		  case 'c':
		    ite_reset (sc);
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    sc->escape = 0;
		    return;


		  case '7':
		    sc->save_curx = sc->curx;
		    sc->save_cury = sc->cury;
		    sc->save_attribute = sc->attribute;
		    sc->escape = 0;
		    return;
		    
		  case '8':
		    sc->curx = sc->save_curx;
		    sc->cury = sc->save_cury;
		    sc->attribute = sc->save_attribute;
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    sc->escape = 0;
		    return;
		    
		  case '=':
		    sc->keypad_appmode = 1;
		    sc->escape = 0;
		    return;
		    
		  case '>':
		    sc->keypad_appmode = 0;
		    sc->escape = 0;
		    return;
		  
		  case 'Z':	/* request ID */
		    if (sc->emul_level == EMUL_VT100)
		      ite_sendstr ("\033[?61;0c"); /* XXX not clean */
		    else
		      ite_sendstr ("\033[?63;0c"); /* XXX not clean */
		    sc->escape = 0;
		    return;

		  /* default catch all for not recognized ESC sequences */
		  default:
		    sc->escape = 0;
		    return;
		  }
		break;


	      case '(':
	      case ')':
		sc->escape = 0;
		return;


	      case ' ':
	        switch (c)
	          {
	          case 'F':
		    sc->eightbit_C1 = 0;
		    sc->escape = 0;
		    return;
		    
		  case 'G':
		    sc->eightbit_C1 = 1;
		    sc->escape = 0;
		    return;
		    
		  default:
		    /* not supported */
		    sc->escape = 0;
		    return;
		  }
		break;
		
		
	      case '#':
		switch (c)
		  {
		  case '5':
		    /* single height, single width */
		    sc->escape = 0;
		    return;
		    
		  case '6':
		    /* double width, single height */
		    sc->escape = 0;
		    return;
		    
		  case '3':
		    /* top half */
		    sc->escape = 0;
		    return;
		    
		  case '4':
		    /* bottom half */
		    sc->escape = 0;
		    return;
		    
		  case '8':
		    /* screen alignment pattern... */
		    alignment_display (sc);
		    sc->escape = 0;
		    return;
		    
		  default:
		    sc->escape = 0;
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
	            if (sc->ap < sc->argbuf + MAX_ARGSIZE)
	              *sc->ap++ = c;
	            return;

		  case BS:
		    /* you wouldn't believe such perversion is possible?
		       it is.. BS is allowed in between cursor sequences
		       (at least), according to vttest.. */
		    if (--sc->curx < 0)
		      sc->curx = 0;
		    else
		      SUBR_CURSOR(sc, MOVE_CURSOR);
		    break;

	          case 'p':
		    *sc->ap = 0;
	            if (! strncmp (sc->argbuf, "61\"", 3))
	              sc->emul_level = EMUL_VT100;
	            else if (! strncmp (sc->argbuf, "63;1\"", 5)
	            	     || ! strncmp (sc->argbuf, "62;1\"", 5))
	              sc->emul_level = EMUL_VT300_7;
	            else
	              sc->emul_level = EMUL_VT300_8;
	            sc->escape = 0;
	            return;
	            
	          
	          case '?':
		    *sc->ap = 0;
	            sc->escape = '?';
	            sc->ap = sc->argbuf;
	            return;


		  case 'c':
  		    *sc->ap = 0;
		    if (sc->argbuf[0] == '>')
		      {
		        ite_sendstr ("\033[>24;0;0;0c");
		      }
		    else switch (ite_zargnum(sc))
		      {
		      case 0:
			/* primary DA request, send primary DA response */
			if (sc->emul_level == EMUL_VT100)
		          ite_sendstr ("\033[?1;1c");
		        else
		          ite_sendstr ("\033[?63;1c");
			break;
		      }
		    sc->escape = 0;
		    return;

		  case 'n':
		    switch (ite_zargnum(sc))
		      {
		      case 5:
		        ite_sendstr ("\033[0n");	/* no malfunction */
			break;
		      case 6:
			/* cursor position report */
		        snprintf (sc->argbuf, sizeof(sc->argbuf), "\033[%d;%dR",
				 sc->cury + 1, sc->curx + 1);
			ite_sendstr (sc->argbuf);
			break;
		      }
		    sc->escape = 0;
		    return;
	          
  
		  case 'x':
		    switch (ite_zargnum(sc))
		      {
		      case 0:
			/* Fake some terminal parameters.  */
		        ite_sendstr ("\033[2;1;1;112;112;1;0x");
			break;
		      case 1:
		        ite_sendstr ("\033[3;1;1;112;112;1;0x");
			break;
		      }
		    sc->escape = 0;
		    return;


		  case 'g':
		    switch (ite_zargnum(sc))
		      {
		      case 0:
			if (sc->curx < sc->cols)
			  sc->tabs[sc->curx] = 0;
			break;
		      case 3:
		        for (n = 0; n < sc->cols; n++)
		          sc->tabs[n] = 0;
			break;
		      }
		    sc->escape = 0;
		    return;

	          
  	          case 'h': case 'l':
		    n = ite_zargnum (sc);
		    switch (n)
		      {
		      case 4:
		        sc->imode = (c == 'h');	/* insert/replace mode */
			break;
		      case 20:
			sc->linefeed_newline = (c == 'h');
			break;
		      }
		    sc->escape = 0;
		    return;


		  case 'M':
		    ite_dnline (sc, ite_argnum (sc));
	            sc->escape = 0;
	            return;

		  
		  case 'L':
		    ite_inline (sc, ite_argnum (sc));
	            sc->escape = 0;
	            return;


		  case 'P':
		    ite_dnchar (sc, ite_argnum (sc));
	            sc->escape = 0;
	            return;
		    

		  case '@':
		    ite_inchar (sc, ite_argnum (sc));
	            sc->escape = 0;
	            return;


		  case 'G':
		    /* this one was *not* in my vt320 manual but in 
		       a vt320 termcap entry.. who is right?
		       It's supposed to set the horizontal cursor position. */
		    *sc->ap = 0;
		    x = atoi (sc->argbuf);
		    if (x) x--;
		    sc->curx = min(x, sc->cols - 1);
		    sc->escape = 0;
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;


		  case 'd':
		    /* same thing here, this one's for setting the absolute
		       vertical cursor position. Not documented... */
		    *sc->ap = 0;
		    y = atoi (sc->argbuf);
		    if (y) y--;
		    if (sc->inside_margins)
		      y += sc->top_margin;
		    sc->cury = min(y, sc->rows - 1);
		    sc->escape = 0;
		    snap_cury(sc);
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;


		  case 'H':
		  case 'f':
		    *sc->ap = 0;
		    y = atoi (sc->argbuf);
		    x = 0;
		    cp = strchr(sc->argbuf, ';');
		    if (cp)
		      x = atoi (cp + 1);
		    if (x) x--;
		    if (y) y--;
		    if (sc->inside_margins)
		      y += sc->top_margin;
		    sc->cury = min(y, sc->rows - 1);
		    sc->curx = min(x, sc->cols - 1);
		    sc->escape = 0;
		    snap_cury(sc);
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;
		    
		  case 'A':		    
		    n = ite_argnum (sc);
		    n = sc->cury - (n ? n : 1);
		    if (n < 0) n = 0;
		    if (sc->inside_margins)
		      n = max(sc->top_margin, n);
		    else if (n == sc->top_margin - 1)
		      /* allow scrolling outside region, but don't scroll out
			 of active region without explicit CUP */
		      n = sc->top_margin;
		    sc->cury = n;
		    sc->escape = 0;
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;
		  
		  case 'B':
		    n = ite_argnum (sc);
		    n = sc->cury + (n ? n : 1);
		    n = min(sc->rows - 1, n);
		    if (sc->inside_margins)
		      n = min(sc->bottom_margin, n);
		    else if (n == sc->bottom_margin + 1)
		      /* allow scrolling outside region, but don't scroll out
			 of active region without explicit CUP */
		      n = sc->bottom_margin;
		    sc->cury = n;
		    sc->escape = 0;
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;
		  
		  case 'C':
		    n = ite_argnum (sc);
		    n = n ? n : 1;
		    sc->curx = min(sc->curx + n, sc->cols - 1);
		    sc->escape = 0;
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;
		  
		  case 'D':
		    n = ite_argnum (sc);
		    n = n ? n : 1;
		    n = sc->curx - n;
		    sc->curx = n >= 0 ? n : 0;
		    sc->escape = 0;
		    SUBR_CURSOR(sc, MOVE_CURSOR);
		    clr_attr (sc, ATTR_INV);
		    return;
		  
		    


		  case 'J':
		    *sc->ap = 0;
		    n = ite_zargnum (sc);
		    if (n == 0)
	              ite_clrtoeos(sc);
		    else if (n == 1)
		      ite_clrtobos(sc);
		    else if (n == 2)
		      ite_clrscreen(sc);
	            sc->escape = 0;
	            return;


		  case 'K':
		    n = ite_zargnum (sc);
		    if (n == 0)
		      ite_clrtoeol(sc);
		    else if (n == 1)
		      ite_clrtobol(sc);
		    else if (n == 2)
		      ite_clrline(sc);
		    sc->escape = 0;
		    return;


		  case 'X':
		    n = ite_argnum(sc) - 1;
		    n = min(n, sc->cols - 1 - sc->curx);
		    for (; n >= 0; n--)
		      {
			attrclr(sc, sc->cury, sc->curx + n, 1, 1);
			SUBR_PUTC(sc, ' ', sc->cury, sc->curx + n, ATTR_NOR);
		      }
		    sc->escape = 0;
		    return;

	          
	          case '}': case '`':
	            /* status line control */
	            sc->escape = 0;
	            return;


		  case 'r':
		    *sc->ap = 0;
		    x = atoi (sc->argbuf);
		    x = x ? x : 1;
		    y = sc->rows;
		    cp = strchr(sc->argbuf, ';');
		    if (cp)
		      {
			y = atoi (cp + 1);
			y = y ? y : sc->rows;
		      }
		    if (y - x < 2)
		      {
			/* if illegal scrolling region, reset to defaults */
			x = 1;
			y = sc->rows;
		      }
		    x--;
		    y--;
		    sc->top_margin = min(x, sc->rows - 1);
		    sc->bottom_margin = min(y, sc->rows - 1);
		    if (sc->inside_margins)
		      {
			sc->cury = sc->top_margin;
			sc->curx = 0;
			SUBR_CURSOR(sc, MOVE_CURSOR);
		      }
		    sc->escape = 0;
		    return;
		    
		  
		  case 'm':
		    /* big attribute setter/resetter */
		    {
		      char *chp;
		      *sc->ap = 0;
		      /* kludge to make CSIm work (== CSI0m) */
		      if (sc->ap == sc->argbuf)
		        sc->ap++;
		      for (chp = sc->argbuf; chp < sc->ap; )
		        {
			  switch (*chp)
			    {
			    case 0:
			    case '0':
			      clr_attr (sc, ATTR_ALL);
			      chp++;
			      break;
			      
			    case '1':
			      set_attr (sc, ATTR_BOLD);
			      chp++;
			      break;
			      
			    case '2':
			      switch (chp[1])
			        {
			        case '2':
			          clr_attr (sc, ATTR_BOLD);
			          chp += 2;
			          break;
			        
			        case '4':
			          clr_attr (sc, ATTR_UL);
			          chp += 2;
			          break;
			          
			        case '5':
			          clr_attr (sc, ATTR_BLINK);
			          chp += 2;
			          break;
			          
			        case '7':
			          clr_attr (sc, ATTR_INV);
			          chp += 2;
			          break;
		        	
		        	default:
		        	  chp++;
		        	  break;
		        	}
			      break;
			      
			    case '4':
			      set_attr (sc, ATTR_UL);
			      chp++;
			      break;
			      
			    case '5':
			      set_attr (sc, ATTR_BLINK);
			      chp++;
			      break;
			      
			    case '7':
			      set_attr (sc, ATTR_INV);
			      chp++;
			      break;
			    
			    default:
			      chp++;
			      break;
			    }
		        }
		    
		    }
		    sc->escape = 0;
		    return;


		  case 'u':
		    /* DECRQTSR */
		    ite_sendstr ("\033P\033\\");
		    sc->escape = 0;
		    return;

		  
		  
		  default:
		    sc->escape = 0;
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
	            if (sc->ap < sc->argbuf + MAX_ARGSIZE - 1)
	              *sc->ap++ = c;
	            return;


		  case 'n':
		    *sc->ap = 0;
		    if (sc->ap == &sc->argbuf[2])
		      {
		        if (! strncmp (sc->argbuf, "15", 2))
		          /* printer status: no printer */
		          ite_sendstr ("\033[13n");
		          
		        else if (! strncmp (sc->argbuf, "25", 2))
		          /* udk status */
		          ite_sendstr ("\033[20n");
		          
		        else if (! strncmp (sc->argbuf, "26", 2))
		          /* keyboard dialect: US */
		          ite_sendstr ("\033[27;1n");
		      }
		    sc->escape = 0;
		    return;


  		  case 'h': case 'l':
		    n = ite_zargnum (sc);
		    switch (n)
		      {
		      case 1:
		        sc->cursor_appmode = (c == 'h');
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
			sc->inside_margins = (c == 'h');
			sc->curx = 0;
			sc->cury = sc->inside_margins ? sc->top_margin : 0;
			SUBR_CURSOR(sc, MOVE_CURSOR);
			break;

		      case 7: /* auto wraparound */
			sc->auto_wrap = (c == 'h');
			break;

		      case 8: /* keyboard repeat */
			sc->key_repeat = (c == 'h');
			break;

		      case 20: /* newline mode */
			sc->linefeed_newline = (c == 'h');
			break;

		      case 25: /* cursor on/off */
			SUBR_CURSOR(sc, (c == 'h') ? DRAW_CURSOR : ERASE_CURSOR);
			break;
		      }
		    sc->escape = 0;
		    return;
		    
		  default:
		    sc->escape = 0;
		    return;
		  }
		break;

	      
	      default:
	        sc->escape = 0;
	        return;
	      }
          }

	switch (c) {

	case VT:	/* VT is treated like LF */
	case FF:	/* so is FF */
	case LF:
		/* cr->crlf distinction is done here, on output, 
		   not on input! */
		if (sc->linefeed_newline)
		  ite_crlf (sc);
		else
		  ite_lf (sc);
		break;

	case CR:
		ite_cr (sc);
		break;
	
	case BS:
		if (--sc->curx < 0)
			sc->curx = 0;
		else
			SUBR_CURSOR(sc, MOVE_CURSOR);
		break;

	case HT:
		for (n = sc->curx + 1; n < sc->cols; n++) {
			if (sc->tabs[n]) {
				sc->curx = n;
				SUBR_CURSOR(sc, MOVE_CURSOR);
				break;
			}
		}
		break;

	case BEL:
		if (kbd_tty && kbd_ite && kbd_ite->tp == kbd_tty)
			kbdbell();
		break;

	case SO:
		sc->GL = sc->G1;
		break;
		
	case SI:
		sc->GL = sc->G0;
		break;

	case ENQ:
		/* send answer-back message !! */
		break;

	case CAN:
		sc->escape = 0;	/* cancel any escape sequence in progress */
		break;
		
	case SUB:
		sc->escape = 0;	/* dito, but see below */
		/* should also display a reverse question mark!! */
		break;

	case ESC:
		sc->escape = ESC;
		break;


	/* now it gets weird.. 8bit control sequences.. */
	case IND:	/* index: move cursor down, scroll */
		ite_lf (sc);
		break;
		
	case NEL:	/* next line. next line, first pos. */
		ite_crlf (sc);
		break;

	case HTS:	/* set horizontal tab */
		if (sc->curx < sc->cols)
		  sc->tabs[sc->curx] = 1;
		break;
		
	case RI:	/* reverse index */
		ite_rlf (sc);
		break;

	case SS2:	/* go into G2 for one character */
		/* not yet supported */
		break;
		
	case SS3:	/* go into G3 for one character */
		break;
		
	case DCS:	/* device control string introducer */
		sc->escape = DCS;
		sc->ap = sc->argbuf;
		break;
		
	case CSI:	/* control sequence introducer */
		sc->escape = CSI;
		sc->ap = sc->argbuf;
		break;
		
	case ST:	/* string terminator */
		/* ignore, if not used as terminator */
		break;
		
	case OSC:	/* introduces OS command. Ignore everything upto ST */
		sc->escape = OSC;
		break;

	case PM:	/* privacy message, ignore everything upto ST */
		sc->escape = PM;
		break;
		
	case APC:	/* application program command, ignore everything upto ST */
		sc->escape = APC;
		break;

	default:
		if (c < ' ' || c == DEL)
			break;
		if (sc->imode)
			ite_inchar(sc, 1);
		iteprecheckwrap(sc);
#ifdef DO_WEIRD_ATTRIBUTES
		if ((sc->attribute & ATTR_INV) || attrtest(sc, ATTR_INV)) {
			attrset(sc, ATTR_INV);
			SUBR_PUTC(sc, c, sc->cury, sc->curx, ATTR_INV);
		}			
		else
			SUBR_PUTC(sc, c, sc->cury, sc->curx, ATTR_NOR);
#else
		SUBR_PUTC(sc, c, sc->cury, sc->curx, sc->attribute);
#endif
		SUBR_CURSOR(sc, DRAW_CURSOR);
		itecheckwrap(sc);
		break;
	}
}

static void
iteprecheckwrap(struct ite_softc *sc)
{
	if (sc->auto_wrap && sc->curx == sc->cols) {
		sc->curx = 0;
		clr_attr(sc, ATTR_INV);
		if (++sc->cury >= sc->bottom_margin + 1) {
			sc->cury = sc->bottom_margin;
			SUBR_CURSOR(sc, MOVE_CURSOR);
			SUBR_SCROLL(sc, sc->top_margin + 1, 0, 1, SCROLL_UP);
			ite_clrtoeol(sc);
		} else
			SUBR_CURSOR(sc, MOVE_CURSOR);
	}
}

static void
itecheckwrap(struct ite_softc *sc)
{
	if (sc->curx < sc->cols) {
		sc->curx++;
		SUBR_CURSOR(sc, MOVE_CURSOR);
	}
}
