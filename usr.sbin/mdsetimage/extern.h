/* $NetBSD: extern.h,v 1.9 2001/10/01 23:32:34 cgd Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#ifdef NLIST_AOUT
int	check_aout __P((const char *, size_t));
int	findoff_aout __P((const char *, size_t, u_long, size_t *));
#endif
#ifdef NLIST_ECOFF
int	check_ecoff __P((const char *, size_t));
int	findoff_ecoff __P((const char *, size_t, u_long, size_t *));
#endif
#ifdef NLIST_ELF32
int	check_elf32 __P((const char *, size_t));
int	findoff_elf32 __P((const char *, size_t, u_long, size_t *));
#endif
#ifdef NLIST_ELF64
int	check_elf64 __P((const char *, size_t));
int	findoff_elf64 __P((const char *, size_t, u_long, size_t *));
#endif
#ifdef NLIST_COFF
int	check_coff __P((const char *, size_t));
int	findoff_coff __P((const char *, size_t, u_long, size_t *));
#endif
