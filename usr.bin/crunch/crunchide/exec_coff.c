/* $NetBSD: exec_coff.c,v 1.3 2000/06/14 06:49:20 cgd Exp $ */

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
__RCSID("$NetBSD: exec_coff.c,v 1.3 2000/06/14 06:49:20 cgd Exp $");
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <a.out.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "extern.h"

#if defined(NLIST_COFF)

#include <sys/exec_coff.h>

int
check_coff(int fd, const char *filename)
{
	struct coff_filehdr fh;
	struct stat sb;

	/*
	 * Check the header to make sure it's an COFF file (of the
	 * appropriate size).
	 */
	if (fstat(fd, &sb) == -1)
		return 0;
	if (sb.st_size > SIZE_T_MAX)
		return 0;
	if (read(fd, &fh, sizeof fh) != sizeof fh)
		return 0;

	if (COFF_BADMAG(&fh))
		return 0;

	return 1;
}

int
hide_coff(int fd, const char *filename)
{

	fprintf(stderr, "%s: COFF executables not currently supported\n",
		filename);
	return 1;
}

#endif /* defined(NLIST_COFF) */
