/* $NetBSD: crt0-common.c,v 1.15 2018/03/09 20:20:47 joerg Exp $ */

/*
 * Copyright (c) 1998 Christos Zoulas
 * Copyright (c) 1995 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: crt0-common.c,v 1.15 2018/03/09 20:20:47 joerg Exp $");

#include <sys/types.h>
#include <sys/exec.h>
#include <sys/syscall.h>
#include <machine/profile.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtld.h"

extern int main(int, char **, char **);

#ifndef HAVE_INITFINI_ARRAY
extern void	_init(void);
extern void	_fini(void);
#endif
extern void	_libc_init(void);

/*
 * Arrange for _DYNAMIC to be weak and undefined (and therefore to show up
 * as being at address zero, unless something else defines it).  That way,
 * if we happen to be compiling without -static but with without any
 * shared libs present, things will still work.
 */

__weakref_visible int rtld_DYNAMIC __weak_reference(_DYNAMIC);

#ifdef MCRT0
extern void	monstartup(u_long, u_long);
extern void	_mcleanup(void);
extern unsigned char __etext, __eprol;
#endif /* MCRT0 */

char		**environ;
struct ps_strings *__ps_strings = 0;

static char	 empty_string[] = "";
char		*__progname = empty_string;

__dead __dso_hidden void ___start(void (*)(void), const Obj_Entry *,
			 struct ps_strings *);

#define	write(fd, s, n)	__syscall(SYS_write, (fd), (s), (n))

#define	_FATAL(str)				\
do {						\
	write(2, str, sizeof(str)-1);		\
	_exit(1);				\
} while (0)

#ifdef HAVE_INITFINI_ARRAY
/*
 * If we are using INIT_ARRAY/FINI_ARRAY and we are linked statically,
 * we have to process these instead of relying on RTLD to do it for us.
 *
 * Since we don't need .init or .fini sections, just code them in C
 * to make life easier.
 */
extern const fptr_t __preinit_array_start[] __dso_hidden;
extern const fptr_t __preinit_array_end[] __dso_hidden __weak;
extern const fptr_t __init_array_start[] __dso_hidden;
extern const fptr_t __init_array_end[] __dso_hidden __weak;
extern const fptr_t __fini_array_start[] __dso_hidden;
extern const fptr_t __fini_array_end[] __dso_hidden __weak;

static inline void
_preinit(void)
{
	for (const fptr_t *f = __preinit_array_start; f < __preinit_array_end; f++) {
		(*f)();
	}
}

static inline void
_init(void)
{
	for (const fptr_t *f = __init_array_start; f < __init_array_end; f++) {
		(*f)();
	}
}

static void
_fini(void)
{
	for (const fptr_t *f = __fini_array_start; f < __fini_array_end; f++) {
		(*f)();
	}
}
#endif /* HAVE_INITFINI_ARRAY */

#if defined(__x86_64__) || defined(__powerpc__) || defined(__sparc__)
#define HAS_IPLTA
static void fix_iplta(void) __noinline;
#elif defined(__i386__) || defined(__arm__)
#define HAS_IPLT
static void fix_iplt(void) __noinline;
#endif


#ifdef HAS_IPLTA
#include <stdio.h>
extern const Elf_Rela __rela_iplt_start[] __dso_hidden __weak;
extern const Elf_Rela __rela_iplt_end[] __dso_hidden __weak;

static void
fix_iplta(void)
{
	const Elf_Rela *rela, *relalim;
	uintptr_t relocbase = 0;
	Elf_Addr *where, target;

	rela = __rela_iplt_start;
	relalim = __rela_iplt_end;
#if DEBUG
	printf("%p - %p\n", rela, relalim);
#endif
	for (; rela < relalim; ++rela) {
		if (ELF_R_TYPE(rela->r_info) != R_TYPE(IRELATIVE))
			abort();
		where = (Elf_Addr *)(relocbase + rela->r_offset);
#if DEBUG
		printf("location: %p\n", where);
#endif
		target = (Elf_Addr)(relocbase + rela->r_addend);
#if DEBUG
		printf("target: %p\n", (void *)target);
#endif
		target = ((Elf_Addr(*)(void))target)();
#if DEBUG
		printf("...resolves to: %p\n", (void *)target);
#endif
		*where = target;
	}
}
#endif
#ifdef HAS_IPLT
extern const Elf_Rel __rel_iplt_start[] __dso_hidden __weak;
extern const Elf_Rel __rel_iplt_end[] __dso_hidden __weak;

static void
fix_iplt(void)
{
	const Elf_Rel *rel, *rellim;
	uintptr_t relocbase = 0;
	Elf_Addr *where, target;

	rel = __rel_iplt_start;
	rellim = __rel_iplt_end;
	for (; rel < rellim; ++rel) {
		if (ELF_R_TYPE(rel->r_info) != R_TYPE(IRELATIVE))
			abort();
		where = (Elf_Addr *)(relocbase + rel->r_offset);
		target = ((Elf_Addr(*)(void))*where)();
		*where = target;
	}
}
#endif

void
___start(void (*cleanup)(void),			/* from shared loader */
    const Obj_Entry *obj,			/* from shared loader */
    struct ps_strings *ps_strings)
{

	if (ps_strings == NULL)
		_FATAL("ps_strings missing\n");
	__ps_strings = ps_strings;

	environ = ps_strings->ps_envstr;

	if (ps_strings->ps_argvstr[0] != NULL) {
		char *c;
		__progname = ps_strings->ps_argvstr[0];
		for (c = ps_strings->ps_argvstr[0]; *c; ++c) {
			if (*c == '/')
				__progname = c + 1;
		}
	} else {
		__progname = empty_string;
	}

	if (&rtld_DYNAMIC != NULL) {
		if (obj == NULL)
			_FATAL("NULL Obj_Entry pointer in GOT\n");
		if (obj->magic != RTLD_MAGIC)
			_FATAL("Corrupt Obj_Entry pointer in GOT\n");
		if (obj->version != RTLD_VERSION)
			_FATAL("Dynamic linker version mismatch\n");
		atexit(cleanup);
	}

	_libc_init();

	if (&rtld_DYNAMIC == NULL) {
#ifdef HAS_IPLTA
		fix_iplta();
#endif
#ifdef HAS_IPLT
		fix_iplt();
#endif
	}

#ifdef HAVE_INITFINI_ARRAY
	_preinit();
#endif

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&__eprol, (u_long)&__etext);
#endif

	atexit(_fini);
	_init();

	exit(main(ps_strings->ps_nargvstr, ps_strings->ps_argvstr, environ));
}
