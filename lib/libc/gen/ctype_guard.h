/*	$NetBSD: ctype_guard.h,v 1.4 2025/03/30 15:38:38 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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

#ifndef	_LIBC_CTYPE_GUARD_H_
#define	_LIBC_CTYPE_GUARD_H_

#include <sys/cdefs.h>

#include "ctype_local.h"

/*
 * On platforms where char is signed, the ctype(3) functions have an
 * insidious trap where logic like
 *
 *	char *s = ...;
 *
 *	if (isspace(*s)) ...
 *
 * is undefined behaviour whenever the string s has any bytes outside
 * the 7-bit US-ASCII range.  The correct way to do this is to cast any
 * char inputs to unsigned char first:
 *
 *	if (isspace((unsigned char)*s)) ...
 *
 * Unfortunately, the buggy idiom without an unsigned char cast is
 * extremely prevalent in C code in the wild.  (See the ctype(3) man
 * page for details on why this idiom is required and why isspace
 * itself can't just be `fixed'.)
 *
 * The ctype(3) functions are implemented as macros that expand to
 * simple table lookups -- partly for performance, and partly to
 * deliberately trigger -Wchar-subscript compiler warnings for
 * suspicious code like the above.
 *
 * To noisily detect the undefined behaviour without incurring overhead
 * for the defined cases of the ctype(3) functions, we put a guard page
 * before the tables in memory so that (at least for inputs other than
 * 0xff, which coincides as signed char with EOF = -1) attempts to use
 * the ctype(3) functions on invalid inputs will reliably -- and
 * safely! -- crash with SIGSEGV instead of sometimes returning
 * nondeterministic nonsense results.
 *
 * We do this by defining two symbols, one local and one global:
 *
 *	- The local (C `static') symbol _C_ctype_tab_guarded_ is
 *	  page-aligned and has an extra page-sized buffer at the
 *	  beginning for a guard page.  It is defined as an ordinary C
 *	  array to keep the source code legible.
 *
 *	- The global symbol _C_ctype_tab_ starts one page past the
 *	  start of _C_ctype_tab_guarded_.  It is defined, via some
 *	  macros for convenience, by the assembler directives:
 *
 *		.type _C_ctype_tab_,@object
 *		.global _C_ctype_tab_
 *		_C_ctype_tab_ = _C_ctype_tab_guarded_ + PAGE_SIZE
 *
 *	  (with PAGE_SIZE being replaced by the numeric value from
 *	  vmparam.h -- actually, we use MAX_PAGE_SIZE for the handful
 *	  of architectures that support different page sizes on
 *	  different machines).
 *
 * Then, at startup, we mprotect the guard page PROT_NONE.
 *
 * The global symbol _C_ctype_tab_ is an immutable part of the libc
 * ABI, and is used by, e.g., libstdc++.so, so we have to define it
 * compatibly -- this is why it must be defined as an ELF global symbol
 * in its own right, and not simply handled inside libc.so by
 * additional arithmetic relative to _C_ctype_tab_guarded_.
 */

#if defined(__CHAR_UNSIGNED__)	/* disable if char is unsigned */
#  define	_CTYPE_GUARD_PAGE	0
#elif defined(__PIC__)		/* enable in shared libc */
#  define	_CTYPE_GUARD_PAGE	1
#else	/* static libc -- let's aim for space-efficiency for now */
#  define	_CTYPE_GUARD_PAGE	0
#endif

#ifdef __arm__
#  define	__ctype_table_object(name)				      \
	__asm(".type " #name ",%object")
#else
#  define	__ctype_table_object(name)				      \
	__asm(".type " #name ",@object")
#endif

#define	__ctype_table_size(name, guard, nelem, elemsize)		      \
	__CTASSERT(sizeof((guard)[0]) == (elemsize));			      \
	__CTASSERT(sizeof(guard) == _CTYPE_GUARD_SIZE + (nelem)*(elemsize));  \
	__asm(".size " #name "," ___STRING((nelem) * (elemsize)))

#if _CTYPE_GUARD_PAGE

#  include <machine/vmparam.h>

/*
 * _CTYPE_GUARD_SIZE must be a macro so it will work through ___STRING
 * to produce a string for symbol arithmetic in __asm.
 */
#  ifdef MAX_PAGE_SIZE
#    define	_CTYPE_GUARD_SIZE	MAX_PAGE_SIZE
#  else
#    define	_CTYPE_GUARD_SIZE	PAGE_SIZE
#  endif

enum {
	_C_CTYPE_TAB_GUARD = _CTYPE_GUARD_SIZE/sizeof(_C_ctype_tab_[0]),
#  ifdef __BUILD_LEGACY
	_C_COMPAT_BSDCTYPE_GUARD =
	    _CTYPE_GUARD_SIZE/sizeof(_C_compat_bsdctype[0]),
#  endif
	_C_TOLOWER_TAB_GUARD = _CTYPE_GUARD_SIZE/sizeof(_C_tolower_tab_[0]),
	_C_TOUPPER_TAB_GUARD = _CTYPE_GUARD_SIZE/sizeof(_C_toupper_tab_[0]),
};

#  define	__ctype_table	__aligned(_CTYPE_GUARD_SIZE)
#  define	__ctype_table_guarded(name, guard, nelem, elemsize)	      \
	__ctype_table_object(name);					      \
	__asm(".global " _C_LABEL_STRING(#name));			      \
	__asm(_C_LABEL_STRING(#name) " = " _C_LABEL_STRING(#guard) " + "      \
	    ___STRING(_CTYPE_GUARD_SIZE));				      \
	__ctype_table_size(name, guard, nelem, elemsize)

#else  /* !_CTYPE_GUARD_PAGE */

#  define	_CTYPE_GUARD_SIZE	0

enum {
	_C_CTYPE_TAB_GUARD = 0,
#  ifdef __BUILD_LEGACY
	_C_COMPAT_BSDCTYPE_GUARD = 0,
#  endif
	_C_TOLOWER_TAB_GUARD = 0,
	_C_TOUPPER_TAB_GUARD = 0,
};

/* Compiler can't see into __strong_alias, so mark it __used. */
#  define	__ctype_table	__used
#  define	__ctype_table_guarded(name, guard, nelem, elemsize)	      \
	__ctype_table_object(name);					      \
	__strong_alias(name, guard);					      \
	__ctype_table_size(name, guard, nelem, elemsize)

#endif	/* _CTYPE_GUARD_PAGE */

#endif	/* _LIBC_CTYPE_GUARD_H_ */
