/*	$NetBSD: ibus_3100.c,v 1.4 1999/04/24 08:01:09 simonb Exp $	*/


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

__KERNEL_RCSID(0, "$NetBSD: ibus_3100.c,v 1.4 1999/04/24 08:01:09 simonb Exp $");

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


int	dec_3100_ibusdev_match
	    __P((struct device *, struct cfdata *, void *));
void	dec_3100_ibusdev_attach
	    __P((struct device *, struct device *, void *));

extern struct cfattach kn01bus_ca;
struct cfattach kn01bus_ca = {
	sizeof(struct device), dec_3100_ibusdev_match, dec_3100_ibusdev_attach
};

extern ibus_intr_establish_t	dec_3100_intr_establish;
extern ibus_intr_disestablish_t	dec_3100_intr_disestablish;


#define KV(x) MIPS_PHYS_TO_KSEG1(x)

static struct ibus_attach_args kn01_devs[] = {
	/* name     cookie   addr			  */
	{ "pm",		0, KV(KN01_PHYS_FBUF_START)	},
	{ "dc",  	1, KV(KN01_SYS_DZ)		},
	{ "lance", 	2, KV(KN01_SYS_LANCE)		},
	{ "sii",	3, KV(KN01_SYS_SII)		},
	{ "mc146818",	4, KV(KN01_SYS_CLOCK)		},
};

int kn01_attached = 0;

int
dec_3100_ibusdev_match(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;

{

	if (kn01_attached)
		return 0;
	if (systype != DS_PMAX)
		return (0);
	return(1);
}


void
dec_3100_ibusdev_attach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
	struct ibus_dev_attach_args ibd;

	kn01_attached = 1;

	ibd.ibd_busname = "ibus";
	ibd.ibd_devs = kn01_devs;
	ibd.ibd_ndevs = sizeof(kn01_devs) / sizeof(kn01_devs[0]);
	ibd.ibd_establish = dec_3100_intr_establish;
	ibd.ibd_disestablish = dec_3100_intr_disestablish;
	config_found(self, &ibd, ibusprint);	/* XXX*/
}
