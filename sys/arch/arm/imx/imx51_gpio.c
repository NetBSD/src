/*	$NetBSD: imx51_gpio.c,v 1.3 2014/07/25 07:49:56 hkenken Exp $ */

/* derived from imx31_gpio.c */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas
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
__KERNEL_RCSID(0, "$NetBSD: imx51_gpio.c,v 1.3 2014/07/25 07:49:56 hkenken Exp $");

#include "opt_imx.h"

#include "locators.h"
#include "gpio.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>

#include <machine/intr.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <sys/bus.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/pic/picvar.h>

#include <arm/imx/imxgpioreg.h>
#include <arm/imx/imxgpiovar.h>

#if NGPIO > 0
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>
#endif

const int imxgpio_ngroups = GPIO_NGROUPS;

int
imxgpio_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct axi_attach_args *aa = aux;

	switch(aa->aa_addr) {
	case GPIO1_BASE:
	case GPIO2_BASE:
	case GPIO3_BASE:
	case GPIO4_BASE:
#ifdef IMX50
	case GPIO5_BASE:
	case GPIO6_BASE:
#endif
		return 1;
	}

	return 0;
}

void
imxgpio_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * const aa = aux;
	bus_space_handle_t ioh;
	int error;

	if (aa->aa_irq == AXICF_IRQ_DEFAULT &&
	    aa->aa_irqbase != AXICF_IRQBASE_DEFAULT) {
		aprint_error_dev(self, "missing intr in config\n");
		return;
	}
	if (aa->aa_irq != AXICF_IRQ_DEFAULT &&
	    aa->aa_irqbase == AXICF_IRQBASE_DEFAULT) {
		aprint_error_dev(self, "missing irqbase in config\n");
		return;
	}
		
	if (aa->aa_size == AXICF_SIZE_DEFAULT)
		aa->aa_size = GPIO_SIZE;

	error = bus_space_map(aa->aa_iot, aa->aa_addr, aa->aa_size,
	    0, &ioh);

	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    aa->aa_size, aa->aa_addr, error);
		return;
	}

	imxgpio_attach_common(self, aa->aa_iot, ioh,
	    (aa->aa_addr - GPIO1_BASE) / 0x4000,
	    aa->aa_irq, aa->aa_irqbase);
}

