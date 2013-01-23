/*	$NetBSD: autoconf.c,v 1.12.34.3 2013/01/23 00:05:45 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.12.34.3 2013/01/23 00:05:45 yamt Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/bootconfig.h>

void	(*evbarm_device_register)(device_t, void *);

#ifndef MEMORY_DISK_IS_ROOT
static void get_device(char *name);
static void set_root_device(void);
#endif

#ifndef MEMORY_DISK_IS_ROOT
/* Decode a device name to a major and minor number */

static void
get_device(char *name)
{
	int unit, part;
	char devname[16], *cp;
	device_t dv;

	if (strncmp(name, "/dev/", 5) == 0)
		name += 5;

	if (devsw_name2blk(name, devname, sizeof(devname)) == -1)
		return;

	name += strlen(devname);
	unit = part = 0;

	cp = name;
	while (*cp >= '0' && *cp <= '9')
		unit = (unit * 10) + (*cp++ - '0');
	if (cp == name)
		return;

	if (*cp >= 'a' && *cp < ('a' + MAXPARTITIONS))
		part = *cp - 'a';
	else if (*cp != '\0' && *cp != ' ')
		return;
	if ((dv = device_find_by_driver_unit(devname, unit)) != NULL) {
		booted_device = dv;
		booted_partition = part;
	}
}

/* Set the rootdev variable from the root specifier in the boot args */

static void
set_root_device(void)
{
	char *ptr;
	if (boot_args &&
	    get_bootconf_option(boot_args, "root", BOOTOPT_TYPE_STRING, &ptr))
		get_device(ptr);
}
#endif

/*
 * Set up the root device from the boot args
 */
void
cpu_rootconf(void)
{
#ifndef MEMORY_DISK_IS_ROOT
	set_root_device();
#endif
	aprint_normal("boot device: %s\n",
	    booted_device != NULL ? device_xname(booted_device) : "<unknown>");
	rootconf();
}


/*
 * void cpu_configure()
 *
 * Configure all the root devices
 * The root devices are expected to configure their own children
 */
void
cpu_configure(void)
{
	struct mainbus_attach_args maa;

	(void) splhigh();
	(void) splserial();	/* XXX need an splextreme() */

	maa.ma_name = "mainbus";

	config_rootfound("mainbus", &maa);

	/* Time to start taking interrupts so lets open the flood gates .... */
	spl0();
}

void
device_register(device_t dev, void *aux)
{

	if (evbarm_device_register != NULL)
		(*evbarm_device_register)(dev, aux);
}
