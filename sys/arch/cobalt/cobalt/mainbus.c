/*	$NetBSD: mainbus.c,v 1.2 2000/03/31 14:51:50 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

#include <mips/cpuregs.h>

#include <machine/autoconf.h>

#include "locators.h"
#include "pci.h"

static int	mainbus_match(struct device *, struct cfdata *, void *);
static void	mainbus_attach(struct device *, struct device *, void *);
static int	mainbus_search(struct device *, struct cfdata *, void *);
int		mainbus_print(void *, const char *);

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

static int
mainbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
mainbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	/*
	 * XXX Check for Qube/RaQ 1 vs. 2.
	 */

	printf("\n");
	
	config_search(mainbus_search, self, ma);
}

static int
mainbus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf; 
	void *aux;
{ 
	struct mainbus_attach_args *ma = aux;

	do {
		ma->ma_addr = cf->cf_loc[MAINBUSCF_ADDR];
		ma->ma_iot = 0;
		ma->ma_ioh = MIPS_PHYS_TO_KSEG1(ma->ma_addr);
		ma->ma_level = cf->cf_loc[MAINBUSCF_LEVEL];
		if ((*cf->cf_attach->ca_match)(parent, cf, ma) > 0)
			config_attach(parent, cf, ma, mainbus_print);
	} while (cf->cf_fstate == FSTATE_STAR);

	return 0;
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct mainbus_attach_args *ma = aux;

	if (pnp != 0)
		return QUIET;

	if (ma->ma_addr != MAINBUSCF_ADDR_DEFAULT)
		printf(" addr 0x%lx", ma->ma_addr);
	if (ma->ma_level != MAINBUSCF_LEVEL_DEFAULT)
		printf(" level %d", ma->ma_level);

	return UNCONF;
}
