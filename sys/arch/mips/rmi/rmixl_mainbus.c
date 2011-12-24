/*	$NetBSD: rmixl_mainbus.c,v 1.1.2.7 2011/12/24 01:57:54 matt Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * mainbus.c
 *
 * mainbus configuration
 *
 * Created      : 15/12/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_mainbus.c,v 1.1.2.7 2011/12/24 01:57:54 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <evbmips/rmixl/autoconf.h>
#include <mips/rmi/rmixlvar.h>
#include "locators.h"

static int  mainbusmatch(device_t, cfdata_t, void *);
static void mainbusattach(device_t, device_t, void *);
static int  mainbus_search(device_t, cfdata_t, const int *, void *);
static int  mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
	mainbusmatch, mainbusattach, NULL, NULL);

static bool mainbus_found;

static int
mainbusmatch(device_t parent, cfdata_t cf, void *aux)
{
	if (mainbus_found)
		return 0;
	return 1;
}

static void
mainbusattach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct mainbus_attach_args ma;

	aprint_naive("\n");
	aprint_normal("\n");

	mainbus_found = true;

	/*
	 * Attach first cpunode!
	 */
	ma.ma_node = 0;
	ma.ma_dmat29 = rcp->rc_dmat29;
	ma.ma_dmat32 = rcp->rc_dmat32;
	ma.ma_dmat64 = rcp->rc_dmat64;

	config_found_ia(self, "mainbus", &ma, mainbus_print);

	/*
	 * attach mainbus devices 
	 */
        config_search_ia(mainbus_search, self, "mainbus", mainbus_print);
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma = aux;

	if (pnp != NULL)
		aprint_normal("%s:", pnp);
	if (ma->ma_node != MAINBUSCF_NODE_DEFAULT)
		aprint_normal(" node %d", ma->ma_node);

	return (UNCONF);
}

static int
mainbus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct mainbus_attach_args ma;

	ma.ma_node = cf->cf_loc[MAINBUSCF_NODE];
	ma.ma_dmat29 = rcp->rc_dmat29;
	ma.ma_dmat32 = rcp->rc_dmat32;
	ma.ma_dmat64 = rcp->rc_dmat64;

	if (config_match(parent, cf, &ma) > 0)
		config_attach(parent, cf, &ma, mainbus_print);

	return 0;
}
