/* $NetBSD: exec_ecoff.c,v 1.4 2000/06/14 06:49:20 cgd Exp $ */

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: exec_ecoff.c,v 1.4 2000/06/14 06:49:20 cgd Exp $");
#endif
 
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#if defined(NLIST_ECOFF)

#include <sys/exec_ecoff.h>

int
check_ecoff(int fd, const char *filename)
{
	struct ecoff_exechdr eh;
	struct stat sb;

	/*
	 * Check the header to make sure it's an ECOFF file (of the
	 * appropriate size).
	 */
	if (fstat(fd, &sb) == -1)
		return 0;
	if (sb.st_size < sizeof eh)
		return 0;
	if (read(fd, &eh, sizeof eh) != sizeof eh)
		return 0;

	if (ECOFF_BADMAG(&eh))
		return 0;

	return 1;
}

int
hide_ecoff(int fd, const char *filename)
{

	fprintf(stderr, "%s: ECOFF executables not currently supported\n",
		filename);
	return 1;
}

#endif /* defined(NLIST_ECOFF) */
