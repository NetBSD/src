/*	$NetBSD: obio.c,v 1.16.4.1 2011/06/12 00:24:01 rmind Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.16.4.1 2011/06/12 00:24:01 rmind Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/mainboard.h>
#include <machine/bus.h>
#include <machine/sysconf.h>

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_search(device_t, cfdata_t, const int *, void *);
static int	obio_print(void *, const char *);
static void	obio_intr_establish(bus_space_tag_t, int, int, int,
		    int (*)(void *), void *);

CFATTACH_DECL_NEW(obio, 0,
    obio_match, obio_attach, NULL, NULL);

extern struct cfdriver obio_cd;

struct mipsco_bus_space obio_bustag;
struct mipsco_bus_dma_tag obio_dmatag;
 
static int
obio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, obio_cd.cd_name) != 0)
		return 0;

	return 1;
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;

	/* PIZAZZ (Mips 3000 Magnum)  Address Map */
	mipsco_bus_space_init(&obio_bustag, "obio", 
			      0x18000000, 0xb8000000,
			      0xb8000000, 0x08000000);

	_bus_dma_tag_init(&obio_dmatag);
	obio_bustag.bs_intr_establish = obio_intr_establish; /* XXX */

	ca->ca_bustag = &obio_bustag;
	ca->ca_dmatag = &obio_dmatag;

	printf("\n");
	config_search_ia(obio_search, self, "obio", ca);
}

static int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct confargs *ca = aux;

	ca->ca_addr = cf->cf_addr;
	ca->ca_name = cf->cf_name;

	if (config_match(parent, cf, ca) != 0)
		config_attach(parent, cf, ca, obio_print);

	return 0;
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
obio_print(void *args, const char *name)
{
	struct confargs *ca = args;

	if (name)
		return(QUIET);

	if (ca->ca_addr != -1)
		aprint_normal(" addr 0x%x", ca->ca_addr);

	return(UNCONF);
}

void
obio_intr_establish(bus_space_tag_t bst, int level, int pri, int flags,
	int (*func)(void *), void *arg)
{
	(*platform.intr_establish)(level, func, arg);
}
