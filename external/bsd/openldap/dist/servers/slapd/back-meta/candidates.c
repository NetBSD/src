/* $OpenLDAP: pkg/ldap/servers/slapd/back-meta/candidates.c,v 1.28.2.5 2008/02/11 23:26:46 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2008 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include "ac/string.h"

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

/*
 * The meta-directory has one suffix, called <suffix>.
 * It handles a pool of target servers, each with a branch suffix
 * of the form <branch X>,<suffix>
 *
 * When the meta-directory receives a request with a dn that belongs
 * to a branch, the corresponding target is invoked. When the dn
 * does not belong to a specific branch, all the targets that
 * are compatible with the dn are selected as candidates, and
 * the request is spawned to all the candidate targets
 *
 * A request is characterized by a dn. The following cases are handled:
 * 	- the dn is the suffix: <dn> == <suffix>,
 * 		all the targets are candidates (search ...)
 * 	- the dn is a branch suffix: <dn> == <branch X>,<suffix>, or
 * 	- the dn is a subtree of a branch suffix:
 * 		<dn> == <rdn>,<branch X>,<suffix>,
 * 		the target is the only candidate.
 *
 * A possible extension will include the handling of multiple suffixes
 */


/*
 * returns 1 if suffix is candidate for dn, otherwise 0
 *
 * Note: this function should never be called if dn is the <suffix>.
 */
int 
meta_back_is_candidate(
	metatarget_t	*mt,
	struct berval	*ndn,
	int		scope )
{
	if ( dnIsSuffix( ndn, &mt->mt_nsuffix ) ) {
		if ( mt->mt_subtree_exclude ) {
			int	i;

			for ( i = 0; !BER_BVISNULL( &mt->mt_subtree_exclude[ i ] ); i++ ) {
				if ( dnIsSuffix( ndn, &mt->mt_subtree_exclude[ i ] ) ) {
					return META_NOT_CANDIDATE;
				}
			}
		}

		switch ( mt->mt_scope ) {
		case LDAP_SCOPE_SUBTREE:
		default:
			return META_CANDIDATE;

		case LDAP_SCOPE_SUBORDINATE:
			if ( ndn->bv_len > mt->mt_nsuffix.bv_len ) {
				return META_CANDIDATE;
			}
			break;

		/* nearly useless; not allowed by config */
		case LDAP_SCOPE_ONELEVEL:
			if ( ndn->bv_len > mt->mt_nsuffix.bv_len ) {
				struct berval	rdn = *ndn;

				rdn.bv_len -= mt->mt_nsuffix.bv_len
					+ STRLENOF( "," );
				if ( dnIsOneLevelRDN( &rdn ) ) {
					return META_CANDIDATE;
				}
			}
			break;

		/* nearly useless; not allowed by config */
		case LDAP_SCOPE_BASE:
			if ( ndn->bv_len == mt->mt_nsuffix.bv_len ) {
				return META_CANDIDATE;
			}
			break;
		}

		return META_NOT_CANDIDATE;
	}

	if ( scope == LDAP_SCOPE_SUBTREE && dnIsSuffix( &mt->mt_nsuffix, ndn ) ) {
		/*
		 * suffix longer than dn, but common part matches
		 */
		return META_CANDIDATE;
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
meta_back_select_unique_candidate(
	metainfo_t	*mi,
	struct berval	*ndn )
{
	int	i, candidate = META_TARGET_NONE;

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		metatarget_t	*mt = mi->mi_targets[ i ];

		if ( meta_back_is_candidate( mt, ndn, LDAP_SCOPE_BASE ) ) {
			if ( candidate == META_TARGET_NONE ) {
				candidate = i;

			} else {
				return META_TARGET_MULTIPLE;
			}
		}
	}

	return candidate;
}

/*
 * meta_clear_unused_candidates
 *
 * clears all candidates except candidate
 */
int
meta_clear_unused_candidates(
	Operation	*op,
	int		candidate )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	int		i;
	SlapReply	*candidates = meta_back_candidates_get( op );
	
	for ( i = 0; i < mi->mi_ntargets; ++i ) {
		if ( i == candidate ) {
			continue;
		}
		META_CANDIDATE_RESET( &candidates[ i ] );
	}

	return 0;
}

/*
 * meta_clear_one_candidate
 *
 * clears the selected candidate
 */
int
meta_clear_one_candidate(
	Operation	*op,
	metaconn_t	*mc,
	int		candidate )
{
	metasingleconn_t	*msc = &mc->mc_conns[ candidate ];

	if ( msc->msc_ld != NULL ) {

#ifdef DEBUG_205
		char	buf[ BUFSIZ ];

		snprintf( buf, sizeof( buf ), "meta_clear_one_candidate ldap_unbind_ext[%d] mc=%p ld=%p",
			candidate, (void *)mc, (void *)msc->msc_ld );
		Debug( LDAP_DEBUG_ANY, "### %s %s\n",
			op ? op->o_log_prefix : "", buf, 0 );
#endif /* DEBUG_205 */

		ldap_unbind_ext( msc->msc_ld, NULL, NULL );
		msc->msc_ld = NULL;
	}

	if ( !BER_BVISNULL( &msc->msc_bound_ndn ) ) {
		ber_memfree_x( msc->msc_bound_ndn.bv_val, NULL );
		BER_BVZERO( &msc->msc_bound_ndn );
	}

	if ( !BER_BVISNULL( &msc->msc_cred ) ) {
		memset( msc->msc_cred.bv_val, 0, msc->msc_cred.bv_len );
		ber_memfree_x( msc->msc_cred.bv_val, NULL );
		BER_BVZERO( &msc->msc_cred );
	}

	msc->msc_mscflags = 0;

	return 0;
}

