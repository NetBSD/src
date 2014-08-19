/*	$NetBSD: boot.c,v 1.2.6.4 2014/08/20 00:03:10 tls Exp $	*/

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
#include <sys/boot_flag.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/status.h>
#include <lib/libsa/loadfile.h>

int
boot(int argc, char *argv[])
{
	char *line, *opts;
	int i, howto;
	char c;

	line = NULL;
	howto = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			opts = argv[i];
			while ((c = *++opts) && c != '\0')
				BOOT_FLAG(c, howto);
		} else if (line == NULL)
			line = argv[i];
	}
	if (line == NULL)
		line = default_file;

	printf("Booting %s", line);
	if (howto != 0)
		printf(" (howto 0x%x)", howto);
	printf("\n");

	return bootnetbsd(line, howto);
}

int
bootnetbsd(char *line, int howto)
{
	int io;
	u_long marks[MARK_MAX];

	/* Note marks[MARK_START] is passed as an load address offset */
	memset(marks, 0, sizeof(marks));

	io = loadfile(line, marks, LOAD_KERNEL);
	if (io >= 0) {
		int dev = 0, unit = 0, part = 0;
		uint adpt, ctlr, id;
		uint32_t bootdev;

		make_device(line, &dev, &unit, &part, NULL);
		adpt = dev2adpt[dev];
		ctlr = CTLR(unit);
		id   = TARGET(unit);
		bootdev = MAKEBOOTDEV(0, adpt, ctlr, id, part);
#ifdef DEBUG
		printf("entry = 0x%lx\n", marks[MARK_ENTRY]);
		printf("ssym  = 0x%lx\n", marks[MARK_SYM]);
		printf("esym  = 0x%lx\n", marks[MARK_END]);
#endif
		__asm volatile (
			"movl	%0,%%d7;"
			"movl	%1,%%d6;"
			"movl	%2,%%a0;"
			"jbsr	%%a0@"
			:
			: "g" (howto), "g" (bootdev),
			  "g" ((void *)marks[MARK_ENTRY])
			: "d6", "d7", "a0");
	}
	printf("Booting kernel failed. (%s)\n", strerror(errno));

	return ST_ERROR;
}
