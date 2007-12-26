/*	$NetBSD: acpi_wakeup.c,v 1.1.4.2 2007/12/26 21:38:48 ad Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup.c,v 1.1.4.2 2007/12/26 21:38:48 ad Exp $");

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <machine/bus.h>
#include <sys/proc.h>
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

#include "acpi.h"

#include <dev/ic/i8253reg.h>
#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>
#include <machine/cpu.h>
#ifdef __i386__
#  include <machine/npx.h>
#else
#  include <machine/fpu.h>
#endif
#include <machine/mtrr.h>

#include <x86/cpuvar.h>

#include "acpi_wakecode.h"

/* Address is also hard-coded in acpi_wakecode.S */
static paddr_t acpi_wakeup_paddr = 3 * PAGE_SIZE;
static vaddr_t acpi_wakeup_vaddr;

static int acpi_md_node = CTL_EOL;
static int acpi_md_vbios_reset = 1;
static int acpi_md_beep_on_reset = 1;

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
	bcopy(&(val), addr, sizeof(type));			\
} while (0)

	paddr_t				tmp_pdir;

	tmp_pdir = pmap_init_tmp_pgtbl(acpi_wakeup_paddr);

	/* Execute Sleep */
	bcopy(wakecode, (void *)acpi_wakeup_vaddr, sizeof(wakecode));

	if (CPU_IS_PRIMARY(ci)) {
		WAKECODE_FIXUP(WAKEUP_vbios_reset, uint8_t, acpi_md_vbios_reset);
		WAKECODE_FIXUP(WAKEUP_beep_on_reset, uint8_t, acpi_md_beep_on_reset);
	} else {
		WAKECODE_FIXUP(WAKEUP_vbios_reset, uint8_t, 0);
		WAKECODE_FIXUP(WAKEUP_beep_on_reset, uint8_t, 0);
	}

#ifdef __i386__
	WAKECODE_FIXUP(WAKEUP_r_cr4, uint32_t, ci->ci_suspend_cr4);
#else
	WAKECODE_FIXUP(WAKEUP_msr_efer, uint32_t, ci->ci_suspend_msr_efer);
#endif

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

/*
 * S4 sleep using S4BIOS support, from FreeBSD.
 *
 * FreeBSD: src/sys/dev/acpica/acpica_support.c,v 1.4 2002/03/12 09:45:17 dfr Exp
 */

static ACPI_STATUS
enter_s4_with_bios(void)
{
	ACPI_OBJECT_LIST	ArgList;
	ACPI_OBJECT		Arg;
	UINT32			ret;
	ACPI_STATUS		status;

	/* run the _PTS and _GTS methods */

	ACPI_MEMSET(&ArgList, 0, sizeof(ArgList));
	ArgList.Count = 1;
	ArgList.Pointer = &Arg;

	ACPI_MEMSET(&Arg, 0, sizeof(Arg));
	Arg.Type = ACPI_TYPE_INTEGER;
	Arg.Integer.Value = ACPI_STATE_S4;

	AcpiEvaluateObject(NULL, "\\_PTS", &ArgList, NULL);
	AcpiEvaluateObject(NULL, "\\_GTS", &ArgList, NULL);

	/* clear wake status */

	AcpiSetRegister(ACPI_BITREG_WAKE_STATUS, 1);

	AcpiHwDisableAllGpes();
	AcpiHwEnableAllWakeupGpes();

	/* flush caches */

	ACPI_FLUSH_CPU_CACHE();

	/*
	 * write the value to command port and wait until we enter sleep state
	 */
	do {
		AcpiOsStall(1000000);
		AcpiOsWritePort(AcpiGbl_FADT.SmiCommand,
				AcpiGbl_FADT.S4BiosRequest, 8);
		status = AcpiGetRegister(ACPI_BITREG_WAKE_STATUS, &ret);
		if (ACPI_FAILURE(status))
			break;
	} while (!ret);

	AcpiHwDisableAllGpes();
	AcpiHwEnableAllRuntimeGpes();

	return (AE_OK);
}

void
acpi_md_sleep_enter(int state)
{
	ACPI_STATUS			status;
	struct cpu_info			*ci;

	ci = curcpu();

#ifdef MULTIPROCESSOR
	if (!CPU_IS_PRIMARY(ci)) {
		atomic_and_32(&ci->ci_flags, ~CPUF_RUNNING);
		atomic_and_32(&cpus_running, ~ci->ci_cpumask);

		ACPI_FLUSH_CPU_CACHE();

		for (;;)
			x86_hlt();
	}
#endif

	acpi_md_sleep_patch(ci);

	ACPI_FLUSH_CPU_CACHE();

	if (state == ACPI_STATE_S4) {
		ACPI_TABLE_FACS *facs;
		status = AcpiGetTable(ACPI_SIG_FACS, 0, (ACPI_TABLE_HEADER **)&facs);
		if (ACPI_FAILURE(status)) {
			printf("acpi: S4BIOS not supported: cannot load FACS\n");
			return;
		}
		if (facs == NULL ||
		    (facs->Flags & ACPI_FACS_S4_BIOS_PRESENT) == 0) {
			printf("acpi: S4BIOS not supported: not present");
			return;
		}
		status = enter_s4_with_bios();
	} else {
		status = AcpiEnterSleepState(state);
	}

	if (ACPI_FAILURE(status)) {
		printf("acpi: AcpiEnterSleepState failed: %s\n",
		       AcpiFormatException(status));
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
#ifdef __i386__
	npxinit(ci);
#else
	cpu_init_msrs(ci, false);
	fpuinit(ci);
#endif
#if NLAPIC > 0
	lapic_enable();
	lapic_set_lvt();
	lapic_initclocks();
#endif

	atomic_or_32(&ci->ci_flags, CPUF_RUNNING);
	atomic_or_32(&cpus_running, ci->ci_cpumask);

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
#endif

	KASSERT(acpi_wakeup_paddr != 0);
	KASSERT(sizeof(wakecode) <= PAGE_SIZE);

	if (!CPU_IS_PRIMARY(curcpu())) {
		printf("acpi0: WARNING: ignoring sleep from secondary CPU\n");
		return -1;
	}

	AcpiSetFirmwareWakingVector(acpi_wakeup_paddr);

	s = splipi();
#ifdef __i386__
	npxsave_cpu(curcpu(), 1);
#else
	fpusave_cpu(curcpu(), 1);
#endif
	splx(s);

	s = splhigh();
	x86_disable_intr();

#ifdef MULTIPROCESSOR
	/* Save and suspend Application Processors */
	x86_broadcast_ipi(X86_IPI_ACPI_CPU_SLEEP);
	while (cpus_running != curcpu()->ci_cpumask)
		delay(1); 
#endif

	if (acpi_md_sleep_prepare(state))
		goto out;

	/* Execute Wakeup */
#ifdef __i386__
	npxinit(&cpu_info_primary);
#else
	cpu_init_msrs(&cpu_info_primary, false);
	fpuinit(&cpu_info_primary);
#endif
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

	AcpiClearEvent(ACPI_EVENT_POWER_BUTTON);

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
	    VM_PROT_READ | VM_PROT_WRITE);
	pmap_update(pmap_kernel());
}

SYSCTL_SETUP(sysctl_md_acpi_setup, "acpi x86 sysctl setup")
{
	const struct sysctlnode *node;
	const struct sysctlnode *ssnode;

	if (sysctl_createv(NULL, 0, NULL, &node, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL, NULL, 0, NULL, 0, CTL_MACHDEP,
	    CTL_EOL) != 0)
		return;
	if (sysctl_createv(NULL, 0, &node, &ssnode, CTLFLAG_READWRITE,
	    CTLTYPE_INT, "acpi_vbios_reset", NULL, sysctl_md_acpi_vbios_reset,
	    0, NULL, 0, CTL_CREATE, CTL_EOL) != 0)
		return;
	if (sysctl_createv(NULL, 0, &node, &ssnode, CTLFLAG_READWRITE,
	    CTLTYPE_INT, "acpi_beep_on_reset", NULL, sysctl_md_acpi_beep_on_reset,
	    0, NULL, 0, CTL_CREATE, CTL_EOL) != 0)
		return;

	acpi_md_node = node->sysctl_num;
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

	if (t < 0 || t > 1)
		return EINVAL;

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
