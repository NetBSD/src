/*	$NetBSD: autoconf.c,v 1.6.6.2 2006/06/01 22:34:09 kardel Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.6.6.2 2006/06/01 22:34:09 kardel Exp $");

#include "opt_algor_p4032.h"
#include "opt_algor_p5064.h"
#include "opt_algor_p6032.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/intr.h>

#ifdef ALGOR_P4032
#include <algor/algor/algor_p4032var.h>
#endif

void
cpu_configure(void)
{

	intr_init();

	(void) splhigh();
	if (config_rootfound("mainbus", NULL) == 0)
		panic("no mainbus found");
	(void) spl0();
}

void
cpu_rootconf(void)
{

	setroot(booted_device, booted_partition);
}

#if defined(ALGOR_P4032)
#define	BUILTIN_ETHERNET_P(pa)						\
	((pa)->pa_bus == 0 && (pa)->pa_device == 5 && (pa)->pa_function == 0)
#elif defined(ALGOR_P5064)
#define	BUILTIN_ETHERNET_P(pa)						\
	((pa)->pa_bus == 0 && (pa)->pa_device == 0 && (pa)->pa_function == 0)
#elif defined(ALGOR_P6032)
#define	BUILTIN_ETHERNET_P(pa)						\
	((pa)->pa_bus == 0 && (pa)->pa_device == 16 && (pa)->pa_function == 0)
#else
#define	BUILTIN_ETHERNET_P(pa)	0
#endif

void
device_register(struct device *dev, void *aux)
{
	struct device *pdev;

	/*
	 * We don't ever know the boot device.  But that's because the
	 * firmware only loads from the network or the parallel port.
	 */

	/*
	 * Fetch the MAC address for the built-in Ethernet (we grab it
	 * from PMON earlier in the boot process).
	 */
	if ((pdev = device_parent(dev)) != NULL && device_is_a(pdev, "pci")) {
		struct pci_attach_args *pa = aux;

		if (BUILTIN_ETHERNET_P(pa)) {
			prop_data_t pd = prop_data_create_data_nocopy(
			    algor_ethaddr, ETHER_ADDR_LEN);
			KASSERT(pd != NULL);
			if (prop_dictionary_set(device_properties(dev),
						"mac-addr", pd) == FALSE) {
				printf("WARNING: unable to set mac-addr "
				    "property for %s\n", dev->dv_xname);
			}
			prop_object_release(pd);
#if defined(ALGOR_P4032)
			/*
			 * XXX This is gross, disgusting, and otherwise vile,
			 * XXX but V962 rev. < B2 have broken DMA FIFOs.  Give
			 * XXX the on-board Ethernet a different DMA window
			 * XXX that has pre-fetching disabled so that Ethernet
			 * XXX performance doesn't completely suck.
			 */
			pa->pa_dmat = &p4032_configuration.ac_pci_pf_dmat;
			pa->pa_dmat64 = NULL;
#endif /* ALGOR_P4032 */
		}
	}
}
