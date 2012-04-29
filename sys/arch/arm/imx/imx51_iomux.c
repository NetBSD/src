/*	$NetBSD: imx51_iomux.c,v 1.2.6.1 2012/04/29 23:04:38 mrg Exp $	*/

/*
 * Copyright (c) 2009, 2010  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx51_iomux.c,v 1.2.6.1 2012/04/29 23:04:38 mrg Exp $");

#define	_INTR_PRIVATE

#include "locators.h"

#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#include <sys/bus.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>

struct iomux_softc {
	struct device iomux_dev;
	bus_space_tag_t iomux_memt;
	bus_space_handle_t iomux_memh;
};

#define	IOMUX_READ(iomux, reg) \
	bus_space_read_4((iomux)->iomux_memt, (iomux)->iomux_memh, (reg))
#define	IOMUX_WRITE(iomux, reg, val) \
	bus_space_write_4((iomux)->iomux_memt, (iomux)->iomux_memh, (reg), (val))

static int iomux_match(device_t, cfdata_t, void *);
static void iomux_attach(device_t, device_t, void *);

static struct iomux_softc *iomuxsc = NULL;

CFATTACH_DECL_NEW(imxiomux, sizeof(struct iomux_softc),
    iomux_match, iomux_attach, NULL, NULL);

int
iomux_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct axi_attach_args *axia = aux;

	if (axia->aa_addr != IOMUXC_BASE)
		return 0;

	return 1;
}

void
iomux_attach(device_t parent, device_t self, void *aux)
{
	struct axi_attach_args * const axia = aux;
	struct iomux_softc * const iomux = device_private(self);
	int error;

	if (axia->aa_size == AXICF_SIZE_DEFAULT)
		axia->aa_size = IOMUXC_SIZE;

	iomux->iomux_memt = axia->aa_iot;
	error = bus_space_map(axia->aa_iot, axia->aa_addr, axia->aa_size,
			      0, &iomux->iomux_memh);

	if (error) {
		aprint_error(": failed to map register %#lx@%#lx: %d\n",
		    axia->aa_size, axia->aa_addr, error);
		return;
	}

	aprint_normal("\n");

	iomuxsc = iomux;
}

static void
iomux_set_function_sub(struct iomux_softc *sc, uint32_t pin, uint32_t fn)
{
	bus_size_t mux_ctl_reg = IOMUX_PIN_TO_MUX_ADDRESS(pin);

	if (mux_ctl_reg != IOMUX_MUX_NONE)
		bus_space_write_4(sc->iomux_memt, sc->iomux_memh,
		    mux_ctl_reg, fn);
}

void
iomux_set_function(unsigned int pin, unsigned int fn)
{
	iomux_set_function_sub(iomuxsc, pin, fn);
}


static void
iomux_set_pad_sub(struct iomux_softc *sc, uint32_t pin, uint32_t config)
{
	bus_size_t pad_ctl_reg = IOMUX_PIN_TO_PAD_ADDRESS(pin);

	if (pad_ctl_reg != IOMUX_PAD_NONE)
		bus_space_write_4(sc->iomux_memt, sc->iomux_memh,
		    pad_ctl_reg, config);
}

void
iomux_set_pad(unsigned int pin, unsigned int config)
{
	iomux_set_pad_sub(iomuxsc, pin, config);
}

#if 0
void
iomux_set_input(unsigned int input, unsigned int config)
{
	bus_size_t input_ctl_reg = input;

	bus_space_write_4(iomuxsc->iomux_memt, iomuxsc->iomux_memh,
	    input_ctl_reg, config);
}
#endif

void
iomux_mux_config(const struct iomux_conf *conflist)
{
	int i;

	for (i = 0; conflist[i].pin != IOMUX_CONF_EOT; i++) {
		iomux_set_pad_sub(iomuxsc, conflist[i].pin, conflist[i].pad);
		iomux_set_function_sub(iomuxsc, conflist[i].pin,
		    conflist[i].mux);
	}
}

#if 0
void
iomux_input_config(const struct iomux_input_conf *conflist)
{
	int i;

	for (i = 0; conflist[i].inout != -1; i++) {
		iomux_set_inout(iomuxsc, conflist[i].inout,
		    conflist[i].inout_mode);
	}
}
#endif

