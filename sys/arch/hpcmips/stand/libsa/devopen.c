/*	$NetBSD: devopen.c,v 1.6.22.1 2017/12/03 11:36:15 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
#define STANDALONE_WINDOWS_SIDE
#include <stand.h>
#include <winblk.h>
#include <winfs.h>
#include <lib/libsa/ufs.h>

extern int parsebootfile(const char *, char**, char**, unsigned int*,
                              unsigned int*, const char**);

struct devsw devsw[] = {
	{"winblk", winblkstrategy, winblkopen, winblkclose, winblkioctl },
};
int ndevs = sizeof(devsw) / sizeof(struct devsw);

static struct fs_ops winop = {
        win_open, win_close, win_read, win_write, win_seek, win_stat
};

static struct fs_ops ufsop = {
        ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat
};

struct fs_ops   file_system[] = {
	{ 0 },
};
int nfsys = 1;

int
parsebootfile(const char *fnamexx, char **fsmode, char **devname, unsigned int *unit, unsigned int *partition, const char **file)
	/* fsmode:  out */
	/* devname:  out */
	/* unit, *partition:  out */
	/* file:  out */
{
	TCHAR *fname = (TCHAR*)fnamexx;

	if (fname[0] == TEXT('\\')) {
	        *fsmode = "win";
	        *devname = "";
	        *unit = 0;
	        *partition = 0;
		*file = fname;
	} else {
		static char name[1024]; /* XXX */

		if (wcstombs(name, (TCHAR*)fname, sizeof(name)) == (size_t)-1) {
			return (ENOENT);
		}
		if ('1' <= name[0] && name[0] <= '9' && name[1] == ':') {
			*unit = name[0] - '0';
			*file = &name[2];
		} else {
		        *unit = 1;
			*file = name;
		}
	        *fsmode = "ufs";
	        *devname = "DSK";
	        *partition = 0;
	}

	return (0);
}


int
devopen(struct open_file *f, const char *fname, char **file)
{
        char           *devname;
        char           *fsmode;
        unsigned int    unit, partition;
        int             error;

        if ((error = parsebootfile(fname, &fsmode, &devname, &unit,
            &partition, (const char **) file)))
                return (error);
	
        if (!strcmp(fsmode, "win")) {
                file_system[0] = winop;
                f->f_flags |= F_NODEV;  /* handled by Windows */
                return (0);
	} else
        if (!strcmp(fsmode, "ufs")) {
                file_system[0] = ufsop;
		f->f_dev = &devsw[0];	/* Windows block device. */
                return (*f->f_dev->dv_open)(f, devname, unit, partition);
	} else {
                printf("no file system\n");
                return (ENXIO);
	}
	/* NOTREACHED */
}
