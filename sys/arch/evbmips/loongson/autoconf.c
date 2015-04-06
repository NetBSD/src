/*	$NetBSD: autoconf.c,v 1.5.14.1 2015/04/06 15:17:56 skrll Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_md.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.5.14.1 2015/04/06 15:17:56 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <evbmips/loongson/autoconf.h>

#ifndef MEMORY_DISK_IS_ROOT
static void	findroot(void);
#endif

enum devclass bootdev_class = DV_DULL;
char          bootdev[16];
extern const struct platform *sys_platform;

void
cpu_configure(void)
{

	intr_init();

	/* Kick off autoconfiguration. */
	(void)splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/*
	 * Hardware interrupts will be enabled in
	 * sys/arch/mips/mips/mips3_clockintr.c:mips3_initclocks()
	 * to avoid hardclock(9) by CPU INT5 before softclockintr is
	 * initialized in initclocks().
	 */
}

void
cpu_rootconf(void)
{
#ifndef MEMORY_DISK_IS_ROOT
	findroot();
#endif

	printf("boot device: %s\n",
		booted_device ? device_xname(booted_device) : "<unknown>");

	rootconf();
}

extern char	bootstring[];
extern int	netboot;

#ifndef MEMORY_DISK_IS_ROOT
static void
findroot(void)
{
	device_t dv;
	deviter_t di;

	if (booted_device)
		return;

	if ((booted_device == NULL) && netboot == 0) {
		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST); dv != NULL;
		     dv = deviter_next(&di)) {
			if (device_class(dv) == DV_DISK &&
			    device_is_a(dv, "wd"))
				    booted_device = dv;
		}
		deviter_release(&di);
	}

	/*
	 * XXX Match up MBR boot specification with BSD disklabel for root?
	 */
	booted_partition = 0;

	return;
}
#endif

void
device_register(device_t dev, void *aux)
{
	prop_dictionary_t dict;

	if ((booted_device == NULL) && (netboot == 1))
		if (device_class(dev) == DV_IFNET)
			booted_device = dev;

	if (sys_platform->device_register != NULL) {
		sys_platform->device_register(dev, aux);
	}

	/* is this yeeloong specific? */
	if (device_is_a(dev, "lynxfb")) {
		dict = device_properties(dev);
		/*
		 * this is a hack
		 * is_console and address need to be checked against reality
		 */
		prop_dictionary_set_bool(dict, "is_console", 1);
		prop_dictionary_set_uint32(dict, "width", 1024);
		prop_dictionary_set_uint32(dict, "height", 600);
		prop_dictionary_set_uint32(dict, "depth", 16);
		prop_dictionary_set_uint32(dict, "linebytes", 2048);
		prop_dictionary_set_bool(dict, "swapBR", 1);
	}
}
