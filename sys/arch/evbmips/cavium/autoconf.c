/*	$NetBSD: autoconf.c,v 1.2.2.3 2015/09/22 12:05:41 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.2.2.3 2015/09/22 12:05:41 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <net/if_ether.h>

#include <mips/cpu.h>
#include <mips/locore.h>

#include <evbmips/cavium/octeon_uboot.h>

static void	findroot(void);

void
cpu_configure(void)
{

	intr_init();

	/* Kick off autoconfiguration. */
	(void)splhigh();
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	/* XXX need this? */
	(void)spl0();
	KDASSERT(mips_cp0_status_read() & MIPS_SR_INT_IE);
}

void
cpu_rootconf(void)
{
	findroot();

	printf("boot device: %s\n",
		booted_device ? booted_device->dv_xname : "<unknown>");

	rootconf();
}

extern char	bootstring[];
extern int	netboot;

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
			    device_is_a(dv, "sd"))
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

static void
prop_set_cnmac(device_t dev)
{
	prop_dictionary_t dict = device_properties(dev);
	prop_data_t pd;
	prop_number_t pn;
	uint8_t enaddr[ETHER_ADDR_LEN];
	uint32_t mac_lo;
	int unit = device_unit(dev);

	/* ethernet mac address */
	memcpy(enaddr, octeon_btinfo.obt_mac_addr_base,
	    sizeof(enaddr));
	mac_lo = enaddr[3] << 16;
	mac_lo += enaddr[4] << 8;
	mac_lo += enaddr[5];
	KASSERT(unit < octeon_btinfo.obt_mac_addr_count);
	mac_lo += unit;
	enaddr[3] = (mac_lo >> 16) & 0xff;
	enaddr[4] = (mac_lo >> 8) & 0xff;
	enaddr[5] = mac_lo & 0xff;
	pd = prop_data_create_data(enaddr, ETHER_ADDR_LEN);
	KASSERT(pd != NULL);
	prop_dictionary_set_and_rel(dict, "mac-address", pd);

	/* ethernet phy adddress */
	switch (octeon_btinfo.obt_board_type) {
	case BOARD_TYPE_UBIQUITI_E100:
		pn = prop_number_create_integer(0x07 - unit);
		break;
	default:
		pn = prop_number_create_integer(-1);
		break;
	}
	KASSERT(pn != NULL);
	prop_dictionary_set_and_rel(dict, "phy-addr", pn);
}

static void
prop_set_octeon_gmx(device_t dev)
{
	prop_dictionary_t dict = device_properties(dev);
	prop_number_t tx, rx;

	/* ethernet rgmii phy dependent timing parameter. */
	switch (octeon_btinfo.obt_board_type) {
	case BOARD_TYPE_UBIQUITI_E100:
		tx = prop_number_create_integer(16);
		rx = prop_number_create_integer(0);
		break;
	default:
		tx = prop_number_create_integer(0);
		rx = prop_number_create_integer(0);
		break;
	}
	KASSERT(tx != NULL);
	KASSERT(rx != NULL);
	prop_dictionary_set_and_rel(dict, "rgmii-tx", tx);
	prop_dictionary_set_and_rel(dict, "rgmii-rx", rx);
}

void
device_register(device_t dev, void *aux)
{
	if ((booted_device == NULL) && (netboot == 1))
		if (device_class(dev) == DV_IFNET)
			booted_device = dev;

	if (device_is_a(dev, "cnmac")) {
		prop_set_cnmac(dev);
	} else if (device_is_a(dev, "octeon_gmx")) {
		prop_set_octeon_gmx(dev);
	}
}
