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

#if !defined(__FreeBSD__)
#define _BSD_RUNE_T_    int
#define _BSD_CT_RUNE_T_ _rune_t
extern int __fl_rune_singlebyte;
#include "rune.h"
#else
#include <rune.h>
#endif
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include "setlocale.h"
#define	LC_CTYPE	2	/* XXX */

extern int		_none_init __P((_RuneLocale *));
#ifndef DLRUNE
extern int		_UTF2_init __P((_RuneLocale *));
extern int		_EUC_init __P((_RuneLocale *));
extern int		_BIG5_init __P((_RuneLocale *));
extern int		_MSKanji_init __P((_RuneLocale *));
extern int		_ISO2022_init __P((_RuneLocale *));
#endif /* !DLRUNE */
extern int              _xpg4_setrunelocale __P((char *));
extern _RuneLocale      *_Read_RuneMagi __P((FILE *));

static void *_CurrentRuneModule = NULL;
static char *_PathModule = NULL;

#ifdef DLRUNE
static int
loadrunemodule(_RuneLocale *rl, void **rhandle)
{
	char name[PATH_MAX];
	int ret;
        int (*init) __P((_RuneLocale *));
	if (_PathModule == NULL) {
		char *p = getenv("PATH_LOCALEMODULE");

		if (p != NULL && !issetugid()) {
			if (strlen(p) + 4/*"/lib"*/ + sizeof(rl->__encoding) +
			    3/*".so"*/
#ifdef RUNEMOD_MAJOR
                            +4/*".###"*/
#endif
                            >= PATH_MAX)
				return(EFAULT);
			_PathModule = strdup(p);
			if (_PathModule == NULL)
				return (errno);
		} else
			_PathModule = _PATH_LOCALEMODULE;
	}
	/* Range checking not needed, encoding length already checked above */
#ifdef RUNEMOD_MAJOR
        (void) sprintf(name, "%s/lib%s.so.%d",
                       _PathModule, rl->__encoding, RUNEMOD_MAJOR);
#else
        (void) sprintf(name, "%s/lib%s.so",
                       _PathModule, rl->__encoding);
#endif
	*rhandle=dlopen(name, RTLD_LAZY);
	if ( !*rhandle ) {
		return(EINVAL);
        }
#ifdef DLRUNE_AOUT
	(void) strcpy(name, "__");
#else
	(void) strcpy(name, "_");
#endif
	(void) strcat(name, rl->__encoding);
	(void) strcat(name, "_init");
	init = (int (*)__P((_RuneLocale *))) dlsym(*rhandle, name);
	if ( !init ) {
		dlclose(*rhandle);
		return(EINVAL);
	}
	ret = (*init)(rl);
	if ( ret )
		dlclose(*rhandle);
	return(ret);
}
#endif /* DLRUNE */

int
#if defined(__FreeBSD__)
setrunelocale(encoding)
#else
_xpg4_setrunelocale(encoding)
#endif
	char *encoding;
{
	FILE *fp;
	char name[PATH_MAX];
	_RuneLocale *rl;
        int ret;
	void *module = NULL;

	if (!encoding || strlen(encoding) > ENCODING_LEN)
	    return(EFAULT);

	/*
	 * The "C" and "POSIX" locale are always here.
	 */
	if (!strcmp(encoding, "C") || !strcmp(encoding, "POSIX")) {
		_CurrentRuneLocale = &_DefaultRuneLocale;
		_CurrentRuneState = &_DefaultRuneState;
		goto success;
	}

	if (_PathLocale == NULL) {
		char *p = getenv("PATH_LOCALE");

		if (p != NULL && !issetugid()) {
			if (strlen(p) + 1/*"/"*/ + ENCODING_LEN +
			    1/*"/"*/ + CATEGORY_LEN >= PATH_MAX)
				return(EFAULT);
			_PathLocale = strdup(p);
			if (_PathLocale == NULL)
				return (errno);
			_PathLocaleUnshared = _PathLocale;
		} else {
			_PathLocale = _PATH_LOCALE;
			_PathLocaleUnshared = _PATH_LOCALE_UNSHARED;
		}
	}
	/* Range checking not needed, encoding length already checked above */
	(void) strcpy(name, *_LocalePaths[LC_CTYPE]);
	(void) strcat(name, "/");
	(void) strcat(name, encoding);
	(void) strcat(name, "/LC_CTYPE");

	if ((fp = fopen(name, "r")) == NULL)
		return(ENOENT);

	if ((rl = _Read_RuneMagi(fp)) == 0) {
		fclose(fp);
		return(EFTYPE);
	}
	fclose(fp);

	if (_StreamStateTable) {
		free(_StreamStateTable);
		_StreamStateTable = NULL;
	}

	if (!rl->__encoding[0])
		return(EINVAL);
	else if (!strcmp(rl->__encoding, "NONE")) {
		ret = _none_init(rl);
        }
#ifdef DLRUNE
        else
#if !defined(__FreeBSD__)
	if ( __fl_rune_singlebyte )
		ret = EINVAL;
	else
#endif
		ret = loadrunemodule(rl, &module);
#else
	else if (!strcmp(rl->encoding, "EUC"))
		ret = _EUC_init(rl);
	else if (!strcmp(rl->encoding, "BIG5"))
		ret = _BIG5_init(rl);
	else if (!strcmp(rl->encoding, "MSKanji"))
		ret = _MSKanji_init(rl);
	else if (!strcmp(rl->encoding, "ISO2022"))
		ret = _ISO2022_init(rl);
	else if (!strcmp(rl->encoding, "UTF2"))
		ret = _UTF2_init(rl);
	else if (!strcmp(rl->encoding, "EUCTW"))
		ret = _EUCTW_init(rl);
	else
		ret = EINVAL;
#endif

	if ( ret )
		return(ret);

success:
#ifdef DLRUNE
	if ( _CurrentRuneModule )
		dlclose(_CurrentRuneModule);
	_CurrentRuneModule = module;
#endif
	return 0;
}
