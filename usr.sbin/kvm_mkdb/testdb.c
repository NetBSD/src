/*	$NetBSD: testdb.c,v 1.8 2001/03/19 15:18:59 msaitoh Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)testdb.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: testdb.c,v 1.8 2001/03/19 15:18:59 msaitoh Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/sysctl.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <kvm.h>
#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <paths.h>

#include "extern.h"

/* Return true if the db file is valid, else false */
int
testdb()
{
	DB *db;
	int kd, ret, dbversionlen;
	DBT rec;
	char dbversion[_POSIX2_LINE_MAX];
	char *kversion;
	int mib[2];
	size_t size;

	ret = 0;
	db = NULL;

	if ((kd = open(_PATH_KMEM, O_RDONLY, 0)) < 0)
		goto close;

	if ((db = dbopen(_PATH_KVMDB, O_RDONLY, 0, DB_HASH, NULL)) == NULL)
		goto close;

	/* Read the version out of the database */
	rec.data = VRS_KEY;
	rec.size = sizeof(VRS_KEY) - 1;
	if ((db->get)(db, &rec, &rec, 0))
		goto close;
	if (rec.data == 0 || rec.size == 0 || rec.size > sizeof(dbversion))
		goto close;
	memmove(dbversion, rec.data, rec.size);
	dbversionlen = rec.size;

	/* Read version string from kernel memory */
	mib[0] = CTL_KERN;
	mib[1] = KERN_VERSION;
	if (sysctl(mib, 2, NULL, &size, NULL, 0) == -1)
		errx(1, "can't get size of kernel version string");

	if ((kversion = malloc(size)) == NULL)
		err(1, "couldn't allocate space for buffer data");

	if (sysctl(mib, 2, kversion, &size, NULL, 0) == -1)
		errx(1, "can't get kernel version string");

	/* If they match, we win */
	ret = memcmp(dbversion, kversion, dbversionlen) == 0;

close:	if (kd >= 0)
		(void)close(kd);
	if (db)
		(void)(db->close)(db);
	return (ret);
}
