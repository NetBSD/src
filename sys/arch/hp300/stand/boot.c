/*	$NetBSD: boot.c,v 1.5 1995/02/21 06:39:01 mycroft Exp $	*/

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
#include <a.out.h>
#include "stand.h"

#ifndef INSECURE
#include <sys/stat.h>
struct stat sb;
#endif

/*
 * Boot program... bits in `howto' determine whether boot stops to
 * ask for system name.	 Boot device is derived from ROM provided
 * information.
 */

char line[100];
char *ssym, *esym;

extern	unsigned opendev;
extern	char *lowram;
extern	int noconsole;
extern	int howto, bootdev;

main()
{
	int io, retry, type;

	printf("%s CPU\nBoot\n", getmachineid());
#ifdef JUSTASK
	howto = RB_ASKNAME|RB_SINGLE;
#else
	if ((howto & RB_ASKNAME) == 0) {
		type = (bootdev >> B_TYPESHIFT) & B_TYPEMASK;
		if ((unsigned)type < ndevs && devsw[type].dv_name)
			strcpy(line, "/netbsd");
		else
			howto |= RB_SINGLE|RB_ASKNAME;
	}
#endif
	for (retry = 0;;) {
		if (!noconsole && (howto & RB_ASKNAME)) {
			printf(": ");
			gets(line);
			if (line[0] == 0) {
				strcpy(line, "/netbsd");
				printf(": %s\n", line);
			}
		} else
			printf(": %s\n", line);
		io = open(line, 0);
		if (io >= 0) {
#ifndef INSECURE
			(void) fstat(io, &sb);
			if (sb.st_uid || (sb.st_mode & 2)) {
				printf("non-secure file, will not load\n");
				close(io);
				howto = RB_SINGLE|RB_ASKNAME;
				continue;
			}
#endif
			copyunix(howto, opendev, io);
			close(io);
			howto |= RB_SINGLE|RB_ASKNAME;
		}
		if (++retry > 2)
			howto |= RB_SINGLE|RB_ASKNAME;
	}
}

/*ARGSUSED*/
copyunix(howto, devtype, io)
	register int howto;	/* d7 contains boot flags */
	register u_int devtype;	/* d6 contains boot device */
	register int io;
{
	struct exec x;
	int i;
	register char *load;	/* a5 contains load addr for unix */
	register char *addr;

	i = read(io, (char *)&x, sizeof(x));
	if (i != sizeof(x) ||
	    N_BADMAG(x)) {
		printf("Bad format\n");
		return;
	}
	printf("%d", x.a_text);
	if (N_GETMAGIC(x) == ZMAGIC && lseek(io, 0x400, SEEK_SET) == -1)
		goto shread;
	load = addr = lowram;
	if (read(io, (char *)addr, x.a_text) != x.a_text)
		goto shread;
	addr += x.a_text;
	if (N_GETMAGIC(x) == ZMAGIC || N_GETMAGIC(x) == NMAGIC)
		while ((int)addr & CLOFSET)
			*addr++ = 0;
	printf("+%d", x.a_data);
	if (read(io, addr, x.a_data) != x.a_data)
		goto shread;
	addr += x.a_data;
	printf("+%d", x.a_bss);
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	ssym = addr;
	bcopy(&x.a_syms, addr, sizeof(x.a_syms));
	addr += sizeof(x.a_syms);
	printf("+[%d", x.a_syms);
	if (read(io, addr, x.a_syms) != x.a_syms)
		goto shread;
	addr += x.a_syms;

	if (read(io, &i, sizeof(int)) != sizeof(int))
		goto shread;

	bcopy(&i, addr, sizeof(int));
	if (i) {
		i -= sizeof(int);
		addr += sizeof(int);
		if (read(io, addr, i) != i)
		    goto shread;
		addr += i;
	}
	printf("+%d] ", i);

#define	round_to_size(x) \
	(((int)(x) + sizeof(int) - 1) & ~(sizeof(int) - 1))
	esym = (char *)round_to_size(addr - lowram);
#undef round_to_size

	x.a_entry += (int)lowram;
	printf(" start 0x%x\n", x.a_entry);
#ifdef __GNUC__
	asm("	movl %0,d7" : : "m" (howto));
	asm("	movl %0,d6" : : "m" (devtype));
	asm("	movl %0,a5" : : "a" (load));
	asm("	movl %0,a4" : : "a" (esym));
#endif
	(*((int (*)()) x.a_entry))();
	return;
shread:
	printf("Short read\n");
	return;
}
