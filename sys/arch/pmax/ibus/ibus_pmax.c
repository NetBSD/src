/* $NetBSD: ibus_pmax.c,v 1.8 1999/11/24 00:18:37 simonb Exp $ */

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ibus_pmax.c,v 1.8 1999/11/24 00:18:37 simonb Exp $");

#include "opt_dec_3100.h"
#include "opt_dec_5100.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <pmax/ibus/ibusvar.h>
#include <machine/autoconf.h>
#include <pmax/pmax/kn01.h>
#include <pmax/pmax/pmaxtype.h>


static int  ibus_pmax_match __P((struct device *, struct cfdata *, void *));
static void ibus_pmax_attach __P((struct device *, struct device *, void *));

struct cfattach ibus_pmax_ca = {
	sizeof(struct ibus_softc), ibus_pmax_match, ibus_pmax_attach
};

#define KV(x) MIPS_PHYS_TO_KSEG1(x)

#ifdef DEC_3100
struct ibus_attach_args ibus_pmax_devs[] = {
	{ "pm",		0,	KV(KN01_PHYS_FBUF_START),	0	},
	{ "dc",		1,	KV(KN01_SYS_DZ),		0	},
	{ "lance",	2,	KV(KN01_SYS_LANCE),		0	},
	{ "sii",	3,	KV(KN01_SYS_SII),		0	},
	{ "mc146818",	4,	KV(KN01_SYS_CLOCK),		0	},
};
const int ibus_pmax_ndevs = sizeof(ibus_pmax_devs) / sizeof(ibus_pmax_devs[0]);
#endif /* DEC_3100 */

#ifdef DEC_5100
struct ibus_attach_args ibus_mipsmate_devs[] = {
	{ "dc",		1,	KV(KN01_SYS_DZ),		0	},
	{ "lance",	2,	KV(KN01_SYS_LANCE),		0	},
	{ "sii",	3,	KV(KN01_SYS_SII),		0	},
	{ "mc146818",	4,	KV(KN01_SYS_CLOCK),		0	},
	{ "dc",		5,	KV(0x15000000),			2	},
	{ "dc",		6,	KV(0x15200000),			2	},
#if 0
	/*
	 * Ultrix configures it at 0x86400400.  The first 0x400 bytes
	 * used for NVRAM state??
	 */
	{ "nvram",	7,	KV(0x86400000),			0	},
#endif
};
const int ibus_mipsmate_ndevs =
	sizeof(ibus_mipsmate_devs) / sizeof(ibus_mipsmate_devs[0]);
#endif /* DEC_5100 */

static int ibus_attached;

int
ibus_pmax_match(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (ibus_attached)
		return 0;
	if (systype != DS_PMAX && systype != DS_MIPSMATE)
		return 0;
	if (strcmp(ma->ma_name, "baseboard") != 0)
		return 0;

	return 1;
}

void
ibus_pmax_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
{
	struct ibus_dev_attach_args ida;

	ibus_attached = 1;

	ida.ida_busname = "ibus";
	switch (systype) {
#ifdef DEC_3100
	case DS_PMAX:
		ida.ida_devs = ibus_pmax_devs;
		ida.ida_ndevs = ibus_pmax_ndevs;
		ida.ida_establish = dec_3100_intr_establish;
		ida.ida_disestablish = dec_3100_intr_disestablish;
		break;
#endif
#ifdef DEC_5100
	case DS_MIPSMATE:
		ida.ida_devs = ibus_mipsmate_devs;
		ida.ida_ndevs = ibus_mipsmate_ndevs;
		ida.ida_establish = dec_5100_intr_establish;
		ida.ida_disestablish = dec_5100_intr_disestablish;
		break;
#endif
	default:
		panic("ibus_pmax_attach: no ibus configured for systype = %d", systype);
	}

	ibusattach(parent, self, &ida);
}
