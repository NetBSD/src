/*	$NetBSD: autoconf.c,v 1.6.2.2 2017/12/03 11:36:10 jdolecek Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.6.2.2 2017/12/03 11:36:10 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/cpu.h>

static void	findroot(void);

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
	findroot();

	printf("boot device: %s\n",
		booted_device ? device_xname(booted_device) : "<unknown>");

	rootconf();
}

extern char	bootstring[];
extern int	netboot;

static void
findroot(void)
{
	if (booted_device)
		return;

	if ((booted_device == NULL) && netboot == 0) {
		device_t dv;
		deviter_t di;

		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST); dv != NULL;
		    dv = deviter_next(&di)) {
			if (device_class(dv) == DV_DISK
			    && (device_is_a(dv, "wd")
				|| device_is_a(dv, "sd")
				|| device_is_a(dv, "ld")))
				booted_device = dv;
		}
		deviter_release(&di);
	}

	/*
	 * XXX Match up MBR boot specification with BSD disklabel for root?
	 */
	booted_partition = 0;
}

void
device_register(device_t dev, void *aux)
{

	if ((booted_device == NULL) && (netboot == 1))
		if (device_class(dev) == DV_IFNET)
			booted_device = dev;
}
