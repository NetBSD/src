/*	$NetBSD: freebsd_exec.h,v 1.17.22.1 2017/12/03 11:36:53 jdolecek Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	from: @(#)exec.h	8.1 (Berkeley) 6/11/93
 */

#ifndef	_FREEBSD_EXEC_H
#define	_FREEBSD_EXEC_H

#ifdef EXEC_ELF32
#define FREEBSD_ELF_BRAND_STRING "FreeBSD"
#define FREEBSD_ELF_INTERP_PREFIX_STRING "/usr/libexec/ld-elf.so"

int freebsd_elf32_probe(struct lwp *, struct exec_package *, void *,
    char *, vaddr_t *);
#endif /* EXEC_ELF32 */

void freebsd_setregs(struct lwp *, struct exec_package *, vaddr_t);

extern char freebsd_sigcode[], freebsd_esigcode[];
extern struct emul emul_freebsd;

#endif /* !_FREEBSD_EXEC_H */
