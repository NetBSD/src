/*	$NetBSD: pwctl_vrgiu.c,v 1.5.2.3 2001/03/27 15:30:56 bouyer Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 * Copyright (c) 2000,2001
 *         SATO Kazumi. All rights reserved.
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

#define PWCTLVRGIUDEBUG
#ifdef PWCTLVRGIUDEBUG
int	pwctl_vrgiu_debug = 0;
#define	DPRINTF(arg) if (pwctl_vrgiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

struct pwctl_vrgiu_softc {
	struct device sc_dev;
	vrgiu_chipset_tag_t sc_gc;
	vrgiu_function_tag_t sc_gf;
	int sc_port;
	long sc_id;
	int sc_on, sc_off;
	config_hook_tag sc_hook_tag;
	config_hook_tag sc_hook_hardpower;
	config_hook_tag sc_ghook_tag;
	int sc_save;
	int sc_initvalue;
};

static int	pwctl_vrgiu_match __P((struct device *, struct cfdata *,
				       void *));
static void	pwctl_vrgiu_attach __P((struct device *, struct device *,
					void *));
static int	pwctl_vrgiu_hook __P((void *ctx, int type, long id,
				      void *msg));
static int	pwctl_vrgiu_ghook __P((void *ctx, int type, long id,
				      void *msg));
int	pwctl_vrgiu_hardpower __P((void *, int, long, void *));

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
	sc->sc_on = loc[NEWGPBUSIFCF_ACTIVE] ? 1 : 0;
	sc->sc_off = loc[NEWGPBUSIFCF_ACTIVE] ? 0 : 1;
	sc->sc_initvalue = loc[NEWGPBUSIFCF_INITVALUE];
	printf(" port=%d id=%ld on=%d%s",
		 sc->sc_port, sc->sc_id, sc->sc_on,
		 sc->sc_initvalue == -1 ? "" :
		 sc->sc_initvalue ? " init=on" : " init=off");

	if (sc->sc_port == NEWGPBUSIFCF_PORT_DEFAULT ||
	    sc->sc_id == NEWGPBUSIFCF_ID_DEFAULT) {
		printf(" (ignored)");
	} else {
		sc->sc_hook_tag = config_hook(CONFIG_HOOK_POWERCONTROL,
					      sc->sc_id, CONFIG_HOOK_SHARE,
					      pwctl_vrgiu_hook, sc);
		sc->sc_ghook_tag = config_hook(CONFIG_HOOK_GET,
					      sc->sc_id, CONFIG_HOOK_SHARE,
					      pwctl_vrgiu_ghook, sc);
		sc->sc_hook_hardpower = config_hook(CONFIG_HOOK_PMEVENT,
						CONFIG_HOOK_PMEVENT_HARDPOWER,
						CONFIG_HOOK_SHARE,
						pwctl_vrgiu_hardpower, sc);

	}
	if (sc->sc_initvalue != -1)
		sc->sc_gf->gf_portwrite(sc->sc_gc, sc->sc_port,
					sc->sc_initvalue ? sc->sc_on : sc->sc_off);
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

	DPRINTF(("pwctl hook: port %d %s(%d)", sc->sc_port,
		 msg ? "ON" : "OFF", msg ? sc->sc_on : sc->sc_off));
	sc->sc_gf->gf_portwrite(sc->sc_gc, sc->sc_port,
				msg ? sc->sc_on : sc->sc_off);
	return (0);
}

int
pwctl_vrgiu_ghook(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct pwctl_vrgiu_softc *sc = ctx;

	if (CONFIG_HOOK_VALUEP(msg))
		return 1;

	*(int*)msg = sc->sc_gf->gf_portread(sc->sc_gc, sc->sc_port) == sc->sc_on;
	DPRINTF(("pwctl ghook: port %d %s(%d)", sc->sc_port,
		 *(int*)msg? "ON" : "OFF", *(int*)msg ? sc->sc_on : sc->sc_off));
	return 0;
}

int
pwctl_vrgiu_hardpower(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
	struct pwctl_vrgiu_softc *sc = ctx;
	int why =(int)msg;

#if 0
	/* XXX debug print cause hang system... Huum...*/
	DPRINTF(("pwctl hardpower: port %d %s: %s(%d)\n", sc->sc_port,
		why == PWR_RESUME? "resume" 
		: why == PWR_SUSPEND? "suspend" : "standby",
		sc->sc_save == sc->sc_on ? "on": "off", sc->sc_save));
#endif /* 0 */
	switch (why) {
	case PWR_STANDBY:
		break;
	case PWR_SUSPEND:
		sc->sc_save = sc->sc_gf->gf_portread(sc->sc_gc, sc->sc_port);
		sc->sc_gf->gf_portwrite(sc->sc_gc, sc->sc_port, sc->sc_off);
		break;
	case PWR_RESUME:
		sc->sc_gf->gf_portwrite(sc->sc_gc, sc->sc_port, sc->sc_save);
		break;
	}
	return (0);
}
