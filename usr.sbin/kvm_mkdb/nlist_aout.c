/* $NetBSD: nlist_aout.c,v 1.10 2018/01/23 21:06:25 sevan Exp $ */

/*-
 * Copyright (c) 1990, 1993
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
 */

/*-
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)nlist.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: nlist_aout.c,v 1.10 2018/01/23 21:06:25 sevan Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>

#include <a.out.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#ifdef NLIST_AOUT

typedef struct nlist NLIST;
#define	_strx	n_un.n_strx
#define	_name	n_un.n_name

#define	badfmt(str)							\
	do {								\
		warnx("%s: %s: %s", kfile, str, strerror(EFTYPE));	\
		punt();							\
	} while (0)

static void	badread(int, char *);
static u_long	get_kerntext(const char *kfn);

static const char *kfile;

int
create_knlist_aout(const char *name, DB *db)
{
	int nsyms;
	struct exec ebuf;
	FILE *fp;
	NLIST nbuf;
	DBT data, key;
	int fd, nr, strsize;
	u_long kerntextoff;
	char *strtab, buf[1024];

	kfile = name;
	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		warn("%s", kfile);
		punt();
	}

	/* Read in exec structure. */
	nr = read(fd, &ebuf, sizeof(struct exec));
	if (nr != sizeof(struct exec)) {
		(void)close(fd);
		return (-1);
	}

	/* Check magic number. */
	if (N_BADMAG(ebuf)) {
		(void)close(fd);
		return (-1);
	}

	/*
	 * We've recognized it as an a.out binary.  From here
	 * on out, all errors are fatal.
	 */

	/* Check symbol count. */
	if (!ebuf.a_syms)
		badfmt("stripped");

	/* Seek to string table. */
	if (lseek(fd, N_STROFF(ebuf), SEEK_SET) == -1)
		badfmt("corrupted string table");

	/* Read in the size of the symbol table. */
	nr = read(fd, (char *)&strsize, sizeof(strsize));
	if (nr != sizeof(strsize))
		badread(nr, "no symbol table");

	/* Read in the string table. */
	strsize -= sizeof(strsize);
	if (!(strtab = malloc(strsize))) {
		warn("malloc");
		punt();
	}
	if ((nr = read(fd, strtab, strsize)) != strsize)
		badread(nr, "corrupted symbol table");

	/* Seek to symbol table. */
	if (!(fp = fdopen(fd, "r"))) {
		warn("%s", name);
		punt();
	}
	if (fseek(fp, N_SYMOFF(ebuf), SEEK_SET) == -1) {
		warn("%s", name);
		punt();
	}
	
	data.data = (u_char *)&nbuf;
	data.size = sizeof(NLIST);

	kerntextoff = get_kerntext(name);

	/* Read each symbol and enter it into the database. */
	nsyms = ebuf.a_syms / sizeof(struct nlist);
	while (nsyms--) {
		if (fread((char *)&nbuf, sizeof (NLIST), 1, fp) != 1) {
			if (feof(fp))
				badfmt("corrupted symbol table");
			warn("%s", name);
			punt();
		}
		if (!nbuf._strx || nbuf.n_type&N_STAB)
			continue;

		key.data = (u_char *)strtab + nbuf._strx - sizeof(long);
		key.size = strlen((char *)key.data);
		if (db->put(db, &key, &data, 0)) {
			warn("record enter");
			punt();
		}

		if (strcmp((char *)key.data, VRS_SYM) == 0) {
			long cur_off, voff;
			/*
			 * Calculate offset relative to a normal (non-kernel)
			 * a.out.  Kerntextoff is where the kernel is really
			 * loaded; N_TXTADDR is where a normal file is loaded.
			 * From there, locate file offset in text or data.
			 */
			voff = nbuf.n_value - kerntextoff + N_TXTADDR(ebuf);
			if ((nbuf.n_type & N_TYPE) == N_TEXT)
				voff += N_TXTOFF(ebuf) - N_TXTADDR(ebuf);
			else
				voff += N_DATOFF(ebuf) - N_DATADDR(ebuf);
			cur_off = ftell(fp);
			if (fseek(fp, voff, SEEK_SET) == -1)
				badfmt("corrupted string table");

			/*
			 * Read version string up to, and including newline.
			 * This code assumes that a newline terminates the
			 * version line.
			 */
			if (fgets(buf, sizeof(buf), fp) == NULL)
				badfmt("corrupted string table");

			key.data = (u_char *)VRS_KEY;
			key.size = sizeof(VRS_KEY) - 1;
			data.data = (u_char *)buf;
			data.size = strlen(buf);
			if (db->put(db, &key, &data, 0)) {
				warn("record enter");
				punt();
			}

			/* Restore to original values. */
			data.data = (u_char *)&nbuf;
			data.size = sizeof(NLIST);
			if (fseek(fp, cur_off, SEEK_SET) == -1)
				badfmt("corrupted string table");
		}
	}
	(void)fclose(fp);

	return (0);
}

static void
badread(int nr, char *p)
{
	if (nr < 0) {
		warn("%s", kfile);
		punt();
	}
	badfmt(p);
}

/*
 * Instead of compiling in KERNTEXTOFF or KERNBASE, try to
 * determine the text start address from a standard symbol.
 * For backward compatibility, use the old compiled-in way
 * when the standard symbol name is not found.
 */
static u_long
get_kerntext(const char *name)
{
	struct nlist nl[2];

	memset((caddr_t)nl, 0, sizeof(nl));
	nl[0].n_un.n_name = "_kernel_text";

	if (nlist(name, nl) != 0) {
		warnx("%s: %s symbol missing",
		    name, nl[0].n_un.n_name);
		punt();
	}

	return (nl[0].n_value);
}
#endif /* NLIST_AOUT */
