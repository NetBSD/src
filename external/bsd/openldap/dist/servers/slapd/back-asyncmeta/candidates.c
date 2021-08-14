/*	$NetBSD: candidates.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* candidates.c - candidate targets selection and processing for
 * back-asyncmeta */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2021 The OpenLDAP Foundation.
 * Portions Copyright 2016 Symas Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* ACKNOWLEDGEMENTS:
+ * This work was developed by Symas Corporation
+ * based on back-meta module for inclusion in OpenLDAP Software.
+ * This work was sponsored by Ericsson. */

#include <sys/cdefs.h>
__RCSID("$NetBSD: candidates.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "ac/string.h"

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

/*
 * The meta-directory has one suffix, called <suffix>.
 * It handles a pool of target servers, each with a branch suffix
 * of the form <branch X>,<suffix>, where <branch X> may be empty.
 *
 * When the meta-directory receives a request with a request DN that belongs
 * to a branch, the corresponding target is invoked. When the request DN
 * does not belong to a specific branch, all the targets that
 * are compatible with the request DN are selected as candidates, and
 * the request is spawned to all the candidate targets
 *
 * A request is characterized by a request DN. The following cases are
 * handled:
 * 	- the request DN is the suffix: <dn> == <suffix>,
 * 		all the targets are candidates (search ...)
 * 	- the request DN is a branch suffix: <dn> == <branch X>,<suffix>, or
 * 	- the request DN is a subtree of a branch suffix:
 * 		<dn> == <rdn>,<branch X>,<suffix>,
 * 		the target is the only candidate.
 *
 * A possible extension will include the handling of multiple suffixes
 */

static a_metasubtree_t *
asyncmeta_subtree_match( a_metatarget_t *mt, struct berval *ndn, int scope )
{
	a_metasubtree_t *ms = mt->mt_subtree;

	for ( ms = mt->mt_subtree; ms; ms = ms->ms_next ) {
		switch ( ms->ms_type ) {
		case META_ST_SUBTREE:
			if ( dnIsSuffix( ndn, &ms->ms_dn ) ) {
				return ms;
			}
			break;

		case META_ST_SUBORDINATE:
			if ( dnIsSuffix( ndn, &ms->ms_dn ) &&
				( ndn->bv_len > ms->ms_dn.bv_len || scope != LDAP_SCOPE_BASE ) )
			{
				return ms;
			}
			break;

		case META_ST_REGEX:
			/* NOTE: cannot handle scope */
			if ( regexec( &ms->ms_regex, ndn->bv_val, 0, NULL, 0 ) == 0 ) {
				return ms;
			}
			break;
		}
	}

	return NULL;
}

/*
 * returns 1 if suffix is candidate for dn, otherwise 0
 *
 * Note: this function should never be called if dn is the <suffix>.
 */
int
asyncmeta_is_candidate(
	a_metatarget_t	*mt,
	struct berval	*ndn,
	int		scope )
{
	struct berval rdn;
	int d = ndn->bv_len - mt->mt_nsuffix.bv_len;

	if ( d >= 0 ) {
		if ( !dnIsSuffix( ndn, &mt->mt_nsuffix ) ) {
			return META_NOT_CANDIDATE;
		}

		/*
		 * |  match  | exclude |
		 * +---------+---------+-------------------+
		 * |    T    |    T    | not candidate     |
		 * |    F    |    T    | continue checking |
		 * +---------+---------+-------------------+
		 * |    T    |    F    | candidate         |
		 * |    F    |    F    | not candidate     |
		 * +---------+---------+-------------------+
		 */

		if ( mt->mt_subtree ) {
			int match = ( asyncmeta_subtree_match( mt, ndn, scope ) != NULL );

			if ( !mt->mt_subtree_exclude ) {
				return match ? META_CANDIDATE : META_NOT_CANDIDATE;
			}

			if ( match /* && mt->mt_subtree_exclude */ ) {
				return META_NOT_CANDIDATE;
			}
		}

		switch ( mt->mt_scope ) {
		case LDAP_SCOPE_SUBTREE:
		default:
			return META_CANDIDATE;

		case LDAP_SCOPE_SUBORDINATE:
			if ( d > 0 ) {
				return META_CANDIDATE;
			}
			break;

		/* nearly useless; not allowed by config */
		case LDAP_SCOPE_ONELEVEL:
			if ( d > 0 ) {
				rdn.bv_val = ndn->bv_val;
				rdn.bv_len = (ber_len_t)d - STRLENOF( "," );
				if ( dnIsOneLevelRDN( &rdn ) ) {
					return META_CANDIDATE;
				}
			}
			break;

		/* nearly useless; not allowed by config */
		case LDAP_SCOPE_BASE:
			if ( d == 0 ) {
				return META_CANDIDATE;
			}
			break;
		}

	} else /* if ( d < 0 ) */ {
		if ( !dnIsSuffix( &mt->mt_nsuffix, ndn ) ) {
			return META_NOT_CANDIDATE;
		}

		switch ( scope ) {
		case LDAP_SCOPE_SUBTREE:
		case LDAP_SCOPE_SUBORDINATE:
			/*
			 * suffix longer than dn, but common part matches
			 */
			return META_CANDIDATE;

		case LDAP_SCOPE_ONELEVEL:
			rdn.bv_val = mt->mt_nsuffix.bv_val;
			rdn.bv_len = (ber_len_t)(-d) - STRLENOF( "," );
			if ( dnIsOneLevelRDN( &rdn ) ) {
				return META_CANDIDATE;
			}
			break;
		}
	}

	return META_NOT_CANDIDATE;
}

/*
 * meta_back_select_unique_candidate
 *
 * returns the index of the candidate in case it is unique, otherwise
 * META_TARGET_NONE if none matches, or
 * META_TARGET_MULTIPLE if more than one matches
 * Note: ndn MUST be normalized.
 */
int
asyncmeta_select_unique_candidate(
	a_metainfo_t	*mi,
	struct berval	*ndn )
{
	int	i, candidate = META_TARGET_NONE;

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		a_metatarget_t	*mt = mi->mi_targets[ i ];

		if ( asyncmeta_is_candidate( mt, ndn, LDAP_SCOPE_BASE ) ) {
			if ( candidate == META_TARGET_NONE ) {
				candidate = i;

			}
		}
	}

	return candidate;
}

/*
 * asyncmeta_clear_unused_candidates
 *
 * clears all candidates except candidate
 */
int
asyncmeta_clear_unused_candidates(
	Operation	*op,
	int		candidate,
	a_metaconn_t	*mc,
	SlapReply	*candidates)
{
	a_metainfo_t	*mi = mc->mc_info;
	int		i;

	for ( i = 0; i < mi->mi_ntargets; ++i ) {
		if ( i == candidate ) {
			continue;
		}
		META_CANDIDATE_RESET( &candidates[ i ] );
	}

	return 0;
}
