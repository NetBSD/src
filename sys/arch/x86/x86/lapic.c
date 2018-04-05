/*	$NetBSD: lapic.c,v 1.58.2.5 2018/04/05 18:15:03 martin Exp $	*/

/*-
 * Copyright (c) 2000, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
__KERNEL_RCSID(0, "$NetBSD: lapic.c,v 1.58.2.5 2018/04/05 18:15:03 martin Exp $");

#include "acpica.h"
#include "ioapic.h"
#include "opt_acpi.h"
#include "opt_ddb.h"
#include "opt_mpbios.h"		/* for MPDEBUG */
#include "opt_multiprocessor.h"
#include "opt_ntp.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <uvm/uvm_extern.h>

#include <dev/ic/i8253reg.h>

#include <machine/cpu.h>
#include <machine/cpu_counter.h>
#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>
#include <machine/mpacpi.h>
#include <machine/mpbiosvar.h>
#include <machine/pcb.h>
#include <machine/specialreg.h>
#include <machine/segments.h>
#include <x86/x86/tsc.h>
#include <x86/i82093var.h>

#include <machine/apicvar.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>

#if NACPICA > 0
#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#ifdef MULTIPROCESSOR
#ifdef __x86_64__
typedef void (vector)(void);
extern vector Xintr_x2apic_ddbipi;
extern int ddb_vec;
#endif
#endif
#endif

#include <x86/x86/vmtreg.h>	/* for vmt_hvcall() */
#include <x86/x86/vmtvar.h>	/* for vmt_hvcall() */

/* Referenced from vector.S */
void		lapic_clockintr(void *, struct intrframe *);

static void	lapic_delay(unsigned int);
static uint32_t lapic_gettick(void);
static void 	lapic_setup_bsp(paddr_t);
static void 	lapic_map(paddr_t);

static void lapic_hwmask(struct pic *, int);
static void lapic_hwunmask(struct pic *, int);
static void lapic_setup(struct pic *, struct cpu_info *, int, int, int);
/* Make it public to call via ddb */
void	lapic_dump(void);

struct pic local_pic = {
	.pic_name = "lapic",
	.pic_type = PIC_LAPIC,
	.pic_lock = __SIMPLELOCK_UNLOCKED,
	.pic_hwmask = lapic_hwmask,
	.pic_hwunmask = lapic_hwunmask,
	.pic_addroute = lapic_setup,
	.pic_delroute = lapic_setup,
};

static int i82489_ipi(int vec, int target, int dl);
static int x2apic_ipi(int vec, int target, int dl);
int (*x86_ipi)(int, int, int) = i82489_ipi;

bool x2apic_mode __read_mostly;
#ifdef LAPIC_ENABLE_X2APIC
bool x2apic_enable = true;
#else
bool x2apic_enable = false;
#endif

static uint32_t
i82489_readreg(u_int reg)
{
	return *((volatile uint32_t *)(local_apic_va + reg));
}

static void
i82489_writereg(u_int reg, uint32_t val)
{
	*((volatile uint32_t *)(local_apic_va + reg)) = val;
}

static uint32_t
i82489_cpu_number(void)
{
	return i82489_readreg(LAPIC_ID) >> LAPIC_ID_SHIFT;
}

static uint32_t
x2apic_readreg(u_int reg)
{
	return rdmsr(MSR_X2APIC_BASE + (reg >> 4));
}

static void
x2apic_writereg(u_int reg, uint32_t val)
{
	x86_mfence();
	wrmsr(MSR_X2APIC_BASE + (reg >> 4), val);
}

static void
x2apic_writereg64(u_int reg, uint64_t val)
{
	KDASSERT(reg == LAPIC_ICRLO);
	x86_mfence();
	wrmsr(MSR_X2APIC_BASE + (reg >> 4), val);
}

static void
x2apic_write_icr(uint32_t hi, uint32_t lo)
{
	x2apic_writereg64(LAPIC_ICRLO, ((uint64_t)hi << 32) | lo);
}

static uint32_t
x2apic_cpu_number(void)
{
	return x2apic_readreg(LAPIC_ID);
}

uint32_t
lapic_readreg(u_int reg)
{
	if (x2apic_mode)
		return x2apic_readreg(reg);
	return i82489_readreg(reg);
}

void
lapic_writereg(u_int reg, uint32_t val)
{
	if (x2apic_mode)
		x2apic_writereg(reg, val);
	else
		i82489_writereg(reg, val);
}

void
lapic_write_tpri(uint32_t val)
{

	val &= LAPIC_TPRI_MASK;
#ifdef i386
	lapic_writereg(LAPIC_TPRI, val);
#else
	lcr8(val >> 4);
#endif
}

void
lapic_eoi(void)
{
	lapic_writereg(LAPIC_EOI, 0);
}

uint32_t
lapic_cpu_number(void)
{
	if (x2apic_mode)
		return x2apic_cpu_number();
	return i82489_cpu_number();
}

static void
lapic_enable_x2apic(void)
{
	uint64_t apicbase;

	apicbase = rdmsr(MSR_APICBASE);
	if (!ISSET(apicbase, APICBASE_EN)) {
		apicbase |= APICBASE_EN;
		wrmsr(MSR_APICBASE, apicbase);
	}
	apicbase |= APICBASE_EXTD;
	wrmsr(MSR_APICBASE, apicbase);
}

bool
lapic_is_x2apic(void)
{
	uint64_t msr;

	if (!ISSET(cpu_feature[0], CPUID_APIC) ||
	    rdmsr_safe(MSR_APICBASE, &msr) == EFAULT)
		return false;
	return (msr & (APICBASE_EN | APICBASE_EXTD)) ==
	    (APICBASE_EN | APICBASE_EXTD);
}

/*
 * Initialize the local APIC on the BSP.
 */
static void
lapic_setup_bsp(paddr_t lapic_base)
{
	u_int regs[6];
	const char *reason = NULL;
	const char *hw_vendor;
	bool bios_x2apic;

	if (ISSET(cpu_feature[1], CPUID2_X2APIC)) {
#if NACPICA > 0
		if (acpi_present) {
			ACPI_TABLE_DMAR *dmar;
			ACPI_STATUS status;

			/*
			 * Automatically detect several configurations where
			 * x2APIC mode is known to cause troubles.  User can
			 * override the setting with hw.x2apic_enable tunable.
			 */
			status = AcpiGetTable(ACPI_SIG_DMAR, 1,
			    (ACPI_TABLE_HEADER **)&dmar);
			if (ACPI_SUCCESS(status)) {
				if (ISSET(dmar->Flags, ACPI_DMAR_X2APIC_OPT_OUT)) {
					reason = "by DMAR table";
				}
				AcpiPutTable(&dmar->Header);
			}
		}
#endif	/* NACPICA > 0 */
		if (vm_guest == VM_GUEST_VMWARE) {
			vmt_hvcall(VM_CMD_GET_VCPU_INFO, regs);
			if (ISSET(regs[0], VCPUINFO_VCPU_RESERVED) ||
			    !ISSET(regs[0], VCPUINFO_LEGACY_X2APIC))
				reason = "inside VMWare without intr redirection";
		} else if (vm_guest == VM_GUEST_XEN) {
			reason = "due to running under XEN";
		} else if (vm_guest == VM_GUEST_NO &&
		    CPUID_TO_FAMILY(curcpu()->ci_signature) == 6 &&
		    CPUID_TO_MODEL(curcpu()->ci_signature) == 0x2a) {
			hw_vendor = pmf_get_platform("board-vendor");
			if (hw_vendor != NULL) {
				/*
				 * It seems that some Lenovo and ASUS
				 * SandyBridge-based notebook BIOSes have a bug
				 * which prevents booting AP in x2APIC mode.
				 * Since the only way to detect mobile CPU is
				 * to check northbridge pci id, which cannot be done
				 * that early, disable x2APIC for all Lenovo and ASUS
				 * SandyBridge machines.
				 */
				if (strcmp(hw_vendor, "LENOVO") == 0 ||
				    strcmp(hw_vendor, "ASUSTeK Computer Inc.") == 0) {
					reason =
					    "for a suspected SandyBridge BIOS bug";
				}
			}
		}
		bios_x2apic = lapic_is_x2apic();
		if (reason != NULL && bios_x2apic) {
			aprint_verbose("x2APIC should be disabled %s but "
			    "already enabled by BIOS; enabling.\n", reason);
			reason = NULL;
		}
		if (reason == NULL)
			x2apic_mode = true;
		else
			aprint_verbose("x2APIC available but disabled %s\n", reason);
		if (x2apic_enable != x2apic_mode) {
			if (bios_x2apic && !x2apic_enable)
				aprint_verbose("x2APIC disabled by user and "
				    "enabled by BIOS; ignoring user setting.\n");
			else
				x2apic_mode = x2apic_enable;
		}
	}
	if (x2apic_mode) {
		x86_ipi = x2apic_ipi;
#if NIOAPIC > 0
		struct ioapic_softc *ioapic;
		for (ioapic = ioapics; ioapic != NULL; ioapic = ioapic->sc_next) {
			ioapic->sc_pic.pic_edge_stubs = x2apic_edge_stubs;
			ioapic->sc_pic.pic_level_stubs = x2apic_level_stubs;
		}
#endif
#if defined(DDB) && defined(MULTIPROCESSOR)
#ifdef __x86_64__
		setgate(&idt[ddb_vec], &Xintr_x2apic_ddbipi, 1, SDT_SYS386IGT, SEL_KPL,
		    GSEL(GCODE_SEL, SEL_KPL));
#else
		/* Set DDB IPI handler in cpu_set_tss_gates() when cpu0 is attached. */
#endif
#endif

		x86_disable_intr();
		lapic_enable_x2apic();
#ifdef MULTIPROCESSOR
		cpu_init_first();	/* catch up to changed cpu_number() */
#endif
		lapic_write_tpri(0);
		x86_enable_intr();
	} else
		lapic_map(lapic_base);
}

static void
lapic_map(paddr_t lapic_base)
{
	pt_entry_t *pte;
	vaddr_t va = local_apic_va;

	/*
	 * If the CPU has an APIC MSR, use it and ignore the supplied value:
	 * some ACPI implementations have been observed to pass bad values.
	 * Additionally, ensure that the lapic is enabled as we are committed
	 * to using it at this point.  Be conservative and assume that the MSR
	 * is not present on the Pentium (is it?).
	 */
	if (CPUID_TO_FAMILY(curcpu()->ci_signature) >= 6) {
		lapic_base = (paddr_t)rdmsr(MSR_APICBASE);
		if ((lapic_base & APICBASE_PHYSADDR) == 0) {
			lapic_base |= LAPIC_BASE;
		}
		wrmsr(MSR_APICBASE, lapic_base | APICBASE_EN);
		lapic_base &= APICBASE_PHYSADDR;
	}

	x86_disable_intr();

	/*
	 * Map local apic.  If we have a local apic, it's safe to assume
	 * we're on a 486 or better and can use invlpg and non-cacheable PTE's
	 *
	 * Whap the PTE "by hand" rather than calling pmap_kenter_pa because
	 * the latter will attempt to invoke TLB shootdown code just as we
	 * might have changed the value of cpu_number()..
	 */

	pte = kvtopte(va);
	*pte = lapic_base | PG_RW | PG_V | PG_N | pmap_pg_g | pmap_pg_nx;
	invlpg(va);

#ifdef MULTIPROCESSOR
	cpu_init_first();	/* catch up to changed cpu_number() */
#endif

	lapic_write_tpri(0);
	x86_enable_intr();
}

/*
 * enable local apic
 */
void
lapic_enable(void)
{
	lapic_writereg(LAPIC_SVR, LAPIC_SVR_ENABLE | LAPIC_SPURIOUS_VECTOR);
}

void
lapic_set_lvt(void)
{
	struct cpu_info *ci = curcpu();
	int i;
	struct mp_intr_map *mpi;
	uint32_t lint0, lint1;

#ifdef MULTIPROCESSOR
	if (mp_verbose) {
		apic_format_redir(device_xname(ci->ci_dev), "prelint", 0, 0,
		    lapic_readreg(LAPIC_LVINT0));
		apic_format_redir(device_xname(ci->ci_dev), "prelint", 1, 0,
		    lapic_readreg(LAPIC_LVINT1));
	}
#endif

	/*
	 * If an I/O APIC has been attached, assume that it is used instead of
	 * the 8259A for interrupt delivery.  Otherwise request the LAPIC to
	 * get external interrupts via LINT0 for the primary CPU.
	 */
	lint0 = LAPIC_LVT_DM_EXTINT;
	if (nioapics > 0 || !CPU_IS_PRIMARY(curcpu()))
		lint0 |= LAPIC_LVT_MASKED;
	lapic_writereg(LAPIC_LVINT0, lint0);

	/*
	 * Non Maskable Interrupts are to be delivered to the primary CPU.
	 */
	lint1 = LAPIC_LVT_DM_NMI;
	if (!CPU_IS_PRIMARY(curcpu()))
		lint1 |= LAPIC_LVT_MASKED;
	lapic_writereg(LAPIC_LVINT1, lint1);

	for (i = 0; i < mp_nintr; i++) {
		mpi = &mp_intrs[i];
		if (mpi->ioapic == NULL && (mpi->cpu_id == MPS_ALL_APICS ||
		    mpi->cpu_id == ci->ci_cpuid)) {
			if (mpi->ioapic_pin > 1)
				aprint_error_dev(ci->ci_dev,
				    "%s: WARNING: bad pin value %d\n",
				    __func__, mpi->ioapic_pin);
			if (mpi->ioapic_pin == 0)
				lapic_writereg(LAPIC_LVINT0, mpi->redir);
			else
				lapic_writereg(LAPIC_LVINT1, mpi->redir);
		}
	}

#ifdef MULTIPROCESSOR
	if (mp_verbose)
		lapic_dump();
#endif
}

/*
 * Initialize fixed idt vectors for use by local apic.
 */
void
lapic_boot_init(paddr_t lapic_base)
{

	lapic_setup_bsp(lapic_base);

#ifdef MULTIPROCESSOR
	idt_vec_reserve(LAPIC_IPI_VECTOR);
	idt_vec_set(LAPIC_IPI_VECTOR, x2apic_mode ? Xintr_x2apic_ipi : Xintr_lapic_ipi);
	idt_vec_reserve(LAPIC_TLB_VECTOR);
	idt_vec_set(LAPIC_TLB_VECTOR, x2apic_mode ? Xintr_x2apic_tlb : Xintr_lapic_tlb);
#endif
	idt_vec_reserve(LAPIC_SPURIOUS_VECTOR);
	idt_vec_set(LAPIC_SPURIOUS_VECTOR, Xintrspurious);

	idt_vec_reserve(LAPIC_TIMER_VECTOR);
	idt_vec_set(LAPIC_TIMER_VECTOR, x2apic_mode ? Xintr_x2apic_ltimer :
	    Xintr_lapic_ltimer);
}

static uint32_t
lapic_gettick(void)
{
	return lapic_readreg(LAPIC_CCR_TIMER);
}

#include <sys/kernel.h>		/* for hz */

int lapic_timer = 0;
uint32_t lapic_tval;

/*
 * this gets us up to a 4GHz busclock....
 */
uint32_t lapic_per_second;
uint32_t lapic_frac_usec_per_cycle;
uint64_t lapic_frac_cycle_per_usec;
uint32_t lapic_delaytab[26];

static u_int
lapic_get_timecount(struct timecounter *tc)
{
	struct cpu_info *ci;
	uint32_t cur_timer;
	int s;

	s = splhigh();
	ci = curcpu();

	/*
	 * Check for a race against the clockinterrupt.
	 * The update of ci_lapic_counter is blocked by splhigh() and
	 * the check for a pending clockinterrupt compensates for that.
	 *
	 * If the current tick is almost the Initial Counter, explicitly
	 * check for the pending interrupt bit as the interrupt delivery
	 * could be asynchronious and compensate as well.
	 *
	 * This can't be done without splhigh() as the calling code might
	 * have masked the clockinterrupt already.
	 *
	 * This code assumes that clockinterrupts are not missed.
	 */
	cur_timer = lapic_gettick();
	if (cur_timer >= lapic_tval - 1) {
		uint16_t reg = LAPIC_IRR + LAPIC_TIMER_VECTOR / 32 * 16;

		if (lapic_readreg(reg) & (1 << (LAPIC_TIMER_VECTOR % 32))) {
			cur_timer -= lapic_tval;
		}
	} else if (ci->ci_istate.ipending & (1 << LIR_TIMER))
		cur_timer = lapic_gettick() - lapic_tval;
	cur_timer = ci->ci_lapic_counter - cur_timer;
	splx(s);

	return cur_timer;
}

static struct timecounter lapic_timecounter = {
	lapic_get_timecount,
	NULL,
	~0u,
	0,
	"lapic",
#ifndef MULTIPROCESSOR
	2100,
#else
	-100, /* per CPU state */
#endif
	NULL,
	NULL,
};

extern u_int i8254_get_timecount(struct timecounter *);

void
lapic_clockintr(void *arg, struct intrframe *frame)
{
	struct cpu_info *ci = curcpu();

	ci->ci_lapic_counter += lapic_tval;
	ci->ci_isources[LIR_TIMER]->is_evcnt.ev_count++;
	hardclock((struct clockframe *)frame);
}

void
lapic_initclocks(void)
{
	/*
	 * Start local apic countdown timer running, in repeated mode.
	 *
	 * Mask the clock interrupt and set mode,
	 * then set divisor,
	 * then unmask and set the vector.
	 */
	lapic_writereg(LAPIC_LVTT, LAPIC_LVTT_TM | LAPIC_LVTT_M);
	lapic_writereg(LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	lapic_writereg(LAPIC_ICR_TIMER, lapic_tval);
	lapic_writereg(LAPIC_LVTT, LAPIC_LVTT_TM | LAPIC_TIMER_VECTOR);
	lapic_eoi();
}

extern unsigned int gettick(void);	/* XXX put in header file */
extern u_long rtclock_tval; /* XXX put in header file */
extern void (*initclock_func)(void); /* XXX put in header file */

/*
 * Calibrate the local apic count-down timer (which is running at
 * bus-clock speed) vs. the i8254 counter/timer (which is running at
 * a fixed rate).
 *
 * The Intel MP spec says: "An MP operating system may use the IRQ8
 * real-time clock as a reference to determine the actual APIC timer clock
 * speed."
 *
 * We're actually using the IRQ0 timer.  Hmm.
 */
void
lapic_calibrate_timer(struct cpu_info *ci)
{
	unsigned int seen, delta, initial_i8254, initial_lapic;
	unsigned int cur_i8254, cur_lapic;
	uint64_t tmp;
	int i;
	char tbuf[9];

	aprint_debug_dev(ci->ci_dev, "calibrating local timer\n");

	/*
	 * Configure timer to one-shot, interrupt masked,
	 * large positive number.
	 */
	lapic_writereg(LAPIC_LVTT, LAPIC_LVTT_M);
	lapic_writereg(LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
	lapic_writereg(LAPIC_ICR_TIMER, 0x80000000);

	x86_disable_intr();

	initial_lapic = lapic_gettick();
	initial_i8254 = gettick();

	for (seen = 0; seen < TIMER_FREQ / 100; seen += delta) {
		cur_i8254 = gettick();
		if (cur_i8254 > initial_i8254)
			delta = rtclock_tval - (cur_i8254 - initial_i8254);
		else
			delta = initial_i8254 - cur_i8254;
		initial_i8254 = cur_i8254;
	}
	cur_lapic = lapic_gettick();

	x86_enable_intr();

	tmp = initial_lapic - cur_lapic;
	lapic_per_second = (tmp * TIMER_FREQ + seen / 2) / seen;

	humanize_number(tbuf, sizeof(tbuf), lapic_per_second, "Hz", 1000);

	aprint_debug_dev(ci->ci_dev, "apic clock running at %s\n", tbuf);

	if (lapic_per_second != 0) {
		/*
		 * reprogram the apic timer to run in periodic mode.
		 * XXX need to program timer on other CPUs, too.
		 */
		lapic_tval = (lapic_per_second * 2) / hz;
		lapic_tval = (lapic_tval / 2) + (lapic_tval & 0x1);

		lapic_writereg(LAPIC_LVTT, LAPIC_LVTT_TM | LAPIC_LVTT_M
		    | LAPIC_TIMER_VECTOR);
		lapic_writereg(LAPIC_DCR_TIMER, LAPIC_DCRT_DIV1);
		lapic_writereg(LAPIC_ICR_TIMER, lapic_tval);

		/*
		 * Compute fixed-point ratios between cycles and
		 * microseconds to avoid having to do any division
		 * in lapic_delay.
		 */

		tmp = (1000000 * (uint64_t)1 << 32) / lapic_per_second;
		lapic_frac_usec_per_cycle = tmp;

		tmp = (lapic_per_second * (uint64_t)1 << 32) / 1000000;

		lapic_frac_cycle_per_usec = tmp;

		/*
		 * Compute delay in cycles for likely short delays in usec.
		 */
		for (i = 0; i < 26; i++)
			lapic_delaytab[i] = (lapic_frac_cycle_per_usec * i) >>
			    32;

		/*
		 * Now that the timer's calibrated, use the apic timer routines
		 * for all our timing needs..
		 */
		delay_func = lapic_delay;
		initclock_func = lapic_initclocks;
		initrtclock(0);

		if (lapic_timecounter.tc_frequency == 0) {
			/*
			 * Hook up time counter.
			 * This assume that all LAPICs have the same frequency.
			 */
			lapic_timecounter.tc_frequency = lapic_per_second;
			tc_init(&lapic_timecounter);
		}
	}
}

/*
 * delay for N usec.
 */

static void
lapic_delay(unsigned int usec)
{
	int32_t xtick, otick;
	int64_t deltat;		/* XXX may want to be 64bit */

	otick = lapic_gettick();

	if (usec <= 0)
		return;
	if (usec <= 25)
		deltat = lapic_delaytab[usec];
	else
		deltat = (lapic_frac_cycle_per_usec * usec) >> 32;

	while (deltat > 0) {
		xtick = lapic_gettick();
		if (xtick > otick)
			deltat -= lapic_tval - (xtick - otick);
		else
			deltat -= otick - xtick;
		otick = xtick;

		x86_pause();
	}
}

/*
 * XXX the following belong mostly or partly elsewhere..
 */

static void
i82489_icr_wait(void)
{
#ifdef DIAGNOSTIC
	unsigned j = 100000;
#endif /* DIAGNOSTIC */

	while ((i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0) {
		x86_pause();
#ifdef DIAGNOSTIC
		j--;
		if (j == 0)
			panic("i82489_icr_wait: busy");
#endif /* DIAGNOSTIC */
	}
}

static int
i82489_ipi_init(int target)
{
	uint32_t esr;

	i82489_writereg(LAPIC_ESR, 0);
	(void)i82489_readreg(LAPIC_ESR);

	i82489_writereg(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);

	i82489_writereg(LAPIC_ICRLO, LAPIC_DLMODE_INIT | LAPIC_LEVEL_ASSERT);
	i82489_icr_wait();
	i8254_delay(10000);
	i82489_writereg(LAPIC_ICRLO,
	    LAPIC_DLMODE_INIT | LAPIC_TRIGGER_LEVEL | LAPIC_LEVEL_DEASSERT);
	i82489_icr_wait();

	if ((i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0)
		return EBUSY;

	esr = i82489_readreg(LAPIC_ESR);
	if (esr != 0)
		aprint_debug("%s: ESR %08x\n", __func__, esr);

	return 0;
}

static int
i82489_ipi_startup(int target, int vec)
{
	uint32_t esr;

	i82489_writereg(LAPIC_ESR, 0);
	(void)i82489_readreg(LAPIC_ESR);

	i82489_icr_wait();
	i82489_writereg(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);
	i82489_writereg(LAPIC_ICRLO, vec | LAPIC_DLMODE_STARTUP |
	    LAPIC_LEVEL_ASSERT);
	i82489_icr_wait();

	if ((i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) != 0)
		return EBUSY;

	esr = i82489_readreg(LAPIC_ESR);
	if (esr != 0)
		aprint_debug("%s: ESR %08x\n", __func__, esr);

	return 0;
}

static int
i82489_ipi(int vec, int target, int dl)
{
	int result, s;

	s = splhigh();

	i82489_icr_wait();

	if ((target & LAPIC_DEST_MASK) == 0)
		i82489_writereg(LAPIC_ICRHI, target << LAPIC_ID_SHIFT);

	i82489_writereg(LAPIC_ICRLO,
	    (target & LAPIC_DEST_MASK) | vec | dl | LAPIC_LEVEL_ASSERT);

#ifdef DIAGNOSTIC
	i82489_icr_wait();
	result = (i82489_readreg(LAPIC_ICRLO) & LAPIC_DLSTAT_BUSY) ? EBUSY : 0;
#else
	/* Don't wait - if it doesn't go, we're in big trouble anyway. */
        result = 0;
#endif
	splx(s);

	return result;
}

static int
x2apic_ipi_init(int target)
{

	x2apic_write_icr(target, LAPIC_DLMODE_INIT | LAPIC_LEVEL_ASSERT);

	i8254_delay(10000);

	x2apic_write_icr(0,
	    LAPIC_DLMODE_INIT | LAPIC_TRIGGER_LEVEL | LAPIC_LEVEL_DEASSERT);

	return 0;
}

static int
x2apic_ipi_startup(int target, int vec)
{

	x2apic_write_icr(target, vec | LAPIC_DLMODE_STARTUP | LAPIC_LEVEL_ASSERT);

	return 0;
}

static int
x2apic_ipi(int vec, int target, int dl)
{
	uint32_t dest_id = 0;

	if ((target & LAPIC_DEST_MASK) == 0)
		dest_id = target;

	x2apic_write_icr(dest_id,
	    (target & LAPIC_DEST_MASK) | vec | dl | LAPIC_LEVEL_ASSERT);

	return 0;
}

int
x86_ipi_init(int target)
{
	if (x2apic_mode)
		return x2apic_ipi_init(target);
	return i82489_ipi_init(target);
}

int
x86_ipi_startup(int target, int vec)
{
	if (x2apic_mode)
		return x2apic_ipi_startup(target, vec);
	return i82489_ipi_startup(target, vec);
}

/*
 * Using 'pin numbers' as:
 * 0 - timer
 * 1 - unused
 * 2 - PCINT
 * 3 - LVINT0
 * 4 - LVINT1
 * 5 - LVERR
 */

static void
lapic_hwmask(struct pic *pic, int pin)
{
	int reg;
	uint32_t val;

	reg = LAPIC_LVTT + (pin << 4);
	val = lapic_readreg(reg);
	val |= LAPIC_LVT_MASKED;
	lapic_writereg(reg, val);
}

static void
lapic_hwunmask(struct pic *pic, int pin)
{
	int reg;
	uint32_t val;

	reg = LAPIC_LVTT + (pin << 4);
	val = lapic_readreg(reg);
	val &= ~LAPIC_LVT_MASKED;
	lapic_writereg(reg, val);
}

static void
lapic_setup(struct pic *pic, struct cpu_info *ci,
    int pin, int idtvec, int type)
{
}

void
lapic_dump(void)
{
	struct cpu_info *ci = curcpu();

	apic_format_redir(device_xname(ci->ci_dev), "timer", 0, 0,
	    lapic_readreg(LAPIC_LVTT));
	apic_format_redir(device_xname(ci->ci_dev), "pcint", 0, 0,
	    lapic_readreg(LAPIC_PCINT));
	apic_format_redir(device_xname(ci->ci_dev), "lint", 0, 0,
	    lapic_readreg(LAPIC_LVINT0));
	apic_format_redir(device_xname(ci->ci_dev), "lint", 1, 0,
	    lapic_readreg(LAPIC_LVINT1));
	apic_format_redir(device_xname(ci->ci_dev), "err", 0, 0,
	    lapic_readreg(LAPIC_LVERR));
}
