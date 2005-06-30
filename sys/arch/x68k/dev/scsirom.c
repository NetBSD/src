/*	$NetBSD: scsirom.c,v 1.13 2005/06/30 17:03:54 drochner Exp $	*/

/*-
 * Copyright (c) 1999 NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
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
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * SCSI BIOS ROM.
 * Used to probe the board.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scsirom.c,v 1.13 2005/06/30 17:03:54 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arch/x68k/dev/intiovar.h>
#include <arch/x68k/dev/scsiromvar.h>

struct {
	paddr_t addr, devaddr;
	int intr;
	const char id[7];
} scsirom_descr[] = {{
	0x00fc0000, 0x00e96020, 108, "SCSIIN"
}, {
	0x00ea0020, 0x00ea0000, 246, "SCSIEX"
}};

#define SCSIROM_ID	0x24

/*
 * autoconf stuff
 */
static int scsirom_find(struct device *, struct intio_attach_args *);
static int scsirom_match(struct device *, struct cfdata *, void *);
static void scsirom_attach(struct device *, struct device *, void *);

CFATTACH_DECL(scsirom, sizeof(struct scsirom_softc),
    scsirom_match, scsirom_attach, NULL, NULL);

static int 
scsirom_find(struct device *parent, struct intio_attach_args *ia)
{
	bus_space_handle_t ioh;
	char buf[10];
	int which;
	int r = -1;

	if (ia->ia_addr == scsirom_descr[INTERNAL].addr)
		which = INTERNAL;
	else if (ia->ia_addr == scsirom_descr[EXTERNAL].addr)
		which = EXTERNAL;
	else
		return -1;

	ia->ia_size = 0x1fe0;
	if (intio_map_allocate_region (parent, ia, INTIO_MAP_TESTONLY))
		return -1;

	if (bus_space_map (ia->ia_bst, ia->ia_addr, ia->ia_size, 0, &ioh) < 0)
		return -1;
	if (badaddr (INTIO_ADDR(ia->ia_addr+SCSIROM_ID))) {
		bus_space_unmap (ia->ia_bst, ioh, ia->ia_size);
		return -1;
	}
	bus_space_read_region_1 (ia->ia_bst, ioh, SCSIROM_ID, buf, 6);
	if (memcmp(buf, scsirom_descr[which].id, 6) == 0)
		r = which;
	bus_space_unmap (ia->ia_bst, ioh, ia->ia_size);

	return r;
}

static int 
scsirom_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	int r;

	if (strcmp (ia->ia_name, "scsirom") != 0)
		return 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT) {
		ia->ia_addr = scsirom_descr[0].addr;
		r = scsirom_find (parent, ia);
		if (r == INTERNAL)
			return 1;
		ia->ia_addr = scsirom_descr[1].addr;
		r = scsirom_find (parent, ia);
		if (r == EXTERNAL)
			return 1;
		return 0;
	} else if (scsirom_find(parent, ia) >= 0)
		return 1;
	else
		return 0;
}

static void 
scsirom_attach(struct device *parent, struct device *self, void *aux)
{
	struct scsirom_softc *sc = (struct scsirom_softc *)self;
	struct intio_attach_args *ia = aux;
	int r;
	struct cfdata *cf;

	sc->sc_addr = ia->ia_addr;
	sc->sc_which = scsirom_find (parent, ia);
#ifdef DIAGNOSTIC
	if (sc->sc_which < 0)
		panic ("SCSIROM curruption??");
#endif
	r = intio_map_allocate_region (parent, ia, INTIO_MAP_ALLOCATE);
#ifdef DIAGNOSTIC
	if (r)
		panic ("IO map for SCSIROM corruption??");
#endif

	ia->ia_addr = scsirom_descr[sc->sc_which].devaddr;
	if (ia->ia_intr == INTIOCF_INTR_DEFAULT)
		ia->ia_intr = scsirom_descr[sc->sc_which].intr;

	if (sc->sc_which == INTERNAL)
		printf (": On-board at %p\n", (void*)ia->ia_addr);
	else
		printf (": External at %p\n", (void*)ia->ia_addr);

	cf = config_search_ia(NULL, self, "scsirom", ia);
	if (cf) {
		config_attach(self, cf, ia, NULL);
	} else {
		printf ("%s: no matching device; ignored.\n",
			self->dv_xname);
	}

	return;
}
