/*	Id: path.c$	/	
/*	$NetBSD: path.c,v 1.1.1.1 2016/02/09 20:29:12 plunky Exp $/

/*-
 * Copyright (c) 2011 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "driver.h"

static void
expand_sysroot(void)
{
	struct string *s;
	struct strlist *lists[] = { &crtdirs, &sysincdirs, &incdirs,
	    &user_sysincdirs, &libdirs, &progdirs, NULL };
	const char *sysroots[] = { sysroot, isysroot, isysroot, isysroot,
	    sysroot, sysroot, NULL };
	size_t i, sysroot_len, value_len;
	char *path;

	assert(sizeof(lists) / sizeof(lists[0]) ==
	       sizeof(sysroots) / sizeof(sysroots[0]));

	for (i = 0; lists[i] != NULL; ++i) {
		STRLIST_FOREACH(s, lists[i]) {
			if (s->value[0] != '=')
				continue;
			sysroot_len = strlen(sysroots[i]);
			/* Skipped '=' compensates additional space for '\0' */
			value_len = strlen(s->value);
			path = xmalloc(sysroot_len + value_len);
			memcpy(path, sysroots[i], sysroot_len);
			memcpy(path + sysroot_len, s->value + 1, value_len);
			free(s->value);
			s->value = path;
		}
	}
}

static char *
find_file(const char *file, struct strlist *path, int mode)
{
	struct string *s;
	char *f;
	size_t lf, lp;
	int need_sep;

	lf = strlen(file);
	STRLIST_FOREACH(s, path) {
		lp = strlen(s->value);
		need_sep = (lp && s->value[lp - 1] != '/') ? 1 : 0;
		f = xmalloc(lp + lf + need_sep + 1);
		memcpy(f, s->value, lp);
		if (need_sep)
			f[lp] = '/';
		memcpy(f + lp + need_sep, file, lf + 1);
		if (access(f, mode) == 0)
			return f;
		free(f);
	}
	return xstrdup(file);
}

static char *
output_name(const char *file, const char *new_suffix, int counter, int last)
{
	const char *old_suffix;
	char *name;
	size_t lf, ls, len;
	int counter_len;

	if (last && final_output)
		return xstrdup(final_output);

	old_suffix = strrchr(file, '.');
	if (old_suffix != NULL && strchr(old_suffix, '/') != NULL)
		old_suffix = NULL;
	if (old_suffix == NULL)
		old_suffix = file + strlen(file);

	ls = strlen(new_suffix);
	if (save_temps || last) {
		lf = old_suffix - file;
		name = xmalloc(lf + ls + 1);
		memcpy(name, file, lf);
		memcpy(name + lf, new_suffix, ls + 1);
		return name;
	}
	if (temp_directory == NULL) {
		const char *template;
		char *path;
		size_t template_len;
		int need_sep;

		template = getenv("TMPDIR");
		if (template == NULL)
			template = "/tmp";
		template_len = strlen(template);
		if (template_len && template[template_len - 1] == '/')
			need_sep = 0;
		else
			need_sep = 1;
		path = xmalloc(template_len + need_sep + 6 + 1);
		memcpy(path, template, template_len);
		if (need_sep)
			path[template_len] = '/';
		memcpy(path + template_len + need_sep, "pcc-XXXXXX", 11);
		if (mkdtemp(path) == NULL)
			error("mkdtemp failed: %s", strerror(errno));
		temp_directory = path;
	}
	lf = strlen(temp_directory);
	counter_len = snprintf(NULL, 0, "%d", counter);
	if (counter_len < 1)
		error("snprintf failure");
	len = lf + 1 + (size_t)counter_len + ls + 1;
	name = xmalloc(len);
	snprintf(name, len, "%s/%d%s", temp_directory, counter, new_suffix);
	strlist_add(&temp_outputs, name);
	return name;
}
