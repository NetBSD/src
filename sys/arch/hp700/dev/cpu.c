/*	$NetBSD: cpu.c,v 1.19.4.1 2012/04/17 00:06:21 yamt Exp $	*/

/*	$OpenBSD: cpu.c,v 1.29 2009/02/08 18:33:28 miod Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.19.4.1 2012/04/17 00:06:21 yamt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/atomic.h>
#include <sys/reboot.h>

#include <uvm/uvm.h>

#include <machine/cpufunc.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hppa/hppa/cpuvar.h>
#include <hp700/hp700/intr.h>
#include <hp700/hp700/machdep.h>
#include <hp700/dev/cpudevs.h>

#ifdef MULTIPROCESSOR

int hppa_ncpu;

struct cpu_info *cpu_hatch_info;
static volatile int start_secondary_cpu;
#endif

int	cpumatch(device_t, cfdata_t, void *);
void	cpuattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, sizeof(struct cpu_softc),
    cpumatch, cpuattach, NULL, NULL);

int
cpumatch(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	/* probe any 1.0, 1.1 or 2.0 */
	if (ca->ca_type.iodc_type != HPPA_TYPE_NPROC ||
	    ca->ca_type.iodc_sv_model != HPPA_NPROC_HPPA)
		return 0;

	if (cf->cf_unit >= MAXCPUS)
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
	struct cpu_info *ci;
	static const char lvls[4][4] = { "0", "1", "1.5", "2" };
	u_int mhz = 100 * cpu_ticksnum / cpu_ticksdenom;
	int cpuno = device_unit(self);

#ifdef MULTIPROCESSOR
	struct pglist mlist;
	struct vm_page *m;
	int error;
#endif

	sc->sc_dev = self;

	ci = &cpus[cpuno];
	ci->ci_cpuid = cpuno;
	ci->ci_hpa = ca->ca_hpa;

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

	if (pdc_cache.dc_conf.cc_fsel)
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
	aprint_normal("\n");

	/*
	 * Describe the floating-point support.
	 */
	aprint_normal("%s: %s floating point, rev %d\n", self->dv_xname,
	    hppa_mod_info(HPPA_TYPE_FPU, (fpu_version >> 16) & 0x1f),
	    (fpu_version >> 11) & 0x1f);

	/* sanity against luser amongst config editors */
	if (ca->ca_irq != 31) {
		aprint_error_dev(self, "bad irq number %d\n", ca->ca_irq);
		return;
	}
	
	sc->sc_ihclk = hp700_intr_establish(IPL_CLOCK, clock_intr,
	    NULL /*clockframe*/, &ir_cpu, 31);

#ifdef MULTIPROCESSOR

	/* Allocate stack for spin up and FPU emulation. */
	TAILQ_INIT(&mlist);
	error = uvm_pglistalloc(PAGE_SIZE, 0, -1L, PAGE_SIZE, 0, &mlist, 1,
	    0);

	if (error) {
		aprint_error(": unable to allocate CPU stack!\n");
		return;
	}
	m = TAILQ_FIRST(&mlist);
	ci->ci_stack = VM_PAGE_TO_PHYS(m);

	if (ci->ci_hpa == hppa_mcpuhpa) {
		ci->ci_flags |= CPUF_PRIMARY|CPUF_RUNNING;
		hppa_ncpu++;
	} else {
		int err;

		err = mi_cpu_attach(ci);
		if (err) {
			aprint_error_dev(self,
			    "mi_cpu_attach failed with %d\n", err);
			return;
		}
	}

#endif

	/*
	 * Set the allocatable bits in the CPU interrupt registers.
	 * These should only be used by major chipsets, like ASP and
	 * LASI, and the bits used appear to be important - the
	 * ASP doesn't seem to like to use interrupt bits above 28
	 * or below 27.
	 */
	ir_cpu.ir_bits =
		(1 << 28) | (1 << 27) | (1 << 26);
}


#ifdef MULTIPROCESSOR
void
cpu_boot_secondary_processors(void)
{
	struct cpu_info *ci;
	struct iomod *cpu;
	int i, j;

	for (i = 0; i < HPPA_MAXCPUS; i++) {

		ci = &cpus[i];
		if (ci->ci_cpuid == 0)
			continue;

		if (ci->ci_data.cpu_idlelwp == NULL)
			continue;

		if (ci->ci_flags & CPUF_PRIMARY)
			continue;

		/* Release the specified CPU by triggering an EIR{0}. */
		cpu_hatch_info = ci;
		cpu = (struct iomod *)(ci->ci_hpa);
		cpu->io_eir = 0;
		membar_sync();

		/* Wait for CPU to wake up... */
		j = 0;
		while (!(ci->ci_flags & CPUF_RUNNING) && j++ < 10000)
			delay(1000);
		if (!(ci->ci_flags & CPUF_RUNNING))
			printf("failed to hatch cpu %i!\n", ci->ci_cpuid);
	}

	/* Release secondary CPUs. */
	start_secondary_cpu = 1;
	membar_sync();
}

void
cpu_hw_init(void)
{
	struct cpu_info *ci = curcpu();

	/* Purge TLB and flush caches. */
	ptlball();
	fcacheall();

	/* Enable address translations. */
	ci->ci_psw = PSW_I | PSW_Q | PSW_P | PSW_C | PSW_D;
	ci->ci_psw |= (cpus[0].ci_psw & PSW_O);

	ci->ci_curlwp = ci->ci_data.cpu_idlelwp;
}

void
cpu_hatch(void)
{
	struct cpu_info *ci = curcpu();

	ci->ci_flags |= CPUF_RUNNING;
#if 0
	hppa_ncpu++;
#endif

	/* Wait for additional CPUs to spinup. */
	while (!start_secondary_cpu)
		;

	/* Spin for now */
	for (;;)
		;

}
#endif
