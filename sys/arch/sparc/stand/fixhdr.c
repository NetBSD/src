/*	$NetBSD: fixhdr.c,v 1.2 1994/11/20 20:54:56 deraadt Exp $ */

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

#include <stdio.h>
#include <fcntl.h>

main(argc, argv)
int	argc;
char	*argv[];
{
	int	fd;
	struct sun_magic {
		unsigned int	a_dynamic:1;
		unsigned int	a_toolversion:7;
		unsigned char	a_machtype;
#define SUN_MID_SPARC	3
		unsigned short	a_magic;
	} sm;

	if (argc != 2) {
		fprintf(stderr, "Usage: fixhdr <bootfile>\n");
		exit(1);
	}

	if ((fd = open(argv[1], O_RDWR, 0)) < 0) {
		perror("open");
		exit(1);
	}

	if (read(fd, &sm, sizeof(sm)) != sizeof(sm)) {
		perror("read");
		exit(1);
	}

	sm.a_dynamic = 0;
	sm.a_toolversion = 1;
	sm.a_machtype = SUN_MID_SPARC;

	lseek(fd, 0, 0);
	if (write(fd, &sm, sizeof(sm)) != sizeof(sm)) {
		perror("write");
		exit(1);
	}
	(void)close(fd);
	return 0;
}
