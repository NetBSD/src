/* $NetBSD: riscv_platform.c,v 1.2 2023/06/12 19:04:13 skrll Exp $ */

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

/*
 * This is the default RISC-V FDT platform implementation. It assumes the
 * following:
 *
 *  - Console UART is pre-configured by firmware
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: riscv_platform.c,v 1.2 2023/06/12 19:04:13 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <uvm/uvm_extern.h>
#include <uvm/pmap/pmap_devmap.h>

#include <riscv/fdt/riscv_fdtvar.h>

static const struct pmap_devmap *
riscv_platform_devmap(void)
{
	static const struct pmap_devmap devmap_empty[] = {
		DEVMAP_ENTRY_END
	};

	static struct pmap_devmap devmap_uart[] = {
		DEVMAP_ENTRY(VM_KERNEL_IO_BASE, 0, PAGE_SIZE),
		DEVMAP_ENTRY_END
	};
	bus_addr_t uart_base;

	const int phandle = fdtbus_get_stdout_phandle();
	if (phandle <= 0)
		return devmap_empty;

	if (fdtbus_get_reg(phandle, 0, &uart_base, NULL) != 0)
		return devmap_empty;

	devmap_uart[0].pd_pa = DEVMAP_ALIGN(uart_base);

	return devmap_uart;
}


static u_int
riscv_platform_uart_freq(void)
{
	return 0;
}

static const struct fdt_platform riscv_platform = {
	.fp_devmap = riscv_platform_devmap,
	.fp_bootstrap = riscv_fdt_cpu_bootstrap,
	.fp_uart_freq = riscv_platform_uart_freq,
	.fp_mpstart = riscv_fdt_cpu_mpstart,
};

FDT_PLATFORM(rv, FDT_PLATFORM_DEFAULT, &riscv_platform);
