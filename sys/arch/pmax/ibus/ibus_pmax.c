/*	$NetBSD: ibus_pmax.c,v 1.1.2.1 1998/10/15 02:41:15 nisimura Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: ibus_pmax.c,v 1.1.2.1 1998/10/15 02:41:15 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <pmax/ibus/ibusvar.h>
#include <pmax/pmax/kn01.h>
#include <pmax/pmax/pmaxtype.h>

#include "opt_dec_3100.h"
#include "opt_dec_5100.h"

static int  ibus_pmax_match __P((struct device *, struct cfdata *, void *));
static void ibus_pmax_attach __P((struct device *, struct device *, void *));

struct cfattach ibus_pmax_ca = {
	sizeof(struct ibus_softc), ibus_pmax_match, ibus_pmax_attach,
};

#define KV(x)	MIPS_PHYS_TO_KSEG1(x)
#define	C(x)	(void *)(x)

static struct ibus_attach_args ibus_devs[] = {
	{ "mc146818",	KV(KN01_SYS_CLOCK),	C(SYS_DEV_BOGUS)	},
	{ "dc",		KV(KN01_SYS_DZ),	C(SYS_DEV_SCC0)		},
	{ "lance",	KV(KN01_SYS_LANCE),	C(SYS_DEV_LANCE)	},
	{ "sii",	KV(KN01_SYS_SII),	C(SYS_DEV_SCSI)		},
	{ "pm",		KV(KN01_PHYS_FBUF_START), C(SYS_DEV_BOGUS)	},
	{ "dc",  	KV(0x15000000),		C(SYS_DEV_SCC1)		},
	{ "dc",  	KV(0x15200000),		C(SYS_DEV_SCC2)		},
#ifdef notyet
	/*
	 * XXX Ultrix configures at 0x86400400. the first 0x400 byte are
	 * used for NVRAM state??
	 */
	{ "nvram",	KV(0x86400000),		C(SYS_DEV_BOGUS)	},
#endif
};

extern void dec_3100_intr_establish __P((struct device *, void *,
		int, int (*)(void *), void *));
extern void dec_3100_intr_disestablish __P((struct device *, void *));

extern void dec_5100_intr_establish __P((struct device *, void *,
		int, int (*)(void *), void *));
extern void dec_5100_intr_disestablish __P((struct device *, void *));

int ibus_attached = 0;

static int
ibus_pmax_match(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "baseboard") != 0)
		return 0;
	if (systype != DS_PMAX || systype != DS_MIPSMATE || ibus_attached)
		return 0;
	return 1;
}

static void
ibus_pmax_attach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
	struct ibus_dev_attach_args ibd;

	printf("\n");
	ibus_attached = 1;

	ibd.ibd_busname = "ibus"; /* XXX */
	ibd.ibd_devs = ibus_devs;
	ibd.ibd_ndevs = sizeof(ibus_devs) / sizeof(ibus_devs[0]);
#ifdef DEC_3100
	if (systype == DS_PMAX) {
		ibd.ibd_establish = dec_3100_intr_establish;
		ibd.ibd_disestablish = dec_3100_intr_disestablish;
	}
#endif
#ifdef DEC_5100
	if (systype == DS_MIPSMATE) {
		ibd.ibd_establish = dec_5100_intr_establish;
		ibd.ibd_disestablish = dec_5100_intr_disestablish;
	}
#endif
	ibus_devattach(self, &ibd);
}
