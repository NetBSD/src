/*	$NetBSD: dncache.c,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* dncache.c - dn caching for back-asyncmeta */
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
 * This work was developed by Symas Corporation
 * based on back-meta module for inclusion in OpenLDAP Software.
 * This work was sponsored by Ericsson. */

#include <sys/cdefs.h>
__RCSID("$NetBSD: dncache.c,v 1.2 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-asyncmeta.h"

/*
 * The dncache, at present, maps an entry to the target that holds it.
 */

typedef struct metadncacheentry_t {
	struct berval	dn;
	int 		target;

	time_t 		lastupdated;
} metadncacheentry_t;

/*
 * asyncmeta_dncache_cmp
 *
 * compares two struct metadncacheentry; used by avl stuff
 * FIXME: modify avl stuff to delete an entry based on cmp
 * (e.g. when ttl expired?)
 */
int
asyncmeta_dncache_cmp(
	const void	*c1,
	const void	*c2 )
{
	metadncacheentry_t	*cc1 = ( metadncacheentry_t * )c1;
	metadncacheentry_t	*cc2 = ( metadncacheentry_t * )c2;

	/*
	 * case sensitive, because the dn MUST be normalized
	 */
	return ber_bvcmp( &cc1->dn, &cc2->dn);
}

/*
 * asyncmeta_dncache_dup
 *
 * returns -1 in case a duplicate struct metadncacheentry has been inserted;
 * used by avl stuff
 */
int
asyncmeta_dncache_dup(
	void		*c1,
	void		*c2 )
{
	metadncacheentry_t	*cc1 = ( metadncacheentry_t * )c1;
	metadncacheentry_t	*cc2 = ( metadncacheentry_t * )c2;

	/*
	 * case sensitive, because the dn MUST be normalized
	 */
	return ( ber_bvcmp( &cc1->dn, &cc2->dn ) == 0 ) ? -1 : 0;
}

/*
 * asyncmeta_dncache_get_target
 *
 * returns the target a dn belongs to, or -1 in case the dn is not
 * in the cache
 */
int
asyncmeta_dncache_get_target(
	a_metadncache_t	*cache,
	struct berval	*ndn )
{
	metadncacheentry_t	tmp_entry,
				*entry;
	int			target = META_TARGET_NONE;

	assert( cache != NULL );
	assert( ndn != NULL );

	tmp_entry.dn = *ndn;
	ldap_pvt_thread_mutex_lock( &cache->mutex );
	entry = ( metadncacheentry_t * )ldap_avl_find( cache->tree,
			( caddr_t )&tmp_entry, asyncmeta_dncache_cmp );

	if ( entry != NULL ) {

		/*
		 * if cache->ttl < 0, cache never expires;
		 * if cache->ttl = 0 no cache is used; shouldn't get here
		 * else, cache is used with ttl
		 */
		if ( cache->ttl < 0 ) {
			target = entry->target;

		} else {
			if ( entry->lastupdated+cache->ttl > slap_get_time() ) {
				target = entry->target;
			}
		}
	}
	ldap_pvt_thread_mutex_unlock( &cache->mutex );

	return target;
}

/*
 * asyncmeta_dncache_update_entry
 *
 * updates target and lastupdated of a struct metadncacheentry if exists,
 * otherwise it gets created; returns -1 in case of error
 */
int
asyncmeta_dncache_update_entry(
	a_metadncache_t	*cache,
	struct berval	*ndn,
	int 		target )
{
	metadncacheentry_t	*entry,
				tmp_entry;
	time_t			curr_time = 0L;
	int			err = 0;

	assert( cache != NULL );
	assert( ndn != NULL );

	/*
	 * if cache->ttl < 0, cache never expires;
	 * if cache->ttl = 0 no cache is used; shouldn't get here
	 * else, cache is used with ttl
	 */
	if ( cache->ttl > 0 ) {
		curr_time = slap_get_time();
	}

	tmp_entry.dn = *ndn;

	ldap_pvt_thread_mutex_lock( &cache->mutex );
	entry = ( metadncacheentry_t * )ldap_avl_find( cache->tree,
			( caddr_t )&tmp_entry, asyncmeta_dncache_cmp );

	if ( entry != NULL ) {
		entry->target = target;
		entry->lastupdated = curr_time;

	} else {
		entry = ch_malloc( sizeof( metadncacheentry_t ) + ndn->bv_len + 1 );
		if ( entry == NULL ) {
			err = -1;
			goto error_return;
		}

		entry->dn.bv_len = ndn->bv_len;
		entry->dn.bv_val = (char *)&entry[ 1 ];
		AC_MEMCPY( entry->dn.bv_val, ndn->bv_val, ndn->bv_len );
		entry->dn.bv_val[ ndn->bv_len ] = '\0';

		entry->target = target;
		entry->lastupdated = curr_time;

		err = ldap_avl_insert( &cache->tree, ( caddr_t )entry,
				asyncmeta_dncache_cmp, asyncmeta_dncache_dup );
	}

error_return:;
	ldap_pvt_thread_mutex_unlock( &cache->mutex );

	return err;
}

int
asyncmeta_dncache_delete_entry(
	a_metadncache_t	*cache,
	struct berval	*ndn )
{
	metadncacheentry_t	*entry,
				tmp_entry;

	assert( cache != NULL );
	assert( ndn != NULL );

	tmp_entry.dn = *ndn;

	ldap_pvt_thread_mutex_lock( &cache->mutex );
	entry = ldap_avl_delete( &cache->tree, ( caddr_t )&tmp_entry,
			asyncmeta_dncache_cmp );
	ldap_pvt_thread_mutex_unlock( &cache->mutex );

	if ( entry != NULL ) {
		asyncmeta_dncache_free( ( void * )entry );
	}

	return 0;
}

/*
 * meta_dncache_free
 *
 * frees an entry
 *
 */
void
asyncmeta_dncache_free(
	void		*e )
{
	free( e );
}
