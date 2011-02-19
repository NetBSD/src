/*      $NetBSD: h_cwd.c,v 1.1 2011/02/19 13:19:52 pooka Exp $	*/

/*-
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
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int
main(void)
{
	char pwd[1024];

	if (chdir("/rump") == -1)
		err(1, "chdir1");
	if (getcwd(pwd, sizeof(pwd)) == NULL)
		err(1, "getcwd1");
	if (strcmp(pwd, "/rump") != 0)
		errx(1, "strcmp1");

	if (mkdir("dir", 0777) == -1)
		err(1, "mkdir2");
	if (chdir("dir") == -1)
		err(1, "chdir2");
	if (getcwd(pwd, sizeof(pwd)) == NULL)
		err(1, "getcwd2");
	if (strcmp(pwd, "/rump/dir") != 0)
		errx(1, "strcmp2");

	if (mkdir("dir", 0777) == -1)
		err(1, "mkdir3");
	if (chdir("dir") == -1)
		err(1, "chdir3");
	if (getcwd(pwd, sizeof(pwd)) == NULL)
		err(1, "getcwd3");
	if (strcmp(pwd, "/rump/dir/dir") != 0)
		errx(1, "strcmp3");

	if (chdir("..") == -1)
		err(1, "chdir4");
	if (getcwd(pwd, sizeof(pwd)) == NULL)
		err(1, "getcwd4");
	if (strcmp(pwd, "/rump/dir") != 0)
		errx(1, "strcmp4");

	if (chdir("../../../../../../..") == -1)
		err(1, "chdir5");
	if (getcwd(pwd, sizeof(pwd)) == NULL)
		err(1, "getcwd5");
	if (strcmp(pwd, "/rump") != 0)
		errx(1, "strcmp5");

	if (chdir("/") == -1)
		err(1, "chdir6");
	if (getcwd(pwd, sizeof(pwd)) == NULL)
		err(1, "getcwd6");
	if (strcmp(pwd, "/") != 0)
		errx(1, "strcmp6");

	return 0;
}
