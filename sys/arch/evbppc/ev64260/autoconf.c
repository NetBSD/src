/*	$NetBSD: autoconf.c,v 1.12.2.1 2010/04/30 14:39:17 uebayasi Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.12.2.1 2010/04/30 14:39:17 uebayasi Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <dev/pci/pcivar.h>

#include <machine/pci_machdep.h>

#include <dev/marvell/gtreg.h>
#include <dev/marvell/marvellvar.h>


static void findroot(void);


/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	aprint_normal("biomask %jx netmask %jx ttymask %jx\n",
	    imask[IPL_BIO] & 0x3fffffffffffffff,
	    imask[IPL_NET] & 0x3fffffffffffffff,
	    imask[IPL_TTY] & 0x3fffffffffffffff);

	spl0();
}

void
cpu_rootconf(void)
{
	findroot();

	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

dev_t	bootdev = 0;

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
static void
findroot(void)
{
	device_t dv;
	const char *name;

#if 0
	printf("howto %x bootdev %x ", boothowto, bootdev);
#endif

	if ((bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC)
		return;


	name = devsw_blk2name(B_TYPE(bootdev));
	if (name == NULL)
		return;

	if ((dv = device_find_by_driver_unit(name, B_UNIT(bootdev))) != NULL) {
		booted_device = dv;
		booted_partition = B_PARTITION(bootdev);
	}
}

void
device_register(struct device *dev, void *aux)
{
	prop_dictionary_t dict = device_properties(dev);

	if (device_is_a(dev, "gfe") &&
	    device_is_a(device_parent(dev), "gt") ) {
		struct marvell_attach_args *mva = aux;
		prop_data_t mac;
		char enaddr[ETHER_ADDR_LEN] =
		    { 0x02, 0x00, 0x04, 0x00, 0x00, 0x04 };

		switch (mva->mva_offset) {
		case ETH0_BASE:	enaddr[5] |= 0; break;
		case ETH1_BASE: enaddr[5] |= 1; break;
		case ETH2_BASE: enaddr[5] |= 2; break;
		default:
			aprint_error("WARNING: unknown mac-no. for %s\n",
			    dev->dv_xname);
		}

		mac = prop_data_create_data_nocopy(enaddr, ETHER_ADDR_LEN);
		KASSERT(mac != NULL);
		if (prop_dictionary_set(dict, "mac-addr", mac) == false)
			aprint_error(
			    "WARNING: unable to set mac-addr property for %s\n",
			    dev->dv_xname);
		prop_object_release(mac);
	}
	if (device_is_a(dev, "gtpci")) {
		extern struct powerpc_bus_space
		    ev64260_pci0_io_bs_tag, ev64260_pci0_mem_bs_tag,
		    ev64260_pci1_io_bs_tag, ev64260_pci1_mem_bs_tag;
		extern struct genppc_pci_chipset
		    genppc_gtpci0_chipset, genppc_gtpci1_chipset;

		struct marvell_attach_args *mva = aux;
		struct powerpc_bus_space *pci_io_bs_tag, *pci_mem_bs_tag;
		struct genppc_pci_chipset *genppc_gtpci_chipset;
		prop_data_t io_bs_tag, mem_bs_tag, pc;

		if (mva->mva_unit == 0) {
			pci_io_bs_tag = &ev64260_pci0_io_bs_tag;
			pci_mem_bs_tag = &ev64260_pci0_mem_bs_tag;
			genppc_gtpci_chipset = &genppc_gtpci0_chipset;
		} else {
			pci_io_bs_tag = &ev64260_pci1_io_bs_tag;
			pci_mem_bs_tag = &ev64260_pci1_mem_bs_tag;
			genppc_gtpci_chipset = &genppc_gtpci1_chipset;
		}

		io_bs_tag = prop_data_create_data_nocopy(
		    pci_io_bs_tag, sizeof(struct powerpc_bus_space));
		KASSERT(io_bs_tag != NULL);
		prop_dictionary_set(dict, "io-bus-tag", io_bs_tag);
		prop_object_release(io_bs_tag);
		mem_bs_tag = prop_data_create_data_nocopy(
		    pci_mem_bs_tag, sizeof(struct powerpc_bus_space));
		KASSERT(mem_bs_tag != NULL);
		prop_dictionary_set(dict, "mem-bus-tag", mem_bs_tag);
		prop_object_release(mem_bs_tag);

		genppc_gtpci_chipset->pc_conf_v = device_private(dev);
		pc = prop_data_create_data_nocopy(genppc_gtpci_chipset,
		    sizeof(struct genppc_pci_chipset));
		KASSERT(pc != NULL);
		prop_dictionary_set(dict, "pci-chipset", pc);
		prop_object_release(pc);

		prop_dictionary_set_uint64(dict, "iostart", 0x00000600);
		prop_dictionary_set_uint64(dict, "ioend", 0x0000ffff);
		prop_dictionary_set_uint64(dict, "memstart",
		    pci_mem_bs_tag->pbs_base);
		prop_dictionary_set_uint64(dict, "memend",
		    pci_mem_bs_tag->pbs_limit - 1);
		prop_dictionary_set_uint32(dict, "cache-line-size", 32);
	}
	if (device_is_a(dev, "obio") &&
	    device_is_a(device_parent(dev), "gt") ) {
		extern struct powerpc_bus_space *ev64260_obio_bs_tags[5];
		struct marvell_attach_args *mva = aux;
		struct powerpc_bus_space *bst =
		    ev64260_obio_bs_tags[mva->mva_unit];
		prop_data_t bstd;

		bstd =
		    prop_data_create_data_nocopy(bst, sizeof(bus_space_tag_t));
		KASSERT(bstd != NULL);
		if (prop_dictionary_set(dict, "bus-tag", bstd) == false)
			aprint_error(
			    "WARNING: unable to set bus-tag property for %s\n",
			    dev->dv_xname);
		prop_object_release(bstd);
	}
}
