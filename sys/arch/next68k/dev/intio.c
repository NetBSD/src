/*	$NetBSD: intio.c,v 1.19 2023/02/11 02:31:34 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Autoconfiguration support for next68k internal i/o space.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intio.c,v 1.19 2023/02/11 02:31:34 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <next68k/dev/intiovar.h>

int	intiomatch(device_t, cfdata_t, void *);
void	intioattach(device_t, device_t, void *);
int	intioprint(void *, const char *);
int	intiosearch(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(intio, 0,
    intiomatch, intioattach, NULL, NULL);

static bool intio_attached;

int
intiomatch(device_t parent, cfdata_t match, void *aux)
{
	/* Allow only one instance. */
	if (intio_attached)
		return 0;

	return 1;
}

void
intioattach(device_t parent, device_t self, void *aux)
{

	printf("\n");

	/* Search for and attach children. */
	config_search(self, aux,
	    CFARGS(.search = intiosearch));

	intio_attached = true;
}

int
intioprint(void *aux, const char *pnp)
{
	struct intio_attach_args *ia = aux;

	if (ia->ia_addr)
		aprint_normal(" addr %p", ia->ia_addr);

	return UNCONF;
}

int
intiosearch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args *mba = aux;
	struct intio_attach_args ia;

	do {
		ia.ia_addr = NULL;
		ia.ia_bst = NEXT68K_INTIO_BUS_SPACE;
		ia.ia_dmat = mba->mba_dmat;

		if (!config_probe(parent, cf, &ia))
			break;
		config_attach(parent, cf, &ia, intioprint, CFARGS_NONE);
	} while (cf->cf_fstate == FSTATE_STAR);

	return 0;
}

int
bus_space_map(bus_space_tag_t bst, bus_addr_t addr, bus_size_t size,
    int flags, bus_space_handle_t *bsh)
{

	if (addr >= INTIOBASE && (addr + size) < INTIOTOP) {
		*bsh = IIOV(addr);
		return 0;
	}

	return EINVAL;
}

paddr_t
bus_space_mmap(bus_space_tag_t bst, bus_addr_t addr, off_t offset, int prot,
    int flags)
{

	if (addr >= INTIOBASE && (addr + offset) < INTIOTOP)
		return m68k_btop(addr + offset);

	return -1;
}
