/*	$NetBSD: autoconf.c,v 1.8.10.1 2012/08/08 15:51:07 martin Exp $	*/

/*-
 * Copyright (c) 2001, 2004 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.8.10.1 2012/08/08 15:51:07 martin Exp $");

#include "opt_sbd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <machine/sbdvar.h>
#include <machine/disklabel.h>

char __boot_kernel_name[64];

void
cpu_configure(void)
{

	intr_init();

	splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");
	spl0();
}

void
cpu_rootconf(void)
{
	device_t dv;
	char *p;
	const char *bootdev_name, *netdev_name;
	int unit, partition;

	/* Extract boot device */
	for (p = __boot_kernel_name; *p; p++) {
		if (*p == ':') {
			*p = '\0';
			break;
		}
	}
	p = __boot_kernel_name;

	bootdev_name = 0;
	unit = 0;

	switch (SBD_INFO->machine) {
#ifdef EWS4800_TR2
	case MACHINE_TR2:
		netdev_name = "iee0";
		break;
#endif
#ifdef EWS4800_TR2A
	case MACHINE_TR2A:
		netdev_name = "le0";
		break;
#endif
	default:
		netdev_name = NULL;
	}
	partition = 0;

	if (strncmp(p, "sd", 2) == 0) {
		unit = p[2] - '0';
		partition = p[3] - 'a';
		if (unit >= 0 && unit <= 9 && partition >= 0 &&
		    partition < MAXPARTITIONS) {
			p[3] = '\0';
			bootdev_name = __boot_kernel_name;
		}
	} else if (strncmp(p, "nfs", 3) == 0) {
		bootdev_name = netdev_name;
	} else if (strncmp(p, "mem", 3) == 0) {
		int bootdev = (*platform.ipl_bootdev)();
		if (bootdev == NVSRAM_BOOTDEV_HARDDISK)
			bootdev_name = "sd0";
		else if (bootdev == NVSRAM_BOOTDEV_NETWORK)
			bootdev_name = netdev_name;
		else
			bootdev_name = 0;
	}

	if (bootdev_name &&
	    (dv = device_find_by_xname(bootdev_name)) != NULL) {
		booted_device = dv;
		booted_partition = partition;
	}
	rootconf();
}
