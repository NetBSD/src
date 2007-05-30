/* $NetBSD: eumb.c,v 1.1.2.1 2007/05/30 17:02:50 nisimura Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eumb.c,v 1.1.2.1 2007/05/30 17:02:50 nisimura Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/tty.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <sandpoint/sandpoint/eumbvar.h>
#include "locators.h"

static int  eumb_match(struct device *, struct cfdata *, void *);
static void eumb_attach(struct device *, struct device *, void *);
static int  eumb_print(void *, const char *);
static int  eumb_search(struct device *, struct cfdata *, const int *, void *);

CFATTACH_DECL(eumb, sizeof(struct device),
    eumb_match, eumb_attach, NULL, NULL);

extern struct cfdriver eumb_cd;

static int
eumb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	const char **ca_name = aux; /* XXX */

	if (strcmp(*ca_name, eumb_cd.cd_name) != 0)
		return (0);
	return (1);
}

static void
eumb_attach(struct device *parent, struct device *self, void *aux)
{

	printf("\n");
	config_search_ia(eumb_search, self, "eumb", NULL);
}

static int
eumb_search(struct device *parent, struct cfdata *cf,
    const int *ldesc, void *aux)
{
	struct eumb_attach_args eaa;

	eaa.eumb_name = cf->cf_name;
	eaa.eumb_bt = &sandpoint_eumb_space_tag;
	eaa.eumb_unit = cf->cf_loc[EUMBCF_UNIT];
        if (config_match(parent, cf, &eaa) > 0)
                config_attach(parent, cf, &eaa, eumb_print);

	return (0);
}

static int
eumb_print(void *aux, const char *pnp)
{
	struct eumb_attach_args *eaa = aux;

	if (pnp)
		printf("%s at %s", eaa->eumb_name, pnp);
	if (eaa->eumb_unit != EUMBCF_UNIT_DEFAULT)
		printf(" unit %d", eaa->eumb_unit);
	return (UNCONF);
}
