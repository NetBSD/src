/*	$NetBSD: exec.c,v 1.6 1999/01/28 20:21:24 christos Exp $	 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

/*
 * starts NetBSD a.out kernel needs lowlevel startup from startprog.S
 */
#include <sys/param.h>
#include <sys/reboot.h>
#ifdef COMPAT_OLDBOOT
#include <sys/disklabel.h>
#endif

#include <lib/libsa/stand.h>

#include "loadfile.h"
#include "libi386.h"
#include "bootinfo.h"

#ifdef COMPAT_OLDBOOT
static int
dev2major(devname, major)
	char           *devname;
	int            *major;
{
	static char    *devices[] = {"wd", "", "fd", "", "sd"};
#define NUMDEVICES (sizeof(devices)/sizeof(char *))
	int             i;

	for (i = 0; i < NUMDEVICES; i++)
		if (!strcmp(devname, devices[i])) {
			*major = i;
			return (0);
		}
	return (-1);
}
#endif
#define BOOT_NARGS	6

extern struct btinfo_console btinfo_console;

int 
exec_netbsd(file, loadaddr, boothowto)
	const char     *file;
	physaddr_t      loadaddr;
	int             boothowto;
{
	u_long          boot_argv[BOOT_NARGS];
	int		fd;
#ifdef COMPAT_OLDBOOT
	char           *fsname, *devname;
	int             unit, part;
	const char     *filename;
	int             bootdevnr;
	u_long		marks[LOAD_MAX];
	struct btinfo_symtab btinfo_symtab;
#endif

#ifdef	DEBUG
	printf("exec: file=%s loadaddr=0x%lx\n", file, loadaddr);
#endif

	BI_ALLOC(6); /* ??? */

	BI_ADD(&btinfo_console, BTINFO_CONSOLE, sizeof(struct btinfo_console));

	marks[LOAD_START] = loadaddr;
	if ((fd = loadfile(file, marks)) == -1)
		goto out;

	marks[LOAD_END] = (((u_long) marks[LOAD_END] + sizeof(int) - 1)) &
	    (-sizeof(int));

	btinfo_symtab.nsym = marks[LOAD_NSYM];
	btinfo_symtab.ssym = marks[LOAD_SYM];
	btinfo_symtab.esym = marks[LOAD_END];
	BI_ADD(&btinfo_symtab, BTINFO_SYMTAB, sizeof(struct btinfo_symtab));

	boot_argv[0] = boothowto;

#ifdef COMPAT_OLDBOOT
	/* prepare boot device information for kernel */
	if (parsebootfile(file, &fsname, &devname, &unit, &part, &filename)
	    || strcmp(fsname, "ufs"))
		bootdevnr = 0;	/* XXX error out if parse error??? */
	else {
		int             major;

		if (strcmp(devname, "hd") == 0) {
			/* generic BIOS disk, have to guess type */
			struct open_file *f = &files[fd];	/* XXX */

			if (biosdisk_gettype(f) == DTYPE_SCSI)
				devname = "sd";
			else
				devname = "wd";

			/*
			 * The old boot block performed the following
			 * conversion:
			 *
			 *	hdN -> Xd0
			 *
			 * where X is the type specified by the label.
			 * We mimmick that here, for lack of any better
			 * way of doing things.
			 */
			unit = 0;
		}

		if (dev2major(devname, &major))
			bootdevnr = 0;	/* XXX error out??? */
		else
			bootdevnr = MAKEBOOTDEV(major, 0, 0, unit, part);
	}

	boot_argv[1] = bootdevnr;
#else
	boot_argv[1] = 0;
#endif
#ifdef PASS_BIOSGEOM
	bi_getbiosgeom();
#endif
	boot_argv[2] = vtophys(bootinfo);	/* old cyl offset */
	boot_argv[3] = marks[LOAD_END];
	boot_argv[4] = getextmem();
	boot_argv[5] = getbasemem();

	close(fd);

#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n", marks[LOAD_START],
	    marks[LOAD_NSYM], marks[LOAD_SYM], marks[LOAD_END]);
#endif

	startprog(marks[LOAD_START], BOOT_NARGS, boot_argv, 0x90000);
	panic("exec returned");

out:
	BI_FREE();
	bootinfo = 0;
	return (-1);
}
