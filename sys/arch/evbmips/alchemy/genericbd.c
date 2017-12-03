/* $NetBSD: genericbd.c,v 1.4.12.2 2017/12/03 11:36:08 jdolecek Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genericbd.c,v 1.4.12.2 2017/12/03 11:36:08 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <mips/locore.h>

#include <evbmips/alchemy/obiovar.h>
#include <evbmips/alchemy/board.h>

static void genericbd_init(void);

/*
 * Generically, we have no OBIO devices.
 */
static const struct obiodev genericbd_devices[] = {
	{ NULL },
};

static struct alchemy_board genericbd_info = {
	NULL,
	genericbd_devices,
	genericbd_init,
	NULL,	/* no PCI */
};

/*
 * XXX: A cleaner way would be to get this from the cpu table in the MIPS
 * CPU handler code (or even better, have *that* code fill in cpu model.)
 */
static struct {
	int		id;
	const char 	*name;
} cpus[] = {
	{ MIPS_AU1000,	"Generic Alchemy Au1000" },
	{ MIPS_AU1100,	"Generic Alchemy Au1100" },
	{ MIPS_AU1500,  "Generic Alchemy Au1500" },
	{ MIPS_AU1550,	"Generic Alchemy Au1550" },
	{ 0, NULL },
};

const struct alchemy_board *
board_info(void)
{
	const mips_prid_t cpu_id = mips_options.mips_cpu_id;

	/* at least try to report the correct processor name */
	if (genericbd_info.ab_name == NULL) {
		int	i;
		for (i = 0; cpus[i].name; i++) {
			if (cpus[i].id == MIPS_PRID_COPTS(cpu_id)) {
				genericbd_info.ab_name = cpus[i].name;
				break;
			}
		}
		if (genericbd_info.ab_name == NULL)
			genericbd_info.ab_name = "Unknown Alchemy";
	}

	return &genericbd_info;
}

void
genericbd_init(void)
{

	/* leave console and clocks alone -- YAMON should have got it right! */
}
