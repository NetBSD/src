/*	$NetBSD: trap.h,v 1.9 2003/03/13 13:44:18 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_TRAP_H
#define	_SH5_TRAP_H

/*
 * SH5 Traps are identified by a value in the EXPEVT control register.
 *
 * We fabricate an AST trap: T_AST.
 */

/* Power-On Reset Exception (0x0) */
#define	T_POWERON	0x000	/* Power-On reset */


/* General Exceptions (VBR + 0x100) */
#define	T_READPROT	0x0a0	/* Data Protection violation Read */
#define	T_WRITEPROT	0x0c0	/* Data Protection violation Write */
#define	T_RADDERR	0x0e0	/* Data Address error Read */
#define	T_WADDERR	0x100	/* Data Address error Write */
#define	T_FPUEXC	0x120	/* FPU exception */
#define	T_TRAP		0x160	/* Unconditional trap (syscall) */
#define	T_RESINST	0x180	/* Reserved instruction */
#define	T_ILLSLOT	0x1a0	/* Illegal slot exception */
#define	T_FPUDIS	0x800	/* FPU disabled */
#define	T_SLOTFPUDIS	0x820	/* Delay Slot FPU disabled */
#define	T_EXECPROT	0xaa0	/* Instruction Protection Violation */
#define	T_IADDERR	0xae0	/* Instruction Address Error */


/* Debug Exceptions (RESVEC/DBRVEC + 0x100) */
#define	T_PANIC		?????
#define	T_CPURESET	0x020	/* CPU reset */
#define	T_DEBUGIA	0x900	/* Instruction Address Debug */
#define	T_DEBUGIV	0x920	/* Instruction Value Debug */
#define	T_BREAK		0x940	/* Software break */
#define	T_DEBUGOA	0x960	/* Operand Address Debug */
#define	T_DEBUGSS	0x980	/* Single Step Debug */


/* Debug Interrupts (RESVEC/DBRVEC + 0x200) */
/* Nothing */


/* TLB Miss Exceptions (VBR + 0x400) */
#define	T_RTLBMISS	0x040	/* Data TLB miss read */
#define	T_WTLBMISS	0x060	/* Data TLB miss write */
#define	T_ITLBMISS	0xa40	/* Instruction TLB Miss Error */


/* External Interrupts (VBR + 0x600) */
/* Nothing */

/* Software Exception Types */
#define	T_AST		0x0002	/* Asynchronous System Trap */
#define	T_NMI		0x0004	/* NMI trap */

/* Bit 0 set == trap came from user mode */
#define	T_USER		0x0001


/*
 * TRAPA codes
 */
#define	TRAPA_SYSCALL	0x80	/* NetBSD/sh5 native system call */


/*
 * Critical section owners
 */
#define	CRIT_FREE		0	/* Nobody is in the critical section */
#define	CRIT_EXIT		0x01	/* Flag bit for exiting crit section */
#define	CRIT_SYNC_EXCEPTION	0x02	/* Synchronous Exception Handler */
#define	CRIT_ASYNC_EXCEPTION	0x04	/* Asynchronous Exception Handler */
#define	CRIT_TLBMISS_TRAP	0x06	/* TLB Miss promoted to TRAP */


#if defined(_KERNEL) && !defined(_LOCORE)
extern void	userret(struct lwp *);
extern void	trap(struct lwp *, struct trapframe *);
extern void	trapa(struct lwp *, struct trapframe *);
extern void	panic_trap(struct trapframe *, register_t, register_t,
		    register_t);
extern const char *trap_type(int);
#if defined(DIAGNOSTIC) || defined(DDB)
extern void	dump_trapframe(void (*)(const char *, ...), const char *,
		    struct trapframe *);
#endif
extern label_t	*onfault;
#endif

#endif /* _SH5_TRAP_H */
