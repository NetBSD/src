/*	$NetBSD: boot.c,v 1.2 2000/07/29 20:06:29 jdolecek Exp $	*/

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
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>

#include <pc532/stand/common/samachdep.h>

void		main __P((void));
static void	getbootdev __P((int *));

/*
 * Boot program... bits in `howto' determine whether boot stops to
 * ask for system name.	 Boot device is derived from ROM provided
 * information.
 */

static char	line[100];

extern int	noconsole;

/*
 * XXX UFS accepts a /, NFS doesn't.
 */
char *name;
char *names[] = {
	"netbsd",		"netbsd.gz",
	"netbsd.bak",		"netbsd.bak.gz",
	"netbsd.old",		"netbsd.old.gz",
	"onetbsd",		"onetbsd.gz",
	NULL
};
#define NUMNAMES	(sizeof(names) / sizeof(char *))

static int bdev, badapt, bctlr, bunit, bpart;

void
main()
{
	int currname = 0;

	printf("\n");
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);  
	printf(">> (%s, %s)\n", bootprog_maker, bootprog_date);

	bdev   = B_TYPE(bootdev);
	badapt = B_ADAPTOR(bootdev);
	bctlr  = B_CONTROLLER(bootdev);
	bunit  = B_UNIT(bootdev);
	bpart  = B_PARTITION(bootdev);

	for (;;) {
		name = names[currname++];
		if (currname == NUMNAMES)
			currname = 0;

		if (!noconsole) {
			howto = 0;
			getbootdev(&howto);
		} else
			printf(": %s\n", name);

		exec(name, (char *)0x2000, howto);
		printf("boot: %s\n", strerror(errno));
	}
}

void
getbootdev(howto)
	int *howto;
{
	char c, *ptr = line;

	printf("Boot: [[[%s%d%c:]%s][-abds]] :- ",
	    devsw[bdev].dv_name, bctlr + (8 * badapt), 'a' + bpart, name);

	if (tgets(line)) {
		while ((c = *ptr) != 0) {
			while (c == ' ')
				c = *++ptr;
			if (!c)
				return;
			if (c == '-')
				while ((c = *++ptr) && c != ' ')
					switch (c) {
					case 'a':
						*howto |= RB_ASKNAME;
						continue;
					case 'b':
						*howto |= RB_HALT;
						continue;
					case 'd':
						*howto |= RB_KDB;
						continue;
					case 's':
						*howto |= RB_SINGLE;
						continue;
					}
			else {
				name = ptr;
				while ((c = *++ptr) && c != ' ');
				if (c)
					*ptr++ = 0;
			}
		}
	} else
		printf("\n");
}
