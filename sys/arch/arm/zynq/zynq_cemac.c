/*	$NetBSD: zynq_cemac.c,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: zynq_cemac.c,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $");

#include "opt_zynq.h"

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <machine/intr.h>

#include <arm/zynq/zynq7000_var.h>
#include <arm/zynq/zynq7000_reg.h>

#include <dev/cadence/cemacreg.h>
#include <dev/cadence/if_cemacvar.h>

int
cemac_match(device_t parent, cfdata_t cfdata, void *aux)
{

	return cemac_match_common(parent, cfdata, aux);
}

void
cemac_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * const aa = aux;
	bus_space_handle_t ioh;
	int error;

	if (aa->aa_irq == AXICF_IRQ_DEFAULT) {
		aprint_error_dev(self, "missing intr in config\n");
		return;
	}
	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = ETH_SIZE;

	error = bus_space_map(aa->aa_iot, aa->aa_addr, aa->aa_size, 0, &ioh);
	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    aa->aa_size, aa->aa_addr, error);
		return;
	}

	void *ih = intr_establish(aa->aa_irq, IPL_NET, IST_EDGE,
	    cemac_intr, device_private(self));
	if (ih == NULL)
		aprint_error_dev(self, "intr_establish failed\n");

	cemac_attach_common(self, aa->aa_iot, ioh, aa->aa_dmat, CEMAC_FLAG_GEM);
}

