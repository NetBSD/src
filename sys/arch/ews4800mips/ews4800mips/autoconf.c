/*	$NetBSD: autoconf.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $");

#include "opt_sbd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

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
	struct device *dv;
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

	dv = 0;
	if (bootdev_name) {
		for (dv = alldevs.tqh_first; dv; dv = dv->dv_list.tqe_next) {
			if (strcmp(dv->dv_xname, bootdev_name) == 0) {
				setroot(dv, partition);
				break;
			}
		}
	}
	if (dv == 0)
		setroot(0, 0);
}
