/*	$NetBSD: spkr_synth.c,v 1.7 2016/12/13 20:20:34 christos Exp $	*/

/*-
 * Copyright (c) 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spkr_synth.c,v 1.7 2016/12/13 20:20:34 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/condvar.h>
#include <sys/mutex.h>
#include <sys/kthread.h>
#include <sys/sysctl.h>
#include <dev/audio_if.h>
#include <dev/audiovar.h>

struct vbell_args {
	u_int pitch;
	u_int period;
	u_int volume;
	bool dying;
};

static void bell_thread(void *) __dead;

#include <dev/audiobellvar.h>

#include <dev/spkrvar.h>
#include <dev/spkrio.h>

static int spkr_synth_probe(device_t, cfdata_t, void *);
static void spkr_synth_attach(device_t, device_t, void *);
static int spkr_synth_detach(device_t, int);

struct spkr_synth_softc {
	struct spkr_softc sc_spkr;
	lwp_t		*sc_bellthread;
	kmutex_t	sc_bellock;
	kcondvar_t	sc_bellcv;
	device_t		sc_audiodev;
	struct vbell_args sc_bell_args;
};

CFATTACH_DECL_NEW(spkr_synth, sizeof(struct spkr_synth_softc),
    spkr_synth_probe, spkr_synth_attach, spkr_synth_detach, NULL);

MODULE(MODULE_CLASS_DRIVER, spkr, NULL /* "audio" */);

static int
spkr_modcmd(modcmd_t cmd, void *arg)
{
	return spkr__modcmd(cmd, arg);
}

static void
spkr_synth_tone(device_t self, u_int xhz, u_int ticks)
{
	struct spkr_synth_softc *sc = device_private(self);

#ifdef SPKRDEBUG
	aprint_debug_dev(self, "%s: %u %d\n", __func__, xhz, ticks);
#endif /* SPKRDEBUG */
	audiobell(sc->sc_audiodev, xhz, ticks * (1000 / hz), 80, 0);
}

static void
spkr_synth_rest(device_t self, int ticks)
{
	struct spkr_synth_softc *sc = device_private(self);
	
#ifdef SPKRDEBUG
	aprint_debug_dev(self, "%s: %d\n", __func__, ticks);
#endif /* SPKRDEBUG */
	if (ticks > 0)
		audiobell(sc->sc_audiodev, 0, ticks * (1000 / hz), 80, 0);
}

#ifdef notyet
static void
spkr_synth_play(device_t self, u_int pitch, u_int period, u_int volume)
{
	struct spkr_synth_softc *sc = device_private(self);

	mutex_enter(&sc->sc_bellock);
	sc->sc_bell_args.dying = false;
	sc->sc_bell_args.pitch = pitch;
	sc->sc_bell_args.period = period;
	sc->sc_bell_args.volume = volume;

	cv_broadcast(&sc->sc_bellcv);
	mutex_exit(&sc->sc_bellock);
}
#endif

static int
spkr_synth_probe(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
spkr_synth_attach(device_t parent, device_t self, void *aux)
{
	struct spkr_synth_softc *sc = device_private(self);

	aprint_normal("\n");

	sc->sc_audiodev = parent;
	
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n"); 
	mutex_init(&sc->sc_bellock, MUTEX_DEFAULT, IPL_SCHED);
	cv_init(&sc->sc_bellcv, "bellcv");

	kthread_create(PRI_BIO, KTHREAD_MPSAFE | KTHREAD_MUSTJOIN, NULL,
	    bell_thread, sc, &sc->sc_bellthread, device_xname(self));

	spkr_attach(self, spkr_synth_tone, spkr_synth_rest);
}

static int
spkr_synth_detach(device_t self, int flags)
{
	struct spkr_synth_softc *sc = device_private(self);

	pmf_device_deregister(self);

	mutex_enter(&sc->sc_bellock);
	sc->sc_bell_args.dying = true;

	cv_broadcast(&sc->sc_bellcv);
	mutex_exit(&sc->sc_bellock);

	kthread_join(sc->sc_bellthread);
	cv_destroy(&sc->sc_bellcv);
	mutex_destroy(&sc->sc_bellock);


	return 0;
}

static void
bell_thread(void *arg)
{
	struct spkr_synth_softc *sc = arg;
	struct vbell_args *vb = &sc->sc_bell_args;
	u_int bpitch;
	u_int bperiod;
	u_int bvolume;
	
	for (;;) {
		mutex_enter(&sc->sc_bellock);
		cv_wait_sig(&sc->sc_bellcv, &sc->sc_bellock);
		
		if (vb->dying == true) {
			mutex_exit(&sc->sc_bellock);
			kthread_exit(0);
		}
		
		bpitch = vb->pitch;
		bperiod = vb->period;
		bvolume = vb->volume;
		mutex_exit(&sc->sc_bellock);
		audiobell(sc->sc_audiodev, bpitch, bperiod, bvolume, 0);
	}
}
