/*	$NetBSD: mainbus.c,v 1.2 2003/03/07 18:24:01 matt Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
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

#include "mainbus.h"
#include "pci.h"
#include "opt_multiprocessor.h"
#include "opt_pci.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <dev/marvell/gtvar.h>

#if NCPU == 0
#error	A cpu device is now required
#endif

struct	mainbus_attach_args {
	const char *mba_busname;
	int mba_unit;
};

int	mainbus_cfprint(void *, const char *);
int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
	mainbus_match, mainbus_attach, NULL, NULL);

int
mainbus_cfprint(void *aux, const char *pnp)
{
	struct mainbus_attach_args *mba = aux;

	if (pnp)
		printf("%s at %s", mba->mba_busname, pnp);
	return (UNCONF);
}


/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args mba;

	printf("\n");

	memset(&mba, 0, sizeof(mba));

	/*
	 * Always find the CPU
	 */
	mba.mba_busname = "cpu";
	mba.mba_unit = 0;
	config_found(self, &mba, mainbus_cfprint);

#ifdef MULTIPROCESSOR
	/*
	 * Try for a second one...
	 */
	mba.mba_busname = "cpu";
	mba.mba_unit = 1;
	config_found(self, &mba, mainbus_cfprint);
#endif

	/*
	 * Now try to configure the Discovery
	 */
	mba.mba_busname = "gt";
	mba.mba_unit = 1;
	config_found(self, &mba, mainbus_cfprint);
}

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct device), cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *mba = aux;

	if (strcmp(mba->mba_busname, cpu_cd.cd_name) != 0)
		return 0;

	if (cpu_info[0].ci_dev != NULL)
		return 0;

	return 1;
}

void
cpu_attach(struct device *parent, struct device *self, void *aux)
{
	(void) cpu_attach_common(self, 0);
}
