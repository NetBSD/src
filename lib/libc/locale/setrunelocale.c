/*	$NetBSD: setrunelocale.c,v 1.12 2002/04/17 13:40:35 kleink Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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
__RCSID("$NetBSD: setrunelocale.c,v 1.12 2002/04/17 13:40:35 kleink Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include "rune.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <locale.h>
#include <citrus/citrus_module.h>
#include <citrus/citrus_ctype.h>
#include "rune_local.h"
#include "multibyte.h"

struct localetable {
	char path[PATH_MAX];
	_RuneLocale *runelocale;
	struct localetable *next;
};
static struct localetable *localetable_head;

_RuneLocale *
_findrunelocale(path)
	char *path;
{
	struct localetable *lt;

	_DIAGASSERT(path != NULL);

	/* ones which we have seen already */
	for (lt = localetable_head; lt; lt = lt->next)
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

	/* path may be NULL (actually, it's checked below) */

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

	rl->rl_citrus_ctype = NULL;
	ret = _citrus_ctype_open(&rl->rl_citrus_ctype, rl->rl_encoding,
				 rl->rl_variable, rl->rl_variable_len,
				 _PRIVSIZE);
	if (ret) {
		_NukeRune(rl);
		return ret;
	}
	if (__MB_LEN_MAX_RUNTIME <
	    _citrus_ctype_get_mb_cur_max(rl->rl_citrus_ctype)) {
		_NukeRune(rl);
		return EINVAL;
	}

	/* register it */
	lt = malloc(sizeof(struct localetable));
	if (lt == NULL) {
		_NukeRune(rl);
		return ENOMEM;
	}
	strlcpy(lt->path, path, sizeof(lt->path));
	lt->runelocale = rl;
	lt->next = localetable_head;
	localetable_head = lt;

	return 0;
}

#if 0
static int
delrunelocale(path)
	char *path;
{
	struct localetable *lt, *prev;
	_RuneLocale *rl;

	_DIAGASSERT(path != NULL);

	prev = NULL;
	for (lt = localetable_head; lt; prev = lt, lt = prev->next)
		if (strcmp(path, lt->path) == 0)
			break;
	if (!lt)
		return ENOENT;

	/* remove runelocale registeration */
	if (prev)
		prev->next = lt->next;
	else
		localetable_head = lt->next;
	lt->next = NULL;
	free(lt);

	/* cleanup everything */
	rl = lt->runelocale;
	dlclose(rl->rl_rune_RuneModule);
	rl->rl_rune_RuneModule = NULL;
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

	_DIAGASSERT(encoding != NULL);

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
	__mb_cur_max = _citrus_ctype_get_mb_cur_max(rl->rl_citrus_ctype);

	return 0;
}
