/*	$NetBSD: exec.c,v 1.16 2001/05/19 18:15:14 jdolecek Exp $	 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996
 * 	Perry E. Metzger.  All rights reserved.
 * Copyright (c) 1997
 * 	Martin Husemann.  All rights reserved.
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
 * starts NetBSD a.out kernel
 * needs lowlevel startup from startprog.S
 * This is a special version of exec.c to support use of XMS.
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
#ifdef SUPPORT_PS2
#include "biosmca.h"
#endif

#ifdef COMPAT_OLDBOOT
static int dev2major __P((char *, int *));

static int
dev2major(devname, major)
	char           *devname;
	int            *major;
{
	static const struct {
		const char *name;
		int maj;
	} devices[] = {
		{ "wd", 0  },
		{ "fd", 2  },
		{ "sd", 4  },
#ifdef SUPPORT_PS2
		{ "ed", 20 },
#endif
	};
#define NUMDEVICES (sizeof(devices)/sizeof(devices[0]))
	int             i;

	for (i = 0; i < NUMDEVICES; i++)
		if (!strcmp(devname, devices[i].name)) {
			*major = devices[i].maj;
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
	u_long		marks[MARK_MAX];
	struct btinfo_symtab btinfo_symtab;
	u_long		extmem;
	u_long		basemem;
#ifdef XMS
	u_long		xmsmem;
	physaddr_t	origaddr = loadaddr;
#endif

#ifdef COMPAT_OLDBOOT
	char           *fsname, *devname;
	int             unit, part;
	const char     *filename;
	int             bootdevnr;
#endif

#ifdef	DEBUG
	printf("exec: file=%s loadaddr=0x%lx\n",
	       file ? file : "NULL", loadaddr);
#endif

	BI_ALLOC(6); /* ??? */

	BI_ADD(&btinfo_console, BTINFO_CONSOLE, sizeof(struct btinfo_console));

	extmem = getextmem();
	basemem = getbasemem();

#ifdef XMS
	if ((getextmem1() == 0) && (xmsmem = checkxms())) {
	        u_long kernsize;

		/*
		 * With "CONSERVATIVE_MEMDETECT", extmem is 0 because
		 *  getextmem() is getextmem1(). Without, the "smart"
		 *  methods could fail to report all memory as well.
		 * xmsmem is a few kB less than the actual size, but
		 *  better than nothing.
		 */
		if (xmsmem > extmem)
			extmem = xmsmem;
		/* 
		 * Get the size of the kernel
		 */
		marks[MARK_START] = loadaddr;
		if ((fd = loadfile(file, marks, COUNT_KERNEL)) == -1)
			goto out;
		close(fd);

		kernsize = marks[MARK_END];
		kernsize = (kernsize + 1023) / 1024;

		loadaddr = xmsalloc(kernsize);
		if (!loadaddr)
			return(ENOMEM);
	}
#endif
	marks[MARK_START] = loadaddr;
	if ((fd = loadfile(file, marks, LOAD_KERNEL)) == -1)
		goto out;

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

			switch (biosdisk_gettype(f)) {
			case DTYPE_SCSI:
				devname = "sd";
				break;
#ifdef SUPPORT_PS2
			case DTYPE_ESDI:
				if (biosmca_ps2model) {
					devname = "ed";
					break;
				}
#endif
			default:
				devname = "wd";
				break;
			}

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
	boot_argv[2] = vtophys(bootinfo);	/* old cyl offset */
	/* argv[3] below */
	boot_argv[4] = extmem;
	boot_argv[5] = basemem;

	close(fd);

	/*
	 * Gather some information for the kernel. Do this after the
	 * "point of no return" to avoid memory leaks.
	 * (but before DOS might be trashed in the XMS case)
	 */
#ifdef PASS_BIOSGEOM
	bi_getbiosgeom();
#endif
#ifdef PASS_MEMMAP
	bi_getmemmap();
#endif

#ifdef XMS
	if (loadaddr != origaddr) {
		/*
		 * We now have done our last DOS IO, so we may
		 * trash the OS. Copy the data from the temporary
		 * buffer to its real adress.
		 */
		marks[MARK_START] -= loadaddr;
		marks[MARK_END] -= loadaddr;
		marks[MARK_SYM] -= loadaddr;
		marks[MARK_END] -= loadaddr;
		ppbcopy(loadaddr, origaddr, marks[MARK_END]);
	}
#endif
	marks[MARK_END] = (((u_long) marks[MARK_END] + sizeof(int) - 1)) &
	    (-sizeof(int));

	boot_argv[3] = marks[MARK_END];


#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n", marks[MARK_ENTRY],
	    marks[MARK_NSYM], marks[MARK_SYM], marks[MARK_END]);
#endif

	btinfo_symtab.nsym = marks[MARK_NSYM];
	btinfo_symtab.ssym = marks[MARK_SYM];
	btinfo_symtab.esym = marks[MARK_END];
	BI_ADD(&btinfo_symtab, BTINFO_SYMTAB, sizeof(struct btinfo_symtab));

	startprog(marks[MARK_ENTRY], BOOT_NARGS, boot_argv,
		i386_trunc_page(basemem*1024));
	panic("exec returned");

out:
	BI_FREE();
	bootinfo = 0;
	return (-1);
}
