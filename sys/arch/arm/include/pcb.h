/*	$NetBSD: pcb.h,v 1.1 2001/09/03 19:20:27 matt Exp $	*/

/*
 * Copyright (c) 2001 Matt Thomas <matt@3am-software.com>.
 * Copyright (c) 1994 Mark Brinicombe.
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name "RiscBSD" nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RISCBSD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL RISCBSD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_ARM_PCB_H_
#define	_ARM_PCB_H_

#include <machine/frame.h>
#include <machine/pte.h>
#include <machine/fp.h>

struct trapframe;

struct pcb_arm32 {
	pd_entry_t *pcb32_pagedir;		/* PT hooks */
	u_int	pcb32_r8;			/* used */
	u_int	pcb32_r9;			/* used */
	u_int	pcb32_r10;			/* used */
	u_int	pcb32_r11;			/* used */
	u_int	pcb32_r12;			/* used */
	u_int	pcb32_sp;			/* used */
	u_int	pcb32_lr;
	u_int	pcb32_pc;
	u_int	pcb32_und_sp;
};
#define	pcb_pagedir	pcb_un.un_32.pcb32_pagedir
#define	pcb_r8		pcb_un.un_32.pcb32_r8
#define	pcb_r9		pcb_un.un_32.pcb32_r9
#define	pcb_r10		pcb_un.un_32.pcb32_r10
#define	pcb_r11		pcb_un.un_32.pcb32_r11
#define	pcb_r12		pcb_un.un_32.pcb32_r12
#define	pcb_sp		pcb_un.un_32.pcb32_sp
#define	pcb_lr		pcb_un.un_32.pcb32_lr
#define	pcb_pc		pcb_un.un_32.pcb32_pc
#define	pcb_und_sp	pcb_un.un_32.pcb32_und_sp

struct pcb_arm26 {
	struct	switchframe *pcb26_sf;
};
#define	pcb_sf	pcb_un.un_26.pcb26_sf

struct pcb {
	u_int	pcb_flags;
#define	PCB_OWNFPU	0x00000001
	struct	trapframe *pcb_tf;
	caddr_t	pcb_onfault;			/* On fault handler */
	union	{
		struct	pcb_arm32 un_32;
		struct	pcb_arm26 un_26;
	} pcb_un;
	struct	fpe_sp_state pcb_fpstate;	/* Floating Point state */
};
#define	pcb_ff	pcb_fpstate			/* for arm26 */

/*
 * No additional data for core dumps.
 */
struct md_coredump {
	int	md_empty;
};

#ifdef _KERNEL
extern struct pcb *curpcb;
#endif	/* _KERNEL */

#endif	/* _ARM_PCB_H_ */
