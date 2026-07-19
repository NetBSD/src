/*	$NetBSD: genintc.c,v 1.1 2026/07/19 01:48:21 thorpej Exp $	*/

/*-
 * Copyright (c) 2025, 2026 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: genintc.c,v 1.1 2026/07/19 01:48:21 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "motorola,68000-generic-interrupts" },
	DEVICE_COMPAT_EOL
};

/*
 * Generic m68k interrupt "controller".  Supports vectored and auto-vectored
 * interrupts.  Can use a syscon to enable / disable interrupts globally.
 *
 * Our FDT bindings:
 *
 *	#interrupt-cells = 2
 *	0 - CPU interrupt priority level
 *	1 - interrupt vector (0 == auto-vector)
 */

static void *
genintc_intr_establish(device_t dev, u_int *specifier, int ipl,
    int flags, int (*func)(void *), void *arg, const char *xname)
{
	const u_int cpu_ipl = be32toh(specifier[0]);
	const u_int vector = be32toh(specifier[1]);

	if (cpu_ipl < 1 || cpu_ipl > 6) {
		return NULL;
	}

	return m68k_intr_establish(func,
				   arg,
				   NULL,	/* evcnt */
				   vector,
				   cpu_ipl,
				   0,		/* isrpri */
				   0);		/* flags */
}

static void
genintc_intr_disestablish(device_t dev, void *ih)
{
	(void)m68k_intr_disestablish(ih);
}

static bool
genintc_intr_string(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	const u_int cpu_ipl = be32toh(specifier[0]);
	const u_int vector = be32toh(specifier[1]);

	if (cpu_ipl < 1 || cpu_ipl > 6) {
		return false;
	}

	if (vector) {
		snprintf(buf, buflen, "vector 0x%x ipl %u",
		    vector, cpu_ipl);
	} else {
		snprintf(buf, buflen, "auto-vector ipl %u",
		    cpu_ipl);
	}

	return true;
}

static const struct fdtbus_interrupt_controller_func genintc_fdt_funcs = {
	.establish = genintc_intr_establish,
	.disestablish = genintc_intr_disestablish,
	.intrstr = genintc_intr_string,
};

static int
genintc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
genintc_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	int error;

	aprint_naive("\n");
	aprint_normal("\n");

	error = fdtbus_register_interrupt_controller(self, phandle,
	    &genintc_fdt_funcs);
	if (error) {
		aprint_error_dev(self,
		    "couldn't register with fdtbus (error=%d)\n", error);
	}

	/*
	 * If the device tree specifies an interrupt-enable controller,
	 * go ahead and enable them now.
	 */
	struct syscon *ena =
	    fdtbus_syscon_acquire(phandle, "interrupt-enable-controller");
	if (ena != NULL) {
		uint32_t reg, mask, ena_val, val;

		if (of_getprop_uint32(phandle,
				      "interrupt-enable-reg-mask", &mask)) {
			mask = 0xffffffff;
		}
		if (of_getprop_uint32(phandle,
				      "interrupt-enable-reg",
				      &reg) == 0 &&
		    of_getprop_uint32(phandle,
				      "interrupt-enable-value",
				      &ena_val) == 0) {
			aprint_verbose_dev(self,
			    "enabling interrupts in system controller\n");
			syscon_lock(ena);
			val = syscon_read_4(ena, reg);
			val = (val & mask) | ena_val;
			syscon_write_4(ena, reg, val);
			syscon_unlock(ena);
		}
	}
}

CFATTACH_DECL_NEW(genintc, 0,
    genintc_match, genintc_attach, NULL, NULL);
