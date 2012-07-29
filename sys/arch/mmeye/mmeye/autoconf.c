/*	$NetBSD: autoconf.c,v 1.9 2012/07/29 18:05:44 mlelstv Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.9 2012/07/29 18:05:44 mlelstv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#if defined(SH7750R)
#include <sys/callout.h>
#include <sys/kernel.h>
#include <machine/mmeye.h>
#endif
#include <machine/bootinfo.h>

static void findroot(void);

static int bootunit;

void
cpu_configure(void)
{
	/* Start configuration */
	splhigh();

	findroot();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* Configuration is finished, turn on interrupts. */
	spl0();

#if defined(MMEYE_EPC_WDT)
	callout_init(&epc_wdtc, 0);
	callout_setfunc(&epc_wdtc, epc_watchdog_timer_reset, NULL);
	epc_watchdog_timer_reset(NULL);
#endif
}

void
cpu_rootconf(void)
{

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	rootconf();
}

void
device_register(device_t dev, void *aux)
{

	if (booted_device != NULL)
		return;

	/* check wd drive */
	if (device_class(dev) == DV_DISK &&
	    device_is_a(dev, "wd")) {
		device_t bdev, cdev;

		bdev = device_parent(dev);
		if (!device_is_a(bdev, "atabus"))
			return;
		cdev = device_parent(bdev);
		if (!device_is_a(cdev, "wdc"))
			return;

		if (device_unit(dev) == bootunit)
			booted_device = dev;
	}
}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
static void
findroot(void)
{
	struct btinfo_bootdev *bi;
	int x, p;

	bi = (struct btinfo_bootdev *)lookup_bootinfo(BTINFO_BOOTDEV);
	if (bi == NULL)
		return;
	x = strlen(bi->bootdev) - 2;
	p = strlen(bi->bootdev) - 1;
	if (!isdigit(bi->bootdev[x]) ||
	    !isalpha(bi->bootdev[p]))
		return;			/* bad format */

	bootunit = bi->bootdev[x] - '0';
	booted_partition = bi->bootdev[p] - 'a';
}
