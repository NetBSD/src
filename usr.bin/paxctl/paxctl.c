/* $NetBSD: paxctl.c,v 1.8 2006/11/20 16:51:44 elad Exp $ */

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
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
#ifdef __RCSID
__RCSID("$NetBSD: paxctl.c,v 1.8 2006/11/20 16:51:44 elad Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <elf.h>
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) __attribute__((__noreturn__));
static int pax_flag(const char *);
static int pax_flags_sane(u_long);
static int pax_haveflags(u_long);
static void pax_printflags(u_long);

#ifndef PF_PAXMPROTECT
#define PF_PAXMPROTECT		0x08000000
#define PF_PAXNOMPROTECT	0x04000000
#endif
#ifndef __arraycount
#define __arraycount(a) (sizeof(a) / sizeof(a[0]))
#endif


static const struct paxflag {
	char mark;
	const char *name;
	int bits;
} flags[] = {
	{ 'M', "mprotect(2) restrictions, explicit enable", PF_PAXMPROTECT },
	{ 'm', "mprotect(2) restrictions, explicit disable", PF_PAXNOMPROTECT },
};

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [ <-|+>m | <-|+>M ] <file>\n",
#if HAVE_NBTOOL_CONFIG_H
	    "paxctl"
#else
	    getprogname()
#endif
	);
	exit(1);
}

static int
pax_flag(const char *s)
{
	size_t i;

	if (s[0] == '\0' || s[1] != '\0')
		return -1;

	for (i = 0; i < __arraycount(flags); i++)
		if (*s == flags[i].mark)
			return flags[i].bits;

	return -1;
}

static int
pax_flags_sane(u_long f)
{
	size_t i;

	for (i = 0; i < __arraycount(flags) - 1; i += 2) {
		int g = flags[i].bits | flags[i+1].bits;
		if ((f & g) == g)
			return 0;
	}

	return 1;
}

static int
pax_haveflags(u_long f)
{
	size_t i;

	for (i = 0; i < __arraycount(flags); i++)
		if (f & flags[i].bits)
			return (1);

	return (0);
}

static void
pax_printflags(u_long f)
{
	size_t i;

	for (i = 0; i < __arraycount(flags); i++)
		if (f & flags[i].bits)
			(void)printf("  %c: %s\n",
			    flags[i].mark, flags[i].name);
}

int
main(int argc, char **argv)
{
	union {
	    Elf32_Ehdr h32;
	    Elf64_Ehdr h64;
	} e;
	union {
	    Elf32_Phdr h32;
	    Elf64_Phdr h64;
	} p;
#define EH(field)	(size == 32 ? e.h32.field : e.h64.field)
#define PH(field)	(size == 32 ? p.h32.field : p.h64.field)
#define SPH(field, val)	do { \
    if (size == 32) \
	    p.h32.field val; \
    else \
	    p.h64.field val; \
} while (/*CONSTCOND*/0)
#define PHSIZE		(size == 32 ? sizeof(p.h32) : sizeof(p.h64))

	int size;
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
			err(EXIT_FAILURE, "Can't open `%s'", opt);
	}

	if (read(fd, &e, sizeof(e)) != sizeof(e))
		err(EXIT_FAILURE, "Can't read ELF header from `%s'", opt);

	if (memcmp(e.h32.e_ident, ELFMAG, SELFMAG) != 0)
		errx(EXIT_FAILURE,
		    "Bad ELF magic from `%s' (maybe it's not an ELF?)", opt);

	if (e.h32.e_ehsize == sizeof(e.h32))
		size = 32;
	else if (e.h64.e_ehsize == sizeof(e.h64))
		size = 64;
	else
		errx(EXIT_FAILURE,
		    "Bad ELF size %d from `%s' (maybe it's not an ELF?)",
		    (int)e.h32.e_ehsize, opt);

	for (i = 0; i < EH(e_phnum); i++) {
		if (pread(fd, &p, PHSIZE,
		    (off_t)EH(e_phoff) + i * PHSIZE) != PHSIZE)
			err(EXIT_FAILURE, "Can't read data from `%s'", opt);

		if (PH(p_type) != PT_NOTE)
			continue;

		ok = 1;

		if (list) {
			if (!pax_haveflags((u_long)PH(p_flags)))
				break;

			if (!pax_flags_sane((u_long)PH(p_flags)))
				warnx("Current flags %lx don't make sense",
				    (u_long)PH(p_flags));

			(void)printf("PaX flags:\n");

			pax_printflags((u_long)PH(p_flags));

			flagged = 1;

			break;
		}

		SPH(p_flags, |= add_flags);
		SPH(p_flags, &= ~del_flags);

		if (!pax_flags_sane((u_long)PH(p_flags)))
			errx(EXIT_FAILURE, "New flags %lx don't make sense",
			    (u_long)PH(p_flags));

		if (pwrite(fd, &p, PHSIZE,
		    (off_t)EH(e_phoff) + i * PHSIZE) != PHSIZE)
			err(EXIT_FAILURE, "Can't modify flags on `%s'", opt);

		break;
	}

	(void)close(fd);

	if (!ok)
		errx(EXIT_FAILURE,
		    "Could not find an ELF PT_NOTE section in `%s'", opt);

	if (list && !flagged)
		(void)printf("No PaX flags.\n");
	return 0;
}
