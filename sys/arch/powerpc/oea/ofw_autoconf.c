/* $NetBSD: ofw_autoconf.c,v 1.21 2018/03/04 00:21:20 macallan Exp $ */
/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ofw_autoconf.c,v 1.21 2018/03/04 00:21:20 macallan Exp $");

#ifdef ofppc
#include "gtpci.h"
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <dev/ofw/openfirm.h>
#include <dev/marvell/marvellvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#if NGTPCI > 0
#include <dev/marvell/gtpcivar.h>
#endif
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#include <machine/pci_machdep.h>

#include <prop/proplib.h>

extern char bootpath[256];
char cbootpath[256];
static int boot_node = 0;	/* points at boot device if we netboot */

static void canonicalize_bootpath(void);

/*
 * Determine device configuration for a machine.
 */
void
cpu_configure(void)
{
	init_interrupt();
	canonicalize_bootpath();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	genppc_cpu_configure();
}

static void
canonicalize_bootpath(void)
{
	int node, len;
	char *p, *lastp;
	char last[32], type[32];

	/*
	 * If the bootpath doesn't start with a / then it isn't
	 * an OFW path and probably is an alias, so look up the alias
	 * and regenerate the full bootpath so device_register will work.
	 */
	if (bootpath[0] != '/' && bootpath[0] != '\0') {
		int aliases = OF_finddevice("/aliases");
		char tmpbuf[100];
		char aliasbuf[256];
		if (aliases != 0) {
			char *cp1, *cp2, *cp;
			char saved_ch = '\0';
			cp1 = strchr(bootpath, ':');
			cp2 = strchr(bootpath, ',');
			cp = cp1;
			if (cp1 == NULL || (cp2 != NULL && cp2 < cp1))
				cp = cp2;
			tmpbuf[0] = '\0';
			if (cp != NULL) {
				strcpy(tmpbuf, cp);
				saved_ch = *cp;
				*cp = '\0';
			}
			len = OF_getprop(aliases, bootpath, aliasbuf,
			    sizeof(aliasbuf));
			if (len > 0) {
				if (aliasbuf[len-1] == '\0')
					len--;
				memcpy(bootpath, aliasbuf, len);
				strcpy(&bootpath[len], tmpbuf);
			} else {
				*cp = saved_ch;
			}
		}
	}

	/*
	 * Strip kernel name.  bootpath contains "OF-path"/"kernel".
	 *
	 * for example:
	 *   /bandit@F2000000/gc@10/53c94@10000/sd@0,0/netbsd	(OF-1.x)
	 *   /pci/mac-io/ata-3@2000/disk@0:0/netbsd.new		(OF-3.x)
	 */
	strcpy(cbootpath, bootpath);
	while ((node = OF_finddevice(cbootpath)) == -1) {
		if ((p = strrchr(cbootpath, '/')) == NULL)
			break;
		*p = '\0';
	}

	printf("bootpath: %s\n", bootpath);
	if (node == -1) {
		/* Cannot canonicalize... use bootpath anyway. */
		strcpy(cbootpath, bootpath);

		return;
	}

	/* see if we netbooted */
	len = OF_getprop(node, "device_type", type, sizeof(type) - 1);
	if (len > -1) {
		type[len] = 0;
		if (strcmp(type, "network") == 0) {
			boot_node = node;
		}
	}

	/*
	 * cbootpath is a valid OF path.  Use package-to-path to
	 * canonicalize pathname.
	 */

	/* Back up the last component for later use. */
	if ((p = strrchr(cbootpath, '/')) != NULL)
		strcpy(last, p + 1);
	else
		last[0] = '\0';

	memset(cbootpath, 0, sizeof(cbootpath));
	OF_package_to_path(node, cbootpath, sizeof(cbootpath) - 1);

	/*
	 * OF_1.x (at least) always returns addr == 0 for
	 * SCSI disks (i.e. "/bandit@.../.../sd@0,0").
	 * also check for .../disk@ which some Adaptec firmware uses
	 */
	lastp = strrchr(cbootpath, '/');
	if (lastp != NULL) {
		lastp++;
		if ((strncmp(lastp, "sd@", 3) == 0
		     && strncmp(last, "sd@", 3) == 0) ||
		    (strncmp(lastp, "disk@", 5) == 0
		     && strncmp(last, "disk@", 5) == 0))
			strcpy(lastp, last);
	} else {
		lastp = cbootpath;
	}

	/*
	 * At this point, cbootpath contains like:
	 * "/pci@80000000/mac-io@10/ata-3@20000/disk"
	 *
	 * The last component may have no address... so append it.
	 */
	if (strchr(lastp, '@') == NULL) {
		/* Append it. */
		if ((p = strrchr(last, '@')) != NULL)
			strcat(cbootpath, p);
	}

	if ((p = strrchr(lastp, ':')) != NULL) {
		*p++ = '\0';
		/* booted_partition = *p - '0';		XXX correct? */
	}
}

/*
 * device_register is called from config_attach as each device is
 * attached. We use it to find the NetBSD device corresponding to the
 * known OF boot device.
 */
void
device_register(device_t dev, void *aux)
{
	static device_t parent;
	static char *bp = bootpath + 1, *cp = cbootpath;
	unsigned long addr, addr2;
	char *p;
#if NGTPCI > 0
	struct powerpc_bus_space *gtpci_mem_bs_tag = NULL;
#endif

	/* Skip over devices not represented in the OF tree. */
	if (device_is_a(dev, "mainbus")) {
		parent = dev;
		return;
	}

	if (device_is_a(dev, "valkyriefb")) {
		struct confargs *ca = aux;
		prop_dictionary_t dict;

		dict = device_properties(dev);
		copy_disp_props(dev, ca->ca_node, dict);
	}

#if NGTPCI > 0
	if (device_is_a(dev, "gtpci")) {
		extern struct gtpci_prot gtpci0_prot, gtpci1_prot;
		extern struct powerpc_bus_space
		    gtpci0_io_bs_tag, gtpci0_mem_bs_tag,
		    gtpci1_io_bs_tag, gtpci1_mem_bs_tag;
		extern struct genppc_pci_chipset
		    genppc_gtpci0_chipset, genppc_gtpci1_chipset;

		struct marvell_attach_args *mva = aux;
		struct gtpci_prot *gtpci_prot;
		struct powerpc_bus_space *gtpci_io_bs_tag;
		struct genppc_pci_chipset *genppc_gtpci_chipset;
		prop_dictionary_t dict = device_properties(dev);
		prop_data_t prot, io_bs_tag, mem_bs_tag, pc;
		int iostart, ioend;

		if (mva->mva_unit == 0) {
			gtpci_prot = &gtpci0_prot;
			gtpci_io_bs_tag = &gtpci0_io_bs_tag;
			gtpci_mem_bs_tag = &gtpci0_mem_bs_tag;
			genppc_gtpci_chipset = &genppc_gtpci0_chipset;
			iostart = 0;
			ioend = 0;
		} else {
			gtpci_prot = &gtpci1_prot;
			gtpci_io_bs_tag = &gtpci1_io_bs_tag;
			gtpci_mem_bs_tag = &gtpci1_mem_bs_tag;
			genppc_gtpci_chipset = &genppc_gtpci1_chipset;
			iostart = 0x1400;
			ioend = 0xffff;
		}

		prot = prop_data_create_data_nocopy(
		    gtpci_prot, sizeof(struct gtpci_prot));
		KASSERT(prot != NULL);
		prop_dictionary_set(dict, "prot", prot);
		prop_object_release(prot);

		io_bs_tag = prop_data_create_data_nocopy(
		    gtpci_io_bs_tag, sizeof(struct powerpc_bus_space));
		KASSERT(io_bs_tag != NULL);
		prop_dictionary_set(dict, "io-bus-tag", io_bs_tag);
		prop_object_release(io_bs_tag);
		mem_bs_tag = prop_data_create_data_nocopy(
		    gtpci_mem_bs_tag, sizeof(struct powerpc_bus_space));
		KASSERT(mem_bs_tag != NULL);
		prop_dictionary_set(dict, "mem-bus-tag", mem_bs_tag);
		prop_object_release(mem_bs_tag);

		genppc_gtpci_chipset->pc_conf_v = device_private(dev);
		pc = prop_data_create_data_nocopy(genppc_gtpci_chipset,
		    sizeof(struct genppc_pci_chipset));
		KASSERT(pc != NULL);
		prop_dictionary_set(dict, "pci-chipset", pc);
		prop_object_release(pc);

		prop_dictionary_set_uint64(dict, "iostart", iostart);
		prop_dictionary_set_uint64(dict, "ioend", ioend);
		prop_dictionary_set_uint64(dict, "memstart",
		    gtpci_mem_bs_tag->pbs_base);
		prop_dictionary_set_uint64(dict, "memend",
		    gtpci_mem_bs_tag->pbs_limit - 1);
		prop_dictionary_set_uint32(dict, "cache-line-size",
		    CACHELINESIZE);
	}
#endif
	if (device_is_a(dev, "atapibus") || device_is_a(dev, "pci") ||
	    device_is_a(dev, "scsibus") || device_is_a(dev, "atabus"))
		return;

	if (device_is_a(device_parent(dev), "pci")) {
		struct pci_attach_args *pa = aux;
		prop_dictionary_t dict;
		prop_bool_t b;
		int node;
		char name[32];

		dict = device_properties(dev);
		node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);

		/* enable configuration of irq 14/15 for VIA native IDE */
		if (device_is_a(dev, "viaide") &&
		    strncmp(model_name, "Pegasos", 7) == 0) {
			b = prop_bool_create(true);
			KASSERT(b != NULL);
			(void)prop_dictionary_set(dict,
			    "use-compat-native-irq", b);
			prop_object_release(b);
		}

		if (node != 0) {
			int pci_class = 0;

			prop_dictionary_set_uint32(dict, "device_node", node);

			if (node == boot_node) {
				/* we netbooted from whatever this is */
				booted_device = dev;
			}
			/* see if this is going to be console */
			memset(name, 0, sizeof(name));
			OF_getprop(node, "device_type", name, sizeof(name));

			OF_getprop(node, "class-code", &pci_class,
				sizeof pci_class);
			pci_class = (pci_class >> 16) & 0xff;

			if (strcmp(name, "display") == 0 ||
			    strcmp(name, "ATY,DDParent") == 0 ||
			    pci_class == PCI_CLASS_DISPLAY) {
				/* setup display properties for fb driver */
				prop_dictionary_set_bool(dict, "is_console", 0);
				copy_disp_props(dev, node, dict);
			}
			if (pci_class == PCI_CLASS_NETWORK) {
				of_to_dataprop(dict, node, "local-mac-address",
				    "mac-address");
				of_to_dataprop(dict, node, "shared-pins",
				    "shared-pins");
			}
		}
#ifdef macppc
		/*
		 * XXX
		 * some macppc boxes have onboard devices where parts or all of
		 * the PCI_INTERRUPT register are hardwired to 0
		 */
		if (pa->pa_intrpin == 0)
			pa->pa_intrpin = 1;
#endif
	}

	if (booted_device)
		return;

	/*
	 * Skip over devices that are really just layers of NetBSD
	 * autoconf(9) we should just skip as they do not have any
	 * OFW devices.
	 */
	if (device_is_a(device_parent(dev), "atapibus") ||
	    device_is_a(device_parent(dev), "atabus") ||
	    device_is_a(device_parent(dev), "pci") ||
	    device_is_a(device_parent(dev), "scsibus")) {
		if (device_parent(device_parent(dev)) != parent)
			return;
	} else {
		if (device_parent(dev) != parent)
			return;
	}

	/*
	 * Get the address part of the current path component. The
	 * last component of the canonical bootpath may have no
	 * address (eg, "disk"), in which case we need to get the
	 * address from the original bootpath instead.
	 */
	p = strchr(cp, '@');
	if (!p) {
		if (bp)
			p = strchr(bp, '@');
		if (!p)
			addr = 0;
		else
			addr = strtoul(p + 1, &p, 16);
	} else
		addr = strtoul(p + 1, &p, 16);

	/* if the current path has more address, grab that too */
	if (p && *p == ',')
		addr2 = strtoul(p + 1, &p, 16);
	else
		addr2 = 0;

	if (device_is_a(device_parent(dev), "mainbus")) {
		struct confargs *ca = aux;

		if (strcmp(ca->ca_name, "ofw") == 0)		/* XXX */
			return;
		if (strcmp(ca->ca_name, "gt") == 0)
			parent = dev;
		if (addr != ca->ca_reg[0])
			return;
	} else if (device_is_a(device_parent(dev), "gt")) {
		/*
		 * Special handle for MV64361 on PegasosII(ofppc).
		 */
		if (device_is_a(dev, "mvgbec")) {
			/*
			 * Fix cp to /port@N from /ethernet/portN. (N is 0...2)
			 */
			static char fix_cp[8] = "/port@N";

			if (strlen(cp) != 15 ||
			    strncmp(cp, "/ethernet/port", 14) != 0)
				return;
			fix_cp[7] = *(cp + 15);
			p = fix_cp;
#if NGTPCI > 0
		} else if (device_is_a(dev, "gtpci")) {
			if (gtpci_mem_bs_tag != NULL &&
			    addr != gtpci_mem_bs_tag->pbs_base)
				return;
#endif
		} else
			return;
	} else if (device_is_a(device_parent(dev), "pci")) {
		struct pci_attach_args *pa = aux;

		if (addr != pa->pa_device ||
		    addr2 != pa->pa_function)
			return;
	} else if (device_is_a(device_parent(dev), "obio")) {
		struct confargs *ca = aux;

		if (addr != ca->ca_reg[0])
			return;
	} else if (device_is_a(device_parent(dev), "scsibus") ||
		   device_is_a(device_parent(dev), "atapibus")) {
		struct scsipibus_attach_args *sa = aux;

		/* periph_target is target for scsi, drive # for atapi */
		if (addr != sa->sa_periph->periph_target)
			return;
	} else if (device_is_a(device_parent(device_parent(dev)), "pciide") ||
		   device_is_a(device_parent(device_parent(dev)), "viaide") ||
		   device_is_a(device_parent(device_parent(dev)), "slide")) {
		struct ata_device *adev = aux;

		if (addr != adev->adev_channel ||
		    addr2 != adev->adev_drv_data->drive)
			return;
	} else if (device_is_a(device_parent(device_parent(dev)), "wdc")) {
		struct ata_device *adev = aux;

		if (addr != adev->adev_drv_data->drive)
			return;
	} else
		return;

	/* If we reach this point, then dev is a match for the current
	 * path component.
	 */

	if (p && *p) {
		parent = dev;
		cp = p;
		bp = strchr(bp, '/');
		if (bp)
			bp++;
		return;
	} else {
		booted_device = dev;
		booted_partition = 0; /* XXX -- should be extracted from bootpath */
		return;
	}
}

/*
 * Setup root device.
 * Configure swap area.
 */
void
cpu_rootconf(void)
{
	printf("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");

	rootconf();
}

/*
 * Find OF-device corresponding to the PCI device.
 */
int
pcidev_to_ofdev(pci_chipset_tag_t pc, pcitag_t tag)
{
	int bus, dev, func;
	u_int reg[5];
	int p, q;
	int l, b, d, f;

	pci_decompose_tag(pc, tag, &bus, &dev, &func);

	for (q = OF_peer(0); q; q = p) {
		l = OF_getprop(q, "assigned-addresses", reg, sizeof(reg));
		if (l > 4) {
			b = (reg[0] >> 16) & 0xff;
			d = (reg[0] >> 11) & 0x1f;
			f = (reg[0] >> 8) & 0x07;

			if (b == bus && d == dev && f == func)
				return q;
		}
		if ((p = OF_child(q)))
			continue;
		while (q) {
			if ((p = OF_peer(q)))
				break;
			q = OF_parent(q);
		}
	}
	return 0;
}
