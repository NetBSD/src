/*
 * Copyright (c) 1995 Christopher G. Demetriou
 * All rights reserved.
 *
 * Modifications for NetBSD/mips:
 *
 *	Jonathan Stone
 *	Jason R. Thorpe, Mumerical Aerospace Simulation Facility,
 *	    NASA Ames Research Center
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

#ifdef ECOFF_COMPAT
#undef DYNAMIC
#endif

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/syscall.h>
#include <vm/vm.h>

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

#ifndef ECOFF_COMPAT
extern void	__init __P((void));
extern void	__fini __P((void));
#endif /* ECOFF_COMPAT */

#ifdef DYNAMIC
void		rtld_setup __P((void (*)(void), const Obj_Entry *obj));

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
 *	C start-up.  Assumes kernel (or ld.so) passes the
 *	following parameters to user-space in registers:
 *
 *	a0	stack pointer (0 if setregs didn't fill this in)
 *	a1	cleanup
 *	a2	Obj_Entry
 *	a3	ps_strings
 *
 *	XXX Does this violate the ABI?
 *	as well as the usual registers (pc, sp, and t9 == pc for ABI).
 */

void
__start(sp, cleanup, obj, ps_strings)
	register u_long sp;
	register void (*cleanup) __P((void));	/* from shared loader */
	register const Obj_Entry *obj;		/* from shared loader */
	register struct ps_strings *ps_strings;
{
	register char **ksp;
	register char **argv, *namep;
	register int argc;

	/*
	 * Grab the argc, argv, and envp set up by the kernel.
	 * Layout of stuff on the stack:
	 *
	 *	[ high ]
	 *	char	kenvstr[1];	// size varies 
	 *	char	kargstr[1];	// size varies
	 *	char	*argv[1];	// varies on argc
	 *	int	argc;
	 *	[ low ] 		<--- kernel's SP points here
	 *	.
	 *	.
	 *	.
	 *	[ current stack pointer ]
	 *
	 * WARNING!  There is an implicit sizeof(int) == sizeof(char *) here!
	 */

#ifndef DYNAMIC
	__asm __volatile("la $28,_gp");
#endif

	ksp = (char**)sp;
	if (ksp == 0) {
		/*
		 * Uh, oh. We're running on a old kernel that passed
		 * us zero in $a0-$a3.  Try adjusting the current
		 * $sp for our own stack-frame size and see  what
		 * we find.
		 * WARNING!  The constants 56 and 64 below were determined
		 * by examining GCC assembler output of __start to find the
		 * frame size.  If you change the code or compiler,
		 * you _will_ lose!
		 */
 
#ifndef DYNAMIC
		/* XXX 56 is compiler and stackframe dependent */
		__asm __volatile("	addiu	%0,$29,56" : "=r" (ksp));
#else
		/* XXX 64 is compiler and stackframe dependent */
		__asm __volatile("	addiu	%0,$29,64" : "=r" (ksp));
#endif
	}


	argc = *(int *)ksp;
	argv = ksp + 1;
	environ = ksp + 2 + argc;	/* 2: argc + NULL ending argv */

	if ((namep = argv[0]) != NULL) {	/* NULL ptr if argc = 0 */
		if ((__progname = _strrchr(namep, '/')) == NULL)
			__progname = namep;
		else
			__progname++;
	}

	/*
	 * Deal with stuff that is only provided if setregs() did
	 * did the right thing.
	 */
	if (sp != 0) {
		if (ps_strings != (struct ps_strings *)0)
			__ps_strings = ps_strings;
#ifdef DYNAMIC
		/*
		 * XXX Old MIPS ld.so didn't do any of this at all.
		 * XXX If we were loaded by that loader, just abort
		 * XXX the rtld setup.
		 */
		if (&_DYNAMIC != NULL && cleanup != NULL /*&& obj != NULL*/)
			rtld_setup(cleanup, obj);
#endif
	}

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&_eprol, (u_long)&_etext);
#endif

	/* XXX __fini and __init are not yet  on a MIPS. */
#if 0
#ifndef ECOFF_COMPAT
	atexit(__fini);
	__init();
#endif /* ECOFF_COMPAT */
#endif
	exit(main(argc, argv, environ));
}


static char *
_strrchr(p, ch)
register char *p, ch;
{
	register char *save;

	for (save = NULL; ; ++p) {
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
rtld_setup(cleanup, obj)
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
