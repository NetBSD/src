/*	$NetBSD: mainbus.c,v 1.2 2009/08/06 16:37:01 matt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.2 2009/08/06 16:37:01 matt Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#if defined(PCI_NETBSD_CONFIGURE)
#include <sys/extent.h>
#include <sys/malloc.h>
#endif

#include <dev/pci/pcivar.h>
#if defined(PCI_NETBSD_CONFIGURE)
#include <dev/pci/pciconf.h>
#endif

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <mips/bonito/bonitoreg.h>
#include <evbmips/gdium/gdiumvar.h>

#include "locators.h"
#include "pci.h"

static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);
static int	mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

/* There can be only one. */
static bool mainbus_found;

const char * const mainbusdevs[] = {
	"cpu",
	"bonito",
#if 0
	"i2c",
	"gpio",
#endif
	NULL,
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
	size_t i;

	mainbus_found = true;
	aprint_normal("\n");

#if defined(PCI_NETBSD_CONFIGURE)
	{
		struct extent *ioext, *memext;

		ioext = extent_create("pciio",  0x00001000, 0x00003fff,
		    M_DEVBUF, NULL, 0, EX_NOWAIT);
		memext = extent_create("pcimem", 0, BONITO_PCILO_SIZE,
		    M_DEVBUF, NULL, 0, EX_NOWAIT);

		pci_configure_bus(&gdium_configuration.gc_pc, ioext, memext,
		    NULL, 0, mips_dcache_align);
		extent_destroy(ioext);
		extent_destroy(memext);
	}
#endif /* PCI_NETBSD_CONFIGURE */

	for (i = 0; i < __arraycount(mainbusdevs); i++) {
		struct mainbus_attach_args maa;
		maa.maa_name = mainbusdevs[i];
		(void) config_found_ia(self, "mainbus", &maa, mainbus_print);
	}
}

static int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *maa = aux;

	if (pnp != 0)
		return QUIET;

	if (pnp)
		aprint_normal("%s at %s", maa->maa_name, pnp);

	return UNCONF;
}
