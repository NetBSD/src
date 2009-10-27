/* $NetBSD: paxctl.c,v 1.12 2009/10/27 16:27:47 christos Exp $ */

/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
 * Copyright (c) 2008 Christos Zoulas <christos@NetBSD.org>
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
__RCSID("$NetBSD: paxctl.c,v 1.12 2009/10/27 16:27:47 christos Exp $");
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
static uint32_t pax_flag(char);
static int pax_flags_sane(uint32_t);
static int pax_haveflags(uint32_t);
static void pax_printflags(const char *, int, uint32_t);

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
	uint32_t bits;
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

static uint32_t
pax_flag(char s)
{
	size_t i;

	if (s == '\0')
		return (uint32_t)-1;

	for (i = 0; i < __arraycount(flags); i++)
		if (s == flags[i].mark)
			return flags[i].bits;

	return (uint32_t)-1;
}

static int
pax_flags_sane(uint32_t f)
{
	size_t i;

	for (i = 0; i < __arraycount(flags) - 1; i += 2) {
		uint32_t g = flags[i].bits | flags[i+1].bits;
		if ((f & g) == g)
			return 0;
	}

	return 1;
}

static int
pax_haveflags(uint32_t f)
{
	size_t i;

	for (i = 0; i < __arraycount(flags); i++)
		if (f & flags[i].bits)
			return (1);

	return (0);
}

static void
pax_printflags(const char *name, int many, uint32_t f)
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
process_one(const char *name, uint32_t add_flags, uint32_t del_flags,
    int list, int many)
{
	union {
	    Elf32_Ehdr h32;
	    Elf64_Ehdr h64;
	} e;
	union {
	    Elf32_Shdr h32;
	    Elf64_Shdr h64;
	} s;
	union {
	    Elf32_Nhdr h32;
	    Elf64_Nhdr h64;
	} n;
#define SWAP(a)	(swap == 0 ? (a) : \
    /*LINTED*/(sizeof(a) == 1 ? (a) : \
    /*LINTED*/(sizeof(a) == 2 ? bswap16(a) : \
    /*LINTED*/(sizeof(a) == 4 ? bswap32(a) : \
    /*LINTED*/(sizeof(a) == 8 ? bswap64(a) : (abort(), (a)))))))
#define EH(field)	(size == 32 ? SWAP(e.h32.field) : SWAP(e.h64.field))
#define SH(field)	(size == 32 ? SWAP(s.h32.field) : SWAP(s.h64.field))
#define NH(field)	(size == 32 ? SWAP(n.h32.field) : SWAP(n.h64.field))
#define SHSIZE		(size == 32 ? sizeof(s.h32) : sizeof(s.h64))
#define NHSIZE		(size == 32 ? sizeof(n.h32) : sizeof(n.h64))
	struct {
		char name[ELF_NOTE_PAX_NAMESZ];
		uint32_t flags;
	} pax_tag;
	int fd, size, ok = 0, flagged = 0, swap, error = 1;
	size_t i;

	fd = open(name, list ? O_RDONLY: O_RDWR, 0);
	if (fd == -1) {
		warn("Can't open `%s'", name);
		return error;
	}

	if (read(fd, &e, sizeof(e)) != sizeof(e)) {
		warn("Can't read ELF header from `%s'", name);
		goto out;
	}

	if (memcmp(e.h32.e_ident, ELFMAG, SELFMAG) != 0) {
		warnx("Bad ELF magic from `%s' (maybe it's not an ELF?)", name);
		goto out;
	}

	if (e.h32.e_ehsize == sizeof(e.h32)) {
		size = 32;
		swap = 0;
	} else if (e.h64.e_ehsize == sizeof(e.h64)) {
		size = 64;
		swap = 0;
	} else if (bswap16(e.h32.e_ehsize) == sizeof(e.h32)) {
		size = 32;
		swap = 1;
	} else if (bswap16(e.h64.e_ehsize) == sizeof(e.h64)) {
		size = 64;
		swap = 1;
	} else {
		warnx("Bad ELF size %d from `%s' (maybe it's not an ELF?)",
		    (int)e.h32.e_ehsize, name);
		goto out;
	}

	for (i = 0; i < EH(e_shnum); i++) {
		if ((size_t)pread(fd, &s, SHSIZE,
		    (off_t)EH(e_shoff) + i * SHSIZE) != SHSIZE) {
			warn("Can't read section header data from `%s'", name);
			goto out;
		}

		if (SH(sh_type) != SHT_NOTE)
			continue;

		if (pread(fd, &n, NHSIZE, (off_t)SH(sh_offset)) != NHSIZE) {
			warn("Can't read note header from `%s'", name);
			goto out;
		}
		if (NH(n_type) != ELF_NOTE_TYPE_PAX_TAG ||
		    NH(n_descsz) != ELF_NOTE_PAX_DESCSZ ||
		    NH(n_namesz) != ELF_NOTE_PAX_NAMESZ)
			continue;
		if (pread(fd, &pax_tag, sizeof(pax_tag), SH(sh_offset) + NHSIZE)
		    != sizeof(pax_tag)) {
			warn("Can't read pax_tag from `%s'", name);
			goto out;
		}
		if (memcmp(pax_tag.name, ELF_NOTE_PAX_NAME,
		    sizeof(pax_tag.name)) != 0) {
			warn("Unknown pax_tag name `%*.*s' from `%s'",
			    ELF_NOTE_PAX_NAMESZ, ELF_NOTE_PAX_NAMESZ,
			    pax_tag.name, name);
			goto out;
		}
		ok = 1;

		if (list) {
			if (!pax_haveflags(SWAP(pax_tag.flags)))
				break;

			if (!pax_flags_sane(SWAP(pax_tag.flags)))
				warnx("Current flags 0x%x don't make sense",
				    (uint32_t)SWAP(pax_tag.flags));

			if (many)
				(void)printf("%s: ", name);
			(void)printf("PaX flags:\n");

			pax_printflags(name, many, SWAP(pax_tag.flags));
			flagged = 1;
			break;
		}

		pax_tag.flags |= SWAP(add_flags);
		pax_tag.flags &= SWAP(~del_flags);

		if (!pax_flags_sane(SWAP(pax_tag.flags))) {
			warnx("New flags 0x%x don't make sense",
			    (uint32_t)SWAP(pax_tag.flags));
			goto out;
		}

		if (pwrite(fd, &pax_tag, sizeof(pax_tag),
		    (off_t)SH(sh_offset) + NHSIZE) != sizeof(pax_tag))
			warn("Can't modify flags on `%s'", name);
		break;
	}

	if (!ok) {
		warnx("Could not find an ELF PaX SHT_NOTE section in `%s'",
		    name);
		goto out;
	}

	error = 0;
	if (list && !flagged) {
		if (many)
			(void)printf("%s: ", name);
		(void)printf("No PaX flags.\n");
	}
out:
	(void)close(fd);
	return error;
}

int
main(int argc, char **argv)
{
	char *opt;
	int i, list = 0, bad = 0, many, minus;
	uint32_t add_flags = 0, del_flags = 0;

	setprogname(argv[0]);

	if (argc < 2)
		usage();

	for (i = 1; i < argc; i++) {
		opt = argv[i];

		if (*opt == '-' || *opt == '+') {
			uint32_t t;
			minus = 0;

			while (*opt) {
				switch (*opt) {
				case '+':
					minus = 0;
					opt++;
					break;
				case '-':
					minus = 1;
					opt++;
					break;
				case ',':
					opt++;
					break;
				default:
					t = pax_flag(*opt++);
					if (t == (uint32_t)-1)
						usage();
					if (minus)
						del_flags |= t;
					else
						add_flags |= t;
					break;
				}
			}
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
