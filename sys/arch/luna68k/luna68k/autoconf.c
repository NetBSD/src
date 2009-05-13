/* $NetBSD: autoconf.c,v 1.11.14.1 2009/05/13 17:17:59 jym Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.11.14.1 2009/05/13 17:17:59 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <machine/disklabel.h>
#include <machine/cpu.h>

#include <luna68k/luna68k/isr.h>

/*
 * Determine mass storage and memory configuration for a machine.
 */
void
cpu_configure(void)
{
	booted_device = NULL;	/* set by device drivers (if found) */

	(void)splhigh();
	isrinit();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("autoconfig failed, no root");

	spl0();
}

void
cpu_rootconf(void)
{
#if 1 /* XXX to be reworked with helps of 2nd stage loaders XXX */
	int i;
	const char *devname;
	char *cp;
	extern char bootarg[64];

	cp = bootarg;
	devname = "sd0";
	for (i = 0; i < sizeof(bootarg); i++) {
		if (*cp == '\0')
			break;
		if (*cp == 'E' && memcmp("ENADDR=", cp, 7) == 0) {
			devname = "le0";
			break;
		}
		cp++;
	}
	booted_device = device_find_by_xname(devname);

#endif
	printf("boot device: %s\n",
		(booted_device) ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, 0); /* XXX partition 'a' XXX */
}
