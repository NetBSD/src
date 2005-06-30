/*	$NetBSD: mainbus.c,v 1.16 2005/06/30 17:03:54 drochner Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.16 2005/06/30 17:03:54 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cpuregs.h>

#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include "locators.h"

static int	mainbus_match(struct device *, struct cfdata *, void *);
static void	mainbus_attach(struct device *, struct device *, void *);
static int	mainbus_search(struct device *, struct cfdata *,
			       const locdesc_t *, void *);
int		mainbus_print(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

static int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mainbus_attach_args ma;

	printf(": %s [%s, %s], %d processor%s", arcbios_system_identifier,
	    arcbios_sysid_vendor, arcbios_sysid_product,
	    ncpus, ncpus == 1 ? "" : "s");

	printf("\n");

	config_search_ia(mainbus_search, self, "mainbus", &ma);
}

static int
mainbus_search(struct device *parent, struct cfdata *cf,
	       const locdesc_t *ldesc, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	do {
		ma->ma_name = NULL;
		ma->ma_addr = cf->cf_loc[MAINBUSCF_ADDR];
		ma->ma_iot = 0;
		ma->ma_ioh = MIPS_PHYS_TO_KSEG1(ma->ma_addr);
		if (config_match(parent, cf, ma) > 0)
			config_attach(parent, cf, ma, mainbus_print);
	} while (cf->cf_fstate == FSTATE_STAR);

	return 0;
}

int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma = aux;

	if (pnp != 0)
		return QUIET;

	if (ma->ma_addr != (u_long) MAINBUSCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%lx", ma->ma_addr);

	return UNCONF;
}
