/* $NetBSD: autoconf.c,v 1.50.2.1 2012/04/17 00:05:53 yamt Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)autoconf.c	8.4 (Berkeley) 10/1/93
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.50.2.1 2012/04/17 00:05:53 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <dev/cons.h>

#include <machine/autoconf.h>
#include <machine/alpha.h>
#include <machine/cpu.h>
#include <machine/prom.h>
#include <machine/cpuconf.h>
#include <machine/intr.h>

struct bootdev_data	*bootdev_data;

void	parse_prom_bootdev(void);
static inline int atoi(const char *);

/*
 * cpu_configure:
 * called at boot time, configure all devices on system
 */
void
cpu_configure(void)
{

	parse_prom_bootdev();

	/*
	 * Disable interrupts during autoconfiguration.  splhigh() won't
	 * work, because it simply _raises_ the IPL, so if machine checks
	 * are disabled, they'll stay disabled.  Machine checks are needed
	 * during autoconfig.
	 */
	(void)alpha_pal_swpipl(ALPHA_PSL_IPL_HIGH);
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");
	(void)spl0();

	/*
	 * Note that bootstrapping is finished, and set the HWRPB up
	 * to do restarts.
	 */
	hwrpb_restart_setup();
}

void
cpu_rootconf(void)
{

	if (booted_device == NULL)
		printf("WARNING: can't figure what device matches \"%s\"\n",
		    bootinfo.booted_dev);
	setroot(booted_device, booted_partition);
}

void
parse_prom_bootdev(void)
{
	static char hacked_boot_dev[128];
	static struct bootdev_data bd;
	char *cp, *scp, *boot_fields[8];
	int i, done;

	booted_device = NULL;
	booted_partition = 0;
	bootdev_data = NULL;

	memcpy(hacked_boot_dev, bootinfo.booted_dev,
	    min(sizeof bootinfo.booted_dev, sizeof hacked_boot_dev));
#if 0
	printf("parse_prom_bootdev: boot dev = \"%s\"\n", hacked_boot_dev);
#endif

	i = 0;
	scp = cp = hacked_boot_dev;
	for (done = 0; !done; cp++) {
		if (*cp != ' ' && *cp != '\0')
			continue;
		if (*cp == '\0')
			done = 1;

		*cp = '\0';
		boot_fields[i++] = scp;
		scp = cp + 1;
		if (i == 8)
			done = 1;
	}
	if (i != 8)
		return;		/* doesn't look like anything we know! */

#if 0
	printf("i = %d, done = %d\n", i, done);
	for (i--; i >= 0; i--)
		printf("%d = %s\n", i, boot_fields[i]);
#endif

	bd.protocol = boot_fields[0];
	bd.bus = atoi(boot_fields[1]);
	bd.slot = atoi(boot_fields[2]);
	bd.channel = atoi(boot_fields[3]);
	bd.remote_address = boot_fields[4];
	bd.unit = atoi(boot_fields[5]);
	bd.boot_dev_type = atoi(boot_fields[6]);
	bd.ctrl_dev_type = boot_fields[7];

#if 0
	printf("parsed: proto = %s, bus = %d, slot = %d, channel = %d,\n",
	    bd.protocol, bd.bus, bd.slot, bd.channel);
	printf("\tremote = %s, unit = %d, dev_type = %d, ctrl_type = %s\n",
	    bd.remote_address, bd.unit, bd.boot_dev_type, bd.ctrl_dev_type);
#endif

	bootdev_data = &bd;
}

static inline int
atoi(const char *s)
{
	return (int)strtoll(s, NULL, 10);
}

void
device_register(device_t dev, void *aux)
{
	if (bootdev_data == NULL) {
		/*
		 * There is no hope.
		 */
		return;
	}
	if (platform.device_register)
		(*platform.device_register)(dev, aux);
}
