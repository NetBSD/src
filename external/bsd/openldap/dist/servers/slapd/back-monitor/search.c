/* search.c - monitor backend search function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-monitor/search.c,v 1.39.2.5 2008/02/11 23:26:47 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2008 The OpenLDAP Foundation.
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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-monitor.h"
#include "proto-back-monitor.h"

static int
monitor_send_children(
	Operation	*op,
	SlapReply	*rs,
	Entry		*e_parent,
	int		sub )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	Entry 			*e,
				*e_tmp,
				*e_ch = NULL,
				*e_nonvolatile = NULL;
	monitor_entry_t *mp;
	int			rc,
				nonvolatile = 0;

	mp = ( monitor_entry_t * )e_parent->e_private;
	e_nonvolatile = e = mp->mp_children;

	if ( MONITOR_HAS_VOLATILE_CH( mp ) ) {
		monitor_entry_create( op, rs, NULL, e_parent, &e_ch );
	}
	monitor_cache_release( mi, e_parent );

	/* no volatile entries? */
	if ( e_ch == NULL ) {
		/* no persistent entries? return */
		if ( e == NULL ) {
			return LDAP_SUCCESS;
		}
	
	/* volatile entries */
	} else {
		/* if no persistent, return only volatile */
		if ( e == NULL ) {
			e = e_ch;

		/* else append persistent to volatile */
		} else {
			e_tmp = e_ch;
			do {
				mp = ( monitor_entry_t * )e_tmp->e_private;
				e_tmp = mp->mp_next;
	
				if ( e_tmp == NULL ) {
					mp->mp_next = e;
					break;
				}
			} while ( e_tmp );
			e = e_ch;
		}
	}

	/* return entries */
	for ( monitor_cache_lock( e ); e != NULL; ) {
		monitor_entry_update( op, rs, e );

		if ( op->o_abandon ) {
			/* FIXME: may leak generated children */
			if ( nonvolatile == 0 ) {
				for ( e_tmp = e; e_tmp != NULL; ) {
					mp = ( monitor_entry_t * )e_tmp->e_private;
					e = e_tmp;
					e_tmp = mp->mp_next;
					monitor_cache_release( mi, e );

					if ( e_tmp == e_nonvolatile ) {
						break;
					}
				}

			} else {
				monitor_cache_release( mi, e );
			}

			return SLAPD_ABANDON;
		}
		
		rc = test_filter( op, e, op->oq_search.rs_filter );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_flags = 0;
			rc = send_search_entry( op, rs );
			rs->sr_entry = NULL;
		}

		mp = ( monitor_entry_t * )e->e_private;
		e_tmp = mp->mp_next;

		if ( sub ) {
			rc = monitor_send_children( op, rs, e, sub );
			if ( rc ) {
				/* FIXME: may leak generated children */
				if ( nonvolatile == 0 ) {
					for ( ; e_tmp != NULL; ) {
						mp = ( monitor_entry_t * )e_tmp->e_private;
						e = e_tmp;
						e_tmp = mp->mp_next;
						monitor_cache_release( mi, e );
	
						if ( e_tmp == e_nonvolatile ) {
							break;
						}
					}
				}

				return( rc );
			}
		}

		if ( e_tmp != NULL ) {
			monitor_cache_lock( e_tmp );
		}

		if ( !sub ) {
			/* otherwise the recursive call already released */
			monitor_cache_release( mi, e );
		}

		e = e_tmp;
		if ( e == e_nonvolatile ) {
			nonvolatile = 1;
		}
	}
	
	return LDAP_SUCCESS;
}

int
monitor_back_search( Operation *op, SlapReply *rs )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	int		rc = LDAP_SUCCESS;
	Entry		*e = NULL, *matched = NULL;
	slap_mask_t	mask;

	Debug( LDAP_DEBUG_TRACE, "=> monitor_back_search\n", 0, 0, 0 );


	/* get entry with reader lock */
	monitor_cache_dn2entry( op, rs, &op->o_req_ndn, &e, &matched );
	if ( e == NULL ) {
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		if ( matched ) {
			if ( !access_allowed_mask( op, matched,
					slap_schema.si_ad_entry,
					NULL, ACL_DISCLOSE, NULL, NULL ) )
			{
				/* do nothing */ ;
			} else {
				rs->sr_matched = matched->e_dn;
			}
		}

		send_ldap_result( op, rs );
		if ( matched ) {
			monitor_cache_release( mi, matched );
			rs->sr_matched = NULL;
		}

		return rs->sr_err;
	}

	/* NOTE: __NEW__ "search" access is required
	 * on searchBase object */
	if ( !access_allowed_mask( op, e, slap_schema.si_ad_entry,
				NULL, ACL_SEARCH, NULL, &mask ) )
	{
		monitor_cache_release( mi, e );

		if ( !ACL_GRANT( mask, ACL_DISCLOSE ) ) {
			rs->sr_err = LDAP_NO_SUCH_OBJECT;
		} else {
			rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
		}

		send_ldap_result( op, rs );

		return rs->sr_err;
	}

	rs->sr_attrs = op->oq_search.rs_attrs;
	switch ( op->oq_search.rs_scope ) {
	case LDAP_SCOPE_BASE:
		monitor_entry_update( op, rs, e );
		rc = test_filter( op, e, op->oq_search.rs_filter );
 		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_flags = 0;
			send_search_entry( op, rs );
			rs->sr_entry = NULL;
		}
		rc = LDAP_SUCCESS;
		monitor_cache_release( mi, e );
		break;

	case LDAP_SCOPE_ONELEVEL:
	case LDAP_SCOPE_SUBORDINATE:
		rc = monitor_send_children( op, rs, e,
			op->oq_search.rs_scope == LDAP_SCOPE_SUBORDINATE );
		break;

	case LDAP_SCOPE_SUBTREE:
		monitor_entry_update( op, rs, e );
		rc = test_filter( op, e, op->oq_search.rs_filter );
		if ( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = e;
			rs->sr_flags = 0;
			send_search_entry( op, rs );
			rs->sr_entry = NULL;
		}

		rc = monitor_send_children( op, rs, e, 1 );
		break;

	default:
		rc = LDAP_UNWILLING_TO_PERFORM;
		monitor_cache_release( mi, e );
	}

	rs->sr_attrs = NULL;
	rs->sr_err = rc;
	if ( rs->sr_err != SLAPD_ABANDON ) {
		send_ldap_result( op, rs );
	}

	return rs->sr_err;
}

