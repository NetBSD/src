/*	$NetBSD: clint_fdt.c,v 1.1 2023/05/07 12:41:48 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clint_fdt.c,v 1.1 2023/05/07 12:41:48 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/intr.h>

#include <dev/fdt/fdtvar.h>

#include <riscv/fdt/riscv_fdtvar.h>

#include <machine/sysreg.h>


#define CLINT_MSIP_HARTN(n)	0x0000 + 4 * (n)
#define CLINT_MTIMECMP_HARTN(n)	0x4000 + 8 * (n)
#define CLINT_MTIME		0xbff8
#define CLINT_MTIME_LO		CLINT_MTIME + 0x0
#define CLINT_MTIME_HI		CLINT_MTIME + 0x4

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "riscv,clint0" },
	DEVICE_COMPAT_EOL
};

struct clint_fdt_softc {
	device_t		 sc_dev;
	bus_space_tag_t		 sc_bst;
	bus_space_handle_t	 sc_bsh;

	uint64_t		 sc_timebase_frequency;
	uint64_t		 sc_ticks_per_hz;
};

static struct clint_fdt_softc *clint_sc;

#define CLINT_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	CLINT_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
clint_ipi_handler(void *arg)
{
	cpu_Debugger();
	return 1;
}


static uint64_t
clint_time_read(struct clint_fdt_softc *sc)
{
#if _LP64
	return bus_space_read_8(sc->sc_bst, sc->sc_bsh, CLINT_MTIME);
#else
	uint32_t hi, lo;
	do {
		hi = CLINT_READ(sc, CLINT_MTIME_HI);
		lo = CLINT_READ(sc, CLINT_MTIME_LO);

	} while (hi != CLINT_READ(sc, CLINT_MTIME_HI));
	return
	    __SHIFTIN(hi, __BITS(63, 32)) |
	    __SHIFTIN(lo, __BITS(31,  0));
#endif
}


static void
clint_timer_set(struct clint_fdt_softc *sc, struct cpu_info *ci)
{
	const uint64_t now = clint_time_read(sc);

	ci->ci_lastintr = now;
	ci->ci_ev_timer.ev_count++;

	ci->ci_lastintr_scheduled += sc->sc_ticks_per_hz;

	CLINT_WRITE(sc, CLINT_MTIMECMP_HARTN(ci->ci_cpuid),
	    ci->ci_lastintr_scheduled);
}

static int
clint_timer_intr(void *arg)
{
	struct cpu_info * const ci = curcpu();
	struct clockframe * const cf = arg;
	struct clint_fdt_softc * const sc = clint_sc;

printf_nolog("%s: sip %#" PRIxPTR "\n", __func__, csr_sip_read());

	csr_sip_clear(SIP_STIP);	/* clean pending interrupt status */

	clint_timer_set(sc, ci);

	hardclock(cf);

	return 1;
}

static void
clint_cpu_initclocks(void)
{
	struct cpu_info * const ci = curcpu();
	struct clint_fdt_softc * const sc = clint_sc;

	clint_timer_set(sc, ci);

	csr_sie_set(SIE_STIE);		/* enable supervisor timer intr */

	printf("init clocks at time %"PRId64"\n",
	    ci->ci_lastintr);
}

static int
clint_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
clint_attach(device_t parent, device_t self, void *aux)
{
	struct clint_fdt_softc * const sc = device_private(self);
	const struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	const int cpus = OF_finddevice("/cpus");
	if (cpus == -1) {
		aprint_error(": couldn't get 'cpus' node\n");
		return;
	}

	uint32_t tbfreq;
	int ret = of_getprop_uint32(cpus, "timebase-frequency", &tbfreq);
	if (ret < 0) {
		aprint_error(": can't get timebase frequency\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_timebase_frequency = tbfreq;

	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	printf("%s: addr %#" PRIxBUSADDR " size %#" PRIxBUSSIZE "\n", __func__,
	    addr, size);

	aprint_naive("\n");
	aprint_normal(": local interrupt control. %" PRId64 " KHz timer.\n",
	    sc->sc_timebase_frequency / 1000);

	sc->sc_ticks_per_hz = sc->sc_timebase_frequency / hz;
	int len;
	const uint32_t *data =
	    fdtbus_get_prop(phandle, "interrupts-extended", &len);
	if (data == NULL) {
		aprint_error_dev(self, "couldn't get context data\n");
		return;
	}

	clint_sc = sc;
//	riscv_fdt_timer_register(clint_cpu_initclocks);

	int context = 0;
	while (len > 0) {
		const int pphandle = be32toh(data[0]);
		const int xref = fdtbus_get_phandle_from_native(pphandle);
		const int intr_source = be32toh(data[1]);
		uint32_t intr_cells;

		int error =
		    of_getprop_uint32(xref, "#interrupt-cells", &intr_cells);
		if (error) {
			aprint_error_dev(self, "couldn't get cell length "
			    "for parent CPU for context %d", context);
			return;
		}
		int (*handler)(void *) = NULL;
		void *arg = NULL;
		int ipl = IPL_HIGH;
		switch (intr_source) {
		case IRQ_MACHINE_SOFTWARE:
			handler = clint_ipi_handler;
			arg = sc;
			break;
		case IRQ_MACHINE_TIMER:
			handler = clint_timer_intr;
			arg = NULL;			/* clock frame */
			ipl = IPL_SCHED;
			break;
		default:
			aprint_error_dev(self, "unknown interrupt source %d",
			    intr_source);
		}

		if (handler) {
			void *ih = fdtbus_intr_establish_xname(phandle,
			    context, ipl, FDT_INTR_MPSAFE,
			    handler, arg, device_xname(self));
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
		}

		len -= (intr_cells + 1) * 4;
		data += (intr_cells + 1);
		context++;
	}
}

CFATTACH_DECL_NEW(clint_fdt, sizeof(struct clint_fdt_softc),
	clint_match, clint_attach, NULL, NULL);
