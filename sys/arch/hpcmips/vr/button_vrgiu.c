/*	$NetBSD: button_vrgiu.c,v 1.2.6.4 2001/04/21 17:53:46 bouyer Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <machine/config_hook.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include "opt_vr41xx.h"
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrgiureg.h>

#include "locators.h"

struct button_vrgiu_softc {
	struct device sc_dev;
	vrgiu_chipset_tag_t sc_gc;
	vrgiu_function_tag_t sc_gf;
	int sc_port;
	long sc_id;
	int sc_active;
	config_hook_tag sc_hook_tag;
	config_hook_tag sc_ghook_tag;
};

static int	button_vrgiu_match __P((struct device *, struct cfdata *,
				       void *));
static void	button_vrgiu_attach __P((struct device *, struct device *,
					void *));
static int	button_vrgiu_intr __P((void*));
static int	button_vrgiu_state __P((void *ctx, int type, long id,
				      void *msg));

struct cfattach button_vrgiu_ca = {
	sizeof(struct button_vrgiu_softc), button_vrgiu_match, button_vrgiu_attach
};

int
button_vrgiu_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	platid_mask_t mask;
	struct gpbus_attach_args *gpa = aux;

	if (strcmp(gpa->gpa_busname, "gpbus"))
		return 0;
	if (match->cf_loc[NEWGPBUSIFCF_PLATFORM] == 0)
		return 0;
	mask = PLATID_DEREF(match->cf_loc[NEWGPBUSIFCF_PLATFORM]);
	return platid_match(&platid, &mask);
}

void
button_vrgiu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct gpbus_attach_args *gpa = aux;
	int *loc;
	struct button_vrgiu_softc *sc = (void*)self;
	int mode;
    
	sc->sc_gc = gpa->gpa_gc;
	sc->sc_gf = gpa->gpa_gf;

	loc = sc->sc_dev.dv_cfdata->cf_loc;
	sc->sc_port = loc[NEWGPBUSIFCF_PORT];
	sc->sc_id = loc[NEWGPBUSIFCF_ID];
	sc->sc_active = loc[NEWGPBUSIFCF_ACTIVE];
	printf(" port=%d id=%ld active=%s",
	       sc->sc_port, sc->sc_id, sc->sc_active ? "high" : "low");

#if 0
#if 1 /* Windows CE default */
	mode = VRGIU_INTR_EDGE_HOLD; 
#else /* XXX Don't challenge! Freestyle Only */
	mode = VRGIU_INTR_LEVEL_LOW_HOLD;
#endif
#endif
	mode = VRGIU_INTR_HOLD;
	if (loc[NEWGPBUSIFCF_LEVEL] != NEWGPBUSIFCF_LEVEL_DEFAULT) {
		mode |= VRGIU_INTR_LEVEL;
		if (loc[NEWGPBUSIFCF_LEVEL] == 0)
			mode |= VRGIU_INTR_LOW;
		else
			mode |= VRGIU_INTR_HIGH;
		printf(" sense=level");
	} else {
		mode |= VRGIU_INTR_EDGE;
		printf(" sense=edge");
	}

	if (sc->sc_port == NEWGPBUSIFCF_PORT_DEFAULT ||
	    sc->sc_id == NEWGPBUSIFCF_ID_DEFAULT)
		printf(" (ignored)");
	else
		sc->sc_gf->gf_intr_establish(sc->sc_gc, sc->sc_port,
					     mode, IPL_TTY,
					     button_vrgiu_intr, sc);
	sc->sc_ghook_tag = config_hook(CONFIG_HOOK_GET,
				       sc->sc_id,
				       CONFIG_HOOK_SHARE,
				       button_vrgiu_state,
				       sc);	
	printf("\n");
}

int
button_vrgiu_state(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct button_vrgiu_softc *sc = ctx;

	if (type != CONFIG_HOOK_GET || id != sc->sc_id)
		return 1;

	if (CONFIG_HOOK_VALUEP(msg))
		return 1;

	*(int*)msg = (sc->sc_gf->gf_portread(sc->sc_gc, sc->sc_port) == sc->sc_active);
	return 0;
}

int
button_vrgiu_intr(ctx)
	void *ctx;
{
	struct button_vrgiu_softc *sc = ctx;
	u_int32_t reg;
	int on;

	on = (sc->sc_gf->gf_portread(sc->sc_gc, sc->sc_port) == sc->sc_active);

	/* Clear interrupt */
	reg = sc->sc_gf->gf_regread_4(sc->sc_gc, GIUINTSTAT_REG);
	sc->sc_gf->gf_regwrite_4(sc->sc_gc, GIUINTSTAT_REG,
				 reg & ~(1 << sc->sc_port));

	config_hook_call(CONFIG_HOOK_BUTTONEVENT, sc->sc_id, (void*)on);

	return 0;
}
