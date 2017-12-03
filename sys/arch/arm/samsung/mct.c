/*	$NetBSD: mct.c,v 1.3.6.3 2017/12/03 11:35:56 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

__KERNEL_RCSID(1, "$NetBSD: mct.c,v 1.3.6.3 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/kmem.h>

#include <prop/proplib.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <arm/samsung/mct_reg.h>
#include <arm/samsung/mct_var.h>

#include <arm/cortex/gtmr_intr.h>
#include <arm/cortex/mpcore_var.h>
#include <arm/cortex/gtmr_var.h>

#include <dev/fdt/fdtvar.h>
#include <arm/fdt/arm_fdtvar.h>

static int  mct_match(device_t, cfdata_t, void *);
static void mct_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exyo_mct, 0, mct_match, mct_attach, NULL, NULL);

static inline uint32_t
mct_read_global(struct mct_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
mct_write_global(struct mct_softc *sc, bus_size_t o, uint32_t v)
{
	bus_size_t wreg;
	uint32_t bit;
	int i;

	/* do the write */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
//	printf("%s: write %#x at %#x\n",
//		__func__, ((uint32_t) sc->sc_bsh + (uint32_t) o), v);

	/* dependent on the write address, do the ack dance */
	if (o == MCT_G_CNT_L || o == MCT_G_CNT_U) {
		wreg = MCT_G_CNT_WSTAT;
		bit  = (o == MCT_G_CNT_L) ? G_CNT_WSTAT_L : G_CNT_WSTAT_U;
	} else {
		wreg = MCT_G_WSTAT;
		switch (o) {
		case MCT_G_COMP0_L:
			bit  = G_WSTAT_COMP0_L;
			break;
		case MCT_G_COMP0_U:
			bit  = G_WSTAT_COMP0_U;
			break;
		case MCT_G_COMP0_ADD_INCR:
			bit  = G_WSTAT_ADD_INCR;
			break;
		case MCT_G_TCON:
			bit  = G_WSTAT_TCON;
			break;
		default:
			/* all other registers */
			return;
		}
	}

	/* wait for ack */
	for (i = 0; i < 10000000; i++) {
		/* value accepted by the hardware/hal ? */
		if (mct_read_global(sc, wreg) & bit) {
			/* ack */
			bus_space_write_4(sc->sc_bst, sc->sc_bsh, wreg, bit);
			return;
		}
	}
	panic("MCT hangs after writing %#x at %#x", v, (uint32_t) o);
}

static void
mct_fdt_cpu_hatch(void *priv, struct cpu_info *ci)
{
	gtmr_init_cpu_clock(ci);
}

static int
mct_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,exynos4210-mct",
					    NULL };

	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mct_attach(device_t parent, device_t self, void *aux)
{
	struct mct_softc * const sc = &mct_sc;
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	self->dv_private = sc;
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	sc->sc_freq = EXYNOS_F_IN_FREQ;

	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Exynos SoC multi core timer (64 bits)\n");

	/* Start the timer */
	uint32_t tcon = mct_read_global(sc, MCT_G_TCON);
	tcon |= G_TCON_START;
	mct_write_global(sc, MCT_G_TCON, tcon);

	/* Attach ARMv7 generic timer */
	struct mpcore_attach_args mpcaa = {
		.mpcaa_name = "armgtmr",
		.mpcaa_irq = IRQ_GTMR_PPI_VTIMER
	};

	config_found(self, &mpcaa, NULL);

	arm_fdt_cpu_hatch_register(self, mct_fdt_cpu_hatch);
}
