/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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
__KERNEL_RCSID(1, "$NetBSD: com_pci.c,v 1.1.2.1 2011/12/24 01:57:54 matt Exp $");

#include "opt_com.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/termios.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <mips/rmi/rmixl_comvar.h>
#include <mips/rmi/rmixlvar.h>

static int com_pci_match(device_t, cfdata_t, void *);
static void com_pci_attach(device_t, device_t, void *);
static void com_pci_initmap(struct com_regs *);

CFATTACH_DECL_NEW(com_pci, sizeof(struct com_softc),
    com_pci_match, com_pci_attach, NULL, NULL);

/* span of UART regs in bytes */
#define COM_PCI_UART_SIZE	(COM_NPORTS * sizeof(uint32_t))

#define COM_PCI_INIT_REGS(regs, bst, bsh, addr) 	\
	do {						\
		memset(&regs, 0, sizeof(regs));		\
		COM_INIT_REGS(regs, bst, bsh, addr);	\
		regs.cr_nports = COM_PCI_UART_SIZE;	\
		com_pci_initmap(&regs);		\
	} while (0)

int
com_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct pci_attach_args * const pa = aux;

	if (pa->pa_id != PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_UART)) {
		return 0;
	}

	return 1;
}

void
com_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	bus_space_tag_t bst = &rcp->rc_pci_ecfg_eb_memt;
	bus_space_handle_t bsh0 = rcp->rc_pci_ecfg_eb_memh;
	struct pci_attach_args * const pa = aux;
	struct com_softc * const sc = device_private(self);
	bus_addr_t base = pa->pa_tag | 0x100;
	bus_size_t size = COM_PCI_UART_SIZE;
	pci_intr_handle_t pcih;
	bus_space_handle_t bsh;
	prop_number_t prop;

	sc->sc_dev = self;

	prop = prop_dictionary_get(device_properties(sc->sc_dev), "frequency");
	if (prop == NULL) {
		aprint_error(": unable to get frequency property\n");
		return;
	}
	KASSERT(prop_object_type(prop) == PROP_TYPE_NUMBER);

	sc->sc_frequency = (int)prop_number_integer_value(prop);

	if (!com_is_console(bst, base, &bsh)
	    && bus_space_subregion(bst, bsh0, base, size, &bsh) != 0) {
		aprint_error(": can't map registers\n");
		return;
	}

	COM_PCI_INIT_REGS(sc->sc_regs, bst, bsh, base);

	com_attach_subr(sc);

	pci_intr_map(pa, &pcih);

	if (pci_intr_establish(pa->pa_pc, pcih, IPL_VM, comintr, sc) == NULL) {
		aprint_error_dev(self, "failed to establish interrupt\n");
	} else {
		const char * const intrstr = pci_intr_string(pa->pa_pc, pcih);
		aprint_normal_dev(self, "interrupting at %s\n", intrstr);
	}
}

void
com_pci_initmap(struct com_regs *regsp)
{
	/*
	 * map the 4 byte register stride
	 */
	for (size_t i = 0; i < __arraycount(regsp->cr_map); i++) {
		regsp->cr_map[i] = com_std_map[i] * 4;
#if _BYTE_ORDER == _BIG_ENDIAN
		regsp->cr_map[i] += 3;
#endif
	}
}

void
com_pci_cnattach(pcitag_t tag, int speed, int freq, int type, tcflag_t mode)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	bus_space_tag_t bst = &rcp->rc_pci_ecfg_eb_memt;
	struct com_regs	regs;
	bus_addr_t addr = tag | 0x100;

	KASSERT(tag == RMIXLP_UART1_PCITAG || tag == RMIXLP_UART2_PCITAG);

	COM_PCI_INIT_REGS(regs, bst, 0, addr);

	comcnattach1(&regs, speed, freq, type, mode);
}
