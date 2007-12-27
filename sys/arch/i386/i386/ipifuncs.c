/*	$NetBSD: ipifuncs.c,v 1.20.2.3 2007/12/27 00:43:06 mjf Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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


/*
 * Interprocessor interrupt handlers.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: ipifuncs.c,v 1.20.2.3 2007/12/27 00:43:06 mjf Exp $");

#include "opt_ddb.h"
#include "opt_mtrr.h"
#include "npx.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/atomic.h>

#include <uvm/uvm_extern.h>

#include <x86/cpu_msr.h>
#include <machine/intr.h>
#include <machine/cpuvar.h>
#include <machine/i82093var.h>
#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#include <machine/mtrr.h>
#include <machine/gdt.h>

#include <ddb/db_output.h>

#include "acpi.h"

void i386_ipi_halt(struct cpu_info *);

#if NNPX > 0
void i386_ipi_synch_fpu(struct cpu_info *);
void i386_ipi_flush_fpu(struct cpu_info *);
#else
#define i386_ipi_synch_fpu 0
#define i386_ipi_flush_fpu 0
#endif

#ifdef MTRR
void i386_reload_mtrr(struct cpu_info *);
#else
#define i386_reload_mtrr NULL
#endif

#if NACPI > 0
void acpi_cpu_sleep(struct cpu_info *);
#else
#define	acpi_cpu_sleep NULL
#endif

void (*ipifunc[X86_NIPI])(struct cpu_info *) =
{
	i386_ipi_halt,
	tsc_calibrate_cpu,	/* keep cycle counters synchronized */
	i386_ipi_flush_fpu,
	i386_ipi_synch_fpu,
	i386_reload_mtrr,
	gdt_reload_cpu,
	msr_write_ipi,
	acpi_cpu_sleep,
};

void
i386_ipi_halt(struct cpu_info *ci)
{
	x86_disable_intr();
	atomic_and_32(&ci->ci_flags, ~CPUF_RUNNING);

	for(;;) {
		x86_hlt();
	}
}

#if NNPX > 0
void
i386_ipi_flush_fpu(struct cpu_info *ci)
{
	npxsave_cpu(ci, 0);
}

void
i386_ipi_synch_fpu(struct cpu_info *ci)
{
	npxsave_cpu(ci, 1);
}
#endif

#ifdef MTRR

/*
 * mtrr_reload_cpu() is a macro in mtrr.h which picks the appropriate
 * function to use..
 */

void
i386_reload_mtrr(struct cpu_info *ci)
{
	if (mtrr_funcs != NULL)
		mtrr_reload_cpu(ci);
}
#endif
