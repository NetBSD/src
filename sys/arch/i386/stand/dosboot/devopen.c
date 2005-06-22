/*	$NetBSD: devopen.c,v 1.8 2005/06/22 20:36:17 junyoung Exp $	 */

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


#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/ufs.h>

#include <libi386.h>
#include <biosdisk.h>
#include <dosfile.h>
#include <bootinfo.h>

struct devsw devsw[] = {
	{"disk", biosdisk_strategy, biosdisk_open, biosdisk_close, biosdisk_ioctl},
};
int ndevs = sizeof(devsw) / sizeof(struct devsw);

static struct fs_ops dosfs = {
	dos_open, dos_close, dos_read, dos_write, dos_seek, dos_stat
};
static struct fs_ops ufsfs = {
	ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat
};

struct fs_ops   file_system[] = {
	{0},
};
int  nfsys = 1;

static struct {
	char           *name;
	int             biosdev;
}               biosdevtab[] = {
	{
		"fd", 0
	},
	{
		"wd", 0x80
	},
	{
		"sd", 0x80
	},
	{
		"hd", 0x80
	}
};
#define NUMBIOSDEVS (sizeof(biosdevtab) / sizeof(biosdevtab[0]))

static int dev2bios __P((char *, unsigned int, int *));

static int
dev2bios(devname, unit, biosdev)
	char           *devname;
	unsigned int    unit;
	int            *biosdev;
{
	unsigned             i;

	for (i = 0; i < NUMBIOSDEVS; i++)
		if (!strcmp(devname, biosdevtab[i].name)) {
			*biosdev = biosdevtab[i].biosdev + unit;
			if (unit >= 4)	/* ??? */
				return (EUNIT);
			return (0);
		}
	return (ENXIO);
}

struct btinfo_bootpath bibp;

int
devopen(f, fname, file)
	struct open_file *f;
	const char     *fname;
	char          **file;
{
	char           *devname;
	char           *fsmode;
	unsigned int    unit, partition;
	int             biosdev;
	int             error;
	struct devsw   *dp;

	if ((error = parsebootfile(fname, &fsmode, &devname, &unit,
	    &partition, (const char **) file)))
		return (error);

	if (!strcmp(fsmode, "dos")) {
		file_system[0] = dosfs;	/* structure assignment! */
		f->f_flags |= F_NODEV;	/* handled by DOS */
		return (0);
	} else if (!strcmp(fsmode, "ufs")) {
		if ((error = dev2bios(devname, unit, &biosdev)))
			return (error);
		file_system[0] = ufsfs;	/* structure assignment! */
		dp = &devsw[0];	/* must be biosdisk */
		f->f_dev = dp;

		strncpy(bibp.bootpath, *file, sizeof(bibp.bootpath));
		BI_ADD(&bibp, BTINFO_BOOTPATH, sizeof(bibp));

		return (biosdisk_open(f, biosdev, partition));
	} else {
		printf("no file system\n");
		return (ENXIO);
	}
	/* NOTREACHED */
}
