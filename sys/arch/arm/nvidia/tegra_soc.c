/* $NetBSD: tegra_soc.c,v 1.14.2.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_tegra.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_soc.c,v 1.14.2.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <arm/bootconfig.h>
#include <arm/cpufunc.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_apbreg.h>
#include <arm/nvidia/tegra_mcreg.h>
#include <arm/nvidia/tegra_var.h>

bus_space_handle_t tegra_host1x_bsh;
bus_space_handle_t tegra_ppsb_bsh;
bus_space_handle_t tegra_apb_bsh;
bus_space_handle_t tegra_ahb_a2_bsh;

void
tegra_bootstrap(void)
{
	if (bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_HOST1X_BASE, TEGRA_HOST1X_SIZE, 0,
	    &tegra_host1x_bsh) != 0)
		panic("couldn't map HOST1X");
	if (bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_PPSB_BASE, TEGRA_PPSB_SIZE, 0,
	    &tegra_ppsb_bsh) != 0)
		panic("couldn't map PPSB");
	if (bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_APB_BASE, TEGRA_APB_SIZE, 0,
	    &tegra_apb_bsh) != 0)
		panic("couldn't map APB");
	if (bus_space_map(&armv7_generic_bs_tag,
	    TEGRA_AHB_A2_BASE, TEGRA_AHB_A2_SIZE, 0,
	    &tegra_ahb_a2_bsh) != 0)
		panic("couldn't map AHB A2");
}
