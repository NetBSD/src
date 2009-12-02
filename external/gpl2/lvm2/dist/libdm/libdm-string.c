/*	$NetBSD: libdm-string.c,v 1.1.1.2 2009/12/02 00:26:08 haad Exp $	*/

/*
 * Copyright (C) 2006-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include "libdevmapper.h"

#include <ctype.h>

/*
 * consume characters while they match the predicate function.
 */
static char *_consume(char *buffer, int (*fn) (int))
{
	while (*buffer && fn(*buffer))
		buffer++;

	return buffer;
}

static int _isword(int c)
{
	return !isspace(c);
}

/*
 * Split buffer into NULL-separated words in argv.
 * Returns number of words.
 */
int dm_split_words(char *buffer, unsigned max,
		   unsigned ignore_comments __attribute((unused)),
		   char **argv)
{
	unsigned arg;

	for (arg = 0; arg < max; arg++) {
		buffer = _consume(buffer, isspace);
		if (!*buffer)
			break;

		argv[arg] = buffer;
		buffer = _consume(buffer, _isword);

		if (*buffer) {
			*buffer = '\0';
			buffer++;
		}
	}

	return arg;
}

/*
 * Remove hyphen quoting from a component of a name.
 * NULL-terminates the component and returns start of next component.
 */
static char *_unquote(char *component)
{
	char *c = component;
	char *o = c;
	char *r;

	while (*c) {
		if (*(c + 1)) {
			if (*c == '-') {
				if (*(c + 1) == '-')
					c++;
				else
					break;
			}
		}
		*o = *c;
		o++;
		c++;
	}

	r = (*c) ? c + 1 : c;
	*o = '\0';

	return r;
}

int dm_split_lvm_name(struct dm_pool *mem, const char *dmname,
		      char **vgname, char **lvname, char **layer)
{
	if (mem && !(*vgname = dm_pool_strdup(mem, dmname)))
		return 0;

	_unquote(*layer = _unquote(*lvname = _unquote(*vgname)));

	return 1;
}

/*
 * On error, up to glibc 2.0.6, snprintf returned -1 if buffer was too small;
 * From glibc 2.1 it returns number of chars (excl. trailing null) that would 
 * have been written had there been room.
 *
 * dm_snprintf reverts to the old behaviour.
 */
int dm_snprintf(char *buf, size_t bufsize, const char *format, ...)
{
	int n;
	va_list ap;

	va_start(ap, format);
	n = vsnprintf(buf, bufsize, format, ap);
	va_end(ap);

	if (n < 0 || ((unsigned) n + 1 > bufsize))
		return -1;

	return n;
}

char *dm_basename(const char *path)
{
	char *p = strrchr(path, '/');

	return p ? p + 1 : (char *) path;
}

int dm_asprintf(char **result, const char *format, ...)
{
	int n, ok = 0, size = 32;
	va_list ap;
	char *buf = dm_malloc(size);

	*result = 0;

	if (!buf)
		return -1;

	while (!ok) {
		va_start(ap, format);
		n = vsnprintf(buf, size, format, ap);
		if (0 <= n && n < size)
			ok = 1;
		else {
			dm_free(buf);
			size *= 2;
			buf = dm_malloc(size);
			if (!buf)
				return -1;
		};
		va_end(ap);
	}

	*result = dm_strdup(buf);
	dm_free(buf);
	return n + 1;
}
