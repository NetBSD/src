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
#include <stdio.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <locale.h>
#include <wchar.h>
#include "unicode_ucd.h"
#include "unicode_dm_data.h"
/* #include "unicode_decomp_data.h" */
/* #include "unicode_collation_data.h" - DUCET */
#include "unicode_ccc_data.h"
#include "unicode_nfd_qc_data.h"
#include "unicode_reserved_cp_data.h"
#include "unicode_reserved_range_data.h"

#include "setlocale_local.h"
#include "collate_local.h"

static int ucd_get_collation_element(wchar_t *, const struct xlocale_collate *, struct collation_element *, int);

/* #define USE_NFD_QC_HASH_TABLE */
#define USE_CCC_HASH_TABLE
/* #define USE_COLL_HASH_TABLE */

#ifdef USE_NFD_QC_HASH_TABLE
#define NFD_QC_WIDTH 1024
#define NFD_QC_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_nfd_qc_head *nfd_qc_hash;
#endif

#define CCC_WIDTH 1024
#define CCC_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_ccc_head *ccc_hash;

#define RESV_CP_WIDTH 1024
#define RESV_CP_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_resv_cp_head *resv_cp_hash;

static struct ucd_resv_range_head resv_range_list;

uint8_t unicode_get_nfd_qc(wchar_t wc)
{
	int dfl = 1;
#ifdef USE_NFD_QC_HASH_TABLE
	struct ucd_nfd_qc *ccp;
	int i;

	if (nfd_qc_hash == NULL) {
		nfd_qc_hash = (struct ucd_nfd_qc_head *)calloc(NFD_QC_WIDTH, sizeof(*nfd_qc_hash));
		for (i = 0; i < NFD_QC_WIDTH; i++)
			SLIST_INIT(nfd_qc_hash + i);
		for (ccp = ucd_nfd_qc_data; ccp->cp < 0x0FFFFFFF; ++ccp) {
			SLIST_INSERT_HEAD(&nfd_qc_hash[NFD_QC_HASH(ccp->cp)], ccp, hash_next);
		}
	}

	SLIST_FOREACH(ccp, &nfd_qc_hash[NFD_QC_HASH(wc)], hash_next) {
		if (ccp->cp < wc) {
			return dfl;
		}
		if (ccp->cp == wc) {
			return ccp->nfd_qc;
		}
	}
#else
	int l, r, m;
	wchar_t u;

	/* Try single substitutions first */
	l = 0;
	r = UCD_NFD_QC_DATA_LENGTH - 1;
	while (l <= r) {
		m = (l + r) / 2;
		u = ucd_nfd_qc_data[m].cp;
		if (u == wc) {
			return ucd_nfd_qc_data[m].nfd_qc;
		}
		if (u < wc)
			l = m + 1;
		else
			r = m - 1;
	}
#endif /* USE_NFD_QC_HASH_TABLE */
	return dfl;
}

uint8_t unicode_get_ccc(wchar_t wc)
{
	struct ucd_ccc *ccp;
#ifdef USE_CCC_HASH_TABLE
	int i;

	if (ccc_hash == NULL) {
		ccc_hash = (struct ucd_ccc_head *)calloc(CCC_WIDTH, sizeof(*ccc_hash));
		for (i = 0; i < CCC_WIDTH; i++)
			SLIST_INIT(ccc_hash + i);
		for (ccp = ucd_ccc_data; ccp->cp < 0x0FFFFFFF; ++ccp) {
			SLIST_INSERT_HEAD(&ccc_hash[CCC_HASH(ccp->cp)], ccp, hash_next);
		}
	}

	SLIST_FOREACH(ccp, &ccc_hash[CCC_HASH(wc)], hash_next) {
#if 0
		if (ccp->cp < wc) {
			return 0;
		}
#endif
		if (ccp->cp == wc) {
			return ccp->ccc;
		}
	}
#else /* ! USE_CCC_HASH_TABLE */
	for (ccp = ucd_ccc_data; ccp->cp; ++ccp) {
		if (ccp->cp > wc) {
			return 0;
		}
		if (ccp->cp == wc) {
			return ccp->ccc;
		}
	}
#endif
	return 0;
}

/*
 * Return a canonical decomposition of wc,
 * or null if it decomposes to itself.
 */
wchar_t *unicode_decomposition(wchar_t wc, int *np)
{
	int l, r, m;
	wchar_t u;

	/* Try single substitutions first */
	l = 0;
	r = UCD_DECOMP_SINGLE_DATA_LENGTH - 1;
	while (l <= r) {
		m = (l + r) / 2;
		u = ucd_decomp_single_data[m].cp;
		if (u == wc) {
			*np = 1;
			return &ucd_decomp_single_data[m].dm;
		}
		if (u < wc)
			l = m + 1;
		else
			r = m - 1;
	}
	
	/* Then pairs */
	l = 0;
	r = UCD_DECOMP_PAIR_DATA_LENGTH - 1;
	while (l <= r) {
		m = (l + r) / 2;
		u = ucd_decomp_pair_data[m].cp;
		if (u == wc) {
			*np = 2;
			return ucd_decomp_pair_data[m].dm;
		}
		if (u < wc)
			l = m + 1;
		else
			r = m - 1;
	}

	/* Then the rest */
	l = 0;
	r = UCD_DECOMP_MISC_DATA_LENGTH - 1;
	while (l <= r) {
		m = (l + r) / 2;
		u = ucd_decomp_misc_data[m].cp;
		if (u == wc) {
			*np = ucd_decomp_misc_data[m].n;
			return ucd_decomp_misc_data[m].dm;
		}
		if (u < wc)
			l = m + 1;
		else
			r = m - 1;
		}
	return NULL;
	}

int
ucd_get_collation_element(wchar_t *wcp, const struct xlocale_collate *xc, struct collation_element *out, int do_ccc)
{
	struct collation_element sc;
	wchar_t *wcccp, tmp, u;
	size_t i, sclen, best = 0;
	uint8_t ccc, this_ccc;
	int m, l, r;

	sc.flags = 0x0;
	out->pri = NULL;
	out->len = 1;

	if (xc != NULL) {
		/* Find the index of the leading character, if it appears */
		l = m = 0;
		r = xc->info->chain_count - 1;
		while (l <= r) {
			m = (l + r) / 2;
			u = xc->chain_pri_table[m].str[0];
			if (u == wcp[0]) {
				break;
			}
			if (u < wcp[0])
				l = m + 1;
			else
				r = m - 1;
		}

		/* Find the first element that begins with wcp[0] */
		while (wcp[0] == xc->chain_pri_table[m].str[0])
			--m;
		++m;

		if (wcp[0] == xc->chain_pri_table[m].str[0]) {
			const wchar_t * pm = xc->chain_pri_table[m].str;
			/* There is at least one matching chain. */
			/* Walk through all the possibilities. */
			while (wcp[0] == pm[0]) {
				pm = xc->chain_pri_table[m].str;
				/* Check that this element matches */
				for (i = 0; pm[i]; i++) {
					if (wcp[i] != pm[i])
						break;
				}
				if (pm[i] == 0) {
					/* It matches */
					const wchar_t key = xc->chain_pri_table[m].key;
					if (key < 0x80) {
						out->pri = xc->char_pri_table[key].pri;
						best = 1;
					} else {
						/* Now look at large matches */
						l = 0;
						r = xc->info->large_count;
						while (l <= r) {
							m = (l + r) / 2;
							u = xc->large_pri_table[m].val;
							if (u == key) {
								out->pri = xc->large_pri_table[m].pri;
								best = 1;
								break;
							}
							if (u < wcp[0])
								l = m + 1;
							else
								r = m - 1;
						}
					}
					best = i;
				}
				++m;
			}
		}

		if (best == 0) {
			/* There is no matching chain. Look up individual weights. */
			if (wcp[0] < 0x80) {
				out->pri = xc->char_pri_table[wcp[0]].pri;
				best = 1;
			} else {
				/* Now look at large matches */
				l = 0;
				r = xc->info->large_count;
				while (l <= r) {
					m = (l + r) / 2;
					u = xc->large_pri_table[m].val;
					if (u == wcp[0]) {
						out->pri = xc->large_pri_table[m].pri;
						best = 1;
						break;
					}
					if (u < wcp[0])
						l = m + 1;
					else
						r = m - 1;
				}
			}
		}

		/* Check reverse chains. */
		if (best == 0) {
			l = 0;
			r = xc->info->rchain_count;
			while (l <= r) {
				m = (l + r) / 2;
				u = xc->rchain_pri_table[m].val;
				if (u == wcp[0]) {
					out->pri = (weight_t *)xc->rchain_pri_table[m].pri;
					for (i = 0; i < COLLATE_STR_LEN && xc->rchain_pri_table[m].pri[i][0] != (weight_t)0xFFFFFFFF; ++i)
						;
					out->len = i;
					best = 1;
					break;
				}
				if (u < wcp[0])
					l = m + 1;
				else
					r = m - 1;
			}
		}

		if (best > 0 && do_ccc) {
			/*
			 * We have found the longest string S that matches in
			 * the table.  Now let the set of unblocked
			 * non-starters immediately following S be called CC.
			 * For any C in CC, if S+C has a match in the table,
			 * replace S with S+C and remove C from CC.  Repeat as
			 * long as we are expanding S.  Then copy the
			 * collation weights.
			 */
			ccc = 0xff;
			for (wcccp = wcp + best; *wcccp; ++wcccp) {
				this_ccc = unicode_get_ccc(*wcccp);
				if (this_ccc == 0)
					break;
				if (this_ccc == ccc)
					continue;
				ccc = this_ccc;

				/* Swap this and wcp + best, and look for a match */
				tmp = *wcccp;
				*wcccp = *(wcp + best);
				*(wcp + best) = tmp;
				sclen = ucd_get_collation_element(wcp, xc, &sc, 0);
				if (sclen > best) {
					*out = sc; /* Structure copy */
					best = sclen;
					/* Back up and start over */
					wcccp = wcp + best;
				} else {
					/* swap back */
					tmp = *wcccp;
					*wcccp = *(wcp + best);
					*(wcp + best) = tmp;
				}
			}
		}
	}

	/* Fall back to DUCET if we don't find anything in the locale */
	if (best == 0 && xc != &_DefaultCollateLocale)
		return ucd_get_collation_element(wcp, &_DefaultCollateLocale, out, do_ccc);

	return best;
}

#define UNASSIGNED_CACHE_WIDTH 16
static int unassigned_cache_idx;
static wchar_t unassigned_cache[UNASSIGNED_CACHE_WIDTH];

int
ucd_check_unassigned(wchar_t wc)
{
	struct ucd_resv_cp *ccp;
	struct ucd_resv_range *rrp;
	int unassigned;
	int i;
	int count = 0;

	if (resv_cp_hash == NULL) {
		/* Initialize reserved code point hash and range list */
		resv_cp_hash = (struct ucd_resv_cp_head *)calloc(RESV_CP_WIDTH, sizeof(*resv_cp_hash));
		for (i = 0; i < RESV_CP_WIDTH; i++)
			SLIST_INIT(resv_cp_hash + i);
		for (ccp = ucd_reserved_cp_data; ccp->cp < 0x0FFFFFFF; ++ccp) {
			SLIST_INSERT_HEAD(&resv_cp_hash[RESV_CP_HASH(ccp->cp)], ccp, hash_next);
		}
		
		/* For range list, we store them as individual code points if the range is short. */
		/* This means we need to allocate storage, but should end up being faster. */
		/* XXX make this separation in the build process instead or at run time. */
		for (rrp = ucd_reserved_range_data; rrp->firstcp < 0x0FFFFFFF; ++rrp) {
			if (rrp->lastcp - rrp->firstcp < RESV_CP_WIDTH) {
				for (i = rrp->firstcp; i <= rrp->lastcp; ++i) {
					ccp = (struct ucd_resv_cp *)malloc(sizeof(*ccp));
					ccp->cp = i;
					SLIST_INSERT_HEAD(&resv_cp_hash[RESV_CP_HASH(i)], ccp, hash_next);
				}
			} else {
				++count;
				SLIST_INSERT_HEAD(&resv_range_list, rrp, hash_next);
			}
		}
	}
	
	unassigned = 0;
	
	/*
	 * We know these blocks are full; use those
	 * to avoid the search below for e.g. Han
	 * ideograms.
	 */
	switch (wc & ~0x0FFF) {
	case 0x3000: /* Other Han ideographs */
	case 0x4000: /* Han core ideographs */
	case 0x5000:
	case 0x6000:
	case 0x7000:
	case 0x8000:
	case 0x9000:
		if (wc > 0x31EF && wc != 0x321F && wc != 0x32FF && wc < 0x4DB6)
			unassigned = -1;
		else if (wc > 0x4DBF && wc < 0x9FEB)
			unassigned = -1;
		break;
	case 0xF000:
		if (wc < 0xFA6E)
			unassigned = -1;
		break;
	case 0x17000: /* Tangut block completely full */
	case 0x18000: /* Tangut_Components */
		if (wc < 0x187ED)
			unassigned = -1;
		else if (wc >= 0x18800 && wc < 0x18AF3)
			unassigned = -1;
		break;
	case 0x1B000: /* Nushu */
		if (wc < 0x1B11F)
			unassigned = -1;
		break;
	default:
		break;
	}

#if 1
	if (!unassigned) {
		/* Look in the cache */
		for (i = 0; i < UNASSIGNED_CACHE_WIDTH; i++) {
			if (unassigned_cache[i] == wc) {
				unassigned = 1;
				break;
			}
		}
	}
#endif
	/* Next look in ranges */
	if (!unassigned) {
		SLIST_FOREACH(rrp, &resv_range_list, hash_next) {
			if (rrp->firstcp <= wc && wc <= rrp->lastcp) {
				unassigned = 1;
				break;
			}
		}
	}
	
	/* Finally look at individual code points */
	if (!unassigned) {
		SLIST_FOREACH(ccp, &resv_cp_hash[RESV_CP_HASH(wc)], hash_next) {
			if (ccp->cp == wc) {
				unassigned = 1;
				break;
			}
		}
	}
	
	if (unassigned < 0)
		unassigned = 0;
	
	if (unassigned) {
		unassigned_cache[unassigned_cache_idx] = wc;
		unassigned_cache_idx = (unassigned_cache_idx + 1) & 0x0f;
	}
	
	return unassigned;
}

/*
 * Add the collation element sequence that corresponds with
 * the beginning of the given string to head.
 * Return the number of wchars consumed, 0 if no match was found.
 */
int unicode_get_collation_element(struct collation_set *head, wchar_t *wcp, locale_t loc)
{
	struct collation_element *cc;
	int n, unassigned, len;
	struct xlocale_collate *cloc;
	weight_t *wp;
	weight_t aaaa = 0x0, bbbb = 0x0;

	cc = (struct collation_element *)malloc(sizeof(*cc));
	cc->flags = 0x0;

	if (loc == NULL)
		cloc = NULL;
	else
		cloc = _COLLATE_LOCALE(loc);
	len = ucd_get_collation_element(wcp, cloc, cc, 1);

	if (len)
		n = len;
	else {
		unassigned = ucd_check_unassigned(*wcp);

		/* If we couldn't find it, supply implicit weights. */
		if (!unassigned) {
			if ((*wcp >= 0x17000 && *wcp <= 0x187FF) /* Tangut block */
			    || (*wcp >= 0x18800 && *wcp <= 0x18AFF)) /* Tangut_Components block */
			{
				aaaa = 0xFB00;
				bbbb = (*wcp - 0x17000) | 0x8000;
			} else if (*wcp >= 0x1B170 && *wcp <= 0x1B2FF) { /* Nushu block */
				aaaa = 0xFB01;
				bbbb = (*wcp - 0x1B170) | 0x8000;
			} else if ((*wcp >= 0x4E00 && *wcp <= 0x9FFF)  /* Han core ideographs - XXX should use unicode DB characteristics here */
				   || (*wcp >= 0xF900 && *wcp <= 0xFAFF)) {
				aaaa = 0xFB40 + (*wcp >> 15);
				bbbb = (*wcp & 0x7FFF) | 0x8000;
			} else if ((*wcp >=  0x2E80 && *wcp <=  0x2EFF) /* Other Han ideographs - XXX and here */
				   || (*wcp >=  0x2F00 && *wcp <=  0x2FDF)
				   || (*wcp >=  0x2FF0 && *wcp <=  0x2FFF)
				   || (*wcp >=  0x3000 && *wcp <=  0x303F)
				   || (*wcp >=  0x31C0 && *wcp <=  0x31EF)
				   || (*wcp >=  0x3200 && *wcp <=  0x32FF)
				   || (*wcp >=  0x3300 && *wcp <=  0x33FF)
				   || (*wcp >=  0x3400 && *wcp <=  0x4DBF)
				   || (*wcp >=  0xF900 && *wcp <=  0xFAFF)
				   || (*wcp >=  0xFE30 && *wcp <=  0xFE4F)
				   || (*wcp >= 0x1F200 && *wcp <= 0x1F2FF)
				   || (*wcp >= 0x20000 && *wcp <= 0x2A6DF)
				   || (*wcp >= 0x2A700 && *wcp <= 0x2B73F)
				   || (*wcp >= 0x2B740 && *wcp <= 0x2B81F)
				   || (*wcp >= 0x2B820 && *wcp <= 0x2CEAF)
				   || (*wcp >= 0x2CEB0 && *wcp <= 0x2EBEF)
				   || (*wcp >= 0x2F800 && *wcp <= 0x2FA1F))
			{
				aaaa = 0xFB80 + (*wcp >> 15);
				bbbb = (*wcp & 0x7FFF) | 0x8000;
			}
		}
		if (aaaa == 0x0) {
			/* Any other code point */
			aaaa = 0xFBC0 + (*wcp >> 15);
			bbbb = (*wcp & 0x7FFF) | 0x8000;
		}

		wp = (weight_t *)calloc(2 * COLL_WEIGHTS_MAX, sizeof(weight_t));
		wp[0] = aaaa;
		wp[1] = 0x0020;
		wp[2] = 0x0002;
		wp[1 * COLL_WEIGHTS_MAX + 0] = bbbb;
		cc->pri = (const weight_t *)wp;
		cc->len = 2;
		cc->flags |= CE_FLAGS_NEEDSFREE;
		n = 1;
	}

	TAILQ_INSERT_TAIL(head, cc, tailq);

	return n;
}
