/*	$NetBSD: machdep.h,v 1.1 2002/06/05 01:04:20 fredette Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 * Definitions for the hppa that are completely private 
 * to the machine-dependent code.  Anything needed by
 * machine-independent code is covered in cpu.h or in
 * other headers.
 */

/*
 * XXX there is a lot of stuff in various headers under 
 * hp700/include and hppa/include, and a lot of one-off 
 * `extern's in C files that could probably be moved here.
 */

#ifdef _KERNEL

/*      
 * cache configuration, for most machines is the same
 * numbers, so it makes sense to do defines w/ numbers depending
 * on configured cpu types in the kernel.
 */
extern int icache_stride, icache_line_mask;
extern int dcache_stride, dcache_line_mask;

extern vaddr_t vmmap;	/* XXX - See mem.c */

/* Kernel virtual address space available: */
extern vaddr_t virtual_start, virtual_end;

/* Physical pages available. */
extern int totalphysmem;

/* FPU variables and functions. */
extern int fpu_present;
extern u_int fpu_version;
extern u_int fpu_csw;
extern paddr_t fpu_cur_uspace;
void hppa_fpu_bootstrap __P((u_int));
void hppa_fpu_flush __P((struct proc *));
void hppa_fpu_emulate __P((struct trapframe *, struct proc *));

/* Interrupt dispatching. */
extern u_int hppa_intr_depth;
void hppa_intr __P((struct trapframe *));

/* Special pmap functions. */
void pmap_bootstrap __P((vaddr_t *, vaddr_t *));
void pmap_redzone __P((vaddr_t, vaddr_t, int));

/* Functions to write low memory and the kernel text. */
void hppa_ktext_stw __P((vaddr_t, int));
void hppa_ktext_stb __P((vaddr_t, char));

/* Machine check handling. */
extern u_int os_hpmc;
extern u_int os_hpmc_cont;
extern u_int os_hpmc_cont_end;
int os_toc __P((void));
extern u_int os_toc_end;
void hppa_machine_check __P((int));

/* This reloads the BTLB. */
int hppa_btlb_reload __P((void)); 

#endif /* _KERNEL */
