/* $NetBSD: bsdctype.c,v 1.6 2010/06/02 16:04:52 tnozaki Exp $ */

/*-
 * Copyright (c)2008 Citrus Project,
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
__RCSID("$NetBSD: bsdctype.c,v 1.6 2010/06/02 16:04:52 tnozaki Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bsdctype.h"

const _BSDCTypeLocale _DefaultBSDCTypeLocale = {
    _C_ctype_,
    _C_tolower_,
    _C_toupper_
};

const _BSDCTypeLocale *_CurrentBSDCTypeLocale = &_DefaultBSDCTypeLocale;

typedef struct {
	_BSDCTypeLocale	bl;
	unsigned char	blp_ctype_tab  [_CTYPE_NUM_CHARS + 1];
	short		blp_tolower_tab[_CTYPE_NUM_CHARS + 1];
	short		blp_toupper_tab[_CTYPE_NUM_CHARS + 1];
} _BSDCTypeLocalePriv;

static __inline void
_bsdctype_init_priv(_BSDCTypeLocalePriv *blp)
{
	blp->blp_ctype_tab  [0] = 0;
	blp->blp_tolower_tab[0] = EOF;
	blp->blp_toupper_tab[0] = EOF;
	blp->bl.bl_ctype_tab   = &blp->blp_ctype_tab  [0];
	blp->bl.bl_tolower_tab = &blp->blp_tolower_tab[0];
	blp->bl.bl_toupper_tab = &blp->blp_toupper_tab[0];
}

static __inline int
_bsdctype_read_file(const char * __restrict var, size_t lenvar,
    _BSDCTypeLocalePriv * __restrict blp)
{
	const _FileBSDCTypeLocale *fbl;
	uint32_t value;
	int i;

	if (lenvar < sizeof(*fbl))
		return EFTYPE;
	fbl = (const _FileBSDCTypeLocale *)(const void *)var;
	if (memcmp(&fbl->fbl_id[0], _CTYPE_ID, sizeof(fbl->fbl_id)))
		return EFTYPE;
	value = ntohl(fbl->fbl_rev);
	if (value != _CTYPE_REV)
		return EFTYPE;
	value = ntohl(fbl->fbl_num_chars);
	if (value != _CTYPE_CACHE_SIZE)
		return EFTYPE;
	for (i = 0; i < _CTYPE_CACHE_SIZE; ++i) {
		blp->blp_ctype_tab  [i + 1] = fbl->fbl_ctype_tab[i];
		blp->blp_tolower_tab[i + 1] = ntohs(fbl->fbl_tolower_tab[i]);
		blp->blp_toupper_tab[i + 1] = ntohs(fbl->fbl_toupper_tab[i]);
	}
#if _CTYPE_CACHE_SIZE != _CTYPE_NUM_CHARS
	for (i = _CTYPE_CACHE_SIZE; i < _CTYPE_NUM_CHARS; ++i) {
		blp->blp_ctype_tab  [i + 1] = 0;
		blp->blp_tolower_tab[i + 1] = i;
		blp->blp_toupper_tab[i + 1] = i;
	}
#endif
	return 0;
}

static __inline int
_bsdctype_load_priv(const char * __restrict path,
    _BSDCTypeLocalePriv * __restrict blp)
{
	int fd, ret;
	struct stat st;
	size_t lenvar;
	char *var;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		goto err;
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
		goto err;
	if (fstat(fd, &st) == -1)
		goto err;
	if (!S_ISREG(st.st_mode)) {
		close(fd);
		return EBADF;
	}
	lenvar = (size_t)st.st_size;
	if (lenvar < 1) {
		close(fd);
		return EFTYPE;
	}
	var = mmap(NULL, lenvar, PROT_READ,
	    MAP_FILE|MAP_PRIVATE, fd, (off_t)0);
	if (var == MAP_FAILED)
		goto err;
	if (close(fd) == -1) {
		ret = errno;
		munmap(var, lenvar);
		return ret;
	}
	switch (*var) {
	case 'B':
		ret = _bsdctype_read_file(var, lenvar, blp);
		break;
	default:
		ret = EFTYPE;
	}
	munmap(var, lenvar);
	return ret;
err:
	ret = errno;
	close(fd);
	return ret;
}

int
_bsdctype_load(const char * __restrict path,
    _BSDCTypeLocale ** __restrict pbl)
{
	int sverr, ret;
	_BSDCTypeLocalePriv *blp;

	sverr = errno;
	errno = 0;
	blp = malloc(sizeof(*blp));
	if (blp == NULL) {
		ret = errno;
		errno = sverr;
		return ret;
	}
	_bsdctype_init_priv(blp);
	ret = _bsdctype_load_priv(path, blp);
	if (ret) {
		free(blp);
		errno = sverr;
		return ret;
	}
	*pbl = &blp->bl;
	return 0;
}
