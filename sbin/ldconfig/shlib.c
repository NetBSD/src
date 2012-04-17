/*	$NetBSD: shlib.c,v 1.1.6.1 2012/04/17 00:05:40 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifdef sun
char	*strsep();
int	isdigit();
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/exec_aout.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <a.out.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <link_aout.h>

#include "shlib.h"

/*
 * Standard directories to search for files specified by -l.
 */
#ifndef STANDARD_SEARCH_DIRS
#define	STANDARD_SEARCH_DIRS	"/usr/lib"
#endif

static	void	add_search_dir(const char *);

/*
 * Actual vector of library search directories,
 * including `-L'ed and LD_LIBRARY_PATH spec'd ones.
 */
char	 **search_dirs;
int	n_search_dirs;

const char	*standard_search_dirs[] = {
	STANDARD_SEARCH_DIRS
};

static void
add_search_dir(const char *name)
{
	n_search_dirs += 2;
	search_dirs = (char **)
		xrealloc(search_dirs, n_search_dirs * sizeof search_dirs[0]);
	search_dirs[n_search_dirs - 2] = strdup(name);
	search_dirs[n_search_dirs - 1] =
	    xmalloc(sizeof(_PATH_EMUL_AOUT) + strlen(name));
	strcpy(search_dirs[n_search_dirs - 1], _PATH_EMUL_AOUT);
	strcat(search_dirs[n_search_dirs - 1], name);
}

void
std_search_path(void)
{
	int	i, n;

	/* Append standard search directories */
	n = sizeof standard_search_dirs / sizeof standard_search_dirs[0];
	for (i = 0; i < n; i++)
		add_search_dir(standard_search_dirs[i]);
}

/*
 * Return true if CP points to a valid dewey number.
 * Decode and leave the result in the array DEWEY.
 * Return the number of decoded entries in DEWEY.
 */

int
getdewey(int dewey[], char *cp)
{
	int	i, n;

	for (n = 0, i = 0; i < MAXDEWEY; i++) {
		if (*cp == '\0')
			break;

		if (*cp == '.') cp++;
#ifdef SUNOS_LIB_COMPAT
		if (!(isdigit)(*cp))
#else
		if (!isdigit((unsigned char)*cp))
#endif
			return 0;

		dewey[n++] = strtol(cp, &cp, 10);
	}

	return n;
}

/*
 * Compare two dewey arrays.
 * Return -1 if `d1' represents a smaller value than `d2'.
 * Return  1 if `d1' represents a greater value than `d2'.
 * Return  0 if equal.
 */
int
cmpndewey(int d1[], int n1, int d2[], int n2)
{
	register int	i;

	for (i = 0; i < n1 && i < n2; i++) {
		if (d1[i] < d2[i])
			return -1;
		if (d1[i] > d2[i])
			return 1;
	}

	if (n1 == n2)
		return 0;

	if (i == n1)
		return -1;

	if (i == n2)
		return 1;

	errx(1, "cmpndewey: cant happen");
	return 0;
}


/*
 * Utility functions shared with others.
 */


/*
 * Like malloc but get fatal error if memory is exhausted.
 */
void *
xmalloc(size_t size)
{
	void	*result = (void *)malloc(size);

	if (!result)
		errx(1, "virtual memory exhausted");

	return (result);
}

/*
 * Like realloc but get fatal error if memory is exhausted.
 */
void *
xrealloc(void *ptr, size_t size)
{
	void	*result;

	result = (ptr == NULL) ? malloc(size) : realloc(ptr, size);
	if (result == NULL)
		errx(1, "virtual memory exhausted");

	return (result);
}

/*
 * Return a newly-allocated string whose contents concatenate
 * the strings S1, S2, S3.
 */
char *
concat(const char *s1, const char *s2, const char *s3)
{
	int	len1 = strlen(s1),
		len2 = strlen(s2),
		len3 = strlen(s3);

	char *result = (char *)xmalloc(len1 + len2 + len3 + 1);

	strcpy(result, s1);
	strcpy(result + len1, s2);
	strcpy(result + len1 + len2, s3);
	result[len1 + len2 + len3] = 0;

	return (result);
}

