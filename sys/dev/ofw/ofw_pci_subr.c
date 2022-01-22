/*	$NetBSD: ofw_pci_subr.c,v 1.3 2022/01/22 11:49:18 thorpej Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ofw_pci_subr.c,v 1.3 2022/01/22 11:49:18 thorpej Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/errno.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pci_calls.h>

#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_pci.h>

static int
ofw_pci_bus_get_child_devhandle(device_t dev, devhandle_t call_handle, void *v)
{
	struct pci_bus_get_child_devhandle_args *args = v;
	int phandle = devhandle_to_of(call_handle);
	struct ofw_pci_register opr;
	int d, f, len;
	uint32_t phys_hi;

	/*
	 * No need to compare the bus number; we are searching
	 * only direct children of the specified node.  Skipping
	 * the bus number comparison allows us to dodge a slight
	 * difference between the OpenFirmware and FDT PCI bindings
	 * as it relates to PCI-PCI bridges.
	 */

	pci_decompose_tag(args->pc, args->tag, NULL, &d, &f);

	for (phandle = OF_child(phandle); phandle != 0;
	     phandle = OF_peer(phandle)) {
		len = OF_getprop(phandle, "reg", &opr, sizeof(opr));
		if (len < sizeof(opr)) {
			continue;
		}

		phys_hi = be32toh(opr.phys_hi);

		if (d != OFW_PCI_PHYS_HI_DEVICE(phys_hi) ||
		    f != OFW_PCI_PHYS_HI_FUNCTION(phys_hi)) {
			continue;
		}

		/* Found it! */
		args->devhandle = devhandle_from_of(call_handle, phandle);
		return 0;
	}

	return ENODEV;
}
OF_DEVICE_CALL_REGISTER(PCI_BUS_GET_CHILD_DEVHANDLE_STR,
			ofw_pci_bus_get_child_devhandle)
