/*	$NetBSD: xxboot.c,v 1.2 2000/07/16 21:56:14 jdolecek Exp $ */

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

#include <sys/param.h>
#include <sys/reboot.h>
#include <machine/mon.h>

#include <stand.h>
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
	struct open_file	f;
	char **npp;
	char *cp, *file;
	char *entry;
	int	io, x;

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
	cp = prom_bootfile;
	if (cp && *cp) {
		file = cp;
		goto try_open;
	}

	/*
	 * Try the default kernel names.
	 */
	for (npp = kernelnames; *npp; npp++) {
		file = *npp;
		printf("%s: trying %s\n", boot_type, file);
		if ((io = open(file, 0)) >= 0) {
			/* The open succeeded. */
			goto try_load;
		}
	}

	/*
	 * Ask what kernel name to load.
	 */
	for (;;) {

	just_ask:
		file = kernelnames[0];
		printf("filename? [%s]: ", file);
		gets(line);
		if (line[0])
			file = line;

	try_open:
		/* Can we open the file? */
		io = open(file, 0);
		if (io < 0)
			goto err;

	try_load:
		/* The open succeeded.  Try loading. */
		printf("%s: loading %s\n", boot_type, file);
		x = load_sun(io, (char *)LOADADDR, &entry);
		close(io);
		if (x == 0)
			break;

	err:
		printf("%s: %s: %s\n", boot_type, file, strerror(errno));
	}

	/* Do the "last close" on the underlying device. */
	f.f_dev->dv_close(&f);

	printf("Starting program at 0x%x\n", entry);
	chain_to((void*)entry);
}
