/*	$NetBSD: eeprom.c,v 1.9.66.1 2021/04/03 22:28:38 thorpej Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/*
 * eeprom driver. Sun 4 machines use the old-style (a'la Sun 3) EEPROM.
 * On the 4/100's and 4/200's, this is at a separate obio space.
 * On the 4/300's and 4/400's, however, it is the cl_nvram[] chunk of
 * the Mostek chip.  Therefore, eeprom_match will only return true on
 * the 100/200 models, and the eeprom will be attached separately.
 * On the 300/400 models, the eeprom will be dealt with when the clock
 * is attached.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eeprom.c,v 1.9.66.1 2021/04/03 22:28:38 thorpej Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>

#include <dev/clock_subr.h>

static int	eeprom_match(device_t, cfdata_t, void *);
static void	eeprom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(eeprom, 0,
    eeprom_match, eeprom_attach, NULL, NULL);

/* We support only one eeprom device */
static int eeprom_attached;

/*
 * Sun 4/100, 4/200 EEPROM match routine.
 */
static int
eeprom_match(device_t parent, cfdata_t cf, void *aux)
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	if (eeprom_attached)
		/* We support only one eeprom device */
		return (0);

	/* Only these sun4s have oclock */
	if (!CPU_ISSUN4 ||
	    (cpuinfo.cpu_type != CPUTYP_4_100 &&
	     cpuinfo.cpu_type != CPUTYP_4_200))
		return (0);

	/* Make sure there is something there */
	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, oba->oba_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL));
}

static void
eeprom_attach(device_t parent, device_t self, void *aux)
{
#if defined(SUN4)
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	bus_space_handle_t bh;

	eeprom_attached = 1;
	printf("\n");

	if (bus_space_map(oba->oba_bustag,
			  oba->oba_paddr,
			  EEPROM_SIZE,
			  BUS_SPACE_MAP_LINEAR,	/* flags */
			  &bh) != 0) {
		printf("%s: can't map register\n",
			device_xname(self));
		return;
	}
	eeprom_va = (char *)bh;
#endif /* SUN4 */
}
