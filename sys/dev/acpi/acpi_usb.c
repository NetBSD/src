/* $NetBSD: acpi_usb.c,v 1.2 2021/01/14 14:38:22 thorpej Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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
 * USB Device-Specific Method (_DSM) support:
 *
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/bringup/usb-device-specific-method---dsm-
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_usb.c,v 1.2 2021/01/14 14:38:22 thorpej Exp $");

#include <sys/param.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_usb.h>

/* Windows-defined USB subsystem device-specific method (_DSM) */
static UINT8 ehci_acpi_dsm_uuid[ACPI_UUID_LENGTH] = {
	0x85, 0xe3, 0x2e, 0xce, 0xe6, 0x00, 0xcb, 0x48,
	0x9f, 0x05, 0x2e, 0xdb, 0x92, 0x7c, 0x48, 0x99
};

void
acpi_usb_post_reset(ACPI_HANDLE handle)
{

	/*
	 * Invoke the _DSM control method for post-reset processing function
	 * for dual-role USB controllers. If supported, this will perform any
	 * controller-specific initialization required to put the dual-role
	 * device into host mode.
	 */

	(void)acpi_dsm(handle, ehci_acpi_dsm_uuid, 0, 1, NULL, NULL);
}
