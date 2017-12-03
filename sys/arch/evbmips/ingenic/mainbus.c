/*	$NetBSD: mainbus.c,v 1.7.8.2 2017/12/03 11:36:09 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.7.8.2 2017/12/03 11:36:09 jdolecek Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <mips/ingenic/ingenic_regs.h>

#include "opt_ingenic.h"

#include "locators.h"

static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);
static int	mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

/* There can be only one. */
int	mainbus_found = 0;

struct mainbusdev {
	const char *md_name;
};

struct mainbusdev mainbusdevs[] = {
	{ "cpu",	},
#ifdef MULTIPROCESSOR
	{ "cpu",	},
#endif
	{ "apbus",	},
	{ NULL,		}
};

static int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	if (mainbus_found)
		return (0);

	return (1);
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	const struct mainbusdev *md;

	mainbus_found = 1;
	printf("\n");

	for (md = mainbusdevs; md->md_name != NULL; md++) {
		struct mainbusdev ma = *md;
		config_found_ia(self, "mainbus", &ma, mainbus_print);
	}

#ifdef INGENIC_DEBUG
	printf("TFR: %08x\n", readreg(JZ_TC_TFR));
	printf("TMR: %08x\n", readreg(JZ_TC_TMR));

	/* send ourselves an IPI */
	mips_cp0_corembox_write(0x12345678, 0);
	delay(1000);

	/* send the other core an IPI */
	mips_cp0_corembox_write(0x12345678, 1);
	delay(1000);
#endif
}

static int
mainbus_print(void *aux, const char *pnp)
{

	return (QUIET);
}
