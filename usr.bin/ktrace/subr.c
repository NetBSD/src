/*	$NetBSD: subr.c,v 1.17.2.1 2011/06/23 14:20:44 cherry Exp $	*/

/*-
 * Copyright (c) 1988, 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)subr.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: subr.c,v 1.17.2.1 2011/06/23 14:20:44 cherry Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>

#include <stdio.h>

#include "ktrace.h"

int
getpoints(int facs, char *s)
{
	int fac;
	int add = 1;

	/* Make -t-x equivalent to -t+-x since that is rather more useful */
	if (*s == '-' && (facs & ALL_POINTS) == 0)
		facs |= DEF_POINTS;

	for (;;) {
		switch (*s++) {
		case 0:
			return facs;
		case 'A':
			fac = ALL_POINTS;
			break;
		case 'a':
			fac = KTRFAC_EXEC_ARG;
			break;
		case 'c':
			fac = KTRFAC_SYSCALL | KTRFAC_SYSRET;
			break;
		case 'e':
			fac = KTRFAC_EMUL;
			break;
		case 'f':
			fac = KTRFAC_EXEC_FD;
			break;
		case 'i':
			fac = KTRFAC_GENIO;
			break;
		case 'n':
			fac = KTRFAC_NAMEI;
			break;
		case 's':
			fac = KTRFAC_PSIG;
			break;
		case 'u':
			fac = KTRFAC_USER;
			break;
		case 'S':
			fac = KTRFAC_MIB;
			break;
		case 'v':
			fac = KTRFAC_EXEC_ENV;
			break;
		case 'w':
			fac = KTRFAC_CSW;
			break;
		case '+':
			if (!add) {
				add = 1;
				continue;
			}
			fac = DEF_POINTS;
			break;
		case '-':
			add = 0;
			continue;
		default:
			return (-1);
		}
		if (add)
			facs |= fac;
		else
			facs &= ~fac;
	}
}
