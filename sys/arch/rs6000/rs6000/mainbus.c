/*	$NetBSD: mainbus.c,v 1.1 2007/12/17 19:09:43 garbled Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1 2007/12/17 19:09:43 garbled Exp $");

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <powerpc/pio.h>
#include <machine/bus.h>

#include "mca.h"

#if NMCA > 0
#include <dev/mca/mcavar.h>
#endif

int	mainbus_match(struct device *, struct cfdata *, void *);
void	mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);

int	mainbus_print(void *, const char *);

union mainbus_attach_args {
	const char *mba_busname;		/* first elem of all */
#if NMCA > 0
	struct mcabus_attach_args mba_mba;
#endif
};

/* There can be only one. */
int mainbus_found = 0;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	if (mainbus_found)
		return 0;
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	union mainbus_attach_args mba;
	struct confargs ca;
	int slot;

	mainbus_found = 1;

	aprint_normal("\n");

	ca.ca_name = "cpu";
	ca.ca_node = 0;
	config_found_ia(self, "mainbus", &ca, mainbus_print);

#if DEBUG
	printf("scanning MCA bus\n");
	for (slot=0; slot < 16; slot++) {
		printf("slot %d == %x%x\n", slot,
		    inb(0xc0400100 + 1 + (slot<<16)),
		    inb(0xc0400100 + 0 + (slot<<16)));
	}
	printf("done\n");

	printf(" CFG reg (1)= %x\n", inl(0xc0010080));
	printf(" TCE reg (1)= %x\n", inl(0xc001009c));
#endif

	mba.mba_mba.mba_iot = &rs6000_iocc0_io_space_tag;
	mba.mba_mba.mba_memt = &rs6000_iocc0_io_space_tag; /* XXX ??? */
	mba.mba_mba.mba_dmat = NULL; /*&mca_bus_dma_tag;*/
	mba.mba_mba.mba_mc = NULL;
	mba.mba_mba.mba_bus = 0;
	config_found_ia(self, "mcabus", &mba.mba_mba, mcabusprint);

}

int
mainbus_print(void *aux, const char *pnp)
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->mba_busname, pnp);

	return (UNCONF);
}
