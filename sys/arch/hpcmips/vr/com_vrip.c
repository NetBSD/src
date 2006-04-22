/*	$NetBSD: com_vrip.c,v 1.17.6.1 2006/04/22 11:37:30 simonb Exp $	*/

/*-
 * Copyright (c) 1999 SASAKI Takesi. All rights reserved.
 * Copyright (c) 1999, 2002 PocketBSD Project. All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_vrip.c,v 1.17.6.1 2006/04/22 11:37:30 simonb Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <sys/termios.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/cmureg.h>
#include <hpcmips/vr/siureg.h>

#include <dev/ic/comvar.h>
#include <dev/ic/comreg.h>

#include "opt_vr41xx.h"
#include <hpcmips/vr/com_vripvar.h>

#include "locators.h"

#define COMVRIPDEBUG
#ifdef COMVRIPDEBUG
int	com_vrip_debug = 0;
#define	DPRINTF(arg) if (com_vrip_debug) printf arg;
#define	VPRINTF(arg) if (com_vrip_debug || bootverbose) printf arg;
#else
#define	DPRINTF(arg)
#define	VPRINTF(arg) if (bootverbose) printf arg;
#endif

struct com_vrip_softc {
	struct com_softc	sc_com;
	int sc_pwctl;
};

static int com_vrip_probe(struct device *, struct cfdata *, void *);
static void com_vrip_attach(struct device *, struct device *, void *);
static int com_vrip_common_probe(bus_space_tag_t, int);

void vrcmu_init(void);
void vrcmu_supply(int);
void vrcmu_mask(int);

CFATTACH_DECL(com_vrip, sizeof(struct com_vrip_softc),
    com_vrip_probe, com_vrip_attach, NULL, NULL);

int
com_vrip_cndb_attach(bus_space_tag_t iot, int iobase, int rate, int frequency,
    tcflag_t cflag, int kgdb)
{

	if (!com_vrip_common_probe(iot, iobase))
		return (EIO);	/* I can't find appropriate error number. */
#ifdef KGDB
	if (kgdb)
		return (com_kgdb_attach(iot, iobase, rate, frequency,
		    COM_TYPE_NORMAL, cflag));
	else
#endif
		return (comcnattach(iot, iobase, rate, frequency,
		    COM_TYPE_NORMAL, cflag));
}

static int
com_vrip_common_probe(bus_space_tag_t iot, int iobase)
{
	bus_space_handle_t ioh;
	int rv;

	if (bus_space_map(iot, iobase, 1, 0, &ioh)) {
		printf(": can't map i/o space\n");
		return 0;
	}
	rv = comprobe1(iot, ioh);
	bus_space_unmap(iot, ioh, 1);
	return (rv);
}

static int
com_vrip_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vrip_attach_args *va = aux;
	bus_space_tag_t iot = va->va_iot;
	int rv;
	
	DPRINTF(("==com_vrip_probe"));

	if (va->va_addr == VRIPIFCF_ADDR_DEFAULT ||
	    va->va_unit == VRIPIFCF_UNIT_DEFAULT) {
		printf(": need addr and intr.\n");
		return (0);
	}

	vrip_power(va->va_vc, va->va_unit, 1);

	if (com_is_console(iot, va->va_addr, 0)) {
		/*
		 *  We have alredy probed.
		 */
		rv = 1;
	} else {
		rv = com_vrip_common_probe(iot, va->va_addr);
	}

	DPRINTF((rv ? ": found COM ports\n" : ": can't probe COM device\n"));

	if (rv) {
		va->va_size = COM_NPORTS;
	}
	return (rv);
}


static void
com_vrip_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_vrip_softc *vsc = (void *) self;
	struct com_softc *sc = &vsc->sc_com;
	struct vrip_attach_args *va = aux;

	bus_space_tag_t iot = va->va_iot;
	bus_space_handle_t ioh;

	vsc->sc_pwctl = device_cfdata(&sc->sc_dev)->cf_loc[VRIPIFCF_PWCTL];

	DPRINTF(("==com_vrip_attach"));

	if (bus_space_map(iot, va->va_addr, 1, 0, &ioh)) {
		printf(": can't map bus space\n");
		return;
	}
	sc->sc_iobase = va->va_addr;
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	sc->enable = NULL; /* XXX: CMU control */
	sc->disable = NULL;

	sc->sc_frequency = VRCOM_FREQ;
	/* Power management */
	vrip_power(va->va_vc, va->va_unit, 1);
	/* XXX, locale 'ID' must be need */
	config_hook_call(CONFIG_HOOK_POWERCONTROL, vsc->sc_pwctl, (void*)1);

	DPRINTF(("Try to attach com.\n"));
	com_attach_subr(sc);

	DPRINTF(("Establish intr"));
	if (!vrip_intr_establish(va->va_vc, va->va_unit, 0, IPL_TTY,
	    comintr, self)) {
		printf("%s: can't map interrupt line.\n", sc->sc_dev.dv_xname);
	}

	DPRINTF((":return()"));
	VPRINTF(("%s: pwctl %d\n", vsc->sc_com.sc_dev.dv_xname, vsc->sc_pwctl));
}
