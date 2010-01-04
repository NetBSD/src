/*	$NetBSD: wmi_acpivar.h,v 1.6 2010/01/04 10:06:53 jruoho Exp $	*/

/*-
 * Copyright (c) 2009, 2010 Jukka Ruohonen <jruohonen@iki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef WMI_ACPIVAR_H
#define WMI_ACPIVAR_H

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wmi_acpivar.h,v 1.6 2010/01/04 10:06:53 jruoho Exp $");

ACPI_STATUS acpi_wmi_event_register(device_t, ACPI_NOTIFY_HANDLER);
ACPI_STATUS acpi_wmi_event_get(device_t, uint32_t, ACPI_BUFFER *);
int         acpi_wmi_guid_match(device_t, const char *);
ACPI_STATUS acpi_wmi_data_query(device_t, const char *,
                                uint8_t, ACPI_BUFFER *);
ACPI_STATUS acpi_wmi_data_write(device_t, const char *,
                                uint8_t, ACPI_BUFFER *);
ACPI_STATUS acpi_wmi_method(device_t, const char *, uint8_t,
                            uint32_t, ACPI_BUFFER *, ACPI_BUFFER *);

#endif	/* WMI_ACPIVAR_H */
