/*	$NetBSD: acpi_wakeup.c,v 1.38.6.2 2016/10/05 20:55:36 skrll Exp $	*/

/*-
 * Copyright (c) 2002, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup.c,v 1.38.6.2 2016/10/05 20:55:36 skrll Exp $");

/*-
 * Copyright (c) 2001 Takanori Watanabe <takawata@jp.freebsd.org>
 * Copyright (c) 2001 Mitsuru IWASAKI <iwasaki@jp.freebsd.org>
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
 *
 *      FreeBSD: src/sys/i386/acpica/acpi_wakeup.c,v 1.9 2002/01/10 03:26:46 wes Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup.c,v 1.38.6.2 2016/10/05 20:55:36 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kcpuset.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#ifdef __i386__
#include "opt_mtrr.h"
#endif
#include "ioapic.h"
#include "lapic.h"

#if NLAPIC > 0
#include <machine/i82489var.h>
#endif
#if NIOAPIC > 0
#include <machine/i82093var.h>
#endif
#include <machine/i8259.h>

#include "acpica.h"

#include <dev/ic/i8253reg.h>
#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>
#include <machine/cpu.h>
#include <machine/mtrr.h>

#include <x86/cpuvar.h>
#include <x86/x86/tsc.h>
#include <x86/fpu.h>

#include "opt_vga.h"

#include "acpi_wakecode.h"

/* Address is also hard-coded in acpi_wakecode.S */
static paddr_t acpi_wakeup_paddr = 3 * PAGE_SIZE;
static vaddr_t acpi_wakeup_vaddr;

int acpi_md_vbios_reset = 0; /* Referenced by dev/pci/vga_pci.c */
int acpi_md_vesa_modenum = 0; /* Referenced by arch/x86/x86/genfb_machdep.c */
static int acpi_md_beep_on_reset = 0;

static int	acpi_md_s4bios(void);
static int	sysctl_md_acpi_vbios_reset(SYSCTLFN_ARGS);
static int	sysctl_md_acpi_beep_on_reset(SYSCTLFN_ARGS);

/* Implemented in acpi_wakeup_low.S. */
int	acpi_md_sleep_prepare(int);
int	acpi_md_sleep_exit(int);

/* Referenced by acpi_wakeup_low.S. */
void	acpi_md_sleep_enter(int);

#ifdef MULTIPROCESSOR
/* Referenced in ipifuncs.c. */
void	acpi_cpu_sleep(struct cpu_info *);
#endif

static void
acpi_md_sleep_patch(struct cpu_info *ci)
{
#define WAKECODE_FIXUP(offset, type, val) do	{		\
	type	*addr;						\
	addr = (type *)(acpi_wakeup_vaddr + offset);		\
	*addr = val;						\
} while (0)

#define WAKECODE_BCOPY(offset, type, val) do	{		\
	void	**addr;						\
	addr = (void **)(acpi_wakeup_vaddr + offset);		\
	memcpy(addr, &(val), sizeof(type));			\
} while (0)

	paddr_t				tmp_pdir;

	tmp_pdir = pmap_init_tmp_pgtbl(acpi_wakeup_paddr);

	/* Execute Sleep */
	memcpy((void *)acpi_wakeup_vaddr, wakecode, sizeof(wakecode));

	if (CPU_IS_PRIMARY(ci)) {
		WAKECODE_FIXUP(WAKEUP_vesa_modenum, uint16_t, acpi_md_vesa_modenum);
		WAKECODE_FIXUP(WAKEUP_vbios_reset, uint8_t, acpi_md_vbios_reset);
		WAKECODE_FIXUP(WAKEUP_beep_on_reset, uint8_t, acpi_md_beep_on_reset);
	} else {
		WAKECODE_FIXUP(WAKEUP_vesa_modenum, uint16_t, 0);
		WAKECODE_FIXUP(WAKEUP_vbios_reset, uint8_t, 0);
		WAKECODE_FIXUP(WAKEUP_beep_on_reset, uint8_t, 0);
	}

#ifdef __i386__
	WAKECODE_FIXUP(WAKEUP_r_cr4, uint32_t, ci->ci_suspend_cr4);
#endif
	WAKECODE_FIXUP(WAKEUP_efer, uint32_t, ci->ci_suspend_efer);
	WAKECODE_FIXUP(WAKEUP_curcpu, void *, ci);
#ifdef __i386__
	WAKECODE_FIXUP(WAKEUP_r_cr3, uint32_t, tmp_pdir);
#else
	WAKECODE_FIXUP(WAKEUP_r_cr3, uint64_t, tmp_pdir);
#endif
	WAKECODE_FIXUP(WAKEUP_restorecpu, void *, acpi_md_sleep_exit);
#undef WAKECODE_FIXUP
#undef WAKECODE_BCOPY
}

static int
acpi_md_s4bios(void)
{
	ACPI_TABLE_FACS *facs;
	ACPI_STATUS rv;

	rv = AcpiGetTable(ACPI_SIG_FACS, 0, (ACPI_TABLE_HEADER **)&facs);

	if (ACPI_FAILURE(rv) || facs == NULL)
		return 0;

	if ((facs->Flags & ACPI_FACS_S4_BIOS_PRESENT) == 0)
		return 0;

	return 1;
}

void
acpi_md_sleep_enter(int state)
{
	static int s4bios = -1;
	struct cpu_info *ci;
	ACPI_STATUS rv;

	ci = curcpu();

#ifdef MULTIPROCESSOR
	if (!CPU_IS_PRIMARY(ci)) {
		atomic_and_32(&ci->ci_flags, ~CPUF_RUNNING);
		kcpuset_atomic_clear(kcpuset_running, cpu_index(ci));

		ACPI_FLUSH_CPU_CACHE();

		for (;;)
			x86_hlt();
	}
#endif

	acpi_md_sleep_patch(ci);

	ACPI_FLUSH_CPU_CACHE();

	switch (state) {

	case ACPI_STATE_S4:

		if (s4bios < 0)
			s4bios = acpi_md_s4bios();

		if (s4bios == 0) {
			aprint_error("acpi0: S4 not supported\n");
			return;
		}

		rv = AcpiEnterSleepStateS4bios();
		break;

	default:
		rv = AcpiEnterSleepState(state);
		break;
	}

	if (ACPI_FAILURE(rv)) {
		aprint_error("acpi0: failed to enter S%d\n", state);
		return;
	}

	for (;;)
		x86_hlt();
}

#ifdef MULTIPROCESSOR
void
acpi_cpu_sleep(struct cpu_info *ci)
{
	KASSERT(!CPU_IS_PRIMARY(ci));
	KASSERT(ci == curcpu());

	x86_disable_intr();

	if (acpi_md_sleep_prepare(-1))
		return;

	/* Execute Wakeup */
	cpu_init_msrs(ci, false);
	fpuinit(ci);

#if NLAPIC > 0
	lapic_enable();
	lapic_set_lvt();
	lapic_initclocks();
#endif

	atomic_or_32(&ci->ci_flags, CPUF_RUNNING);
	kcpuset_atomic_set(kcpuset_running, cpu_index(ci));
	tsc_sync_ap(ci);

	x86_enable_intr();
}
#endif

int
acpi_md_sleep(int state)
{
	int s, ret = 0;
#ifdef MULTIPROCESSOR
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	cpuid_t cid;
#endif

	KASSERT(acpi_wakeup_paddr != 0);
	KASSERT(sizeof(wakecode) <= PAGE_SIZE);

	if (!CPU_IS_PRIMARY(curcpu())) {
		printf("acpi0: WARNING: ignoring sleep from secondary CPU\n");
		return -1;
	}

	AcpiSetFirmwareWakingVector(acpi_wakeup_paddr, acpi_wakeup_paddr);

	s = splhigh();
	fpusave_cpu(true);
	x86_disable_intr();

#ifdef MULTIPROCESSOR
	/* Save and suspend Application Processors. */
	x86_broadcast_ipi(X86_IPI_ACPI_CPU_SLEEP);
	cid = cpu_index(curcpu());
	while (kcpuset_isotherset(kcpuset_running, cid)) {
		delay(1);
	}
#endif

	if (acpi_md_sleep_prepare(state))
		goto out;

	/* Execute Wakeup */
#ifndef __i386__
	cpu_init_msrs(&cpu_info_primary, false);
#endif
	fpuinit(&cpu_info_primary);
	i8259_reinit();
#if NLAPIC > 0
	lapic_enable();
	lapic_set_lvt();
	lapic_initclocks();
#endif
#if NIOAPIC > 0
	ioapic_reenable();
#endif

	initrtclock(TIMER_FREQ);
	inittodr(time_second);

	/*
	 * The BIOS should always re-enable the SCI upon
	 * resume from the S3 state. The following is a
	 * workaround for systems that fail to do this.
	 */
	(void)AcpiWriteBitRegister(ACPI_BITREG_SCI_ENABLE, 1);

	/*
	 * Clear fixed events (see e.g. ACPI 3.0, p. 62).
	 * Also prevent GPEs from misfiring by disabling
	 * all GPEs before interrupts are enabled. The
	 * AcpiLeaveSleepState() function will enable
	 * and handle the general purpose events later.
	 */
	(void)AcpiClearEvent(ACPI_EVENT_PMTIMER);
	(void)AcpiClearEvent(ACPI_EVENT_GLOBAL);
	(void)AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);
	(void)AcpiClearEvent(ACPI_EVENT_SLEEP_BUTTON);
	(void)AcpiClearEvent(ACPI_EVENT_RTC);
	(void)AcpiHwDisableAllGpes();

	acpi_pci_link_resume();

out:

#ifdef MULTIPROCESSOR
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (CPU_IS_PRIMARY(ci))
			continue;
		acpi_md_sleep_patch(ci);

		CPU_STARTUP(ci, acpi_wakeup_paddr);
		CPU_START_CLEANUP(ci);

		while ((ci->ci_flags & CPUF_RUNNING) == 0)
			x86_pause();

		tsc_sync_bp(ci);
	}
#endif

	x86_enable_intr();
	splx(s);

#ifdef MTRR
	if (mtrr_funcs != NULL)
		mtrr_commit();
#endif

	return (ret);
}

void
acpi_md_sleep_init(void)
{
	/* Map ACPI wakecode */
	acpi_wakeup_vaddr = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY);
	if (acpi_wakeup_vaddr == 0)
		panic("acpi: can't allocate address for wakecode.\n");

	pmap_kenter_pa(acpi_wakeup_vaddr, acpi_wakeup_paddr,
	    VM_PROT_READ | VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
}

SYSCTL_SETUP(sysctl_md_acpi_setup, "ACPI x86 sysctl setup")
{
	const struct sysctlnode *rnode;
	int err;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "acpi", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err != 0)
		return;

	err = sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE,
	    "sleep", SYSCTL_DESCR("ACPI sleep"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (err != 0)
		return;

	(void)sysctl_createv(NULL, 0, &rnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "beep",
	    NULL, sysctl_md_acpi_beep_on_reset,
	    0, NULL, 0, CTL_CREATE, CTL_EOL);

	(void)sysctl_createv(NULL, 0, &rnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "vbios",
	    NULL, sysctl_md_acpi_vbios_reset,
	    0, NULL, 0, CTL_CREATE, CTL_EOL);
}

static int
sysctl_md_acpi_vbios_reset(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = acpi_md_vbios_reset;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 2)
		return EINVAL;

#ifndef VGA_POST
	if (t == 2) {
		aprint_error("WARNING: hw.acpi.sleep.vbios=2 "
		    "unsupported (no option VGA_POST in kernel config)\n");
		return EINVAL;
	}
#endif

	acpi_md_vbios_reset = t;

	return 0;
}

static int
sysctl_md_acpi_beep_on_reset(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = acpi_md_beep_on_reset;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 1)
		return EINVAL;

	acpi_md_beep_on_reset = t;

	return 0;
}
