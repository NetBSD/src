/*	$NetBSD: hb.c,v 1.19.100.3 2021/04/03 01:57:14 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 Izumi Tsutsui.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: hb.c,v 1.19.100.3 2021/04/03 01:57:14 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <news68k/news68k/isr.h>
#include <news68k/dev/hbvar.h>

#include "ioconf.h"

static int  hb_match(device_t, cfdata_t, void *);
static void hb_attach(device_t, device_t, void *);
static int  hb_search(device_t, cfdata_t, const int *, void *);
static int  hb_print(void *, const char *);

CFATTACH_DECL_NEW(hb, 0,
    hb_match, hb_attach, NULL, NULL);

static int
hb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, hb_cd.cd_name) != 0)
		return 0;

	if (ma->ma_systype != -1 && ma->ma_systype != systype)
		return 0;

	return 1;
}

static void
hb_attach(device_t parent, device_t self, void *aux)
{
	struct hb_attach_args ha;

	aprint_normal("\n");
	memset(&ha, 0, sizeof(ha));

	config_search(self, &ha,
	    CFARG_SUBMATCH, hb_search,
	    CFARG_EOL);
}

static int
hb_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct hb_attach_args *ha = aux;

	ha->ha_name = cf->cf_name;
	ha->ha_address = cf->cf_addr;
	ha->ha_ipl = cf->cf_ipl;
	ha->ha_vect = cf->cf_vect;

	/* XXX news68k Hyper-bus is not a real bus... */
	ha->ha_bust = ISIIOPA(ha->ha_address) ?
	    NEWS68K_BUS_SPACE_INTIO : NEWS68K_BUS_SPACE_EIO;

	if (config_match(parent, cf, ha) > 0)
		config_attach(parent, cf, ha, hb_print, CFARG_EOL);

	return 0;
}

/*
 * Print out the confargs.  The (parent) name is non-NULL
 * when there was no match found by config_found().
 */
static int
hb_print(void *args, const char *name)
{
	struct hb_attach_args *ha = args;

#if 0
	if (ha->ha_addr > 0)
#endif
		aprint_normal(" addr 0x%08lx", ha->ha_address);
	if (ha->ha_ipl > 0)
		aprint_normal(" ipl %d", ha->ha_ipl);
	if (ha->ha_vect > 0) {
		aprint_normal(" vect %d", ha->ha_vect);
	}

	return QUIET;
}

/*
 * hb_intr_establish: establish hb interrupt
 */
void
hb_intr_establish(int hbvect, int (*hand)(void *), int ipl, void *arg)
{

	if ((ipl < 1) || (ipl > 7)) {
		printf("hb: illegal interrupt level: %d\n", ipl);
		panic("hb_intr_establish");
	}

	if ((hbvect < 0) || (hbvect > 255)) {
		printf("hb: illegal vector offset: 0x%x\n", hbvect);
		panic("hb_intr_establish");
	}

	isrlink_vectored(hand, arg, ipl, hbvect);
}

void
hb_intr_disestablish(int hbvect)
{

	if ((hbvect < 0) || (hbvect > 255)) {
		printf("hb: illegal vector offset: 0x%x\n", hbvect);
		panic("hb_intr_disestablish");
	}

	isrunlink_vectored(hbvect);
}
