/*	$NetBSD: external.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: external.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/epoc32.h>

extern struct bus_space external_bs_tag;

static int external_match(device_t, cfdata_t, void *);
static void external_attach(device_t parent, device_t self, void *aux);

static int external_print(void *, const char *);

CFATTACH_DECL_NEW(external, 0, external_match, external_attach, NULL, NULL);

static const struct {
	const char *name;
	bus_addr_t addr;
	bus_addr_t addr2;
	int irq;
} knowndevs[] = {
	{ "etna",	0x20000000, 0x40000000, 5 },	/* 5mx */
	{ "etna",	0x50000000, 0x30000000, 5 },	/* 5mx pro */
};

/* ARGSUSED */
static int
external_match(device_t parent, cfdata_t match, void *aux)
{
	/* always attach */
	return 1;
}

/* ARGSUSED */
static void
external_attach(device_t parent, device_t self, void *aux)
{
	struct external_attach_args aa;
	int i;

	aprint_naive("\n");
	aprint_normal("\n");

	for (i = 0; i < __arraycount(knowndevs); i++) {
		aa.name = knowndevs[i].name;
		aa.iot = &external_bs_tag;
		aa.addr = knowndevs[i].addr;
		aa.addr2 = knowndevs[i].addr2;
		aa.irq = knowndevs[i].irq;
		config_found_sm_loc(self, "external", NULL, &aa,
		    external_print, NULL);
	}
}

static int
external_print(void *aux, const char *pnp)
{
	struct external_attach_args *aa = aux;

	if (pnp)
		return QUIET;

	if (aa->addr != -1)
		aprint_normal(" addr 0x%04lx", aa->addr);
	if (aa->addr2 != -1)
		aprint_normal(",0x%04lx", aa->addr2);
	if (aa->irq != -1)
		aprint_normal(" irq %d", aa->irq);

	return UNCONF;
}
