/*	$NetBSD: boot.c,v 1.4 1998/06/29 20:17:03 gwr Exp $ */

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

#define XX "net"

/*
 * If the kernel name was not specified, try the extended name,
 * and if that is not found, try the default name, and if that
 * is not found, ask the user for help.
 */
char	defname[32] = "netbsd";
char	extname[32] = "netbsd.sun3";
char	line[80];

main()
{
	struct open_file	f;
	char *cp, *file;
	char *entry;
	int	io, x;

	printf(">> NetBSD " XX "boot [%s]\n", version);
	prom_get_boot_info();

	/*
	 * Hold the raw device open so it will not be
	 * closed and reopened on every attempt to
	 * load files that did not exist.
	 */
	f.f_flags = F_RAW;
	if (devopen(&f, 0, &file)) {
		printf(XX "boot: devopen failed\n");
		return;
	}

	/* If running on a Sun3X, append an x. */
	if (_is3x)
		extname[11] = 'x';
	file = extname;

	/* If the PROM gave us a file name, use it. */
	cp = prom_bootfile;
	if (cp && *cp)
		file = cp;

	for (;;) {
		if (prom_boothow & RB_ASKNAME) {
			printf("filename? [%s]: ", defname);
			gets(line);
			if (line[0])
				file = line;
			else
				file = defname;
		}

#ifdef DEBUG
		printf(XX "boot: trying \"%s\"\n", file);
#endif

		/* Can we open the file? */
		io = open(file, 0);
		if (io < 0) {
			/*
			 * Failed to open the file.  If we were
			 * trying the extended name (first time)
			 * then quietly retry with the plain name.
			 */
			if (file == extname) {
				file = defname;
				continue;
			}
			goto err;
		}

		/* The open succeeded.  Try loading. */
		if (file != line)
			printf(XX "boot: loading %s\n", file);
		x = load_sun(io, (char *)LOADADDR, &entry);
		close(io);
		if (x == 0)
			break;

	err:
		printf(XX "boot: %s: %s\n", file, strerror(errno));
		prom_boothow |= RB_ASKNAME;
	}

	/* Do the "last close" on the underlying device. */
	f.f_dev->dv_close(&f);

	printf("Starting program at 0x%x\n", entry);
	chain_to((void*)entry);
}
