/*	$NetBSD: acpi_verbose.c,v 1.1 2010/05/31 20:32:29 pgoyette Exp $ */

/*-
 * Copyright (c) 2003, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum of By Noon Software, Inc.
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
 * Copyright 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_verbose.c,v 1.1 2010/05/31 20:32:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/module.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpidevs_data.h>

void	acpi_print_devnodes_real(struct acpi_softc *);
void	acpi_print_tree_real(struct acpi_devnode *, uint32_t);
void	acpi_print_dev_real(const char *);

MODULE(MODULE_CLASS_MISC, acpiverbose, NULL); 

__weak_alias(acpi_wmidump_real, acpi_null);
 
static int
acpiverbose_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		acpi_print_devnodes = acpi_print_devnodes_real;
		acpi_print_tree = acpi_print_tree_real;
		acpi_print_dev = acpi_print_dev_real;
		acpi_wmidump = acpi_wmidump_real;
		return 0;
	case MODULE_CMD_FINI:
		acpi_print_devnodes = (void *)acpi_null;
		acpi_print_tree = (void *)acpi_null;
		acpi_print_dev = (void *)acpi_null;
		acpi_wmidump = (void *)acpi_null;
		return 0;
	default:
		return ENOTTY;
	}
}

void
acpi_print_devnodes_real(struct acpi_softc *sc)
{
	struct acpi_devnode *ad;
	ACPI_DEVICE_INFO *di;

	SIMPLEQ_FOREACH(ad, &sc->ad_head, ad_list) {

		di = ad->ad_devinfo;
		aprint_normal_dev(sc->sc_dev, "%-5s ", ad->ad_name);

		aprint_normal("HID %-10s ",
		    ((di->Valid & ACPI_VALID_HID) != 0) ?
		    di->HardwareId.String: "-");

		aprint_normal("UID %-4s ",
		    ((di->Valid & ACPI_VALID_UID) != 0) ?
		    di->UniqueId.String : "-");

		if ((di->Valid & ACPI_VALID_STA) != 0)
			aprint_normal("STA 0x%08X ", di->CurrentStatus);
		else
			aprint_normal("STA %10s ", "-");

		if ((di->Valid & ACPI_VALID_ADR) != 0)
			aprint_normal("ADR 0x%016" PRIX64"", di->Address);
		else
			aprint_normal("ADR -");

		aprint_normal("\n");
	}
	aprint_normal("\n");
}

void
acpi_print_tree_real(struct acpi_devnode *ad, uint32_t level)
{
	struct acpi_devnode *child;
	uint32_t i;

	for (i = 0; i < level; i++)
		aprint_normal("    ");

	aprint_normal("%-5s [%02u] [%c%c] ", ad->ad_name, ad->ad_type,
	    ((ad->ad_flags & ACPI_DEVICE_POWER)  != 0) ? 'P' : ' ',
	    ((ad->ad_flags & ACPI_DEVICE_WAKEUP) != 0) ? 'W' : ' ');

	if (ad->ad_pciinfo != NULL) {

		aprint_normal("(PCI) @ 0x%02X:0x%02X:0x%02X:0x%02X ",
		    ad->ad_pciinfo->ap_segment, ad->ad_pciinfo->ap_bus,
		    ad->ad_pciinfo->ap_device, ad->ad_pciinfo->ap_function);

		if ((ad->ad_devinfo->Flags & ACPI_PCI_ROOT_BRIDGE) != 0)
			aprint_normal("[R] ");

		if (ad->ad_pciinfo->ap_bridge != false)
			aprint_normal("[B] -> 0x%02X",
			    ad->ad_pciinfo->ap_downbus);
	}

	aprint_normal("\n");

	SIMPLEQ_FOREACH(child, &ad->ad_child_head, ad_child_list)
	    acpi_print_tree(child, level + 1);
}

void acpi_print_dev_real(const char *pnpstr)
{
	int i;

	for (i = 0; i < __arraycount(acpi_knowndevs); i++) {
		if (strcmp(acpi_knowndevs[i].pnp, pnpstr) == 0) {
			aprint_normal("[%s] ", acpi_knowndevs[i].str);
		}
	}
}
