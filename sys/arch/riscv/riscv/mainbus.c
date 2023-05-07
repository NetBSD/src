/*	$NetBSD: mainbus.c,v 1.6 2023/05/07 12:41:49 skrll Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include "opt_console.h"
#include "opt_efi.h"
#include "opt_modular.h"

#include <sys/cdefs.h>

__RCSID("$NetBSD: mainbus.c,v 1.6 2023/05/07 12:41:49 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#ifdef CONSADDR
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif

#include <machine/sysreg.h>

extern struct bus_space riscv_generic_bs_tag;

bus_space_tag_t
fdtbus_bus_tag_create(int phandle, uint32_t flags)
{
        return &riscv_generic_bs_tag;
}

static inline bool
cpu_earlydevice_va_p(void)
{

	return __SHIFTOUT(csr_satp_read(), SATP_MODE);
}

void com_platform_early_putchar(char);

void __noasan
com_platform_early_putchar(char c)
{
#ifdef CONSADDR
#define CONSADDR_VA	(VM_KERNEL_IO_BASE + (CONSADDR & SEGOFSET))

	volatile uint8_t *uartaddr = cpu_earlydevice_va_p() ?
	    (volatile uint8_t *)CONSADDR_VA :
	    (volatile uint8_t *)CONSADDR;

	while ((uartaddr[com_lsr] & LSR_TXRDY) == 0)
		;

	uartaddr[com_data] = c;
#endif
}

static int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	static int once = 0;

	if (once != 0)
		return 0;
	once = 1;

	return 1;
}


static void
mainbus_attach_devicetree(device_t self)
{
	struct fdt_attach_args faa = {
		.faa_name = "",
		.faa_phandle = OF_peer(0),
		.faa_bst = &riscv_generic_bs_tag,
	};

	aprint_normal("\n");

	config_found(self, &faa, NULL, CFARGS(.iattr = "fdt"));
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	if (fdtbus_get_data() != NULL) {
		mainbus_attach_devicetree(self);
	}
}

CFATTACH_DECL_NEW(mainbus, 0, mainbus_match, mainbus_attach, NULL, NULL);
