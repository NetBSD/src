/*	$NetBSD: irix_exec.h,v 1.4.2.4 2002/06/20 03:42:49 nathanw Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
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

#ifndef	_IRIX_EXEC_H_
#define	_IRIX_EXEC_H_

#include <sys/types.h> 
#include <sys/exec.h>
#include <sys/signal.h>
#include <sys/exec_elf.h>

#include <machine/vmparam.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>

/* IRIX specific per-process data, zero'ed on allocation */
struct irix_emuldata {
#define ied_startcopy ied_sigtramp
	void *ied_sigtramp[SVR4_NSIG];	/* Address of signal trampoline */
#define ied_endcopy ied_pptr	
	struct proc *ied_pptr;	/* parent process or NULL, for SIGHUP on exit */
	int ied_procblk_count;	/* semaphore for blockproc */
	struct proc *ied_shareparent; /* parent of the share group */
	int ied_shareaddr;	/* VM space is shared with parent */
	/* Only the share group parent keeps track of this: */
};

/* e_flags used by IRIX for ABI selection */
#define IRIX_EF_IRIX_ABI64	0x00000010
#define IRIX_EF_IRIX_ABIN32	0x00000020
#define IRIX_EF_IRIX_ABIO32	0x00000000
#define IRIX_EF_IRIX_ABI_MASK	0x00000030

#define IRIX_ELF_AUX_ENTRIES 7

int irix_check_exec __P((struct proc *p));

#ifdef EXEC_ELF32
#define IRIX_AUX_ARGSIZ howmany(IRIX_ELF_AUX_ENTRIES * \
    sizeof(Aux32Info), sizeof (Elf32_Addr))

int irix_elf32_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));

int irix_elf32_probe_o32 __P((struct proc *, struct exec_package *, void *,  
    char *, vaddr_t *));

int irix_elf32_probe_n32 __P((struct proc *, struct exec_package *, void *,  
    char *, vaddr_t *));
#endif

#ifdef EXEC_ELF64
/* #define IRIX_AUX_ARGSIZ howmany(IRIX_ELF_AUX_ENTRIES * \
    sizeof(Aux64Info), sizeof (Elf64_Addr))  */

int irix_elf64_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));

int irix_elf64_probe __P((struct proc *, struct exec_package *, void *,  
    char *, vaddr_t *));
#endif

extern const struct emul emul_irix_o32;
extern const struct emul emul_irix_n32;

#endif /* !_IRIX_EXEC_H_ */
