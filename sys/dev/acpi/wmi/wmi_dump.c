/*	$NetBSD: wmi_dump.c,v 1.1 2010/05/31 20:32:29 pgoyette Exp $ */

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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wmi_dump.c,v 1.1 2010/05/31 20:32:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/wmi/wmi_acpivar.h>

void
acpi_wmidump_real(void *arg)
{
	struct wmi_t *wmi;
	struct acpi_wmi_softc *sc = (struct acpi_wmi_softc *)arg;

	KASSERT(SIMPLEQ_EMPTY(&sc->wmi_head) == 0);

	SIMPLEQ_FOREACH(wmi, &sc->wmi_head, wmi_link) {

		aprint_normal_dev(sc->sc_dev, "{%08X-%04X-%04X-",
		    wmi->guid.data1, wmi->guid.data2, wmi->guid.data3);

		aprint_normal("%02X%02X-%02X%02X%02X%02X%02X%02X} ",
		    wmi->guid.data4[0], wmi->guid.data4[1],
		    wmi->guid.data4[2], wmi->guid.data4[3],
		    wmi->guid.data4[4], wmi->guid.data4[5],
		    wmi->guid.data4[6], wmi->guid.data4[7]);

		aprint_normal("oid %04X count %02X flags %02X\n",
		    UGET16(wmi->guid.oid), wmi->guid.count, wmi->guid.flags);
	}
}
