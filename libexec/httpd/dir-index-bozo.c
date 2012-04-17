/*	$NetBSD: dir-index-bozo.c,v 1.12.4.1 2012/04/17 00:05:35 yamt Exp $	*/

/*	$eterna: dir-index-bozo.c,v 1.20 2011/11/18 09:21:15 mrg Exp $	*/

/*
 * Copyright (c) 1997-2011 Matthew R. Green
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

static void
directory_hr(bozohttpd_t *httpd)
{

	bozo_printf(httpd,
		"<hr noshade align=\"left\" width=\"80%%\">\r\n\r\n");
}

/*
 * output a directory index.  return 1 if it actually did something..
 */
int
bozo_dir_index(bozo_httpreq_t *request, const char *dirname, int isindex)
{
	bozohttpd_t *httpd = request->hr_httpd;
	struct stat sb;
	struct dirent **de, **deo;
	struct tm *tm;
	DIR *dp;
	char buf[MAXPATHLEN];
	char spacebuf[48];
	char *file = NULL;
	int l, k, j, i;

	if (!isindex || !httpd->dir_indexing)
		return 0;

	if (strlen(dirname) <= strlen(httpd->index_html))
		dirname = ".";
	else {
		file = bozostrdup(httpd, dirname);

		file[strlen(file) - strlen(httpd->index_html)] = '\0';
		dirname = file;
	}
	debug((httpd, DEBUG_FAT, "bozo_dir_index: dirname ``%s''", dirname));
	if (stat(dirname, &sb) < 0 ||
	    (dp = opendir(dirname)) == NULL) {
		if (errno == EPERM)
			(void)bozo_http_error(httpd, 403, request,
			    "no permission to open directory");
		else if (errno == ENOENT)
			(void)bozo_http_error(httpd, 404, request, "no file");
		else
			(void)bozo_http_error(httpd, 500, request,
					"open directory");
		goto done;
		/* NOTREACHED */
	}

	bozo_printf(httpd, "%s 200 OK\r\n", request->hr_proto);

	if (request->hr_proto != httpd->consts.http_09) {
		bozo_print_header(request, NULL, "text/html", "");
		bozo_printf(httpd, "\r\n");
	}
	bozo_flush(httpd, stdout);

	if (request->hr_method == HTTP_HEAD) {
		closedir(dp);
		goto done;
	}

	bozo_printf(httpd,
		"<html><head><title>Index of %s</title></head>\r\n",
		request->hr_file);
	bozo_printf(httpd, "<body><h1>Index of %s</h1>\r\n",
		request->hr_file);
	bozo_printf(httpd, "<pre>\r\n");
#define NAMELEN 40
#define LMODLEN 19
	bozo_printf(httpd, "Name                                     "
	    "Last modified          "
	    "Size\n");
	bozo_printf(httpd, "</pre>");
	directory_hr(httpd);
	bozo_printf(httpd, "<pre>");

	for (j = k = scandir(dirname, &de, NULL, alphasort), deo = de;
	    j--; de++) {
		int nostat = 0;
		char *name = (*de)->d_name;

		if (strcmp(name, ".") == 0 ||
		    (strcmp(name, "..") != 0 &&
		     httpd->hide_dots && name[0] == '.'))
			continue;

		snprintf(buf, sizeof buf, "%s/%s", dirname, name);
		if (stat(buf, &sb))
			nostat = 1;

		l = 0;

		if (strcmp(name, "..") == 0) {
			bozo_printf(httpd, "<a href=\"../\">");
			l += bozo_printf(httpd, "Parent Directory");
		} else if (S_ISDIR(sb.st_mode)) {
			bozo_printf(httpd, "<a href=\"%s/\">", name);
			l += bozo_printf(httpd, "%s/", name);
		} else if (strchr(name, ':') != NULL) {
			/* RFC 3986 4.2 */
			bozo_printf(httpd, "<a href=\"./%s\">", name);
			l += bozo_printf(httpd, "%s", name);
		} else {
			bozo_printf(httpd, "<a href=\"%s\">", name);
			l += bozo_printf(httpd, "%s", name);
		}
		bozo_printf(httpd, "</a>");

		/* NAMELEN spaces */
		/*LINTED*/
		assert(/*CONSTCOND*/sizeof(spacebuf) > NAMELEN);
		i = (l < NAMELEN) ? (NAMELEN - l) : 0;
		i++;
		memset(spacebuf, ' ', (size_t)i);
		spacebuf[i] = '\0';
		bozo_printf(httpd, "%s", spacebuf);
		l += i;

		if (nostat)
			bozo_printf(httpd, "?                         ?");
		else {
			tm = gmtime(&sb.st_mtime);
			strftime(buf, sizeof buf, "%d-%b-%Y %R", tm);
			l += bozo_printf(httpd, "%s", buf);

			/* LMODLEN spaces */
			/*LINTED*/
			assert(/*CONSTCOND*/sizeof(spacebuf) > LMODLEN);
			i = (l < (LMODLEN+NAMELEN+1)) ?
				((LMODLEN+NAMELEN+1) - l) : 0;
			i++;
			memset(spacebuf, ' ', (size_t)i);
			spacebuf[i] = '\0';
			bozo_printf(httpd, "%s", spacebuf);

			bozo_printf(httpd, "%7ukB",
			    ((unsigned)((unsigned)(sb.st_size) >> 10)));
		}
		bozo_printf(httpd, "\r\n");
	}

	closedir(dp);
	while (k--)
        	free(deo[k]);
	free(deo);
	bozo_printf(httpd, "</pre>");
	directory_hr(httpd);
	bozo_printf(httpd, "</body></html>\r\n\r\n");
	bozo_flush(httpd, stdout);

done:
	if (file)
		free(file);
	return 1;
}
#endif /* NO_DIRINDEX_SUPPORT */

