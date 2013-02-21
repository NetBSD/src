/*	$NetBSD: cpu.c,v 1.30 2013/02/21 15:16:02 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.30 2013/02/21 15:16:02 skrll Exp $");

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
	static const char lvls[4][4] = { "0", "1", "1.5", "2" };
	struct hp700_interrupt_register *ir;
	struct cpu_info *ci;
	u_int mhz = 100 * cpu_ticksnum / cpu_ticksdenom;
	int cpuno = device_unit(self);

#ifdef MULTIPROCESSOR
	struct pglist mlist;
	struct vm_page *m;
	int error;
#endif

	sc->sc_dev = self;

	/* Print the CPU chip name, nickname, and rev. */
	aprint_normal(": %s", hppa_cpu_info->hci_chip_name);
	if (hppa_cpu_info->hci_chip_nickname != NULL)
		aprint_normal(" (%s)", hppa_cpu_info->hci_chip_nickname);
	aprint_normal(" rev %d", cpu_revision);

	/* sanity against luser amongst config editors */
	if (ca->ca_irq != 31) {
		aprint_error_dev(self, "bad irq number %d\n", ca->ca_irq);
		return;
	}

	/* Print the CPU type, spec, level, category, and speed. */
	aprint_normal("\n%s: %s, PA-RISC %s", device_xname(self),
	    hppa_cpu_info->hci_chip_type,
	    hppa_cpu_info->hci_chip_spec);
	aprint_normal(", lev %s, cat %c, ",
	    lvls[pdc_model.pa_lvl], "AB"[pdc_model.mc]);

	aprint_normal("%d", mhz / 100);
	if (mhz % 100 > 9)
		aprint_normal(".%02d", mhz % 100);

	aprint_normal(" MHz clk\n%s: %s", device_xname(self),
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
	KASSERT(fpu_present);
	aprint_normal("%s: %s floating point, rev %d\n", device_xname(self),
	    hppa_mod_info(HPPA_TYPE_FPU, (fpu_version >> 16) & 0x1f),
	    (fpu_version >> 11) & 0x1f);

	if (cpuno >= HPPA_MAXCPUS) {
		aprint_normal_dev(self, "not started\n");
		return;
	}

	ci = &cpus[cpuno];
	ci->ci_cpuid = cpuno;
	ci->ci_hpa = ca->ca_hpa;

	hp700_intr_initialise(ci);

	ir = &ci->ci_ir;
	hp700_interrupt_register_establish(ci, ir);
	ir->ir_iscpu = true;
	ir->ir_ci = ci;
	ir->ir_name = device_xname(self);

	sc->sc_ihclk = hp700_intr_establish(IPL_CLOCK, clock_intr,
	    NULL /*clockframe*/, &ci->ci_ir, 31);
#ifdef MULTIPROCESSOR
	sc->sc_ihipi = hp700_intr_establish(IPL_HIGH, hppa_ipi_intr,
	    NULL /*clockframe*/, &ci->ci_ir, 30);
#endif

	/*
	 * Reserve some bits for chips that don't like to be moved
	 * around, e.g. lasi and asp.
	 */
	ir->ir_rbits = ((1 << 28) | (1 << 27));
	ir->ir_bits &= ~ir->ir_rbits;

#ifdef MULTIPROCESSOR
	/* Allocate stack for spin up and FPU emulation. */
	TAILQ_INIT(&mlist);
	error = uvm_pglistalloc(PAGE_SIZE, 0, -1L, PAGE_SIZE, 0, &mlist, 1, 0);

	if (error) {
		aprint_error(": unable to allocate CPU stack!\n");
		return;
	}
	m = TAILQ_FIRST(&mlist);
	ci->ci_stack = VM_PAGE_TO_PHYS(m);
	ci->ci_softc = sc;

	if (ci->ci_hpa == hppa_mcpuhpa) {
		ci->ci_flags |= CPUF_PRIMARY|CPUF_RUNNING;
	} else {
		int err;

		err = mi_cpu_attach(ci);
		if (err) {
			aprint_error_dev(self,
			    "mi_cpu_attach failed with %d\n", err);
			return;
		}
	}
	hppa_ncpu++;
	hppa_ipi_init(ci);
#endif
	KASSERT(ci->ci_cpl == -1);
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

	/* Wait for additional CPUs to spinup. */
	while (!start_secondary_cpu)
		;

	/* Spin for now */
	for (;;)
		;

}
#endif
