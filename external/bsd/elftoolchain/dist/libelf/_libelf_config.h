/*	$NetBSD: _libelf_config.h,v 1.6 2024/03/03 17:37:33 christos Exp $	*/

/*-
 * Copyright (c) 2008-2011 Joseph Koshy
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Id: _libelf_config.h 3975 2022-04-30 20:10:58Z jkoshy
 */

#if defined(__NetBSD__) || defined(HAVE_NBTOOL_CONFIG_H)

#include <machine/elf_machdep.h>

#if	!defined(ARCH_ELFSIZE)
#error	ARCH_ELFSIZE is not defined.
#endif

#if	ARCH_ELFSIZE == 32
#define	LIBELF_ARCH		ELF32_MACHDEP_ID
#define	LIBELF_BYTEORDER	ELF32_MACHDEP_ENDIANNESS
#define	LIBELF_CLASS		ELFCLASS32
#define	Elf_Note		Elf32_Nhdr
#else
#define	LIBELF_ARCH		ELF64_MACHDEP_ID
#define	LIBELF_BYTEORDER	ELF64_MACHDEP_ENDIANNESS
#define	LIBELF_CLASS		ELFCLASS64
#define	Elf_Note		Elf64_Nhdr
#endif

#endif	/* __NetBSD__ || HAVE_NBTOOL_CONFIG_H */

/*
 * Downstream projects can replace the following placeholder with a custom
 * definition of LIBELF_BYTEORDER, if the host's native byte order cannot
 * be determined using the compilation environment.
 */
/* @LIBELF-DEFINE-HOST-BYTEORDER@ */

#ifndef HAVE_NBTOOL_CONFIG_H
#if	!defined(LIBELF_BYTEORDER)

/*
 * Use the __BYTE_ORDER__ and __ORDER_{LITTLE|BIG}_ENDIAN__ macros to
 * determine the host's byte order.  These macros are predefined by the
 * GNU and CLANG C compilers.
 */
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define	LIBELF_BYTEORDER	ELFDATA2LSB
#else
#define	LIBELF_BYTEORDER	ELFDATA2MSB
#endif

#else

#error unknown host byte order

#endif	/* defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) */
#endif	/* !defined(LIBELF_BYTEORDER) */
#endif /* HAVE_NBTOOL_CONFIG_H */
