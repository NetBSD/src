/*	$NetBSD: macho_machdep.h,v 1.2 2008/04/28 20:23:32 martin Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#ifndef _POWERPC_MACHO_MACHDEP_H_
#define	_POWERPC_MACHO_MACHDEP_H_

/* From Darwin's xnu/osfmk/mach/ppc/thread_status.h (s/ppc/powerpc/) */
#define MACHO_POWERPC_THREAD_STATE	1

struct exec_macho_powerpc_thread_state {
	unsigned int srr0;
	unsigned int srr1;
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;        
	unsigned int r3;        
	unsigned int r4;
	unsigned int r5;                
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int r13;
	unsigned int r14;
	unsigned int r15;
	unsigned int r16;
	unsigned int r17;
	unsigned int r18;
	unsigned int r19;
	unsigned int r20;
	unsigned int r21;
	unsigned int r22;
	unsigned int r23;
	unsigned int r24;
	unsigned int r25;
	unsigned int r26;
	unsigned int r27;
	unsigned int r28;
	unsigned int r29;
	unsigned int r30;
	unsigned int r31;
	unsigned int cr;
	unsigned int xer;
	unsigned int lr;
	unsigned int ctr;
	unsigned int mq;
	unsigned int vrsave;
};

#define MACHO_POWERPC_NEW_THREAD_STATE	1

/* 
 * From Darwin's xnu/osfmk/ppc/savearea.h  
 * struct exec_macho_powerpc_saved_state is called struct savearea there. 
 */
struct exec_macho_powerpc_saved_state {
	struct exec_macho_powerpc_savearea_comm {
		struct exec_macho_powerpc_saved_state *save_prev;
		unsigned int *sac_next;
		unsigned int *sac_prev;
		unsigned int save_flags;
		unsigned int save_level;
		unsigned int save_time[2];
		struct thread_activation *save_act;
		unsigned int sac_vrswap;
		unsigned int sac_alloc;
		unsigned int sac_flags;
		unsigned int save_misc0;
		unsigned int save_misc1;
		unsigned int save_misc2;
		unsigned int save_misc3;
		unsigned int save_misc4;
		unsigned int save_040[8];
	} save_hdr;
	unsigned int save_060[8];
	unsigned int save_r0;
	unsigned int save_r1;
	unsigned int save_r2;
	unsigned int save_r3;
	unsigned int save_r4;
	unsigned int save_r5;
	unsigned int save_r6;
	unsigned int save_r7;
	unsigned int save_r8;
	unsigned int save_r9;
	unsigned int save_r10;
	unsigned int save_r11;
	unsigned int save_r12;
	unsigned int save_r13;
	unsigned int save_r14;
	unsigned int save_r15;
	unsigned int save_r16;
	unsigned int save_r17;
	unsigned int save_r18;
	unsigned int save_r19;
	unsigned int save_r20;
	unsigned int save_r21;
	unsigned int save_r22;
	unsigned int save_r23;
	unsigned int save_r24;
	unsigned int save_r25;
	unsigned int save_r26;
	unsigned int save_r27;
	unsigned int save_r28;
	unsigned int save_r29;
	unsigned int save_r30;
	unsigned int save_r31;
	unsigned int save_srr0;
	unsigned int save_srr1;
	unsigned int save_cr;
	unsigned int save_xer;
	unsigned int save_lr;
	unsigned int save_ctr;
	unsigned int save_dar;
	unsigned int save_dsisr;
	unsigned int save_vscr[4];
	unsigned int save_fpscrpad;
	unsigned int save_fpscr;
	unsigned int save_exception;
	unsigned int save_vrsave;
	unsigned int save_sr0;
	unsigned int save_sr1;
	unsigned int save_sr2;
	unsigned int save_sr3;
	unsigned int save_sr4;
	unsigned int save_sr5;
	unsigned int save_sr6;
	unsigned int save_sr7;
	unsigned int save_sr8;
	unsigned int save_sr9;
	unsigned int save_sr10;
	unsigned int save_sr11;
	unsigned int save_sr12;
	unsigned int save_sr13;
	unsigned int save_sr14;
	unsigned int save_sr15;
	unsigned int save_180[8];
	unsigned int save_1A0[8];
	unsigned int save_1C0[8];
	unsigned int save_1E0[8];
	unsigned int save_200[8];
	unsigned int save_220[8];
	unsigned int save_240[8];
	unsigned int save_260[8];
};

#endif /* !_POWERPC_MACHO_MACHDEP_H_ */
