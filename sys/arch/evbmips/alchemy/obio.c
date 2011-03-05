/* $NetBSD: obio.c,v 1.2.8.1 2011/03/05 15:09:36 bouyer Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.2.8.1 2011/03/05 15:09:36 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <evbmips/alchemy/board.h>
#include <evbmips/alchemy/obiovar.h>

#include "locators.h"

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_submatch(device_t, cfdata_t, const int *, void *);
static int	obio_print(void *, const char *);

CFATTACH_DECL_NEW(obio, 0,
    obio_match, obio_attach, NULL, NULL);

/* There can be only one. */
static int	obio_found = 0;

static int
obio_match(device_t parent, cfdata_t match, void *aux)
{

	if (obio_found)
		return (0);

	return (1);
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_attach_args oa;
	const struct obiodev *od;
	const struct alchemy_board *board;

	obio_found = 1;
	printf("\n");

	board = board_info();
	for (od = board->ab_devices; od->od_name != NULL; od++) {
		oa.oba_name = od->od_name;
		oa.oba_addr = od->od_addr;
		oa.oba_irq = od->od_irq;
		(void) config_found_sm_loc(self, "obio", NULL, &oa, obio_print,
		    obio_submatch);
	}
}

static int
obio_submatch(device_t parent, cfdata_t cf,
	      const int *ldesc, void *aux)
{
	struct obio_attach_args *oa = aux;

	if (cf->cf_loc[OBIOCF_ADDR] != OBIOCF_ADDR_DEFAULT &&
	    cf->cf_loc[OBIOCF_ADDR] != oa->oba_addr)
		return (0);

	return (config_match(parent, cf, aux));
}

static int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *oa = aux;

	if (pnp)
		aprint_normal("%s at %s", oa->oba_name, pnp);
	if (oa->oba_addr != OBIOCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%"PRIxBUSADDR, oa->oba_addr);

	return (UNCONF);
}
