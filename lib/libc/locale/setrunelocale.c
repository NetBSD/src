/*	$NetBSD: setrunelocale.c,v 1.3 2000/12/21 18:20:03 itojun Exp $	*/

/*-
 * Copyright (c)1999 Citrus Project,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: setrunelocale.c,v 1.3 2000/12/21 18:20:03 itojun Exp $");
#endif /* LIBC_SCCS and not lint */

#include "rune.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <locale.h>
#include "rune_local.h"

#ifdef DLRUNE
#include <sys/types.h>
#include <dirent.h>

static int getdewey __P((int [], char *));
static int cmpndewey __P((int [], int, int [], int));
static const char *findshlib __P((char *, int *, int *));
static void *findfunc __P((void *, const char *));
static int loadrunemodule __P((_RuneLocale *, void **));
#endif

/* locale handlers */
extern int		_none_init __P((_RuneLocale *));
#ifndef DLRUNE
extern int		_UTF2_init __P((_RuneLocale *));
extern int		_UTF8_init __P((_RuneLocale *));
extern int		_EUC_init __P((_RuneLocale *));
extern int		_EUCTW_init __P((_RuneLocale *));
extern int		_BIG5_init __P((_RuneLocale *));
extern int		_MSKanji_init __P((_RuneLocale *));
extern int		_ISO2022_init __P((_RuneLocale *));
extern int		_ISO2022_init_stream __P((_RuneLocale *));
#endif

struct localetable {
	char path[PATH_MAX];
	_RuneLocale *runelocale;
	struct localetable *next;
};
static struct localetable localetable;

#ifdef DLRUNE
static char *_PathModule = NULL;

/* from libexec/ld.aout_so/shlib.c */
#undef major
#undef minor
#define MAXDEWEY	3	/*ELF*/

static int
getdewey(dewey, cp)
int	dewey[];
char	*cp;
{
	int	i, n;

	for (n = 0, i = 0; i < MAXDEWEY; i++) {
		if (*cp == '\0')
			break;

		if (*cp == '.') cp++;
		if (*cp < '0' || '9' < *cp)
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
static int
cmpndewey(d1, n1, d2, n2)
int	d1[], d2[];
int	n1, n2;
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

	/* XXX cannot happen */
	return 0;
}

static const char *
findshlib(name, majorp, minorp)
char	*name;
int	*majorp, *minorp;
{
	int		dewey[MAXDEWEY];
	int		ndewey;
	int		tmp[MAXDEWEY];
	int		i;
	int		len;
	char		*lname;
	static char	path[PATH_MAX];
	int		major = *majorp, minor = *minorp;
	const char	*search_dirs[1];
	const int	n_search_dirs = 1;

	path[0] = '\0';
	search_dirs[0] = _PathModule;
	len = strlen(name);
	lname = name;

	ndewey = 0;

	for (i = 0; i < n_search_dirs; i++) {
		DIR		*dd = opendir(search_dirs[i]);
		struct dirent	*dp;
		int		found_dot_a = 0;
		int		found_dot_so = 0;

		if (dd == NULL)
			continue;

		while ((dp = readdir(dd)) != NULL) {
			int	n;

			if (dp->d_namlen < len + 4)
				continue;
			if (strncmp(dp->d_name, lname, len) != 0)
				continue;
			if (strncmp(dp->d_name+len, ".so.", 4) != 0)
				continue;

			if ((n = getdewey(tmp, dp->d_name+len+4)) == 0)
				continue;

			if (major != -1 && found_dot_a)
				found_dot_a = 0;

			/* XXX should verify the library is a.out/ELF? */

			if (major == -1 && minor == -1) {
				goto compare_version;
			} else if (major != -1 && minor == -1) {
				if (tmp[0] == major)
					goto compare_version;
			} else if (major != -1 && minor != -1) {
				if (tmp[0] == major) {
					if (n == 1 || tmp[1] >= minor)
						goto compare_version;
				}
			}

			/* else, this file does not qualify */
			continue;

		compare_version:
			if (cmpndewey(tmp, n, dewey, ndewey) <= 0)
				continue;

			/* We have a better version */
			found_dot_so = 1;
			snprintf(path, sizeof(path), "%s/%s", search_dirs[i],
			    dp->d_name);
			found_dot_a = 0;
			bcopy(tmp, dewey, sizeof(dewey));
			ndewey = n;
			*majorp = dewey[0];
			*minorp = dewey[1];
		}
		closedir(dd);

		if (found_dot_a || found_dot_so)
			/*
			 * There's a lib in this dir; take it.
			 */
			return path[0] ? path : NULL;
	}

	return path[0] ? path : NULL;
}

static void *
findfunc(handle, sym)
	void *handle;
	const char *sym;
{
	char name[PATH_MAX];
	void *p;

	p = dlsym(handle, sym);
	if (!p) {
		/* a.out case */
		snprintf(name, sizeof(name), "_%s", sym);
		p = dlsym(handle, sym);
	}
	return p;
}

static int
loadrunemodule(_RuneLocale *rl, void **rhandle)
{
	const char *p;
	char namebase[PATH_MAX], name[PATH_MAX];
	int maj, min;
	int ret;
        int (*init) __P((_RuneLocale *));
        int (*initstream) __P((_RuneLocale *));

	if (_PathModule == NULL) {
		/* "/libencoding.so.###" */
		const size_t offset = 4 + sizeof(rl->__encoding) + 7;

		p = getenv("PATH_LOCALEMODULE");
		if (p != NULL && !issetugid()) {
			if (strlen(p) + offset + 1 >= PATH_MAX)
				return EFAULT;
			_PathModule = strdup(p);
			if (_PathModule == NULL)
				return ENOMEM;
		} else
			_PathModule = _PATH_LOCALEMODULE;
	}
	/* Range checking not needed, encoding length already checked above */
	(void)snprintf(namebase, sizeof(namebase), "lib%s", rl->__encoding);
	maj = RUNEMOD_MAJOR;
	min = -1;
	p = findshlib(namebase, &maj, &min);
	if (!p)
		return (EINVAL);
	*rhandle = dlopen(p, RTLD_LAZY);
	if (!*rhandle)
		return(EINVAL);
	snprintf(name, sizeof(name), "_%s_init", rl->__encoding);
	init = (int (*)__P((_RuneLocale *))) findfunc(*rhandle, name);
	dlsym(*rhandle, name);
	if (!init) {
		ret = EINVAL;
		goto fail;
	}
	ret = (*init)(rl);
	if (ret)
		goto fail;
	snprintf(name, sizeof(name), "_%s_init_stream", rl->__encoding);
	initstream = (int (*)__P((_RuneLocale *))) findfunc(*rhandle, name);
	if (initstream) {
		ret = (*initstream)(rl);
		if (ret)
			goto fail;
	}

	return(ret);

fail:
	dlclose(*rhandle);
	*rhandle = NULL;
	return ret;
}
#endif /* DLRUNE */

_RuneLocale *
_findrunelocale(path)
	char *path;
{
	struct localetable *lt;

	/* ones which we have seen already */
	for (lt = localetable.next; lt; lt = lt->next)
		if (strcmp(path, lt->path) == 0)
			return lt->runelocale;

	return NULL;
}

int
_newrunelocale(path)
	char *path;
{
	struct localetable *lt;
	FILE *fp;
	_RuneLocale *rl;
        int ret;

	ret = 0;

	if (!path || strlen(path) + 1 > sizeof(lt->path))
		return EFAULT;

	rl = _findrunelocale(path);
	if (rl)
		return 0;

	if ((fp = fopen(path, "r")) == NULL)
		return ENOENT;

	if ((rl = _Read_RuneMagi(fp)) != NULL)
		goto found;
	/* necessary for backward compatibility */
	if ((rl = _Read_CTypeAsRune(fp)) != NULL)
		goto found;

	fclose(fp);
	return EFTYPE;

found:
	fclose(fp);
	if (_StreamStateTable) {
		free(_StreamStateTable);
		_StreamStateTable = NULL;
	}

	if (!rl->__encoding[0])
		ret = EINVAL;
	else if (!strcmp(rl->__encoding, "NONE"))
		ret = _none_init(rl);
#ifdef DLRUNE
	else
		ret = loadrunemodule(rl, &rl->__rune_RuneModule);

	if (ret) {
		if (rl->__rune_RuneModule)
			dlclose(rl->__rune_RuneModule);
		_NukeRune(rl);
		return ret;
	}

	if (__MB_LEN_MAX_RUNTIME < __mb_cur_max) {
		if (rl->__rune_RuneModule)
			dlclose(rl->__rune_RuneModule);
		_NukeRune(rl);
		return ret;
	}
#else
#if 0
	else if (!strcmp(rl->__encoding, "EUC"))
		ret = _EUC_init(rl);
	else if (!strcmp(rl->__encoding, "BIG5"))
		ret = _BIG5_init(rl);
	else if (!strcmp(rl->__encoding, "MSKanji"))
		ret = _MSKanji_init(rl);
	else if (!strcmp(rl->__encoding, "ISO2022")) {
		ret = _ISO2022_init(rl);

		if (!ret)
			ret = _ISO2022_init_stream(rl);
	}
	else if (!strcmp(rl->__encoding, "UTF2"))
		ret = _UTF2_init(rl);
	else if (!strcmp(rl->__encoding, "UTF8"))
		ret = _UTF8_init(rl);
	else if (!strcmp(rl->__encoding, "EUCTW"))
		ret = _EUCTW_init(rl);
#endif
	else
		ret = EINVAL;
#endif

	if (ret) {
		_NukeRune(rl);
		return ret;
	}

	/* register it */
	lt = malloc(sizeof(struct localetable));
	strlcpy(lt->path, path, sizeof(lt->path));
	lt->runelocale = rl;
	lt->next = localetable.next;
	localetable.next = lt;

	return 0;
}

#if 0
static int
delrunelocale(path)
	char *path;
{
	struct localetable *lt, *prev;
	_RuneLocale *rl;

	prev = NULL;
	for (lt = localetable.next; lt; prev = lt, lt = prev->next)
		if (strcmp(path, lt->path) == 0)
			break;
	if (!lt)
		return ENOENT;

	/* remove runelocale registeration */
	if (prev)
		prev->next = lt->next;
	else
		localetable.next = lt->next;
	lt->next = NULL;
	free(lt);

	/* cleanup everything */
	rl = lt->runelocale;
	dlclose(rl->__rune_RuneModule);
	rl->__rune_RuneModule = NULL;
	_NukeRune(rl);

	return 0;
}
#endif

int
_xpg4_setrunelocale(encoding)
	char *encoding;
{
	char path[PATH_MAX];
	_RuneLocale *rl;
	int error;

	if (!strcmp(encoding, "C") || !strcmp(encoding, "POSIX")) {
		rl = &_DefaultRuneLocale;
		goto found;
	}

	snprintf(path, sizeof(path), "%s/%s/LC_CTYPE", _PathLocale, encoding);

	error = _newrunelocale(path);
	if (error)
		return error;
	rl = _findrunelocale(path);
	if (!rl)
		return ENOENT;

found:
	_CurrentRuneLocale = rl;
	__mb_cur_max = (__mb_cur_max > rl->__rune_mb_cur_max)
	    ? __mb_cur_max : rl->__rune_mb_cur_max;
	return 0;
}
