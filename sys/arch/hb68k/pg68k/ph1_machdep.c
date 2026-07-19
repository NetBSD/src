/*	$NetBSD: ph1_machdep.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ph1_machdep.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <hb68k/pg68k/pgromcalls.h>

#include <dev/ata/atavar.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_platform.h>

#include <dev/ofw/openfirm.h>

/*
 * plat_machine_init --
 *	Perform any machine-specific initialization.  "nextpa" is
 *	the next free physical memory page, from which any memory
 *	can be stolen on a whole-page basis as needed.  Returns the
 *	next free physical memory page.
 */
static paddr_t
ph1_plat_machine_init(paddr_t nextpa)
{
	return nextpa;
}

/*
 * plat_device_register --
 *	device_register() hook -- boot device detection and an
 *	opportunity to set any device properties as devices are
 *	attached during autoconfiguration.
 */
static void
ph1_plat_device_register(device_t dev, void *aux)
{
	extern device_t machine_booted_device;	/* XXX extern */
	device_t parent = device_parent(dev);

	static device_t booted_controller;
	static paddr_t booted_controller_phys = (paddr_t)-1;
	static int booted_unit = -1;
	static bool initialized;

	/*
	 * The Phaethon 1 firmware provides several properties
	 * in /chosen to help us find the boot device:
	 *
	 *	pg68k,booted-controller-type -- e.g. "ata"
	 *	pg68k,booted-controller-phys -- phys addr of device
	 *	pg68k,booted-unit            -- unit (i.e. drive) #
	 *
	 * We'll be concerned with pg68k,booted-controller-phys and
	 * pg68k,booted-unit.
	 */
	if (! initialized) {
		int chosen = OF_finddevice("/chosen");
		uint32_t val;

		if (chosen != -1) {
			if (of_getprop_uint32(chosen,
					      "pg68k,booted-controller-phys",
					      &val) == 0) {
				booted_controller_phys = val;
			}
			if (of_getprop_uint32(chosen,
					      "pg68k,booted-unit",
					      &val) == 0) {
				booted_unit = val;
			}
		}
		initialized = true;
	}

	if (booted_controller == NULL) {
		if (device_is_a(parent, "simplebus")) {
			struct fdt_attach_args *faa = aux;
			bus_addr_t addr;
			bus_size_t size;

			if (fdtbus_get_reg(faa->faa_phandle,
					   0, &addr, &size) != 0) {
				return;
			}

			if (addr == booted_controller_phys) {
				booted_controller = dev;
			}
		}
		return;
	}

	if (machine_booted_device == NULL) {
		if (device_is_a(dev, "wd") &&
		    device_is_a(parent, "atabus") &&
		    device_parent(parent) == booted_controller) {
			struct ata_device *adev = aux;

			if (adev->adev_drv_data->drive == booted_unit) {
				machine_booted_device = dev;
			}
		}
	}
}

/*
 * plat_halt --
 *	Halt the machine, returning back to the firmware monitor,
 *	if possible.
 */
static void
ph1_plat_halt(void)
{
	printf("Transferring control back to firmware...\n");
	pgromcall_halt();
}

/*
 * plat_powerdown --
 *	Power-down the machine, if possible.  If not, then halt.
 */
static void
ph1_plat_powerdown(void)
{
	/*
	 * The Phaethon 1 doesn't have software power control.  It
	 * provides a poweroff vector, but it just halts.
	 */
	ph1_plat_halt();
}

/*
 * plat_reboot --
 *	Reboot the system according to "howto".  Attempt to
 *	use "bootstr" as the arguments for the resulting system
 *	boot.
 */
static void
ph1_plat_reboot(int howto, char *bootstr)
{
	printf("Requesting reboot from firmware...\n");
	pgromcall_reboot();
}

/*
 * plat_uart_freq --
 *	Return the console UART clock frequency in Hz.  This is
 *	called if the information is not available in the device
 *	tree.
 */
static u_int
ph1_plat_uart_freq(void)
{
	return 1843200;
}

static const struct fdt_platform phaethon1_platform = {
	.fp_machine_init = ph1_plat_machine_init,
	.fp_device_register = ph1_plat_device_register,
	.fp_powerdown = ph1_plat_powerdown,
	.fp_halt = ph1_plat_halt,
	.fp_reboot = ph1_plat_reboot,
	.fp_uart_freq = ph1_plat_uart_freq,
};
FDT_PLATFORM(phaethon1, "pg68k,cpu010-mk1", &phaethon1_platform);
