/* $NetBSD: wsdisplay.c,v 1.76 2003/09/21 18:47:59 manu Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsdisplay.c,v 1.76 2003/09/21 18:47:59 manu Exp $");

#include "opt_wsdisplay_compat.h"
#include "opt_compat_netbsd.h"
#include "wskbd.h"
#include "wsmux.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#include <dev/wscons/wseventvar.h>
#include <dev/wscons/wsmuxvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsemulvar.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/cons.h>

struct wsscreen_internal {
	const struct wsdisplay_emulops *emulops;
	void	*emulcookie;

	const struct wsscreen_descr *scrdata;

	const struct wsemul_ops *wsemul;
	void	*wsemulcookie;
};

struct wsscreen {
	struct wsscreen_internal *scr_dconf;

	struct tty *scr_tty;
	int	scr_hold_screen;		/* hold tty output */

	int scr_flags;
#define SCR_OPEN 1		/* is it open? */
#define SCR_WAITACTIVE 2	/* someone waiting on activation */
#define SCR_GRAPHICS 4		/* graphics mode, no text (emulation) output */
	const struct wscons_syncops *scr_syncops;
	void *scr_synccookie;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	int scr_rawkbd;
#endif

	struct wsdisplay_softc *sc;
};

struct wsscreen *wsscreen_attach(struct wsdisplay_softc *, int,
				 const char *,
				 const struct wsscreen_descr *, void *,
				 int, int, long);
void wsscreen_detach(struct wsscreen *);
int wsdisplay_addscreen(struct wsdisplay_softc *, int, const char *, const char *);
static void wsdisplay_shutdownhook(void *);
static void wsdisplay_addscreen_print(struct wsdisplay_softc *, int, int);
static void wsdisplay_closescreen(struct wsdisplay_softc *, struct wsscreen *);
int wsdisplay_delscreen(struct wsdisplay_softc *, int, int);

#define WSDISPLAY_MAXSCREEN 8

struct wsdisplay_softc {
	struct device sc_dv;

	const struct wsdisplay_accessops *sc_accessops;
	void	*sc_accesscookie;

	const struct wsscreen_list *sc_scrdata;

	struct wsscreen *sc_scr[WSDISPLAY_MAXSCREEN];
	int sc_focusidx;	/* available only if sc_focus isn't null */
	struct wsscreen *sc_focus;

	struct wseventvar evar;

	int	sc_isconsole;

	int sc_flags;
#define SC_SWITCHPENDING 1
	int sc_screenwanted, sc_oldscreen; /* valid with SC_SWITCHPENDING */

#if NWSKBD > 0
	struct wsevsrc *sc_input;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int sc_rawkbd;
#endif
#endif /* NWSKBD > 0 */
};

extern struct cfdriver wsdisplay_cd;

/* Autoconfiguration definitions. */
static int wsdisplay_emul_match(struct device *, struct cfdata *, void *);
static void wsdisplay_emul_attach(struct device *, struct device *, void *);
static int wsdisplay_noemul_match(struct device *, struct cfdata *, void *);
static void wsdisplay_noemul_attach(struct device *, struct device *, void *);

CFATTACH_DECL(wsdisplay_emul, sizeof (struct wsdisplay_softc),
    wsdisplay_emul_match, wsdisplay_emul_attach, NULL, NULL);

CFATTACH_DECL(wsdisplay_noemul, sizeof (struct wsdisplay_softc),
    wsdisplay_noemul_match, wsdisplay_noemul_attach, NULL, NULL);
 
dev_type_open(wsdisplayopen);
dev_type_close(wsdisplayclose);
dev_type_read(wsdisplayread);
dev_type_write(wsdisplaywrite);
dev_type_ioctl(wsdisplayioctl);
dev_type_stop(wsdisplaystop);
dev_type_tty(wsdisplaytty);
dev_type_poll(wsdisplaypoll);
dev_type_mmap(wsdisplaymmap);
dev_type_kqfilter(wsdisplaykqfilter);

const struct cdevsw wsdisplay_cdevsw = {
	wsdisplayopen, wsdisplayclose, wsdisplayread, wsdisplaywrite,
	wsdisplayioctl, wsdisplaystop, wsdisplaytty, wsdisplaypoll,
	wsdisplaymmap, wsdisplaykqfilter, D_TTY
};

static void wsdisplaystart(struct tty *);
static int wsdisplayparam(struct tty *, struct termios *);


/* Internal macros, functions, and variables. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

#define	WSDISPLAYUNIT(dev)	(minor(dev) >> 8)
#define	WSDISPLAYSCREEN(dev)	(minor(dev) & 0xff)
#define ISWSDISPLAYSTAT(dev)	(WSDISPLAYSCREEN(dev) == 254)
#define ISWSDISPLAYCTL(dev)	(WSDISPLAYSCREEN(dev) == 255)
#define WSDISPLAYMINOR(unit, screen)	(((unit) << 8) | (screen))

#define	WSSCREEN_HAS_EMULATOR(scr)	((scr)->scr_dconf->wsemul != NULL)
#define	WSSCREEN_HAS_TTY(scr)	((scr)->scr_tty != NULL)

static void wsdisplay_common_attach(struct wsdisplay_softc *sc,
	    int console, int kbdmux, const struct wsscreen_list *,
	    const struct wsdisplay_accessops *accessops,
	    void *accesscookie);

#ifdef WSDISPLAY_COMPAT_RAWKBD
int wsdisplay_update_rawkbd(struct wsdisplay_softc *,
				 struct wsscreen *);
#endif

static int wsdisplay_console_initted;
static struct wsdisplay_softc *wsdisplay_console_device;
static struct wsscreen_internal wsdisplay_console_conf;

static int wsdisplay_getc_dummy(dev_t);
static void wsdisplay_pollc(dev_t, int);

static int wsdisplay_cons_pollmode;
static void (*wsdisplay_cons_kbd_pollc)(dev_t, int);

static struct consdev wsdisplay_cons = {
	NULL, NULL, wsdisplay_getc_dummy, wsdisplay_cnputc,
	wsdisplay_pollc, NULL, NULL, NULL, NODEV, CN_NORMAL
};

#ifndef WSDISPLAY_DEFAULTSCREENS
# define WSDISPLAY_DEFAULTSCREENS	0
#endif
int wsdisplay_defaultscreens = WSDISPLAY_DEFAULTSCREENS;

int wsdisplay_switch1(void *, int, int);
int wsdisplay_switch2(void *, int, int);
int wsdisplay_switch3(void *, int, int);

int wsdisplay_clearonclose;

struct wsscreen *
wsscreen_attach(struct wsdisplay_softc *sc, int console, const char *emul,
	const struct wsscreen_descr *type, void *cookie, int ccol,
	int crow, long defattr)
{
	struct wsscreen_internal *dconf;
	struct wsscreen *scr;

	scr = malloc(sizeof(struct wsscreen), M_DEVBUF, M_WAITOK);
	if (!scr)
		return (NULL);

	if (console) {
		dconf = &wsdisplay_console_conf;
		/*
		 * If there's an emulation, tell it about the callback argument.
		 * The other stuff is already there.
		 */
		if (dconf->wsemul != NULL)
			(*dconf->wsemul->attach)(1, 0, 0, 0, 0, scr, 0);
	} else { /* not console */
		dconf = malloc(sizeof(struct wsscreen_internal),
			       M_DEVBUF, M_NOWAIT);
		dconf->emulops = type->textops;
		dconf->emulcookie = cookie;
		if (dconf->emulops) {
			dconf->wsemul = wsemul_pick(emul);
			if (dconf->wsemul == NULL) {
				free(dconf, M_DEVBUF);
				free(scr, M_DEVBUF);
				return (NULL);
			}
			dconf->wsemulcookie =
			  (*dconf->wsemul->attach)(0, type, cookie,
						   ccol, crow, scr, defattr);
		} else
			dconf->wsemul = NULL;
		dconf->scrdata = type;
	}

	scr->scr_dconf = dconf;

	scr->scr_tty = ttymalloc();
	tty_attach(scr->scr_tty);
	scr->scr_hold_screen = 0;
	if (WSSCREEN_HAS_EMULATOR(scr))
		scr->scr_flags = 0;
	else
		scr->scr_flags = SCR_GRAPHICS;

	scr->scr_syncops = 0;
	scr->sc = sc;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	scr->scr_rawkbd = 0;
#endif
	return (scr);
}

void
wsscreen_detach(struct wsscreen *scr)
{
	u_int ccol, crow; /* XXX */

	if (WSSCREEN_HAS_TTY(scr)) {
		tty_detach(scr->scr_tty);
		ttyfree(scr->scr_tty);
	}
	if (WSSCREEN_HAS_EMULATOR(scr))
		(*scr->scr_dconf->wsemul->detach)(scr->scr_dconf->wsemulcookie,
						  &ccol, &crow);
	free(scr->scr_dconf, M_DEVBUF);
	free(scr, M_DEVBUF);
}

const struct wsscreen_descr *
wsdisplay_screentype_pick(const struct wsscreen_list *scrdata, const char *name)
{
	int i;
	const struct wsscreen_descr *scr;

	KASSERT(scrdata->nscreens > 0);

	if (name == NULL)
		return (scrdata->screens[0]);

	for (i = 0; i < scrdata->nscreens; i++) {
		scr = scrdata->screens[i];
		if (!strcmp(name, scr->name))
			return (scr);
	}

	return (0);
}

/*
 * print info about attached screen
 */
static void
wsdisplay_addscreen_print(struct wsdisplay_softc *sc, int idx, int count)
{
	printf("%s: screen %d", sc->sc_dv.dv_xname, idx);
	if (count > 1)
		printf("-%d", idx + (count-1));
	printf(" added (%s", sc->sc_scr[idx]->scr_dconf->scrdata->name);
	if (WSSCREEN_HAS_EMULATOR(sc->sc_scr[idx])) {
		printf(", %s emulation",
			sc->sc_scr[idx]->scr_dconf->wsemul->name);
	}
	printf(")\n");
}

int
wsdisplay_addscreen(struct wsdisplay_softc *sc, int idx,
	const char *screentype, const char *emul)
{
	const struct wsscreen_descr *scrdesc;
	int error;
	void *cookie;
	int ccol, crow;
	long defattr;
	struct wsscreen *scr;
	int s;

	if (idx < 0 || idx >= WSDISPLAY_MAXSCREEN)
		return (EINVAL);
	if (sc->sc_scr[idx] != NULL)
		return (EBUSY);

	scrdesc = wsdisplay_screentype_pick(sc->sc_scrdata, screentype);
	if (!scrdesc)
		return (ENXIO);
	error = (*sc->sc_accessops->alloc_screen)(sc->sc_accesscookie,
			scrdesc, &cookie, &ccol, &crow, &defattr);
	if (error)
		return (error);

	scr = wsscreen_attach(sc, 0, emul, scrdesc,
			      cookie, ccol, crow, defattr);
	if (scr == NULL) {
		(*sc->sc_accessops->free_screen)(sc->sc_accesscookie,
						 cookie);
		return (ENXIO);
	}

	sc->sc_scr[idx] = scr;

	/* if no screen has focus yet, activate the first we get */
	s = spltty();
	if (!sc->sc_focus) {
		(*sc->sc_accessops->show_screen)(sc->sc_accesscookie,
						 scr->scr_dconf->emulcookie,
						 0, 0, 0);
		sc->sc_focusidx = idx;
		sc->sc_focus = scr;
	}
	splx(s);
	return (0);
}

static void
wsdisplay_closescreen(struct wsdisplay_softc *sc, struct wsscreen *scr)
{
	int maj, mn, idx;

	/* hangup */
	if (WSSCREEN_HAS_TTY(scr)) {
		struct tty *tp = scr->scr_tty;
		(*tp->t_linesw->l_modem)(tp, 0);
	}

	/* locate the major number */
	maj = cdevsw_lookup_major(&wsdisplay_cdevsw);
	/* locate the screen index */
	for (idx = 0; idx < WSDISPLAY_MAXSCREEN; idx++)
		if (scr == sc->sc_scr[idx])
			break;
#ifdef DIAGNOSTIC
	if (idx == WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_forceclose: bad screen");
#endif

	/* nuke the vnodes */
	mn = WSDISPLAYMINOR(sc->sc_dv.dv_unit, idx);
	vdevgone(maj, mn, mn, VCHR);
}

int
wsdisplay_delscreen(struct wsdisplay_softc *sc, int idx, int flags)
{
	struct wsscreen *scr;
	int s;
	void *cookie;

	if (idx < 0 || idx >= WSDISPLAY_MAXSCREEN)
		return (EINVAL);
	if ((scr = sc->sc_scr[idx]) == NULL)
		return (ENXIO);

	if (scr->scr_dconf == &wsdisplay_console_conf ||
	    scr->scr_syncops ||
	    ((scr->scr_flags & SCR_OPEN) && !(flags & WSDISPLAY_DELSCR_FORCE)))
		return(EBUSY);

	wsdisplay_closescreen(sc, scr);

	/*
	 * delete pointers, so neither device entries
	 * nor keyboard input can reference it anymore
	 */
	s = spltty();
	if (sc->sc_focus == scr) {
		sc->sc_focus = 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
		wsdisplay_update_rawkbd(sc, 0);
#endif
	}
	sc->sc_scr[idx] = 0;
	splx(s);

	/*
	 * Wake up processes waiting for the screen to
	 * be activated. Sleepers must check whether
	 * the screen still exists.
	 */
	if (scr->scr_flags & SCR_WAITACTIVE)
		wakeup(scr);

	/* save a reference to the graphics screen */
	cookie = scr->scr_dconf->emulcookie;

	wsscreen_detach(scr);

	(*sc->sc_accessops->free_screen)(sc->sc_accesscookie,
					 cookie);

	printf("%s: screen %d deleted\n", sc->sc_dv.dv_xname, idx);
	return (0);
}

/*
 * Autoconfiguration functions.
 */
int
wsdisplay_emul_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct wsemuldisplaydev_attach_args *ap = aux;

	if (match->wsemuldisplaydevcf_console !=
	    WSEMULDISPLAYDEVCF_CONSOLE_UNK) {
		/*
		 * If console-ness of device specified, either match
		 * exactly (at high priority), or fail.
		 */
		if (match->wsemuldisplaydevcf_console != 0 &&
		    ap->console != 0)
			return (10);
		else
			return (0);
	}

	/* If console-ness unspecified, it wins. */
	return (1);
}

void
wsdisplay_emul_attach(struct device *parent, struct device *self, void *aux)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)self;
	struct wsemuldisplaydev_attach_args *ap = aux;

	wsdisplay_common_attach(sc, ap->console,
	     sc->sc_dv.dv_cfdata->wsemuldisplaydevcf_kbdmux, ap->scrdata,
	     ap->accessops, ap->accesscookie);

	if (ap->console) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&wsdisplay_cdevsw);

		cn_tab->cn_dev = makedev(maj, WSDISPLAYMINOR(self->dv_unit, 0));
	}
}

/* Print function (for parent devices). */
int
wsemuldisplaydevprint(void *aux, const char *pnp)
{
#if 0 /* -Wunused */
	struct wsemuldisplaydev_attach_args *ap = aux;
#endif

	if (pnp)
		aprint_normal("wsdisplay at %s", pnp);
#if 0 /* don't bother; it's ugly */
	aprint_normal(" console %d", ap->console);
#endif

	return (UNCONF);
}

int
wsdisplay_noemul_match(struct device *parent, struct cfdata *match, void *aux)
{
#if 0 /* -Wunused */
	struct wsdisplaydev_attach_args *ap = aux;
#endif

	/* Always match. */
	return (1);
}

void
wsdisplay_noemul_attach(struct device *parent, struct device *self, void *aux)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)self;
	struct wsdisplaydev_attach_args *ap = aux;

	wsdisplay_common_attach(sc, 0,
	    sc->sc_dv.dv_cfdata->wsemuldisplaydevcf_kbdmux, NULL,
	    ap->accessops, ap->accesscookie);
}

/* Print function (for parent devices). */
int
wsdisplaydevprint(void *aux, const char *pnp)
{
#if 0 /* -Wunused */
	struct wsdisplaydev_attach_args *ap = aux;
#endif

	if (pnp)
		aprint_normal("wsdisplay at %s", pnp);

	return (UNCONF);
}

static void
wsdisplay_common_attach(struct wsdisplay_softc *sc, int console, int kbdmux,
	const struct wsscreen_list *scrdata,
	const struct wsdisplay_accessops *accessops,
	void *accesscookie)
{
	static int hookset;
	int i, start=0;
#if NWSKBD > 0
	struct wsevsrc *kme;
#if NWSMUX > 0
	struct wsmux_softc *mux;

	if (kbdmux >= 0)
		mux = wsmux_getmux(kbdmux);
	else
		mux = wsmux_create("dmux", sc->sc_dv.dv_unit);
	/* XXX panic()ing isn't nice, but attach cannot fail */
	if (mux == NULL)
		panic("wsdisplay_common_attach: no memory");
	sc->sc_input = &mux->sc_base;
	mux->sc_base.me_dispdv = &sc->sc_dv;
	printf(" kbdmux %d", kbdmux);
#else
	if (kbdmux >= 0)
		printf(" (kbdmux ignored)");
#endif
#endif

	sc->sc_isconsole = console;

	if (console) {
		KASSERT(wsdisplay_console_initted);
		KASSERT(wsdisplay_console_device == NULL);

		sc->sc_scr[0] = wsscreen_attach(sc, 1, 0, 0, 0, 0, 0, 0);
		wsdisplay_console_device = sc;

		printf(": console (%s, %s emulation)",
		       wsdisplay_console_conf.scrdata->name,
		       wsdisplay_console_conf.wsemul->name);

#if NWSKBD > 0
		kme = wskbd_set_console_display(&sc->sc_dv, sc->sc_input);
		if (kme != NULL)
			printf(", using %s", kme->me_dv.dv_xname);
#if NWSMUX == 0
		sc->sc_input = kme;
#endif
#endif

		sc->sc_focusidx = 0;
		sc->sc_focus = sc->sc_scr[0];
		start = 1;
	}
	printf("\n");

#if NWSKBD > 0 && NWSMUX > 0
	wsmux_set_display(mux, &sc->sc_dv);
#endif

	sc->sc_accessops = accessops;
	sc->sc_accesscookie = accesscookie;
	sc->sc_scrdata = scrdata;

	/*
	 * Set up a number of virtual screens if wanted. The
	 * WSDISPLAYIO_ADDSCREEN ioctl is more flexible, so this code
	 * is for special cases like installation kernels.
	 */
	for (i = start; i < wsdisplay_defaultscreens; i++) {
		if (wsdisplay_addscreen(sc, i, 0, 0))
			break;
	}

	if (i > start) 
		wsdisplay_addscreen_print(sc, start, i-start);
	
	if (hookset == 0)
		shutdownhook_establish(wsdisplay_shutdownhook, NULL);
	hookset = 1;
}

void
wsdisplay_cnattach(const struct wsscreen_descr *type, void *cookie,
	int ccol, int crow, long defattr)
{
	const struct wsemul_ops *wsemul;

	KASSERT(!wsdisplay_console_initted);
	KASSERT(type->nrows > 0);
	KASSERT(type->ncols > 0);
	KASSERT(crow < type->nrows);
	KASSERT(ccol < type->ncols);

	wsdisplay_console_conf.emulops = type->textops;
	wsdisplay_console_conf.emulcookie = cookie;
	wsdisplay_console_conf.scrdata = type;

	wsemul = wsemul_pick(0); /* default */
	wsdisplay_console_conf.wsemul = wsemul;
	wsdisplay_console_conf.wsemulcookie = (*wsemul->cnattach)(type, cookie,
								  ccol, crow,
								  defattr);

	cn_tab = &wsdisplay_cons;
	wsdisplay_console_initted = 1;
}

/*
 * Tty and cdevsw functions.
 */
int
wsdisplayopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int newopen, error;
	struct wsscreen *scr;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));
	if (sc == NULL)			/* make sure it was attached */
		return (ENXIO);

	if (ISWSDISPLAYSTAT(dev)) {
		wsevent_init(&sc->evar);
		sc->evar.io = p;
		return (0);
	}

	if (ISWSDISPLAYCTL(dev))
		return (0);

	if (WSDISPLAYSCREEN(dev) >= WSDISPLAY_MAXSCREEN)
		return (ENXIO);
	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (ENXIO);

	if (WSSCREEN_HAS_TTY(scr)) {
		tp = scr->scr_tty;
		tp->t_oproc = wsdisplaystart;
		tp->t_param = wsdisplayparam;
		tp->t_dev = dev;
		newopen = (tp->t_state & TS_ISOPEN) == 0;
		if (newopen) {
			ttychars(tp);
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
			wsdisplayparam(tp, &tp->t_termios);
			ttsetwater(tp);
		} else if ((tp->t_state & TS_XCLUDE) != 0 &&
			   p->p_ucred->cr_uid != 0)
			return EBUSY;
		tp->t_state |= TS_CARR_ON;

		error = ((*tp->t_linesw->l_open)(dev, tp));
		if (error)
			return (error);

		if (newopen && WSSCREEN_HAS_EMULATOR(scr)) {
			/* set window sizes as appropriate, and reset
			 the emulation */
			tp->t_winsize.ws_row = scr->scr_dconf->scrdata->nrows;
			tp->t_winsize.ws_col = scr->scr_dconf->scrdata->ncols;

			/* wsdisplay_set_emulation() */
		}
	}

	scr->scr_flags |= SCR_OPEN;
	return (0);
}

int
wsdisplayclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct wsdisplay_softc *sc;
	struct tty *tp;
	struct wsscreen *scr;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));

	if (ISWSDISPLAYSTAT(dev)) {
		wsevent_fini(&sc->evar);
		return (0);
	}

	if (ISWSDISPLAYCTL(dev))
		return (0);

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (0);

	if (WSSCREEN_HAS_TTY(scr)) {
		if (scr->scr_hold_screen) {
			int s;

			/* XXX RESET KEYBOARD LEDS, etc. */
			s = spltty();	/* avoid conflict with keyboard */
			wsdisplay_kbdholdscreen((struct device *)sc, 0);
			splx(s);
		}
		tp = scr->scr_tty;
		(*tp->t_linesw->l_close)(tp, flag);
		ttyclose(tp);
	}

	if (scr->scr_syncops)
		(*scr->scr_syncops->destroy)(scr->scr_synccookie);

	if (WSSCREEN_HAS_EMULATOR(scr)) {
		scr->scr_flags &= ~SCR_GRAPHICS;
		(*scr->scr_dconf->wsemul->reset)(scr->scr_dconf->wsemulcookie,
						 WSEMUL_RESET);
		if (wsdisplay_clearonclose)
			(*scr->scr_dconf->wsemul->reset)
				(scr->scr_dconf->wsemulcookie,
				 WSEMUL_CLEARSCREEN);
	}

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (scr->scr_rawkbd) {
		int kbmode = WSKBD_TRANSLATED;
		(void)wsdisplay_internal_ioctl(sc, scr, WSKBDIO_SETMODE,
					       (caddr_t)&kbmode, 0, p);
	}
#endif

	scr->scr_flags &= ~SCR_OPEN;

	return (0);
}

int
wsdisplayread(dev_t dev, struct uio *uio, int flag)
{
	struct wsdisplay_softc *sc;
	struct tty *tp;
	struct wsscreen *scr;
	int error;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));

	if (ISWSDISPLAYSTAT(dev)) {
		error = wsevent_read(&sc->evar, uio, flag);
		return (error);
	}

	if (ISWSDISPLAYCTL(dev))
		return (0);

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (ENXIO);

	if (!WSSCREEN_HAS_TTY(scr))
		return (ENODEV);

	tp = scr->scr_tty;
	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
wsdisplaywrite(dev_t dev, struct uio *uio, int flag)
{
	struct wsdisplay_softc *sc;
	struct tty *tp;
	struct wsscreen *scr;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));

	if (ISWSDISPLAYSTAT(dev)) {
		return (0);
	}

	if (ISWSDISPLAYCTL(dev))
		return (0);

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (ENXIO);

	if (!WSSCREEN_HAS_TTY(scr))
		return (ENODEV);

	tp = scr->scr_tty;
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
wsdisplaypoll(dev_t dev, int events, struct proc *p)
{
	struct wsdisplay_softc *sc;
	struct tty *tp;
	struct wsscreen *scr;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));

	if (ISWSDISPLAYSTAT(dev))
		return (wsevent_poll(&sc->evar, events, p));

	if (ISWSDISPLAYCTL(dev))
		return (0);

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (ENXIO);

	if (!WSSCREEN_HAS_TTY(scr))
		return (ENODEV);

	tp = scr->scr_tty;
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}

int
wsdisplaykqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	struct wsdisplay_softc *sc =
	    device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));
	struct wsscreen *scr;

	if (ISWSDISPLAYCTL(dev))
		return (1);

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (1);
	

	if (WSSCREEN_HAS_TTY(scr))
		return (ttykqfilter(dev, kn));
	else
		return (1);
}

struct tty *
wsdisplaytty(dev_t dev)
{
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));

	if (ISWSDISPLAYSTAT(dev))
		panic("wsdisplaytty() on status device");

	if (ISWSDISPLAYCTL(dev))
		panic("wsdisplaytty() on ctl device");

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return NULL;

	return (scr->scr_tty);
}

int
wsdisplayioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int error;
	struct wsscreen *scr;

	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));

#ifdef WSDISPLAY_COMPAT_USL
	error = wsdisplay_usl_ioctl1(sc, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);
#endif

	if (ISWSDISPLAYSTAT(dev))
		return (wsdisplay_stat_ioctl(sc, cmd, data, flag, p));

	if (ISWSDISPLAYCTL(dev))
		return (wsdisplay_cfg_ioctl(sc, cmd, data, flag, p));

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (ENXIO);

	if (WSSCREEN_HAS_TTY(scr)) {
		tp = scr->scr_tty;

/* printf("disc\n"); */
		/* do the line discipline ioctls first */
		error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
		if (error != EPASSTHROUGH)
			return (error);

/* printf("tty\n"); */
		/* then the tty ioctls */
		error = ttioctl(tp, cmd, data, flag, p);
		if (error != EPASSTHROUGH)
			return (error);
	}

#ifdef WSDISPLAY_COMPAT_USL
	error = wsdisplay_usl_ioctl2(sc, scr, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);
#endif

	return (wsdisplay_internal_ioctl(sc, scr, cmd, data, flag, p));
}

int
wsdisplay_param(struct device *dev, u_long cmd, struct wsdisplay_param *dp)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	return ((*sc->sc_accessops->ioctl)(sc->sc_accesscookie, cmd,
					   (caddr_t)dp, 0, NULL));
}

int
wsdisplay_internal_ioctl(struct wsdisplay_softc *sc, struct wsscreen *scr,
	u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int error;
	char namebuf[16];
	struct wsdisplay_font fd;

#if NWSKBD > 0
	struct wsevsrc *inp;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	switch (cmd) {
	case WSKBDIO_SETMODE:
		scr->scr_rawkbd = (*(int *)data == WSKBD_RAW);
		return (wsdisplay_update_rawkbd(sc, scr));
	case WSKBDIO_GETMODE:
		*(int *)data = (scr->scr_rawkbd ?
				WSKBD_RAW : WSKBD_TRANSLATED);
		return (0);
	}
#endif
	inp = sc->sc_input;
	if (inp == NULL)
		return (ENXIO);
	error = wsevsrc_display_ioctl(inp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return (error);
#endif /* NWSKBD > 0 */

	switch (cmd) {
	case WSDISPLAYIO_GMODE:
		*(u_int *)data = (scr->scr_flags & SCR_GRAPHICS ?
				  WSDISPLAYIO_MODE_MAPPED :
				  WSDISPLAYIO_MODE_EMUL);
		return (0);

	case WSDISPLAYIO_SMODE:
#define d (*(int *)data)
		if (d != WSDISPLAYIO_MODE_EMUL &&
		    d != WSDISPLAYIO_MODE_MAPPED)
			return (EINVAL);

	    if (WSSCREEN_HAS_EMULATOR(scr)) {
		    scr->scr_flags &= ~SCR_GRAPHICS;
		    if (d == WSDISPLAYIO_MODE_MAPPED)
			    scr->scr_flags |= SCR_GRAPHICS;
	    } else if (d == WSDISPLAYIO_MODE_EMUL)
		    return (EINVAL);

	    (void)(*sc->sc_accessops->ioctl)(sc->sc_accesscookie, cmd, data,
		    flag, p);

	    return (0);
#undef d

	case WSDISPLAYIO_SFONT:
#define d ((struct wsdisplay_usefontdata *)data)
		if (!sc->sc_accessops->load_font)
			return (EINVAL);
		if (d->name) {
			error = copyinstr(d->name, namebuf, sizeof(namebuf), 0);
			if (error)
				return (error);
			fd.name = namebuf;
		} else
			fd.name = 0;
		fd.data = 0;
		error = (*sc->sc_accessops->load_font)(sc->sc_accesscookie,
					scr->scr_dconf->emulcookie, &fd);
		if (!error && WSSCREEN_HAS_EMULATOR(scr))
			(*scr->scr_dconf->wsemul->reset)
				(scr->scr_dconf->wsemulcookie, WSEMUL_SYNCFONT);
		return (error);
#undef d

#if defined(WSDISPLAY_CHARFUNCS)
	case WSDISPLAYIO_GETWSCHAR:
#define d ((struct wsdisplay_char *)data)
		if (!sc->sc_accessops->getwschar)
			return (EINVAL);
		return ((*sc->sc_accessops->getwschar)
			(scr->scr_dconf->emulcookie, d));
#undef d

	case WSDISPLAYIO_PUTWSCHAR:
#define d ((struct wsdisplay_char *)data)
		if (!sc->sc_accessops->putwschar)
			return (EINVAL);
		return ((*sc->sc_accessops->putwschar)
			(scr->scr_dconf->emulcookie, d));
#undef d
		return 1;
#endif /* WSDISPLAY_CHARFUNCS */

	}

	/* check ioctls for display */
	return ((*sc->sc_accessops->ioctl)(sc->sc_accesscookie, cmd, data,
	    flag, p));
}

int
wsdisplay_stat_ioctl(struct wsdisplay_softc *sc, u_long cmd, caddr_t data,
	int flag, struct proc *p)
{
	switch (cmd) {
	case WSDISPLAYIO_GETACTIVESCREEN:
		*(int*)data = wsdisplay_getactivescreen(sc);
		return (0);
	}

	return (EPASSTHROUGH);
}

int
wsdisplay_cfg_ioctl(struct wsdisplay_softc *sc, u_long cmd, caddr_t data,
	int flag, struct proc *p)
{
	int error;
	char *type, typebuf[16], *emul, emulbuf[16];
	void *buf;
	u_int fontsz;
#if defined(COMPAT_14) && NWSKBD > 0
	struct wsmux_device wsmuxdata;
#endif
#if NWSKBD > 0
	struct wsevsrc *inp;
#endif

	switch (cmd) {
	case WSDISPLAYIO_ADDSCREEN:
#define d ((struct wsdisplay_addscreendata *)data)
		if (d->screentype) {
			error = copyinstr(d->screentype, typebuf,
					  sizeof(typebuf), 0);
			if (error)
				return (error);
			type = typebuf;
		} else
			type = 0;
		if (d->emul) {
			error = copyinstr(d->emul, emulbuf, sizeof(emulbuf),0);
			if (error)
				return (error);
			emul = emulbuf;
		} else
			emul = 0;

		if ((error = wsdisplay_addscreen(sc, d->idx, type, emul)) == 0)
			wsdisplay_addscreen_print(sc, d->idx, 0);
		return (error);
#undef d
	case WSDISPLAYIO_DELSCREEN:
#define d ((struct wsdisplay_delscreendata *)data)
		return (wsdisplay_delscreen(sc, d->idx, d->flags));
#undef d
	case WSDISPLAYIO_LDFONT:
#define d ((struct wsdisplay_font *)data)
		if (!sc->sc_accessops->load_font)
			return (EINVAL);
		if (d->name) {
			error = copyinstr(d->name, typebuf, sizeof(typebuf), 0);
			if (error)
				return (error);
			d->name = typebuf;
		} else
			d->name = "loaded"; /* ??? */
		fontsz = d->fontheight * d->stride * d->numchars;
		if (fontsz > WSDISPLAY_MAXFONTSZ)
			return (EINVAL);

		buf = malloc(fontsz, M_DEVBUF, M_WAITOK);
		error = copyin(d->data, buf, fontsz);
		if (error) {
			free(buf, M_DEVBUF);
			return (error);
		}
		d->data = buf;
		error =
		  (*sc->sc_accessops->load_font)(sc->sc_accesscookie, 0, d);
		free(buf, M_DEVBUF);
#undef d
		return (error);

#if NWSKBD > 0
#ifdef COMPAT_14
	case _O_WSDISPLAYIO_SETKEYBOARD:
#define d ((struct wsdisplay_kbddata *)data)
		inp = sc->sc_input;
		if (inp == NULL)
			return (ENXIO);
		switch (d->op) {
		case _O_WSDISPLAY_KBD_ADD:
			if (d->idx == -1) {
				d->idx = wskbd_pickfree();
				if (d->idx == -1)
					return (ENXIO);
			}
			wsmuxdata.type = WSMUX_KBD;
			wsmuxdata.idx = d->idx;
			return (wsevsrc_ioctl(inp, WSMUX_ADD_DEVICE,
					      &wsmuxdata, flag, p));
		case _O_WSDISPLAY_KBD_DEL:
			wsmuxdata.type = WSMUX_KBD;
			wsmuxdata.idx = d->idx;
			return (wsevsrc_ioctl(inp, WSMUX_REMOVE_DEVICE,
					      &wsmuxdata, flag, p));
		default:
			return (EINVAL);
		}
#undef d
#endif

	case WSMUXIO_ADD_DEVICE:
#define d ((struct wsmux_device *)data)
		if (d->idx == -1 && d->type == WSMUX_KBD)
			d->idx = wskbd_pickfree();
#undef d
		/* fall into */
	case WSMUXIO_INJECTEVENT:
	case WSMUXIO_REMOVE_DEVICE:
	case WSMUXIO_LIST_DEVICES:
		inp = sc->sc_input;
		if (inp == NULL)
			return (ENXIO);
		return (wsevsrc_ioctl(inp, cmd, data, flag, p));
#endif /* NWSKBD > 0 */

	}
	return (EPASSTHROUGH);
}

int
wsdisplay_stat_inject(struct device *dev, u_int type, int value)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *) dev;
	struct wseventvar *evar;
	struct wscons_event *ev;
	struct timeval thistime;
	int put;

	evar = &sc->evar;

	if (evar == NULL)
		return (0);

	if (evar->q == NULL)
		return (1);

	put = evar->put;
	ev = &evar->q[put];
	put = (put + 1) % WSEVENT_QSIZE;
	if (put == evar->get) {
		log(LOG_WARNING, "wsdisplay: event queue overflow\n");
		return (1);
	}
	ev->type = type;
	ev->value = value;
	microtime(&thistime);
	TIMEVAL_TO_TIMESPEC(&thistime, &ev->time);
	evar->put = put;
	WSEVENT_WAKEUP(evar);

	return (0);
}

paddr_t
wsdisplaymmap(dev_t dev, off_t offset, int prot)
{
	struct wsdisplay_softc *sc =
	    device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(dev));
	struct wsscreen *scr;

	if (ISWSDISPLAYSTAT(dev))
		return (-1);

	if (ISWSDISPLAYCTL(dev))
		return (-1);

	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(dev)]) == NULL)
		return (-1);

	if (!(scr->scr_flags & SCR_GRAPHICS))
		return (-1);

	/* pass mmap to display */
	return ((*sc->sc_accessops->mmap)(sc->sc_accesscookie, offset, prot));
}

void
wsdisplaystart(struct tty *tp)
{
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;
	int s, n;
	u_char *buf;
		
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
		return;
	}
	sc = device_lookup(&wsdisplay_cd, WSDISPLAYUNIT(tp->t_dev));
	if ((scr = sc->sc_scr[WSDISPLAYSCREEN(tp->t_dev)]) == NULL) {
		splx(s);
		return; 
	}

	if (scr->scr_hold_screen) {
		tp->t_state |= TS_TIMEOUT;
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY; 
	splx(s);
	
	/*
	 * Drain output from ring buffer.
	 * The output will normally be in one contiguous chunk, but when the 
	 * ring wraps, it will be in two pieces.. one at the end of the ring, 
	 * the other at the start.  For performance, rather than loop here, 
	 * we output one chunk, see if there's another one, and if so, output 
	 * it too.
	 */

	n = ndqb(&tp->t_outq, 0);
	buf = tp->t_outq.c_cf;

	if (!(scr->scr_flags & SCR_GRAPHICS)) {
		KASSERT(WSSCREEN_HAS_EMULATOR(scr));
		(*scr->scr_dconf->wsemul->output)(scr->scr_dconf->wsemulcookie,
						  buf, n, 0);
	}
	ndflush(&tp->t_outq, n);

	if ((n = ndqb(&tp->t_outq, 0)) > 0) {
		buf = tp->t_outq.c_cf;

		if (!(scr->scr_flags & SCR_GRAPHICS)) {
			KASSERT(WSSCREEN_HAS_EMULATOR(scr));
			(*scr->scr_dconf->wsemul->output)
			    (scr->scr_dconf->wsemulcookie, buf, n, 0);
		}
		ndflush(&tp->t_outq, n);
	}

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	/* Come back if there's more to do */
	if (tp->t_outq.c_cc) {
		tp->t_state |= TS_TIMEOUT;
		callout_reset(&tp->t_rstrt_ch, (hz > 128) ? (hz / 128) : 1,
		    ttrstrt, tp);
	}
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	splx(s);
}

void
wsdisplaystop(struct tty *tp, int flag)
{
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY))
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	splx(s);
}

/* Set line parameters. */
int
wsdisplayparam(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

/*
 * Callbacks for the emulation code.
 */
void
wsdisplay_emulbell(void *v)
{
	struct wsscreen *scr = v;

	if (scr == NULL)		/* console, before real attach */
		return;

	if (scr->scr_flags & SCR_GRAPHICS) /* can this happen? */
		return;

	(void) wsdisplay_internal_ioctl(scr->sc, scr, WSKBDIO_BELL, NULL,
					FWRITE, NULL);
}

void
wsdisplay_emulinput(void *v, const u_char *data, u_int count)
{
	struct wsscreen *scr = v;
	struct tty *tp;

	if (v == NULL)			/* console, before real attach */
		return;

	if (scr->scr_flags & SCR_GRAPHICS) /* XXX can't happen */
		return;
	if (!WSSCREEN_HAS_TTY(scr))
		return;

	tp = scr->scr_tty;
	while (count-- > 0)
		(*tp->t_linesw->l_rint)(*data++, tp);
}

/*
 * Calls from the keyboard interface.
 */
void
wsdisplay_kbdinput(struct device *dev, keysym_t ks)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	struct wsscreen *scr;
	char *dp;
	int count;
	struct tty *tp;

	KASSERT(sc != NULL);

	scr = sc->sc_focus;

	if (!scr || !WSSCREEN_HAS_TTY(scr))
		return;

	tp = scr->scr_tty;

	if (KS_GROUP(ks) == KS_GROUP_Ascii)
		(*tp->t_linesw->l_rint)(KS_VALUE(ks), tp);
	else if (WSSCREEN_HAS_EMULATOR(scr)) {
		count = (*scr->scr_dconf->wsemul->translate)
		    (scr->scr_dconf->wsemulcookie, ks, &dp);
		while (count-- > 0)
			(*tp->t_linesw->l_rint)(*dp++, tp);
	}
}

#if defined(WSDISPLAY_COMPAT_RAWKBD)
int
wsdisplay_update_rawkbd(struct wsdisplay_softc *sc, struct wsscreen *scr)
{
#if NWSKBD > 0
	int s, raw, data, error;
	struct wsevsrc *inp;

	s = spltty();

	raw = (scr ? scr->scr_rawkbd : 0);

	if (scr != sc->sc_focus ||
	    sc->sc_rawkbd == raw) {
		splx(s);
		return (0);
	}

	data = raw ? WSKBD_RAW : WSKBD_TRANSLATED;
	inp = sc->sc_input;
	if (inp == NULL)
		return (ENXIO);
	error = wsevsrc_display_ioctl(inp, WSKBDIO_SETMODE, &data, 0, 0);
	if (!error)
		sc->sc_rawkbd = raw;
	splx(s);
	return (error);
#else
	return (0);
#endif
}
#endif

int
wsdisplay_switch3(void *arg, int error, int waitok)
{
	struct wsdisplay_softc *sc = arg;
	int no;
	struct wsscreen *scr;

	if (!(sc->sc_flags & SC_SWITCHPENDING)) {
		printf("wsdisplay_switch3: not switching\n");
		return (EINVAL);
	}

	no = sc->sc_screenwanted;
	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_switch3: invalid screen %d", no);
	scr = sc->sc_scr[no];
	if (!scr) {
		printf("wsdisplay_switch3: screen %d disappeared\n", no);
		error = ENXIO;
	}

	if (error) {
		/* try to recover, avoid recursion */

		if (sc->sc_oldscreen == WSDISPLAY_NULLSCREEN) {
			printf("wsdisplay_switch3: giving up\n");
			sc->sc_focus = 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
			wsdisplay_update_rawkbd(sc, 0);
#endif
			sc->sc_flags &= ~SC_SWITCHPENDING;
			return (error);
		}

		sc->sc_screenwanted = sc->sc_oldscreen;
		sc->sc_oldscreen = WSDISPLAY_NULLSCREEN;
		return (wsdisplay_switch1(arg, 0, waitok));
	}

	sc->sc_flags &= ~SC_SWITCHPENDING;

	if (!error && (scr->scr_flags & SCR_WAITACTIVE))
		wakeup(scr);
	return (error);
}

int
wsdisplay_switch2(void *arg, int error, int waitok)
{
	struct wsdisplay_softc *sc = arg;
	int no;
	struct wsscreen *scr;

	if (!(sc->sc_flags & SC_SWITCHPENDING)) {
		printf("wsdisplay_switch2: not switching\n");
		return (EINVAL);
	}

	no = sc->sc_screenwanted;
	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_switch2: invalid screen %d", no);
	scr = sc->sc_scr[no];
	if (!scr) {
		printf("wsdisplay_switch2: screen %d disappeared\n", no);
		error = ENXIO;
	}

	if (error) {
		/* try to recover, avoid recursion */

		if (sc->sc_oldscreen == WSDISPLAY_NULLSCREEN) {
			printf("wsdisplay_switch2: giving up\n");
			sc->sc_focus = 0;
			sc->sc_flags &= ~SC_SWITCHPENDING;
			return (error);
		}

		sc->sc_screenwanted = sc->sc_oldscreen;
		sc->sc_oldscreen = WSDISPLAY_NULLSCREEN;
		return (wsdisplay_switch1(arg, 0, waitok));
	}

	sc->sc_focusidx = no;
	sc->sc_focus = scr;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	(void) wsdisplay_update_rawkbd(sc, scr);
#endif
	/* keyboard map??? */

#define wsswitch_cb3 ((void (*)(void *, int, int))wsdisplay_switch3)
	if (scr->scr_syncops) {
		error = (*scr->scr_syncops->attach)(scr->scr_synccookie, waitok,
	  sc->sc_isconsole && wsdisplay_cons_pollmode ? 0 : wsswitch_cb3, sc);
		if (error == EAGAIN) {
			/* switch will be done asynchronously */
			return (0);
		}
	}

	return (wsdisplay_switch3(sc, error, waitok));
}

int
wsdisplay_switch1(void *arg, int error, int waitok)
{
	struct wsdisplay_softc *sc = arg;
	int no;
	struct wsscreen *scr;

	if (!(sc->sc_flags & SC_SWITCHPENDING)) {
		printf("wsdisplay_switch1: not switching\n");
		return (EINVAL);
	}

	no = sc->sc_screenwanted;
	if (no == WSDISPLAY_NULLSCREEN) {
		sc->sc_flags &= ~SC_SWITCHPENDING;
		if (!error) {
			sc->sc_focus = 0;
		}
		wakeup(sc);
		return (error);
	}
	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_switch1: invalid screen %d", no);
	scr = sc->sc_scr[no];
	if (!scr) {
		printf("wsdisplay_switch1: screen %d disappeared\n", no);
		error = ENXIO;
	}

	if (error) {
		sc->sc_flags &= ~SC_SWITCHPENDING;
		return (error);
	}

#define wsswitch_cb2 ((void (*)(void *, int, int))wsdisplay_switch2)
	error = (*sc->sc_accessops->show_screen)(sc->sc_accesscookie,
						 scr->scr_dconf->emulcookie,
						 waitok,
	  sc->sc_isconsole && wsdisplay_cons_pollmode ? 0 : wsswitch_cb2, sc);
	if (error == EAGAIN) {
		/* switch will be done asynchronously */
		return (0);
	}

	return (wsdisplay_switch2(sc, error, waitok));
}

int
wsdisplay_switch(struct device *dev, int no, int waitok)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	int s, res = 0;
	struct wsscreen *scr;

	if (no != WSDISPLAY_NULLSCREEN) {
		if ((no < 0 || no >= WSDISPLAY_MAXSCREEN))
			return (EINVAL);
		if (sc->sc_scr[no] == NULL)
			return (ENXIO);
	}

	wsdisplay_stat_inject(dev, WSCONS_EVENT_SCREEN_SWITCH, no);

	s = spltty();

	if ((sc->sc_focus && no == sc->sc_focusidx) ||
	    (sc->sc_focus == NULL && no == WSDISPLAY_NULLSCREEN)) {
		splx(s);
		return (0);
	}

	if (sc->sc_flags & SC_SWITCHPENDING) {
		splx(s);
		return (EBUSY);
	}

	sc->sc_flags |= SC_SWITCHPENDING;
	sc->sc_screenwanted = no;

	splx(s);

	scr = sc->sc_focus;
	if (!scr) {
		sc->sc_oldscreen = WSDISPLAY_NULLSCREEN;
		return (wsdisplay_switch1(sc, 0, waitok));
	} else
		sc->sc_oldscreen = sc->sc_focusidx;

#define wsswitch_cb1 ((void (*)(void *, int, int))wsdisplay_switch1)
	if (scr->scr_syncops) {
		res = (*scr->scr_syncops->detach)(scr->scr_synccookie, waitok,
	  sc->sc_isconsole && wsdisplay_cons_pollmode ? 0 : wsswitch_cb1, sc);
		if (res == EAGAIN) {
			/* switch will be done asynchronously */
			return (0);
		}
	} else if (scr->scr_flags & SCR_GRAPHICS) {
		/* no way to save state */
		res = EBUSY;
	}

	return (wsdisplay_switch1(sc, res, waitok));
}

void
wsdisplay_reset(struct device *dev, enum wsdisplay_resetops op)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	struct wsscreen *scr;

	KASSERT(sc != NULL);
	scr = sc->sc_focus;

	if (!scr)
		return;

	switch (op) {
	case WSDISPLAY_RESETEMUL:
		if (!WSSCREEN_HAS_EMULATOR(scr))
			break;
		(*scr->scr_dconf->wsemul->reset)(scr->scr_dconf->wsemulcookie,
						 WSEMUL_RESET);
		break;
	case WSDISPLAY_RESETCLOSE:
		wsdisplay_closescreen(sc, scr);
		break;
	}
}

/*
 * Interface for (external) VT switch / process synchronization code
 */
int
wsscreen_attach_sync(struct wsscreen *scr, const struct wscons_syncops *ops,
	void *cookie)
{
	if (scr->scr_syncops) {
		/*
		 * The screen is already claimed.
		 * Check if the owner is still alive.
		 */
		if ((*scr->scr_syncops->check)(scr->scr_synccookie))
			return (EBUSY);
	}
	scr->scr_syncops = ops;
	scr->scr_synccookie = cookie;
	return (0);
}

int
wsscreen_detach_sync(struct wsscreen *scr)
{
	if (!scr->scr_syncops)
		return (EINVAL);
	scr->scr_syncops = 0;
	return (0);
}

int
wsscreen_lookup_sync(struct wsscreen *scr,
	const struct wscons_syncops *ops, /* used as ID */
	void **cookiep)
{
	if (!scr->scr_syncops || ops != scr->scr_syncops)
		return (EINVAL);
	*cookiep = scr->scr_synccookie;
	return (0);
}

/*
 * Interface to virtual screen stuff
 */
int
wsdisplay_maxscreenidx(struct wsdisplay_softc *sc)
{
	return (WSDISPLAY_MAXSCREEN - 1);
}

int
wsdisplay_screenstate(struct wsdisplay_softc *sc, int idx)
{
	if (idx < 0 || idx >= WSDISPLAY_MAXSCREEN)
		return (EINVAL);
	if (!sc->sc_scr[idx])
		return (ENXIO);
	return ((sc->sc_scr[idx]->scr_flags & SCR_OPEN) ? EBUSY : 0);
}

int
wsdisplay_getactivescreen(struct wsdisplay_softc *sc)
{
	return (sc->sc_focus ? sc->sc_focusidx : WSDISPLAY_NULLSCREEN);
}

int
wsscreen_switchwait(struct wsdisplay_softc *sc, int no)
{
	struct wsscreen *scr;
	int s, res = 0;

	if (no == WSDISPLAY_NULLSCREEN) {
		s = spltty();
		while (sc->sc_focus && res == 0) {
			res = tsleep(sc, PCATCH, "wswait", 0);
		}
		splx(s);
		return (res);
	}

	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		return (ENXIO);
	scr = sc->sc_scr[no];
	if (!scr)
		return (ENXIO);

	s = spltty();
	if (scr != sc->sc_focus) {
		scr->scr_flags |= SCR_WAITACTIVE;
		res = tsleep(scr, PCATCH, "wswait", 0);
		if (scr != sc->sc_scr[no])
			res = ENXIO; /* disappeared in the meantime */
		else
			scr->scr_flags &= ~SCR_WAITACTIVE;
	}
	splx(s);
	return (res);
}

void
wsdisplay_kbdholdscreen(struct device *dev, int hold)
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	struct wsscreen *scr;

	scr = sc->sc_focus;

	if (hold)
		scr->scr_hold_screen = 1;
	else {
		scr->scr_hold_screen = 0;
		callout_reset(&scr->scr_tty->t_rstrt_ch, 0,
		    ttrstrt, scr->scr_tty);		/* "immediate" */
	}
}

#if NWSKBD > 0
void
wsdisplay_set_console_kbd(struct wsevsrc *src)
{
	if (wsdisplay_console_device == NULL) {
		src->me_dispdv = NULL;
		return;
	}
#if NWSMUX > 0
	if (wsmux_attach_sc((struct wsmux_softc *)
			    wsdisplay_console_device->sc_input, src)) {
		src->me_dispdv = NULL;
		return;
	}
#else
	wsdisplay_console_device->sc_input = src;
#endif
	src->me_dispdv = &wsdisplay_console_device->sc_dv;
}
#endif /* NWSKBD > 0 */

/*
 * Console interface.
 */
void
wsdisplay_cnputc(dev_t dev, int i)
{
	struct wsscreen_internal *dc;
	u_char c = i;

	if (!wsdisplay_console_initted)
		return;

	if ((wsdisplay_console_device != NULL) && 
	    (wsdisplay_console_device->sc_scr[0] != NULL) &&
	    (wsdisplay_console_device->sc_scr[0]->scr_flags & SCR_GRAPHICS))
		return;

	dc = &wsdisplay_console_conf;
	(*dc->wsemul->output)(dc->wsemulcookie, &c, 1, 1);
}

static int
wsdisplay_getc_dummy(dev_t dev)
{
	/* panic? */
	return (0);
}

static void
wsdisplay_pollc(dev_t dev, int on)
{

	wsdisplay_cons_pollmode = on;

	/* notify to fb drivers */
	if (wsdisplay_console_device != NULL &&
	    wsdisplay_console_device->sc_accessops->pollc != NULL)
		(*wsdisplay_console_device->sc_accessops->pollc)
			(wsdisplay_console_device->sc_accesscookie, on);

	/* notify to kbd drivers */
	if (wsdisplay_cons_kbd_pollc)
		(*wsdisplay_cons_kbd_pollc)(NODEV, on);
}

void
wsdisplay_set_cons_kbd(int (*get)(dev_t), void (*poll)(dev_t, int),
	void (*bell)(dev_t, u_int, u_int, u_int))
{
	wsdisplay_cons.cn_getc = get;
	wsdisplay_cons.cn_bell = bell;
	wsdisplay_cons_kbd_pollc = poll;
}

void
wsdisplay_unset_cons_kbd(void)
{
	wsdisplay_cons.cn_getc = wsdisplay_getc_dummy;
	wsdisplay_cons.cn_bell = NULL;
	wsdisplay_cons_kbd_pollc = 0;
}

/*
 * Switch the console display to it's first screen.
 */
void
wsdisplay_switchtoconsole(void)
{
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;

	if (wsdisplay_console_device != NULL) {
		sc = wsdisplay_console_device;
		if ((scr = sc->sc_scr[0]) == NULL)
			return;
		(*sc->sc_accessops->show_screen)(sc->sc_accesscookie,
						 scr->scr_dconf->emulcookie,
						 0, NULL, NULL);
	}
}

/*
 * Switch the console at shutdown.
 */
static void
wsdisplay_shutdownhook(void *arg)
{

	wsdisplay_switchtoconsole();
}
