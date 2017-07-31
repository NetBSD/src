/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder.
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

#ifdef COLLATION_TEST
#include "collation_test.h"
#endif /* COLLATION_TEST */

#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <wchar.h>

/* #include "ucd.h" */
#include "unicode_collate.h"
#include "setlocale_local.h"

/* These should be in system header files */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/*******************/

/*
 * Compare strings using collating information.
 */
int
wcscoll_l(const wchar_t *s1, const wchar_t *s2, locale_t loc)
{
	if (loc && loc->part_impl[LC_COLLATE] == _lc_C_locale.part_impl[LC_COLLATE])
		return wcscmp(s1, s2);
	return unicode_wcscoll_l(s1, s2, loc);
}

int
wcscoll(const wchar_t *s1, const wchar_t *s2)
{
	return wcscoll_l(s1, s2, _current_locale());
}

int
strcoll_l(const char *arg0, const char *arg1, locale_t loc)
{
	wchar_t *wcs0, *wcs1;
	int ret;
	size_t len0, len1;

	/* Short-circuit, if there is no valid collation */
	if (COLLATE_MAX_LEVEL(loc) <= 0)
		return strcmp(arg0, arg1);

	/*
	 * Convert to wchar.  We assume that no given
	 * source char converts to more than three
	 * wide chars.
	 */
	len0 = strlen(arg0) + 1;
	len1 = strlen(arg1) + 1;
	wcs0 = (wchar_t *)malloc(3 * len0 * sizeof(*wcs0));
	wcs1 = (wchar_t *)malloc(3 * len1 * sizeof(*wcs1));
	mbstowcs(wcs0, arg0, len0);
	mbstowcs(wcs1, arg1, len1);

	ret = unicode_wcscoll_l(wcs0, wcs1, loc);

	free(wcs0);
	free(wcs1);

	return ret;
}

static void
unicode_wcstokey(uint16_t **key, size_t *keylen, const wchar_t *arg0, wchar_t **m_arg0, locale_t loc)
{
	int clen0;
	struct collation_set c_arg0;
	struct collation_element *entry, *tentry;

	/*
	 * 1. Convert each string to Unicode NFD.
	 *    If we need to expand these strings, 
	 *    they will have to be freed later.
	 */
	arg0 = unicode_recursive_decompose(arg0, m_arg0);
	clen0 = wcslen(arg0);

	/*
	 * 2. Produce collation elements for each string.
	 */
	TAILQ_INIT(&c_arg0);
	unicode_wchar2coll(&c_arg0, arg0, loc);

	/*
	 * 3. Produce a sort key from the collation elements.
	 */
	*key = unicode_coll2numkey(&c_arg0, &clen0, loc);
	*keylen = clen0;

#ifdef DEBUG_FINAL_WEIGHTS
	printf(" final weights:");
	for (clen0 = 0; (size_t)clen0 < *keylen; ++clen0) {
		if ((*key)[clen0] == 0x0)
			printf(" |");
		else
			printf(" %4.4hx", (*key)[clen0]);
	}
	printf("\n");
#endif

	/* Free the tailq */
	TAILQ_FOREACH_SAFE(entry, &c_arg0, tailq, tentry) {
		TAILQ_REMOVE(&c_arg0, entry, tailq);
		if (entry->flags & CE_FLAGS_NEEDSFREE)
			free(__UNCONST(entry->pri));
		free(entry);
	}
	TAILQ_INIT(&c_arg0);
}

/*
 * Retrieve numeric keys, shift left and add one.
 * NOTE this assumes that the keys fit in 30 bits.
 * We're using 16 bit keys.
 */
size_t
unicode_wcsxfrm_l(wchar_t *dst, const wchar_t *src, size_t n, locale_t loc)
{
	uint16_t *key;
	size_t i, keylen;
	wchar_t *m_arg0 = NULL;

	unicode_wcstokey(&key, &keylen, src, &m_arg0, loc);
	if (keylen > n || dst == NULL)
		return keylen;
	for (i = 0; i < keylen; i++) {
		dst[i] = (key[i] << 1) | 0x1;
	}
	if (keylen < n)
		dst[keylen] = 0;
	if (m_arg0)
		free(m_arg0);
	free(key);
	return keylen;
}

/*
 * Convert a given string to collation key, then
 * convert that back into char *, with the
 * restrictions that (a) no zero values are
 * represented before the terminating NUL, and
 * (b) the high bit is not set.  The easy
 * way to do this is to use six bits per byte,
 * which means that, in effect, we are base64-encoding;
 * except that true base64 encoding doesn't sort right.
 */
size_t
strxfrm_l(char * __restrict dst, const char * __restrict src, size_t n, locale_t loc)
{
	uint16_t *key0;
	size_t keylen0;
	size_t i, r, srclen;
	unsigned char *k;
	wchar_t *wsrc, *m_arg0 = NULL;

	srclen = strlen(src) + 1;
	wsrc = (wchar_t *)malloc(srclen * sizeof(*wsrc));
	mbstowcs(wsrc, src, srclen);

	unicode_wcstokey(&key0, &keylen0, wsrc, &m_arg0, loc);
	r = (keylen0 * 4 + 2) / 3;
	if (r > n || dst == NULL) {
		if (m_arg0)
			free(m_arg0);
		return r;
	}

	k = (unsigned char *)key0;
	for (i = 0; i < n && i < keylen0; i ++) {
		switch (i & 0x03) {
			case 0:
				*dst = ((*k & 0xfc) >> 1) | 1;
				break;
			case 1:
				*dst = ((*k & 0x03) << 5) | ((k[1] & 0x80) >> 3) | 1;
				++key0;
				break;
			case 2:
				*dst = ((*k & 0x07) << 3) | ((k[1] & 0xc0) >> 5) | 1;
				++key0;
				break;
			case 3:
				*dst = ((*k & 0x3f) << 1) | 1;
				++key0;
				break;
		}
		++dst;
	}
	if (i < n)
		*dst = 0;

	if (m_arg0)
		free(m_arg0);
	free(key0);
	return i;
}

int 
unicode_wcscoll_l(const wchar_t *arg0, const wchar_t *arg1, locale_t loc)
{
#ifdef TIE_BREAKER
	wchar_t *oarg0 = arg0, *oarg1 = arg1;
#endif /* TIE_BREAKER */
	wchar_t *m_arg0 = NULL, *m_arg1 = NULL;
	uint16_t *key0, *key1;
	size_t keylen0, keylen1;
	size_t i;
	int ret = 0;

	/*
	 * 0. If locale doesn't define a valid
	 *    collation system, fall back on wcscmp.
	 */
	if (COLLATE_MAX_LEVEL(loc) <= 0) {
		return wcscmp(arg0, arg1);
	}

	unicode_wcstokey(&key0, &keylen0, arg0, &m_arg0, loc);
	unicode_wcstokey(&key1, &keylen1, arg1, &m_arg1, loc);
	
	/*
	 * 4. Compare sort keys.
	 */
	for (i = 0; i <= MIN(keylen0, keylen1); i++) {
		if (key0[i] != key1[i]) {
			ret = key0[i] - key1[i];
			break;
		}
	}

#ifdef TIE_BREAKER
	/*
	 * 5. (Optional.)  If the strings collate equal,
	 *    use binary comparison on the NFD representation
	 *    of the original strings.
	 */
	if (ret == 0) {
		fprint_unicode_string(stderr, (m_arg0 ? m_arg0 : arg0));
		fprintf(stderr, " vs ");
		fprint_unicode_string(stderr, (m_arg1 ? m_arg1 : arg1));
		ret = wcscmp((m_arg0 ? m_arg0 : arg0), (m_arg1 ? m_arg1 : arg1));
		fprintf(stderr, " returns %d\n", ret);
	}
#endif /* TIE_BREAKER */

	if (m_arg0)
		free(m_arg0);
	if (m_arg1)
		free(m_arg1);
	free(key0);
	free(key1);
	return ret;
}

static wchar_t *
unicode_recursive_decompose(const wchar_t *arg0, wchar_t **m_arg0)
{
	wchar_t *wcp, *dst, *decomposition, *targ0, *dend;
	int recurse = 0;
	int swap_again = 0;
	int max_factor = 1, n = 0;
	int alloc = 0, decompose = 0;
	wchar_t tmp;
	uint8_t ccc, occc = 0;

	/* Scan for NFD. If we are already in NFD     */
	/* and have no combining characters, we don't */
	/* need to allocate memory. */
	alloc = 0;
	for (wcp = (wchar_t *)__UNCONST(arg0); *wcp; ++wcp) {
		if (*wcp < 0x80) /* ASCII does not decopmose but is not in the decomp table either */
			continue;
		if (unicode_get_nfd_qc(*wcp) == 0) {
			decompose = 1;
			alloc = 1;
			if (unicode_decomposition(*wcp, &n)) {
				if (max_factor < n)
					max_factor = n;
			} else {
				fprintf(stderr, "WTF, char 0x%x is not NFD but has no decomposition?!\n", *wcp);
			}
		}
		ccc = unicode_get_ccc(*wcp);
		if (ccc > 0) {
			alloc = 1;
		}
	}
	if (alloc == 0)
		return __UNCONST(arg0);
	
	/*
	 * We need to make a copy, though we might not need to decompose.
	 */
	targ0 = wcsdup(arg0);
	*m_arg0 = (wchar_t *)realloc(*m_arg0, (max_factor * wcslen(arg0) + 1) * sizeof(*arg0));

	if (decompose == 0) {
		dst = *m_arg0;
		wcscpy(dst, targ0);
	} else {
		for (wcp = targ0, dst = *m_arg0; *wcp; ++wcp) {
			if (*wcp < 0x80)
				decomposition = NULL;
			if (unicode_get_nfd_qc(*wcp) > 0) {
				decomposition = NULL;
			} else
				decomposition = unicode_decomposition(*wcp, &n);
			if (decomposition == NULL) {
				*dst++ = *wcp;
			} else {
				dend = decomposition + n;
				while (decomposition < dend) {
					/* int n2 = 0; */
					if (*decomposition >= 0x80
					    && unicode_get_nfd_qc(*decomposition) == 0
					    /* && unicode_decomposition(*decomposition, &n2) != NULL */) {
						recurse = 1;
					}
					*dst++ = *decomposition++;
				}
			}
		}
		*dst = 0;
	}
	free(targ0);

	/*
	 * Scan through dst looking for out-of-order
	 * combining characters.  If we find any,
	 * swap them.  We may need to swap more.
	 */
	do {
		swap_again = 0;
		occc = 0;
		for (wcp = *m_arg0; *wcp; ++wcp) {
			ccc = unicode_get_ccc(*wcp);
			if (occc && ccc && occc > ccc) {
				tmp = *wcp;
				*wcp = *(wcp - 1);
				*(wcp - 1) = tmp;
				swap_again = 1;
			}
			occc = ccc;
		}
	} while (swap_again);

	if (!recurse)
		return *m_arg0;
	return unicode_recursive_decompose(*m_arg0, m_arg0);
}

/*
 * Add collation elements from this string to the given tailq.
 */
void
unicode_wchar2coll(struct collation_set *head, const wchar_t *wcs, locale_t loc)
{
	int nwc;
	wchar_t *copy, *wp;
	
	wp = copy = wcsdup(wcs);
	while (*wp) {
		nwc = unicode_get_collation_element(head, wp, loc);
		wp += (nwc > 1 ? nwc : 1);
	}
	free(copy);
}

static uint16_t *
unicode_coll2numkey(struct collation_set *cc, int *lenp, locale_t loc)
{
	struct collation_element *ccp;
	int val;
	uint16_t *ret, *rp;
	int max_level = COLLATE_MAX_LEVEL(loc) - 1; /* 2 or 3 */
	int i, j, count;

	/* Count the weights */
	count = 0;
	TAILQ_FOREACH(ccp, cc, tailq) {
		count += ccp->len;
	}

	rp = ret = (uint16_t *)calloc(2 * (count + 1) * (max_level + 1), sizeof(*rp));
	for (i = 0; i < COLL_WEIGHTS_MAX; i++) {
		if (loc && _COLLATE_LOCALE(loc)->info->directive[i] & COLL_BACKWARDS) {
			TAILQ_FOREACH_REVERSE(ccp, cc, collation_set, tailq) {
			  	for (j = ccp->len - 1; j >= 0; --j) {
					val = ccp->pri[j * COLL_WEIGHTS_MAX + i];
					if (val != 0)
					*rp++ = val;
				}
			}
		} else {
			TAILQ_FOREACH(ccp, cc, tailq) {
				if (ccp->pri == NULL)
					continue;
				for (j = 0; j < ccp->len; j++) {
					val = ccp->pri[j * COLL_WEIGHTS_MAX + i];
					if (val != 0)
					*rp++ = val;
				}
			}
		}
		/* Between each level we place a zero weight marker. */
		*rp++ = 0;
	}

	*lenp = rp - ret;
	return ret;
}

#if 0
/*
 * Convert numeric keys into a character array
 * in big-endian format.
 */
static unsigned char *
unicode_coll2key(struct collation_set *cc, int len, locale_t loc)
{
	uint16_t *ret = unicode_coll2numkey(cc, &len, loc);
	uint16_t *end = ret + COLLATE_MAX_LEVEL(loc) * len;
	uint16_t *rp;
	
	for (rp = ret; rp < end; ++rp) {
		if (*rp)
			*rp = htons(*rp);
	}
	return (unsigned char *)ret;
}
#endif

/*
 * Compare strings with using collating information.
 */
size_t
wcsxfrm_l(wchar_t *s1, const wchar_t *s2, size_t n, locale_t loc)
{
	return unicode_wcsxfrm_l(s1, s2, n, loc);
}

size_t
wcsxfrm(wchar_t *s1, const wchar_t *s2, size_t n)
{
	return wcsxfrm_l(s1, s2, n, _current_locale());
}
