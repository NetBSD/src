/*	$NetBSD: spkr_pcppi.c,v 1.11.4.2 2017/12/03 11:37:05 jdolecek Exp $	*/

/*
 * Copyright (c) 1990 Eric S. Raymond (esr@snark.thyrsus.com)
 * Copyright (c) 1990 Andrew A. Chernov (ache@astral.msk.su)
 * Copyright (c) 1990 Lennart Augustsson (lennart@augustsson.net)
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Eric S. Raymond
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * spkr.c -- device driver for console speaker on 80386
 *
 * v1.1 by Eric S. Raymond (esr@snark.thyrsus.com) Feb 1990
 *      modified for 386bsd by Andrew A. Chernov <ache@astral.msk.su>
 *      386bsd only clean version, all SYSV stuff removed
 *      use hz value from param.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spkr_pcppi.c,v 1.11.4.2 2017/12/03 11:37:05 jdolecek Exp $");

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

#include <sys/bus.h>

#include <dev/isa/pcppivar.h>

#include <dev/spkrvar.h>
#include <dev/spkrio.h>

struct spkr_pcppi_softc {
	struct spkr_softc sc_spkr;
	pcppi_tag_t sc_pcppicookie;
	void (*sc_bell_func)(pcppi_tag_t, int, int, int);
};

static int spkr_pcppi_probe(device_t, cfdata_t, void *);
static void spkr_pcppi_attach(device_t, device_t, void *);
static int spkr_pcppi_detach(device_t, int);
static int spkr_pcppi_rescan(device_t, const char *, const int *);
static void spkr_pcppi_childdet(device_t, device_t);

CFATTACH_DECL3_NEW(spkr_pcppi, sizeof(struct spkr_pcppi_softc),
    spkr_pcppi_probe, spkr_pcppi_attach, spkr_pcppi_detach, NULL,
    spkr_pcppi_rescan, spkr_pcppi_childdet, 0);

#define SPKRPRI (PZERO - 1)

/* emit tone of frequency hz for given number of ticks */
static void
spkr_pcppi_tone(device_t self, u_int xhz, u_int ticks)
{
#ifdef SPKRDEBUG
	aprint_debug_dev(self, "%s: %u %u\n", __func__, xhz, ticks);
#endif /* SPKRDEBUG */
	struct spkr_pcppi_softc *sc = device_private(self);
	(*sc->sc_bell_func)(sc->sc_pcppicookie, xhz, ticks, PCPPI_BELL_SLEEP);
}

/* rest for given number of ticks */
static void
spkr_pcppi_rest(device_t self, int ticks)
{
	/*
	 * Set timeout to endrest function, then give up the timeslice.
	 * This is so other processes can execute while the rest is being
	 * waited out.
	 */
#ifdef SPKRDEBUG
	aprint_debug_dev(self, "%s: %d\n", __func__, ticks);
#endif /* SPKRDEBUG */
	if (ticks > 0)
		tsleep(self, SPKRPRI | PCATCH, device_xname(self), ticks);
}

static int
spkr_pcppi_probe(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
spkr_pcppi_attach(device_t parent, device_t self, void *aux)
{
	struct pcppi_attach_args *pa = aux;
	struct spkr_pcppi_softc *sc = device_private(self);

	aprint_naive("\n");
	aprint_normal(": PC Speaker\n");

	sc->sc_pcppicookie = pa->pa_cookie;
	sc->sc_bell_func = pa->pa_bell_func;
	spkr_attach(self, spkr_pcppi_tone, spkr_pcppi_rest);
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
spkr_pcppi_detach(device_t self, int flags)
{
	struct spkr_pcppi_softc *sc = device_private(self);
	int error;

	if ((error = spkr_detach(self, flags)) != 0)
		return error;

	sc->sc_pcppicookie = NULL;
	pmf_device_deregister(self);
	return 0;
}

static int
spkr_pcppi_rescan(device_t self, const char *iattr, const int *locators)
{

	return spkr_rescan(self, iattr, locators);
}

static void
spkr_pcppi_childdet(device_t self, device_t child)
{

	spkr_childdet(self, child);
}
