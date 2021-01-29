/* $NetBSD: acpi_pcd.c,v 1.2 2021/01/29 15:49:55 thorpej Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * This is a driver for the Processor Container Device. The device acts
 * like a bus in the node namespace. Child nodes can be either processor
 * devices or other processor containers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_pcd.c,v 1.2 2021/01/29 15:49:55 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ACPI0010" },	/* Processor Container Device */
	DEVICE_COMPAT_EOL
};

static int	acpi_pcd_match(device_t, cfdata_t, void *);
static void	acpi_pcd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(acpipcd, 0, acpi_pcd_match, acpi_pcd_attach, NULL, NULL);

static int
acpi_pcd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compat_data);
}

static void
acpi_pcd_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_attach_args * const aa = aux;

	aprint_naive("\n");
	aprint_normal(": Processor Container Device\n");

	/*
	 * Initialize the container device. ACPICA should have done this
	 * for us, but may have done so before a node that this one depends
	 * on had been initialized.
	 */
	AcpiEvaluateObject(aa->aa_node->ad_handle, "_INI", NULL, NULL);
}
