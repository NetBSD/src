/* $NetBSD: paxctl.c,v 1.2.4.1 2008/01/09 02:02:13 matt Exp $ */

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
 * 3. The name of the author may not be used to endorse or promote products
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
__RCSID("$NetBSD: paxctl.c,v 1.2.4.1 2008/01/09 02:02:13 matt Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#ifdef HAVE_NBTOOL_CONFIG_H
#include "../../sys/sys/exec_elf.h"
#else
#include <elf.h>
#endif
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) __dead;
static int pax_flag(const char *);
static int pax_flags_sane(u_long);
static int pax_haveflags(u_long);
static void pax_printflags(const char *, int, u_long);

#ifndef ELF_NOTE_TYPE_PAX_TAG
/* NetBSD-specific note type: PaX.  There should be 1 NOTE per executable.
   section.  desc is a 32 bit bitmask */
#define ELF_NOTE_TYPE_PAX_TAG		3
#define	ELF_NOTE_PAX_MPROTECT		0x01	/* Force enable MPROTECT */
#define	ELF_NOTE_PAX_NOMPROTECT		0x02	/* Force disable MPROTECT */
#define	ELF_NOTE_PAX_GUARD		0x04	/* Force enable Segvguard */
#define	ELF_NOTE_PAX_NOGUARD		0x08	/* Force disable Segvguard */
#define	ELF_NOTE_PAX_ASLR		0x10	/* Force enable ASLR */
#define	ELF_NOTE_PAX_NOASLR		0x20	/* Force disable ASLR */
#define ELF_NOTE_PAX_NAMESZ		4
#define ELF_NOTE_PAX_NAME		"PaX\0"
#define ELF_NOTE_PAX_DESCSZ		4
#endif
#ifndef __arraycount
#define __arraycount(a) (sizeof(a) / sizeof(a[0]))
#endif


static const struct paxflag {
	char mark;
	const char *name;
	int bits;
} flags[] = {
	{ 'A', "ASLR, explicit enable",
	  ELF_NOTE_PAX_ASLR },
	{ 'a', "ASLR, explicit disable",
	  ELF_NOTE_PAX_NOASLR },
	{ 'G', "Segvguard, explicit enable",
	  ELF_NOTE_PAX_GUARD },
	{ 'g', "Segvguard, explicit disable",
	  ELF_NOTE_PAX_NOGUARD },
	{ 'M', "mprotect(2) restrictions, explicit enable",
	  ELF_NOTE_PAX_MPROTECT },
	{ 'm', "mprotect(2) restrictions, explicit disable",
	  ELF_NOTE_PAX_NOMPROTECT },
};

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [ <-|+><A|a|G|g|M|m> ] <file> ...\n",
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
pax_printflags(const char *name, int many, u_long f)
{
	size_t i;

	for (i = 0; i < __arraycount(flags); i++)
		if (f & flags[i].bits) {
			if (many)
				(void)printf("%s: ", name);
			(void)printf("  %c: %s\n",
			    flags[i].mark, flags[i].name);
		}
}

static int
process_one(const char *name, int add_flags, int del_flags, int list, int many)
{
	union {
	    Elf32_Ehdr h32;
	    Elf64_Ehdr h64;
	} e;
	union {
	    Elf32_Phdr h32;
	    Elf64_Phdr h64;
	} p;
	union {
	    Elf32_Nhdr h32;
	    Elf64_Nhdr h64;
	} n;
#define EH(field)	(size == 32 ? e.h32.field : e.h64.field)
#define PH(field)	(size == 32 ? p.h32.field : p.h64.field)
#define NH(field)	(size == 32 ? n.h32.field : n.h64.field)
#define SPH(field, val)	do { \
    if (size == 32) \
	    p.h32.field val; \
    else \
	    p.h64.field val; \
} while (/*CONSTCOND*/0)
#define PHSIZE		(size == 32 ? sizeof(p.h32) : sizeof(p.h64))
#define NHSIZE		(size == 32 ? sizeof(n.h32) : sizeof(n.h64))
	struct {
		char name[ELF_NOTE_PAX_NAMESZ];
		uint32_t flags;
	} pax_tag;
	int i, fd, size, ok = 0, flagged = 0;

	fd = open(name, list ? O_RDONLY: O_RDWR, 0);
	if (fd == -1) {
		warn("Can't open `%s'", name);
		return 1;
	}

	if (read(fd, &e, sizeof(e)) != sizeof(e)) {
		warn("Can't read ELF header from `%s'", name);
		return 1;
	}

	if (memcmp(e.h32.e_ident, ELFMAG, SELFMAG) != 0) {
		warn("Bad ELF magic from `%s' (maybe it's not an ELF?)", name);
		return 1;
	}

	if (e.h32.e_ehsize == sizeof(e.h32))
		size = 32;
	else if (e.h64.e_ehsize == sizeof(e.h64))
		size = 64;
	else {
		warn("Bad ELF size %d from `%s' (maybe it's not an ELF?)",
		    (int)e.h32.e_ehsize, name);
		return 1;
	}

	for (i = 0; i < EH(e_phnum); i++) {
		if (pread(fd, &p, PHSIZE, (off_t)EH(e_phoff) + i * PHSIZE) !=
		    PHSIZE) {
			warn("Can't read program header data from `%s'", name);
			return 1;
		}

		if (PH(p_type) != PT_NOTE)
			continue;

		if (pread(fd, &n, NHSIZE, (off_t)PH(p_offset)) != NHSIZE) {
			warn("Can't read note header from `%s'", name);
			return 1;
		}
		if (NH(n_type) != ELF_NOTE_TYPE_PAX_TAG ||
		    NH(n_descsz) != ELF_NOTE_PAX_DESCSZ ||
		    NH(n_namesz) != ELF_NOTE_PAX_NAMESZ)
			continue;
		if (pread(fd, &pax_tag, sizeof(pax_tag), PH(p_offset) + NHSIZE)
		    != sizeof(pax_tag)) {
			warn("Can't read pax_tag from `%s'", name);
			return 1;
		}
		if (memcmp(pax_tag.name, ELF_NOTE_PAX_NAME,
		    sizeof(pax_tag.name)) != 0) {
			warn("Unknown pax_tag name `%*.*s' from `%s'",
			    ELF_NOTE_PAX_NAMESZ, ELF_NOTE_PAX_NAMESZ,
			    pax_tag.name, name);
			return 1;
		}
		ok = 1;

		if (list) {
			if (!pax_haveflags(pax_tag.flags))
				break;

			if (!pax_flags_sane(pax_tag.flags))
				warnx("Current flags %x don't make sense",
				    pax_tag.flags);

			if (many)
				(void)printf("%s: ", name);
			(void)printf("PaX flags:\n");

			pax_printflags(name, many, pax_tag.flags);

			flagged = 1;

			break;
		}

		pax_tag.flags |= add_flags;
		pax_tag.flags &= ~del_flags;

		if (!pax_flags_sane(pax_tag.flags)) {
			warn("New flags %x don't make sense", pax_tag.flags);
			return 1;
		}

		if (pwrite(fd, &pax_tag, sizeof(pax_tag),
		    (off_t)PH(p_offset) + NHSIZE) != sizeof(pax_tag))
			warn("Can't modify flags on `%s'", name);
		break;
	}

	(void)close(fd);

	if (!ok) {
		warn("Could not find an ELF PaX PT_NOTE section in `%s'", name);
		return 1;
	}

	if (list && !flagged) {
		if (many)
			(void)printf("%s: ", name);
		(void)printf("No PaX flags.\n");
	}
	return 0;
}

int
main(int argc, char **argv)
{
	char *opt;
	int i, add_flags = 0, del_flags = 0, list = 0, bad = 0, many;

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

	if (i == argc)
		usage();

	if (add_flags || del_flags) {
		if (list)
			usage();
	} else
		list = 1;

	many = i != argc - 1;
	for (; i < argc; i++)
		bad |= process_one(argv[i], add_flags, del_flags, list, many);

	return bad ? EXIT_FAILURE : 0;
}
