/*	$NetBSD: crtbegin.c,v 1.18 2002/04/08 22:17:37 skrll Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg, Ross Harvey, and Jason R. Thorpe.
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
 * Run-time module which handles constructors and destructors,
 * and NetBSD identification.
 *
 * The linker constructs the following arrays of pointers to global
 * constructors and destructors. The first element contains the
 * number of pointers in each or -1 to indicate that the run-time
 * code should figure out how many there are.  The tables are also
 * null-terminated.
 */

#include <sys/param.h>		/* sysident.h requires `NetBSD' constant */
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <stdlib.h>

#ifdef DWARF2_EH
#include "dwarf2_eh.h"
#endif
#include "sysident.h"
#include "dot_init.h"

static void (*__CTOR_LIST__[1])(void)
    __attribute__((section(".ctors"))) = { (void *)-1 };	/* XXX */
static void (*__DTOR_LIST__[1])(void)
    __attribute__((section(".dtors"))) = { (void *)-1 };	/* XXX */

#ifdef DWARF2_EH
static char __EH_FRAME_BEGIN__[]
    __attribute__((section(".eh_frame"), aligned(4))) = { };
#endif

#if defined(JCR) && defined(__GNUC__)
extern void _Jv_RegisterClasses(void *) __attribute__((weak));

static void *__JCR_LIST__[]
    __attribute__((section(".jcr"))) = { };
#endif

#if defined(DSO_HANDLE) && defined(__GNUC__)
/*
 * The __dso_handle variable is used to hang C++ local destructors off
 * of.  In the main program (i.e. using crtbegin.o), the value is 0.
 * In shared objects (i.e. using crtbeginS.o), the value must be unique.
 * The symbol is hidden, but the dynamic linker will still relocate it.
 */
#ifdef SHARED
void	*__dso_handle = &__dso_handle;
#else
void	*__dso_handle = NULL;
#endif
__asm(".hidden	__dso_handle");

#ifdef SHARED
extern void __cxa_finalize(void *) __attribute__((weak));
#endif
#endif

static void __dtors(void);
static void __ctors(void);

INIT_FALLTHRU_DECL;
FINI_FALLTHRU_DECL;

extern void _init(void)   __attribute__((section(".init")));
extern void _fini(void)   __attribute__((section(".fini")));
static void __ctors(void) __attribute__((section(".init")));
static void __dtors(void) __attribute__((section(".fini")));

static void
__ctors()
{
	unsigned long i = (unsigned long) __CTOR_LIST__[0];
	void (**p)(void);

	if (i == -1)  {
		for (i = 1; __CTOR_LIST__[i] != NULL; i++)
			;
		i--;
	}
	p = __CTOR_LIST__ + i;
	while (i--)
		(**p--)();
}

static void
__dtors()
{
	void (**p)(void) = __DTOR_LIST__ + 1;

	while (*p)
		(**p++)();
}

void
_init()
{
	static int initialized = 0;
#ifdef DWARF2_EH
#if defined(__GNUC__)
	static struct dwarf2_eh_object object;
#endif /* __GNUC__ */
#endif /* DWARF2_EH */

	/*
	 * Execute code in the .init sections
	 */
	INIT_FALLTHRU();
	if (!initialized) {
		initialized = 1;

#ifdef DWARF2_EH
#if defined(__GNUC__)
		if (__register_frame_info != NULL)
			__register_frame_info(__EH_FRAME_BEGIN__, &object);
#endif /* __GNUC__ */
#endif /* DWARF2_EH */

#if defined(JCR) && defined(__GNUC__)
		if (__JCR_LIST__[0] != NULL && _Jv_RegisterClasses != NULL)
			_Jv_RegisterClasses(__JCR_LIST__);
#endif /* JCR && __GNUC__ */

		/*
		 * Call global constructors.
		 */
		__ctors();
	}
}

void
_fini()
{

#if defined(DSO_HANDLE) && defined(__GNUC__) && defined(SHARED)
	/*
	 * Call local destructors.
	 */
	if (__cxa_finalize != NULL)
		__cxa_finalize(__dso_handle);
#endif /* DSO_HANDLE && __GNUC__ && SHARED */

	/*
	 * Call global destructors.
	 */
	__dtors();

#ifdef DWARF2_EH
#if defined(__GNUC__)
	if (__deregister_frame_info != NULL)
		__deregister_frame_info(__EH_FRAME_BEGIN__);
#endif /* __GNUC__ */
#endif /* DWARF2_EH */

	/*
	 * Execute code in the .fini sections
	 */
	FINI_FALLTHRU();
}

MD_INIT_SECTION_PROLOGUE;

MD_FINI_SECTION_PROLOGUE;
