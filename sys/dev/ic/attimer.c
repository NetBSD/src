/*	$NetBSD: attimer.c,v 1.6 2008/01/03 01:21:44 dyoung Exp $	*/

/*
 *  Copyright (c) 2005 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * AT Timer
 *
 * This code only allows control over the pitch of the speaker (pcppi(4)).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: attimer.c,v 1.6 2008/01/03 01:21:44 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/isa/isareg.h>

#include <dev/ic/attimervar.h>
#include <dev/ic/i8253reg.h>

extern struct cfdriver attimer_cd;

void
attimer_attach(struct attimer_softc *sc)
{
	sc->sc_flags |= ATT_CONFIGURED;

	if (!pmf_device_register(&sc->sc_dev, NULL, NULL))
		aprint_error_dev(&sc->sc_dev, "couldn't establish power handler\n");
}

int
attimer_detach(device_t self, int flags)
{
	struct attimer_softc *sc = device_private(self);

	pmf_device_deregister(self);
	sc->sc_flags &= ~ATT_CONFIGURED;
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_size);
	return 0;
}

/*
 * This is slightly artificial.  While the code looks fine, I can't help
 * but pray I'll get the attimer object associated to the pcppi object
 * that calls the routine.
 *
 * There's not much at risk here, as there's about no chance to have a
 * computer with more than one pcppi/attimer accessible.
 */

struct attimer_softc *
attimer_attach_speaker(void)
{
	int i;
	struct attimer_softc *sc;

	for (i = 0; i < attimer_cd.cd_ndevs; i++) {
		if (attimer_cd.cd_devs[i] == NULL)
			continue;
		sc = (struct attimer_softc *)attimer_cd.cd_devs[i];
		if ((sc->sc_flags & ATT_CONFIGURED) &&
		    !(sc->sc_flags & ATT_ATTACHED)) {
			sc->sc_flags |= ATT_ATTACHED;
			return sc;
		}
	}
	return NULL;
}

void
attimer_set_pitch(struct attimer_softc *sc, int pitch)
{
	int s;
	s = splhigh();
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_MODE,
	    TIMER_SEL2 | TIMER_16BIT | TIMER_SQWAVE);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_CNTR2,
	    TIMER_DIV(pitch) % 256);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, TIMER_CNTR2,
	    TIMER_DIV(pitch) / 256);
	splx(s);
}
