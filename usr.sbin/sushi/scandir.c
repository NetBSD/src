/*      $NetBSD: scandir.c,v 1.2 2001/01/06 15:04:05 veego Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Dante Profeta <dante@netbsd.org>
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "menutree.h"
#include "scandir.h"
#include "handlers.h"
#include "sushi.h"

extern char *lang_id;
extern nl_catd catalog;

void
scan_dir(cqm, basedir)
	struct cqMenu *cqm;
	char *basedir;
{
	FILE *filep;
	struct stat sb;
	char path[PATH_MAX+30];
	int lcnt;
	char *p, *t;
	size_t len;

	if (lang_id == NULL)
		snprintf(path, sizeof(path), "%s/%s", basedir, INDEXFILE);
	else {
		snprintf(path, sizeof(path), "%s/%s.%s", basedir, INDEXFILE,
		    lang_id);
		if (stat(path, &sb) != 0)
			snprintf(path, sizeof(path), "%s/%s", basedir,
			    INDEXFILE);
	}
	if((filep = fopen(path, "r"))) {
		for (lcnt = 1; (p = fgetln(filep, &len)) != NULL; ++lcnt) {
			if (len == 1)		/* Skip empty lines. */
				continue;
			if (p[len - 1] == '#')	/* Skip remarked lines. */
				continue;
			if (p[len - 1] != '\n') {/* Skip corrupted lines. */
				warnx("%s: line %d corrupted", path, lcnt);
				continue;
			}
			p[len - 1] = '\0';	/* Terminate the line. */

						/* Skip leading spaces. */
			for (; *p != '\0' && isspace((unsigned char)*p); ++p);
						/* Skip empty/comment lines. */
			if (*p == '\0' || *p == '#')
				continue;
						/* Find first token. */
			for (t = p; *t && !isspace((unsigned char)*t); ++t);
			if (*t == '\0')		/* Need more than one token.*/
				continue;
			*t = '\0';

			scan_index(cqm, basedir, p);
		}

		fclose(filep);
	}
}

void
scan_index(cqm, basedir, row)
	struct cqMenu *cqm;
	char *basedir;
	char *row;
{
	char *t = row, *p;
	char filename[PATH_MAX];
	char menuname[80];
	char nextpath[PATH_MAX];
	char quickname[80];
	struct stat dirstat;

	while (*++t && !isspace((unsigned char)*t));
	snprintf(filename, t-row+1, "%s", row);
	snprintf(nextpath, sizeof(nextpath), "%s/%s", basedir, filename);

	if (strcmp(filename, "BLANK") && stat(nextpath, &dirstat) < 0) {
		warn("%s %s", catgets(catalog, 1, 18, "can't stat"), nextpath);
		return;
	}

	while (*++t && isspace((unsigned char)*t));
	for (p=t; *p != '\0' && !isspace((unsigned char)*p); ++p);
	*p='\0';
	snprintf(quickname, sizeof(quickname), "%s", t);

	t=p;
	while (*++t && isspace((unsigned char)*t));
	snprintf(menuname, sizeof(menuname), "%s", t);

	tree_appenditem(cqm, filename, menuname, quickname, nextpath);

	if(S_ISDIR(dirstat.st_mode)) {
		scan_dir(&CIRCLEQ_LAST(cqm)->cqSubMenuHead, nextpath);
	}
}
