/*	$NetBSD: prename.c,v 1.2 2002/02/21 07:38:18 itojun Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(void)
{
	int errors = 0;
	struct stat sb;

	(void)unlink("t1");
	(void)unlink("t2");
	if (creat("t1", 0600) < 0) {
		perror("create t1");
		exit(1);
	}

	if (link("t1", "t2")) {
		perror("link t1 t2");
		exit(1);
	}

	/* Check if rename to same name works as expected */
	if (rename("t1", "t1")) {
		perror("rename t1 t1");
		errors++;
	}
	if (stat("t1", &sb)) {
		perror("rename removed file? stat t1");
		exit(1);
	}

	if (rename("t1", "t2")) {
		perror("rename t1 t2");
		errors++;
	}
#if BSD_RENAME
	/* check if rename of hardlinked file works the BSD way */
	if (stat("t1", &sb)) {
		if (errno != ENOENT) {
			perror("BSD rename should remove file! stat t1");
			errors++;
		}
	} else {
		fprintf(stderr, "BSD rename should remove file!");
		errors++;
	}
#else
	/* check if rename of hardlinked file works as the standard says */
	if (stat("t1", &sb)) {
		perror("Posix rename should not remove file! stat t1");
		errors++;
	}
#endif

	/* check if we get the expected error */
	/* this also exercises icky shared libraries goo */
	if (rename("no/such/file/or/dir", "no/such/file/or/dir")) {
		if (errno != ENOENT) {
			perror("rename no/such/file/or/dir");
			errors++;
		}
	} else {
		fprintf(stderr, "No error renaming no/such/file/or/dir\n");
		errors++;
	}
	
	exit(errors ? 1 : 0);
}
