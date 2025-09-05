/*	$NetBSD: cache.c,v 1.4 2025/09/05 21:16:28 christos Exp $	*/

/* cache.c - routines to maintain an in-core cache of entries */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2024 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: cache.c,v 1.4 2025/09/05 21:16:28 christos Exp $");

#include "portable.h"

#include <stdio.h>
#include "ac/string.h"

#include "slap.h"

#include "back-monitor.h"

/*
 * The cache maps DNs to Entries.
 * Each entry, on turn, holds the list of its children in the e_private field.
 * This is used by search operation to perform onelevel and subtree candidate
 * selection.
 */
typedef struct monitor_cache_t {
	struct berval		mc_ndn;
	Entry   		*mc_e;
} monitor_cache_t;

/*
 * compares entries based on the dn
 */
int
monitor_cache_cmp(
	const void	*c1,
	const void	*c2 )
{
	monitor_cache_t 	*cc1 = ( monitor_cache_t * )c1;
	monitor_cache_t 	*cc2 = ( monitor_cache_t * )c2;

	/*
	 * case sensitive, because the dn MUST be normalized
	 */
	return ber_bvcmp( &cc1->mc_ndn, &cc2->mc_ndn );
}

/*
 * checks for duplicate entries
 */
int
monitor_cache_dup(
	void		*c1,
	void		*c2 )
{
	monitor_cache_t *cc1 = ( monitor_cache_t * )c1;
	monitor_cache_t *cc2 = ( monitor_cache_t * )c2;

	/*
	 * case sensitive, because the dn MUST be normalized
	 */
	return ber_bvcmp( &cc1->mc_ndn, &cc2->mc_ndn ) == 0 ? -1 : 0;
}

/*
 * adds an entry to the cache and inits the mutex
 */
int
monitor_cache_add(
	monitor_info_t	*mi,
	Entry		*e,
	Entry		*parent )
{
	monitor_cache_t tmp_mc, *mc, *pmc = NULL;
	Entry **ep = NULL, *prev = NULL;
	int		rc = -1;

	assert( mi != NULL );
	assert( e != NULL );

	dnParent( &e->e_nname, &tmp_mc.mc_ndn );

	mc = ( monitor_cache_t * )ch_malloc( sizeof( monitor_cache_t ) );
	mc->mc_ndn = e->e_nname;
	mc->mc_e = e;

	if ( parent ) {
		/* Shortcut, but follow lock order as a fallback */
		if ( ldap_pvt_thread_mutex_trylock( &mi->mi_cache_lock ) ) {
			monitor_cache_release( mi, parent );
			ldap_pvt_thread_mutex_lock( &mi->mi_cache_lock );
			monitor_cache_lock( parent );
		}
	} else {
		ldap_pvt_thread_mutex_lock( &mi->mi_cache_lock );
	}

	/* Allow database root be added */
	if ( parent == NULL && mi->mi_cache != NULL ) {
		pmc = ldap_avl_find( mi->mi_cache, &tmp_mc, monitor_cache_cmp );
		if ( pmc == NULL ) {
			goto done;
		}
		parent = pmc->mc_e;
		monitor_cache_lock( parent );
	}

	rc = ldap_avl_insert( &mi->mi_cache, mc,
			monitor_cache_cmp, monitor_cache_dup );
	if ( rc != LDAP_SUCCESS ) {
		goto done;
	}

	if ( parent != NULL ) {
		monitor_entry_t *mp = parent->e_private;

		if ( mp->mp_children ) {
			monitor_entry_t *tail;

			monitor_cache_lock( mp->mp_last );
			tail = mp->mp_last->e_private;
			tail->mp_next = e;
			monitor_cache_release( mi, mp->mp_last );
			mp->mp_last = e;
		} else {
			mp->mp_children = mp->mp_last = e;
		}
	}

done:
	if ( pmc != NULL ) {
		monitor_cache_release( mi, parent );
	}
	ldap_pvt_thread_mutex_unlock( &mi->mi_cache_lock );

	if ( rc != LDAP_SUCCESS ) {
		ch_free( mc );
	}
	return rc;
}

/*
 * locks the entry (no r/w)
 */
int
monitor_cache_lock(
	Entry		*e )
{
	monitor_entry_t *mp;

	assert( e != NULL );
	assert( e->e_private != NULL );

	mp = ( monitor_entry_t * )e->e_private;
	ldap_pvt_thread_mutex_lock( &mp->mp_mutex );

	return( 0 );
}

/*
 * tries to lock the entry (no r/w)
 */
int
monitor_cache_trylock(
	Entry		*e )
{
	monitor_entry_t *mp;

	assert( e != NULL );
	assert( e->e_private != NULL );

	mp = ( monitor_entry_t * )e->e_private;
	return ldap_pvt_thread_mutex_trylock( &mp->mp_mutex );
}

/*
 * gets an entry from the cache based on the normalized dn 
 * with mutex locked
 */
int
monitor_cache_get(
	monitor_info_t	*mi,
	struct berval	*ndn,
	Entry		**ep )
{
	monitor_cache_t tmp_mc, *mc;

	assert( mi != NULL );
	assert( ndn != NULL );
	assert( ep != NULL );

	*ep = NULL;

	tmp_mc.mc_ndn = *ndn;

	ldap_pvt_thread_mutex_lock( &mi->mi_cache_lock );
	mc = ( monitor_cache_t * )ldap_avl_find( mi->mi_cache,
			( caddr_t )&tmp_mc, monitor_cache_cmp );

	if ( mc != NULL ) {
		/* entry is returned with mutex locked */
		monitor_cache_lock( mc->mc_e );
		*ep = mc->mc_e;
	}

	ldap_pvt_thread_mutex_unlock( &mi->mi_cache_lock );

	return ( *ep == NULL ? -1 : 0 );
}

/*
 * gets an entry from the cache based on the normalized dn 
 * with mutex locked
 */
int
monitor_cache_remove(
	monitor_info_t	*mi,
	struct berval	*ndn,
	Entry		**ep )
{
	monitor_cache_t tmp_mc, *mc;
	struct berval	pndn;

	assert( mi != NULL );
	assert( ndn != NULL );
	assert( ep != NULL );

	*ep = NULL;

	dnParent( ndn, &pndn );

retry:;
	ldap_pvt_thread_mutex_lock( &mi->mi_cache_lock );

	tmp_mc.mc_ndn = *ndn;
	mc = ( monitor_cache_t * )ldap_avl_find( mi->mi_cache,
			( caddr_t )&tmp_mc, monitor_cache_cmp );

	if ( mc != NULL ) {
		monitor_cache_t *pmc;

		tmp_mc.mc_ndn = pndn;
		pmc = ( monitor_cache_t * )ldap_avl_find( mi->mi_cache,
			( caddr_t )&tmp_mc, monitor_cache_cmp );
		if ( pmc != NULL ) {
			monitor_entry_t	*mp = (monitor_entry_t *)mc->mc_e->e_private,
					*pmp = (monitor_entry_t *)pmc->mc_e->e_private;
			Entry		**entryp, *prev = NULL;

			monitor_cache_lock( pmc->mc_e );

			for ( entryp = &pmp->mp_children; *entryp != NULL;  ) {
				monitor_entry_t	*next = (monitor_entry_t *)(*entryp)->e_private;

				monitor_cache_lock( *entryp );
				if ( next == mp ) {
					if ( mc->mc_e == pmp->mp_last ) {
						pmp->mp_last = prev;
					}
					*entryp = next->mp_next;
					entryp = NULL;
					break;
				}

				if ( prev != NULL ) {
					monitor_cache_release( mi, prev );
				}
				prev = *entryp;
				entryp = &next->mp_next;
			}
			if ( prev ) {
				monitor_cache_release( mi, prev );
			}

			if ( entryp != NULL ) {
				Debug( LDAP_DEBUG_ANY,
					"monitor_cache_remove(\"%s\"): "
					"not in parent's list\n",
					ndn->bv_val );
			}

			/* either succeeded, and the entry is no longer
			 * in its parent's list, or failed, and the
			 * entry is neither mucked with nor returned */
			monitor_cache_release( mi, pmc->mc_e );

			if ( entryp == NULL ) {
				monitor_cache_t *tmpmc;

				tmp_mc.mc_ndn = *ndn;
				tmpmc = ldap_avl_delete( &mi->mi_cache,
					( caddr_t )&tmp_mc, monitor_cache_cmp );
				assert( tmpmc == mc );

				*ep = mc->mc_e;
				ch_free( mc );
				mc = NULL;

				/* NOTE: we destroy the mutex, but otherwise
				 * leave the private data around; specifically,
				 * callbacks need be freed by someone else */

				ldap_pvt_thread_mutex_destroy( &mp->mp_mutex );
				mp->mp_next = NULL;
				mp->mp_children = NULL;
				mp->mp_last = NULL;
			}

		}

		if ( mc ) {
			monitor_cache_release( mi, mc->mc_e );
		}
	}

	ldap_pvt_thread_mutex_unlock( &mi->mi_cache_lock );

	return ( *ep == NULL ? -1 : 0 );
}

/*
 * If the entry exists in cache, it is returned in locked status;
 * otherwise, if the parent exists, if it may generate volatile 
 * descendants an attempt to generate the required entry is
 * performed and, if successful, the entry is returned
 */
int
monitor_cache_dn2entry(
	Operation		*op,
	SlapReply		*rs,
	struct berval		*ndn,
	Entry			**ep,
	Entry			**matched )
{
	monitor_info_t *mi = (monitor_info_t *)op->o_bd->be_private;
	int 			rc;
	struct berval		p_ndn = BER_BVNULL;
	Entry 			*e_parent;
	monitor_entry_t 	*mp;
		
	assert( mi != NULL );
	assert( ndn != NULL );
	assert( ep != NULL );
	assert( matched != NULL );

	*matched = NULL;

	if ( !dnIsSuffix( ndn, &op->o_bd->be_nsuffix[ 0 ] ) ) {
		return( -1 );
	}

	rc = monitor_cache_get( mi, ndn, ep );
       	if ( !rc && *ep != NULL ) {
		return( 0 );
	}

	/* try with parent/ancestors */
	if ( BER_BVISNULL( ndn ) ) {
		BER_BVSTR( &p_ndn, "" );

	} else {
		dnParent( ndn, &p_ndn );
	}

	rc = monitor_cache_dn2entry( op, rs, &p_ndn, &e_parent, matched );
	if ( rc || e_parent == NULL ) {
		return( -1 );
	}

	mp = ( monitor_entry_t * )e_parent->e_private;
	rc = -1;
	if ( mp->mp_flags & MONITOR_F_VOLATILE_CH ) {
		/* parent entry generates volatile children */
		rc = monitor_entry_create( op, rs, ndn, e_parent, ep );
	}

	if ( !rc ) {
		monitor_cache_lock( *ep );
		monitor_cache_release( mi, e_parent );

	} else {
		*matched = e_parent;
	}
	
	return( rc );
}

/*
 * releases the lock of the entry; if it is marked as volatile, it is
 * destroyed.
 */
int
monitor_cache_release(
	monitor_info_t	*mi,
	Entry		*e )
{
	monitor_entry_t *mp;

	assert( mi != NULL );
	assert( e != NULL );
	assert( e->e_private != NULL );
	
	mp = ( monitor_entry_t * )e->e_private;

	if ( mp->mp_flags & MONITOR_F_VOLATILE ) {
		ldap_pvt_thread_mutex_unlock( &mp->mp_mutex );
		ldap_pvt_thread_mutex_destroy( &mp->mp_mutex );
		ch_free( mp );
		e->e_private = NULL;
		entry_free( e );

		return( 0 );
	}
	
	ldap_pvt_thread_mutex_unlock( &mp->mp_mutex );

	return( 0 );
}

static void
monitor_entry_destroy( void *v_mc )
{
	monitor_cache_t		*mc = (monitor_cache_t *)v_mc;

	if ( mc->mc_e != NULL ) {
		monitor_entry_t *mp;

		assert( mc->mc_e->e_private != NULL );
	
		mp = ( monitor_entry_t * )mc->mc_e->e_private;

		if ( mp->mp_cb ) {
			monitor_callback_t	*cb;

			for ( cb = mp->mp_cb; cb != NULL; ) {
				monitor_callback_t	*next = cb->mc_next;

				if ( cb->mc_free ) {
					(void)cb->mc_free( mc->mc_e, &cb->mc_private );
				}
				ch_free( mp->mp_cb );

				cb = next;
			}
		}

		ldap_pvt_thread_mutex_destroy( &mp->mp_mutex );

		ch_free( mp );
		mc->mc_e->e_private = NULL;
		entry_free( mc->mc_e );
	}

	ch_free( mc );
}

int
monitor_cache_destroy(
	monitor_info_t	*mi )
{
	if ( mi->mi_cache ) {
		ldap_avl_free( mi->mi_cache, monitor_entry_destroy );
	}

	return 0;
}

int monitor_back_release(
	Operation *op,
	Entry *e,
	int rw )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	return monitor_cache_release( mi, e );
}
