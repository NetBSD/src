/*	$NetBSD: autoconf.c,v 1.85.14.1 2007/05/22 17:26:58 matt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)autoconf.c	7.1 (Berkeley) 5/9/91
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 *
 * Lots of this is now in x86/x86/x86_autoconf.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.85.14.1 2007/05/22 17:26:58 matt Exp $");

#include "opt_compat_oldboot.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/gdt.h>
#include <machine/pcb.h>
#include <x86/x86/tsc.h>

#include "ioapic.h"
#include "lapic.h"

#if NIOAPIC > 0
#include <machine/i82093var.h>
#endif

#if NLAPIC > 0
#include <machine/i82489var.h>
#endif

#include "bios32.h"
#if NBIOS32 > 0
#include <machine/bios32.h>
#endif

#include "opt_pcibios.h"
#ifdef PCIBIOS
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <i386/pci/pcibios.h>
#endif

#include "opt_kvm86.h"
#ifdef KVM86
#include <machine/kvm86.h>
#endif

#include "opt_viapadlock.h"

/*
 * Determine i/o configuration for a machine.
 */
void
cpu_configure(void)
{

	startrtclock();

#if NBIOS32 > 0
	bios32_init();
#endif
#ifdef PCIBIOS
	pcibios_init();
#endif

#ifdef KVM86
	kvm86_init();
#endif

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

#ifdef INTRDEBUG
	intr_printconfig();
#endif

#if NIOAPIC > 0
	lapic_set_lvt();
	ioapic_enable();
#endif
	/* resync cr0 after FPU configuration */
	lwp0.l_addr->u_pcb.pcb_cr0 = rcr0();
#ifdef MULTIPROCESSOR
	/* propagate this to the idle pcb's. */
	cpu_init_idle_lwps();
#endif

#if defined(I586_CPU) || defined(I686_CPU)
	init_TSC_tc();
#endif

	spl0();
#if NLAPIC > 0
	lapic_tpr = 0;
#endif

#if defined(I686_CPU) && defined(VIA_PADLOCK)
	via_padlock_attach();
#endif /* defined(I686_CPU) && defined(VIA_PADLOCK) */
}
