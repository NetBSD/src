/*	$NetBSD: linux32_exec.h,v 1.6 2009/12/10 14:13:53 matt Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_LINUX32_EXEC_H_
#define	_LINUX32_EXEC_H_

#include <compat/netbsd32/netbsd32.h>

#if defined(__amd64__)
#include <compat/linux32/arch/amd64/linux32_exec.h>
#endif

extern struct emul emul_linux32;

/* XXXmanu Do a.out later... */

#ifdef LINUX32_NPTL
void linux_nptl_exit_hook(struct proc *);
#endif


#ifdef EXEC_ELF32
int linux32_elf32_probe(struct lwp *, struct exec_package *, void *,
    char *, vaddr_t *); 
int linux32_elf32_copyargs(struct lwp *, struct exec_package *,
    struct ps_strings *, char **, void *);
int linux_elf32_signature(struct lwp *, struct exec_package *,
        Elf32_Ehdr *, char *);
#ifdef LINUX32_GCC_SIGNATURE
int linux_elf32_gcc_signature(struct lwp *l,
        struct exec_package *, Elf32_Ehdr *);
#endif
#ifdef LINUX32_DEBUGLINK_SIGNATURE
int linux_elf32_debuglink_signature(struct lwp *l,
        struct exec_package *, Elf32_Ehdr *);
#endif
#ifdef LINUX32_ATEXIT_SIGNATURE
int linux_elf32_atexit_signature(struct lwp *l,
        struct exec_package *, Elf32_Ehdr *);
#endif

#endif /* EXEC_ELF32 */

void linux32_setregs (struct lwp *, struct exec_package *, vaddr_t stack);
int linux32_sigreturn (struct proc *, void *, register_t *);
void linux32_sendsig (const ksiginfo_t *, const sigset_t *);

#endif /* !_LINUX32_EXEC_H_ */
