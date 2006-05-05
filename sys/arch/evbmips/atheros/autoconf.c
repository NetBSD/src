/* $NetBSD: autoconf.c,v 1.2 2006/05/05 18:04:41 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2 2006/05/05 18:04:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <sys/socket.h>		/* these three just to get ETHER_ADDR_LEN(!) */
#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>

#include <mips/atheros/include/ar531xreg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

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
	(void)spl0();
}

void
cpu_rootconf(void)
{

	setroot(booted_device, booted_partition);
}

void
device_register(struct device *dev, void *aux)
{
	struct arbus_attach_args *aa = aux;
	struct ar531x_board_info *info;

	info = ar531x_board_info();
	if (info == NULL) {
		/* nothing known about this board! */
		return;
	}

	/*
	 * We don't ever know the boot device.  But that's because the
	 * firmware only loads from the network.
	 */

	/* Fetch the MAC addresses from YAMON. */
	if (device_is_a(dev, "ae")) {
		prop_data_t pd;
		uint8_t	*enet;

		if (aa->aa_addr == AR531X_ENET0_BASE)
			enet = info->ab_enet0_mac;
		else if (aa->aa_addr == AR531X_ENET1_BASE)
			enet = info->ab_enet1_mac;
		else
			return;

		pd = prop_data_create_data(enet, ETHER_ADDR_LEN);
		KASSERT(pd != NULL);
		if (prop_dictionary_set(device_properties(dev),
					"mac-addr", pd) == FALSE) {
			printf("WARNING: unable to set mac-addr "
			    "property for %s\n", device_xname(dev));
		}
		prop_object_release(pd);
	}

	if (device_is_a(dev, "ath")) {
		prop_data_t pd;
		uint8_t	*enet;

		if (aa->aa_addr == AR531X_WLAN0_BASE)
			enet = info->ab_wlan0_mac;
		else if (aa->aa_addr == AR531X_WLAN1_BASE)
			enet = info->ab_wlan1_mac;
		else
			return;

		pd = prop_data_create_data(enet, ETHER_ADDR_LEN);
		KASSERT(pd != NULL);
		if (prop_dictionary_set(device_properties(dev),
					"mac-addr", pd) == FALSE) {
			printf("WARNING: unable to set mac-addr "
			    "property for %s\n", device_xname(dev));
		}
		prop_object_release(pd);
	}
}
