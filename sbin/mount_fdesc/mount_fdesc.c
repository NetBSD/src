/*
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	$Id: mount_fdesc.c,v 1.3 1993/08/24 13:15:14 pk Exp $
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>

main(c, v)
int c;
char *v[];
{
	extern char *optarg;
	extern int optind;
	int ch;
	int usage = 0;
	int mntflags = 0;
	char *dummy;
	char *mountpt;
	int rc;
	char *pname;

	pname = strrchr(v[0], '/');
	if (pname == NULL)
		pname = v[0];
	else
		pname++;

	/*
	 * Crack -F option
	 */
	while ((ch = getopt(c, v, "F:")) != EOF)
	switch (ch) {
	case 'F':
		mntflags = atoi(optarg);
		break;
	default:
	case '?':
		usage++;
		break;
	}

	/*
	 * Need two more arguments
	 */
	if (optind != (c - 2))
		usage++;

	if (usage) {
		fputs("usage: mount_fdesc [ fsoptions ] fdesc mount-point\n", stderr);
		exit(1);
	}

	/*
	 * Get target and mount point
	 */
	dummy = v[optind];
	mountpt = v[optind+1];

	rc = mount(MOUNT_FDESC, mountpt, mntflags, (caddr_t) 0);
	if (rc < 0) {
		perror(pname);
		exit(1);
	}
	exit(0);
}
