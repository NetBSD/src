/* $NetBSD: autoconf.c,v 1.17.18.1 2012/08/08 15:51:02 martin Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.17.18.1 2012/08/08 15:51:02 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <sys/socket.h>		/* these three just to get ETHER_ADDR_LEN(!) */
#include <net/if.h>
#include <net/if_ether.h>

#include <machine/yamon.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>

/*
 * Configure all devices on system
 */     
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

	rootconf();
}

void
device_register(struct device *dev, void *aux)
{
	struct aubus_attach_args *aa = aux;

	/*
	 * We don't ever know the boot device.  But that's because the
	 * firmware only loads from the network.
	 */

	/* Fetch the MAC addresses from YAMON. */
	if (device_is_a(dev, "aumac")) {
		prop_data_t pd;
		const char *cp;
		char *cp0;
		int i;
		uint8_t ethaddr[ETHER_ADDR_LEN];

		/* Get the Ethernet address of the first on-board Ethernet. */
#if defined(ETHADDR)
		cp = ETHADDR;
#else
		cp = yamon_getenv("ethaddr");
#endif
		if (cp != NULL) {
			for (i = 0; i < ETHER_ADDR_LEN; i++) {
				ethaddr[i] = strtoul(cp, &cp0, 16);
				cp = cp0 + 1;
			}
			if (aa->aa_addr != MAC0_BASE &&
			    aa->aa_addr != AU1500_MAC0_BASE) {
				/* XXX
				 * The PROM has a variable for the MAC address
				 * of the first interface.  For now, just add
				 * 0x10 to the second last octet(!) for the
				 * second interface (Linux does the same).
				 */
				ethaddr[4] += 0x10;
			}
			pd = prop_data_create_data(ethaddr, ETHER_ADDR_LEN);
			KASSERT(pd != NULL);
			if (prop_dictionary_set(device_properties(dev),
						"mac-address", pd) == false) {
				printf("WARNING: unable to set mac-addr "
				    "property for %s\n", dev->dv_xname);
			}
			prop_object_release(pd);
		}
	}
}
