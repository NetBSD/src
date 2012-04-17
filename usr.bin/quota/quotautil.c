/*	$NetBSD: quotautil.c,v 1.3.4.1 2012/04/17 00:09:39 yamt Exp $ */

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quota.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: quotautil.c,v 1.3.4.1 2012/04/17 00:09:39 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fstab.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>

#include <ufs/ufs/quota1.h>

#include "quotautil.h"

const char *qfextension[] = INITQFNAMES;
const char *qfname = QUOTAFILENAME;
 
/*
 * Check to see if a particular quota is to be enabled.
 */
int
hasquota(char *buf, size_t len, struct fstab *fs, int type)
{
	char *opt;
	char *cp = NULL;
	static char initname, usrname[100], grpname[100];
	char optbuf[256];

	if (!initname) {
		(void)snprintf(usrname, sizeof(usrname), "%s%s",
		    qfextension[USRQUOTA], qfname);
		(void)snprintf(grpname, sizeof(grpname), "%s%s",
		    qfextension[GRPQUOTA], qfname);
		initname = 1;
	}
	strlcpy(optbuf, fs->fs_mntops, sizeof(optbuf));
	for (opt = strtok(optbuf, ","); opt; opt = strtok(NULL, ",")) {
		if ((cp = strchr(opt, '=')) != NULL)
			*cp++ = '\0';
		if (type == USRQUOTA && strcmp(opt, usrname) == 0)
			break;
		if (type == GRPQUOTA && strcmp(opt, grpname) == 0)
			break;
	}
	if (!opt)
		return 0;
	if (cp) {
		strlcpy(buf, cp, len);
		return 1;
	}
	(void)snprintf(buf, len, "%s/%s.%s", fs->fs_file, qfname,
	    qfextension[type]);
	return 1;
}

/*
 * Check to see if target appears in list of size cnt.
 */
int
oneof(const char *target, char *list[], int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		if (strcmp(target, list[i]) == 0)
			return i;
	return -1;
}
