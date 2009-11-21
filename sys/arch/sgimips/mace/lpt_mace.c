/*	$NetBSD: lpt_mace.c,v 1.8 2009/11/21 17:40:28 rmind Exp $	*/

/*
 * Copyright (c) 2003 Christopher SEKIYA 
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: lpt_mace.c,v 1.8 2009/11/21 17:40:28 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>

#include <sgimips/mace/macevar.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

struct lpt_mace_softc {
	struct lpt_softc sc_lpt;

	/* XXX intr cookie */
};

static int	lpt_mace_match(device_t, cfdata_t , void *);
static void	lpt_mace_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lpt_mace, sizeof(struct lpt_mace_softc),
    lpt_mace_match, lpt_mace_attach, NULL, NULL);

static int
lpt_mace_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

static void
lpt_mace_attach(device_t parent, device_t self, void *aux)
{
	struct lpt_mace_softc *msc = device_private(self);
	struct lpt_softc *sc = &msc->sc_lpt;
	struct mace_attach_args *maa = aux;

	sc->sc_dev = self;
	sc->sc_iot = maa->maa_st;

	/* XXX should use bus_space_map() */
	if (bus_space_subregion(sc->sc_iot, maa->maa_sh,
	    maa->maa_offset, LPT_NPORTS, &sc->sc_ioh) != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	aprint_normal("\n");

	lpt_attach_subr(sc);

	cpu_intr_establish(maa->maa_intr, maa->maa_intrmask, lptintr, sc);

	return;
}
