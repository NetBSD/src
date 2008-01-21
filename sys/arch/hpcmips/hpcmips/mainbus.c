/*	$NetBSD: mainbus.c,v 1.26.2.2 2008/01/21 09:36:37 yamt Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.26.2.2 2008/01/21 09:36:37 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <machine/autoconf.h>
#include <machine/platid.h>
#include <machine/bus_space_hpcmips.h>

#include "locators.h"

#ifdef DEBUG
#define STATIC
#else
#define STATIC	static
#endif

STATIC int mainbus_match(struct device *, struct cfdata *, void *);
STATIC void mainbus_attach(struct device *, struct device *, void *);
STATIC int mainbus_search(struct device *, struct cfdata *,
			  const int *, void *);
STATIC int mainbus_print(void *, const char *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

STATIC int __mainbus_attached;

int
mainbus_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (__mainbus_attached ? 0 : 1);	/* don't attach twice */
}

void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	static const char *devnames[] = {	/* ATTACH ORDER */
		"cpu",				/* 1. CPU */
		"vrip", "vr4102ip", "vr4122ip",
		"vr4181ip",			/* 2. System BUS */
		"txsim",			
		"bivideo", "btnmgr", 		/* 3. misc */
	};
	struct mainbus_attach_args ma;
	int i;

	__mainbus_attached = 1;
	
	printf("\n");

	/* system bus_space */
	ma.ma_iot = hpcmips_system_bus_space();
	hpcmips_init_bus_space((struct bus_space_tag_hpcmips *)ma.ma_iot,
	    NULL, "main bus", 0, 0xffffffff);


	/* search and attach devices in order */
	for (i = 0; i < sizeof(devnames) / sizeof(devnames[0]); i++) {
		ma.ma_name = devnames[i];
		config_search_ia(mainbus_search, self, "mainbus", &ma);
	}

	/* APM */
	config_found_ia(self, "hpcapmif", NULL, mainbus_print);
}

int
mainbus_search(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct mainbus_attach_args *ma = (void *)aux;
	int locator = cf->cf_loc[MAINBUSCF_PLATFORM];

	/* check device name */
	if (strcmp(ma->ma_name, cf->cf_name) != 0)
		return (0);

	/* check platform ID in config file */
	if (locator != MAINBUSCF_PLATFORM_DEFAULT &&
	    !platid_match(&platid, PLATID_DEREFP(locator)))
		return (0);

	/* attach device */
	if (config_match(parent, cf, ma))
		config_attach(parent, cf, ma, mainbus_print);

	return (0);
}

int
mainbus_print(void *aux, const char *pnp)
{

	return (pnp ? QUIET : UNCONF);
}

