/*	$NetBSD: obio.c,v 1.1 1994/08/24 09:16:46 deraadt Exp $	*/

/*
 * Copyright (c) 1993, 1994 Theo de Raadt
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
 *	This product includes software developed by Theo de Raadt.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>

struct vme_attach_args {
	u_long	paddr;
	int	irq;
};

struct obio_softc {
	struct	device sc_dev;		/* base device */
	int	nothing;
};

/* autoconfiguration driver */
static int	obiomatch(struct device *, struct cfdata *, void *);
static void	obioattach(struct device *, struct device *, void *);
struct cfdriver obiocd = { NULL, "cgsix", obiomatch, obioattach,
	DV_DULL, sizeof(struct obio_softc)
};

int
obiomatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
printf("obiomatch\n");
	if (cputyp != CPU_SUN4)
		return 0;
	return 1;
}

int
obio_print(args, obio)
	void *args;
	char *obio;
{
	printf("obio_print ");
	return (UNCONF);
}

void
obioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	extern struct cfdata cfdata[];

	register struct obio_softc *sc = (struct obio_softc *)self;
	struct vme_attach_args va;
	register short *p;
	struct cfdata *cf;

	if (sc->sc_dev.dv_unit > 0) {
		printf(" unsupported\n");
		return;
	}

	for (cf = cfdata; cf->cf_driver; cf++) {
		if (cf->cf_fstate == FSTATE_FOUND)
			continue;
		for (p = cf->cf_parents; *p >= 0; p++)
			if (parent->dv_cfdata == &cfdata[*p]) {
				va.paddr = cf->cf_loc[0];
				va.irq = cf->cf_loc[1];
				if ((*cf->cf_driver->cd_match)(self, cf, &va))
					config_attach(self, cf, &va, NULL);
			}
	}
}
