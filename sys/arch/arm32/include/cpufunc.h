/*	$NetBSD: cpufunc.h,v 1.1 1997/01/26 01:31:20 mark Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufunc.h
 *
 * Prototypes for cpu related functions.
 */

#ifndef _ARM32_CPUFUNC_H_
#define _ARM32_CPUFUNC_H_

#include <sys/types.h>

#ifdef _KERNEL

/* Assembly modules */

/*
 * Functions to manipulate the CPSR
 * (in arm32/arm32/setcpsr.S)
 */

u_int SetCPSR		__P((u_int bic, u_int eor));
u_int GetCPSR		__P((void));

/*
 * Functions to manipulate coproc #15 registers
 * (in arm32/arm32/coproc15.S
 */

void tlb_flush		__P((void));
void cache_clean	__P((void));
void sync_caches	__P((void));
void sync_icache	__P((void));
void cpu_control	__P((u_int control));
void cpu_domains	__P((u_int domains));
void setttb		__P((u_int ttb));

u_int cpu_id		__P((void));
u_int cpu_faultstatus	__P((void));
u_int cpu_faultaddress	__P((void));

/*
 * Functions to manipulate cpu r13
 * (in arm32/arm32/setstack.S)
 */

void set_stackptr	__P((u_int mode, u_int address));
u_int get_stackptr	__P((u_int mode));

#endif	/* _KERNEL */
#endif	/* _ARM32_CPUFUNC_H_ */

/* End of cpufunc.h */
