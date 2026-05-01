/*	$NetBSD: openspecial.c,v 1.1 2026/05/01 20:39:26 christos Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: openspecial.c,v 1.1 2026/05/01 20:39:26 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <util.h>
#include <unistd.h>
#include <stdlib.h>
#include <fstab.h>
#include <string.h>

#include "partutil.h"


int
openspecial(const char *special, int flags, char *device, size_t devsize,
    struct stat *sb)
{
	char specname[MAXPATHLEN], rawname[MAXPATHLEN];
	const char *raw;
	struct stat st;
	struct fstab *fs;
	int fd;

	fs = getfsfile(special);
	if (fs)
		special = fs->fs_spec;

	raw = getfsspecname(specname, sizeof(specname), special);
	if (raw == NULL)
		err(EXIT_FAILURE, "%s: %s", special, specname);

	special = getdiskrawname(rawname, sizeof(rawname), raw);
	if (special == NULL)
		special = raw;

	fd = opendisk(special, flags, device, devsize, 0);
	if (sb == NULL)
		sb = &st;

	if (fd == -1 || fstat(fd, sb) == -1)
		err(EXIT_FAILURE, "Can't open `%s'", special);

	if (S_ISBLK(sb->st_mode))
		errx(EXIT_FAILURE, "`%s' is a block device. use raw device",
		    special);

	return fd;
}
