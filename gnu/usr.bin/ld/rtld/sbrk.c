/*
 * Copyright (c) 1993 Paul Kranenburg
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
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *	$Id: sbrk.c,v 1.1 1993/12/08 10:33:45 pk Exp $
 */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#ifndef BSD
#define MAP_COPY	MAP_PRIVATE
#define MAP_FILE	0
#define MAP_ANON	0
#endif
#include <fcntl.h>
#include <a.out.h>
#include <stab.h>
#include <string.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "ld.h"

#ifndef BSD		/* Need do better than this */
#define NEED_DEV_ZERO	1
#endif

/*
 * Private heap functions.
 */
extern char	**environ;

static caddr_t	curbrk;

static void
init_brk()
{
	struct rlimit   rlim;
	char		*cp, **cpp = environ;

	if (getrlimit(RLIMIT_STACK, &rlim) < 0) {
		xprintf("ld.so: brk: getrlimit failure\n");
		_exit(1);
	}

	/*
	 * Walk to the top of stack
	 */
	if (*cpp) {
		while (*cpp) cpp++;
		cp = *--cpp;
		while (*cp) cp++;
	} else
		cp = (char *)&cp;

#define TRY_THIS_FOR_A_CHANGE 0

#if TRY_THIS_FOR_A_CHANGE
	curbrk = (caddr_t)
		(((long)(cp - 1 - rlim.rlim_max) + PAGSIZ) & ~(PAGSIZ - 1));
	curbrk -= PAGSIZ;
#else
	curbrk = (caddr_t)
		(((long)(cp - 1 - rlim.rlim_cur) + PAGSIZ) & ~(PAGSIZ - 1));
#endif
}

#if 1
caddr_t
sbrk(incr)
int incr;
{
	int	fd = -1;
	caddr_t	oldbrk;

	if (curbrk == 0)
		init_brk();

#if DEBUG
xprintf("sbrk: incr = %#x, curbrk = %#x\n", incr, curbrk);
#endif
	if (incr == 0)
		return curbrk;

#if TRY_THIS_FOR_A_CHANGE
	if (incr > PAGSIZ)
		return (caddr_t)-1;
#endif

	incr = (incr + PAGSIZ - 1) & ~(PAGSIZ - 1);

#ifdef NEED_DEV_ZERO
	fd = open("/dev/zero", O_RDWR, 0);
	if (fd == -1)
		perror("/dev/zero");
#endif

	if (mmap(curbrk, incr,
			PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_FIXED|MAP_COPY, fd, 0) == (caddr_t)-1) {
		xprintf("Cannot map anonymous memory");
		_exit(1);
	}

#ifdef NEED_DEV_ZERO
	close(fd);
#endif

	oldbrk = curbrk;
#if TRY_THIS_FOR_A_CHANGE
	curbrk -= incr;
#else
	curbrk += incr;
#endif

	return oldbrk;
}
#else

caddr_t
sbrk(incr)
int incr;
{
	int	fd = -1;
	caddr_t	oldbrk;

xprintf("sbrk: incr = %#x, curbrk = %#x\n", incr, curbrk);
#if DEBUG
xprintf("sbrk: incr = %#x, curbrk = %#x\n", incr, curbrk);
#endif
	if (curbrk == 0 && (curbrk = mmap(0, PAGSIZ,
			PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_COPY, fd, 0)) == (caddr_t)-1) {
		xprintf("Cannot map anonymous memory");
		_exit(1);
	}

	/* There's valid memory from `curbrk' to next page boundary */
	if ((long)curbrk + incr <= (((long)curbrk + PAGSIZ) & ~(PAGSIZ - 1))) {
		oldbrk = curbrk;
		curbrk += incr;
		return oldbrk;
	}
	/*
	 * If asking for than currently left in this chunk,
	 * go somewhere completely different.
	 */

#ifdef NEED_DEV_ZERO
	fd = open("/dev/zero", O_RDWR, 0);
	if (fd == -1)
		perror("/dev/zero");
#endif

	if ((curbrk = mmap(0, incr,
			PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_COPY, fd, 0)) == (caddr_t)-1) {
		perror("Cannot map anonymous memory");
	}

#ifdef NEED_DEV_ZERO
	close(fd);
#endif

	oldbrk = curbrk;
	curbrk += incr;

	return oldbrk;
}
#endif
