/*	$NetBSD: vrc4173bcuvar.h,v 1.1 2001/06/13 07:32:48 enami Exp $	*/

/*-
 * Copyright (c) 2001 Enami Tsugutomo.
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
 */

#include <dev/pci/pcivar.h>

struct vrc4173bcu_softc {
	struct device sc_dev;

	pci_chipset_tag_t sc_pc;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_size_t sc_size;

	bus_space_handle_t sc_icuh;	/* I/O handle for ICU. */
	void *sc_ih;

#define	VRC4173BCU_NINTRHAND	(16)	/* XXX */
	struct intrhand {
		int (*ih_func)(void *);
		void *ih_arg;
	} sc_intrhand[VRC4173BCU_NINTRHAND];
};

struct vrc4173bcu_attach_args {
	bus_space_tag_t vaa_iot;
	bus_space_handle_t vaa_ioh;
};

int	vrc4173bcu_intr(void *);
void	*vrc4173bcu_intr_establish(struct vrc4173bcu_softc *, int,
	    int (*)(void *), void *);
void	vrc4173bcu_intr_disestablish(struct vrc4173bcu_softc *, void *);
int	vrc4173bcu_pci_bus_devorder(pci_chipset_tag_t, int, char *);
int	vrc4173bcu_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *
	vrc4173bcu_pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t);
const struct evcnt *
	vrc4173bcu_pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
