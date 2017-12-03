/*	$NetBSD: xxboot.c,v 1.6.24.1 2017/12/03 11:36:46 jdolecek Exp $ */

/*-
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

#include <sys/param.h>
#include <sys/reboot.h>
#include <machine/mon.h>

#include <stand.h>
#include <loadfile.h>
#include "libsa.h"

/*
 * Note that extname is edited based on the running machine type
 * (sun3 vs sun3x).  EXTNAMEX is the position of the 'x'.
 */
char	extname[] = "netbsd.sun3x";
#define EXTNAMEX (sizeof(extname)-2)

/*
 * If the PROM did not give us a specific kernel name to use,
 * and did not specify the -a flag (ask), then try the names
 * in the following list.
 */
char *kernelnames[] = {
	"netbsd",
	"netbsd.old",
	extname,
	NULL
};
char	line[80];

void
xxboot_main(const char *boot_type)
{
	struct open_file f;
	char **npp;
	char *file;
	void *entry;
	int fd;
	u_long marks[MARK_MAX];

	memset(marks, 0, sizeof(marks));
	if (_is2)
		marks[MARK_START] = sun2_map_mem_load();
	printf(">> %s %s [%s]\n", bootprog_name, boot_type, bootprog_rev);
	prom_get_boot_info();

	/*
	 * Hold the raw device open so it will not be
	 * closed and reopened on every attempt to
	 * load files that did not exist.
	 */
	f.f_flags = F_RAW;
	if (devopen(&f, 0, &file)) {
		printf("%s: devopen failed\n", boot_type);
		return;
	}

	/*
	 * Edit the "extended" kernel name based on
	 * the type of machine we are running on.
	 */
	if (_is2)
		extname[EXTNAMEX - 1] = '2';
	if (_is3x == 0)
		extname[EXTNAMEX] = 0;

	/* If we got the "-a" flag, ask for the name. */
	if (prom_boothow & RB_ASKNAME)
		goto just_ask;

	/*
	 * If the PROM gave us a file name,
	 * it means the user asked for that
	 * kernel name explicitly.
	 */
	file = prom_bootfile;
	if (file && *file) {
		fd = loadfile(file, marks, LOAD_KERNEL);
		if (fd == -1) {
			goto err;
		} else {
			goto gotit;
		}
	}

	/*
	 * Try the default kernel names.
	 */
	for (npp = kernelnames; *npp; npp++) {
		file = *npp;
		printf("%s: trying %s\n", boot_type, file);
		fd = loadfile(file, marks, LOAD_KERNEL);
		if (fd != -1)
			goto gotit;
	}

	/*
	 * Ask what kernel name to load.
	 */
	for (;;) {

	just_ask:
		file = kernelnames[0];
		printf("filename? [%s]: ", file);
		kgets(line, sizeof(line));
		if (line[0])
			file = line;

		fd = loadfile(file, marks, LOAD_KERNEL);
		if (fd != -1)
			break;

	err:
		printf("%s: %s: loadfile() failed.\n", boot_type, file);
	}

gotit:
	entry = (void *)marks[MARK_ENTRY];
	if (_is2) {
		printf("relocating program...");
		entry = sun2_map_mem_run(entry);
	}
	printf("starting program at 0x%x\n", (u_int)entry);
	chain_to(entry);
}
