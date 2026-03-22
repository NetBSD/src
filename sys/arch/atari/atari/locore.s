/*	$NetBSD: locore.s,v 1.138 2026/03/22 15:06:00 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1980, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: locore.s 1.58 91/04/22$
 *
 *	@(#)locore.s	7.11 (Berkeley) 5/9/91
 */

/*
 *
 * Original (hp300) Author: unknown, maybe Mike Hibler?
 * Amiga author: Markus Wild
 * Atari Modifications: Leo Weppelman
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#include "opt_ddb.h"
#include "opt_fpsp.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_mbtype.h"
#include "opt_m68k_arch.h"

#include "kbd.h"
#include "ncrscsi.h"
#include "zs.h"

#include "assym.h"
#include <machine/asm.h>

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */
	.text
	GLOBAL(kernel_text)

/*
 * Clear & skip page zero, it will not be mapped
 */
	.fill	PAGE_SIZE/4,4,0

#include <atari/atari/vectors.s>

	.text
	.even
/*
 * Do a dump.
 * Called by auto-restart.
 */
ENTRY_NOPROFILE(doadump)
	jbsr	_C_LABEL(dumpsys)
	jbsr	_C_LABEL(doboot)
	/*NOTREACHED*/

	/*
	 * This is where the default vectors end-up!
	 * At the time of the 'machine-type' probes, it seems necessary
	 * that the 'nofault' test is done first. Because the MMU is not
	 * yet setup at this point, the real fault handlers sometimes
	 * misinterpret the cause of the fault.
	 */
ENTRY_NOPROFILE(buserr_early)
ENTRY_NOPROFILE(addrerr_early)
	tstl	_C_LABEL(nofault)	| device probe?
	jeq	1f			| no, halt...
	movl	_C_LABEL(nofault),%sp@-	| yes,
	jbsr	_C_LABEL(longjmp)	|  longjmp(nofault)
	/* NOTREACHED */
1:
	jra	_C_LABEL(badtrap)	| only catch probes!

/*
 * Other exceptions only cause four and six word stack frame and require
 * no post-trap stack adjustment.
 */

ENTRY_NOPROFILE(intr_glue)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(intr_dispatch)	|  handle interrupt
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)

ENTRY_NOPROFILE(lev2intr)
	rte				|  HBL, can't be turned off on Falcon!

ENTRY_NOPROFILE(lev4intr)		|  VBL interrupt
#ifdef FALCON_VIDEO
	tstl	_C_LABEL(falcon_needs_vbl)
	jne	1f			|  Yes, go service a VBL-request
	rte				|  Nothing to do.
1:
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG
	jbsr	_C_LABEL(falcon_display_switch)
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(intr_depth)
#endif /* FALCON_VIDEO */
	rte

ENTRY_NOPROFILE(lev5intr)
ENTRY_NOPROFILE(lev6intr)

#ifdef _MILANHW_
	/* XXX
	 * Need to find better places to define these (Leo)
	 */
#define	PLX_PCICR	0x4204
#define	PLX_CNTRL	0x42ec
#define	PLX_DMCFGA	0x42ac
	addql	#1,_C_LABEL(intr_depth)
	moveml	%d0-%d2/%a0-%a1,%sp@-
	movw	%sp@(20),%sp@-		|  push previous SR value
	clrw	%sp@-			|	padded to longword
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movew	#0xffff,%a0@(PLX_PCICR)	| clear PCI_SR error bits
	movel	%a0@(PLX_CNTRL),%d0	| Change PCI command code from
	andw	#0xf0ff,%d0
	movw	%sr,%d2			| Block interrupts for now
	oriw	#0x0700,%sr
	movl	%d0,%a0@(PLX_CNTRL)
	movq	#0,%d1			| clear upper bits
					| Read any (uncached!) PCI address
					|  to fetch vector number
	movl	_C_LABEL(pci_mem_uncached),%a1
	movb	%a1@,%d1
	orw	#0x0600,%d0		| Change PCI command code back
	movel	%d0,%a0@(PLX_CNTRL)	|  to Read Cycle
	movew	%d2,%sr			| Re-enable interrupts
	movel	%d1,%sp@-		| Call handler
	jbsr	_C_LABEL(milan_isa_intr)
	addql	#8,%sp
	moveml  %sp@+,%d0-%d2/%a0-%a1
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)

/*
 * Support functions for reading and writing the Milan PCI config space.
 * Of interest:
 *   - We need exclusive access to the PLX9080 during config space
 *     access, hence the splhigh().
 *   - The 'confread' function shortcircuits the NMI to make probes to
 *     unexplored pci-config space possible.
 */
ENTRY(milan_pci_confread)
	movl	%sp@(4),%d0		| get tag and regno
	bset	#31,%d0			| add config space flag
	andl	#~3,%d0			| access type 0
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movw	%sr,%d1			| goto splhigh
	oriw	#0x0700,%sr
	movb	#1,_ASM_LABEL(plx_nonmi)| no NMI interrupts please!
	movl	%d0,%a0@(PLX_DMCFGA)	| write tag to the config register
	movl	_C_LABEL(pci_io_addr),%a1
	movl	%a1@,%d0		| fetch value
	movl	#0,%a0@(PLX_DMCFGA)	| back to normal PCI access

					| Make sure the C-function can peek
	movw	%a0@(PLX_PCICR),_C_LABEL(plx_status) | at the access results.

	movw	#0xf900,%a0@(PLX_PCICR)	| Clear potential error bits
	movb	#0, _ASM_LABEL(plx_nonmi)
	movw	%d1,%sr			| splx
	rts

ENTRY(milan_pci_confwrite)
	movl	%sp@(4),%d0		| get tag and regno
	bset	#31,%d0			| add config space flag
	andl	#~3,%d0			| access type 0
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movw	%sr,%d1			| goto splhigh
	oriw	#0x0700,%sr
	movl	%d0,%a0@(PLX_DMCFGA)	| write tag to the config register
	movl	_C_LABEL(pci_io_addr),%a1
	movl	%sp@(8),%a1@		| write value
	movl	#0,%a0@(PLX_DMCFGA)	| back to normal PCI access
	movw	%d1,%sr			| splx
	rts

ENTRY_NOPROFILE(lev7intr)
	tstl	_ASM_LABEL(plx_nonmi)	| milan_conf_read shortcut
	jne	1f			| .... get out immediately
	INTERRUPT_SAVEREG
	movl	_C_LABEL(stio_addr),%a0	| get KVA of ST-IO area
	movw	%a0@(PLX_PCICR),_C_LABEL(plx_status)
	movw	#0xf900,%a0@(PLX_PCICR)	| Clear error bits
	jbsr	_C_LABEL(nmihandler)	| notify...
	INTERRUPT_RESTOREREG
	addql	#1,_C_LABEL(intrcnt)+28	| add another nmi interrupt
1:
	rte				| all done
#endif /* _MILANHW_ */

ENTRY_NOPROFILE(lev3intr)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG
	movw	%sp@(22),%sp@-		|  push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		|  and PC
	jbsr	_C_LABEL(straytrap)	|  report
	addql	#8,%sp			|  pop args
	INTERRUPT_RESTOREREG		|  restore regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		|  all done

ENTRY_NOPROFILE(badmfpint)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG		|  save scratch regs
	movw	%sp@(22),%sp@-		|  push exception vector info
	clrw	%sp@-
	movl	%sp@(22),%sp@-		|  and PC
	jbsr	_C_LABEL(straymfpint)	|  report
	addql	#8,%sp			|  pop args
	INTERRUPT_RESTOREREG		|  restore regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		|  all done

/*
 * Trace (single-step) trap.  Kernel-mode is special.
 * User mode traps are simply passed on to trap().
 */
ENTRY_NOPROFILE(trace)
	clrl	%sp@-			| stack adjust count
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRACE,%d0

	| Check PSW and see what happen.
	|   T=0 S=0	(should not happen)
	|   T=1 S=0	trace trap from user mode
	|   T=0 S=1	trace trap on a trap instruction
	|   T=1 S=1	trace trap from system mode (kernel breakpoint)

	movw	%sp@(FR_HW),%d1		| get PSW
	notw	%d1			| XXX no support for T0 on 680[234]0
	andw	#PSL_TS,%d1		| from system mode (T=1, S=1)?
	jeq	Lkbrkpt			| yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	| no, user-mode fault

/*
 * Trap 15 is used for:
 *	- GDB breakpoints (in user programs)
 *	- KGDB breakpoints (in the kernel)
 *	- trace traps for SUN binaries (not fully supported yet)
 * User mode traps are simply passed to trap().
 */
ENTRY_NOPROFILE(trap15)
	clrl	%sp@-
	moveml	#0xFFFF,%sp@-
	moveq	#T_TRAP15,%d0
	movw	%sp@(FR_HW),%d1		|  get PSW
	andw	#PSL_S,%d1		|  from system mode?
	jne	Lkbrkpt			|  yes, kernel breakpoint
	jra	_ASM_LABEL(fault)	|  no, user-mode fault

Lkbrkpt:
	| Kernel-mode breakpoint or trace trap. (d0=trap_type)
	| Save the system sp rather than the user sp.
	movw	#PSL_HIGHIPL,%sr	| lock out interrupts
	lea	%sp@(FR_SIZE),%a6	| Save stack pointer
	movl	%a6,%sp@(FR_SP)		|  from before trap

	| If were are not on tmpstk switch to it.
	| (so debugger can change the stack pointer)
	movl	%a6,%d1
	cmpl	#_ASM_LABEL(tmpstk),%d1
	jls	Lbrkpt2				| already on tmpstk
	| Copy frame to the temporary stack
	movl	%sp,%a0				| a0=src
	lea	_ASM_LABEL(tmpstk)-96,%a1	| a1=dst
	movl	%a1,%sp				| sp=new frame
	moveq	#FR_SIZE,%d1
Lbrkpt1:
	movl	%a0@+,%a1@+
	subql	#4,%d1
	bgt	Lbrkpt1

Lbrkpt2:
	| Call the trap handler for the kernel debugger.
	| Do not call trap() to do it, so that we can
	| set breakpoints in trap() if we want.  We know
	| the trap type is either T_TRACE or T_BREAKPOINT.
	| If we have both DDB and KGDB, let KGDB see it first,
	| because KGDB will just return 0 if not connected.
	| Save args in d2, a2
	movl	%d0,%d2			| trap type
	movl	%sp,%a2			| frame ptr
#ifdef KGDB
	| Let KGDB handle it (if connected)
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kgdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
	cmpl	#0,%d0			| did kgdb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#ifdef DDB
	| Let DDB handle it
	movl	%a2,%sp@-		| push frame ptr
	movl	%d2,%sp@-		| push trap type
	jbsr	_C_LABEL(kdb_trap)	| handle the trap
	addql	#8,%sp			| pop args
#if 0	/* not needed on atari */
	cmpl	#0,%d0			| did ddb handle it?
	jne	Lbrkpt3			| yes, done
#endif
#endif
	/* Sun 3 drops into PROM here. */
Lbrkpt3:
	| The stack pointer may have been modified, or
	| data below it modified (by kgdb push call),
	| so push the hardware frame at the current sp
	| before restoring registers and returning.

	movl	%sp@(FR_SP),%a0		| modified sp
	lea	%sp@(FR_SIZE),%a1	| end of our frame
	movl	%a1@-,%a0@-		| copy 2 longs with
	movl	%a1@-,%a0@-		| ... predecrement
	movl	%a0,%sp@(FR_SP)		| sp = h/w frame
	moveml	%sp@+,#0x7FFF		| restore all but sp
	movl	%sp@,%sp		| ... and sp
	rte				| all done

/*
 * Interrupt handlers.
 *
 *	Level 0:	Spurious: ignored.
 *	Level 1:	softint
 *	Level 2:	HBL
 *	Level 3:	not used
 *	Level 4:	not used
 *	Level 5:	SCC (not used)
 *	Level 6:	MFP1/MFP2 (not used -> autovectored)
 *	Level 7:	Non-maskable: shouldn't be possible. ignore.
 */

/* Provide a generic interrupt dispatcher, only handle hardclock (int6)
 * specially, to improve performance
 */

ENTRY_NOPROFILE(spurintr)
	addql	#1,_C_LABEL(intrcnt)+0
	INTERRUPT_SAVEREG		|  save scratch registers
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG		|  restore scratch regs
	jra	_ASM_LABEL(rei)

	/* MFP timer A handler --- System clock --- */
ASENTRY_NOPROFILE(mfp_tima)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG		|  save scratch registers
	movl	%sp,%sp@-		|  push pointer to clockframe
	jbsr	_C_LABEL(hardclock)	|  call generic clock int routine
	addql	#4,%sp			|  pop params
	addql	#1,_C_LABEL(intrcnt_user)+52
					|  add another system clock interrupt
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG		|  restore scratch regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		|  all done

#ifdef STATCLOCK
	/* MFP timer C handler --- Stat/Prof clock --- */
ASENTRY_NOPROFILE(mfp_timc)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG		|  save scratch registers
	jbsr	_C_LABEL(statintr)	|  call statistics clock handler
	addql	#1,_C_LABEL(intrcnt)+36	|  add another stat clock interrupt
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG		|  restore scratch regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)		|  all done
#endif /* STATCLOCK */

#if NKBD > 0
	/* MFP ACIA handler --- keyboard/midi --- */
ASENTRY_NOPROFILE(mfp_kbd)
	addql	#1,_C_LABEL(intr_depth)
	addql	#1,_C_LABEL(intrcnt)+8	|  add another kbd/mouse interrupt

	INTERRUPT_SAVEREG		|  save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(kbdintr)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG		|  restore scratch regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)
#endif /* NKBD */

#if NNCRSCSI > 0
	/* MFP2 SCSI DMA handler --- NCR5380 --- */
ASENTRY_NOPROFILE(mfp2_5380dm)
	addql	#1,_C_LABEL(intr_depth)
	addql	#1,_C_LABEL(intrcnt)+24	|  add another 5380-DMA interrupt

	INTERRUPT_SAVEREG		|  save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(scsi_dma)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG		|  restore scratch regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)

	/* MFP2 SCSI handler --- NCR5380 --- */
ASENTRY_NOPROFILE(mfp2_5380)
	addql	#1,_C_LABEL(intr_depth)
	addql	#1,_C_LABEL(intrcnt)+20	|  add another 5380-SCSI interrupt

	INTERRUPT_SAVEREG		|  save scratch registers
	movw	%sp@(16),%sp@-		|  push previous SR value
	clrw	%sp@-			|     padded to longword
	jbsr	_C_LABEL(scsi_ctrl)	|  handle interrupt
	addql	#4,%sp			|  pop SR
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG		|  restore scratch regs
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)
#endif /* NNCRSCSI > 0 */

#ifdef _ATARIHW_
	/* Level 1 (Software) interrupt handler */
ENTRY_NOPROFILE(lev1intr)
	addql	#1,_C_LABEL(intr_depth)
	INTERRUPT_SAVEREG		|  save scratch registers
	movl	_C_LABEL(stio_addr),%a0 |  get KVA of ST-IO area
	moveb	#0, %a0@(SCU_SOFTINT)	|  Turn off software interrupt
	addql	#1,_C_LABEL(intrcnt)+16	|  add another software interrupt
	jbsr	_C_LABEL(nullop)	|  XXX handle software interrupts
	CPUINFO_INCREMENT(CI_NINTR)
	INTERRUPT_RESTOREREG
	subql	#1,_C_LABEL(intr_depth)
	jra	_ASM_LABEL(rei)

	/*
	 * Should never occur, except when special hardware modification
	 * is installed. In this case, one expects to be dropped into
	 * the debugger.
	 */
ENTRY_NOPROFILE(lev7intr)
#ifdef DDB
	/*
	 * Note that the nmi has to be turned off while handling it because
	 * the hardware modification has no de-bouncing logic....
	 */
	addql	#1,_C_LABEL(intr_depth)
	movl	%a0, %sp@-		|  save a0
	movl	_C_LABEL(stio_addr),%a0	|  get KVA of ST-IO area
	movb	%a0@(SCU_SYSMASK),%sp@-	|  save current sysmask
	movb	#0, %a0@(SCU_SYSMASK)	|  disable all interrupts
	trap	#15			|  drop into the debugger
	movb	%sp@+, %a0@(SCU_SYSMASK)|  restore sysmask
	movl	%sp@+, %a0		|  restore a0
	subql	#1,_C_LABEL(intr_depth)
#endif
	addql	#1,_C_LABEL(intrcnt)+28	|  add another nmi interrupt
	rte				|  all done

#endif /* _ATARIHW_ */

/*
 * Initialization
 *
 * A5 contains physical load point from boot
 * exceptions vector thru our table, that's bad.. just hope nothing exceptional
 * happens till we had time to initialize ourselves..
 */
	BSS(lowram,4)
	BSS(esym,4)

	.globl	_C_LABEL(edata)
	.globl	_C_LABEL(etext),_C_LABEL(end)

GLOBAL(bootversion)
	.word	0x0003			|  Glues kernel/installboot/loadbsd
					|    and other bootcode together.
ASENTRY_NOPROFILE(start)
	movw	#PSL_HIGHIPL,%sr	| No interrupts

	/*
	 * a0 = start of loaded kernel
	 * a1 = value of esym
	 * d0 = fastmem size
	 * d1 = stmem size
	 * d2 = cputype
	 * d3 = boothowto
	 * d4 = size of loaded kernel
	 */
	movl	#8,%a5			| Addresses 0-8 are mapped to ROM on the
	addql	#8,%a0			|  atari ST. We cannot set these.
	subl	#8,%d4

	/*
	 * Copy until end of kernel relocation code.
	 */
Lstart0:
	movl	%a0@+,%a5@+
	subl	#4, %d4
	cmpl	#Lstart3,%a5
	jle	Lstart0
	/*
	 * Enter kernel at destination address and continue copy
	 * Make sure that the jump is absolute (by adding ':l') otherwise
	 * the assembler tries to use a pc-relative jump.
	 * Which is definitely not what is needed at this point!
	 */
	jmp	Lstart2:l
Lstart2:
	movl	%a0@+,%a5@+		| copy the rest of the kernel
	subl	#4, %d4
	jcc	Lstart2
Lstart3:

	lea	_ASM_LABEL(tmpstk),%sp	| give ourselves a temporary stack

	/*
	 *  save the passed parameters. `prepass' them on the stack for
	 *  later catch by _start_c
	 */
	movl	%a1,%sp@-		| pass address of _esym
	movl	%d1,%sp@-		| pass stmem-size
	movl	%d0,%sp@-		| pass fastmem-size
	movl	%d5,%sp@-		| pass fastmem_start
	movl	%d2,%sp@-		| pass machine id
	movl	%d3,_C_LABEL(boothowto)	| save reboot flags


	/*
	 * Set cputype and mmutype dependent on the machine-id passed
	 * in from the loader. Also make sure that all caches are cleared.
	 */
	movl	#ATARI_68030,%d1		| 68030 type from loader
	andl	%d2,%d1
	jeq	Ltestfor020			| Not an 68030, try 68020
	movl	#MMU_68030,_C_LABEL(mmutype)	| Use 68030 MMU
	movl	#CPU_68030,_C_LABEL(cputype)	|   and a 68030 CPU
	movl	#CACHE_OFF,%d0			| 68020/030 cache clear
	jra	Lend_cpuset			| skip to init.
Ltestfor020:
	movl	#ATARI_68020,%d1		| 68020 type from loader
	andl	%d2,%d1
	jeq	Ltestfor040
	movl	#MMU_68851,_C_LABEL(mmutype)	| Assume 68851 with 68020
	movl	#CPU_68020,_C_LABEL(cputype)	|   and a 68020 CPU
	movl	#CACHE_OFF,%d0			| 68020/030 cache clear
	jra	Lend_cpuset			| skip to init.
Ltestfor040:
	movl	#CACHE_OFF,%d0			| 68020/030 cache
	movl	#ATARI_68040,%d1
	andl	%d2,%d1
	jeq	Ltestfor060
	movl	#MMU_68040,_C_LABEL(mmutype)	| Use a 68040 MMU
	movl	#CPU_68040,_C_LABEL(cputype)	|   and a 68040 CPU
	.word	0xf4f8				| cpusha bc - push&inval caches
	movl	#CACHE40_OFF,%d0		| 68040 cache disable
	jra	Lend_cpuset			| skip to init.
Ltestfor060:
	movl    #ATARI_68060,%d1
	andl	%d2,%d1
	jeq	Lend_cpuset
	movl	#MMU_68040,_C_LABEL(mmutype)	| Use a 68040 MMU
	movl	#CPU_68060,_C_LABEL(cputype)	|   and a 68060 CPU
	.word	0xf4f8				| cpusha bc - push&inval caches
	movl	#CACHE40_OFF,%d0		| 68040 cache disable
	orl	#IC60_CABC,%d0			|   and clear  060 branch cache

Lend_cpuset:
	movc	%d0,%cacr		| clear and disable on-chip cache(s)
	movl	#_C_LABEL(vectab),%a0	| set address of vector table
	movc	%a0,%vbr

	/*
	 * let the C function initialize everything and enable the MMU
	 */
	jsr	_C_LABEL(start_c)

	/*
	 * set kernel stack, user SP
	 */
	movl	_C_LABEL(lwp0uarea),%a1	| grab lwp0 uarea
	lea	%a1@(USPACE-4),%sp	| set kernel stack to end of area
	movl	#USRSTACK-4,%a2
	movl	%a2,%usp		| init user SP
	movl	%a2,%a1@(PCB_USP)	| and save it
	clrw	%a1@(PCB_FLAGS)		| clear flags

	/* flush TLB and turn on caches */
	jbsr	_C_LABEL(_TBIA)		|  invalidate TLB
	movl	#CACHE_ON,%d0
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jne	Lcacheon
	/*  is this needed? MLH */
	.word	0xf4f8			|  cpusha bc - push & invalidate caches
	movl	#CACHE40_ON,%d0
#ifdef M68060
	cmpl	#CPU_68060,_C_LABEL(cputype)
	jne	Lcacheon
	movl	#CACHE60_ON,%d0
#endif
Lcacheon:
	movc	%d0,%cacr		|  clear cache(s)

	/*
	 * Final setup for C code
	 */
#ifdef notdef
	movl	%d6,_C_LABEL(bootdev)	|    and boot device
#endif

/*
 * Create a fake exception frame so that cpu_lwp_fork() can copy it.
 * main() nevers returns; we exit to user mode from a forked process
 * later on.
 */
	movl	#0,%a6			| make DDB stack_trace() work
	clrw	%sp@-			| vector offset/frame type
	clrl	%sp@-			| PC - filled in by "execve"
	movw	#PSL_USER,%sp@-		| in user mode
	clrl	%sp@-			| stack adjust count and padding
	lea	%sp@(-64),%sp		| construct space for D0-D7/A0-A7
	lea	_C_LABEL(lwp0),%a0	| save pointer to frame
	movl	%sp,%a0@(L_MD_REGS)     |   in lwp0.l_md.md_regs

	jra	_C_LABEL(main)		| main()

/*
 * Primitives
 */

/*
 * non-local gotos
 */
ENTRY(qsetjmp)
	movl	%sp@(4),%a0		|  savearea pointer
	lea	%a0@(40),%a0		|  skip regs we do not save
	movl	%a6,%a0@+		|  save FP
	movl	%sp,%a0@+		|  save SP
	movl	%sp@,%a0@		|  and return address
	moveq	#0,%d0			|  return 0
	rts

/*
 * Use common m68k process/lwp switch and context save subroutines.
 */
#define FPCOPROC
#include <m68k/m68k/switch_subr.s>

ENTRY(ecacheon)
	rts

ENTRY(ecacheoff)
	rts

/*
 * Check out a virtual address to see if it's okay to write to.
 *
 * probeva(va, fc)
 *
 */
ENTRY(probeva)
	movl	%sp@(8),%d0
	movec	%d0,%dfc
	movl	%sp@(4),%a0
	.word	0xf548			|  ptestw (a0)
	moveq	#FC_USERD,%d0		|  restore DFC to user space
	movc	%d0,%dfc
	.word	0x4e7a,0x0805		|  movec  MMUSR,d0
	rts

/*
 * Handle the nitty-gritty of rebooting the machine.
 *
 */
ENTRY_NOPROFILE(doboot)
	movl	#CACHE_OFF,%d0
	cmpl	#MMU_68040,_C_LABEL(mmutype) |  is it 68040?
	jne	Ldoboot0
	.word	0xf4f8			|  cpusha bc - push and inval caches
	nop
	movl	#CACHE40_OFF,%d0
Ldoboot0:
	movc	%d0,%cacr		|  disable on-chip cache(s)

	movw	#0x2700,%sr		|  cut off any interrupts

	/*
	 * Clear first 2k of ST-memory. We start clearing at address 0x8
	 * because the lower 8 bytes are mapped to ROM.
	 * This makes sure that the machine will 'cold-boot'.
	 */
	movl	_C_LABEL(page_zero),%a0
	addl	#0x8,%a0
	movl	#512,%d0
Ldb1:
	clrl	%a0@+
	dbra	%d0,Ldb1

	lea	Ldoreboot,%a1		| a1 = start of copy range
	lea	Ldorebootend,%a2	| a2 = end of copy range
	movl	_C_LABEL(page_zero),%a0	| a0 = virtual base for page zero
	addl	%a1,%a0			|		+ offset of Ldoreboot
Ldb2:					| Do the copy
	movl	%a1@+,%a0@+
	cmpl	%a2,%a1
	jle	Ldb2

	/*
	 * Ok, turn off MMU..
	 */
Ldoreboot:
	cmpl	#MMU_68040,_C_LABEL(mmutype)
	jeq	Lmmuoff040		| Go turn off 68040 MMU
	lea	_ASM_LABEL(zero),%a0
	pmove	%a0@,%tc		| Turn off MMU
	lea	_ASM_LABEL(nullrp),%a0
	pmove	%a0@,%crp		| Invalidate CPU root pointer
	pmove	%a0@,%srp		|  and the Supervisor root pointer
	jra	Ldoboot1		| Ok, continue with actual reboot
Lmmuoff040:
	movl	#0,%d0
	.word	0x4e7b,0x0003		|  movc d0,TC
	.word	0x4e7b,0x0806		|  movc d0,URP
	.word	0x4e7b,0x0807		|  movc d0,SRP

Ldoboot1:
	movl	#0, %a0
	movc	%a0,%vbr
	movl	%a0@(4), %a0		| fetch reset-vector
	jmp	%a0@			| jump through it
	/* NOTREACHED */

/*  A do-nothing MMU root pointer (includes the following long as well) */

ASLOCAL(nullrp)
	.long	0x7fff0001
ASLOCAL(zero)
	.long	0
Ldorebootend:

	.data
	.p2align 2
	.space	PAGE_SIZE
ASLOCAL(tmpstk)

#ifdef M68060 /* XXX */
L60iem:		.long	0
L60fpiem:	.long	0
L60fpdem:	.long	0
L60fpeaem:	.long	0
#endif
#ifdef DEBUG

ASLOCAL(fulltflush)
	.long	0
ASLOCAL(fullcflush)
	.long	0
GLOBAL(timebomb)
	.long	0
#endif
ASLOCAL(plx_nonmi)
	.long	0
GLOBAL(plx_status)
	.long	0

/* interrupt counters & names */
#include <atari/atari/intrcnt.h>
