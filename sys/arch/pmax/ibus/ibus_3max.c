/*	$NetBSD: ibus_3max.c,v 1.15.12.1 2017/12/03 11:36:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
__KERNEL_RCSID(0, "$NetBSD: ibus_3max.c,v 1.15.12.1 2017/12/03 11:36:35 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/tc/tcvar.h>

#include <pmax/ibus/ibusvar.h>
#include <pmax/pmax/kn02.h>

static struct ibus_attach_args kn02sys_devs[] = {
	{ "mc146818",	SYS_DEV_BOGUS,	KV(KN02_SYS_CLOCK),	0 },
	{ "dc",  	SYS_DEV_SCC0,	KV(KN02_SYS_DZ),	0 },
};

static int	kn02sys_match(device_t, cfdata_t, void *);
static void	kn02sys_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(kn02sys, 0,
    kn02sys_match, kn02sys_attach, NULL, NULL);

static int
kn02sys_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tc_attach_args *ta = aux;

	return (strncmp("KN02SYS ", ta->ta_modname, TC_ROM_LLEN) == 0);
}

static void
kn02sys_attach(device_t parent, device_t self, void *aux)
{
	struct ibus_dev_attach_args ida;

	ida.ida_busname = "ibus";
	ida.ida_devs = kn02sys_devs;
	ida.ida_ndevs = __arraycount(kn02sys_devs);
	ida.ida_memt = normal_memt;

	ibusattach(parent, self, &ida);
}
