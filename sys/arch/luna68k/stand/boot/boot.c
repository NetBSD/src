/*	$NetBSD: boot.c,v 1.2.2.2 2013/01/23 00:05:53 yamt Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

/*
 * boot.c -- boot program
 * by A.Fujita, MAR-01-1992
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/stinger.h>
#include <luna68k/stand/boot/status.h>
#include <lib/libsa/loadfile.h>

int howto;

static int get_boot_device(const char *, int *, int *, int *);

struct exec header;
char default_file[] = "sd(0,0)netbsd";

char *how_to_info[] = {
	"RB_ASKNAME	ask for file name to reboot from",
	"RB_SINGLE	reboot to single user only",
	"RB_NOSYNC	dont sync before reboot",
	"RB_HALT	don't reboot, just halt",
	"RB_INITNAME	name given for /etc/init (unused)",
	"RB_DFLTROOT	use compiled-in rootdev",
	"RB_KDB		give control to kernel debugger",
	"RB_RDONLY	mount root fs read-only"
};

int
how_to_boot(int argc, char *argv[])
{
	int i, h = howto;

	if (argc < 2) {
		printf("howto: 0x%s\n\n", hexstr(howto, 2));

		if (h == 0) {
			printf("\t%s\n", "RB_AUTOBOOT	flags for system auto-booting itself");
		} else {
			for (i = 0; i < 8; i++, h >>= 1) {
				if (h & 0x01) {
					printf("\t%s\n", how_to_info[i]);
				}
			}
		}

		printf("\n");
	}
	return ST_NORMAL;
}

int
get_boot_device(const char *s, int *devp, int *unitp, int *partp)
{
	const char *p = s;
	int unit, part;

	while (*p != '(') {
		if (*p == '\0')
			goto error;
		p++;
	}

	while (*++p != ',') {
		if (*p == '\0')
			goto error;
		if (*p >= '0' && *p <= '9')
			unit = (unit * 10) + (*p - '0');
	}

	while (*++p != ')') {
		if (*p == '\0')
			goto error;
		if (*p >= '0' && *p <= '9')
			part = (part * 10) + (*p - '0');
	}

	*devp  = 0;	/* XXX not yet */
	*unitp = unit;	/* XXX should pass SCSI ID, not logical unit number */
	*partp = part;

	return 0;

error:
	return -1;
}

int
boot(int argc, char *argv[])
{
	char *line;

	if (argc < 2)
		line = default_file;
	else
		line = argv[1];

	printf("Booting %s\n", line);

	return bootnetbsd(line);
}

int
bootnetbsd(char *line)
{
	int io;
	int dev, unit, part;
	u_long marks[MARK_MAX];
	void (*entry)(void);

	if (get_boot_device(line, &dev, &unit, &part) != 0) {
		printf("Bad file name %s\n", line);
		return ST_ERROR;
	}

	/* Note marks[MARK_START] is passed as an load address offset */
	memset(marks, 0, sizeof(marks));

	io = loadfile(line, marks, LOAD_KERNEL);
	if (io >= 0) {
#ifdef DEBUG
		printf("entry = 0x%lx\n", marks[MARK_ENTRY]);
		printf("ssym  = 0x%lx\n", marks[MARK_SYM]);
		printf("esym  = 0x%lx\n", marks[MARK_END]);
#endif

		/*
		 * XXX TODO: fill bootinfo about symbols, boot device etc.
		 */

		entry = (void *)marks[MARK_ENTRY];

		(*entry)();
	}
	printf("Booting kernel failed. (%s)\n", strerror(errno));

	return ST_ERROR;
}
