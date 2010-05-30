/*	$NetBSD: mainbus.c,v 1.7.22.1 2010/05/30 05:16:47 rmind Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.7.22.1 2010/05/30 05:16:47 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

static int mainbus_match(struct device *, struct cfdata *, void *);
static void mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

static int mainbus_search(device_t, cfdata_t, const int *, void *);
static int mainbus_print(void *, const char *);

static int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{

	return (1);
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args maa;

	aprint_naive("\n");
	aprint_normal("\n");

	/* CPU  */
	memset(&maa, 0, sizeof(maa));
	maa.ma_name = "cpu";
	config_found_ia(self, "mainbus", &maa, mainbus_print);

	/* Devices */
	config_search_ia(mainbus_search, self, "mainbus", NULL);
}

static int
mainbus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args maa;

	if (strcmp(cf->cf_name, "cpu") == 0)
		return 0;

	maa.ma_name = cf->cf_name;

	if (config_match(parent, cf, &maa))
		config_attach(parent, cf, &maa, mainbus_print);

	return 0;
}

static int
mainbus_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}
