/*	$NetBSD: pwctl_vrgiu.c,v 1.1 1999/12/23 06:26:10 takemura Exp $	*/

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

#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vrgiureg.h>

#include "locators.h"

struct pwctl_vrgiu_softc {
	struct device sc_dev;
	vrgiu_chipset_tag_t sc_gc;
	vrgiu_function_tag_t sc_gf;
	int sc_port;
	long sc_id;
	config_hook_tag sc_hook_tag;
};

static int	pwctl_vrgiu_match __P((struct device *, struct cfdata *,
				       void *));
static void	pwctl_vrgiu_attach __P((struct device *, struct device *,
					void *));
static int	pwctl_vrgiu_hook __P((void *ctx, int type, long id,
				      void *msg));

struct cfattach pwctl_vrgiu_ca = {
	sizeof(struct pwctl_vrgiu_softc), pwctl_vrgiu_match, pwctl_vrgiu_attach
};

int
pwctl_vrgiu_match(parent, match, aux)
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
	if (!platid_match(&platid, &mask))
		return 0;
	return 1;
}

void
pwctl_vrgiu_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct gpbus_attach_args *gpa = aux;
	int *loc;
	struct pwctl_vrgiu_softc *sc = (void*)self;
    
	sc->sc_gc = gpa->gpa_gc;
	sc->sc_gf = gpa->gpa_gf;

	loc = sc->sc_dev.dv_cfdata->cf_loc;
	sc->sc_port = loc[NEWGPBUSIFCF_PORT];
	sc->sc_id = loc[NEWGPBUSIFCF_ID];
	printf(" port=%d id=%ld", sc->sc_port, sc->sc_id);

	if (sc->sc_port == NEWGPBUSIFCF_PORT_DEFAULT ||
	    sc->sc_id == NEWGPBUSIFCF_ID_DEFAULT) {
		printf(" (ignored)");
	} else {
		sc->sc_hook_tag = config_hook(CONFIG_HOOK_POWERCONTROL,
					      sc->sc_id, CONFIG_HOOK_SHARE,
					      pwctl_vrgiu_hook, sc);
	}
	printf("\n");
}

int
pwctl_vrgiu_hook(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct pwctl_vrgiu_softc *sc = ctx;

	printf("pwctl hook: port %d %s", sc->sc_port, msg ? "ON" : "OFF");
	sc->sc_gf->gf_portwrite(sc->sc_gc, sc->sc_port, msg ? 1 : 0);
	return (0);
}
