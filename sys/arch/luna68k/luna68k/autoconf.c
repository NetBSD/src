/* $NetBSD: autoconf.c,v 1.1 2000/01/05 08:49:02 nisimura Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.1 2000/01/05 08:49:02 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/disklabel.h>
#include <machine/cpu.h>

#include <luna68k/luna68k/isr.h>

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
struct device *booted_device;	/* set by device drivers (if found) */

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure()
{
	booted_device = NULL;	/* set by device drivers (if found) */

	(void)splhigh();
	isrinit();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");

	spl0();
}

void
cpu_rootconf()
{
	printf("boot device: %s\n",
		(booted_device) ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, 0);
}

#if 0 /* What is this for !? */
/*
 * find a device matching "name" and unit number
 */
struct device *
getdevunit(name, unit)
	char *name;
	int unit;
{
	struct device *dev = alldevs.tqh_first;
	char num[10], fullname[16];
	int lunit;

	/* compute length of name and decimal expansion of unit number */
	sprintf(num, "%d", unit);
	lunit = strlen(num);
	if (strlen(name) + lunit >= sizeof(fullname) - 1)
		panic("config_attach: device name too long");

	strcpy(fullname, name);
	strcat(fullname, num);

	while (strcmp(dev->dv_xname, fullname) != 0) {
		if ((dev = dev->dv_list.tqe_next) == NULL)
			return NULL;
	}
	return dev;
}
#endif
