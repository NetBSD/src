/*	$NetBSD: exec.c,v 1.32 2008/10/08 22:57:28 joerg Exp $	 */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * starts NetBSD a.out kernel
 * needs lowlevel startup from startprog.S
 * This is a special version of exec.c to support use of XMS.
 */

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "loadfile.h"
#include "libi386.h"
#include "bootinfo.h"
#include "bootmod.h"
#ifdef SUPPORT_PS2
#include "biosmca.h"
#endif

#define BOOT_NARGS	6

#ifndef	PAGE_SIZE
#define	PAGE_SIZE	4096
#endif

extern struct btinfo_console btinfo_console;

boot_module_t *boot_modules;
bool boot_modules_enabled = true;
bool kernel_loaded;

static struct btinfo_modulelist *btinfo_modulelist;
static size_t btinfo_modulelist_size;
static uint32_t image_end;
static char module_base[64] = "/";

static void	module_init(void);

static int
common_load_kernel(const char *file, u_long *basemem, u_long *extmem,
    physaddr_t loadaddr, int floppy, u_long marks[MARK_MAX])
{
	int fd;
#ifdef XMS
	u_long		xmsmem;
	physaddr_t	origaddr = loadaddr;
#endif

	*extmem = getextmem();
	*basemem = getbasemem();

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
		if (xmsmem > *extmem)
			*extmem = xmsmem;
		/*
		 * Get the size of the kernel
		 */
		marks[MARK_START] = loadaddr;
		if ((fd = loadfile(file, marks, COUNT_KERNEL)) == -1)
			return EIO;
		close(fd);

		kernsize = marks[MARK_END];
		kernsize = (kernsize + 1023) / 1024;

		loadaddr = xmsalloc(kernsize);
		if (!loadaddr)
			return ENOMEM;
	}
#endif
	marks[MARK_START] = loadaddr;
	if ((fd = loadfile(file, marks,
	    LOAD_KERNEL & ~(floppy ? LOAD_NOTE : 0))) == -1)
		return EIO;

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
		 * buffer to its real address.
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
	image_end = marks[MARK_END];
	kernel_loaded = true;

	return 0;
}

int
exec_netbsd(const char *file, physaddr_t loadaddr, int boothowto, int floppy)
{
	u_long          boot_argv[BOOT_NARGS];
	u_long		marks[MARK_MAX];
	struct btinfo_symtab btinfo_symtab;
	u_long		extmem;
	u_long		basemem;

#ifdef	DEBUG
	printf("exec: file=%s loadaddr=0x%lx\n",
	       file ? file : "NULL", loadaddr);
#endif

	BI_ALLOC(32); /* ??? */

	BI_ADD(&btinfo_console, BTINFO_CONSOLE, sizeof(struct btinfo_console));

	if (common_load_kernel(file, &basemem, &extmem, loadaddr, floppy, marks))
		goto out;

	boot_argv[0] = boothowto;
	boot_argv[1] = 0;
	boot_argv[2] = vtophys(bootinfo);	/* old cyl offset */
	boot_argv[3] = marks[MARK_END];
	boot_argv[4] = extmem;
	boot_argv[5] = basemem;

	/* pull in any modules if necessary */
	if (boot_modules_enabled) {
		module_init();
		if (btinfo_modulelist) {
			BI_ADD(btinfo_modulelist, BTINFO_MODULELIST,
			    btinfo_modulelist_size);
		}
	}

#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n", marks[MARK_ENTRY],
	    marks[MARK_NSYM], marks[MARK_SYM], marks[MARK_END]);
#endif

	btinfo_symtab.nsym = marks[MARK_NSYM];
	btinfo_symtab.ssym = marks[MARK_SYM];
	btinfo_symtab.esym = marks[MARK_END];
	BI_ADD(&btinfo_symtab, BTINFO_SYMTAB, sizeof(struct btinfo_symtab));

	startprog(marks[MARK_ENTRY], BOOT_NARGS, boot_argv,
		  x86_trunc_page(basemem*1024));
	panic("exec returned");

out:
	BI_FREE();
	bootinfo = 0;
	return -1;
}

static const char *
module_path(boot_module_t *bm)
{
	static char buf[256];
	const char *name;

	name = bm->bm_path;
	if (name[0] == '/')
		return name;
	snprintf(buf, sizeof(buf), "%s/%s/%s.kmod", module_base, bm->bm_path,
	    bm->bm_path);
	return buf;
}

static int 
module_open(boot_module_t *bm, int mode)
{
	int fd;
	const char *path;
		
	/* check the expanded path first */
	path = module_path(bm);
	fd = open(path, mode);
	if (fd == -1) {
		printf("WARNING: couldn't open %s\n", path);
		/* now attempt the raw path provided */
		fd = open(bm->bm_path, mode);
		if (fd == -1)
			printf("WARNING: couldn't open %s\n", bm->bm_path);
	}
	return fd;
}

static void
module_init(void)
{
	struct bi_modulelist_entry *bi;
	struct stat st;
	const char *machine;
	char *buf;
	boot_module_t *bm;
	size_t len;
	off_t off;
	int err, fd;

	switch (netbsd_elf_class) {
	case ELFCLASS32:
		machine = "i386";
		break;
	case ELFCLASS64:
		machine = "amd64";
		break;
	default:
		machine = "generic";
		break;
	}
	if (netbsd_version / 1000000 % 100 == 99) {
		/* -current */
		snprintf(module_base, sizeof(module_base),
		    "/stand/%s/%d.%d.%d/modules", machine,
		    netbsd_version / 100000000,
		    netbsd_version / 1000000 % 100,
		    netbsd_version / 100 % 100);
	} else if (netbsd_version != 0) {
		/* release */
		snprintf(module_base, sizeof(module_base),
		    "/stand/%s/%d.%d/modules", machine,
		    netbsd_version / 100000000,
		    netbsd_version / 1000000 % 100);
	}

	/* First, see which modules are valid and calculate btinfo size */
	len = sizeof(struct btinfo_modulelist);
	for (bm = boot_modules; bm; bm = bm->bm_next) {
		fd = module_open(bm, 0);
		if (fd == -1) {
			bm->bm_len = -1;
			continue;
		}
		err = fstat(fd, &st);
		if (err == -1 || st.st_size == -1) {
			printf("WARNING: couldn't stat %s\n", bm->bm_path);
			close(fd);
			bm->bm_len = -1;
			continue;
		}
		bm->bm_len = st.st_size;
		close(fd);
		len += sizeof(struct bi_modulelist_entry);
	}

	/* Allocate the module list */
	btinfo_modulelist = alloc(len);
	if (btinfo_modulelist == NULL) {
		printf("WARNING: couldn't allocate module list\n");
		return;
	}
	memset(btinfo_modulelist, 0, len);
	btinfo_modulelist_size = len;

	/* Fill in btinfo structure */
	buf = (char *)btinfo_modulelist;
	btinfo_modulelist->num = 0;
	off = sizeof(struct btinfo_modulelist);

	for (bm = boot_modules; bm; bm = bm->bm_next) {
		if (bm->bm_len == -1)
			continue;
		printf("Loading %s ", bm->bm_path);
		fd = module_open(bm, 0);
		if (fd == -1) {
			printf("ERROR: couldn't open %s\n", bm->bm_path);
			continue;
		}
		image_end = (image_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
		len = pread(fd, (void *)image_end, SSIZE_MAX);
		if (len < bm->bm_len) {
			printf(" FAILED\n");
		} else {
			btinfo_modulelist->num++;
			bi = (struct bi_modulelist_entry *)(buf + off);
			off += sizeof(struct bi_modulelist_entry);
			strncpy(bi->path, bm->bm_path, sizeof(bi->path) - 1);
			bi->base = image_end;
			bi->len = len;
			bi->type = BI_MODULE_ELF;
			printf(" \n");
		}
		if (len > 0)
			image_end += len;
		close(fd);
	}
	btinfo_modulelist->endpa = image_end;
}
