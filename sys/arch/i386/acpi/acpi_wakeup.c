/*	$NetBSD: acpi_wakeup.c,v 1.3 2002/06/18 10:32:02 drochner Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_wakeup.c,v 1.3 2002/06/18 10:32:02 drochner Exp $");

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

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>
#include <machine/isa_machdep.h>

#include "acpi.h"

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>

#include "acpi_wakecode.h"


paddr_t	phys_wakeup = 0;

u_int32_t
acpi_md_get_npages_of_wakecode(void)
{
	return (atop(round_page(sizeof(wakecode))));
}

void
acpi_md_install_wakecode(paddr_t pa)
{
	bcopy(wakecode, (caddr_t)pa, sizeof(wakecode));
	phys_wakeup = pa;
	printf("acpi: wakecode is installed at 0x%lX, size=%u\n",
	       pa, sizeof(wakecode));
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
	u_long			ef;
	UINT32			ret;
	ACPI_STATUS		status;

	/* run the _PTS and _GTS methods */

	ACPI_MEMSET(&ArgList, 0, sizeof(ArgList));
	ArgList.Count = 1;
	ArgList.Pointer = &Arg;

	ACPI_MEMSET(&Arg, 0, sizeof(Arg));
	Arg.Type = ACPI_TYPE_INTEGER;
	Arg.Integer.Value = ACPI_STATE_S4;

	AcpiEvaluateObject (NULL, "\\_PTS", &ArgList, NULL);
	AcpiEvaluateObject (NULL, "\\_GTS", &ArgList, NULL);

	/* clear wake status */

	AcpiHwRegisterWrite(TRUE, ACPI_BITREG_WAKE_STATUS, 1);

	ef = read_eflags();
	disable_intr();

	AcpiHwDisableNonWakeupGpes();

	/* flush caches */

	ACPI_FLUSH_CPU_CACHE();

	/*
	 * write the value to command port and wait until we enter sleep state
	 */
	do {
			AcpiOsStall(1000000);
			AcpiOsWritePort(AcpiGbl_FADT->SmiCmd,
					AcpiGbl_FADT->S4BiosReq, 8);
			status = AcpiHwRegisterRead(TRUE,
						    ACPI_BITREG_WAKE_STATUS,
						    &ret);
			if (ACPI_FAILURE(status))
				break;
	} while (!ret);

	AcpiHwEnableNonWakeupGpes();

	write_eflags(ef);

	return (AE_OK);
}

static u_int16_t	r_ldt;
static u_int16_t	r_cs, r_ds, r_es, r_fs, r_gs, r_ss, r_tr;
static u_int32_t	r_eax, r_ebx, r_ecx, r_edx, r_ebp, r_esi, r_edi,
			r_efl, r_cr0, r_cr2, r_cr3, r_cr4, r_esp;
static u_int32_t	ret_addr;
static struct region_descriptor	r_idt, r_gdt;

/* XXX shut gcc up */
extern int		acpi_savecpu(void);
extern int		acpi_restorecpu(void);

static __inline void
clear_reg(void)
{
	r_ldt = 0;
	r_cs = r_ds = r_es = r_fs = r_gs = r_ss = r_tr = 0;
	r_eax = r_ebx = r_ecx = r_edx = r_ebp = r_esi = r_edi = 0;
	r_efl = r_cr0 = r_cr2 = r_cr3 = r_cr4 = r_esp = 0;
	memset(&r_idt, 0, sizeof(r_idt));
	memset(&r_gdt, 0, sizeof(r_gdt));
}

__asm__("
	.text
	.p2align 2, 0x90
	.type acpi_restorecpu, @function
acpi_restorecpu:
	.align 4
	movl	r_eax,%eax
	movl	r_ebx,%ebx
	movl	r_ecx,%ecx
	movl	r_edx,%edx
	movl	r_ebp,%ebp
	movl	r_esi,%esi
	movl	r_edi,%edi
	movl	r_esp,%esp

	pushl	r_efl
	popfl

	pushl	ret_addr
	xorl	%eax,%eax
	ret

	.text
	.p2align 2, 0x90
	.type acpi_savecpu, @function
acpi_savecpu:
	movw	%cs,r_cs
	movw	%ds,r_ds
	movw	%es,r_es
	movw	%fs,r_fs
	movw	%gs,r_gs
	movw	%ss,r_ss

	movl	%eax,r_eax
	movl	%ebx,r_ebx
	movl	%ecx,r_ecx
	movl	%edx,r_edx
	movl	%ebp,r_ebp
	movl	%esi,r_esi
	movl	%edi,r_edi

	movl	%cr0,%eax
	movl	%eax,r_cr0
	movl	%cr2,%eax
	movl	%eax,r_cr2
	movl	%cr3,%eax
	movl	%eax,r_cr3
	movl	%cr4,%eax
	movl	%eax,r_cr4

	pushfl
	popl	r_efl

	movl	%esp,r_esp

	sgdt	r_gdt
	sidt	r_idt
	sldt	r_ldt
	str	r_tr

	movl	(%esp),%eax
	movl	%eax,ret_addr
	movl	$1,%eax
	ret
");

#ifdef ACPI_PRINT_REG
static void
acpi_printcpu(void)
{

	printf("======== acpi_printcpu() debug dump ========\n");
	printf("gdt[%04x:%08x] idt[%04x:%08x] ldt[%04x] tr[%04x] efl[%08x]\n",
		r_gdt.rd_limit, r_gdt.rd_base, r_idt.rd_limit, r_idt.rd_base,
		r_ldt, r_tr, r_efl);
	printf("eax[%08x] ebx[%08x] ecx[%08x] edx[%08x]\n",
		r_eax, r_ebx, r_ecx, r_edx);
	printf("esi[%08x] edi[%08x] ebp[%08x] esp[%08x]\n",
		r_esi, r_edi, r_ebp, r_esp);
	printf("cr0[%08x] cr2[%08x] cr3[%08x] cr4[%08x]\n",
		r_cr0, r_cr2, r_cr3, r_cr4);
	printf("cs[%04x] ds[%04x] es[%04x] fs[%04x] gs[%04x] ss[%04x]\n",
		r_cs, r_ds, r_es, r_fs, r_gs, r_ss);
}
#endif

int
acpi_md_sleep(int state)
{
#define WAKECODE_FIXUP(offset, type, val) do	{		\
	type	*addr;						\
	addr = (type *)(phys_wakeup + offset);			\
	*addr = val;						\
} while (0)

#define WAKECODE_BCOPY(offset, type, val) do	{		\
	void	**addr;						\
	addr = (void **)(phys_wakeup + offset);			\
	bcopy(&(val), addr, sizeof(type));			\
} while (0)

	ACPI_STATUS			status;
	int				ret = 0;
	u_long				ef;
	struct region_descriptor	*p_gdt;

	if (!phys_wakeup) {
		printf("acpi: can't sleep since wakecode is not installed.\n");
		return (-1);
	}

	AcpiSetFirmwareWakingVector(phys_wakeup);

	clear_reg();
	ret_addr = 0;

	ef = read_eflags();
	disable_intr();
	if (acpi_savecpu()) {
		/* Execute Sleep */
		p_gdt = (struct region_descriptor *)(phys_wakeup+physical_gdt);
		p_gdt->rd_limit = r_gdt.rd_limit;
		p_gdt->rd_base = vtophys(r_gdt.rd_base);

		WAKECODE_FIXUP(previous_cr0, u_int32_t, r_cr0);
		WAKECODE_FIXUP(previous_cr2, u_int32_t, r_cr2);
		WAKECODE_FIXUP(previous_cr3, u_int32_t, r_cr3);
		WAKECODE_FIXUP(previous_cr4, u_int32_t, r_cr4);

		WAKECODE_FIXUP(previous_tr,  u_int16_t, r_tr);
		WAKECODE_BCOPY(previous_gdt, struct region_descriptor, r_gdt);
		WAKECODE_FIXUP(previous_ldt, u_int16_t, r_ldt);
		WAKECODE_BCOPY(previous_idt, struct region_descriptor, r_idt);

		WAKECODE_FIXUP(where_to_recover, void *, acpi_restorecpu);

		WAKECODE_FIXUP(previous_ds,  u_int16_t, r_ds);
		WAKECODE_FIXUP(previous_es,  u_int16_t, r_es);
		WAKECODE_FIXUP(previous_fs,  u_int16_t, r_fs);
		WAKECODE_FIXUP(previous_gs,  u_int16_t, r_gs);
		WAKECODE_FIXUP(previous_ss,  u_int16_t, r_ss);

#ifdef ACPI_PRINT_REG
		acpi_printcpu();
#endif

		ACPI_FLUSH_CPU_CACHE();

		if (state == ACPI_STATE_S4 && AcpiGbl_FACS->S4Bios_f) {
			status = enter_s4_with_bios();
		} else {
			status = AcpiEnterSleepState(state);
		}

		if (status != AE_OK) {
			printf("acpi: AcpiEnterSleepState failed - %s\n",
			       AcpiFormatException(status));
			ret = -1;
			goto out;
		}

		for (;;) ;
	} else {
		/* Execute Wakeup */

		isa_reinit_irq();
		initrtclock();
		inittodr(time.tv_sec);

#ifdef ACPI_PRINT_REG
		acpi_savecpu();
		acpi_printcpu();
#endif
	}

out:
	write_eflags(ef);
	AcpiUtReleaseMutex(ACPI_MTX_HARDWARE);

	return (ret);
#undef WAKECODE_FIXUP
#undef WAKECODE_BCOPY
}
