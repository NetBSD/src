/*	$NetBSD: citrus_iconv.c,v 1.1 2003/06/25 09:51:34 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_iconv.c,v 1.1 2003/06/25 09:51:34 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <paths.h>
#include <dirent.h>
#include <sys/types.h>

#include "citrus_namespace.h"
#include "citrus_bcs.h"
#include "citrus_region.h"
#include "citrus_memstream.h"
#include "citrus_mmap.h"
#include "citrus_module.h"
#include "citrus_lookup.h"
#include "citrus_iconv.h"

#define _CITRUS_ICONV_DIR	"iconv.dir"
#define _CITRUS_ICONV_ALIAS	"iconv.alias"

/*
 * lookup_iconv_entry:
 *	lookup iconv.dir entry in the specified directory.
 *
 * line format of iconv.dir file:
 *	key  module  arg
 * key    : lookup key.
 * module : iconv module name.
 * arg    : argument for the module (generally, description file name)
 *
 */
static __inline int
lookup_iconv_entry(const char *curdir, const char *key,
		   char *linebuf, size_t linebufsize,
		   const char **module, const char **variable)
{
	const char *cp, *cq;
	char *p, path[PATH_MAX];

	/* iconv.dir path */
	snprintf(path, PATH_MAX, "%s/" _CITRUS_ICONV_DIR, curdir);

	/* lookup db */
	cp = p = _lookup_simple(path, key, linebuf, linebufsize);
	if (p == NULL)
		return ENOENT;

	/* get module name */
	*module = p;
	cq = _bcs_skip_nonws(cp);
	p[cq-cp] = '\0';
	p += cq-cp+1;
	cq++;

	/* get variable */
	cp = _bcs_skip_ws(cq);
	*variable = p += cp - cq;
	cq = _bcs_skip_nonws(cp);
	p[cq-cp] = '\0';

	return 0;
}

/*
 * _citrus_iconv_open:
 *	open a converter for the specified in/out codes.
 */
int
_citrus_iconv_open(struct _citrus_iconv * __restrict * __restrict rci,
		   const char * __restrict basedir,
		   const char * __restrict src, const char * __restrict dst)
{
	int ret;
	struct _citrus_iconv *ci;
	_citrus_iconv_getops_t getops;
	char realsrc[PATH_MAX], realdst[PATH_MAX], key[PATH_MAX];
	char linebuf[PATH_MAX], path[PATH_MAX];
	const char *module, *variable;

	/* resolve codeset name aliases */
	snprintf(path, sizeof(path), "%s/%s", basedir, _CITRUS_ICONV_ALIAS);
	strlcpy(realsrc, _lookup_alias(path, src, linebuf, PATH_MAX), PATH_MAX);
	strlcpy(realdst, _lookup_alias(path, dst, linebuf, PATH_MAX), PATH_MAX);

	/* sanity check */
	if (strchr(realsrc, '/') != NULL || strchr(realdst, '/'))
		return EINVAL;

	/* search converter entry */
	snprintf(key, sizeof(key), "%s/%s", realsrc, realdst);
	ret = lookup_iconv_entry(basedir, key, linebuf, PATH_MAX,
				 &module, &variable);
	if (ret) {
		if (ret == ENOENT)
			/* fallback */
			ret = lookup_iconv_entry(basedir, "*",
						 linebuf, PATH_MAX,
						 &module, &variable);
		if (ret)
			return ret;
	}

	/* initialize iconv handle */
	ci = malloc(sizeof(*ci));
	if (!ci) {
		ret = errno;
		goto err;
	}
	ci->ci_module = NULL;
	ci->ci_ops = NULL;
	ci->ci_closure = NULL;

	/* load module */
	ret = _citrus_load_module(&ci->ci_module, module);
	if (ret)
		goto err;

	/* get operators */
	getops = (_citrus_iconv_getops_t)
	    _citrus_find_getops(ci->ci_module, module, "iconv");
	if (!getops) {
		ret = EOPNOTSUPP;
		goto err;
	}
	ci->ci_ops = malloc(sizeof(*ci->ci_ops));
	if (!ci->ci_ops) {
		ret = errno;
		goto err;
	}
	ret = (*getops)(ci->ci_ops, sizeof(*ci->ci_ops),
			_CITRUS_ICONV_ABI_VERSION);
	if (ret)
		goto err;

	if (!ci->ci_ops->io_init ||
	    !ci->ci_ops->io_uninit ||
	    !ci->ci_ops->io_convert)
		goto err;

	/* initialize the converter */
	ret = (*ci->ci_ops->io_init)(ci, basedir, realsrc, realdst,
				     (const void *)variable,
				     strlen(variable)+1);
	if (ret)
		goto err;

	*rci = ci;

	return 0;
err:
	_citrus_iconv_close(ci);
	return ret;
}

/*
 * _citrus_iconv_close:
 *	close the specified converter.
 */
void
_citrus_iconv_close(struct _citrus_iconv *ci)
{
	if (ci) {
		if (ci->ci_module) {
			if (ci->ci_ops) {
				if (ci->ci_closure)
					(*ci->ci_ops->io_uninit)(ci);
				free(ci->ci_ops);
			}
			_citrus_unload_module(ci->ci_module);
		}
		free(ci);
	}
}
