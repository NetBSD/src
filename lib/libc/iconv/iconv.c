/*	$NetBSD: iconv.c,v 1.1 2003/06/27 05:21:53 tshiozak Exp $	*/

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
__RCSID("$NetBSD: iconv.c,v 1.1 2003/06/27 05:21:53 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <errno.h>
#include <paths.h>

#include <iconv.h>

#ifdef CITRUS_ICONV
#include <sys/types.h>
#include <citrus/citrus_types.h>
#include <citrus/citrus_module.h>
#include <citrus/citrus_esdb.h>
#include <citrus/citrus_iconv.h>

#define ISBADF(_h_)	(!(_h_) || (_h_) == (iconv_t)-1)

#ifdef __weak_alias
__weak_alias(iconv, _iconv)
__weak_alias(iconv_open, _iconv_open)
__weak_alias(iconv_close, _iconv_close)
#endif

iconv_t
_iconv_open(const char *out, const char *in)
{
	int ret;
	struct _citrus_iconv *handle;

	ret = _citrus_iconv_open(&handle, _PATH_ICONV, in, out);
	if (ret) {
		errno = ret;
		return ((iconv_t)-1);
	}

	return ((iconv_t)handle);
}

int
_iconv_close(iconv_t handle)
{
	if (ISBADF(handle)) {
		errno = EBADF;
		return (-1);
	}

	_citrus_iconv_close((struct _citrus_iconv *)handle);

	return (0);
}

size_t
_iconv(iconv_t handle, const char **in, size_t *szin, char **out, size_t *szout)
{
	int err;
	size_t ret;

	if (ISBADF(handle)) {
		errno = EBADF;
		return ((size_t)-1);
	}

	err = _citrus_iconv_convert(
		(struct _citrus_iconv *)handle, in, szin, out, szout, 0, &ret);
	if (err) {
		errno = err;
		ret = (size_t)-1;
	}

	return (ret);
}

size_t
__iconv(iconv_t handle, const char **in, size_t *szin, char **out,
	size_t *szout, u_int32_t flags, size_t *invalids)
{
	int err;
	size_t ret;

	if (ISBADF(handle)) {
		errno = EBADF;
		return ((size_t)-1);
	}

	err = _citrus_iconv_convert(
		(struct _citrus_iconv *)handle, in, szin, out, szout,
		flags, &ret);
	if (invalids)
		*invalids = ret;
	if (err) {
		errno = err;
		ret = (size_t)-1;
	}

	return (ret);
}

int
__iconv_get_list(char ***rlist, size_t *rsz)
{
	int ret;

	ret = _citrus_esdb_get_list(rlist, rsz);
	if (ret) {
		errno = ret;
		return -1;
	}

	return 0;
}

void
__iconv_free_list(char **list, size_t sz)
{
	_citrus_esdb_free_list(list, sz);
}

#else
iconv_t
/*ARGSUSED*/
_iconv_open(const char *in, const char *out)
{
	errno = EINVAL;
	return ((iconv_t)-1);
}
int
/*ARGSUSED*/
_iconv_close(iconv_t handle)
{
	errno = EBADF;
	return (-1);
}
size_t
/*ARGSUSED*/
_iconv(iconv_t handle, const char **in, size_t *szin, char **out, size_t *szout)
{
	errno = EBADF;
	return ((size_t)-1);
}
int
/*ARGSUSED*/
__iconv_get_list(char ***rlist, size_t *rsz)
{
	errno = EINVAL;
	return -1;
}
void
/*ARGSUSED*/
__iconv_free_list(char **list, size_t sz)
{
}
#endif
