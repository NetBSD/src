/*	$NetBSD: cpu.c,v 1.12 2009/05/07 15:34:49 skrll Exp $	*/

/*	$OpenBSD: cpu.c,v 1.28 2004/12/28 05:18:25 mickey Exp $	*/

/*
 * Copyright (c) 1998-2003 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.12 2009/05/07 15:34:49 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <machine/cpufunc.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/hp700/intr.h>
#include <hp700/hp700/machdep.h>
#include <hp700/dev/cpudevs.h>

struct cpu_softc {
	device_t sc_dev;
	hppa_hpa_t sc_hpa;
	void *sc_ih;
};

int	cpumatch(device_t, cfdata_t, void *);
void	cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, sizeof(struct cpu_softc),
    cpumatch, cpuattach, NULL, NULL);

static int cpu_attached;

int
cpumatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* there will be only one for now XXX */
	/* probe any 1.0, 1.1 or 2.0 */
	if (cpu_attached ||
	    ca->ca_type.iodc_type != HPPA_TYPE_NPROC ||
	    ca->ca_type.iodc_sv_model != HPPA_NPROC_HPPA)
		return 0;

	return 1;
}

void
cpuattach(device_t parent, device_t self, void *aux)
{
	/* machdep.c */
	extern struct pdc_cache pdc_cache;
	extern struct pdc_btlb pdc_btlb;
	extern struct pdc_model pdc_model;
	extern u_int cpu_ticksnum, cpu_ticksdenom;

	struct cpu_softc *sc = device_private(self);
	struct confargs *ca = aux;
	const char lvls[4][4] = { "0", "1", "1.5", "2" };
	u_int mhz = 100 * cpu_ticksnum / cpu_ticksdenom;

	sc->sc_dev = self;
	cpu_attached = 1;

	/* Print the CPU chip name, nickname, and rev. */
	aprint_normal(": %s", hppa_cpu_info->hci_chip_name);
	if (hppa_cpu_info->hci_chip_nickname != NULL)
		aprint_normal(" (%s)", hppa_cpu_info->hci_chip_nickname);
	aprint_normal(" rev %d", cpu_revision);

	/* Print the CPU type, spec, level, category, and speed. */
	aprint_normal("\n%s: %s, PA-RISC %s", self->dv_xname,
	    hppa_cpu_info->hci_chip_type,
	    hppa_cpu_info->hci_chip_spec);
	aprint_normal(", lev %s, cat %c, ",
	    lvls[pdc_model.pa_lvl], "AB"[pdc_model.mc]);

	aprint_normal("%d", mhz / 100);
	if (mhz % 100 > 9)
		aprint_normal(".%02d", mhz % 100);

	aprint_normal(" MHz clk\n%s: %s", self->dv_xname, 
	    pdc_model.sh? "shadows, ": "");

	if (pdc_cache.dc_conf.cc_sh)
		aprint_normal("%uK cache", pdc_cache.dc_size / 1024);
	else
		aprint_normal("%uK/%uK D/I caches", pdc_cache.dc_size / 1024,
		    pdc_cache.ic_size / 1024);
	if (pdc_cache.dt_conf.tc_sh)
		aprint_normal(", %u shared TLB", pdc_cache.dt_size);
	else
		aprint_normal(", %u/%u D/I TLBs", pdc_cache.dt_size,
		    pdc_cache.it_size);

	if (pdc_btlb.finfo.num_c)
		aprint_normal(", %u shared BTLB", pdc_btlb.finfo.num_c);
	else {
		aprint_normal(", %u/%u D/I BTLBs", pdc_btlb.finfo.num_i,
		    pdc_btlb.finfo.num_d);
	}

	/*
	 * Describe the floating-point support.
	 */
#ifndef	FPEMUL
	if (!fpu_present)
		aprint_normal("\n%s: no floating point support",
		    self->dv_xname);
	else
#endif /* !FPEMUL */
	{
		aprint_normal("\n%s: %s floating point, rev %d", self->dv_xname,
		    hppa_mod_info(HPPA_TYPE_FPU, (fpu_version >> 16) & 0x1f),
		    (fpu_version >> 11) & 0x1f);
	}

	aprint_normal("\n");

	/* sanity against luser amongst config editors */
	if (ca->ca_irq == 31) {
		sc->sc_ih = hp700_intr_establish(sc->sc_dev, IPL_CLOCK,
		    clock_intr, NULL /*trapframe*/, &int_reg_cpu,
		    ca->ca_irq);
	} else {
		aprint_error_dev(self, "bad irq number %d\n", ca->ca_irq);
	}

	/*
	 * Set the allocatable bits in the CPU interrupt registers.
	 * These should only be used by major chipsets, like ASP and
	 * LASI, and the bits used appear to be important - the
	 * ASP doesn't seem to like to use interrupt bits above 28
	 * or below 27.
	 */
	int_reg_cpu.int_reg_allocatable_bits =
		(1 << 28) | (1 << 27) | (1 << 26);
}
