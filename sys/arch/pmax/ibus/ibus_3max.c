/* $NetBSD: ibus_3max.c,v 1.1 1999/11/15 09:50:30 nisimura Exp $ */

/*
 * Copyright (c) 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ibus_3max.c,v 1.1 1999/11/15 09:50:30 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <dev/tc/tcvar.h>
#include <pmax/ibus/ibusvar.h>
#include <pmax/pmax/kn02.h>

#define KV(x) MIPS_PHYS_TO_KSEG1(x)

static struct ibus_attach_args kn02sys_devs[] = {
	{ "mc146818",	0, KV(KN02_SYS_CLOCK),	},
	{ "dc",  	1, KV(KN02_SYS_DZ),	},
};

static int  kn02sys_match __P((struct device *, struct cfdata *, void *));
static void kn02sys_attach __P((struct device *, struct device *, void *));

struct cfattach kn02sys_ca = {
        sizeof(struct ibus_softc), kn02sys_match, kn02sys_attach,
};
extern struct cfdriver tc_cd;

int
kn02sys_match(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;

{
	struct tc_attach_args *ta = aux;

	if (strncmp("KN02SYS ", ta->ta_modname, TC_ROM_LLEN) != 0)
		return 0;
	return 1;
}

void
kn02sys_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	struct ibus_dev_attach_args ida;
	struct tc_softc *sc = tc_cd.cd_devs[0];

	ida.ida_busname = "ibus";
	ida.ida_devs = kn02sys_devs;
	ida.ida_ndevs = sizeof(kn02sys_devs) / sizeof(kn02sys_devs[0]);
	ida.ida_establish = sc->sc_intr_establish;
	ida.ida_disestablish = sc->sc_intr_disestablish;

	ibusattach(parent, self, &ida);
}
