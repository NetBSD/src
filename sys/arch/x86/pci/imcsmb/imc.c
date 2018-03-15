/* $NetBSD: imc.c,v 1.2 2018/03/15 23:57:17 maya Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
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

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Authors: Joe Kloss; Ravi Pokala (rpokala@freebsd.org)
 *
 * Copyright (c) 2017-2018 Panasas
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

/*
 * Driver to expose the SMBus controllers in Intel's Integrated
 * Memory Controllers in certain CPUs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imc.c,v 1.2 2018/03/15 23:57:17 maya Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include "imcsmb_reg.h"
#include "imcsmb_var.h"

#include "ioconf.h"

/* (Sandy,Ivy)bridge-Xeon and (Has,Broad)well-Xeon CPUs contain one or two
 * "Integrated Memory Controllers" (iMCs), and each iMC contains two separate
 * SMBus controllers. These are used for reading SPD data from the DIMMs, and
 * for reading the "Thermal Sensor on DIMM" (TSODs). The iMC SMBus controllers
 * are very simple devices, and have limited functionality compared to
 * full-fledged SMBus controllers, like the one in Intel ICHs and PCHs.
 *
 * The publicly available documentation for the iMC SMBus controllers can be
 * found in the CPU datasheets for (Sandy,Ivy)bridge-Xeon and
 * (Has,broad)well-Xeon, respectively:
 *
 * https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/
 *      Sandybridge     xeon-e5-1600-2600-vol-2-datasheet.pdf
 *      Ivybridge       xeon-e5-v2-datasheet-vol-2.pdf
 *      Haswell         xeon-e5-v3-datasheet-vol-2.pdf
 *      Broadwell       xeon-e5-v4-datasheet-vol-2.pdf
 *
 * Another useful resource is the Linux driver. It is not in the main tree.
 *
 * https://www.mail-archive.com/linux-kernel@vger.kernel.org/msg840043.html
 *
 * The iMC SMBus controllers do not support interrupts (thus, they must be
 * polled for IO completion). All of the iMC registers are in PCI configuration
 * space; there is no support for PIO or MMIO. As a result, this driver does
 * not need to perform and newbus resource manipulation.
 *
 * Because there are multiple SMBus controllers sharing the same PCI device,
 * this driver is actually *two* drivers:
 *
 * - "imcsmb" is an smbus(4)-compliant SMBus controller driver
 *
 * - "imcsmb_pci" recognizes the PCI device and assigns the appropriate set of
 *    PCI config registers to a specific "imcsmb" instance.
 */

/* Depending on the motherboard and firmware, the TSODs might be polled by
 * firmware. Therefore, when this driver accesses these SMBus controllers, the
 * firmware polling must be disabled as part of requesting the bus, and
 * re-enabled when releasing the bus. Unfortunately, the details of how to do
 * this are vendor-specific. Contact your motherboard vendor to get the
 * information you need to do proper implementations.
 *
 * For NVDIMMs which conform to the ACPI "NFIT" standard, the ACPI firmware
 * manages the NVDIMM; for those which pre-date the standard, the operating
 * system interacts with the NVDIMM controller using a vendor-proprietary API
 * over the SMBus. In that case, the NVDIMM driver would be an SMBus slave
 * device driver, and would interface with the hardware via an SMBus controller
 * driver such as this one.
 */

/* PCIe device IDs for (Sandy,Ivy)bridge)-Xeon and (Has,Broad)well-Xeon */

#define IMCSMB_PCI_DEV_ID_IMC0_SBX	0x3ca8
#define IMCSMB_PCI_DEV_ID_IMC0_IBX	0x0ea8
#define IMCSMB_PCI_DEV_ID_IMC0_HSX	PCI_PRODUCT_INTEL_XE5_V3_IMC0_MAIN
#define IMCSMB_PCI_DEV_ID_IMC0_BDX	PCI_PRODUCT_INTEL_XEOND_MEM_0_TTR_1

/* (Sandy,Ivy)bridge-Xeon only have a single memory controller per socket */

#define IMCSMB_PCI_DEV_ID_IMC1_HSX	PCI_PRODUCT_INTEL_XE5_V3_IMC1_MAIN
#define IMCSMB_PCI_DEV_ID_IMC1_BDX	PCI_PRODUCT_INTEL_COREI76K_IMC_0

/* There are two SMBus controllers in each device. These define the registers
 * for each of these devices.
 */
static struct imcsmb_reg_set imcsmb_regs[] = {
	{
		.smb_stat = IMCSMB_REG_STATUS0,
		.smb_cmd  = IMCSMB_REG_COMMAND0,
		.smb_cntl = IMCSMB_REG_CONTROL0
	},
	{
		.smb_stat = IMCSMB_REG_STATUS1,
		.smb_cmd  = IMCSMB_REG_COMMAND1,
		.smb_cntl = IMCSMB_REG_CONTROL1
	},
};

static struct imcsmb_pci_device {
	uint16_t	id;
	const char	*name;
} imcsmb_pci_devices[] = {
	{IMCSMB_PCI_DEV_ID_IMC0_SBX,
	    "Intel Sandybridge Xeon iMC 0 SMBus controllers"	},
	{IMCSMB_PCI_DEV_ID_IMC0_IBX,
	    "Intel Ivybridge Xeon iMC 0 SMBus controllers"	},
	{IMCSMB_PCI_DEV_ID_IMC0_HSX,
	    "Intel Haswell Xeon iMC 0 SMBus controllers"	},
	{IMCSMB_PCI_DEV_ID_IMC1_HSX,
	    "Intel Haswell Xeon iMC 1 SMBus controllers"	},
	{IMCSMB_PCI_DEV_ID_IMC0_BDX,
	    "Intel Broadwell Xeon iMC 0 SMBus controllers"	},
	{IMCSMB_PCI_DEV_ID_IMC1_BDX,
	    "Intel Broadwell Xeon iMC 1 SMBus controllers"	},
	{0, NULL},
};

/* Device methods. */
static void imc_attach(device_t, device_t, void *);
static int  imc_rescan(device_t, const char *, const int *);
static int  imc_detach(device_t, int);
static int  imc_probe(device_t, cfdata_t, void *);
static void imc_chdet(device_t, device_t);

CFATTACH_DECL3_NEW(imc, sizeof(struct imc_softc),
    imc_probe, imc_attach, imc_detach, NULL, imc_rescan, imc_chdet, 0);

/**
 * device_attach() method. Set up the PCI device's softc, then explicitly create
 * children for the actual imcsmbX controllers. Set up the child's ivars to
 * point to the proper set of the PCI device's config registers. Finally, probe
 * and attach anything which might be downstream.
 *
 * @author Joe Kloss, rpokala
 *
 * @param[in,out] dev
 *      Device being attached.
 */
static void
imc_attach(device_t parent, device_t self, void *aux)
{
	struct imc_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	int flags, i;

	sc->sc_dev = self;
	sc->sc_pci_tag = pa->pa_tag;
	sc->sc_pci_chipset_tag = pa->pa_pc;

	pci_aprint_devinfo(pa, NULL);

	for (i = 0; imcsmb_pci_devices[i].id != 0; i++) {
		if (PCI_PRODUCT(pa->pa_id) == imcsmb_pci_devices[i].id) {
			aprint_normal_dev(self, "%s\n",
			    imcsmb_pci_devices[i].name);
			break;
		}
	}

	flags = 0;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	imc_rescan(self, "imc", &flags);
}

/* Create the imcsmbX children */

static int
imc_rescan(device_t self, const char * ifattr, const int *flags)
{
	struct imc_softc *sc = device_private(self);
	struct imc_attach_args imca;
	device_t child;
	int unit;

	for (unit = 0; unit < 2; unit++) {
		if (sc->sc_smbchild[unit] != NULL)
			continue;

		imca.ia_unit = unit;
		imca.ia_regs = &imcsmb_regs[unit];
		imca.ia_pci_tag = sc->sc_pci_tag;
		imca.ia_pci_chipset_tag = sc->sc_pci_chipset_tag;
		child = config_found_ia(self, "imc", &imca, NULL);

		if (child == NULL) {
			aprint_debug_dev(self, "Child %d imcsmb not added\n",
			    unit);
		}
		sc->sc_smbchild[unit] = child;
	}

	return 0;
}

/*
 * device_detach() method. attach() didn't do any allocations, so there's
 * nothing special needed
 */
static int
imc_detach(device_t self, int flags)
{
	struct imc_softc *sc = device_private(self);
	int i, error;

	for (i = 0; i < 2; i++) {
		if (sc->sc_smbchild[i] != NULL) {
			error = config_detach(sc->sc_smbchild[i], flags);     
			if (error)
				return error;
		}
	}

	pmf_device_deregister(self);
	return 0;
}

/**
 * device_probe() method. Look for the right PCI vendor/device IDs.
 *
 * @author Joe Kloss, rpokala
 *
 * @param[in,out] dev
 *      Device being probed.
 */
static int
imc_probe(device_t dev, cfdata_t cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL) {
		switch(PCI_PRODUCT(pa->pa_id)) {
		case  PCI_PRODUCT_INTEL_COREI76K_IMC_0:
		case  PCI_PRODUCT_INTEL_XEOND_MEM_0_TTR_1:
		case  PCI_PRODUCT_INTEL_XE5_V3_IMC0_MAIN:
		case  PCI_PRODUCT_INTEL_XE5_V3_IMC1_MAIN:
		case  PCI_PRODUCT_INTEL_E5_IMC_TA:
		case  PCI_PRODUCT_INTEL_E5V2_IMC_TA:
			return 1;
		}
	}
	return 0;
}

/*
 * child_detach() method
 */
static void
imc_chdet(device_t self, device_t child)
{
	struct imc_softc *sc = device_private(self);
	int unit;

	for (unit = 0; unit < 2; unit++)
		if (sc->sc_smbchild[unit] == child)
			sc->sc_smbchild[unit] = NULL;
	return;
}

/*
 * bios/motherboard access control
 *
 * XXX
 * If necessary, add the code here to prevent concurrent access to the
 * IMC controllers.  The softc argument is for the imcsmb child device
 * (for the specific i2cbus instance); if you need to disable all
 * i2cbus instances on a given IMC (or all instances on all IMCs), you
 * may need to examine the softc's parent.
 * XXX
 */

kmutex_t imc_access_mutex;
static int imc_access_count = 0;

void
imc_callback(struct imcsmb_softc *sc, imc_bios_control action)
{

	mutex_enter(&imc_access_mutex);
	switch (action) {
	case IMC_BIOS_ENABLE:
		imc_access_count--;
		if (imc_access_count == 0) {
			/*
			 * Insert motherboard-specific enable code here!
			 */
		}
		break;
	case IMC_BIOS_DISABLE:
		if (imc_access_count == 0) {
			/*
			 * Insert motherboard-specific disable code here!
			 */
		}
		imc_access_count++;
		break;
	}
	mutex_exit(&imc_access_mutex);
}

MODULE(MODULE_CLASS_DRIVER, imc, "pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
imc_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		mutex_init(&imc_access_mutex, MUTEX_DEFAULT, IPL_NONE);
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_imc,
		    cfattach_ioconf_imc, cfdata_ioconf_imc);
#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_imc,
		    cfattach_ioconf_imc, cfdata_ioconf_imc);
#endif
		if (error == 0)
			mutex_destroy(&imc_access_mutex);
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
