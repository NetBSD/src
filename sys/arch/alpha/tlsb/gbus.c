/* $NetBSD: gbus.c,v 1.4.8.2 1997/04/07 23:41:06 cgd Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Autoconfiguration and support routines for the Gbus: the internal
 * bus on AlphaServer CPU modules.
 */

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: gbus.c,v 1.4.8.2 1997/04/07 23:41:06 cgd Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/tlsb/tlsbreg.h>
#include <alpha/tlsb/tlsbvar.h>

extern int	cputype;

#define KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))

struct gbus_softc {
	struct device	sc_dev;
	int		sc_tlsbnode;	/* node on the TurboLaser */
};

static int	gbusmatch __P((struct device *, struct cfdata *, void *));
static void	gbusattach __P((struct device *, struct device *, void *));
struct cfattach gbus_ca = {
	sizeof(struct gbus_softc), gbusmatch, gbusattach
};

struct cfdriver gbus_cd = {
	NULL, "gbus", DV_DULL,
};

static int	gbusprint __P((void *, const char *));

void	gbus_intr_establish __P((struct confargs *, int (*)(void *), void *));
void	gbus_intr_disestablish __P((struct confargs *));
caddr_t	gbus_cvtaddr __P((struct confargs *));
int	gbus_matchname __P((struct confargs *, char *));

static int
gbusprint(aux, cp)
	void *aux;
	const char *cp;
{
	return (QUIET);
}

static int
gbusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct tlsb_dev_attach_args *ta = aux;
	extern struct cfdriver tlsb_cd;

	/*
	 * Make sure we're looking for a Gbus.
	 * Right now, only Gbus could be a
	 * child of a TLSB CPU Node.
	 */
	if (TLDEV_ISCPU(ta->ta_dtype) &&
	    parent->dv_cfdata->cf_driver == &tlsb_cd)
		return (1);

	return (0);
}

static void
gbusattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct confargs nca;
	struct gbus_softc *sc = (struct gbus_softc *)self;
	struct tlsb_dev_attach_args *ta = aux;

	printf("\n");

	sc->sc_dev = *self;
	sc->sc_tlsbnode = ta->ta_node;

	/* Attach the clock. */
	nca.ca_name = "mcclock";
	nca.ca_slot = -1;
	nca.ca_offset = 0x20000000;
	nca.ca_bus = NULL;
	if (!config_found(self, &nca, gbusprint))
		printf("no clock on %s\n", self->dv_xname);
}
