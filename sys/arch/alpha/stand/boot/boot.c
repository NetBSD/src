/*	$NetBSD: boot.c,v 1.7 1996/06/14 20:03:00 cgd Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_ecoff.h>

#include <machine/prom.h>

#define _KERNEL
#include "include/pte.h"

static int aout_exec __P((int, struct exec *, u_int64_t *));
static int coff_exec __P((int, struct ecoff_exechdr *, u_int64_t *));
static int loadfile __P((char *, u_int64_t *));

char boot_file[128];
char boot_flags[128];

extern char bootprog_name[], bootprog_rev[], bootprog_date[], bootprog_maker[];

vm_offset_t ffp_save, ptbr_save;

void
main()
{
	u_int64_t entry;

	/* Init prom callback vector. */
	init_prom_calls();

	/* print a banner */
	printf("\n");
	printf("%s, Revision %s\n", bootprog_name, bootprog_rev);
	printf("(%s, %s)\n", bootprog_maker, bootprog_date);
	printf("\n");

	/* switch to OSF pal code. */
	OSFpal();

	printf("\n");

	prom_getenv(PROM_E_BOOTED_FILE, boot_file, sizeof(boot_file));
	prom_getenv(PROM_E_BOOTED_OSFLAGS, boot_flags, sizeof(boot_flags));

	if (boot_file[0] == '\0')
		bcopy("netbsd", boot_file, sizeof "netbsd");

	(void)printf("Boot: %s %s\n", boot_file, boot_flags);

	if (!loadfile(boot_file, &entry)) {
		(void)printf("Entering kernel at 0x%lx...\n", entry);
		(*(void (*)())entry)(ffp_save, ptbr_save, 0);
	}

	(void)printf("Boot failed!  Halting...\n");
	halt();
}

/*
 * Open 'filename', read in program and return the entry point or -1 if error.
 */
static int
loadfile(fname, entryp)
	char *fname;
	u_int64_t *entryp;
{
	struct devices *dp;
	union {
		struct exec aout;
		struct ecoff_exechdr coff;
	} hdr;
	ssize_t nr;
	int fd, rval;

	/* Open the file. */
	rval = 1;
	if ((fd = open(fname, 0)) < 0) {
		(void)printf("open %s: error %d\n", fname, errno);
		goto err;
	}

	/* Read the exec header. */
	if ((nr = read(fd, &hdr, sizeof(hdr))) != sizeof(hdr)) {
		(void)printf("read header: error %d\n", errno);
		goto err;
	}

#ifdef ALPHA_BOOT_ECOFF
	if (!ECOFF_BADMAG(&hdr.coff)) {
		rval = coff_exec(fd, &hdr.coff, entryp);
	} else
#endif
#ifdef ALPHA_BOOT_AOUT
	if (XXX) {
		rval = aout_exec(fd, &hdr.aout, entryp) :
	} else
#endif
	{
		(void)printf("%s: unknown executable format\n", fname);
	}

err:
	if (fd >= 0)
		(void)close(fd);
	return (rval);
}

#ifdef ALPHA_BOOT_AOUT
static int
aout_exec(fd, aout, entryp)
	int fd;
	struct exec *aout;
	u_int64_t *entryp;
{
	size_t sz;

	/* Check the magic number. */
	if (N_GETMAGIC(*aout) != OMAGIC) {
		(void)printf("bad magic: %o\n", N_GETMAGIC(*aout));
		return (1);
	}

	/* Read in text, data. */
	(void)printf("%lu+%lu", aout->a_text, aout->a_data);
	if (lseek(fd, (off_t)N_TXTOFF(*aout), SEEK_SET) < 0) {
		(void)printf("lseek: %d\n", errno);
		return (1);
	}
	sz = aout->a_text + aout->a_data;
	if (read(fd, (void *)aout->a_entry, sz) != sz) {
		(void)printf("read text/data: %d\n", errno);
		return (1);
	}

	/* Zero out bss. */
	if (aout->a_bss != 0) {
		(void)printf("+%lu", aout->a_bss);
		bzero(aout->a_entry + sz, aout->a_bss);
	}

	ffp_save = aout->a_entry + aout->a_text + aout->a_data + aout->a_bss;
	ffp_save = k0segtophys((ffp_save + PGOFSET & ~PGOFSET)) >> PGSHIFT;
	ffp_save += 2;		/* XXX OSF/1 does this, no idea why. */

	(void)printf("\n");
	*entryp = aout->a_entry;
	return (0);
}
#endif /* ALPHA_BOOT_AOUT */

#ifdef ALPHA_BOOT_ECOFF
static int
coff_exec(fd, coff, entryp)
	int fd;
	struct ecoff_exechdr *coff;
	u_int64_t *entryp;
{

	/* Read in text. */
	(void)printf("%lu", coff->a.tsize);
	(void)lseek(fd, ECOFF_TXTOFF(coff), 0);
	if (read(fd, (void *)coff->a.text_start, coff->a.tsize) !=
	    coff->a.tsize) {
		(void)printf("read text: %d\n", errno);
		return (1);
	}

	/* Read in data. */
	if (coff->a.dsize != 0) {
		(void)printf("+%lu", coff->a.dsize);
		if (read(fd, (void *)coff->a.data_start, coff->a.dsize) !=
		    coff->a.dsize) {
			(void)printf("read data: %d\n", errno);
			return (1);
		}
	}


	/* Zero out bss. */
	if (coff->a.bsize != 0) {
		(void)printf("+%lu", coff->a.bsize);
		bzero(coff->a.bss_start, coff->a.bsize);
	}

	ffp_save = coff->a.text_start + coff->a.tsize;
	if (ffp_save < coff->a.data_start + coff->a.dsize)
		ffp_save = coff->a.data_start + coff->a.dsize;
	if (ffp_save < coff->a.bss_start + coff->a.bsize)
		ffp_save = coff->a.bss_start + coff->a.bsize;
	ffp_save = k0segtophys((ffp_save + PGOFSET & ~PGOFSET)) >> PGSHIFT;
	ffp_save += 2;		/* XXX OSF/1 does this, no idea why. */

	(void)printf("\n");
	*entryp = coff->a.entry;
	return (0);
}
#endif /* ALPHA_BOOT_ECOFF */
