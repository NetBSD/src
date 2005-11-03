/*	$NetBSD: var.c,v 1.1 2005/11/03 21:16:41 dillo Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron, Thomas Klausner, and Johnny Lam.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: var.c,v 1.1 2005/11/03 21:16:41 dillo Exp $");
#endif

#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>

#include "lib.h"

static const char *var_cmp(const char *, size_t, const char *, size_t);
static void var_print(FILE *, const char *, const char *);

char *
var_get(const char *fname, const char *variable)
{
	FILE   *fp;
	char   *line;
	size_t  len;
	size_t  varlen;
	char   *value;
	size_t  valuelen;
	size_t  thislen;
	const char *p;

	varlen = strlen(variable);
	if (varlen == 0)
		return NULL;

	fp = fopen(fname, "r");
	if (!fp) {
		if (errno != ENOENT)
			warn("var_get: can't open '%s' for reading", fname);
		return NULL;
	}

	value = NULL;
	valuelen = 0;
	
	while ((line = fgetln(fp, &len)) != (char *) NULL) {
		if (line[len - 1] == '\n')
			--len;
		if ((p=var_cmp(line, len, variable, varlen)) == NULL)
			continue;

		thislen = line+len - p;
		if (value) {
			value = realloc(value, valuelen+thislen+2);
			value[valuelen++] = '\n';
		}
		else {
			value = malloc(thislen+1);
		}
		sprintf(value+valuelen, "%.*s", thislen, p);
		valuelen += thislen;
	}
	(void) fclose(fp);
	return value;
}

int
var_set(const char *fname, const char *variable, const char *value)
{
	FILE   *fp;
	FILE   *fout;
	char   *tmpname;
	int     fd;
	char   *line;
	size_t  len;
	size_t  varlen;
	Boolean done;
	struct stat st;

	varlen = strlen(variable);
	if (varlen == 0)
		return 0;

	fp = fopen(fname, "r");
	if (!fp && errno != ENOENT) {
		warn("var_set: can't open '%s' for reading", fname);
		return -1;
	}

	tmpname = malloc(strlen(fname)+8);
	sprintf(tmpname, "%s.XXXXXX", fname);
	if ((fd=mkstemp(tmpname)) < 0) {
		free(tmpname);
		warn("var_set: can't open temp file for '%s' for writing",
		      fname);
		return -1;
	}
	if (chmod(tmpname, 0644) < 0) {
		close(fd);
		free(tmpname);
		warn("var_set: can't set permissions for temp file for '%s'",
		      fname);
		return -1;
	}
	if ((fout=fdopen(fd, "w")) == NULL) {
		close(fd);
		remove(tmpname);
		free(tmpname);
		warn("var_set: can't open temp file for '%s' for writing",
		      fname);
		return -1;
	}

	done = FALSE;

	if (fp) {
		while ((line = fgetln(fp, &len)) != (char *) NULL) {
			if (var_cmp(line, len, variable, varlen) == NULL)
				fprintf(fout, "%.*s", len, line);
			else {
				if (!done && value) {
					var_print(fout, variable, value);
					done = TRUE;
				}
			}
		}
		(void) fclose(fp);
	}

	if (!done && value)
		var_print(fout, variable, value);

	if (fclose(fout) < 0) {
		free(tmpname);
		warn("var_set: write error for '%s'", fname);
		return -1;
	}

	if (stat(tmpname, &st) < 0) {
		free(tmpname);
		warn("var_set: cannot stat tempfile for '%s'", fname);
		return -1;
	}

	if (st.st_size == 0) {
		if (remove(tmpname) < 0) {
			free(tmpname);
			warn("var_set: cannot remove tempfile for '%s'",
			     fname);
			return -1;
		}
		free(tmpname);
		if (remove(fname) < 0) {
			warn("var_set: cannot remove '%s'", fname);
			return -1;
		}
		return 0;
	}

	if (rename(tmpname, fname) < 0) {
		free(tmpname);
		warn("var_set: cannot move tempfile to '%s'", fname);
		return -1;
	}
	free(tmpname);
	return 0;
}

static const char *
var_cmp(const char *line, size_t linelen, const char *var, size_t varlen)
{
	/*
	 * We expect lines to look like one of the following
	 * forms:
	 *      VAR=value
	 *      VAR= value
	 * We print out the value of VAR, or nothing if it
	 * doesn't exist.
	 */
	if (linelen < varlen+1)
		return NULL;
	if (strncmp(var, line, varlen) != 0)
		return NULL;
	
	line += varlen;
	if (*line != '=')
		return NULL;

	++line;
	linelen -= varlen+1;
	if (linelen > 0 && *line == ' ')
		++line;
	return line;
}

static void
var_print(FILE *f, const char *variable, const char *value)
{
	const char *p;

	while ((p=strchr(value, '\n')) != NULL) {
		if (p != value)
			fprintf(f, "%s=%.*s\n", variable, p-value, value);
		value = p+1;
	}

	if (*value)
		fprintf(f, "%s=%s\n", variable, value);
}
