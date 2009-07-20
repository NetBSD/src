/*	$NetBSD: mainbus.c,v 1.5 2009/07/20 05:10:49 kiyohara Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author:
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.5 2009/07/20 05:10:49 kiyohara Exp $");

#include "acpi.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/bus.h>

#include <dev/acpi/acpivar.h>


static int mainbus_match(struct device *, struct cfdata *, void *);
static void mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL_NEW(mainbus, sizeof(struct device),
    mainbus_match, mainbus_attach, NULL, NULL);


/*
 * Probe for the mainbus; always succeeds.
 */
static int
mainbus_match(device_t parent, struct cfdata *match, void *aux)
{

	return 1;
}

/*
 * Attach the mainbus.
 */
static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
#if NACPI > 0
	struct acpibus_attach_args aaa;
#endif

	aprint_naive("\n");
	aprint_normal("\n");

#if NACPI > 0
	acpi_probe();

	aaa.aa_iot = IA64_BUS_SPACE_IO;
	aaa.aa_memt = IA64_BUS_SPACE_MEM;
	aaa.aa_pc = 0;
	aaa.aa_pciflags =
	    PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY |
	    PCI_FLAGS_MWI_OKAY;
	aaa.aa_ic = 0;
	config_found_ia(self, "acpibus", &aaa, 0);
#endif

	return;
}
