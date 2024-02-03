/*	$NetBSD: autoconf.c,v 1.2.2.2 2024/02/03 11:47:07 martin Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba 
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2.2.2 2024/02/03 11:47:07 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/boot_flag.h>

#include <powerpc/pte.h>

#include <machine/wii.h>

void findroot(void);
void disable_intr(void);
void enable_intr(void);

static void parse_cmdline(void);

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{
	parse_cmdline();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	genppc_cpu_configure();
}

void
cpu_rootconf(void)
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");

	rootconf();
}

void
findroot(void)
{
}

void
device_register(device_t dev, void *aux)
{
	if (bootspec != NULL && strcmp(device_xname(dev), bootspec) == 0) {
		booted_device = dev;
		booted_partition = 0;
	}
}

static void
parse_cmdline(void)
{
	static char bootspec_buf[64];
	extern char wii_cmdline[];
	const char *cmdline = wii_cmdline;

	while (*cmdline != '\0') {
		const char *cp = cmdline;

		if (*cp == '-') {
			for (cp++; *cp != '\0'; cp++) {
				BOOT_FLAG(*cp, boothowto);
			}
		} else if (strncmp(cp, "root=", 5) == 0) {
			snprintf(bootspec_buf, sizeof(bootspec_buf), "%s",
			    cp + 5);
			if (bootspec_buf[0] != '\0') {
				bootspec = bootspec_buf;
				booted_method = "bootinfo/rootdevice";
			}
		}

		cmdline += strlen(cmdline) + 1;
	}
}
