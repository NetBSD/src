/* $NetBSD: acpi_cpu_md.c,v 1.71.6.2 2014/08/20 00:03:29 tls Exp $ */

/*-
 * Copyright (c) 2010, 2011 Jukka Ruohonen <jruohonen@iki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu_md.c,v 1.71.6.2 2014/08/20 00:03:29 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpufreq.h>
#include <sys/device.h>
#include <sys/kcore.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <x86/cpu.h>
#include <x86/cpufunc.h>
#include <x86/cputypes.h>
#include <x86/cpuvar.h>
#include <x86/cpu_msr.h>
#include <x86/machdep.h>
#include <x86/x86/tsc.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpi_cpu.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/acpi_machdep.h>

/*
 * Intel IA32_MISC_ENABLE.
 */
#define MSR_MISC_ENABLE_EST	__BIT(16)
#define MSR_MISC_ENABLE_TURBO	__BIT(38)

/*
 * AMD C1E.
 */
#define MSR_CMPHALT		0xc0010055

#define MSR_CMPHALT_SMI		__BIT(27)
#define MSR_CMPHALT_C1E		__BIT(28)
#define MSR_CMPHALT_BMSTS	__BIT(29)

/*
 * AMD families 10h, 11h, 12h, 14h, and 15h.
 */
#define MSR_10H_LIMIT		0xc0010061
#define MSR_10H_CONTROL		0xc0010062
#define MSR_10H_STATUS		0xc0010063
#define MSR_10H_CONFIG		0xc0010064

/*
 * AMD family 0Fh.
 */
#define MSR_0FH_CONTROL		0xc0010041
#define MSR_0FH_STATUS		0xc0010042

#define MSR_0FH_STATUS_CFID	__BITS( 0,  5)
#define MSR_0FH_STATUS_CVID	__BITS(32, 36)
#define MSR_0FH_STATUS_PENDING	__BITS(31, 31)

#define MSR_0FH_CONTROL_FID	__BITS( 0,  5)
#define MSR_0FH_CONTROL_VID	__BITS( 8, 12)
#define MSR_0FH_CONTROL_CHG	__BITS(16, 16)
#define MSR_0FH_CONTROL_CNT	__BITS(32, 51)

#define ACPI_0FH_STATUS_FID	__BITS( 0,  5)
#define ACPI_0FH_STATUS_VID	__BITS( 6, 10)

#define ACPI_0FH_CONTROL_FID	__BITS( 0,  5)
#define ACPI_0FH_CONTROL_VID	__BITS( 6, 10)
#define ACPI_0FH_CONTROL_VST	__BITS(11, 17)
#define ACPI_0FH_CONTROL_MVS	__BITS(18, 19)
#define ACPI_0FH_CONTROL_PLL	__BITS(20, 26)
#define ACPI_0FH_CONTROL_RVO	__BITS(28, 29)
#define ACPI_0FH_CONTROL_IRT	__BITS(30, 31)

#define FID_TO_VCO_FID(fidd)	(((fid) < 8) ? (8 + ((fid) << 1)) : (fid))

static char	  native_idle_text[16];
void		(*native_idle)(void) = NULL;

static int	 acpicpu_md_quirk_piix4(const struct pci_attach_args *);
static void	 acpicpu_md_pstate_hwf_reset(void *, void *);
static int	 acpicpu_md_pstate_fidvid_get(struct acpicpu_softc *,
                                              uint32_t *);
static int	 acpicpu_md_pstate_fidvid_set(struct acpicpu_pstate *);
static int	 acpicpu_md_pstate_fidvid_read(uint32_t *, uint32_t *);
static void	 acpicpu_md_pstate_fidvid_write(uint32_t, uint32_t,
					        uint32_t, uint32_t);
static int	 acpicpu_md_pstate_sysctl_init(void);
static int	 acpicpu_md_pstate_sysctl_get(SYSCTLFN_PROTO);
static int	 acpicpu_md_pstate_sysctl_set(SYSCTLFN_PROTO);
static int	 acpicpu_md_pstate_sysctl_all(SYSCTLFN_PROTO);

extern struct acpicpu_softc **acpicpu_sc;
static struct sysctllog *acpicpu_log = NULL;

struct cpu_info *
acpicpu_md_match(device_t parent, cfdata_t match, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;

	if (strcmp(cfaa->name, "frequency") != 0)
		return NULL;

	return cfaa->ci;
}

struct cpu_info *
acpicpu_md_attach(device_t parent, device_t self, void *aux)
{
	struct cpufeature_attach_args *cfaa = aux;

	return cfaa->ci;
}

uint32_t
acpicpu_md_flags(void)
{
	struct cpu_info *ci = curcpu();
	struct pci_attach_args pa;
	uint32_t family, val = 0;
	uint32_t regs[4];
	uint64_t msr;

	if (acpi_md_ncpus() == 1)
		val |= ACPICPU_FLAG_C_BM;

	if ((ci->ci_feat_val[1] & CPUID2_MONITOR) != 0)
		val |= ACPICPU_FLAG_C_FFH;

	/*
	 * By default, assume that the local APIC timer
	 * as well as TSC are stalled during C3 sleep.
	 */
	val |= ACPICPU_FLAG_C_APIC | ACPICPU_FLAG_C_TSC;

	/*
	 * Detect whether TSC is invariant. If it is not, we keep the flag to
	 * note that TSC will not run at constant rate. Depending on the CPU,
	 * this may affect P- and T-state changes, but especially relevant
	 * are C-states; with variant TSC, states larger than C1 may
	 * completely stop the counter.
	 */
	if (tsc_is_invariant())
		val &= ~ACPICPU_FLAG_C_TSC;

	switch (cpu_vendor) {

	case CPUVENDOR_IDT:

		if ((ci->ci_feat_val[1] & CPUID2_EST) != 0)
			val |= ACPICPU_FLAG_P_FFH;

		if ((ci->ci_feat_val[0] & CPUID_ACPI) != 0)
			val |= ACPICPU_FLAG_T_FFH;

		break;

	case CPUVENDOR_INTEL:

		/*
		 * Bus master control and arbitration should be
		 * available on all supported Intel CPUs (to be
		 * sure, this is double-checked later from the
		 * firmware data). These flags imply that it is
		 * not necessary to flush caches before C3 state.
		 */
		val |= ACPICPU_FLAG_C_BM | ACPICPU_FLAG_C_ARB;

		/*
		 * Check if we can use "native", MSR-based,
		 * access. If not, we have to resort to I/O.
		 */
		if ((ci->ci_feat_val[1] & CPUID2_EST) != 0)
			val |= ACPICPU_FLAG_P_FFH;

		if ((ci->ci_feat_val[0] & CPUID_ACPI) != 0)
			val |= ACPICPU_FLAG_T_FFH;

		/*
		 * Check whether MSR_APERF, MSR_MPERF, and Turbo
		 * Boost are available. Also see if we might have
		 * an invariant local APIC timer ("ARAT").
		 */
		if (cpuid_level >= 0x06) {

			x86_cpuid(0x00000006, regs);

			if ((regs[2] & CPUID_DSPM_HWF) != 0)
				val |= ACPICPU_FLAG_P_HWF;

			if ((regs[0] & CPUID_DSPM_IDA) != 0)
				val |= ACPICPU_FLAG_P_TURBO;

			if ((regs[0] & CPUID_DSPM_ARAT) != 0)
				val &= ~ACPICPU_FLAG_C_APIC;
		}

		break;

	case CPUVENDOR_AMD:

		x86_cpuid(0x80000000, regs);

		if (regs[0] < 0x80000007)
			break;

		x86_cpuid(0x80000007, regs);

		family = CPUID_TO_FAMILY(ci->ci_signature);

    		switch (family) {

		case 0x0f:

			/*
			 * Disable C1E if present.
			 */
			if (rdmsr_safe(MSR_CMPHALT, &msr) != EFAULT)
				val |= ACPICPU_FLAG_C_C1E;

			/*
			 * Evaluate support for the "FID/VID
			 * algorithm" also used by powernow(4).
			 */
			if ((regs[3] & CPUID_APM_FID) == 0)
				break;

			if ((regs[3] & CPUID_APM_VID) == 0)
				break;

			val |= ACPICPU_FLAG_P_FFH | ACPICPU_FLAG_P_FIDVID;
			break;

		case 0x10:
		case 0x11:

			/*
			 * Disable C1E if present.
			 */
			if (rdmsr_safe(MSR_CMPHALT, &msr) != EFAULT)
				val |= ACPICPU_FLAG_C_C1E;

			/* FALLTHROUGH */

		case 0x12:
		case 0x14: /* AMD Fusion */
		case 0x15: /* AMD Bulldozer */

			/*
			 * Like with Intel, detect MSR-based P-states,
			 * and AMD's "turbo" (Core Performance Boost),
			 * respectively.
			 */
			if ((regs[3] & CPUID_APM_HWP) != 0)
				val |= ACPICPU_FLAG_P_FFH;

			if ((regs[3] & CPUID_APM_CPB) != 0)
				val |= ACPICPU_FLAG_P_TURBO;

			/*
			 * Also check for APERF and MPERF,
			 * first available in the family 10h.
			 */
			if (cpuid_level >= 0x06) {

				x86_cpuid(0x00000006, regs);

				if ((regs[2] & CPUID_DSPM_HWF) != 0)
					val |= ACPICPU_FLAG_P_HWF;
			}

			break;
		}

		break;
	}

	/*
	 * There are several erratums for PIIX4.
	 */
	if (pci_find_device(&pa, acpicpu_md_quirk_piix4) != 0)
		val |= ACPICPU_FLAG_PIIX4;

	return val;
}

static int
acpicpu_md_quirk_piix4(const struct pci_attach_args *pa)
{

	/*
	 * XXX: The pci_find_device(9) function only
	 *	deals with attached devices. Change this
	 *	to use something like pci_device_foreach().
	 */
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82371AB_ISA ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82440MX_PMC)
		return 1;

	return 0;
}

void
acpicpu_md_quirk_c1e(void)
{
	const uint64_t c1e = MSR_CMPHALT_SMI | MSR_CMPHALT_C1E;
	uint64_t val;

	val = rdmsr(MSR_CMPHALT);

	if ((val & c1e) != 0)
		wrmsr(MSR_CMPHALT, val & ~c1e);
}

int
acpicpu_md_cstate_start(struct acpicpu_softc *sc)
{
	const size_t size = sizeof(native_idle_text);
	struct acpicpu_cstate *cs;
	bool ipi = false;
	int i;

	/*
	 * Save the cpu_idle(9) loop used by default.
	 */
	x86_cpu_idle_get(&native_idle, native_idle_text, size);

	for (i = 0; i < ACPI_C_STATE_COUNT; i++) {

		cs = &sc->sc_cstate[i];

		if (cs->cs_method == ACPICPU_C_STATE_HALT) {
			ipi = true;
			break;
		}
	}

	x86_cpu_idle_set(acpicpu_cstate_idle, "acpi", ipi);

	return 0;
}

int
acpicpu_md_cstate_stop(void)
{
	static char text[16];
	void (*func)(void);
	uint64_t xc;
	bool ipi;

	x86_cpu_idle_get(&func, text, sizeof(text));

	if (func == native_idle)
		return EALREADY;

	ipi = (native_idle != x86_cpu_idle_halt) ? false : true;
	x86_cpu_idle_set(native_idle, native_idle_text, ipi);

	/*
	 * Run a cross-call to ensure that all CPUs are
	 * out from the ACPI idle-loop before detachment.
	 */
	xc = xc_broadcast(0, (xcfunc_t)nullop, NULL, NULL);
	xc_wait(xc);

	return 0;
}

/*
 * Called with interrupts enabled.
 */
void
acpicpu_md_cstate_enter(int method, int state)
{
	struct cpu_info *ci = curcpu();

	KASSERT(ci->ci_ilevel == IPL_NONE);

	switch (method) {

	case ACPICPU_C_STATE_FFH:

		x86_monitor(&ci->ci_want_resched, 0, 0);

		if (__predict_false(ci->ci_want_resched != 0))
			return;

		x86_mwait((state - 1) << 4, 0);
		break;

	case ACPICPU_C_STATE_HALT:

		x86_disable_intr();

		if (__predict_false(ci->ci_want_resched != 0)) {
			x86_enable_intr();
			return;
		}

		x86_stihlt();
		break;
	}
}

int
acpicpu_md_pstate_start(struct acpicpu_softc *sc)
{
	uint64_t xc, val;

	switch (cpu_vendor) {

	case CPUVENDOR_IDT:
	case CPUVENDOR_INTEL:

		/*
		 * Make sure EST is enabled.
		 */
		if ((sc->sc_flags & ACPICPU_FLAG_P_FFH) != 0) {

			val = rdmsr(MSR_MISC_ENABLE);

			if ((val & MSR_MISC_ENABLE_EST) == 0) {

				val |= MSR_MISC_ENABLE_EST;
				wrmsr(MSR_MISC_ENABLE, val);
				val = rdmsr(MSR_MISC_ENABLE);

				if ((val & MSR_MISC_ENABLE_EST) == 0)
					return ENOTTY;
			}
		}
	}

	/*
	 * Reset the APERF and MPERF counters.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_P_HWF) != 0) {
		xc = xc_broadcast(0, acpicpu_md_pstate_hwf_reset, NULL, NULL);
		xc_wait(xc);
	}

	return acpicpu_md_pstate_sysctl_init();
}

int
acpicpu_md_pstate_stop(void)
{

	if (acpicpu_log == NULL)
		return EALREADY;

	sysctl_teardown(&acpicpu_log);
	acpicpu_log = NULL;

	return 0;
}

int
acpicpu_md_pstate_init(struct acpicpu_softc *sc)
{
	struct cpu_info *ci = sc->sc_ci;
	struct acpicpu_pstate *ps, msr;
	uint32_t family, i = 0;

	(void)memset(&msr, 0, sizeof(struct acpicpu_pstate));

	switch (cpu_vendor) {

	case CPUVENDOR_IDT:
	case CPUVENDOR_INTEL:

		/*
		 * If the so-called Turbo Boost is present,
		 * the P0-state is always the "turbo state".
		 * It is shown as the P1 frequency + 1 MHz.
		 *
		 * For discussion, see:
		 *
		 *	Intel Corporation: Intel Turbo Boost Technology
		 *	in Intel Core(tm) Microarchitectures (Nehalem)
		 *	Based Processors. White Paper, November 2008.
		 */
		if (sc->sc_pstate_count >= 2 &&
		   (sc->sc_flags & ACPICPU_FLAG_P_TURBO) != 0) {

			ps = &sc->sc_pstate[0];

			if (ps->ps_freq == sc->sc_pstate[1].ps_freq + 1)
				ps->ps_flags |= ACPICPU_FLAG_P_TURBO;
		}

		msr.ps_control_addr = MSR_PERF_CTL;
		msr.ps_control_mask = __BITS(0, 15);

		msr.ps_status_addr  = MSR_PERF_STATUS;
		msr.ps_status_mask  = __BITS(0, 15);
		break;

	case CPUVENDOR_AMD:

		if ((sc->sc_flags & ACPICPU_FLAG_P_FIDVID) != 0)
			msr.ps_flags |= ACPICPU_FLAG_P_FIDVID;

		family = CPUID_TO_FAMILY(ci->ci_signature);

		switch (family) {

		case 0x0f:
			msr.ps_control_addr = MSR_0FH_CONTROL;
			msr.ps_status_addr  = MSR_0FH_STATUS;
			break;

		case 0x10:
		case 0x11:
		case 0x12:
		case 0x14:
		case 0x15:
			msr.ps_control_addr = MSR_10H_CONTROL;
			msr.ps_control_mask = __BITS(0, 2);

			msr.ps_status_addr  = MSR_10H_STATUS;
			msr.ps_status_mask  = __BITS(0, 2);
			break;

		default:
			/*
			 * If we have an unknown AMD CPU, rely on XPSS.
			 */
			if ((sc->sc_flags & ACPICPU_FLAG_P_XPSS) == 0)
				return EOPNOTSUPP;
		}

		break;

	default:
		return ENODEV;
	}

	/*
	 * Fill the P-state structures with MSR addresses that are
	 * known to be correct. If we do not know the addresses,
	 * leave the values intact. If a vendor uses XPSS, we do
	 * not necessarily need to do anything to support new CPUs.
	 */
	while (i < sc->sc_pstate_count) {

		ps = &sc->sc_pstate[i];

		if (msr.ps_flags != 0)
			ps->ps_flags |= msr.ps_flags;

		if (msr.ps_status_addr != 0)
			ps->ps_status_addr = msr.ps_status_addr;

		if (msr.ps_status_mask != 0)
			ps->ps_status_mask = msr.ps_status_mask;

		if (msr.ps_control_addr != 0)
			ps->ps_control_addr = msr.ps_control_addr;

		if (msr.ps_control_mask != 0)
			ps->ps_control_mask = msr.ps_control_mask;

		i++;
	}

	return 0;
}

/*
 * Read the IA32_APERF and IA32_MPERF counters. The first
 * increments at the rate of the fixed maximum frequency
 * configured during the boot, whereas APERF counts at the
 * rate of the actual frequency. Note that the MSRs must be
 * read without delay, and that only the ratio between
 * IA32_APERF and IA32_MPERF is architecturally defined.
 *
 * The function thus returns the percentage of the actual
 * frequency in terms of the maximum frequency of the calling
 * CPU since the last call. A value zero implies an error.
 *
 * For further details, refer to:
 *
 *	Intel Corporation: Intel 64 and IA-32 Architectures
 *	Software Developer's Manual. Section 13.2, Volume 3A:
 *	System Programming Guide, Part 1. July, 2008.
 *
 *	Advanced Micro Devices: BIOS and Kernel Developer's
 *	Guide (BKDG) for AMD Family 10h Processors. Section
 *	2.4.5, Revision 3.48, April 2010.
 */
uint8_t
acpicpu_md_pstate_hwf(struct cpu_info *ci)
{
	struct acpicpu_softc *sc;
	uint64_t aperf, mperf;
	uint8_t rv = 0;

	sc = acpicpu_sc[ci->ci_acpiid];

	if (__predict_false(sc == NULL))
		return 0;

	if (__predict_false((sc->sc_flags & ACPICPU_FLAG_P_HWF) == 0))
		return 0;

	aperf = sc->sc_pstate_aperf;
	mperf = sc->sc_pstate_mperf;

	x86_disable_intr();

	sc->sc_pstate_aperf = rdmsr(MSR_APERF);
	sc->sc_pstate_mperf = rdmsr(MSR_MPERF);

	x86_enable_intr();

	aperf = sc->sc_pstate_aperf - aperf;
	mperf = sc->sc_pstate_mperf - mperf;

	if (__predict_true(mperf != 0))
		rv = (aperf * 100) / mperf;

	return rv;
}

static void
acpicpu_md_pstate_hwf_reset(void *arg1, void *arg2)
{
	struct cpu_info *ci = curcpu();
	struct acpicpu_softc *sc;

	sc = acpicpu_sc[ci->ci_acpiid];

	if (__predict_false(sc == NULL))
		return;

	x86_disable_intr();

	wrmsr(MSR_APERF, 0);
	wrmsr(MSR_MPERF, 0);

	x86_enable_intr();

	sc->sc_pstate_aperf = 0;
	sc->sc_pstate_mperf = 0;
}

int
acpicpu_md_pstate_get(struct acpicpu_softc *sc, uint32_t *freq)
{
	struct acpicpu_pstate *ps = NULL;
	uint64_t val;
	uint32_t i;

	if ((sc->sc_flags & ACPICPU_FLAG_P_FIDVID) != 0)
		return acpicpu_md_pstate_fidvid_get(sc, freq);

	/*
	 * Pick any P-state for the status address.
	 */
	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (__predict_true(ps->ps_freq != 0))
			break;
	}

	if (__predict_false(ps == NULL))
		return ENODEV;

	if (__predict_false(ps->ps_status_addr == 0))
		return EINVAL;

	val = rdmsr(ps->ps_status_addr);

	if (__predict_true(ps->ps_status_mask != 0))
		val = val & ps->ps_status_mask;

	/*
	 * Search for the value from known P-states.
	 */
	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (__predict_false(ps->ps_freq == 0))
			continue;

		if (val == ps->ps_status) {
			*freq = ps->ps_freq;
			return 0;
		}
	}

	/*
	 * If the value was not found, try APERF/MPERF.
	 * The state is P0 if the return value is 100 %.
	 */
	if ((sc->sc_flags & ACPICPU_FLAG_P_HWF) != 0) {

		KASSERT(sc->sc_pstate_count > 0);
		KASSERT(sc->sc_pstate[0].ps_freq != 0);

		if (acpicpu_md_pstate_hwf(sc->sc_ci) == 100) {
			*freq = sc->sc_pstate[0].ps_freq;
			return 0;
		}
	}

	return EIO;
}

int
acpicpu_md_pstate_set(struct acpicpu_pstate *ps)
{
	uint64_t val = 0;

	if (__predict_false(ps->ps_control_addr == 0))
		return EINVAL;

	if ((ps->ps_flags & ACPICPU_FLAG_P_FIDVID) != 0)
		return acpicpu_md_pstate_fidvid_set(ps);

	/*
	 * If the mask is set, do a read-modify-write.
	 */
	if (__predict_true(ps->ps_control_mask != 0)) {
		val = rdmsr(ps->ps_control_addr);
		val &= ~ps->ps_control_mask;
	}

	val |= ps->ps_control;

	wrmsr(ps->ps_control_addr, val);
	DELAY(ps->ps_latency);

	return 0;
}

static int
acpicpu_md_pstate_fidvid_get(struct acpicpu_softc *sc, uint32_t *freq)
{
	struct acpicpu_pstate *ps;
	uint32_t fid, i, vid;
	uint32_t cfid, cvid;
	int rv;

	/*
	 * AMD family 0Fh needs special treatment.
	 * While it wants to use ACPI, it does not
	 * comply with the ACPI specifications.
	 */
	rv = acpicpu_md_pstate_fidvid_read(&cfid, &cvid);

	if (rv != 0)
		return rv;

	for (i = 0; i < sc->sc_pstate_count; i++) {

		ps = &sc->sc_pstate[i];

		if (__predict_false(ps->ps_freq == 0))
			continue;

		fid = __SHIFTOUT(ps->ps_status, ACPI_0FH_STATUS_FID);
		vid = __SHIFTOUT(ps->ps_status, ACPI_0FH_STATUS_VID);

		if (cfid == fid && cvid == vid) {
			*freq = ps->ps_freq;
			return 0;
		}
	}

	return EIO;
}

static int
acpicpu_md_pstate_fidvid_set(struct acpicpu_pstate *ps)
{
	const uint64_t ctrl = ps->ps_control;
	uint32_t cfid, cvid, fid, i, irt;
	uint32_t pll, vco_cfid, vco_fid;
	uint32_t val, vid, vst;
	int rv;

	rv = acpicpu_md_pstate_fidvid_read(&cfid, &cvid);

	if (rv != 0)
		return rv;

	fid = __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_FID);
	vid = __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_VID);
	irt = __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_IRT);
	vst = __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_VST);
	pll = __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_PLL);

	vst = vst * 20;
	pll = pll * 1000 / 5;
	irt = 10 * __BIT(irt);

	/*
	 * Phase 1.
	 */
	while (cvid > vid) {

		val = 1 << __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_MVS);
		val = (val > cvid) ? 0 : cvid - val;

		acpicpu_md_pstate_fidvid_write(cfid, val, 1, vst);
		rv = acpicpu_md_pstate_fidvid_read(NULL, &cvid);

		if (rv != 0)
			return rv;
	}

	i = __SHIFTOUT(ctrl, ACPI_0FH_CONTROL_RVO);

	for (; i > 0 && cvid > 0; --i) {

		acpicpu_md_pstate_fidvid_write(cfid, cvid - 1, 1, vst);
		rv = acpicpu_md_pstate_fidvid_read(NULL, &cvid);

		if (rv != 0)
			return rv;
	}

	/*
	 * Phase 2.
	 */
	if (cfid != fid) {

		vco_fid  = FID_TO_VCO_FID(fid);
		vco_cfid = FID_TO_VCO_FID(cfid);

		while (abs(vco_fid - vco_cfid) > 2) {

			if (fid <= cfid)
				val = cfid - 2;
			else {
				val = (cfid > 6) ? cfid + 2 :
				    FID_TO_VCO_FID(cfid) + 2;
			}

			acpicpu_md_pstate_fidvid_write(val, cvid, pll, irt);
			rv = acpicpu_md_pstate_fidvid_read(&cfid, NULL);

			if (rv != 0)
				return rv;

			vco_cfid = FID_TO_VCO_FID(cfid);
		}

		acpicpu_md_pstate_fidvid_write(fid, cvid, pll, irt);
		rv = acpicpu_md_pstate_fidvid_read(&cfid, NULL);

		if (rv != 0)
			return rv;
	}

	/*
	 * Phase 3.
	 */
	if (cvid != vid) {

		acpicpu_md_pstate_fidvid_write(cfid, vid, 1, vst);
		rv = acpicpu_md_pstate_fidvid_read(NULL, &cvid);

		if (rv != 0)
			return rv;
	}

	return 0;
}

static int
acpicpu_md_pstate_fidvid_read(uint32_t *cfid, uint32_t *cvid)
{
	int i = ACPICPU_P_STATE_RETRY * 100;
	uint64_t val;

	do {
		val = rdmsr(MSR_0FH_STATUS);

	} while (__SHIFTOUT(val, MSR_0FH_STATUS_PENDING) != 0 && --i >= 0);

	if (i == 0)
		return EAGAIN;

	if (cfid != NULL)
		*cfid = __SHIFTOUT(val, MSR_0FH_STATUS_CFID);

	if (cvid != NULL)
		*cvid = __SHIFTOUT(val, MSR_0FH_STATUS_CVID);

	return 0;
}

static void
acpicpu_md_pstate_fidvid_write(uint32_t fid,
    uint32_t vid, uint32_t cnt, uint32_t tmo)
{
	uint64_t val = 0;

	val |= __SHIFTIN(fid, MSR_0FH_CONTROL_FID);
	val |= __SHIFTIN(vid, MSR_0FH_CONTROL_VID);
	val |= __SHIFTIN(cnt, MSR_0FH_CONTROL_CNT);
	val |= __SHIFTIN(0x1, MSR_0FH_CONTROL_CHG);

	wrmsr(MSR_0FH_CONTROL, val);
	DELAY(tmo);
}

int
acpicpu_md_tstate_get(struct acpicpu_softc *sc, uint32_t *percent)
{
	struct acpicpu_tstate *ts;
	uint64_t val;
	uint32_t i;

	val = rdmsr(MSR_THERM_CONTROL);

	for (i = 0; i < sc->sc_tstate_count; i++) {

		ts = &sc->sc_tstate[i];

		if (ts->ts_percent == 0)
			continue;

		if (val == ts->ts_status) {
			*percent = ts->ts_percent;
			return 0;
		}
	}

	return EIO;
}

int
acpicpu_md_tstate_set(struct acpicpu_tstate *ts)
{
	uint64_t val;
	uint8_t i;

	val = ts->ts_control;
	val = val & __BITS(0, 4);

	wrmsr(MSR_THERM_CONTROL, val);

	if (ts->ts_status == 0) {
		DELAY(ts->ts_latency);
		return 0;
	}

	for (i = val = 0; i < ACPICPU_T_STATE_RETRY; i++) {

		val = rdmsr(MSR_THERM_CONTROL);

		if (val == ts->ts_status)
			return 0;

		DELAY(ts->ts_latency);
	}

	return EAGAIN;
}

/*
 * A kludge for backwards compatibility.
 */
static int
acpicpu_md_pstate_sysctl_init(void)
{
	const struct sysctlnode	*fnode, *mnode, *rnode;
	const char *str;
	int rv;

	switch (cpu_vendor) {

	case CPUVENDOR_IDT:
	case CPUVENDOR_INTEL:
		str = "est";
		break;

	case CPUVENDOR_AMD:
		str = "powernow";
		break;

	default:
		return ENODEV;
	}


	rv = sysctl_createv(&acpicpu_log, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&acpicpu_log, 0, &rnode, &mnode,
	    0, CTLTYPE_NODE, str, NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&acpicpu_log, 0, &mnode, &fnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&acpicpu_log, 0, &fnode, &rnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    acpicpu_md_pstate_sysctl_set, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&acpicpu_log, 0, &fnode, &rnode,
	    CTLFLAG_READONLY, CTLTYPE_INT, "current", NULL,
	    acpicpu_md_pstate_sysctl_get, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&acpicpu_log, 0, &fnode, &rnode,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "available", NULL,
	    acpicpu_md_pstate_sysctl_all, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	return 0;

fail:
	if (acpicpu_log != NULL) {
		sysctl_teardown(&acpicpu_log);
		acpicpu_log = NULL;
	}

	return rv;
}

static int
acpicpu_md_pstate_sysctl_get(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t freq;
	int err;

	freq = cpufreq_get(curcpu());

	if (freq == 0)
		return ENXIO;

	node = *rnode;
	node.sysctl_data = &freq;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err != 0 || newp == NULL)
		return err;

	return 0;
}

static int
acpicpu_md_pstate_sysctl_set(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t freq;
	int err;

	freq = cpufreq_get(curcpu());

	if (freq == 0)
		return ENXIO;

	node = *rnode;
	node.sysctl_data = &freq;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err != 0 || newp == NULL)
		return err;

	cpufreq_set_all(freq);

	return 0;
}

static int
acpicpu_md_pstate_sysctl_all(SYSCTLFN_ARGS)
{
	struct cpu_info *ci = curcpu();
	struct acpicpu_softc *sc;
	struct sysctlnode node;
	char buf[1024];
	size_t len;
	uint32_t i;
	int err;

	sc = acpicpu_sc[ci->ci_acpiid];

	if (sc == NULL)
		return ENXIO;

	(void)memset(&buf, 0, sizeof(buf));

	mutex_enter(&sc->sc_mtx);

	for (len = 0, i = sc->sc_pstate_max; i < sc->sc_pstate_count; i++) {

		if (sc->sc_pstate[i].ps_freq == 0)
			continue;

		if (len >= sizeof(buf))
			break;
		len += snprintf(buf + len, sizeof(buf) - len, "%u%s",
		    sc->sc_pstate[i].ps_freq,
		    i < (sc->sc_pstate_count - 1) ? " " : "");
	}

	mutex_exit(&sc->sc_mtx);

	node = *rnode;
	node.sysctl_data = buf;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err != 0 || newp == NULL)
		return err;

	return 0;
}

