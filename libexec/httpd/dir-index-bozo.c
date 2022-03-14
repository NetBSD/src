/*	$NetBSD: dir-index-bozo.c,v 1.35 2022/03/14 05:06:59 mrg Exp $	*/

/*	$eterna: dir-index-bozo.c,v 1.20 2011/11/18 09:21:15 mrg Exp $	*/

/*
 * Copyright (c) 1997-2020 Matthew R. Green
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
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#include "bozohttpd.h"

/*
 * output a directory index.  return 1 if it actually did something..
 */
int
bozo_dir_index(bozo_httpreq_t *request, const char *dirpath, int isindex)
{
	bozohttpd_t *httpd = request->hr_httpd;
	struct stat sb;
	struct dirent **de, **deo;
	DIR *dp;
	char buf[MAXPATHLEN];
	char *file = NULL, *printname = NULL, *p;
	int k, j, fd;
	ssize_t rlen;

	if (!isindex || !httpd->dir_indexing)
		return 0;

	if (strlen(dirpath) <= strlen(httpd->index_html))
		dirpath = ".";
	else {
		file = bozostrdup(httpd, request, dirpath);

		file[strlen(file) - strlen(httpd->index_html)] = '\0';
		dirpath = file;
	}
	debug((httpd, DEBUG_FAT, "bozo_dir_index: dirpath '%s'", dirpath));
	if (stat(dirpath, &sb) < 0 ||
	    (dp = opendir(dirpath)) == NULL) {
		if (errno == EPERM)
			bozo_http_error(httpd, 403, request,
					"no permission to open directory");
		else if (errno == ENOENT)
			bozo_http_error(httpd, 404, request, "no file");
		else
			bozo_http_error(httpd, 500, request, "open directory");
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

#ifndef NO_USER_SUPPORT
	if (request->hr_user) {
		bozoasprintf(httpd, &printname, "~%s/%s",
			     request->hr_user, request->hr_file);
	} else
		printname = bozostrdup(httpd, request, request->hr_file);
#else
	printname = bozostrdup(httpd, request, request->hr_file);
#endif /* !NO_USER_SUPPORT */
	if ((p = strstr(printname, httpd->index_html)) != NULL) {
		if (strcmp(printname, httpd->index_html) == 0)
			strcpy(printname, "/");	/* is ``slashdir'' */
		else
			*p = '\0';		/* strip unwanted ``index_html'' */
	}
	if ((p = bozo_escape_html(httpd, printname)) != NULL) {
		free(printname);
		printname = p;
	}

	bozo_printf(httpd,
		"<!DOCTYPE html>\r\n"
		"<html><head><meta charset=\"utf-8\"/>\r\n"
		"<style type=\"text/css\">\r\n"
		"table {\r\n"
		"\tborder-top: 1px solid black;\r\n"
		"\tborder-bottom: 1px solid black;\r\n"
		"}\r\n"
		"th { background: aquamarine; }\r\n"
		"tr:nth-child(even) { background: lavender; }\r\n"
		"</style>\r\n");
	bozo_printf(httpd, "<title>Index of %s</title></head>\r\n",
		printname);
	bozo_printf(httpd, "<body><h1>Index of %s</h1>\r\n",
		printname);
	bozo_printf(httpd,
		"<table cols=3>\r\n<thead>\r\n"
		"<tr><th>Name<th>Last modified<th align=right>Size\r\n"
		"<tbody>\r\n");

	for (j = k = scandir(dirpath, &de, NULL, alphasort), deo = de;
	    j-- > 0; de++) {
		int nostat = 0;
		char *name = (*de)->d_name;
		char *urlname, *htmlname;

		if (strcmp(name, ".") == 0 ||
		    (strcmp(name, "..") != 0 &&
		     httpd->hide_dots && name[0] == '.'))
			continue;

		if (bozo_check_special_files(request, name, false))
			continue;

		snprintf(buf, sizeof buf, "%s/%s", dirpath, name);
		if (stat(buf, &sb))
			nostat = 1;

		urlname = bozo_escape_rfc3986(httpd, name, 0);
		htmlname = bozo_escape_html(httpd, name);
		if (htmlname == NULL)
			htmlname = name;
		bozo_printf(httpd, "<tr><td>");
		if (strcmp(name, "..") == 0) {
			bozo_printf(httpd, "<a href=\"../\">");
			bozo_printf(httpd, "Parent Directory");
		} else if (!nostat && S_ISDIR(sb.st_mode)) {
			bozo_printf(httpd, "<a href=\"%s/\">", urlname);
			bozo_printf(httpd, "%s/", htmlname);
		} else if (strchr(name, ':') != NULL) {
			/* RFC 3986 4.2 */
			bozo_printf(httpd, "<a href=\"./%s\">", urlname);
			bozo_printf(httpd, "%s", htmlname);
		} else {
			bozo_printf(httpd, "<a href=\"%s\">", urlname);
			bozo_printf(httpd, "%s", htmlname);
		}
		if (htmlname != name)
			free(htmlname);
		bozo_printf(httpd, "</a>");

		if (nostat)
			bozo_printf(httpd, "<td>?<td>?\r\n");
		else {
			unsigned long long len;

			strftime(buf, sizeof buf, "%d-%b-%Y %R", gmtime(&sb.st_mtime));
			bozo_printf(httpd, "<td>%s", buf);

			len = ((unsigned long long)sb.st_size + 1023) / 1024;
			bozo_printf(httpd, "<td align=right>%llukB", len);
		}
		bozo_printf(httpd, "\r\n");
	}

	closedir(dp);
	while (k--)
        	free(deo[k]);
	free(deo);
	bozo_printf(httpd, "</table>\r\n");
	if (httpd->dir_readme != NULL) {
		if (httpd->dir_readme[0] == '/')
			snprintf(buf, sizeof buf, "%s", httpd->dir_readme);
		else
			snprintf(buf, sizeof buf, "%s/%s", dirpath, httpd->dir_readme);
		fd = open(buf, O_RDONLY);
		if (fd != -1) {
			bozo_flush(httpd, stdout);
			do {
				rlen = read(fd, buf, sizeof buf);
				if (rlen <= 0)
					break;
				bozo_write(httpd, STDOUT_FILENO, buf, rlen);
			} while (1);
			close(fd);
		}
	}
	bozo_printf(httpd, "</body></html>\r\n\r\n");
	bozo_flush(httpd, stdout);

done:
	free(file);
	free(printname);
	return 1;
}
#endif /* NO_DIRINDEX_SUPPORT */
