/* $NetBSD: cerror.c,v 1.2 2003/07/26 19:38:47 salo Exp $ */

/*
 * Copyright (c) 2003 Christopher G. Demetriou
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

/*
 * Test program to make sure that libc's cerror (error handler) is doing
 * approximately the right thing for both 32-bit and 64-bit error returns.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: cerror.c,v 1.2 2003/07/26 19:38:47 salo Exp $");
__COPYRIGHT(
"@(#) Copyright (c) 2003 Christopher G. Demetriou.  All rights reserved.\n");
#endif /* not lint */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ERROR(x)	do { fprintf(stderr, "%s\n", x); errs++; } while (0)

int
main(int argc, char *argv[])
{
	int rv_int;
	off_t rv_off;
	int errs = 0;

	/* Make sure file desc 4 is closed.  */
	(void)close(4);

	/* Check error and 32-bit return code.  */
	errno = 0;
	rv_int = close(4);
	if (errno != EBADF)
		ERROR("close() on closed FD didn't set errno to EBADF");
	if (rv_int != -1)
		ERROR("close() on closed FD didn't return -1");

	
	/* Check error and 64-bit return code.  */
	errno = 0;
	rv_off = lseek(4, (off_t)0, SEEK_SET);
	if (errno != EBADF)
		ERROR("lseek() on closed FD didn't set errno to EBADF");
	if (rv_off != (off_t)-1)
		ERROR("lseek() on closed FD didn't return -1");

	exit(errs == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
