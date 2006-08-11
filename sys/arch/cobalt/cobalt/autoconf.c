/*	$NetBSD: autoconf.c,v 1.16.2.4 2006/08/11 15:41:19 yamt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.16.2.4 2006/08/11 15:41:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include <cobalt/cobalt/clockvar.h>

extern char	bootstring[];
extern int	netboot;
extern int	bootunit;
extern int	bootpart;

void
cpu_configure(void)
{

	softintr_init();

	(void)splhigh();

	evcnt_attach_static(&hardclock_ev);
#ifdef ENABLE_INT5_STATCLOCK
	evcnt_attach_static(&statclock_ev);
#endif
	icu_init();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

#ifdef ENABLE_INT5_STATCLOCK
	/*
	 * Enable interrupt sources.
	 * We can't enable CPU INT5 which is used by statclock(9) here
	 * until cpu_initclocks(9) is called because there is no way
	 * to disable it other than setting status register by spl(9).
	 */
	_spllower(MIPS_INT_MASK_5);
#else
	/* enable all interrupts */
	_splnone();
#endif
}

void
cpu_rootconf(void)
{

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

static int hd_iterate = -1;

void
device_register(struct device *dev, void *aux)
{

	if (booted_device)
		return;

	if ((booted_device == NULL) && (netboot == 1))
		if (device_class(dev) == DV_IFNET)
			booted_device = dev;

	if ((booted_device == NULL) && (netboot == 0)) {
		if (device_class(dev) == DV_DISK && device_is_a(dev, "wd")) {
			hd_iterate++;
			if (hd_iterate == bootunit) {
				booted_device = dev;
			}
		}
		/*
		 * XXX Match up MBR boot specification with BSD disklabel
		 *     for root?
		 */
		booted_partition = 0;
	}
}
