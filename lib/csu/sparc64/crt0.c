/*	$NetBSD: crt0.c,v 1.4 1999/03/08 10:49:08 kleink Exp $	*/

/*
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#include <sys/types.h>
#include <sys/exec.h>
#include <sys/syscall.h>

#include <stdlib.h>
#ifdef DYNAMIC
#include <dlfcn.h>
#include "rtld.h"
#else
typedef void Obj_Entry;
#endif

/*
 * Lots of the chunks of this file cobbled together from pieces of
 * other NetBSD crt files, including the common code.
 */

extern int	__syscall __P((int, ...));
#define	_exit(v)	__syscall(SYS_exit, (v))
#define	write(fd, s, n)	__syscall(SYS_write, (fd), (s), (n))

#define _FATAL(str)				\
	do {					\
		write(2, str, sizeof(str));	\
		_exit(1);			\
	} while (0)

static char	*_strrchr __P((char *, char));


char	**environ;
char	*__progname = "";
struct ps_strings *__ps_strings = 0;

extern void	_init __P((void));
extern void	_fini __P((void));

#ifdef DYNAMIC
void		_rtld_setup __P((void (*)(void), const Obj_Entry *obj));

const Obj_Entry *__mainprog_obj;

/*
 * Arrange for _DYNAMIC to be weak and undefined (and therefore to show up
 * as being at address zero, unless something else defines it).  That way,
 * if we happen to be compiling without -static but with without any
 * shared libs present, things will still work.
 */
asm(".weak _DYNAMIC");
extern int _DYNAMIC;
#endif /* DYNAMIC */

#ifdef MCRT0
extern void	monstartup __P((u_long, u_long));
extern void	_mcleanup __P((void));
extern unsigned char _etext, _eprol;
#endif /* MCRT0 */

/*
 * _start needs to gather up argc, argv, env_p, ps_strings, the termination
 * routine passed in %g1 and call __start to finish up the startup processing.
 *
 * NB: We are violating the ELF spec by passing a pointer to the ps strings in %g1
 *	instead of a termination routine.
 */

__asm__("
	.data
__data_start:					! Start of data section
	.text
	.align 4
	.global start
	.global _start
start:
_start:
	setx	__data_start, %o0, %g4		! Point %g4 to start of data section
	clr	%g4				! egcs thinks this is zero. XXX
	clr	%fp
	add	%sp, 8*16 + 0x7ff, %o0		! start of stack
	mov	%g1, %o1			! Cleanup routine
	clr	%o1				! XXXX
	clr	%o2				! obj
	mov	%g1, %o3			! ps_strings XXXX
");

void __start __P((char **, void (*cleanup) __P((void)), const Obj_Entry *,
		struct ps_strings *));
int main __P((int, char **, char **));

void
__start(sp, cleanup, obj, ps_strings)
	char **sp;
	void (*cleanup) __P((void));		/* from shared loader */
	const Obj_Entry *obj;			/* from shared loader */
	struct ps_strings *ps_strings;
{
	long argc;
	char **argv, *namep;

	argc = *(int *)sp;
	argv = sp + 1;
	environ = sp + 2 + argc;		/* 2: argc + NULL ending argv */

	if ((namep = argv[0]) != NULL) {	/* NULL ptr if argc = 0 */
		if ((__progname = _strrchr(namep, '/')) == NULL)
			__progname = namep;
		else
			__progname++;
	}

	if (ps_strings != (struct ps_strings *)0 &&
	    ps_strings != (struct ps_strings *)0xbabefacedeadbeef)
		__ps_strings = ps_strings;

#ifdef DYNAMIC
	if (&_DYNAMIC != NULL)
		_rtld_setup(cleanup, obj);
#endif

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&_eprol, (u_long)&_etext);
#endif

	atexit(_fini);
	_init();
	exit(main(argc, argv, environ));
}

/*
 * NOTE: Leave the RCS ID _after_ _start(), in case it gets placed in .text.
 */
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.4 1999/03/08 10:49:08 kleink Exp $");
#endif /* LIBC_SCCS and not lint */

/*
 * The following allows linking w/o crtbegin.o and crtend.o
 */

extern void	__init __P((void));
extern void	__fini __P((void));

#ifdef __weak_alias
__weak_alias(_init, __init);
__weak_alias(_fini, __fini);
#else
asm (" .weak _init; _init = __init");
asm (" .weak _fini; _fini = __fini");
#endif

void
__init() 
{
}

void
__fini()
{
}

static char *
_strrchr(p, ch)
char *p, ch;
{
	char *save;

	for (save = NULL;; ++p) {
		if (*p == ch)
			save = (char *)p;
		if (!*p)
			return(save);
	}
/* NOTREACHED */
}

#ifdef MCRT0
asm ("  .text");
asm ("_eprol:");
#endif

#ifdef DYNAMIC
void
_rtld_setup(cleanup, obj)
	void (*cleanup) __P((void));
	const Obj_Entry *obj;
{

	if ((obj == NULL) || (obj->magic != RTLD_MAGIC))
		_FATAL("Corrupt Obj_Entry pointer in GOT");
	if (obj->version != RTLD_VERSION)
		_FATAL("Dynamic linker version mismatch");

	__mainprog_obj = obj;
	atexit(cleanup);
}

void *
dlopen(name, mode)
	const char *name;
	int mode;
{

	if (__mainprog_obj == NULL)
		return NULL;
	return (__mainprog_obj->dlopen)(name, mode);
}

int
dlclose(fd)
	void *fd;
{

	if (__mainprog_obj == NULL)
		return -1;
	return (__mainprog_obj->dlclose)(fd);
}

void *
dlsym(fd, name)
	void *fd;
	const char *name;
{

	if (__mainprog_obj == NULL)
		return NULL;
	return (__mainprog_obj->dlsym)(fd, name);
}

#if 0 /* not supported for ELF shlibs, apparently */
int
dlctl(fd, cmd, arg)
	void *fd, *arg;
	int cmd;
{

	if (__mainprog_obj == NULL)
		return -1;
	return (__mainprog_obj->dlctl)(fd, cmd, arg);
}
#endif

char *
dlerror()
{

	if (__mainprog_obj == NULL)
		return NULL;
	return (__mainprog_obj->dlerror)();
}
#endif /* DYNAMIC */
