/* $NetBSD: a64gic_mainbus.c,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

__KERNEL_RCSID(1, "$NetBSD: a64gic_mainbus.c,v 1.1.4.2 2014/08/20 00:02:39 tls Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <aarch64/locore.h>

#include <arm/cortex/gic_reg.h>
#include <arm/cortex/gic_var.h>

static bool found;

static int
a64gic_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct mainbus_attach_args * const mba = aux;

	if (mba->mba_addr != MAINBUSCF_ADDR_DEFAULT
	    || mba->mba_size != MAINBUSCF_SIZE_DEFAULT
	    || mba->mba_intr != MAINBUSCF_INTR_DEFAULT
	    || mba->mba_intrbase != MAINBUSCF_INTRBASE_DEFAULT
	    || mba->mba_intrbase != 0
	    || mba->mba_package != MAINBUSCF_PACKAGE_DEFAULT)
		return 0;

	return !found;
}

static void
a64gic_attach(device_t parent, device_t self, void *aux)
{
	const struct mainbus_attach_args * const mba = aux;
	const bus_addr_t cbar = reg_cbar_el1_read();
	prop_dictionary_t dict = device_properties(self);
	bus_space_tag_t memt = mba->mba_memt;
	bus_space_handle_t gicch, gicdh;
	uint32_t gicc_off, gicd_off;
	
	if (!prop_dictionary_get_uint32(dict, "gicc_offset", &gicc_off))
		panic(": gicc_offset missing");
	if (!prop_dictionary_get_uint32(dict, "gicd_offset", &gicd_off))
		panic(": gicd_offset missing");
	
	found = 1;

	if (bus_space_map(memt, cbar + gicc_off, 0x1000, 0, &gicch)
	    || bus_space_map(memt, cbar + gicd_off, 0x1000, 0, &gicdh))
		panic(": failed to map GIC registers\n");

        armgic_common_attach(self, memt, gicch, gicdh); 
}
 
CFATTACH_DECL_NEW(a64gic, 0, a64gic_match, a64gic_attach, NULL, NULL);
