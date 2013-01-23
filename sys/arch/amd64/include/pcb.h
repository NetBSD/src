/*	$NetBSD: pcb.h,v 1.17.8.1 2013/01/23 00:05:38 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	@(#)pcb.h	5.10 (Berkeley) 5/12/91
 */

/*
 * XXXfvdl these copyrights don't really match anymore
 */

#ifndef _AMD64_PCB_H_
#define _AMD64_PCB_H_

#ifdef __x86_64__

#include <sys/signal.h>

#include <machine/segments.h>
#include <machine/tss.h>
#include <machine/fpu.h>
#include <machine/sysarch.h>

#define	NIOPORTS	1024		/* # of ports we allow to be mapped */

struct pcb {
	int	  pcb_flags;
#define	PCB_USER_LDT	0x01		/* has user-set LDT */
#define	PCB_COMPAT32	0x02
	u_int	  pcb_cr0;		/* saved image of CR0 */
	uint64_t pcb_rsp0;
	uint64_t pcb_cr2;		/* page fault address (CR2) */
	uint64_t pcb_cr3;
	uint64_t pcb_rsp;
	uint64_t pcb_rbp;
	uint64_t pcb_usersp;
	uint32_t pcb_unused;		/* unused */
	struct	savefpu_i387 pcb_savefpu_i387; /* i387 status on last exception */
	struct	savefpu pcb_savefpu __aligned(16); /* floating point state */
	uint32_t pcb_unused_1[4];	/* unused */
	void     *pcb_onfault;		/* copyin/out fault recovery */
	struct cpu_info *pcb_fpcpu;	/* cpu holding our fp state. */
	uint64_t  pcb_fs;
	uint64_t  pcb_gs;
	int pcb_iopl;
};

/*    
 * The pcb is augmented with machine-dependent additional data for 
 * core dumps. For the i386, there is nothing to add.
 */     
struct md_coredump {
	long	md_pad[8];
};    

#else	/*	__x86_64__	*/

#include <i386/pcb.h>

#endif	/*	__x86_64__	*/

#endif /* _AMD64_PCB_H_ */
