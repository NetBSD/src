/*	$NetBSD: setrunelocale.c,v 1.2 2000/12/21 11:29:48 itojun Exp $	*/

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
__RCSID("$NetBSD: setrunelocale.c,v 1.2 2000/12/21 11:29:48 itojun Exp $");
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

static int
loadrunemodule(_RuneLocale *rl, void **rhandle)
{
	char name[PATH_MAX];
	int ret;
        int (*init) __P((_RuneLocale *));
        int (*initstream) __P((_RuneLocale *));

	if (_PathModule == NULL) {
		char *p = getenv("PATH_LOCALEMODULE");
#ifndef RUNEMOD_MAJOR
		/* "/libencoding.so" */
		const size_t offset = 4 + sizeof(rl->__encoding) + 3;
#else
		/* "/libencoding.so.###" */
		const size_t offset = 4 + sizeof(rl->__encoding) + 7;
#endif

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
#ifdef RUNEMOD_MAJOR
        (void) snprintf(name, sizeof(name), "%s/lib%s.so.%d",
                       _PathModule, rl->__encoding, RUNEMOD_MAJOR);
#else
        (void) snprintf(name, sizeof(name), "%s/lib%s.so",
                       _PathModule, rl->__encoding);
#endif
	*rhandle = dlopen(name, RTLD_LAZY);
	if (!*rhandle)
		return(EINVAL);
#ifdef DLRUNE_AOUT
	snprintf(name, sizeof(name), "__%s_init", rl->__encoding);
#else
	snprintf(name, sizeof(name), "_%s_init", rl->__encoding);
#endif
	init = (int (*)__P((_RuneLocale *))) dlsym(*rhandle, name);
	if (!init) {
		ret = EINVAL;
		goto fail;
	}
	ret = (*init)(rl);
	if (ret)
		goto fail;
#ifdef DLRUNE_AOUT
	snprintf(name, sizeof(name), "__%s_init_stream", rl->__encoding);
#else
	snprintf(name, sizeof(name), "_%s_init_stream", rl->__encoding);
#endif
	initstream = (int (*)__P((_RuneLocale *))) dlsym(*rhandle, name);
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
