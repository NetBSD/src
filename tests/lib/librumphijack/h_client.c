/*	$NetBSD: h_client.c,v 1.2.2.2 2011/02/17 12:00:54 bouyer Exp $	*/

/*
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/select.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{

	if (argc != 2) {
		errx(1, "need testname as param");
	}

	if (strcmp(argv[1], "select_timeout") == 0) {
		fd_set rfds;
		struct timeval tv;
		int rv;

		tv.tv_sec = 0;
		tv.tv_usec = 1;

		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);

		rv = select(STDIN_FILENO+1, &rfds, NULL, NULL, &tv);
		if (rv == -1)
			err(1, "select");
		if (rv != 0)
			errx(1, "select succesful");

		if (FD_ISSET(STDIN_FILENO, &rfds))
			errx(1, "stdin fileno is still set");
		exit(0);
	} else if (strcmp(argv[1], "select_allunset") == 0) {
		fd_set fds;
		struct timeval tv;
		int rv;

		tv.tv_sec = 0;
		tv.tv_usec = 1;

		FD_ZERO(&fds);

		rv = select(100, &fds, &fds, &fds, &tv);
		if (rv == -1)
			err(1, "select");
		if (rv != 0)
			errx(1, "select succesful");

		rv = select(0, NULL, NULL, NULL, &tv);
		if (rv == -1)
			err(1, "select2");
		if (rv != 0)
			errx(1, "select2 succesful");

		exit(0);
	} else {
		return ENOTSUP;
	}
}
