/*	$NetBSD: ebus.c,v 1.1 2003/08/19 10:51:57 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <pdp10/dev/ebus.h>

static int
ebus_print(void *aux, const char *n)
{
	struct ebus_attach_args *ea = aux;

	printf(" csr %o", ea->ea_ioh);
	return UNCONF;
}

static int
ebus_match(struct device *parent, struct cfdata *cf, void *aux)
{
#ifdef notyet
	if (cputype != TYPE_KL)
		return 0;
#endif
	return 1;
}

static int
ebus_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args ea;
	int rv;

	ea.ea_iot = 0;
	ea.ea_ioh = cf->cf_loc[0];
	rv = config_match(parent, cf, &ea);
	if (rv == 0)
		return 0;
	config_attach(parent, cf, &ea, ebus_print);
	return 1;
}

static void
ebus_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");

	config_search(ebus_search, self, NULL);
}

struct cfattach ebus_ca = {
	sizeof(struct device), ebus_match, ebus_attach,
};
