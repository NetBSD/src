/* $NetBSD: h_spawn.c,v 1.2 2021/05/02 11:18:11 martin Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles Zhang <charles@NetBSD.org> and
 * Martin Husemann <martin@NetBSD.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	unsigned long ret;
	char *endp;

	if (argc == 2 && strcmp(argv[1], "--resetids") == 0) {
		if (getuid() != geteuid() || getgid() != getegid()) {
			fprintf(stderr, "uid/gid do not match effective ids, "
			    "uid: %d euid: %d gid: %d egid: %d\n",
			    getuid(), geteuid(), getgid(), getegid());
			exit(255);
		}
		return 0;
	} else if (argc != 2) {
		fprintf(stderr, "usage:\n\t%s (retcode)\n", getprogname());
		exit(255);
	}
	ret = strtoul(argv[1], &endp, 10);
	if (*endp != 0) {
		fprintf(stderr,
		    "invalid arg: %s\n"
		    "usage:\n\t%s (retcode)\n", endp, getprogname());
		exit(255);
	}

	fprintf(stderr, "%s exiting with status %lu\n", getprogname(), ret);
	return ret;
}
