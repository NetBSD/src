/*	$NetBSD: fsconsole.c,v 1.6.2.2 2007/09/03 10:23:52 skrll Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Finnish Cultural Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * If you can keep a secret: THIS IS NOT FINISHED AT ALL
 */

#include <sys/types.h>
#include <dirent.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ukfs.h"

struct fscons_args {
	char *fspec;
};

int
main(int argc, char *argv[])
{
	uint8_t buf[8192];
	struct ukfs *fs;
	struct fscons_args args;
#ifdef __NetBSD__
	struct dirent *dent;
#endif
	char *p;
	int rv;

	ukfs_init();

	if (argc != 3) {
		fprintf(stderr, "usage: fsconsole fstype fsdevice\n");
		exit(1);
	}

	/* XXXXXXXXXXX: this needs fs-specific handling */
	memset(&args, 0, sizeof(args));
	args.fspec = strdup(argv[2]);
	fs = ukfs_mount(argv[1], argv[2], "/", 0, &args, sizeof(args));

	printf("got fs at %p\n", fs);

	rv = ukfs_getdents(fs, "/", 0, buf, sizeof(buf));
	printf("rv %d\n", rv);
	if (rv == -1)
		rv = 0;

#ifdef __NetBSD__
	dent = (void *)buf;
	while (rv) {
		printf("%s\n", dent->d_name);
		rv -= _DIRENT_SIZE(dent);
		dent = _DIRENT_NEXT(dent);
	}
#endif

	rv = ukfs_read(fs, "/etc/passwd", 0, buf, sizeof(buf));
	printf("rv %d\n%s\n", rv, buf);

	rv = ukfs_remove(fs, "/lopinaa");
	if (rv == -1)
		warn("rv %d", rv);

#if 0
	rv = ukfs_remove(fs, "/etc/LUSIKKAPUOLI");
	if (rv == -1)
		warn("rv %d", rv);
#endif

	rv = ukfs_create(fs, "/lopinaa", 0555);
	if (rv == -1)
		warn("rv %d", rv);

	rv = ukfs_link(fs, "/lopinaa", "/etc/LUSIKKAPUOLI");
	if (rv == -1)
		warn("link rv %d", rv);

	/* XXX */
	p = strdup("jonnekin/aivan/muualle");
	rv = ukfs_symlink(fs, "/mihis_haluat_menna_tanaan", p);
	free(p);
	if (rv == -1)
		warn("symlink %d", rv);

	rv = ukfs_readlink(fs, "/mihis_haluat_menna_tanaan",
	    (char *)buf, sizeof(buf));
	if (rv > 0)
		buf[rv] = '\0';
	printf("readlink rv %d result \"%s\"\n", rv, buf);

	rv = ukfs_getdents(fs, "/etc", 0, buf, sizeof(buf));
	printf("rv %d\n", rv);
	if (rv == -1)
		rv = 0;

#ifdef __NetBSD__
	dent = (void *)buf;
	while (rv) {
		printf("%s\n", dent->d_name);
		rv -= _DIRENT_SIZE(dent);
		dent = _DIRENT_NEXT(dent);
	}
#endif

	ukfs_release(fs, 1);
}
