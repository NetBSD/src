/*	$NetBSD: netbsd32_exec.h,v 1.4 1999/12/30 15:40:45 eeh Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_NETBSD32_EXEC_H_
#define	_NETBSD32_EXEC_H_

#include <compat/netbsd32/netbsd32.h>

/* from <sys/exec_aout.h> */
/*
 * Header prepended to each a.out file.
 * only manipulate the a_midmag field via the
 * N_SETMAGIC/N_GET{MAGIC,MID,FLAG} macros below.
 */
struct netbsd32_exec {
	netbsd32_u_long	a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
	netbsd32_u_long	a_text;		/* text segment size */
	netbsd32_u_long	a_data;		/* initialized data size */
	netbsd32_u_long	a_bss;		/* uninitialized data size */
	netbsd32_u_long	a_syms;		/* symbol table size */
	netbsd32_u_long	a_entry;	/* entry point */
	netbsd32_u_long	a_trsize;	/* text relocation size */
	netbsd32_u_long	a_drsize;	/* data relocation size */
};

#ifdef EXEC_AOUT
int exec_netbsd32_makecmds __P((struct proc *, struct exec_package *));
#endif
#ifdef EXEC_ELF32
int netbsd32_elf32_probe __P((struct proc *, struct exec_package *, Elf32_Ehdr *,
    char *, Elf32_Addr *));
#endif /* EXEC_ELF32 */

#endif /* !_NETBSD32_EXEC_H_ */
