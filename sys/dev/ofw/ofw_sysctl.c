/*	$NetBSD: ofw_sysctl.c,v 1.2 2020/04/03 21:55:07 macallan Exp $ */

/*-
 * Copyright (c) 2020 Michael Lorenz
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofw_sysctl.c,v 1.2 2020/04/03 21:55:07 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <sys/sysctl.h>
#include <dev/ofw/openfirm.h>

char firmwarestring[64] = "unknown";
char firmwareversion[64] = "unknown";

SYSCTL_SETUP(sysctl_hw_misc_setup, "sysctl hw.ofw subtree misc setup")
{
	const struct sysctlnode *me;
	int openprom;

	openprom = OF_finddevice("/openprom");
	if (openprom == 0 || openprom == -1) return;

	sysctl_createv(clog, 0, NULL, &me,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "ofw",
		SYSCTL_DESCR("OpenFirmware information"),
		NULL, 0, NULL, 0,
		CTL_HW, CTL_CREATE, CTL_EOL);

	memset(firmwarestring, 0, 64);
	if (OF_getprop(openprom, "model", firmwarestring, 63) > 0) {

		aprint_debug("firmware: %s\n", firmwarestring);
		sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRING, "model",
			SYSCTL_DESCR("firmware type"),
			NULL, 0, firmwarestring, 0,
			CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
	}

	memset(firmwareversion, 0, 64);
	if (OF_getprop(openprom, "version", firmwareversion, 63) > 0) {

		aprint_debug("version: %s\n", firmwareversion);
		sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRING, "version",
			SYSCTL_DESCR("firmware version"),
			NULL, 0, firmwareversion, 0,
			CTL_HW, me->sysctl_num, CTL_CREATE, CTL_EOL);
	}
}
