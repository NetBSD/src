/*	$NetBSD: pci_subr.c,v 1.49 2002/05/03 16:08:36 nathanw Exp $	*/

/*
 * Copyright (c) 1997 Zubin D. Dittia.  All rights reserved.
 * Copyright (c) 1995, 1996, 1998, 2000
 *	Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI autoconfiguration support functions.
 *
 * Note: This file is also built into a userland library (libpci).
 * Pay attention to this when you make modifications.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_subr.c,v 1.49 2002/05/03 16:08:36 nathanw Exp $");

#ifdef _KERNEL_OPT
#include "opt_pci.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>

#ifdef _KERNEL
#include <machine/intr.h>
#else
#include <pci.h>
#include <stdio.h>
#endif

#include <dev/pci/pcireg.h>
#ifdef _KERNEL
#include <dev/pci/pcivar.h>
#endif
#ifdef PCIVERBOSE
#include <dev/pci/pcidevs.h>
#endif

/*
 * Descriptions of known PCI classes and subclasses.
 *
 * Subclasses are described in the same way as classes, but have a
 * NULL subclass pointer.
 */
struct pci_class {
	const char	*name;
	int		val;		/* as wide as pci_{,sub}class_t */
	const struct pci_class *subclasses;
};

const struct pci_class pci_subclass_prehistoric[] = {
	{ "miscellaneous",	PCI_SUBCLASS_PREHISTORIC_MISC,		},
	{ "VGA",		PCI_SUBCLASS_PREHISTORIC_VGA,		},
	{ 0 }
};

const struct pci_class pci_subclass_mass_storage[] = {
	{ "SCSI",		PCI_SUBCLASS_MASS_STORAGE_SCSI,		},
	{ "IDE",		PCI_SUBCLASS_MASS_STORAGE_IDE,		},
	{ "floppy",		PCI_SUBCLASS_MASS_STORAGE_FLOPPY,	},
	{ "IPI",		PCI_SUBCLASS_MASS_STORAGE_IPI,		},
	{ "RAID",		PCI_SUBCLASS_MASS_STORAGE_RAID,		},
	{ "ATA",		PCI_SUBCLASS_MASS_STORAGE_ATA,		},
	{ "miscellaneous",	PCI_SUBCLASS_MASS_STORAGE_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_network[] = {
	{ "ethernet",		PCI_SUBCLASS_NETWORK_ETHERNET,		},
	{ "token ring",		PCI_SUBCLASS_NETWORK_TOKENRING,		},
	{ "FDDI",		PCI_SUBCLASS_NETWORK_FDDI,		},
	{ "ATM",		PCI_SUBCLASS_NETWORK_ATM,		},
	{ "ISDN",		PCI_SUBCLASS_NETWORK_ISDN,		},
	{ "WorldFip",		PCI_SUBCLASS_NETWORK_WORLDFIP,		},
	{ "PCMIG Multi Computing", PCI_SUBCLASS_NETWORK_PCIMGMULTICOMP,	},
	{ "miscellaneous",	PCI_SUBCLASS_NETWORK_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_display[] = {
	{ "VGA",		PCI_SUBCLASS_DISPLAY_VGA,		},
	{ "XGA",		PCI_SUBCLASS_DISPLAY_XGA,		},
	{ "3D",			PCI_SUBCLASS_DISPLAY_3D,		},
	{ "miscellaneous",	PCI_SUBCLASS_DISPLAY_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_multimedia[] = {
	{ "video",		PCI_SUBCLASS_MULTIMEDIA_VIDEO,		},
	{ "audio",		PCI_SUBCLASS_MULTIMEDIA_AUDIO,		},
	{ "telephony",		PCI_SUBCLASS_MULTIMEDIA_TELEPHONY,	},
	{ "miscellaneous",	PCI_SUBCLASS_MULTIMEDIA_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_memory[] = {
	{ "RAM",		PCI_SUBCLASS_MEMORY_RAM,		},
	{ "flash",		PCI_SUBCLASS_MEMORY_FLASH,		},
	{ "miscellaneous",	PCI_SUBCLASS_MEMORY_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_bridge[] = {
	{ "host",		PCI_SUBCLASS_BRIDGE_HOST,		},
	{ "ISA",		PCI_SUBCLASS_BRIDGE_ISA,		},
	{ "EISA",		PCI_SUBCLASS_BRIDGE_EISA,		},
	{ "MicroChannel",	PCI_SUBCLASS_BRIDGE_MC,			},
	{ "PCI",		PCI_SUBCLASS_BRIDGE_PCI,		},
	{ "PCMCIA",		PCI_SUBCLASS_BRIDGE_PCMCIA,		},
	{ "NuBus",		PCI_SUBCLASS_BRIDGE_NUBUS,		},
	{ "CardBus",		PCI_SUBCLASS_BRIDGE_CARDBUS,		},
	{ "RACEway",		PCI_SUBCLASS_BRIDGE_RACEWAY,		},
	{ "Semi-transparent PCI", PCI_SUBCLASS_BRIDGE_STPCI,		},
	{ "InfiniBand",		PCI_SUBCLASS_BRIDGE_INFINIBAND,		},
	{ "miscellaneous",	PCI_SUBCLASS_BRIDGE_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_communications[] = {
	{ "serial",		PCI_SUBCLASS_COMMUNICATIONS_SERIAL,	},
	{ "parallel",		PCI_SUBCLASS_COMMUNICATIONS_PARALLEL,	},
	{ "multi-port serial",	PCI_SUBCLASS_COMMUNICATIONS_MPSERIAL,	},
	{ "modem",		PCI_SUBCLASS_COMMUNICATIONS_MODEM,	},
	{ "GPIB",		PCI_SUBCLASS_COMMUNICATIONS_GPIB,	},
	{ "smartcard",		PCI_SUBCLASS_COMMUNICATIONS_SMARTCARD,	},
	{ "miscellaneous",	PCI_SUBCLASS_COMMUNICATIONS_MISC,	},
	{ 0 },
};

const struct pci_class pci_subclass_system[] = {
	{ "8259 PIC",		PCI_SUBCLASS_SYSTEM_PIC,		},
	{ "8237 DMA",		PCI_SUBCLASS_SYSTEM_DMA,		},
	{ "8254 timer",		PCI_SUBCLASS_SYSTEM_TIMER,		},
	{ "RTC",		PCI_SUBCLASS_SYSTEM_RTC,		},
	{ "PCI Hot-Plug",	PCI_SUBCLASS_SYSTEM_RTC,		},
	{ "miscellaneous",	PCI_SUBCLASS_SYSTEM_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_input[] = {
	{ "keyboard",		PCI_SUBCLASS_INPUT_KEYBOARD,		},
	{ "digitizer",		PCI_SUBCLASS_INPUT_DIGITIZER,		},
	{ "mouse",		PCI_SUBCLASS_INPUT_MOUSE,		},
	{ "scanner",		PCI_SUBCLASS_INPUT_SCANNER,		},
	{ "game port",		PCI_SUBCLASS_INPUT_GAMEPORT,		},
	{ "miscellaneous",	PCI_SUBCLASS_INPUT_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_dock[] = {
	{ "generic",		PCI_SUBCLASS_DOCK_GENERIC,		},
	{ "miscellaneous",	PCI_SUBCLASS_DOCK_MISC,			},
	{ 0 },
};

const struct pci_class pci_subclass_processor[] = {
	{ "386",		PCI_SUBCLASS_PROCESSOR_386,		},
	{ "486",		PCI_SUBCLASS_PROCESSOR_486,		},
	{ "Pentium",		PCI_SUBCLASS_PROCESSOR_PENTIUM,		},
	{ "Alpha",		PCI_SUBCLASS_PROCESSOR_ALPHA,		},
	{ "PowerPC",		PCI_SUBCLASS_PROCESSOR_POWERPC,		},
	{ "MIPS",		PCI_SUBCLASS_PROCESSOR_MIPS,		},
	{ "Co-processor",	PCI_SUBCLASS_PROCESSOR_COPROC,		},
	{ 0 },
};

const struct pci_class pci_subclass_serialbus[] = {
	{ "Firewire",		PCI_SUBCLASS_SERIALBUS_FIREWIRE,	},
	{ "ACCESS.bus",		PCI_SUBCLASS_SERIALBUS_ACCESS,		},
	{ "SSA",		PCI_SUBCLASS_SERIALBUS_SSA,		},
	{ "USB",		PCI_SUBCLASS_SERIALBUS_USB,		},
	/* XXX Fiber Channel/_FIBRECHANNEL */
	{ "Fiber Channel",	PCI_SUBCLASS_SERIALBUS_FIBER,		},
	{ "SMBus",		PCI_SUBCLASS_SERIALBUS_SMBUS,		},
	{ "InfiniBand",		PCI_SUBCLASS_SERIALBUS_INFINIBAND,	},
	{ "IPMI",		PCI_SUBCLASS_SERIALBUS_IPMI,		},
	{ "SERCOS",		PCI_SUBCLASS_SERIALBUS_SERCOS,		},
	{ "CANbus",		PCI_SUBCLASS_SERIALBUS_CANBUS,		},
	{ 0 },
};

const struct pci_class pci_subclass_wireless[] = {
	{ "IrDA",		PCI_SUBCLASS_WIRELESS_IRDA,		},
	{ "Consumer IR",	PCI_SUBCLASS_WIRELESS_CONSUMERIR,	},
	{ "RF",			PCI_SUBCLASS_WIRELESS_RF,		},
	{ "bluetooth",		PCI_SUBCLASS_WIRELESS_BLUETOOTH,	},
	{ "broadband",		PCI_SUBCLASS_WIRELESS_BROADBAND,	},
	{ "miscellaneous",	PCI_SUBCLASS_WIRELESS_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_i2o[] = {
	{ "standard",		PCI_SUBCLASS_I2O_STANDARD,		},
	{ 0 },
};

const struct pci_class pci_subclass_satcom[] = {
	{ "TV",			PCI_SUBCLASS_SATCOM_TV,			},
	{ "audio",		PCI_SUBCLASS_SATCOM_AUDIO,		},
	{ "voice",		PCI_SUBCLASS_SATCOM_VOICE,		},
	{ "data",		PCI_SUBCLASS_SATCOM_DATA,		},
	{ 0 },
};

const struct pci_class pci_subclass_crypto[] = {
	{ "network/computing",	PCI_SUBCLASS_CRYPTO_NETCOMP,		},
	{ "entertainment",	PCI_SUBCLASS_CRYPTO_ENTERTAINMENT,	},
	{ "miscellaneous",	PCI_SUBCLASS_CRYPTO_MISC,		},
	{ 0 },
};

const struct pci_class pci_subclass_dasp[] = {
	{ "DPIO",		PCI_SUBCLASS_DASP_DPIO,			},
	{ "Time and Frequency",	PCI_SUBCLASS_DASP_TIMEFREQ,		},
	{ "synchronization",	PCI_SUBCLASS_DASP_SYNC,			},
	{ "management",		PCI_SUBCLASS_DASP_MGMT,			},
	{ "miscellaneous",	PCI_SUBCLASS_DASP_MISC,			},
	{ 0 },
};

const struct pci_class pci_class[] = {
	{ "prehistoric",	PCI_CLASS_PREHISTORIC,
	    pci_subclass_prehistoric,				},
	{ "mass storage",	PCI_CLASS_MASS_STORAGE,
	    pci_subclass_mass_storage,				},
	{ "network",		PCI_CLASS_NETWORK,
	    pci_subclass_network,				},
	{ "display",		PCI_CLASS_DISPLAY,
	    pci_subclass_display,				},
	{ "multimedia",		PCI_CLASS_MULTIMEDIA,
	    pci_subclass_multimedia,				},
	{ "memory",		PCI_CLASS_MEMORY,
	    pci_subclass_memory,				},
	{ "bridge",		PCI_CLASS_BRIDGE,
	    pci_subclass_bridge,				},
	{ "communications",	PCI_CLASS_COMMUNICATIONS,
	    pci_subclass_communications,			},
	{ "system",		PCI_CLASS_SYSTEM,
	    pci_subclass_system,				},
	{ "input",		PCI_CLASS_INPUT,
	    pci_subclass_input,					},
	{ "dock",		PCI_CLASS_DOCK,
	    pci_subclass_dock,					},
	{ "processor",		PCI_CLASS_PROCESSOR,
	    pci_subclass_processor,				},
	{ "serial bus",		PCI_CLASS_SERIALBUS,
	    pci_subclass_serialbus,				},
	{ "wireless",		PCI_CLASS_WIRELESS,
	    pci_subclass_wireless,				},
	{ "I2O",		PCI_CLASS_I2O,
	    pci_subclass_i2o,					},
	{ "satellite comm",	PCI_CLASS_SATCOM,
	    pci_subclass_satcom,				},
	{ "crypto",		PCI_CLASS_CRYPTO,
	    pci_subclass_crypto,				},
	{ "DASP",		PCI_CLASS_DASP,
	    pci_subclass_dasp,					},
	{ "undefined",		PCI_CLASS_UNDEFINED,
	    0,							},
	{ 0 },
};

#ifdef PCIVERBOSE
/*
 * Descriptions of of known vendors and devices ("products").
 */
struct pci_knowndev {
	pci_vendor_id_t		vendor;
	pci_product_id_t	product;
	int			flags;
	char			*vendorname, *productname;
};
#define	PCI_KNOWNDEV_NOPROD	0x01		/* match on vendor only */

#include <dev/pci/pcidevs_data.h>
#endif /* PCIVERBOSE */

char *
pci_findvendor(pcireg_t id_reg)
{
#ifdef PCIVERBOSE
	pci_vendor_id_t vendor = PCI_VENDOR(id_reg);
	const struct pci_knowndev *kdp;

	kdp = pci_knowndevs;
        while (kdp->vendorname != NULL) {	/* all have vendor name */
                if (kdp->vendor == vendor)
                        break;
		kdp++;
	}
        return (kdp->vendorname);
#else
	return (NULL);
#endif
}

void
pci_devinfo(pcireg_t id_reg, pcireg_t class_reg, int showclass, char *cp)
{
	pci_vendor_id_t vendor;
	pci_product_id_t product;
	pci_class_t class;
	pci_subclass_t subclass;
	pci_interface_t interface;
	pci_revision_t revision;
	char *vendor_namep, *product_namep;
	const struct pci_class *classp, *subclassp;
#ifdef PCIVERBOSE
	const struct pci_knowndev *kdp;
	const char *unmatched = "unknown ";
#else
	const char *unmatched = "";
#endif

	vendor = PCI_VENDOR(id_reg);
	product = PCI_PRODUCT(id_reg);

	class = PCI_CLASS(class_reg);
	subclass = PCI_SUBCLASS(class_reg);
	interface = PCI_INTERFACE(class_reg);
	revision = PCI_REVISION(class_reg);

#ifdef PCIVERBOSE
	kdp = pci_knowndevs;
        while (kdp->vendorname != NULL) {	/* all have vendor name */
                if (kdp->vendor == vendor && (kdp->product == product ||
		    (kdp->flags & PCI_KNOWNDEV_NOPROD) != 0))
                        break;
		kdp++;
	}
        if (kdp->vendorname == NULL)
		vendor_namep = product_namep = NULL;
	else {
		vendor_namep = kdp->vendorname;
		product_namep = (kdp->flags & PCI_KNOWNDEV_NOPROD) == 0 ?
		    kdp->productname : NULL;
        }
#else /* PCIVERBOSE */
	vendor_namep = product_namep = NULL;
#endif /* PCIVERBOSE */

	classp = pci_class;
	while (classp->name != NULL) {
		if (class == classp->val)
			break;
		classp++;
	}

	subclassp = (classp->name != NULL) ? classp->subclasses : NULL;
	while (subclassp && subclassp->name != NULL) {
		if (subclass == subclassp->val)
			break;
		subclassp++;
	}

	if (vendor_namep == NULL)
		cp += sprintf(cp, "%svendor 0x%04x product 0x%04x",
		    unmatched, vendor, product);
	else if (product_namep != NULL)
		cp += sprintf(cp, "%s %s", vendor_namep, product_namep);
	else
		cp += sprintf(cp, "%s product 0x%04x",
		    vendor_namep, product);
	if (showclass) {
		cp += sprintf(cp, " (");
		if (classp->name == NULL)
			cp += sprintf(cp, "class 0x%02x, subclass 0x%02x",
			    class, subclass);
		else {
			if (subclassp == NULL || subclassp->name == NULL)
				cp += sprintf(cp,
				    "%s subclass 0x%02x",
				    classp->name, subclass);
			else
				cp += sprintf(cp, "%s %s",
				    subclassp->name, classp->name);
		}
		if (interface != 0)
			cp += sprintf(cp, ", interface 0x%02x", interface);
		if (revision != 0)
			cp += sprintf(cp, ", revision 0x%02x", revision);
		cp += sprintf(cp, ")");
	}
}

/*
 * Print out most of the PCI configuration registers.  Typically used
 * in a device attach routine like this:
 *
 *	#ifdef MYDEV_DEBUG
 *		printf("%s: ", sc->sc_dev.dv_xname);
 *		pci_conf_print(pa->pa_pc, pa->pa_tag, NULL);
 *	#endif
 */

#define	i2o(i)	((i) * 4)
#define	o2i(o)	((o) / 4)
#define	onoff(str, bit)							\
	printf("      %s: %s\n", (str), (rval & (bit)) ? "on" : "off");

static void
pci_conf_print_common(
#ifdef _KERNEL
    pci_chipset_tag_t pc, pcitag_t tag,
#endif
    const pcireg_t *regs)
{
#ifdef PCIVERBOSE
	const struct pci_knowndev *kdp;
#endif
	const struct pci_class *classp, *subclassp;
	pcireg_t rval;

	rval = regs[o2i(PCI_ID_REG)];
#ifndef PCIVERBOSE
	printf("    Vendor ID: 0x%04x\n", PCI_VENDOR(rval));
	printf("    Device ID: 0x%04x\n", PCI_PRODUCT(rval));
#else
	for (kdp = pci_knowndevs; kdp->vendorname != NULL; kdp++) {
		if (kdp->vendor == PCI_VENDOR(rval) &&
		    (kdp->product == PCI_PRODUCT(rval) ||
		    (kdp->flags & PCI_KNOWNDEV_NOPROD) != 0)) {
			break;
		}
	}
	if (kdp->vendorname != NULL)
		printf("    Vendor Name: %s (0x%04x)\n", kdp->vendorname,
		    PCI_VENDOR(rval));
	else
		printf("    Vendor ID: 0x%04x\n", PCI_VENDOR(rval));
	if (kdp->productname != NULL && (kdp->flags & PCI_KNOWNDEV_NOPROD) == 0)
		printf("    Device Name: %s (0x%04x)\n", kdp->productname,
		    PCI_PRODUCT(rval));
	else
		printf("    Device ID: 0x%04x\n", PCI_PRODUCT(rval));
#endif /* PCIVERBOSE */

	rval = regs[o2i(PCI_COMMAND_STATUS_REG)];

	printf("    Command register: 0x%04x\n", rval & 0xffff);
	onoff("I/O space accesses", PCI_COMMAND_IO_ENABLE);
	onoff("Memory space accesses", PCI_COMMAND_MEM_ENABLE);
	onoff("Bus mastering", PCI_COMMAND_MASTER_ENABLE);
	onoff("Special cycles", PCI_COMMAND_SPECIAL_ENABLE);
	onoff("MWI transactions", PCI_COMMAND_INVALIDATE_ENABLE);
	onoff("Palette snooping", PCI_COMMAND_PALETTE_ENABLE);
	onoff("Parity error checking", PCI_COMMAND_PARITY_ENABLE);
	onoff("Address/data stepping", PCI_COMMAND_STEPPING_ENABLE);
	onoff("System error (SERR)", PCI_COMMAND_SERR_ENABLE);
	onoff("Fast back-to-back transactions", PCI_COMMAND_BACKTOBACK_ENABLE);

	printf("    Status register: 0x%04x\n", (rval >> 16) & 0xffff);
	onoff("Capability List support", PCI_STATUS_CAPLIST_SUPPORT);
	onoff("66 MHz capable", PCI_STATUS_66MHZ_SUPPORT);
	onoff("User Definable Features (UDF) support", PCI_STATUS_UDF_SUPPORT);
	onoff("Fast back-to-back capable", PCI_STATUS_BACKTOBACK_SUPPORT);
	onoff("Data parity error detected", PCI_STATUS_PARITY_ERROR);

	printf("      DEVSEL timing: ");
	switch (rval & PCI_STATUS_DEVSEL_MASK) {
	case PCI_STATUS_DEVSEL_FAST:
		printf("fast");
		break;
	case PCI_STATUS_DEVSEL_MEDIUM:
		printf("medium");
		break;
	case PCI_STATUS_DEVSEL_SLOW:
		printf("slow");
		break;
	default:
		printf("unknown/reserved");	/* XXX */
		break;
	}
	printf(" (0x%x)\n", (rval & PCI_STATUS_DEVSEL_MASK) >> 25);

	onoff("Slave signaled Target Abort", PCI_STATUS_TARGET_TARGET_ABORT);
	onoff("Master received Target Abort", PCI_STATUS_MASTER_TARGET_ABORT);
	onoff("Master received Master Abort", PCI_STATUS_MASTER_ABORT);
	onoff("Asserted System Error (SERR)", PCI_STATUS_SPECIAL_ERROR);
	onoff("Parity error detected", PCI_STATUS_PARITY_DETECT);

	rval = regs[o2i(PCI_CLASS_REG)];
	for (classp = pci_class; classp->name != NULL; classp++) {
		if (PCI_CLASS(rval) == classp->val)
			break;
	}
	subclassp = (classp->name != NULL) ? classp->subclasses : NULL;
	while (subclassp && subclassp->name != NULL) {
		if (PCI_SUBCLASS(rval) == subclassp->val)
			break;
		subclassp++;
	}
	if (classp->name != NULL) {
		printf("    Class Name: %s (0x%02x)\n", classp->name,
		    PCI_CLASS(rval));
		if (subclassp != NULL && subclassp->name != NULL)
			printf("    Subclass Name: %s (0x%02x)\n",
			    subclassp->name, PCI_SUBCLASS(rval));
		else
			printf("    Subclass ID: 0x%02x\n", PCI_SUBCLASS(rval));
	} else {
		printf("    Class ID: 0x%02x\n", PCI_CLASS(rval));
		printf("    Subclass ID: 0x%02x\n", PCI_SUBCLASS(rval));
	}
	printf("    Interface: 0x%02x\n", PCI_INTERFACE(rval));
	printf("    Revision ID: 0x%02x\n", PCI_REVISION(rval));

	rval = regs[o2i(PCI_BHLC_REG)];
	printf("    BIST: 0x%02x\n", PCI_BIST(rval));
	printf("    Header Type: 0x%02x%s (0x%02x)\n", PCI_HDRTYPE_TYPE(rval),
	    PCI_HDRTYPE_MULTIFN(rval) ? "+multifunction" : "",
	    PCI_HDRTYPE(rval));
	printf("    Latency Timer: 0x%02x\n", PCI_LATTIMER(rval));
	printf("    Cache Line Size: 0x%02x\n", PCI_CACHELINE(rval));
}

static int
pci_conf_print_bar(
#ifdef _KERNEL
    pci_chipset_tag_t pc, pcitag_t tag,
#endif
    const pcireg_t *regs, int reg, const char *name
#ifdef _KERNEL
    , int sizebar
#endif
    )
{
	int width;
	pcireg_t rval, rval64h;
#ifdef _KERNEL
	int s;
	pcireg_t mask, mask64h;
#endif

	width = 4;

	/*
	 * Section 6.2.5.1, `Address Maps', tells us that:
	 *
	 * 1) The builtin software should have already mapped the
	 * device in a reasonable way.
	 *
	 * 2) A device which wants 2^n bytes of memory will hardwire
	 * the bottom n bits of the address to 0.  As recommended,
	 * we write all 1s and see what we get back.
	 */

	rval = regs[o2i(reg)];
	if (PCI_MAPREG_TYPE(rval) == PCI_MAPREG_TYPE_MEM &&
	    PCI_MAPREG_MEM_TYPE(rval) == PCI_MAPREG_MEM_TYPE_64BIT) {
		rval64h = regs[o2i(reg + 4)];
		width = 8;
	} else
		rval64h = 0;

#ifdef _KERNEL
	/* XXX don't size unknown memory type? */
	if (rval != 0 && sizebar) {
		/*
		 * The following sequence seems to make some devices
		 * (e.g. host bus bridges, which don't normally
		 * have their space mapped) very unhappy, to
		 * the point of crashing the system.
		 *
		 * Therefore, if the mapping register is zero to
		 * start out with, don't bother trying.
		 */
		s = splhigh();
		pci_conf_write(pc, tag, reg, 0xffffffff);
		mask = pci_conf_read(pc, tag, reg);
		pci_conf_write(pc, tag, reg, rval);
		if (PCI_MAPREG_TYPE(rval) == PCI_MAPREG_TYPE_MEM &&
		    PCI_MAPREG_MEM_TYPE(rval) == PCI_MAPREG_MEM_TYPE_64BIT) {
			pci_conf_write(pc, tag, reg + 4, 0xffffffff);
			mask64h = pci_conf_read(pc, tag, reg + 4);
			pci_conf_write(pc, tag, reg + 4, rval64h);
		}
		splx(s);
	} else
		mask = 0;
#endif /* _KERNEL */

	printf("    Base address register at 0x%02x", reg);
	if (name)
		printf(" (%s)", name);
	printf("\n      ");
	if (rval == 0) {
		printf("not implemented(?)\n");
		return width;
	} 
	printf("type: ");
	if (PCI_MAPREG_TYPE(rval) == PCI_MAPREG_TYPE_MEM) {
		const char *type, *prefetch;

		switch (PCI_MAPREG_MEM_TYPE(rval)) {
		case PCI_MAPREG_MEM_TYPE_32BIT:
			type = "32-bit";
			break;
		case PCI_MAPREG_MEM_TYPE_32BIT_1M:
			type = "32-bit-1M";
			break;
		case PCI_MAPREG_MEM_TYPE_64BIT:
			type = "64-bit";
			break;
		default:
			type = "unknown (XXX)";
			break;
		}
		if (PCI_MAPREG_MEM_PREFETCHABLE(rval))
			prefetch = "";
		else
			prefetch = "non";
		printf("%s %sprefetchable memory\n", type, prefetch);
		switch (PCI_MAPREG_MEM_TYPE(rval)) {
		case PCI_MAPREG_MEM_TYPE_64BIT:
			printf("      base: 0x%016llx, ",
			    PCI_MAPREG_MEM64_ADDR(
				((((long long) rval64h) << 32) | rval)));
#ifdef _KERNEL
			if (sizebar)
				printf("size: 0x%016llx",
				    PCI_MAPREG_MEM64_SIZE(
				      ((((long long) mask64h) << 32) | mask)));
			else
#endif /* _KERNEL */
				printf("not sized");
			printf("\n");
			break;
		case PCI_MAPREG_MEM_TYPE_32BIT:
		case PCI_MAPREG_MEM_TYPE_32BIT_1M:
		default:
			printf("      base: 0x%08x, ",
			    PCI_MAPREG_MEM_ADDR(rval));
#ifdef _KERNEL
			if (sizebar)
				printf("size: 0x%08x",
				    PCI_MAPREG_MEM_SIZE(mask));
			else
#endif /* _KERNEL */
				printf("not sized");
			printf("\n");
			break;
		}
	} else {
#ifdef _KERNEL
		if (sizebar)
			printf("%d-bit ", mask & ~0x0000ffff ? 32 : 16);
#endif /* _KERNEL */
		printf("i/o\n");
		printf("      base: 0x%08x, ", PCI_MAPREG_IO_ADDR(rval));
#ifdef _KERNEL
		if (sizebar)
			printf("size: 0x%08x", PCI_MAPREG_IO_SIZE(mask));
		else
#endif /* _KERNEL */
			printf("not sized");
		printf("\n");
	}

	return width;
}

static void
pci_conf_print_regs(const pcireg_t *regs, int first, int pastlast)
{
	int off, needaddr, neednl;

	needaddr = 1;
	neednl = 0;
	for (off = first; off < pastlast; off += 4) {
		if ((off % 16) == 0 || needaddr) {
			printf("    0x%02x:", off);
			needaddr = 0;
		}
		printf(" 0x%08x", regs[o2i(off)]);
		neednl = 1;
		if ((off % 16) == 12) {
			printf("\n");
			neednl = 0;
		}
	}
	if (neednl)
		printf("\n");
}

static void
pci_conf_print_type0(
#ifdef _KERNEL
    pci_chipset_tag_t pc, pcitag_t tag,
#endif
    const pcireg_t *regs
#ifdef _KERNEL
    , int sizebars
#endif
    )
{
	int off, width;
	pcireg_t rval;

	for (off = PCI_MAPREG_START; off < PCI_MAPREG_END; off += width) {
#ifdef _KERNEL
		width = pci_conf_print_bar(pc, tag, regs, off, NULL, sizebars);
#else
		width = pci_conf_print_bar(regs, off, NULL);
#endif
	}

	printf("    Cardbus CIS Pointer: 0x%08x\n", regs[o2i(0x28)]);

	rval = regs[o2i(PCI_SUBSYS_ID_REG)];
	printf("    Subsystem vendor ID: 0x%04x\n", PCI_VENDOR(rval));
	printf("    Subsystem ID: 0x%04x\n", PCI_PRODUCT(rval));

	/* XXX */
	printf("    Expansion ROM Base Address: 0x%08x\n", regs[o2i(0x30)]);

	if (regs[o2i(PCI_COMMAND_STATUS_REG)] & PCI_STATUS_CAPLIST_SUPPORT)
		printf("    Capability list pointer: 0x%02x\n",
		    PCI_CAPLIST_PTR(regs[o2i(PCI_CAPLISTPTR_REG)]));
	else
		printf("    Reserved @ 0x34: 0x%08x\n", regs[o2i(0x34)]);

	printf("    Reserved @ 0x38: 0x%08x\n", regs[o2i(0x38)]);

	rval = regs[o2i(PCI_INTERRUPT_REG)];
	printf("    Maximum Latency: 0x%02x\n", (rval >> 24) & 0xff);
	printf("    Minimum Grant: 0x%02x\n", (rval >> 16) & 0xff);
	printf("    Interrupt pin: 0x%02x ", PCI_INTERRUPT_PIN(rval));
	switch (PCI_INTERRUPT_PIN(rval)) {
	case PCI_INTERRUPT_PIN_NONE:
		printf("(none)");
		break;
	case PCI_INTERRUPT_PIN_A:
		printf("(pin A)");
		break;
	case PCI_INTERRUPT_PIN_B:
		printf("(pin B)");
		break;
	case PCI_INTERRUPT_PIN_C:
		printf("(pin C)");
		break;
	case PCI_INTERRUPT_PIN_D:
		printf("(pin D)");
		break;
	default:
		printf("(? ? ?)");
		break;
	}
	printf("\n");
	printf("    Interrupt line: 0x%02x\n", PCI_INTERRUPT_LINE(rval));

	if (regs[o2i(PCI_COMMAND_STATUS_REG)] & PCI_STATUS_CAPLIST_SUPPORT) {
		for (off = PCI_CAPLIST_PTR(regs[o2i(PCI_CAPLISTPTR_REG)]);
		     off != 0;
		     off = PCI_CAPLIST_NEXT(regs[o2i(off)])) {
			rval = regs[o2i(off)];
			printf("    Capability register at 0x%02x\n", off);

			printf("      type: 0x%02x (", PCI_CAPLIST_CAP(rval));
			switch (PCI_CAPLIST_CAP(rval)) {
			case PCI_CAP_RESERVED0:
				printf("reserved");
				break;
			case PCI_CAP_PWRMGMT:
				printf("Power Management, rev. %d.0",
				    (rval >> 0) & 0x07); /* XXX not clear */
				break;
			case PCI_CAP_AGP:
				printf("AGP, rev. %d.%d",
				    (rval >> 24) & 0x0f,
				    (rval >> 20) & 0x0f);
				break;
			case PCI_CAP_VPD:
				printf("VPD");
				break;
			case PCI_CAP_SLOTID:
				printf("SlotID");
				break;
			case PCI_CAP_MBI:
				printf("MBI");
				break;
			case PCI_CAP_CPCI_HOTSWAP:
				printf("CompactPCI Hot-swapping");
				break;
			case PCI_CAP_PCIX:
				printf("PCI-X");
				break;
			case PCI_CAP_LDT:
				printf("LDT");
				break;
			case PCI_CAP_VENDSPEC:
				printf("Vendor-specific");
				break;
			case PCI_CAP_DEBUGPORT:
				printf("Debug Port");
				break;
			case PCI_CAP_CPCI_RSRCCTL:
				printf("CompactPCI Resource Control");
				break;
			case PCI_CAP_HOTPLUG:
				printf("Hot-Plug");
				break;
			default:
				printf("unknown");
			}
			printf(")\n");
		}
	}
}

static void
pci_conf_print_type1(
#ifdef _KERNEL
    pci_chipset_tag_t pc, pcitag_t tag,
#endif
    const pcireg_t *regs
#ifdef _KERNEL
    , int sizebars
#endif
    )
{
	int off, width;
	pcireg_t rval;

	/*
	 * XXX these need to be printed in more detail, need to be
	 * XXX checked against specs/docs, etc.
	 *
	 * This layout was cribbed from the TI PCI2030 PCI-to-PCI
	 * Bridge chip documentation, and may not be correct with
	 * respect to various standards. (XXX)
	 */

	for (off = 0x10; off < 0x18; off += width) {
#ifdef _KERNEL
		width = pci_conf_print_bar(pc, tag, regs, off, NULL, sizebars);
#else
		width = pci_conf_print_bar(regs, off, NULL);
#endif
	}

	printf("    Primary bus number: 0x%02x\n",
	    (regs[o2i(0x18)] >> 0) & 0xff);
	printf("    Secondary bus number: 0x%02x\n",
	    (regs[o2i(0x18)] >> 8) & 0xff);
	printf("    Subordinate bus number: 0x%02x\n",
	    (regs[o2i(0x18)] >> 16) & 0xff);
	printf("    Secondary bus latency timer: 0x%02x\n",
	    (regs[o2i(0x18)] >> 24) & 0xff);

	rval = (regs[o2i(0x1c)] >> 16) & 0xffff;
	printf("    Secondary status register: 0x%04x\n", rval); /* XXX bits */
	onoff("66 MHz capable", 0x0020);
	onoff("User Definable Features (UDF) support", 0x0040);
	onoff("Fast back-to-back capable", 0x0080);
	onoff("Data parity error detected", 0x0100);

	printf("      DEVSEL timing: ");
	switch (rval & 0x0600) {
	case 0x0000:
		printf("fast");
		break;
	case 0x0200:
		printf("medium");
		break;
	case 0x0400:
		printf("slow");
		break;
	default:
		printf("unknown/reserved");	/* XXX */
		break;
	}
	printf(" (0x%x)\n", (rval & 0x0600) >> 9);

	onoff("Signaled Target Abort", 0x0800);
	onoff("Received Target Abort", 0x1000);
	onoff("Received Master Abort", 0x2000);
	onoff("System Error", 0x4000);
	onoff("Parity Error", 0x8000);

	/* XXX Print more prettily */
	printf("    I/O region:\n");
	printf("      base register:  0x%02x\n", (regs[o2i(0x1c)] >> 0) & 0xff);
	printf("      limit register: 0x%02x\n", (regs[o2i(0x1c)] >> 8) & 0xff);
	printf("      base upper 16 bits register:  0x%04x\n",
	    (regs[o2i(0x30)] >> 0) & 0xffff);
	printf("      limit upper 16 bits register: 0x%04x\n",
	    (regs[o2i(0x30)] >> 16) & 0xffff);

	/* XXX Print more prettily */
	printf("    Memory region:\n");
	printf("      base register:  0x%04x\n",
	    (regs[o2i(0x20)] >> 0) & 0xffff);
	printf("      limit register: 0x%04x\n",
	    (regs[o2i(0x20)] >> 16) & 0xffff);

	/* XXX Print more prettily */
	printf("    Prefetchable memory region:\n");
	printf("      base register:  0x%04x\n",
	    (regs[o2i(0x24)] >> 0) & 0xffff);
	printf("      limit register: 0x%04x\n",
	    (regs[o2i(0x24)] >> 16) & 0xffff);
	printf("      base upper 32 bits register:  0x%08x\n", regs[o2i(0x28)]);
	printf("      limit upper 32 bits register: 0x%08x\n", regs[o2i(0x2c)]);

	printf("    Reserved @ 0x34: 0x%08x\n", regs[o2i(0x34)]);
	/* XXX */
	printf("    Expansion ROM Base Address: 0x%08x\n", regs[o2i(0x38)]);

	printf("    Interrupt line: 0x%02x\n",
	    (regs[o2i(0x3c)] >> 0) & 0xff);
	printf("    Interrupt pin: 0x%02x ",
	    (regs[o2i(0x3c)] >> 8) & 0xff);
	switch ((regs[o2i(0x3c)] >> 8) & 0xff) {
	case PCI_INTERRUPT_PIN_NONE:
		printf("(none)");
		break;
	case PCI_INTERRUPT_PIN_A:
		printf("(pin A)");
		break;
	case PCI_INTERRUPT_PIN_B:
		printf("(pin B)");
		break;
	case PCI_INTERRUPT_PIN_C:
		printf("(pin C)");
		break;
	case PCI_INTERRUPT_PIN_D:
		printf("(pin D)");
		break;
	default:
		printf("(? ? ?)");
		break;
	}
	printf("\n");
	rval = (regs[o2i(0x3c)] >> 16) & 0xffff;
	printf("    Bridge control register: 0x%04x\n", rval); /* XXX bits */
	onoff("Parity error response", 0x0001);
	onoff("Secondary SERR forwarding", 0x0002);
	onoff("ISA enable", 0x0004);
	onoff("VGA enable", 0x0008);
	onoff("Master abort reporting", 0x0020);
	onoff("Secondary bus reset", 0x0040);
	onoff("Fast back-to-back capable", 0x0080);
}

static void
pci_conf_print_type2(
#ifdef _KERNEL
    pci_chipset_tag_t pc, pcitag_t tag,
#endif
    const pcireg_t *regs
#ifdef _KERNEL
    , int sizebars
#endif
    )
{
	pcireg_t rval;

	/*
	 * XXX these need to be printed in more detail, need to be
	 * XXX checked against specs/docs, etc.
	 *
	 * This layout was cribbed from the TI PCI1130 PCI-to-CardBus
	 * controller chip documentation, and may not be correct with
	 * respect to various standards. (XXX)
	 */

#ifdef _KERNEL
	pci_conf_print_bar(pc, tag, regs, 0x10,
	    "CardBus socket/ExCA registers", sizebars);
#else
	pci_conf_print_bar(regs, 0x10, "CardBus socket/ExCA registers");
#endif

	printf("    Reserved @ 0x14: 0x%04x\n",
	    (regs[o2i(0x14)] >> 0) & 0xffff);
	rval = (regs[o2i(0x14)] >> 16) & 0xffff;
	printf("    Secondary status register: 0x%04x\n", rval);
	onoff("66 MHz capable", 0x0020);
	onoff("User Definable Features (UDF) support", 0x0040);
	onoff("Fast back-to-back capable", 0x0080);
	onoff("Data parity error detection", 0x0100);

	printf("      DEVSEL timing: ");
	switch (rval & 0x0600) {
	case 0x0000:
		printf("fast");
		break;
	case 0x0200:
		printf("medium");
		break;
	case 0x0400:
		printf("slow");
		break;
	default:
		printf("unknown/reserved");	/* XXX */
		break;
	}
	printf(" (0x%x)\n", (rval & 0x0600) >> 9);
	onoff("PCI target aborts terminate CardBus bus master transactions",
	    0x0800);
	onoff("CardBus target aborts terminate PCI bus master transactions",
	    0x1000);
	onoff("Bus initiator aborts terminate initiator transactions",
	    0x2000);
	onoff("System error", 0x4000);
	onoff("Parity error", 0x8000);

	printf("    PCI bus number: 0x%02x\n",
	    (regs[o2i(0x18)] >> 0) & 0xff);
	printf("    CardBus bus number: 0x%02x\n",
	    (regs[o2i(0x18)] >> 8) & 0xff);
	printf("    Subordinate bus number: 0x%02x\n",
	    (regs[o2i(0x18)] >> 16) & 0xff);
	printf("    CardBus latency timer: 0x%02x\n",
	    (regs[o2i(0x18)] >> 24) & 0xff);

	/* XXX Print more prettily */
	printf("    CardBus memory region 0:\n");
	printf("      base register:  0x%08x\n", regs[o2i(0x1c)]);
	printf("      limit register: 0x%08x\n", regs[o2i(0x20)]);
	printf("    CardBus memory region 1:\n");
	printf("      base register:  0x%08x\n", regs[o2i(0x24)]);
	printf("      limit register: 0x%08x\n", regs[o2i(0x28)]);
	printf("    CardBus I/O region 0:\n");
	printf("      base register:  0x%08x\n", regs[o2i(0x2c)]);
	printf("      limit register: 0x%08x\n", regs[o2i(0x30)]);
	printf("    CardBus I/O region 1:\n");
	printf("      base register:  0x%08x\n", regs[o2i(0x34)]);
	printf("      limit register: 0x%08x\n", regs[o2i(0x38)]);

	printf("    Interrupt line: 0x%02x\n",
	    (regs[o2i(0x3c)] >> 0) & 0xff);
	printf("    Interrupt pin: 0x%02x ",
	    (regs[o2i(0x3c)] >> 8) & 0xff);
	switch ((regs[o2i(0x3c)] >> 8) & 0xff) {
	case PCI_INTERRUPT_PIN_NONE:
		printf("(none)");
		break;
	case PCI_INTERRUPT_PIN_A:
		printf("(pin A)");
		break;
	case PCI_INTERRUPT_PIN_B:
		printf("(pin B)");
		break;
	case PCI_INTERRUPT_PIN_C:
		printf("(pin C)");
		break;
	case PCI_INTERRUPT_PIN_D:
		printf("(pin D)");
		break;
	default:
		printf("(? ? ?)");
		break;
	}
	printf("\n");
	rval = (regs[o2i(0x3c)] >> 16) & 0xffff;
	printf("    Bridge control register: 0x%04x\n", rval);
	onoff("Parity error response", 0x0001);
	onoff("CardBus SERR forwarding", 0x0002);
	onoff("ISA enable", 0x0004);
	onoff("VGA enable", 0x0008);
	onoff("CardBus master abort reporting", 0x0020);
	onoff("CardBus reset", 0x0040);
	onoff("Functional interrupts routed by ExCA registers", 0x0080);
	onoff("Memory window 0 prefetchable", 0x0100);
	onoff("Memory window 1 prefetchable", 0x0200);
	onoff("Write posting enable", 0x0400);

	rval = regs[o2i(0x40)];
	printf("    Subsystem vendor ID: 0x%04x\n", PCI_VENDOR(rval));
	printf("    Subsystem ID: 0x%04x\n", PCI_PRODUCT(rval));

#ifdef _KERNEL
	pci_conf_print_bar(pc, tag, regs, 0x44, "legacy-mode registers",
	    sizebars);
#else
	pci_conf_print_bar(regs, 0x44, "legacy-mode registers");
#endif
}

void
pci_conf_print(
#ifdef _KERNEL
    pci_chipset_tag_t pc, pcitag_t tag,
    void (*printfn)(pci_chipset_tag_t, pcitag_t, const pcireg_t *)
#else
    int pcifd, u_int bus, u_int dev, u_int func
#endif
    )
{
	pcireg_t regs[o2i(256)];
	int off, endoff, hdrtype;
	const char *typename;
#ifdef _KERNEL
	void (*typeprintfn)(pci_chipset_tag_t, pcitag_t, const pcireg_t *, int);
	int sizebars;
#else
	void (*typeprintfn)(const pcireg_t *);
#endif

	printf("PCI configuration registers:\n");

	for (off = 0; off < 256; off += 4) {
#ifdef _KERNEL
		regs[o2i(off)] = pci_conf_read(pc, tag, off);
#else
		if (pcibus_conf_read(pcifd, bus, dev, func, off,
		    &regs[o2i(off)]) == -1)
			regs[o2i(off)] = 0;
#endif
	}

#ifdef _KERNEL
	sizebars = 1;
	if (PCI_CLASS(regs[o2i(PCI_CLASS_REG)]) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(regs[o2i(PCI_CLASS_REG)]) == PCI_SUBCLASS_BRIDGE_HOST)
		sizebars = 0;
#endif

	/* common header */
	printf("  Common header:\n");
	pci_conf_print_regs(regs, 0, 16);

	printf("\n");
#ifdef _KERNEL
	pci_conf_print_common(pc, tag, regs);
#else
	pci_conf_print_common(regs);
#endif
	printf("\n");

	/* type-dependent header */
	hdrtype = PCI_HDRTYPE_TYPE(regs[o2i(PCI_BHLC_REG)]);
	switch (hdrtype) {		/* XXX make a table, eventually */
	case 0:
		/* Standard device header */
		typename = "\"normal\" device";
		typeprintfn = &pci_conf_print_type0;
		endoff = 64;
		break;
	case 1:
		/* PCI-PCI bridge header */
		typename = "PCI-PCI bridge";
		typeprintfn = &pci_conf_print_type1;
		endoff = 64;
		break;
	case 2:
		/* PCI-CardBus bridge header */
		typename = "PCI-CardBus bridge";
		typeprintfn = &pci_conf_print_type2;
		endoff = 72;
		break;
	default:
		typename = NULL;
		typeprintfn = 0;
		endoff = 64;
		break;
	}
	printf("  Type %d ", hdrtype);
	if (typename != NULL)
		printf("(%s) ", typename);
	printf("header:\n");
	pci_conf_print_regs(regs, 16, endoff);
	printf("\n");
	if (typeprintfn) {
#ifdef _KERNEL
		(*typeprintfn)(pc, tag, regs, sizebars);
#else
		(*typeprintfn)(regs);
#endif
	} else
		printf("    Don't know how to pretty-print type %d header.\n",
		    hdrtype);
	printf("\n");

	/* device-dependent header */
	printf("  Device-dependent header:\n");
	pci_conf_print_regs(regs, endoff, 256);
	printf("\n");
#ifdef _KERNEL
	if (printfn)
		(*printfn)(pc, tag, regs);
	else
		printf("    Don't know how to pretty-print device-dependent header.\n");
	printf("\n");
#endif /* _KERNEL */
}
