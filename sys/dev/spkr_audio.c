/*	$NetBSD: spkr_audio.c,v 1.6.4.2 2017/12/03 11:36:58 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: spkr_audio.c,v 1.6.4.2 2017/12/03 11:36:58 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/sysctl.h>
#include <dev/audio_if.h>
#include <dev/audiovar.h>

#include <dev/audiobellvar.h>

#include <dev/spkrvar.h>
#include <dev/spkrio.h>

static int spkr_audio_probe(device_t, cfdata_t, void *);
static void spkr_audio_attach(device_t, device_t, void *);
static int spkr_audio_detach(device_t, int);
static int spkr_audio_rescan(device_t, const char *, const int *);
static void spkr_audio_childdet(device_t, device_t);

struct spkr_audio_softc {
	struct spkr_softc sc_spkr;
	device_t		sc_audiodev;
};

CFATTACH_DECL3_NEW(spkr_audio, sizeof(struct spkr_audio_softc),
    spkr_audio_probe, spkr_audio_attach, spkr_audio_detach, NULL,
    spkr_audio_rescan, spkr_audio_childdet, 0);

static void
spkr_audio_tone(device_t self, u_int xhz, u_int ticks)
{
	struct spkr_audio_softc *sc = device_private(self);

#ifdef SPKRDEBUG
	aprint_debug_dev(self, "%s: %u %d\n", __func__, xhz, ticks);
#endif /* SPKRDEBUG */
	audiobell(sc->sc_audiodev, xhz, ticks * (1000 / hz),
	    sc->sc_spkr.sc_vol, 0);
}

static void
spkr_audio_rest(device_t self, int ticks)
{
	struct spkr_audio_softc *sc = device_private(self);
	
#ifdef SPKRDEBUG
	aprint_debug_dev(self, "%s: %d\n", __func__, ticks);
#endif /* SPKRDEBUG */
	if (ticks > 0)
		audiobell(sc->sc_audiodev, 0, ticks * (1000 / hz),
		    sc->sc_spkr.sc_vol, 0);
}

static int
spkr_audio_probe(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
spkr_audio_attach(device_t parent, device_t self, void *aux)
{
	struct spkr_audio_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": PC Speaker (synthesized)\n");

	sc->sc_audiodev = parent;
	sc->sc_spkr.sc_vol = 80;
	
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n"); 

	spkr_attach(self, spkr_audio_tone, spkr_audio_rest);
}

static int
spkr_audio_detach(device_t self, int flags)
{
	int error;

	if ((error = spkr_detach(self, flags)) != 0)
		return error;

	pmf_device_deregister(self);

	return 0;
}

static int 
spkr_audio_rescan(device_t self, const char *iattr, const int *locators)
{

	return spkr_rescan(self, iattr, locators);
}

static void 
spkr_audio_childdet(device_t self, device_t child)
{
 
	spkr_childdet(self, child);
}
