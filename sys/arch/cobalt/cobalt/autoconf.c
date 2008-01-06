/*	$NetBSD: autoconf.c,v 1.24.18.1 2008/01/06 05:00:51 wrstuden Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.24.18.1 2008/01/06 05:00:51 wrstuden Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/ata/atavar.h>

extern char	bootstring[];
extern int	netboot;
extern int	bootunit;
extern int	bootpart;

void
cpu_configure(void)
{

	softintr_init();

	(void)splhigh();

	icu_init();

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

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
device_register(struct device *dev, void *aux)
{

	if (booted_device != NULL)
		return;

	if (netboot == 1) {
		/* check tlp0 on netboot */
		if (device_class(dev) == DV_IFNET &&
		    device_is_a(dev, "tlp")) {
			struct pci_attach_args *pa = aux;

			if (pa->pa_bus == 0 &&
			    pa->pa_device == 7 &&
			    pa->pa_function == 0)
				booted_device = dev;
		}
	} else {
		/* check wd channel and drive */
		if (device_class(dev) == DV_DISK &&
		    device_is_a(dev, "wd")) {
			struct ata_device *adev = aux;
			int unit;

			unit = adev->adev_channel * 2 +
			    adev->adev_drv_data->drive;
			if (unit == bootunit) {
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
