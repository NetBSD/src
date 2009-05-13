/*	$NetBSD: boot.c,v 1.3.80.1 2009/05/13 17:18:07 jym Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lib/libsa/loadfile.h>

#if 1
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

void LoadAndReset(void *);

char *netbsd = "/netbsd";

int
main(int argc, char *argv[])
{
	u_long marks[MARK_MAX];
	u_long start, entry;
	int fd, sz, i;
	u_long *ap;
	u_long cksum, *lp;
	void *image;

	/* use the specified kernel if any */
	if (argc > 1)
		netbsd = argv[1];

	DPRINTF("loading %s...\n", netbsd);

	/* count kernel size */
	marks[MARK_START] = 0;
	fd = loadfile(netbsd, marks, COUNT_ALL);
	if (fd == -1)
		err(0, "loadfile(1)");
	close(fd);

	sz = marks[MARK_END] - marks[MARK_START];
	start = marks[MARK_START];
	entry = marks[MARK_ENTRY];

	DPRINTF("size  = 0x%x\n", sz);
	DPRINTF("start = 0x%lx\n", start);

	ap = malloc(sz + 2 * sizeof(long));
	if (ap == NULL)
		err(0, "malloc");

	image = &ap[2];

	marks[MARK_START] = (u_long)image - start;
	fd = loadfile(netbsd, marks, LOAD_ALL);
	if (fd == -1)
		err(0, "loadfile(2)");
	close(fd);

	/* ssym = marks[MARK_SYM]; */
	/* esym = marks[MARK_END]; */

	cksum = 0;
	lp = image;
	for (i = 0; i < sz / sizeof(cksum); i++)
		cksum += *lp++;

	ap[0] = sz;
	ap[1] = cksum;
	LoadAndReset(ap);

	printf("LoadAndReset returned...\n");
	free(ap);
	exit(0);
}

void
LoadAndReset(void *image)
{
	int mib[2];
	u_long val;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_LOADANDRESET;
	val = (u_long)image;

	sysctl(mib, 2, NULL, NULL, &val, sizeof(val));
}
