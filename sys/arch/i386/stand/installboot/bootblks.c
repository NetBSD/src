/* $NetBSD: bootblks.c,v 1.4 1999/09/10 16:41:10 drochner Exp $ */

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
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
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "installboot.h"

static char bootblkpath[MAXPATHLEN];
static char backuppath[MAXPATHLEN];
static char backupext[] = ".bak";
static int havebackup;

ino_t
createfileondev(diskdev, bootblkname, bp, size)
	char *diskdev, *bootblkname;
	char *bp;
	unsigned int size;
{
	char *mntpoint;
	int fd = -1;
	struct stat statbuf;
	int allok = 0;

	if ((mntpoint = getmountpoint(diskdev)) == NULL)
		return ((ino_t) - 1);

	/*
	 * create file in fs root for bootloader data
	 * try to rename existing file before
	 */
	havebackup = 0;
	(void) sprintf(bootblkpath, "%s/%s", mntpoint, bootblkname);
	(void) sprintf(backuppath, "%s%s", bootblkpath, backupext);
	if (rename(bootblkpath, backuppath) == -1) {
		if (errno != ENOENT) {
			warn("rename old %s", bootblkpath);
			goto out;
		}
	} else {
		if (verbose)
			(void) fprintf(stderr, "renamed %s -> %s\n",
				       bootblkpath, backuppath);
		havebackup = 1;
	}
	fd = open(bootblkpath, O_RDWR | O_CREAT | O_EXCL, 0400);
	if (fd < 0) {
		warn("open %s", bootblkpath);
		goto out;
	}
	/*
	 * do the write, flush, get inode number
	 */
	if (write(fd, bp, size) < 0) {
		warn("write %s", bootblkpath);
		goto out;
	}
	if (fsync(fd) != 0) {
		warn("fsync: %s", bootblkpath);
		goto out;
	}
	if (fstat(fd, &statbuf) != 0) {
		warn("fstat: %s", bootblkpath);
		goto out;
	}
	allok = 1;

out:
	if (fd != -1) {
		(void) close(fd);
		if (!allok)
			(void) unlink(bootblkpath);
	}
        if (!allok && havebackup)
		(void) rename(backuppath, bootblkpath);
	cleanupmount(mntpoint);
	return (allok ? statbuf.st_ino : (ino_t) - 1);
}

void 
cleanupfileondev(diskdev, bootblkname, recover)
	char *diskdev, *bootblkname;
	int recover;
{
	char *mntpoint;

	/* save some work if nothing to do */
	if (!(recover || havebackup))
		return;

	if ((mntpoint = getmountpoint(diskdev)) == NULL)
		return;

	(void) sprintf(bootblkpath, "%s/%s", mntpoint, bootblkname);
	(void) sprintf(backuppath, "%s%s", bootblkpath, backupext);

	if (recover) {
		(void) unlink(bootblkpath);
		if (havebackup) {
			(void) fprintf(stderr, "renaming %s -> %s\n",
				       backuppath, bootblkpath);
			(void) rename(backuppath, bootblkpath);
		}
	} else if (havebackup) {
		if (verbose)
			(void) fprintf(stderr, "deleting %s\n", backuppath);
		(void) unlink(backuppath);
	}

	cleanupmount(mntpoint);
}
