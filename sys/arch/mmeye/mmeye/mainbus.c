/*	$NetBSD: mainbus.c,v 1.8.2.1 2011/06/23 14:19:23 cherry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.8.2.1 2011/06/23 14:19:23 cherry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/autoconf.h>

#include "locators.h"

static int mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);
static int mainbus_search(device_t, cfdata_t, const int *, void *);
static int mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args maa;

	aprint_naive("\n");
	aprint_normal("\n");

	maa.ma_addr1 = MAINBUSCF_ADDR1_DEFAULT;
	maa.ma_irq1 = MAINBUSCF_IRQ1_DEFAULT;

	/* CPU and SHBus */
	maa.ma_name = "cpu";
	config_found_ia(self, "mainbus", &maa, mainbus_print);
	maa.ma_name = "shb";
	config_found_ia(self, "mainbus", &maa, mainbus_print);

	/* Devices */
	config_search_ia(mainbus_search, self, "mainbus", 0);
}

static int
mainbus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args maa;

	if (strcmp(cf->cf_name, "cpu") == 0 ||
	    strcmp(cf->cf_name, "shb") == 0)
		return 0;

	maa.ma_name = cf->cf_name;
	maa.ma_addr1 = cf->cf_loc[MAINBUSCF_ADDR1];
	maa.ma_addr2 = cf->cf_loc[MAINBUSCF_ADDR2];
	maa.ma_irq1 = cf->cf_loc[MAINBUSCF_IRQ1];
	maa.ma_irq2 = cf->cf_loc[MAINBUSCF_IRQ2];

	if (config_match(parent, cf, &maa))
		config_attach(parent, cf, &maa, mainbus_print);

	return 0;
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *maa = aux;

	if (maa->ma_addr1 != MAINBUSCF_ADDR1_DEFAULT) {
		aprint_normal(" addr 0x%lx", maa->ma_addr1);
		if (maa->ma_addr2 != MAINBUSCF_ADDR2_DEFAULT)
			aprint_normal(",0x%lx", maa->ma_addr2);
	}
	if (maa->ma_irq1 != MAINBUSCF_IRQ1_DEFAULT) {
		aprint_normal(" irq %d", maa->ma_irq1);
		if (maa->ma_irq2 != MAINBUSCF_IRQ2_DEFAULT)
			aprint_normal(",%d", maa->ma_irq2);
	}
	return pnp ? QUIET : UNCONF;
}
