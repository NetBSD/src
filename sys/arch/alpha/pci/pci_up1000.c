/* $NetBSD: pci_up1000.c,v 1.17 2021/06/19 16:59:07 thorpej Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_up1000.c,v 1.17 2021/06/19 16:59:07 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <alpha/pci/irongatevar.h>

#include <alpha/pci/siovar.h>
#include <alpha/pci/sioreg.h>

#include "sio.h"

static int	api_up1000_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

static void
pci_up1000_pickintr(void *core, bus_space_tag_t iot, bus_space_tag_t memt,
    pci_chipset_tag_t pc)
{
#if NSIO
	pc->pc_intr_v = core;
	pc->pc_intr_map = api_up1000_intr_map;
	pc->pc_intr_string = sio_pci_intr_string;
	pc->pc_intr_evcnt = sio_pci_intr_evcnt;
	pc->pc_intr_establish = sio_pci_intr_establish;
	pc->pc_intr_disestablish = sio_pci_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    sio_pciide_compat_intr_establish;

	sio_intr_setup(pc, iot);
#else
	panic("pci_up1000_pickintr: no I/O interrupt handler (no sio)");
#endif
}
ALPHA_PCI_INTR_INIT(ST_API_NAUTILUS, pci_up1000_pickintr)

static int
api_up1000_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	int buspin = pa->pa_intrpin;
	int bustag = pa->pa_intrtag;
	int line = pa->pa_intrline;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("%s: bad interrupt pin %d\n", __func__, buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("%s: no mapping for %d/%d/%d\n", __func__,
		    bus, device, function);
		return (1);
	}

	if (line < 0 || line > 15) {
		printf("%s: bad ISA IRQ (%d)\n", __func__, line);
		return (1);
	}
	if (line == 2) {
		printf("%s: changed IRQ 2 to IRQ 9\n", __func__);
		line = 9;
	}

	alpha_pci_intr_handle_init(ihp, line, 0);
	return (0);
}
