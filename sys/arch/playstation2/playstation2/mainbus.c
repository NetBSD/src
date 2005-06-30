/*	$NetBSD: mainbus.c,v 1.7 2005/06/30 17:03:53 drochner Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.7 2005/06/30 17:03:53 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

static int mainbus_match(struct device *, struct cfdata *, void *);
static void mainbus_attach(struct device *, struct device *, void *);
static int mainbus_search(struct device *, struct cfdata *,
			  const locdesc_t *, void *);
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
	       const locdesc_t *ldesc, void *aux)
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
