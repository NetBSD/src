/* $NetBSD: mainbus.c,v 1.1 2000/05/09 21:56:02 bjh21 Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * mainbus.c - the root device
 */

#include <sys/param.h>

#include <sys/device.h>
#include <sys/systm.h>

static int mainbus_match __P((struct device *parent, struct cfdata *cf, void *aux));
static void mainbus_attach __P((struct device *parent, struct device *self, void *aux));
static int mainbus_search __P((struct device *parent, struct cfdata *cf, void *aux));
static int mainbus_print __P((void *aux, const char *pnp));

struct mainbus_softc {
	struct device	sc_dev;
};

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mainbus_match, mainbus_attach
};

extern struct cfdriver mainbus_cd;

static int
mainbus_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* Only one mainbus */
	if (cf->cf_unit == 0)
		return 1;
	else
		return 0;
}

static void
mainbus_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{

	printf("\n");

	config_search(mainbus_search, self, NULL);
}

static int
mainbus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	
	/*
	 * Things attached at mainbus are assumed not to need to be
	 * told where they are.
	 */

	if ((cf->cf_attach->ca_match)(parent, cf, NULL) > 0)
		config_attach(parent, cf, NULL, mainbus_print);

	return 0;
}

static int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{

	return UNCONF;
}
