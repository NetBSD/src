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

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <locale.h>
#include <wchar.h>
#include "unicode_ucd.h"
#include "unicode_decomp_data.h"
/* #include "unicode_collation_data.h" - DUCET */
#include "unicode_ccc_data.h"
#include "unicode_nfd_qc_data.h"
#include "unicode_reserved_cp_data.h"
#include "unicode_reserved_range_data.h"

#include "setlocale_local.h"
#include "collate_local.h"

static const struct ucd_coll * ucd_get_collation_element(wchar_t *, const _CollateLocale *, int *, int);

#define USE_NFD_QC_HASH_TABLE
#define USE_CCC_HASH_TABLE
#define USE_DECOMP_HASH_TABLE
/* #define USE_COLL_HASH_TABLE */

#define NFD_QC_WIDTH 1024
#define NFD_QC_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_nfd_qc_head *nfd_qc_hash;

#define CCC_WIDTH 1024
#define CCC_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_ccc_head *ccc_hash;

#define DECOMP_WIDTH 1024
#define DECOMP_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_decomp_head *decomp_hash;

#ifdef USE_COLL_HASH_TABLE
#define COLL_WIDTH 1024
#define COLL_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_coll_head *coll_hash;
#endif

#define RESV_CP_WIDTH 1024
#define RESV_CP_HASH(x) (((x) >> 2) & 0x3ff)
static struct ucd_resv_cp_head *resv_cp_hash;

static struct ucd_resv_range_head resv_range_list;

uint8_t unicode_get_nfd_qc(wchar_t wc)
{
	int dfl = 1;
	struct ucd_nfd_qc *ccp;
#ifdef USE_NFD_QC_HASH_TABLE
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
	for (ccp = ucd_nfd_qc_data; ccp->cp; ++ccp) {
		if (ccp->cp > wc) {
			return dfl;
		}
		if (ccp->cp == wc) {
			return ccp->nfd_qc;
		}
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
		if (ccp->cp < wc) {
			return 0;
		}
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
	struct ucd_decomp *ccp;
#ifdef USE_DECOMP_HASH_TABLE
	int i;

	if (decomp_hash == NULL) {
		decomp_hash = (struct ucd_decomp_head *)calloc(DECOMP_WIDTH, sizeof(*decomp_hash));
		for (i = 0; i < DECOMP_WIDTH; i++)
			SLIST_INIT(decomp_hash + i);
		for (ccp = ucd_decomp_data; ccp->cp; ++ccp) {
			SLIST_INSERT_HEAD(&decomp_hash[DECOMP_HASH(ccp->cp)], ccp, hash_next);
		}
	}

	SLIST_FOREACH(ccp, &decomp_hash[DECOMP_HASH(wc)], hash_next) {
		if (ccp->cp < wc) {
			return NULL;
		}
		if (ccp->cp == wc) {
			*np = ccp->n;
			return (wchar_t *)ccp->dm;
		}
	}
#else /* ! USE_DECOMP_HASH_TABLE */
	for (ccp = ucd_decomp_data; ccp->cp; ++ccp) {
		if (ccp->cp > wc)
			return NULL;
		if (ccp->cp == wc) {
			*np = ccp->n;
			return (wchar_t *)ccp->dm;
		}
	}
#endif /* ! USE_DECOMP_HASH_TABLE */
	return NULL;
}

static const struct ucd_coll *
ucd_get_collation_element(wchar_t *wcp, const _CollateLocale *cloc, int *np, int do_ccc)
{
	const struct ucd_coll *cc, *best_cc = NULL, *sc;
#ifdef USE_COLL_HASH_TABLE
	struct ucd_coll *ccp;
#endif
	wchar_t *wcccp, tmp;
	int best_len = 0;
	int i, match;
	uint8_t ccc, this_ccc;

#ifdef USE_COLL_HASH_TABLE
	if (coll_hash == NULL) {
		coll_hash = (struct ucd_coll_head *)calloc(COLL_WIDTH, sizeof(*coll_hash));
		for (i = 0; i < COLL_WIDTH; i++)
			SLIST_INIT(coll_hash + i);
		for (ccp = ucd_collate_data; ccp->cp < 0x0FFFFFFF; ++ccp) {
			wchar_t wc = ccp->cp;
			if (wc == 0x0)
				wc = ccp->cpp[0];
			SLIST_INSERT_HEAD(&coll_hash[COLL_HASH(wc)], ccp, hash_next);
		}
	}

	SLIST_FOREACH(cc, &coll_hash[COLL_HASH(*wcp)], hash_next) {
#else
	for (cc = cloc->coll_data; cc->cp < 0x0FFFFFFF; ++cc) {
		if (cc->cp > wcp[0]) {
			break;
		}
#endif

		/*
		 * Multi-character matches
		 */
		if (cc->cp == 0x0) {
			match = 1;
			for (i = 0; cc->cpp[i] != 0x0; ++i) {
				if (cc->cpp[i] != wcp[i]) {
					match = 0;
					break;
				}
			}
			if (match && i > best_len) {
				best_cc = cc;
				best_len = i;
			}
		} else if (cc->cp == wcp[0] && best_len == 0) {
			/* Single-character match */
			best_cc = cc;
			best_len = 1;
		}
	}

	if (best_cc != NULL && do_ccc) {
		/*
		 * We have found the longest string S that matches in the table.
		 * Now let the set of unblocked non-starters immediately following S
		 * be called CC.  For any C in CC, if S+C has a match in the table,
		 * replace S with S+C and remove C from CC.  Repeat as long as we
		 * are expanding S.  Then copy the collation weights.
		 */
		ccc = 0xff;
		for (wcccp = wcp + best_len; *wcccp; ++wcccp) {
			this_ccc = unicode_get_ccc(*wcccp);
			if (this_ccc == 0 || this_ccc > ccc)
				break;
			/* Swap this and wcp + best_len, and look for a match */
			tmp = *wcccp;
			*wcccp = *(wcp + best_len);
			*(wcp + best_len) = tmp;
			sc = ucd_get_collation_element(wcp, cloc, &best_len, 0);
			if (sc != best_cc) {
				best_cc = sc;
				/* Back up and start over */
				wcccp = wcp + best_len;
			} else {
				/* swap back - best_len should be unchanged */
				tmp = *wcccp;
				*wcccp = *(wcp + best_len);
				*(wcp + best_len) = tmp;
			}
		}
	}

	/* Fall back to DUCET if we don't find anything in the locale */
	if (best_len == 0 && cloc != &_DefaultCollateLocale)
		return ucd_get_collation_element(wcp, &_DefaultCollateLocale, np, do_ccc);

	*np = best_len;
	return best_cc;
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

#define _COLLATE_LOCALE(loc) \
  ((_CollateLocale *)((loc)->part_impl[(size_t)LC_COLLATE]))

/*
 * Add the collation element sequence that corresponds with
 * the beginning of the given string to head.
 * Return the number of wchars consumed, 0 if no match was found.
 */
int unicode_get_collation_element(struct collation_set *head, wchar_t *wcp, locale_t loc)
{
	const struct ucd_coll *cc;
	const weight_t *ce;
	struct collation_element *new_element;
	struct ucd_coll lcc;
	int n, unassigned;
	_CollateLocale *cloc = _COLLATE_LOCALE(loc);

	cc = ucd_get_collation_element(wcp, cloc, &n, 1);

	if (cc == NULL) {
		unassigned = ucd_check_unassigned(*wcp);

		/* If we couldn't find it, supply implicit weights. */
		weight_t aaaa = 0x0, bbbb = 0x0;
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

		lcc.ce[0] = (aaaa << 32) | 0x00200002;
		lcc.ce[1] = (bbbb << 32) | 0x0;
		lcc.ce[2] = 0x0;
		cc = &lcc;
		n = 1;
	}

	for (ce = cc->ce; *ce; ++ce) {
		new_element = (struct collation_element *)malloc(sizeof(*new_element));
		new_element->weight = *ce;
		TAILQ_INSERT_TAIL(head, new_element, tailq);
	}

	return n;
}
