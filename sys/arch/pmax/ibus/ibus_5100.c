/*	$NetBSD: ibus_5100.c,v 1.5 1999/06/29 21:00:27 ad Exp $	*/

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

__KERNEL_RCSID(0, "$NetBSD: ibus_5100.c,v 1.5 1999/06/29 21:00:27 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <pmax/ibus/ibusvar.h>

#include <pmax/pmax/kn01.h>
#include <pmax/pmax/clockreg.h>
#include <pmax/pmax/pmaxtype.h>


int	dec_5100_ibusdev_match
	    __P((struct device *, struct cfdata *, void *));
void	dec_5100_ibusdev_attach
	    __P((struct device *, struct device *, void *));

extern struct cfattach kn01bus_ca;
struct cfattach kn230bus_ca = {
	sizeof(struct device), dec_5100_ibusdev_match, dec_5100_ibusdev_attach
};

extern ibus_intr_establish_t	dec_5100_intr_establish;
extern ibus_intr_disestablish_t	dec_5100_intr_disestablish;


#define KV(x) MIPS_PHYS_TO_KSEG1(x)

static struct ibus_attach_args kn230_devs[] = {
	/* name     cookie   addr			  */

#if 0	/* Be sure to avoid memory fault probing for pm */
	{ "pm",		0, KV(KN01_PHYS_FBUF_START)	},
#endif

	{ "dc",  	1, KV(KN01_SYS_DZ)		},
	{ "lance", 	2, KV(KN01_SYS_LANCE)		},
	{ "sii",	3, KV(KN01_SYS_SII)		},
	{ "mc146818",	4, KV(KN01_SYS_CLOCK),		},
	{ "dc",  	5, KV(0x15000000),		},
	{ "dc",  	6, KV(0x15200000),		},
#ifdef notyet
	/*
	 * XXX Ultrix configures at 0x86400400. the first 0x400 byte are
	 * used for NVRAM state??
	 */
	{ "nvram",	7, KV(0x86400000),		-1, },
#endif
};

int
dec_5100_ibusdev_match(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;

{
	if (systype == DS_MIPSMATE)
		return (1);
	return(0);
}


void
dec_5100_ibusdev_attach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
	struct ibus_dev_attach_args ibd;

	printf("\n");

	ibd.ibd_busname = "ibus";
	ibd.ibd_devs = kn230_devs;
	ibd.ibd_ndevs = sizeof(kn230_devs) / sizeof(kn230_devs[0]);
	ibd.ibd_establish = dec_5100_intr_establish;
	ibd.ibd_disestablish = dec_5100_intr_disestablish;
	config_found(self, &ibd, ibusprint);	/* XXX*/
}
