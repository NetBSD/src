/* $NetBSD: wsbell.c,v 1.9.2.2 2017/12/03 11:37:37 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 * All rights reserved.
 *
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)ms.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Keyboard Bell driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wsbell.c,v 1.9.2.2 2017/12/03 11:37:37 jdolecek Exp $");

#if defined(_KERNEL_OPT)
#include "wsmux.h"
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/kauth.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/device.h>
#include <sys/vnode.h>
#include <sys/callout.h>
#include <sys/module.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsbellvar.h>
#include <dev/wscons/wsbellmuxvar.h>
#include <dev/wscons/wsbelldata.h>

#include <dev/spkrio.h>

#include "ioconf.h"

#if defined(WSMUX_DEBUG) && NWSMUX > 0
#define DPRINTF(x)	if (wsmuxdebug) printf x
#define DPRINTFN(n,x)	if (wsmuxdebug > (n)) printf x
extern int wsmuxdebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

static void bell_thread(void *);
static inline void spkr_audio_play(struct wsbell_softc *, u_int, u_int, u_int);

static int  wsbell_match(device_t, cfdata_t, void *);
static void wsbell_attach(device_t, device_t, void *);
static int  wsbell_detach(device_t, int);
static int  wsbell_activate(device_t, enum devact);

#if NWSMUX > 0
static int  wsbell_mux_open(struct wsevsrc *, struct wseventvar *);
static int  wsbell_mux_close(struct wsevsrc *);

static int  wsbelldoopen(struct wsbell_softc *, struct wseventvar *);
static int  wsbelldoioctl(device_t, u_long, void *, int, struct lwp *);

static int  wsbell_do_ioctl(struct wsbell_softc *, u_long, void *,
			     int, struct lwp *);

#endif

CFATTACH_DECL_NEW(wsbell, sizeof (struct wsbell_softc),
    wsbell_match, wsbell_attach, wsbell_detach, wsbell_activate);

extern struct cfdriver wsbell_cd;

extern dev_type_open(spkropen);
extern dev_type_close(spkrclose);
extern dev_type_ioctl(spkrioctl);

const struct cdevsw wsbell_cdevsw = {
	.d_open = noopen,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

#if NWSMUX > 0
struct wssrcops wsbell_srcops = {
	WSMUX_BELL,
	wsbell_mux_open, wsbell_mux_close, wsbelldoioctl, wsbelldoioctl, NULL
};
#endif

int
wsbell_match(device_t parent, cfdata_t match, void *aux)
{
	return (1);
}

void
wsbell_attach(device_t parent, device_t self, void *aux)
{
        struct wsbell_softc *sc = device_private(self);
	struct wsbelldev_attach_args *ap = aux;
#if NWSMUX > 0
	int mux, error;
#endif

	sc->sc_base.me_dv = self;
	sc->sc_accesscookie = ap->accesscookie;

	sc->sc_dying = false;
	sc->sc_spkr = device_unit(parent);
	sc->sc_bell_data = wskbd_default_bell_data;
#if NWSMUX > 0
	sc->sc_base.me_ops = &wsbell_srcops;
	mux = device_cfdata(self)->wsbelldevcf_mux;
	if (mux >= 0) {
		error = wsmux_attach_sc(wsmux_getmux(mux), &sc->sc_base);
		if (error)
			aprint_error(" attach error=%d", error);
		else
			aprint_normal(" mux %d", mux);
	}
#else
	if (device_cfdata(self)->wsbelldevcf_mux >= 0)
		aprint_normal(" (mux ignored)");
#endif

	aprint_naive("\n");
	aprint_normal("\n");

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	mutex_init(&sc->sc_bellock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_bellcv, "bellcv");

	kthread_create(PRI_BIO, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN, NULL,
	    bell_thread, sc, &sc->sc_bellthread, "%s", device_xname(self));
}

int
wsbell_activate(device_t self, enum devact act)
{
	struct wsbell_softc *sc = device_private(self);

	if (act == DVACT_DEACTIVATE)
		sc->sc_dying = true;
	return (0);
}

int
wsbell_detach(device_t self, int flags)
{
	struct wsbell_softc *sc = device_private(self);
	struct wseventvar *evar;
	int maj, mn;
	int s;

#if NWSMUX > 0
	/* Tell parent mux we're leaving. */
	if (sc->sc_base.me_parent != NULL) {
		DPRINTF(("wsbell_detach:\n"));
		wsmux_detach_sc(&sc->sc_base);
	}
#endif

	/* If we're open ... */
	evar = sc->sc_base.me_evp;
	if (evar != NULL && evar->io != NULL) {
		s = spltty();
		if (--sc->sc_refcnt >= 0) {
			struct wscons_event event;

			/* Wake everyone by generating a dummy event. */
			event.type = 0;
			event.value = 0;
			if (wsevent_inject(evar, &event, 1) != 0)
				wsevent_wakeup(evar);

			/* Wait for processes to go away. */
			if (tsleep(sc, PZERO, "wsmdet", hz * 60))
				printf("wsbell_detach: %s didn't detach\n",
				       device_xname(self));
		}
		splx(s);
	}

	/* locate the major number */
	maj = cdevsw_lookup_major(&wsbell_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = device_unit(self);
	vdevgone(maj, mn, mn, VCHR);

	mutex_enter(&sc->sc_bellock);
	sc->sc_dying = true;

	cv_broadcast(&sc->sc_bellcv);
	mutex_exit(&sc->sc_bellock);

	kthread_join(sc->sc_bellthread);
	cv_destroy(&sc->sc_bellcv);
	mutex_destroy(&sc->sc_bellock);

	return (0);
}

#if NWSMUX > 0
int
wsbelldoopen(struct wsbell_softc *sc, struct wseventvar *evp)
{
	return (0);
}

/* A wrapper around the ioctl() workhorse to make reference counting easy. */
int
wsbelldoioctl(device_t dv, u_long cmd, void *data, int flag,
	       struct lwp *l)
{
	struct wsbell_softc *sc = device_private(dv);
	int error;

	sc->sc_refcnt++;
	error = wsbell_do_ioctl(sc, cmd, data, flag, l);
	if (--sc->sc_refcnt < 0)
		wakeup(sc);
	return (error);
}

int
wsbell_do_ioctl(struct wsbell_softc *sc, u_long cmd, void *data,
		 int flag, struct lwp *l)
{
	struct wskbd_bell_data *ubdp, *kbdp;
	int error;

	if (sc->sc_dying == true)
		return (EIO);

	/*
	 * Try the wsbell specific ioctls.
	 */
	switch (cmd) {
	case WSKBDIO_SETBELL:
		if ((flag & FWRITE) == 0)
			return (EACCES);
		kbdp = &sc->sc_bell_data;
setbell:
		ubdp = (struct wskbd_bell_data *)data;
		SETBELL(kbdp, ubdp, kbdp);
		return (0);

	case WSKBDIO_GETBELL:
		kbdp = &sc->sc_bell_data;
getbell:
		ubdp = (struct wskbd_bell_data *)data;
		SETBELL(ubdp, kbdp, kbdp);
		return (0);

	case WSKBDIO_SETDEFAULTBELL:
		if ((error = kauth_authorize_device(l->l_cred,
		    KAUTH_DEVICE_WSCONS_KEYBOARD_BELL, NULL, NULL,
		    NULL, NULL)) != 0)
			return (error);
		kbdp = &wskbd_default_bell_data;
		goto setbell;


	case WSKBDIO_GETDEFAULTBELL:
		kbdp = &wskbd_default_bell_data;
		goto getbell;

	case WSKBDIO_BELL:
		if ((flag & FWRITE) == 0)
			return (EACCES);
		spkr_audio_play(sc, sc->sc_bell_data.pitch,
		    sc->sc_bell_data.period, sc->sc_bell_data.volume);

		return 0;

	case WSKBDIO_COMPLEXBELL:
		if ((flag & FWRITE) == 0)
			return (EACCES);
		if (data == NULL)
			return 0;
#define d ((struct wskbd_bell_data *)data)
		spkr_audio_play(sc, d->pitch, d->period, d->volume);
#undef d
		return 0;
	}	

	return (EPASSTHROUGH);
}
#endif

static void
bell_thread(void *arg)
{
	struct wsbell_softc *sc = arg;
	struct vbell_args *vb = &sc->sc_bell_args;
	tone_t tone;
	u_int vol;
	
	for (;;) {
		mutex_enter(&sc->sc_bellock);
		cv_wait_sig(&sc->sc_bellcv, &sc->sc_bellock);
		
		if (sc->sc_dying == true) {
			mutex_exit(&sc->sc_bellock);
			kthread_exit(0);
		}
		
		tone.frequency = vb->pitch;
		tone.duration = vb->period;
		vol = vb->volume;
		mutex_exit(&sc->sc_bellock);

		if (spkropen(sc->sc_spkr, FWRITE, 0, NULL) != 0)
			continue;
		spkrioctl(sc->sc_spkr, SPKRSETVOL, &vol, 0, curlwp);
		spkrioctl(sc->sc_spkr, SPKRTONE, &tone, 0, curlwp);
		spkrclose(sc->sc_spkr, FWRITE, 0, curlwp);
	}
}

static inline void
spkr_audio_play(struct wsbell_softc *sc, u_int pitch, u_int period, u_int volume)
{

	mutex_enter(&sc->sc_bellock);
	sc->sc_bell_args.pitch = pitch;
	sc->sc_bell_args.period = period / 5;
	sc->sc_bell_args.volume = volume;

	cv_broadcast(&sc->sc_bellcv);
	mutex_exit(&sc->sc_bellock);
}

#if NWSMUX > 0
int
wsbell_mux_open(struct wsevsrc *me, struct wseventvar *evp)
{
	struct wsbell_softc *sc = (struct wsbell_softc *)me;

	if (sc->sc_base.me_evp != NULL)
		return (EBUSY);

	return wsbelldoopen(sc, evp);
}

int
wsbell_mux_close(struct wsevsrc *me)
{
	struct wsbell_softc *sc = (struct wsbell_softc *)me;

	sc->sc_base.me_evp = NULL;

	return (0);
}
#endif /* NWSMUX > 0 */

MODULE(MODULE_CLASS_DRIVER, wsbell, "spkr");

#ifdef _MODULE
int wsbell_bmajor = -1, wsbell_cmajor = -1;

#include "ioconf.c"
#endif

static int
wsbell_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = devsw_attach("wsbell", NULL, &wsbell_bmajor,
		    &wsbell_cdevsw, &wsbell_cmajor);
		if (error)
			break;

		error = config_init_component(cfdriver_ioconf_wsbell,
		    cfattach_ioconf_wsbell, cfdata_ioconf_wsbell);
		if (error)
			devsw_detach(NULL, &wsbell_cdevsw);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		devsw_detach(NULL, &wsbell_cdevsw);
		error = config_fini_component(cfdriver_ioconf_wsbell,
		    cfattach_ioconf_wsbell, cfdata_ioconf_wsbell);
		if (error)
			devsw_attach("wsbell", NULL, &wsbell_bmajor,
			    &wsbell_cdevsw, &wsbell_cmajor);
#endif
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}
