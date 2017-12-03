/* $NetBSD: mainbus.c,v 1.10.10.2 2017/12/03 11:36:23 jdolecek Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.10.10.2 2017/12/03 11:36:23 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>

static const struct mainbus_attach_args luna_devs[] = {
	{ "clock",  0x45000000, -1 },	/* Mostek TimeKeeper */
	{ "lcd",    0x4d000000, -1 },	/* Sharp LM16X212 LCD module */
	{ "le",	    0xf1000000, 3 },	/* Am7990 */
	{ "sio",    0x51000000, 6 },	/* uPD7201A */
	{ "xp",     0x71000000, 1 },	/* HD647180XP */
	{ "fb",	    0xc1100000, -1 },	/* BrookTree RAMDAC */
	{ "spc",    0xe1000000, 2 },	/* MB89352 */
};

static const struct mainbus_attach_args luna2_devs[] = {
	{ "clock",  0x45000000, -1 },	/* Dallas TimeKeeper */
	{ "lcd",    0x4d000000, -1 },	/* Sharp LM16X212 LCD module */
	{ "le",	    0xf1000000, 3 },	/* Am7990 */
	{ "sio",    0x51000000, 6 },	/* uPD7201A */
	{ "xp",     0x71000000, 1 },	/* HD647180XP */
	{ "fb",	    0xc1100000, -1 },	/* BrookTree RAMDAC */
	{ "spc",    0xe1000000, 2 },	/* internal MB89352 */
	{ "spc",    0xe1000040, 2 },	/* external MB89352 */
};

static void mainbus_attach(device_t, device_t, void *);
static int  mainbus_match(device_t, cfdata_t, void *);
static int  mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int
mainbus_match(device_t parent, cfdata_t cf, void *args)
{
	static bool mainbus_matched;

	if (mainbus_matched)
		return 0;

	mainbus_matched = true;
	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *args)
{
	int i, ndevs;
	const struct mainbus_attach_args *devs;
	struct mainbus_attach_args ma;

	if (machtype == LUNA_II) {
		devs = luna2_devs;
		ndevs = __arraycount(luna2_devs);
	} else {
		devs = luna_devs;
		ndevs = __arraycount(luna_devs);
	}
	aprint_normal("\n");
	for (i = 0; i < ndevs; i++) {
		ma = devs[i];
		config_found(self, &ma, mainbus_print);
	}
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *ma = aux;

	if (pnp)
		aprint_normal("%s at %s", ma->ma_name, pnp);

	return UNCONF;
}
