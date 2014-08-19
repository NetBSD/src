/*	$NetBSD: mainbus.c,v 1.12.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.12.6.2 2014/08/20 00:03:18 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

static int mainbus_match(struct device *, struct cfdata *, void *);
static void mainbus_attach(struct device *, struct device *, void *);
static int mainbus_search(struct device *, struct cfdata *,
			  const int *, void *);
static int mainbus_print(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

static int
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

static void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{

	printf("\n");

	/* attach CPU first */
	config_found(self, &(struct mainbus_attach_args){.ma_name = "cpu"},
	    mainbus_print);

	config_search_ia(mainbus_search, self, "mainbus", 0);
}

static int
mainbus_search(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct mainbus_attach_args ma;

	ma.ma_name = cf->cf_name;
	if (config_match(parent, cf, &ma))
		config_attach(parent, cf, &ma, mainbus_print);
	
	return (0);
}

static int
mainbus_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}
