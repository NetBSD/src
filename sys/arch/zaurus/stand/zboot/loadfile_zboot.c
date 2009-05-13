/*	$NetBSD: loadfile_zboot.c,v 1.1.6.2 2009/05/13 17:18:55 jym Exp $	*/

/*
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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

#include <sys/types.h>
#include <sys/bootblock.h>

#include "boot.h"
#include "bootinfo.h"
#include "disk.h"
#include "unixdev.h"
#include "pathnames.h"

#include <lib/libsa/loadfile.h>

#include "compat_linux.h"

static int fdloadfile_zboot(int fd, u_long *marks, int flags);
static int zboot_exec(int fd, u_long *marks, int flags);

int
loadfile_zboot(const char *fname, u_long *marks, int flags)
{
	int fd, error;

	/* Open the file. */
	if ((fd = open(fname, 0)) < 0) {
		WARN(("open %s", fname ? fname : "<default>"));
		return -1;
	}

	/* Load it; save the value of errno across the close() call */
	if ((error = fdloadfile_zboot(fd, marks, flags)) != 0) {
		(void)close(fd);
		errno = error;
		return -1;
	}

	return fd;
}

static int
fdloadfile_zboot(int fd, u_long *marks, int flags)
{
	Elf32_Ehdr elf32;
	ssize_t nr;
	int rval;

	/* Read the exec header. */
	if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
		goto err;
	nr = read(fd, &elf32, sizeof(elf32));
	if (nr == -1) {
		WARN(("read header failed"));
		goto err;
	}
	if (nr != sizeof(elf32)) {
		WARN(("read header short"));
		errno = EFTYPE;
		goto err;
	}

	if (memcmp(elf32.e_ident, ELFMAG, SELFMAG) == 0 &&
	    elf32.e_ident[EI_CLASS] == ELFCLASS32) {
		rval = zboot_exec(fd, marks, flags);
	} else {
		rval = 1;
		errno = EFTYPE;
	}

	if (rval == 0)
		return 0;
err:
	return errno;
}

static int
zboot_exec(int fd, u_long *marks, int flags)
{
	static char bibuf[BOOTINFO_MAXSIZE];
	char buf[512];
	struct btinfo_common *help;
	char *p;
	int tofd;
	int sz;
	int i;

	/*
	 * set bootargs
	 */
	p = bibuf;
	memcpy(p, &bootinfo->nentries, sizeof(bootinfo->nentries));
	p += sizeof(bootinfo->nentries);
	for (i = 0; i < bootinfo->nentries; i++) {
		help = (struct btinfo_common *)(bootinfo->entry[i]);
		if ((p - bibuf) + help->len > BOOTINFO_MAXSIZE)
			break;

		memcpy(p, help, help->len);
		p += help->len;
	}

	tofd = uopen(_PATH_ZBOOT, LINUX_O_WRONLY);
	if (tofd == -1) {
		printf("%s: can't open (errno %d)\n", _PATH_ZBOOT, errno);
		return 1;
	}

	if (uwrite(tofd, bibuf, p - bibuf) != p - bibuf)
		printf("setbootargs: argument write error\n");

	/* Commit boot arguments. */
	uclose(tofd);

	/*
	 * load kernel
	 */
	tofd = uopen(_PATH_ZBOOT, LINUX_O_WRONLY);
	if (tofd == -1) {
		printf("%s: can't open (errno %d)\n", _PATH_ZBOOT, errno);
		return 1;
	}

	if (lseek(fd, 0, SEEK_SET) != 0) {
		printf("%s: seek error\n", _PATH_ZBOOT);
		goto err;
	}

	while ((sz = read(fd, buf, sizeof(buf))) == sizeof(buf)) {
		if ((sz = uwrite(tofd, buf, sz)) != sizeof(buf)) {
			printf("%s: write error\n", _PATH_ZBOOT);
			goto err;
		}
	}

	if (sz < 0) {
		printf("zboot_exec: read error\n");
		goto err;
	}

	if (sz >= 0 && uwrite(tofd, buf, sz) != sz) {
		printf("zboot_exec: write error\n");
		goto err;
	}

	uclose(tofd);
	/*NOTREACHED*/
	return 0;

err:
	uclose(tofd);
	return 1;
}
