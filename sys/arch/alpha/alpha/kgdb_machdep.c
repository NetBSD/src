/* $NetBSD: kgdb_machdep.c,v 1.1 2001/04/19 02:56:34 thorpej Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Machine-dependent functions for remote KGDB.
 */

#include "opt_kgdb_machdep.h"

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: kgdb_machdep.c,v 1.1 2001/04/19 02:56:34 thorpej Exp $");

#include "com.h"

#include <sys/param.h>
#include <sys/kgdb.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <machine/bus.h>
#include <machine/db_machdep.h>

#include <uvm/uvm_extern.h>

#if NCOM > 0
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#endif /* NCOM > 0 */

#ifndef KGDB_DEVNAME
#error Must define KGDB_DEVNAME
#endif
const char kgdb_devname[] = KGDB_DEVNAME;

#ifndef KGDB_DEVADDR
#error Must define KGDB_DEVADDR
#endif
int kgdb_devaddr = KGDB_DEVADDR;

#ifndef KGDB_DEVRATE
#define	KGDB_DEVRATE	TTYDEF_SPEED
#endif
int kgdb_devrate = KGDB_DEVRATE;

#ifndef KGDB_DEVMODE
			/* 8N1 */
#define	KGDB_DEVMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8)
#endif
int kgdb_devmode = KGDB_DEVMODE;

/*
 * alpha_kgdb_init:
 *
 *	Initialize KGDB -- connect to the device.
 */
void
alpha_kgdb_init(const char **valid_devs, struct alpha_bus_space *bst)
{
	int i;

	for (i = 0; valid_devs[i] != NULL; i++) {
		if (strcmp(kgdb_devname, valid_devs[i]) == 0)
			break;
	}
	if (valid_devs[i] == NULL) {
		printf("%s is not a valid KGDB device for this platform\n",
		    kgdb_devname);
		return;
	}

#if NCOM > 0
	if (strcmp(kgdb_devname, "com") == 0) {
		com_kgdb_attach(bst, kgdb_devaddr, kgdb_devrate, COM_FREQ,
		    kgdb_devmode);
		return;
	}
#endif /* NCOM > 0 */

	printf("The %s driver is not configured into the kernel; "
	    "KGDB not attached\n", kgdb_devname);
}

/*
 * kgdb_acc:
 *
 *	Determine if the mapping at va..(va+len) is valid.
 */
int
kgdb_acc(vaddr_t va, size_t len)
{
	vaddr_t last_va;
	pt_entry_t *pte;

	va = trunc_page(va);
	last_va = round_page(va + len);

	do  {
		if (va < VM_MIN_KERNEL_ADDRESS)
			return (0);
		pte = pmap_l3pte(pmap_kernel(), va, NULL);
		if (pte == NULL || pmap_pte_v(pte) == 0)
			return (0);
		va += PAGE_SIZE;
	} while (va < last_va);

	return (1);
}

/*
 * kgdb_signal:
 *
 *	Translate a trap number into a Unix-compatible signal number.
 *	(GDB only understands Unix signal numbers.)
 */
int
kgdb_signal(int type)
{

	switch (type) {
	case ALPHA_KENTRY_UNA:
		return (SIGBUS);

	case ALPHA_KENTRY_ARITH:
		return (SIGFPE);

	case ALPHA_KENTRY_IF:
		return (SIGILL);

	case ALPHA_KENTRY_MM:
		return (SIGSEGV);

	default:
		return (SIGEMT);
	}
}

/*
 * kgdb_getregs:
 *
 *	Translate the kernel debugger register format into
 *	the GDB register format.
 */
void
kgdb_getregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{

	memset(gdb_regs, 0, sizeof(kgdb_reg_t) * KGDB_NUMREGS);

	gdb_regs[KGDB_REG_V0 ] = regs->tf_regs[FRAME_V0];
	gdb_regs[KGDB_REG_T0 ] = regs->tf_regs[FRAME_T0];
	gdb_regs[KGDB_REG_T1 ] = regs->tf_regs[FRAME_T1];
	gdb_regs[KGDB_REG_T2 ] = regs->tf_regs[FRAME_T2];
	gdb_regs[KGDB_REG_T3 ] = regs->tf_regs[FRAME_T3];
	gdb_regs[KGDB_REG_T4 ] = regs->tf_regs[FRAME_T4];
	gdb_regs[KGDB_REG_T5 ] = regs->tf_regs[FRAME_T5];
	gdb_regs[KGDB_REG_T6 ] = regs->tf_regs[FRAME_T6];
	gdb_regs[KGDB_REG_T7 ] = regs->tf_regs[FRAME_T7];
	gdb_regs[KGDB_REG_S0 ] = regs->tf_regs[FRAME_S0];
	gdb_regs[KGDB_REG_S1 ] = regs->tf_regs[FRAME_S1];
	gdb_regs[KGDB_REG_S2 ] = regs->tf_regs[FRAME_S2];
	gdb_regs[KGDB_REG_S3 ] = regs->tf_regs[FRAME_S3];
	gdb_regs[KGDB_REG_S4 ] = regs->tf_regs[FRAME_S4];
	gdb_regs[KGDB_REG_S5 ] = regs->tf_regs[FRAME_S5];
	gdb_regs[KGDB_REG_S6 ] = regs->tf_regs[FRAME_S6];
	gdb_regs[KGDB_REG_A0 ] = regs->tf_regs[FRAME_A0];
	gdb_regs[KGDB_REG_A1 ] = regs->tf_regs[FRAME_A1];
	gdb_regs[KGDB_REG_A2 ] = regs->tf_regs[FRAME_A2];
	gdb_regs[KGDB_REG_A3 ] = regs->tf_regs[FRAME_A3];
	gdb_regs[KGDB_REG_A4 ] = regs->tf_regs[FRAME_A4];
	gdb_regs[KGDB_REG_A5 ] = regs->tf_regs[FRAME_A5];
	gdb_regs[KGDB_REG_T8 ] = regs->tf_regs[FRAME_T8];
	gdb_regs[KGDB_REG_T9 ] = regs->tf_regs[FRAME_T9];
	gdb_regs[KGDB_REG_T10] = regs->tf_regs[FRAME_T10];
	gdb_regs[KGDB_REG_T11] = regs->tf_regs[FRAME_T11];
	gdb_regs[KGDB_REG_RA ] = regs->tf_regs[FRAME_RA];
	gdb_regs[KGDB_REG_T12] = regs->tf_regs[FRAME_T12];
	gdb_regs[KGDB_REG_AT ] = regs->tf_regs[FRAME_AT];
	gdb_regs[KGDB_REG_GP ] = regs->tf_regs[FRAME_GP];
	gdb_regs[KGDB_REG_SP ] = regs->tf_regs[FRAME_SP];
	gdb_regs[KGDB_REG_PC ] = regs->tf_regs[FRAME_PC];
}

/*
 * kgdb_setregs:
 *
 *	Translate the GDB register format into the kernel
 *	debugger register format.
 */
void
kgdb_setregs(db_regs_t *regs, kgdb_reg_t *gdb_regs)
{

	regs->tf_regs[FRAME_V0 ] = gdb_regs[KGDB_REG_V0];
	regs->tf_regs[FRAME_T0 ] = gdb_regs[KGDB_REG_T0];
	regs->tf_regs[FRAME_T1 ] = gdb_regs[KGDB_REG_T1];
	regs->tf_regs[FRAME_T2 ] = gdb_regs[KGDB_REG_T2];
	regs->tf_regs[FRAME_T3 ] = gdb_regs[KGDB_REG_T3];
	regs->tf_regs[FRAME_T4 ] = gdb_regs[KGDB_REG_T4];
	regs->tf_regs[FRAME_T5 ] = gdb_regs[KGDB_REG_T5];
	regs->tf_regs[FRAME_T6 ] = gdb_regs[KGDB_REG_T6];
	regs->tf_regs[FRAME_T7 ] = gdb_regs[KGDB_REG_T7];
	regs->tf_regs[FRAME_S0 ] = gdb_regs[KGDB_REG_S0];
	regs->tf_regs[FRAME_S1 ] = gdb_regs[KGDB_REG_S1];
	regs->tf_regs[FRAME_S2 ] = gdb_regs[KGDB_REG_S2];
	regs->tf_regs[FRAME_S3 ] = gdb_regs[KGDB_REG_S3];
	regs->tf_regs[FRAME_S4 ] = gdb_regs[KGDB_REG_S4];
	regs->tf_regs[FRAME_S5 ] = gdb_regs[KGDB_REG_S5];
	regs->tf_regs[FRAME_S6 ] = gdb_regs[KGDB_REG_S6];
	regs->tf_regs[FRAME_A0 ] = gdb_regs[KGDB_REG_A0];
	regs->tf_regs[FRAME_A1 ] = gdb_regs[KGDB_REG_A1];
	regs->tf_regs[FRAME_A2 ] = gdb_regs[KGDB_REG_A2];
	regs->tf_regs[FRAME_A3 ] = gdb_regs[KGDB_REG_A3];
	regs->tf_regs[FRAME_A4 ] = gdb_regs[KGDB_REG_A4];
	regs->tf_regs[FRAME_A5 ] = gdb_regs[KGDB_REG_A5];
	regs->tf_regs[FRAME_T8 ] = gdb_regs[KGDB_REG_T8];
	regs->tf_regs[FRAME_T9 ] = gdb_regs[KGDB_REG_T9];
	regs->tf_regs[FRAME_T10] = gdb_regs[KGDB_REG_T10];
	regs->tf_regs[FRAME_T11] = gdb_regs[KGDB_REG_T11];
	regs->tf_regs[FRAME_RA ] = gdb_regs[KGDB_REG_RA];
	regs->tf_regs[FRAME_T12] = gdb_regs[KGDB_REG_T12];
	regs->tf_regs[FRAME_AT ] = gdb_regs[KGDB_REG_AT];
	regs->tf_regs[FRAME_GP ] = gdb_regs[KGDB_REG_GP];
	regs->tf_regs[FRAME_SP ] = gdb_regs[KGDB_REG_SP];
	regs->tf_regs[FRAME_PC ] = gdb_regs[KGDB_REG_PC];
}

/*
 * kgdb_connect:
 *
 *	Trap into KGDB and wait for the remote debugger to
 *	connect.  Display a message on the console indicating
 *	why nothing else is happening.
 */
void
kgdb_connect(int verbose)
{

	if (kgdb_dev < 0)
		return;

	if (verbose)
		printf("kgdb waiting...");

	__asm __volatile("call_pal 0x81");	/* bugchk */

	if (verbose)
		printf("connected.\n");

	kgdb_debug_panic = 1;
}

/*
 * kgdb_panic:
 *
 *	Decide what to do on panic.  (This is called by panic(),
 *	like Debugger().)
 */
void
kgdb_panic(void)
{
	if (kgdb_dev >= 0 && kgdb_debug_panic) {
		printf("entering kgdb\n");
		kgdb_connect(kgdb_active == 0);
	}
}
