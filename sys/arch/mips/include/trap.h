/*	$NetBSD: trap.h,v 1.15.96.2 2011/12/23 08:07:40 matt Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: trap.h 1.1 90/07/09
 *
 *	@(#)trap.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Trap codes
 * also known in trap.c for name strings
 */
#ifndef _MIPS_TRAP_H_
#define _MIPS_TRAP_H_

#define T_INT			0	/* Interrupt pending */
#define T_TLB_MOD		1	/* TLB modified fault */
#define T_TLB_LD_MISS		2	/* TLB miss on load or ifetch */
#define T_TLB_ST_MISS		3	/* TLB miss on a store */
#define T_ADDR_ERR_LD		4	/* Address error on a load or ifetch */
#define T_ADDR_ERR_ST		5	/* Address error on a store */
#define T_BUS_ERR_IFETCH	6	/* Bus error on an ifetch */
#define T_BUS_ERR_LD_ST		7	/* Bus error on a load or store */
#define T_SYSCALL		8	/* System call */
#define T_BREAK			9	/* Breakpoint */
#define T_RES_INST		10	/* Reserved instruction exception */
#define T_COP_UNUSABLE		11	/* Coprocessor unusable */
#define T_OVFLOW		12	/* Arithmetic overflow */

/*
 * Trap definitions added for r4000 port.
 */
#define	T_TRAP			13	/* Trap instruction */
#define	T_VCEI			14	/* Virtual coherency exception */
#define	T_FPE			15	/* Floating point exception */
#define	T_IMPL0			16	/* Implementation dependent */
#define	T_IMPL1			17	/* Implementation dependent */
#define	T_C2E			18	/* Reserved for precise COP2 exception */
#define	T_TLBRI			19	/* TLB Read-Inhibit exception */
#define	T_TLBXI			20	/* TLB Execution-Inhibit exception */
#define	T__RSRVRD21		21	/* Reserved */
#define	T_MDMX			22	/* MDMX Unusable exception */
#define	T_WATCH			23	/* Watch address reference */
#define	T_MCHECK		24	/* Machine Check */
#define	T_THREAD		25	/* Thread (MT ASE) Exceptions */
#define	T_DSPDIS		26	/* DSP ASE State Disabled */
#define	T__RSRVRD27		27	/* Reserved */
#define	T__RSRVRD28		28	/* Reserved */
#define	T__RSRVRD29		29	/* Reserved */
#define	T_CACHEERR		30	/* Cache Errror */
#define	T_VCED			31	/* Virtual coherency data (Reserved) */

#define	T_USER			0x20	/* user-mode flag or'ed with type */

#if defined(_KERNEL) && !defined(_LOCORE)
extern const char * const trap_names[];
#endif

#endif /* _MIPS_TRAP_H_ */
