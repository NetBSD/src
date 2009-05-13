/*	$NetBSD: dir-index-bozo.c,v 1.5.2.1 2009/05/13 19:18:38 jym Exp $	*/

/*	$eterna: dir-index-bozo.c,v 1.14 2009/04/18 01:48:18 mrg Exp $	*/

/*
 * Copyright (c) 1997-2009 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer and
 *    dedication in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* this code implements directory index generation for bozohttpd */

#ifndef NO_DIRINDEX_SUPPORT

#include <sys/param.h>

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "bozohttpd.h"

	int	Xflag;		/* do directory indexing */
	int	Hflag;		/* hide .* */

static void
directory_hr(void)
{

	bozoprintf("<hr noshade align=\"left\" width=\"80%%\">\r\n\r\n");
}

/*
 * output a directory index.  return 1 if it actually did something..
 */
int
directory_index(http_req *request, const char *dirname, int isindex)
{
	struct stat sb;
	struct dirent *de;
	struct tm *tm;
	DIR *dp;
	char buf[MAXPATHLEN];
	char spacebuf[48];
	char *file = NULL;
	int l, i;

	if (!isindex || !Xflag)
		return 0;

	if (strlen(dirname) <= strlen(index_html))
		dirname = ".";
	else {
		file = bozostrdup(dirname);

		file[strlen(file) - strlen(index_html)] = '\0';
		dirname = file;
	}
	debug((DEBUG_FAT, "directory_index: dirname ``%s''", dirname));
	if (stat(dirname, &sb) < 0 ||
	    (dp = opendir(dirname)) == NULL) {
		if (errno == EPERM)
			(void)http_error(403, request,
			    "no permission to open directory");
		else if (errno == ENOENT)
			(void)http_error(404, request, "no file");
		else
			(void)http_error(500, request, "open directory");
		goto done;
		/* NOTREACHED */
	}

	bozoprintf("%s 200 OK\r\n", request->hr_proto);

	if (request->hr_proto != http_09) {
		print_header(request, NULL, "text/html", "");
		bozoprintf("\r\n");
	}
	bozoflush(stdout);

	if (request->hr_method == HTTP_HEAD) {
		closedir(dp);
		goto done;
	}

	bozoprintf("<html><head><title>Index of %s</title></head>\r\n",
	    request->hr_file);
	bozoprintf("<body><h1>Index of %s</h1>\r\n", request->hr_file);
	bozoprintf("<pre>\r\n");
#define NAMELEN 40
#define LMODLEN 19
	bozoprintf("Name                                     "
	    "Last modified          "
	    "Size\n");
	bozoprintf("</pre>");
	directory_hr();
	bozoprintf("<pre>");

	while ((de = readdir(dp)) != NULL) {
		int nostat = 0;
		char *name = de->d_name;

		if (strcmp(name, ".") == 0 || 
		    (strcmp(name, "..") != 0 && Hflag && name[0] == '.'))
			continue;

		snprintf(buf, sizeof buf, "%s/%s", dirname, name);
		if (stat(buf, &sb))
			nostat = 1;

		l = 0;

		if (strcmp(name, "..") == 0) {
			bozoprintf("<a href=\"../\">");
			l += bozoprintf("Parent Directory");
		} else if (S_ISDIR(sb.st_mode)) {
			bozoprintf("<a href=\"%s/\">", name);
			l += bozoprintf("%s/", name);
		} else {
			bozoprintf("<a href=\"%s\">", name);
			l += bozoprintf("%s", name);
		}
		bozoprintf("</a>");

		/* NAMELEN spaces */
		assert(sizeof(spacebuf) > NAMELEN);
		i = (l < NAMELEN) ? (NAMELEN - l) : 0;
		i++;
		memset(spacebuf, ' ', i);
		spacebuf[i] = '\0';
		bozoprintf(spacebuf);
		l += i;

		if (nostat)
			bozoprintf("?                         ?");
		else {
			tm = gmtime(&sb.st_mtime);
			strftime(buf, sizeof buf, "%d-%b-%Y %R", tm);
			l += bozoprintf("%s", buf);

			/* LMODLEN spaces */
			assert(sizeof(spacebuf) > LMODLEN);
			i = (l < (LMODLEN+NAMELEN+1)) ? ((LMODLEN+NAMELEN+1) - l) : 0;
			i++;
			memset(spacebuf, ' ', i);
			spacebuf[i] = '\0';
			bozoprintf(spacebuf);

			bozoprintf("%7ukB",
			    ((unsigned int)(sb.st_size >> 10)));
		}
		bozoprintf("\r\n");
	}

	closedir(dp);
	bozoprintf("</pre>");
	directory_hr();
	bozoprintf("</body></html>\r\n\r\n");
	bozoflush(stdout);
	
done:
	if (file)
		free(file);
	return 1;
}
#endif /* NO_DIRINDEX_SUPPORT */
