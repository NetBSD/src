/*	$NetBSD: kbc.c,v 1.11.2.1 2008/05/18 12:32:31 yamt Exp $	*/

/*-
 * Copyright (C) 2001 Izumi Tsutsui.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kbc.c,v 1.11.2.1 2008/05/18 12:32:31 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/cons.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/kbcvar.h>

#include "ioconf.h"

#define KBC_SIZE 0x10 /* XXX */

/* Definition of the driver for autoconfig. */
static int  kbc_match(device_t, cfdata_t, void *);
static void kbc_attach(device_t, device_t, void *);
static int  kbc_print(void *, const char *name);

CFATTACH_DECL_NEW(kbc, 0,
    kbc_match, kbc_attach, NULL, NULL);

static int kbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hb_attach_args *ha = aux;
	u_int addr;

	if (strcmp(ha->ha_name, "kbc"))
		return 0;

	/* XXX no default address */
	if (ha->ha_address == (u_int)-1)
		return 0;

	addr = IIOV(ha->ha_address); /* XXX */

	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

static void
kbc_attach(device_t parent, device_t self, void *aux)
{
	struct hb_attach_args *ha = aux;
	struct kbc_attach_args ka;
	bus_space_tag_t bt = ha->ha_bust;
	bus_space_handle_t bh;

	if (bus_space_map(bt, ha->ha_address, KBC_SIZE, 0, &bh) != 0) {
		aprint_error(": can't map device space\n");
		return;
	}

	aprint_normal("\n");

	ka.ka_bt = bt;
	ka.ka_bh = bh;
	ka.ka_ipl = ha->ha_ipl;

	if (ka.ka_ipl == -1)
		ka.ka_ipl = KBC_PRI;

	ka.ka_name = "kb";
	config_found(self, (void *)&ka, kbc_print);

	ka.ka_name = "ms";
	config_found(self, (void *)&ka, kbc_print);
}

static int
kbc_print(void *aux, const char *name)
{

	if (name != NULL)
		aprint_normal("%s: ", name);

	return UNCONF;
}
