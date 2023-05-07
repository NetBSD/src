/* $NetBSD: plic_fdt.c,v 1.1 2023/05/07 12:41:48 skrll Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Portions of this code is derived from software contributed to The NetBSD
 * Foundation by Simon Burge and Nick Hudson.
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
__KERNEL_RCSID(0, "$NetBSD: plic_fdt.c,v 1.1 2023/05/07 12:41:48 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>

#include <dev/fdt/fdtvar.h>

#include <riscv/sysreg.h>
#include <riscv/dev/plicreg.h>
#include <riscv/dev/plicvar.h>

static const struct device_compatible_entry compat_data[] = {
        { .compat = "riscv,plic0" },
        { .compat = "sifive,plic-1.0.0" },
        DEVICE_COMPAT_EOL
};

static void *
plic_fdt_intr_establish(device_t dev, u_int *specifier, int ipl, int flags,
    int (*func)(void *), void *arg, const char *xname)
{
	struct plic_softc * const sc = device_private(dev);
	struct plic_intrhand *ih;

	/* 1st cell is the interrupt number */
	const u_int irq = be32toh(specifier[0]);
	if (irq > sc->sc_ndev) {
		aprint_error_dev(dev, "irq %d greater than max irq %d\n",
		    irq, sc->sc_ndev);
		return NULL;
	}
	ih = plic_intr_establish_xname(irq, ipl,
	    (flags & FDT_INTR_MPSAFE) != 0 ? IST_MPSAFE : 0, func, arg, xname);

	return ih;
}
static void
plic_fdt_intr_disestablish(device_t dev, void *cookie)
{
	struct plic_softc * const sc = device_private(dev);
	struct plic_intrhand * const ih = cookie;
	const u_int cidx = ih->ih_cidx;
	const u_int irq = ih->ih_irq;

	plic_disable(sc, cidx, irq);
	plic_set_priority(sc, irq, 0);

	memset(&sc->sc_intr[irq], 0, sizeof(*sc->sc_intr));
}


static bool
plic_intrstr(device_t dev, u_int *specifier, char *buf, size_t buflen)
{
	/* 1st cell is the interrupt number */
	const int irq = be32toh(specifier[0]);

	snprintf(buf, buflen, "%s irq %d", device_xname(dev), irq);

        return true;
}

static struct fdtbus_interrupt_controller_func plic_funcs = {
	.establish = plic_fdt_intr_establish,
	.disestablish = plic_fdt_intr_disestablish,
	.intrstr = plic_intrstr,
};

static int
plic_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
plic_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct plic_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	const uint32_t *data;
	int error, context, len;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	error = of_getprop_uint32(phandle, "riscv,ndev", &sc->sc_ndev);
	if (error) {
		aprint_error("couldn't get supported number of external "
		    "interrupts\n");
		return;
	}
	if (sc->sc_ndev > PLIC_MAX_IRQ) {
		aprint_error(": invalid number of external interrupts (%u)\n",
		    sc->sc_ndev);
		return;
	}
	aprint_verbose("\n");

	/*
	 * PLIC context device mappings is documented at
	 * https://www.kernel.org/doc/Documentation/devicetree/bindings/interrupt-controller/sifive%2Cplic-1.0.0.yaml
	 * We need to walk the "interrupts-extended" property of
	 * and register handlers for the defined contexts.
	 *
	 * XXX
	 * This is usually an abstraction violation to inspect
	 * the current node's properties directly.  We do it
	 * in this case because the current binding spec defines
	 * this case.  We do a bit of error checking to make
	 * sure all the documented assumptions are correct.
	 */

	data = fdtbus_get_prop(phandle, "interrupts-extended", &len);
	if (data == NULL) {
		aprint_error_dev(self, "couldn't get context data\n");
		return;
	}
	context = 0;
	while (len > 0) {
		const int pphandle = be32toh(data[0]);
		const int xref = fdtbus_get_phandle_from_native(pphandle);
		const int intr_source = be32toh(data[1]);
		uint32_t intr_cells;

		error = of_getprop_uint32(xref, "#interrupt-cells", &intr_cells);
		if (error) {
			aprint_error_dev(self, "couldn't get cell length "
			    "for parent CPU for context %d", context);
			return;
		}

		if (intr_source != -1) {
			/* What do we want to pass as arg to plic_intr */
			void *ih = fdtbus_intr_establish_xname(phandle,
			    context, IPL_VM, FDT_INTR_MPSAFE,
			    plic_intr, sc, device_xname(self));
			if (ih == NULL) {
				    aprint_error_dev(self, "couldn't install "
				    "interrupt handler\n");
			} else {
				char intrstr[128];
				bool ok = fdtbus_intr_str(phandle, context,
				    intrstr, sizeof(intrstr));
				aprint_verbose_dev(self, "interrupt %s handler "
				    "installed\n", ok ? intrstr : "(unk)");
			}

			if (intr_source == IRQ_SUPERVISOR_EXTERNAL) {
				/*
				 * When finding context info, parent _must_ be a
				 * compatbile clint device.
				 */
				bus_addr_t cpuid;
				int cpu_ref;
				static const struct device_compatible_entry clint_compat_data[] = {
					{ .compat = "riscv,cpu-intc" },
					DEVICE_COMPAT_EOL
				};

				if (of_compatible_match(xref, clint_compat_data)) {
					/* get cpuid for the parent node */
					cpu_ref = OF_parent(xref);
					fdtbus_get_reg(cpu_ref, 0, &cpuid, NULL);

					KASSERT(context <= PLIC_MAX_CONTEXT);
					sc->sc_context[cpuid] = context;
					aprint_verbose_dev(self,
					    "cpu %"PRId64" context %d\n",
					    cpuid, context);
				} else {
					aprint_error_dev(self, "incompatiable CLINT "
					    " for PLIC for context %d\n", context);
				}

			}
		}
		len -= (intr_cells + 1) * 4;
		data += (intr_cells + 1);
		context++;
	}

	aprint_verbose_dev(self, "");
	error = plic_attach_common(sc, addr, size);
	if (error != 0) {
		return;
	}

	/* Setup complete, register this FDT bus. */
	error = fdtbus_register_interrupt_controller(self, phandle,
	    &plic_funcs);
	if (error != 0) {
		aprint_error(": couldn't register with fdtbus: %d\n", error);
	}
}

CFATTACH_DECL_NEW(plic_fdt, sizeof(struct plic_softc),
	plic_fdt_match, plic_fdt_attach, NULL, NULL);
