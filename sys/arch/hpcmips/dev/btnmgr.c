/*	$NetBSD: btnmgr.c,v 1.9 2000/07/02 10:01:31 takemura Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
#define BTNMGRDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include "opt_wsdisplay_compat.h"
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#ifdef WSDISPLAY_COMPAT_RAWKBD
#include <hpcmips/dev/pckbd_encode.h>
#endif

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/config_hook.h>

#ifdef BTNMGRDEBUG
#ifndef BTNMGRDEBUG_CONF
#define BTNMGRDEBUG_CONF 0
#endif
int	btnmgr_debug = BTNMGRDEBUG_CONF;
#define	DPRINTF(arg) if (btnmgr_debug) printf arg;
#define	DPRINTFN(n, arg) if (btnmgr_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

cdev_decl(btnmgr);

struct btnmgr_softc {
	struct device sc_dev;
	config_hook_tag	sc_hook_tag;
	int sc_enabled;
	struct device *sc_wskbddev;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	int sc_rawkbd;
#endif
};

/*
static struct btnmgr_softc *the_btnmgr_sc;
*/

/*
void		btnmgrattach  __P((int));
 */
int btnmgrmatch __P((struct device *, struct cfdata *, void *));
void btnmgrattach __P((struct device *, struct device *, void *));
char*		btnmgr_name __P((long));
static int	btnmgr_hook __P((void *, int, long, void *));


/*
 * global/static data
 */
struct cfattach btnmgr_ca = {
	sizeof(struct btnmgr_softc), btnmgrmatch, btnmgrattach
};

/* wskbd accessopts */
int	btnmgr_wskbd_enable __P((void *, int));
void	btnmgr_wskbd_set_leds __P((void *, int));
int	btnmgr_wskbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

const struct wskbd_accessops btnmgr_wskbd_accessops = {
	btnmgr_wskbd_enable,
	btnmgr_wskbd_set_leds,
	btnmgr_wskbd_ioctl,
};

/* button config: index by buttun event id */
static const struct {
	int  kevent;
	int  keycode;
	char *name;
} button_config[] = {
	/* id					kevent keycode name	*/
	[CONFIG_HOOK_BUTTONEVENT_POWER] =	{ 0,   0, "Power"	},
	[CONFIG_HOOK_BUTTONEVENT_OK] =		{ 1,  28, "OK"		},
	[CONFIG_HOOK_BUTTONEVENT_CANCEL] =	{ 1,   1, "Cancel"	},
	[CONFIG_HOOK_BUTTONEVENT_UP] =		{ 1,  72, "Up"		},
	[CONFIG_HOOK_BUTTONEVENT_DOWN] =	{ 1,  80, "Down"	},
	[CONFIG_HOOK_BUTTONEVENT_REC] =		{ 0,   0, "Rec"		},
	[CONFIG_HOOK_BUTTONEVENT_COVER] =	{ 0,   0, "Cover"	},
	[CONFIG_HOOK_BUTTONEVENT_LIGHT] =	{ 0,   0, "Light"	},
	[CONFIG_HOOK_BUTTONEVENT_CONTRAST] =	{ 0,   0, "Contrast"	},
	[CONFIG_HOOK_BUTTONEVENT_APP0] =	{ 1,  67, "Application 0" },
	[CONFIG_HOOK_BUTTONEVENT_APP1] =	{ 1,  68, "Application 1" },
	[CONFIG_HOOK_BUTTONEVENT_APP2] =	{ 1,  87, "Application 2" },
	[CONFIG_HOOK_BUTTONEVENT_APP3] =	{ 1,  88, "Application 3" },
};
static const int n_button_config = 
	sizeof(button_config) / sizeof(*button_config);

#define KC(n) KS_KEYCODE(n)
static const keysym_t btnmgr_keydesc_default[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(1), 			KS_Escape,
    KC(28), 			KS_Return,
    KC(67), 			KS_f9,
    KC(68), 			KS_f10,
    KC(72), 			KS_KP_Up,
    KC(80), 			KS_KP_Down,
    KC(87), 			KS_f11,
    KC(88), 			KS_f12,
};
#undef KC
#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }
const struct wscons_keydesc btnmgr_keydesctab[] = {
	KBD_MAP(KB_US,		0,	btnmgr_keydesc_default),
	{0, 0, 0, 0}
};
#undef KBD_MAP

struct wskbd_mapdata btnmgr_keymapdata = {
	btnmgr_keydesctab,
	KB_US, /* XXX, This is bad idea... */
};

/*
 *  function bodies
 */
int
btnmgrmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
    
	if (strcmp(ma->ma_name, match->cf_driver->cd_name))
		return 0;

	return (1);
}

void
btnmgrattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int id;
	struct btnmgr_softc *sc = (struct btnmgr_softc *)self;
	struct wskbddev_attach_args wa;

	printf("\n");

	/*
	 * install button event listener
	 */
	for (id = 0; id < 16; id++)
		sc->sc_hook_tag = config_hook(CONFIG_HOOK_BUTTONEVENT,
					      id, CONFIG_HOOK_SHARE,
					      btnmgr_hook, sc);

	/*
	 * attach wskbd
	 */
	wa.console = 0;
	wa.keymap = &btnmgr_keymapdata;
	wa.accessops = &btnmgr_wskbd_accessops;
	wa.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &wa, wskbddevprint);
}

static int
btnmgr_hook(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct btnmgr_softc *sc = ctx;

	DPRINTF(("%s button: %s\n", btnmgr_name(id), msg ? "ON" : "OFF"));

	if (button_config[id].kevent) {
		u_int evtype;
		evtype = msg ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
#ifdef WSDISPLAY_COMPAT_RAWKBD
		if (sc->sc_rawkbd) {
			int n;
			u_char data[16];
			n = pckbd_encode(evtype, button_config[id].keycode,
					 data);
			wskbd_rawinput(sc->sc_wskbddev, data, n);
		} else
#endif
		wskbd_input(sc->sc_wskbddev, evtype, 
			    button_config[id].keycode);
	}

	if (id == CONFIG_HOOK_BUTTONEVENT_POWER && msg)
		config_hook_call(CONFIG_HOOK_PMEVENT, 
				 CONFIG_HOOK_PMEVENT_SUSPENDREQ, NULL);


	return (0);
}

char*
btnmgr_name(id)
	long id;
{
	if (id < n_button_config)
		return button_config[id].name;
	return ("unknown");
}

int
btnmgr_wskbd_enable(scx, on)
	void *scx;
	int on;
{
	struct btnmgr_softc *sc = scx;

	if (on) {
		if (sc->sc_enabled)
			return (EBUSY);
		sc->sc_enabled = 1;
	} else {
		sc->sc_enabled = 0;
	}

	return (0);
}

void
btnmgr_wskbd_set_leds(scx, leds)
	void *scx;
	int leds;
{
	/*
	 * We have nothing to do.
	 */
}

int
btnmgr_wskbd_ioctl(scx, cmd, data, flag, p)
	void *scx;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
#ifdef WSDISPLAY_COMPAT_RAWKBD
	struct btnmgr_softc *sc = scx;
#endif
	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_HPC_BTN;
		return 0;
	case WSKBDIO_SETLEDS:
		DPRINTF(("%s(%d): no LED\n", __FILE__, __LINE__));
		return 0;
	case WSKBDIO_GETLEDS:
		DPRINTF(("%s(%d): no LED\n", __FILE__, __LINE__));
		*(int *)data = 0;
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = (*(int *)data == WSKBD_RAW);
		DPRINTF(("%s(%d): rawkbd is %s\n", __FILE__, __LINE__,
			sc->sc_rawkbd ? "on" : "off"));
		return (0);
#endif
	}
	return (-1);
}

#ifdef notyet
int
btnmgropen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (EINVAL);
}

int
btnmgrclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (EINVAL);
}

int
btnmgrread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return (EINVAL);
}

int
btnmgrwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return (EINVAL);
}

int
btnmgrioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return (EINVAL);
}
#endif /* notyet */
