/* $NetBSD: paxctl.c,v 1.3 2006/09/27 20:01:50 elad Exp $ */

/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <machine/elf_machdep.h>
#define	ELFSIZE	ARCH_ELFSIZE
#include <sys/exec_elf.h>
#include <elf.h>
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void usage(void);
int pax_flag(char *);
int pax_flags_sane(int);
int pax_haveflags(int);
void pax_printflags(int);

struct paxflag {
	char mark;
	char *name;
	int bits;
} flags[] = {
	{ 'M', "mprotect(2) restrictions, explicit enable", PF_PAXMPROTECT },
	{ 'm', "mprotect(2) restrictions, explicit disable", PF_PAXNOMPROTECT },
};

void
usage(void)
{
	errx(1, "Usage: %s [ <-|+>m | <-|+>M ] <file>", getprogname());
}

int
pax_flag(char *s)
{
	int i;

	if (*s == '\0' || !(*(s + 1) == '\0'))
		return (-1);

	for (i = 0; i < sizeof(flags)/sizeof(flags[0]); i++)
		if (*s == flags[i].mark)
			return (flags[i].bits);

	return (-1);
}

int
pax_flags_sane(int f)
{
	int i;

	for (i = 0; i < sizeof(flags)/sizeof(flags[0]); i += 2)
		if ((f & (flags[i].bits|flags[i+1].bits)) ==
		    (flags[i].bits|flags[i+1].bits))
		return (0);

	return (1);
}

int
pax_haveflags(int f)
{
	int i;

	for (i = 0; i < sizeof(flags)/sizeof(flags[0]); i++)
		if (f & flags[i].bits)
			return (1);

	return (0);
}

void
pax_printflags(int f)
{
	int i;

	for (i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
		if (f & flags[i].bits)
			printf("  %c: %s\n", flags[i].mark, flags[i].name);
	}
}

int
main(int argc, char **argv)
{
	Elf_Ehdr eh;
	Elf_Phdr ph;
	char *opt = NULL;
	int fd, i, add_flags = 0, del_flags = 0, list = 0, ok = 0, flagged = 0;

	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++) {
		opt = argv[i];

		if (*opt == '-' || *opt == '+') {
			int t;

			t = pax_flag(opt + 1);
			if (t == -1)
				usage();

			if (*opt == '-')
				del_flags |= t;
			else
				add_flags |= t;

			opt = NULL;
		} else
			break;
	}

	if (opt == NULL)
		usage();

	if (add_flags || del_flags) {
		if (list)
			usage();
	} else
		list = 1;

	fd = open(opt, O_RDWR, 0);
	if (fd == -1) {
		if (!list || (fd = open(opt, O_RDONLY, 0))  == -1)
			err(1, "Can't open `%s'", opt);
	}

	if (read(fd, &eh, sizeof(eh)) != sizeof(eh))
		err(1, "Can't read ELF header from `%s'", opt);

	if (memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0)
		errx(1, "Bad ELF magic from `%s' (maybe it's not an ELF?)",
		    opt);

	for (i = 0; i < eh.e_phnum; i++) {
		if (pread(fd, &ph, sizeof(ph),
			  eh.e_phoff + i * sizeof(ph)) != sizeof(ph))
			err(1, "Can't read data from `%s'", opt);

		if (ph.p_type == PT_NOTE) {
			ok = 1;

			if (list) {
				if (!pax_haveflags(ph.p_flags))
					break;

				if (!pax_flags_sane(ph.p_flags))
					warnx("Flags don't make sense");

				(void)printf("PaX flags:\n");

				pax_printflags(ph.p_flags);

				flagged = 1;

				break;
			}

			ph.p_flags |= add_flags;
			ph.p_flags &= ~del_flags;

			if (!pax_flags_sane(ph.p_flags))
				errx(1, "New flags don't make sense");

			if (pwrite(fd, &ph, sizeof(ph),
				   eh.e_phoff + i * sizeof(ph)) != sizeof(ph))
				err(1, "Can't modify flags on `%s'", opt);

			break;
		}
	}

	if (!ok)
		errx(1, "Could not find an ELF PT_NOTE section in `%s'",
		     opt);

	if (list && !flagged)
		(void)printf("No PaX flags.\n");

	(void)close(fd);

	return (0);
}
