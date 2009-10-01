/*	$NetBSD: autoconf.c,v 1.3 2009/10/01 15:21:38 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.3 2009/10/01 15:21:38 pooka Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>

static int	mainbus_match(struct device *, struct cfdata *, void *);
static void	mainbus_attach(struct device *, struct device *, void *);
static int	mainbus_search(struct device *, struct cfdata *,
			       const int *, void *);

struct mainbus_softc {
	int mb_nada;
};

static const struct cfiattrdata mainbus_iattrdata = {
        "mainbus", 0, {
		{ NULL, NULL, 0 },
	}
};
static const struct cfiattrdata * const mainbus_attrs[] = {
	&mainbus_iattrdata,
	NULL,
};

CFDRIVER_DECL(mainbus, DV_DULL, mainbus_attrs);
CFATTACH_DECL_NEW(mainbus, sizeof(struct mainbus_softc),
	mainbus_match, mainbus_attach, NULL, NULL);

struct cfdriver * const cfdriver_list_initial[] = {
	&mainbus_cd,
	NULL
};

static struct cfattach * const mainbus_cfattachinit[] = {
	&mainbus_ca, NULL
};
const struct cfattachinit cfattachinit[] = {
	{ "mainbus", mainbus_cfattachinit },
	{ NULL, NULL },
};

struct cfdata cfdata[] = {
	{ "mainbus", "mainbus", 0, FSTATE_NOTFOUND, NULL, 0, NULL},
};

const short cfroots[] = {
	0, /* mainbus */
	-1
};

/* actually used */
#define MAXPDEVS 256
struct pdevinit pdevinit[MAXPDEVS] = {{NULL,0}, }; /* XXX: static limit */
static int pdev_total = 0;

#include "rump_dev_private.h"

void
rump_pdev_add(void (*pdev_attach)(int), int pdev_count)
{
	struct pdevinit *pdev_new;

	KASSERT(cold);

	pdev_new = &pdevinit[pdev_total];
	pdev_new->pdev_attach = pdev_attach;
	pdev_new->pdev_count = pdev_count;

	pdev_total++;
	KASSERT(pdev_total < MAXPDEVS);
}

void
rump_pdev_finalize()
{

	rump_pdev_add(NULL, 0);
}

int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{

	aprint_normal("\n");
	config_search_ia(mainbus_search, self, "mainbus", NULL);
}

static int
mainbus_search(struct device *parent, struct cfdata *cf,
	const int *ldesc, void *aux)
{

	if (config_match(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, NULL);

	return 0;
}
