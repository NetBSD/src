/* dynlist.c - dynamic list overlay */
/* $OpenLDAP: pkg/ldap/servers/slapd/overlays/dynlist.c,v 1.20.2.17 2008/07/10 00:43:03 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2008 The OpenLDAP Foundation.
 * Portions Copyright 2004-2005 Pierangelo Masarati.
 * Portions Copyright 2008 Emmanuel Dreyfus.
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
 * This work was initially developed by Pierangelo Masarati
 * for SysNet s.n.c., for inclusion in OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_DYNLIST

#if LDAP_VENDOR_VERSION_MINOR == X || LDAP_VENDOR_VERSION_MINOR > 3
#if SLAPD_OVER_DYNGROUP != SLAPD_MOD_STATIC
#define TAKEOVER_DYNGROUP
#endif
#else
#if LDAP_VENDOR_VERSION_MINOR < 3
#define OL_2_2_COMPAT
#endif
#endif

#include <stdio.h>

#include <ac/string.h>

#include "slap.h"
#ifndef OL_2_2_COMPAT
#include "config.h"
#endif
#include "lutil.h"

/* FIXME: the code differs if SLAP_OPATTRS is defined or not;
 * SLAP_OPATTRS is not defined in 2.2 yet, while this overlay
 * expects HEAD code at least later than August 6, 2004. */
/* FIXME: slap_anlist_no_attrs was introduced in 2.3; here it
 * is anticipated to allow using this overlay with 2.2. */

#ifdef OL_2_2_COMPAT
static AttributeName anlist_no_attrs[] = {
	{ BER_BVC( LDAP_NO_ATTRS ), NULL, 0, NULL },
	{ BER_BVNULL, NULL, 0, NULL }
};

static AttributeName *slap_anlist_no_attrs = anlist_no_attrs;
#endif

static AttributeDescription *ad_dgIdentity, *ad_dgAuthz;

typedef struct dynlist_map_t {
	AttributeDescription *dlm_member_ad;
	AttributeDescription *dlm_mapped_ad;
	struct dynlist_map_t *dlm_next;
} dynlist_map_t;

typedef struct dynlist_info_t {
	ObjectClass		*dli_oc;
	AttributeDescription	*dli_ad;
	struct dynlist_map_t	*dli_dlm;
	struct berval		dli_default_filter;
	struct dynlist_info_t	*dli_next;
} dynlist_info_t;

#define DYNLIST_USAGE \
	"\"dynlist-attrset <oc> <URL-ad> [[<mapped-ad>:]<member-ad> ...]\": "

static dynlist_info_t *
dynlist_is_dynlist_next( Operation *op, SlapReply *rs, dynlist_info_t *old_dli )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dynlist_info_t	*dli;

	Attribute	*a;

	if ( old_dli == NULL ) {
		dli = (dynlist_info_t *)on->on_bi.bi_private;

	} else {
		dli = old_dli->dli_next;
	}

	a = attrs_find( rs->sr_entry->e_attrs, slap_schema.si_ad_objectClass );
	if ( a == NULL ) {
		/* FIXME: objectClass must be present; for non-storage
		 * backends, like back-ldap, it needs to be added
		 * to the requested attributes */
		return NULL;
	}

	for ( ; dli; dli = dli->dli_next ) {
		if ( attr_valfind( a,
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				&dli->dli_oc->soc_cname, NULL,
				op->o_tmpmemctx ) == 0 )
		{
			return dli;
		}
	}

	return NULL;
}

static int
dynlist_make_filter( Operation *op, struct berval *oldf, struct berval *newf )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dynlist_info_t	*dli = (dynlist_info_t *)on->on_bi.bi_private;

	char		*ptr;

	assert( oldf != NULL );
	assert( newf != NULL );
	assert( !BER_BVISNULL( oldf ) );
	assert( !BER_BVISEMPTY( oldf ) );

	newf->bv_len = STRLENOF( "(&(!(objectClass=" "))" ")" )
		+ dli->dli_oc->soc_cname.bv_len + oldf->bv_len;
	newf->bv_val = op->o_tmpalloc( newf->bv_len + 1, op->o_tmpmemctx );
	if ( newf->bv_val == NULL ) {
		return -1;
	}
	ptr = lutil_strcopy( newf->bv_val, "(&(!(objectClass=" );
	ptr = lutil_strcopy( ptr, dli->dli_oc->soc_cname.bv_val );
	ptr = lutil_strcopy( ptr, "))" );
	ptr = lutil_strcopy( ptr, oldf->bv_val );
	ptr = lutil_strcopy( ptr, ")" );
	newf->bv_len = ptr - newf->bv_val;

	return 0;
}

typedef struct dynlist_sc_t {
	dynlist_info_t    *dlc_dli;
	Entry		*dlc_e;
} dynlist_sc_t;

static int
dynlist_sc_update( Operation *op, SlapReply *rs )
{
	Entry			*e;
	Attribute		*a;
	int			opattrs,
				userattrs;
	AccessControlState	acl_state = ACL_STATE_INIT;

	dynlist_sc_t		*dlc;
	dynlist_map_t		*dlm;

	if ( rs->sr_type != REP_SEARCH ) {
		return 0;
	}

	dlc = (dynlist_sc_t *)op->o_callback->sc_private;
	e = dlc->dlc_e;

	assert( e != NULL );
	assert( rs->sr_entry != NULL );

	/* test access to entry */
	if ( !access_allowed( op, rs->sr_entry, slap_schema.si_ad_entry,
				NULL, ACL_READ, NULL ) )
	{
		goto done;
	}

	/* if there is only one member_ad, and it's not mapped,
	 * consider it as old-style member listing */
	dlm = dlc->dlc_dli->dli_dlm;
	if ( dlm && dlm->dlm_mapped_ad == NULL && dlm->dlm_next == NULL ) {
		/* if access allowed, try to add values, emulating permissive
		 * control to silently ignore duplicates */
		if ( access_allowed( op, rs->sr_entry, slap_schema.si_ad_entry,
					NULL, ACL_READ, NULL ) )
		{
			Modification	mod;
			const char	*text = NULL;
			char		textbuf[1024];
			struct berval	vals[ 2 ], nvals[ 2 ];

			vals[ 0 ] = rs->sr_entry->e_name;
			BER_BVZERO( &vals[ 1 ] );
			nvals[ 0 ] = rs->sr_entry->e_nname;
			BER_BVZERO( &nvals[ 1 ] );

			mod.sm_op = LDAP_MOD_ADD;
			mod.sm_desc = dlm->dlm_member_ad;
			mod.sm_type = dlm->dlm_member_ad->ad_cname;
			mod.sm_values = vals;
			mod.sm_nvalues = nvals;
			mod.sm_numvals = 1;

			(void)modify_add_values( e, &mod, /* permissive */ 1,
					&text, textbuf, sizeof( textbuf ) );
		}

		goto done;
	}

#ifndef SLAP_OPATTRS
	opattrs = ( rs->sr_attrs == NULL ) ? 0 : an_find( rs->sr_attrs, &AllOper );
	userattrs = ( rs->sr_attrs == NULL ) ? 1 : an_find( rs->sr_attrs, &AllUser );
#else /* SLAP_OPATTRS */
	opattrs = SLAP_OPATTRS( rs->sr_attr_flags );
	userattrs = SLAP_USERATTRS( rs->sr_attr_flags );
#endif /* SLAP_OPATTRS */

	for ( a = rs->sr_entry->e_attrs; a != NULL; a = a->a_next ) {
		BerVarray	vals, nvals = NULL;
		int		i, j,
				is_oc = a->a_desc == slap_schema.si_ad_objectClass;

		/* if attribute is not requested, skip it */
		if ( rs->sr_attrs == NULL ) {
			if ( is_at_operational( a->a_desc->ad_type ) ) {
				continue;
			}

		} else {
			if ( is_at_operational( a->a_desc->ad_type ) ) {
				if ( !opattrs && !ad_inlist( a->a_desc, rs->sr_attrs ) )
				{
					continue;
				}

			} else {
				if ( !userattrs && !ad_inlist( a->a_desc, rs->sr_attrs ) )
				{
					continue;
				}
			}
		}

		/* test access to attribute */
		if ( op->ors_attrsonly ) {
			if ( !access_allowed( op, rs->sr_entry, a->a_desc, NULL,
						ACL_READ, &acl_state ) )
			{
				continue;
			}
		}

		/* single-value check: keep first only */
		if ( is_at_single_value( a->a_desc->ad_type ) ) {
			if ( attr_find( e->e_attrs, a->a_desc ) != NULL ) {
				continue;
			}
		}

		/* test access to attribute */
		i = a->a_numvals;

		vals = op->o_tmpalloc( ( i + 1 ) * sizeof( struct berval ), op->o_tmpmemctx );
		if ( a->a_nvals != a->a_vals ) {
			nvals = op->o_tmpalloc( ( i + 1 ) * sizeof( struct berval ), op->o_tmpmemctx );
		}

		for ( i = 0, j = 0; !BER_BVISNULL( &a->a_vals[i] ); i++ ) {
			if ( is_oc ) {
				ObjectClass	*soc = oc_bvfind( &a->a_vals[i] );

				if ( soc->soc_kind == LDAP_SCHEMA_STRUCTURAL ) {
					continue;
				}
			}

			if ( access_allowed( op, rs->sr_entry, a->a_desc,
						&a->a_nvals[i], ACL_READ, &acl_state ) )
			{
				vals[j] = a->a_vals[i];
				if ( nvals ) {
					nvals[j] = a->a_nvals[i];
				}
				j++;
			}
		}

		/* if access allowed, try to add values, emulating permissive
		 * control to silently ignore duplicates */
		if ( j != 0 ) {
			Modification	mod;
			const char	*text = NULL;
			char		textbuf[1024];
			dynlist_map_t	*dlm;
			AttributeDescription *ad;

			BER_BVZERO( &vals[j] );
			if ( nvals ) {
				BER_BVZERO( &nvals[j] );
			}

			ad = a->a_desc;
			for ( dlm = dlc->dlc_dli->dli_dlm; dlm; dlm = dlm->dlm_next ) {
				if ( dlm->dlm_member_ad == a->a_desc ) {
					ad = dlm->dlm_mapped_ad;
					break;
				}
			}

			mod.sm_op = LDAP_MOD_ADD;
			mod.sm_desc = ad;
			mod.sm_type = ad->ad_cname;
			mod.sm_values = vals;
			mod.sm_nvalues = nvals;
			mod.sm_numvals = j;

			(void)modify_add_values( e, &mod, /* permissive */ 1,
					&text, textbuf, sizeof( textbuf ) );
		}

		op->o_tmpfree( vals, op->o_tmpmemctx );
		if ( nvals ) {
			op->o_tmpfree( nvals, op->o_tmpmemctx );
		}
	}

done:;
	if ( rs->sr_flags & REP_ENTRY_MUSTBEFREED ) {
		entry_free( rs->sr_entry );
		rs->sr_entry = NULL;
		rs->sr_flags ^= REP_ENTRY_MUSTBEFREED;
	}

	return 0;
}
	
static int
dynlist_prepare_entry( Operation *op, SlapReply *rs, dynlist_info_t *dli )
{
	Attribute	*a, *id = NULL;
	slap_callback	cb;
	Operation	o = *op;
	SlapReply	r = { REP_SEARCH };
	struct berval	*url;
	Entry		*e;
	slap_mask_t	e_flags;
	int		opattrs,
			userattrs;
	dynlist_sc_t	dlc = { 0 };
	dynlist_map_t	*dlm;

	a = attrs_find( rs->sr_entry->e_attrs, dli->dli_ad );
	if ( a == NULL ) {
		/* FIXME: error? */
		return SLAP_CB_CONTINUE;
	}

#ifndef SLAP_OPATTRS
	opattrs = ( rs->sr_attrs == NULL ) ? 0 : an_find( rs->sr_attrs, &AllOper );
	userattrs = ( rs->sr_attrs == NULL ) ? 1 : an_find( rs->sr_attrs, &AllUser );
#else /* SLAP_OPATTRS */
	opattrs = SLAP_OPATTRS( rs->sr_attr_flags );
	userattrs = SLAP_USERATTRS( rs->sr_attr_flags );
#endif /* SLAP_OPATTRS */

	/* Don't generate member list if it wasn't requested */
	for ( dlm = dli->dli_dlm; dlm; dlm = dlm->dlm_next ) {
		if ( userattrs ||
		     ad_inlist( dlm->dlm_member_ad, rs->sr_attrs ) ) 
			break;
	}
	if ( dli->dli_dlm && !dlm )
		return SLAP_CB_CONTINUE;

	if ( ad_dgIdentity && ( id = attrs_find( rs->sr_entry->e_attrs, ad_dgIdentity ))) {
		Attribute *authz = NULL;

		/* if not rootdn and dgAuthz is present,
		 * check if user can be authorized as dgIdentity */
		if ( ad_dgAuthz && !BER_BVISEMPTY( &id->a_nvals[0] ) && !be_isroot( op )
			&& ( authz = attrs_find( rs->sr_entry->e_attrs, ad_dgAuthz ) ) )
		{
			if ( slap_sasl_matches( op, authz->a_nvals,
				&o.o_ndn, &o.o_ndn ) != LDAP_SUCCESS )
			{
				return SLAP_CB_CONTINUE;
			}
		}

		o.o_dn = id->a_vals[0];
		o.o_ndn = id->a_nvals[0];
		o.o_groups = NULL;
	}

	if ( !( rs->sr_flags & REP_ENTRY_MODIFIABLE ) ) {
		e = entry_dup( rs->sr_entry );
	} else {
		e = rs->sr_entry;
	}
	e_flags = rs->sr_flags | ( REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED );

	dlc.dlc_e = e;
	dlc.dlc_dli = dli;
	cb.sc_private = &dlc;
	cb.sc_response = dynlist_sc_update;
	cb.sc_cleanup = NULL;
	cb.sc_next = NULL;

	o.o_callback = &cb;
	o.ors_deref = LDAP_DEREF_NEVER;
	o.ors_limit = NULL;
	o.ors_tlimit = SLAP_NO_LIMIT;
	o.ors_slimit = SLAP_NO_LIMIT;

	for ( url = a->a_nvals; !BER_BVISNULL( url ); url++ ) {
		LDAPURLDesc	*lud = NULL;
		int		i, j;
		struct berval	dn;
		int		rc;
		dynlist_map_t	*dlm;

		BER_BVZERO( &o.o_req_dn );
		BER_BVZERO( &o.o_req_ndn );
		o.ors_filter = NULL;
		o.ors_attrs = NULL;
		BER_BVZERO( &o.ors_filterstr );

		if ( ldap_url_parse( url->bv_val, &lud ) != LDAP_URL_SUCCESS ) {
			/* FIXME: error? */
			continue;
		}

		if ( lud->lud_host != NULL ) {
			/* FIXME: host not allowed; reject as illegal? */
			Debug( LDAP_DEBUG_ANY, "dynlist_prepare_entry(\"%s\"): "
				"illegal URI \"%s\"\n",
				e->e_name.bv_val, url->bv_val, 0 );
			goto cleanup;
		}

		if ( lud->lud_dn == NULL ) {
			/* note that an empty base is not honored in terms
			 * of defaultSearchBase, because select_backend()
			 * is not aware of the defaultSearchBase option;
			 * this can be useful in case of a database serving
			 * the empty suffix */
			BER_BVSTR( &dn, "" );

		} else {
			ber_str2bv( lud->lud_dn, 0, 0, &dn );
		}
		rc = dnPrettyNormal( NULL, &dn, &o.o_req_dn, &o.o_req_ndn, op->o_tmpmemctx );
		if ( rc != LDAP_SUCCESS ) {
			/* FIXME: error? */
			goto cleanup;
		}
		o.ors_scope = lud->lud_scope;

		for ( dlm = dli->dli_dlm; dlm; dlm = dlm->dlm_next ) {
			if ( dlm->dlm_mapped_ad != NULL ) {
				break;
			}
		}

		if ( dli->dli_dlm && !dlm ) {
			/* if ( lud->lud_attrs != NULL ),
			 * the URL should be ignored */
			o.ors_attrs = slap_anlist_no_attrs;

		} else if ( lud->lud_attrs == NULL ) {
			o.ors_attrs = rs->sr_attrs;

		} else {
			for ( i = 0; lud->lud_attrs[i]; i++)
				/* just count */ ;

			o.ors_attrs = op->o_tmpcalloc( i + 1, sizeof( AttributeName ), op->o_tmpmemctx );
			for ( i = 0, j = 0; lud->lud_attrs[i]; i++) {
				const char	*text = NULL;
	
				ber_str2bv( lud->lud_attrs[i], 0, 0, &o.ors_attrs[j].an_name );
				o.ors_attrs[j].an_desc = NULL;
				(void)slap_bv2ad( &o.ors_attrs[j].an_name, &o.ors_attrs[j].an_desc, &text );
				/* FIXME: ignore errors... */

				if ( rs->sr_attrs == NULL ) {
					if ( o.ors_attrs[j].an_desc != NULL &&
							is_at_operational( o.ors_attrs[j].an_desc->ad_type ) )
					{
						continue;
					}

				} else {
					if ( o.ors_attrs[j].an_desc != NULL &&
							is_at_operational( o.ors_attrs[j].an_desc->ad_type ) )
					{
						if ( !opattrs && !ad_inlist( o.ors_attrs[j].an_desc, rs->sr_attrs ) )
						{
							continue;
						}

					} else {
						if ( !userattrs && 
								o.ors_attrs[j].an_desc != NULL &&
								!ad_inlist( o.ors_attrs[j].an_desc, rs->sr_attrs ) )
						{
							continue;
						}
					}
				}

				j++;
			}

			if ( j == 0 ) {
				goto cleanup;
			}
		
			BER_BVZERO( &o.ors_attrs[j].an_name );
		}

		if ( lud->lud_filter == NULL ) {
			ber_dupbv_x( &o.ors_filterstr,
					&dli->dli_default_filter, op->o_tmpmemctx );

		} else {
			struct berval	flt;
			ber_str2bv( lud->lud_filter, 0, 0, &flt );
			if ( dynlist_make_filter( op, &flt, &o.ors_filterstr ) ) {
				/* error */
				goto cleanup;
			}
		}
		o.ors_filter = str2filter_x( op, o.ors_filterstr.bv_val );
		if ( o.ors_filter == NULL ) {
			goto cleanup;
		}
		
		o.o_bd = select_backend( &o.o_req_ndn, 1 );
		if ( o.o_bd && o.o_bd->be_search ) {
#ifdef SLAP_OPATTRS
			r.sr_attr_flags = slap_attr_flags( o.ors_attrs );
#endif /* SLAP_OPATTRS */
			(void)o.o_bd->be_search( &o, &r );
		}

cleanup:;
		if ( id ) {
			slap_op_groups_free( &o );
		}
		if ( o.ors_filter ) {
			filter_free_x( &o, o.ors_filter );
		}
		if ( o.ors_attrs && o.ors_attrs != rs->sr_attrs
				&& o.ors_attrs != slap_anlist_no_attrs )
		{
			op->o_tmpfree( o.ors_attrs, op->o_tmpmemctx );
		}
		if ( !BER_BVISNULL( &o.o_req_dn ) ) {
			op->o_tmpfree( o.o_req_dn.bv_val, op->o_tmpmemctx );
		}
		if ( !BER_BVISNULL( &o.o_req_ndn ) ) {
			op->o_tmpfree( o.o_req_ndn.bv_val, op->o_tmpmemctx );
		}
		assert( BER_BVISNULL( &o.ors_filterstr )
			|| o.ors_filterstr.bv_val != lud->lud_filter );
		op->o_tmpfree( o.ors_filterstr.bv_val, op->o_tmpmemctx );
		ldap_free_urldesc( lud );
	}

	rs->sr_entry = e;
	rs->sr_flags = e_flags;

	return SLAP_CB_CONTINUE;
}

static int
dynlist_sc_save_entry( Operation *op, SlapReply *rs )
{
	/* save the entry in the private field of the callback,
	 * so it doesn't get freed (it's temporary!) */
	if ( rs->sr_entry != NULL ) {
		dynlist_sc_t	*dlc = (dynlist_sc_t *)op->o_callback->sc_private;
		dlc->dlc_e = rs->sr_entry;
		rs->sr_entry = NULL;
	}

	return 0;
}

static int
dynlist_compare( Operation *op, SlapReply *rs )
{
	slap_overinst	*on = (slap_overinst *)op->o_bd->bd_info;
	dynlist_info_t	*dli = (dynlist_info_t *)on->on_bi.bi_private;
	Operation o = *op;
	Entry *e = NULL;
	dynlist_map_t *dlm;

	for ( ; dli != NULL; dli = dli->dli_next ) {
		for ( dlm = dli->dli_dlm; dlm; dlm = dlm->dlm_next )
			if ( op->oq_compare.rs_ava->aa_desc == dlm->dlm_member_ad )
				break;

		if ( dli->dli_dlm && dlm ) {
			/* This compare is for one of the attributes we're
			 * interested in. We'll use slapd's existing dyngroup
			 * evaluator to get the answer we want.
			 */
			BerVarray id = NULL, authz = NULL;

			o.o_do_not_cache = 1;

			if ( ad_dgIdentity && backend_attribute( &o, NULL, &o.o_req_ndn,
				ad_dgIdentity, &id, ACL_READ ) == LDAP_SUCCESS )
			{
				/* if not rootdn and dgAuthz is present,
				 * check if user can be authorized as dgIdentity */
				if ( ad_dgAuthz && !BER_BVISEMPTY( id ) && !be_isroot( op )
					&& backend_attribute( &o, NULL, &o.o_req_ndn,
						ad_dgAuthz, &authz, ACL_READ ) == LDAP_SUCCESS )
				{
					
					rs->sr_err = slap_sasl_matches( op, authz,
						&o.o_ndn, &o.o_ndn );
					ber_bvarray_free_x( authz, op->o_tmpmemctx );
					if ( rs->sr_err != LDAP_SUCCESS ) {
						goto done;
					}
				}

				o.o_dn = *id;
				o.o_ndn = *id;
				o.o_groups = NULL; /* authz changed, invalidate cached groups */
			}

			rs->sr_err = backend_group( &o, NULL, &o.o_req_ndn,
				&o.oq_compare.rs_ava->aa_value, dli->dli_oc, dli->dli_ad );
			switch ( rs->sr_err ) {
			case LDAP_SUCCESS:
				rs->sr_err = LDAP_COMPARE_TRUE;
				break;

			case LDAP_NO_SUCH_OBJECT:
				/* NOTE: backend_group() returns noSuchObject
				 * if op_ndn does not exist; however, since
				 * dynamic list expansion means that the
				 * member attribute is virtually present, the
				 * non-existence of the asserted value implies
				 * the assertion is FALSE rather than
				 * UNDEFINED */
				rs->sr_err = LDAP_COMPARE_FALSE;
				break;
			}

done:;
			if ( id ) ber_bvarray_free_x( id, o.o_tmpmemctx );

			return SLAP_CB_CONTINUE;
		}
	}

	if ( overlay_entry_get_ov( &o, &o.o_req_ndn, NULL, NULL, 0, &e, on ) !=
		LDAP_SUCCESS || e == NULL )
	{
		return SLAP_CB_CONTINUE;
	}

	if ( ad_dgIdentity ) {
		Attribute *id = attrs_find( e->e_attrs, ad_dgIdentity );
		if ( id ) {
			Attribute *authz;

			/* if not rootdn and dgAuthz is present,
			 * check if user can be authorized as dgIdentity */
			if ( ad_dgAuthz && !BER_BVISEMPTY( &id->a_nvals[0] ) && !be_isroot( op )
				&& ( authz = attrs_find( e->e_attrs, ad_dgAuthz ) ) )
			{
				if ( slap_sasl_matches( op, authz->a_nvals,
					&o.o_ndn, &o.o_ndn ) != LDAP_SUCCESS )
				{
					goto release;
				}
			}

			o.o_dn = id->a_vals[0];
			o.o_ndn = id->a_nvals[0];
			o.o_groups = NULL;
		}
	}

	dli = (dynlist_info_t *)on->on_bi.bi_private;
	for ( ; dli != NULL && rs->sr_err != LDAP_COMPARE_TRUE; dli = dli->dli_next ) {
		Attribute	*a;
		slap_callback	cb;
		SlapReply	r = { REP_SEARCH };
		AttributeName	an[2];
		int		rc;
		dynlist_sc_t	dlc = { 0 };

		if ( !is_entry_objectclass_or_sub( e, dli->dli_oc ))
			continue;

		/* if the entry has the right objectClass, generate
		 * the dynamic list and compare */
		dlc.dlc_dli = dli;
		cb.sc_private = &dlc;
		cb.sc_response = dynlist_sc_save_entry;
		cb.sc_cleanup = NULL;
		cb.sc_next = NULL;
		o.o_callback = &cb;

		o.o_tag = LDAP_REQ_SEARCH;
		o.ors_limit = NULL;
		o.ors_tlimit = SLAP_NO_LIMIT;
		o.ors_slimit = SLAP_NO_LIMIT;

		o.o_bd = select_backend( &o.o_req_ndn, 1 );
		if ( !o.o_bd || !o.o_bd->be_search ) {
			goto release;
		}

		o.ors_filterstr = *slap_filterstr_objectClass_pres;
		o.ors_filter = (Filter *) slap_filter_objectClass_pres;

		o.ors_scope = LDAP_SCOPE_BASE;
		o.ors_deref = LDAP_DEREF_NEVER;
		an[0].an_name = op->orc_ava->aa_desc->ad_cname;
		an[0].an_desc = op->orc_ava->aa_desc;
		BER_BVZERO( &an[1].an_name );
		o.ors_attrs = an;
		o.ors_attrsonly = 0;

		o.o_acl_priv = ACL_COMPARE;

		rc = o.o_bd->be_search( &o, &r );

		if ( o.o_dn.bv_val != op->o_dn.bv_val ) {
			slap_op_groups_free( &o );
		}

		if ( rc != 0 ) {
			goto release;
		}

		if ( dlc.dlc_e != NULL ) {
			r.sr_entry = dlc.dlc_e;
		}

		if ( r.sr_err != LDAP_SUCCESS || r.sr_entry == NULL ) {
			/* error? */
			goto release;
		}

		for ( a = attrs_find( r.sr_entry->e_attrs, op->orc_ava->aa_desc );
			a != NULL;
			a = attrs_find( a->a_next, op->orc_ava->aa_desc ) )
		{
			/* if we're here, we got a match... */
			rs->sr_err = LDAP_COMPARE_FALSE;

			if ( attr_valfind( a,
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
					SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				&op->orc_ava->aa_value, NULL, op->o_tmpmemctx ) == 0 )
			{
				rs->sr_err = LDAP_COMPARE_TRUE;
				break;
			}
		}

		if ( r.sr_flags & REP_ENTRY_MUSTBEFREED ) {
			entry_free( r.sr_entry );
		}
	}

release:;
	if ( e != NULL ) {
		overlay_entry_release_ov( op, e, 0, on );
	}

	return SLAP_CB_CONTINUE;
}

static int
dynlist_response( Operation *op, SlapReply *rs )
{
	dynlist_info_t	*dli;

	switch ( op->o_tag ) {
	case LDAP_REQ_SEARCH:
		if ( rs->sr_type == REP_SEARCH && !get_manageDSAit( op ) )
		{
			int	rc = LDAP_OTHER;

			for ( dli = dynlist_is_dynlist_next( op, rs, NULL );
				dli;
				dli = dynlist_is_dynlist_next( op, rs, dli ) )
			{
				rc = dynlist_prepare_entry( op, rs, dli );
			}

			if ( rc != LDAP_OTHER ) {
				return rc;
			}
		}
		break;

	case LDAP_REQ_COMPARE:
		switch ( rs->sr_err ) {
		/* NOTE: we waste a few cycles running the dynamic list
		 * also when the result is FALSE, which occurs if the
		 * dynamic entry itself contains the AVA attribute  */
		/* FIXME: this approach is less than optimal; a dedicated
		 * compare op should be implemented, that fetches the
		 * entry, checks if it has the appropriate objectClass
		 * and, in case, runs a compare thru all the URIs,
		 * stopping at the first positive occurrence; see ITS#3756 */
		case LDAP_COMPARE_FALSE:
		case LDAP_NO_SUCH_ATTRIBUTE:
			return dynlist_compare( op, rs );
		}
		break;

	default:
		break;
	}

	return SLAP_CB_CONTINUE;
}

static int
dynlist_build_def_filter( dynlist_info_t *dli )
{
	char	*ptr;

	dli->dli_default_filter.bv_len = STRLENOF( "(!(objectClass=" "))" )
		+ dli->dli_oc->soc_cname.bv_len;
	dli->dli_default_filter.bv_val = ch_malloc( dli->dli_default_filter.bv_len + 1 );
	if ( dli->dli_default_filter.bv_val == NULL ) {
		Debug( LDAP_DEBUG_ANY, "dynlist_db_open: malloc failed.\n",
			0, 0, 0 );
		return -1;
	}

	ptr = lutil_strcopy( dli->dli_default_filter.bv_val, "(!(objectClass=" );
	ptr = lutil_strcopy( ptr, dli->dli_oc->soc_cname.bv_val );
	ptr = lutil_strcopy( ptr, "))" );

	assert( dli->dli_default_filter.bv_len == ptr - dli->dli_default_filter.bv_val );

	return 0;
}

#ifdef OL_2_2_COMPAT
static int
dynlist_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;

	int		rc = 0;

	if ( strcasecmp( argv[0], "dynlist-attrset" ) == 0 ) {
		dynlist_info_t		**dlip;
		ObjectClass		*oc;
		AttributeDescription	*ad = NULL,
					*member_ad = NULL;
		dynlist_map_t		*dlm = NULL;
		const char		*text;

		if ( argc < 3 ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: " DYNLIST_USAGE
				"invalid arg number #%d.\n",
				fname, lineno, argc );
			return 1;
		}

		oc = oc_find( argv[1] );
		if ( oc == NULL ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: " DYNLIST_USAGE
				"unable to find ObjectClass \"%s\"\n",
				fname, lineno, argv[ 1 ] );
			return 1;
		}

		rc = slap_str2ad( argv[2], &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: " DYNLIST_USAGE
				"unable to find AttributeDescription \"%s\"\n",
				fname, lineno, argv[2] );
			return 1;
		}

		if ( !is_at_subtype( ad->ad_type, slap_schema.si_ad_labeledURI->ad_type ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: " DYNLIST_USAGE
				"AttributeDescription \"%s\" "
				"must be a subtype of \"labeledURI\"\n",
				fname, lineno, argv[2] );
			return 1;
		}

		for ( i = 3; i < argc; i++ ) {
			char *arg; 
			char *cp;
			AttributeDescription *member_ad = NULL;
			AttributeDescription *mapped_ad = NULL;
			dynlist_map_t *dlmp;
			dynlist_map_t *dlml;


			/*
			 * If no mapped attribute is given, dn is used 
			 * for backward compatibility.
			 */
			arg = argv[i];
			if ( cp = strchr( arg, (int)':' ) != NULL ) {
				struct berval bv;
				ber_str2bv( arg, cp - arg, 0, &bv );
				rc = slap_bv2ad( &bv, &mapped_ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					Debug( LDAP_DEBUG_ANY, "%s: line %d: "
						DYNLIST_USAGE
						"unable to find mapped AttributeDescription \"%s\"\n",
						fname, lineno, arg );
					return 1;
				}
				
				arg = cp + 1;
			}

			rc = slap_str2ad( arg, &member_ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "%s: line %d: "
					DYNLIST_USAGE
					"unable to find AttributeDescription \"%s\"\n",
					fname, lineno, arg );
				return 1;
			}

			dlmp = (dynlist_map_t *)ch_calloc( 1, sizeof( dynlist_map_t ) );
			if ( dlm == NULL ) {
				dlm = dlmp;
				dlml = NULL;
			}
			dlmp->dlm_member_ad = member_ad;
			dlmp->dlm_mapped_ad = mapped_ad;
			dlmp->dlm_next = NULL;
		
			if ( dlml != NULL )
				dlml->dlm_next = dlmp;
			dlml = dlmp;
		}

		for ( dlip = (dynlist_info_t **)&on->on_bi.bi_private;
			*dlip; dlip = &(*dlip)->dli_next )
		{
			/* 
			 * The same URL attribute / member attribute pair
			 * cannot be repeated, but we enforce this only 
			 * when the member attribute is unique. Performing
			 * the check for multiple values would require
			 * sorting and comparing the lists, which is left
			 * as a future improvement
			 */
			if ( (*dlip)->dli_ad == ad &&
			     (*dlip)->dli_dlm->dlm_next == NULL &&
			     dlm->dlm_next == NULL &&
			     dlm->dlm_member_ad == (*dlip)->dli_dlm->dlm_member_ad &&
			     dlm->dlm_mapped_ad == (*dlip)->dli_dlm->dlm_mapped_ad ) {
				Debug( LDAP_DEBUG_ANY, "%s: line %d: "
					DYNLIST_USAGE
					"URL attributeDescription \"%s\" already mapped.\n",
					fname, lineno, ad->ad_cname.bv_val );
#if 0
				/* make it a warning... */
				return 1;
#endif
			}
		}

		*dlip = (dynlist_info_t *)ch_calloc( 1, sizeof( dynlist_info_t ) );
		(*dlip)->dli_oc = oc;
		(*dlip)->dli_ad = ad;
		(*dlip)->dli_dlm = dlm;

		if ( dynlist_build_def_filter( *dlip ) ) {
			dynlist_map_t *dlm = (*dlip)->ldi_dlm;
			dynlist_map_t *dlm_next;

			while ( dlm != NULL ) {
				dlm_next = dlm->dlm_next;
				ch_free( dlm );
				dlm = dlm_next;
			}

			ch_free( *dlip );
			*dlip = NULL;
			return 1;
		}

	/* allow dyngroup syntax */
	} else if ( strcasecmp( argv[0], "dynlist-attrpair" ) == 0 ) {
		dynlist_info_t		**dlip;
		ObjectClass		*oc;
		AttributeDescription	*ad = NULL,
					*member_ad = NULL;
		const char		*text;

		if ( argc != 3 ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"invalid arg number #%d.\n",
				fname, lineno, argc );
			return 1;
		}

		oc = oc_find( "groupOfURLs" );
		if ( oc == NULL ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"unable to find default ObjectClass \"groupOfURLs\"\n",
				fname, lineno, 0 );
			return 1;
		}

		rc = slap_str2ad( argv[1], &member_ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"unable to find AttributeDescription \"%s\"\n",
				fname, lineno, argv[1] );
			return 1;
		}

		rc = slap_str2ad( argv[2], &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"unable to find AttributeDescription \"%s\"\n",
				fname, lineno, argv[2] );
			return 1;
		}

		if ( !is_at_subtype( ad->ad_type, slap_schema.si_ad_labeledURI->ad_type ) ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"AttributeDescription \"%s\" "
				"must be a subtype of \"labeledURI\"\n",
				fname, lineno, argv[2] );
			return 1;
		}

		for ( dlip = (dynlist_info_t **)&on->on_bi.bi_private;
			*dlip; dlip = &(*dlip)->dli_next )
		{
			/* 
			 * The same URL attribute / member attribute pair
			 * cannot be repeated, but we enforce this only 
			 * when the member attribute is unique. Performing
			 * the check for multiple values would require
			 * sorting and comparing the lists, which is left
			 * as a future improvement
			 */
			if ( (*dlip)->dli_ad == ad &&
			     (*dlip)->dli_dlm->dlm_next == NULL &&
			     member_ad == (*dlip)->dli_dlm->dlm_member_ad ) {
				Debug( LDAP_DEBUG_ANY, "%s: line %d: "
					"\"dynlist-attrpair <member-ad> <URL-ad>\": "
					"URL attributeDescription \"%s\" already mapped.\n",
					fname, lineno, ad->ad_cname.bv_val );
#if 0
				/* make it a warning... */
				return 1;
#endif
			}
		}

		*dlip = (dynlist_info_t *)ch_calloc( 1, sizeof( dynlist_info_t ) );
		(*dlip)->dli_oc = oc;
		(*dlip)->dli_ad = ad;
		(*dlip)->dli_dlm = (dynlist_map_t *)ch_calloc( 1, sizeof( dynlist_map_t ) );
		(*dlip)->dli_dlm->dlm_member_ad = member_ad;
		(*dlip)->dli_dlm->dlm_mapped_ad = NULL;

		if ( dynlist_build_def_filter( *dlip ) ) {
			ch_free( (*dlip)->dli_dlm );
			ch_free( *dlip );
			*dlip = NULL;
			return 1;
		}

	} else {
		rc = SLAP_CONF_UNKNOWN;
	}

	return rc;
}

#else
enum {
	DL_ATTRSET = 1,
	DL_ATTRPAIR,
	DL_ATTRPAIR_COMPAT,
	DL_LAST
};

static ConfigDriver	dl_cfgen;

/* XXXmanu 255 is the maximum arguments we allow. Can we go beyond? */
static ConfigTable dlcfg[] = {
	{ "dynlist-attrset", "group-oc> <URL-ad> <member-ad",
		3, 255, 0, ARG_MAGIC|DL_ATTRSET, dl_cfgen,
		"( OLcfgOvAt:8.1 NAME 'olcDLattrSet' "
			"DESC 'Dynamic list: <group objectClass>, <URL attributeDescription>, <member attributeDescription>' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
			NULL, NULL },
	{ "dynlist-attrpair", "member-ad> <URL-ad",
		3, 3, 0, ARG_MAGIC|DL_ATTRPAIR, dl_cfgen,
			NULL, NULL, NULL },
#ifdef TAKEOVER_DYNGROUP
	{ "attrpair", "member-ad> <URL-ad",
		3, 3, 0, ARG_MAGIC|DL_ATTRPAIR_COMPAT, dl_cfgen,
			NULL, NULL, NULL },
#endif
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs dlocs[] = {
	{ "( OLcfgOvOc:8.1 "
		"NAME 'olcDynamicList' "
		"DESC 'Dynamic list configuration' "
		"SUP olcOverlayConfig "
		"MAY olcDLattrSet )",
		Cft_Overlay, dlcfg, NULL, NULL },
	{ NULL, 0, NULL }
};

static int
dl_cfgen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	dynlist_info_t	*dli = (dynlist_info_t *)on->on_bi.bi_private;

	int		rc = 0, i;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case DL_ATTRSET:
			for ( i = 0; dli; i++, dli = dli->dli_next ) {
				struct berval	bv;
				char		*ptr = c->cr_msg;
				dynlist_map_t	*dlm;

				assert( dli->dli_oc != NULL );
				assert( dli->dli_ad != NULL );

				ptr += snprintf( c->cr_msg, sizeof( c->cr_msg ),
					SLAP_X_ORDERED_FMT "%s %s", i,
					dli->dli_oc->soc_cname.bv_val,
					dli->dli_ad->ad_cname.bv_val );

				for ( dlm = dli->dli_dlm; dlm; dlm = dlm->dlm_next ) {
					ptr[ 0 ] = ' ';
					ptr++;
					if ( dlm->dlm_mapped_ad ) {
						ptr = lutil_strcopy( ptr, dlm->dlm_mapped_ad->ad_cname.bv_val );
						ptr[ 0 ] = ':';
						ptr++;
					}
						
					ptr = lutil_strcopy( ptr, dlm->dlm_member_ad->ad_cname.bv_val );
				}

				bv.bv_val = c->cr_msg;
				bv.bv_len = ptr - bv.bv_val;
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		case DL_ATTRPAIR_COMPAT:
		case DL_ATTRPAIR:
			rc = 1;
			break;

		default:
			rc = 1;
			break;
		}

		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case DL_ATTRSET:
			if ( c->valx < 0 ) {
				dynlist_info_t	*dli_next;

				for ( dli_next = dli; dli_next; dli = dli_next ) {
					dynlist_map_t *dlm = dli->dli_dlm;
					dynlist_map_t *dlm_next;

					dli_next = dli->dli_next;

					ch_free( dli->dli_default_filter.bv_val );

					while ( dlm != NULL ) {
						dlm_next = dlm->dlm_next;
						ch_free( dlm );
						dlm = dlm_next;
					}
					ch_free( dli );
				}

				on->on_bi.bi_private = NULL;

			} else {
				dynlist_info_t	**dlip;
				dynlist_map_t *dlm;
				dynlist_map_t *dlm_next;

				for ( i = 0, dlip = (dynlist_info_t **)&on->on_bi.bi_private;
					i < c->valx; i++ )
				{
					if ( *dlip == NULL ) {
						return 1;
					}
					dlip = &(*dlip)->dli_next;
				}

				dli = *dlip;
				*dlip = dli->dli_next;
				ch_free( dli->dli_default_filter.bv_val );

				dlm = dli->dli_dlm;
				while ( dlm != NULL ) {
					dlm_next = dlm->dlm_next;
					ch_free( dlm );
					dlm = dlm_next;
				}
				ch_free( dli );

				dli = (dynlist_info_t *)on->on_bi.bi_private;
			}
			break;

		case DL_ATTRPAIR_COMPAT:
		case DL_ATTRPAIR:
			rc = 1;
			break;

		default:
			rc = 1;
			break;
		}

		return rc;
	}

	switch( c->type ) {
	case DL_ATTRSET: {
		dynlist_info_t		**dlip,
					*dli_next = NULL;
		ObjectClass		*oc = NULL;
		AttributeDescription	*ad = NULL;
		dynlist_map_t           *dlm = NULL;
		const char		*text;

		oc = oc_find( c->argv[ 1 ] );
		if ( oc == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), DYNLIST_USAGE
				"unable to find ObjectClass \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		rc = slap_str2ad( c->argv[ 2 ], &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), DYNLIST_USAGE
				"unable to find AttributeDescription \"%s\"",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		if ( !is_at_subtype( ad->ad_type, slap_schema.si_ad_labeledURI->ad_type ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), DYNLIST_USAGE
				"AttributeDescription \"%s\" "
				"must be a subtype of \"labeledURI\"",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		for ( i = 3; i < c->argc; i++ ) {
			char *arg; 
			char *cp;
			AttributeDescription *member_ad = NULL;
			AttributeDescription *mapped_ad = NULL;
			dynlist_map_t *dlmp;
			dynlist_map_t *dlml;


			/*
			 * If no mapped attribute is given, dn is used 
			 * for backward compatibility.
			 */
			arg = c->argv[i];
			if ( ( cp = strchr( arg, ':' ) ) != NULL ) {
				struct berval bv;
				ber_str2bv( arg, cp - arg, 0, &bv );
				rc = slap_bv2ad( &bv, &mapped_ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						DYNLIST_USAGE
						"unable to find mapped AttributeDescription #%d \"%s\"\n",
						i - 3, c->argv[ i ] );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
						c->log, c->cr_msg, 0 );
					return 1;
				}
				arg = cp + 1;
			}

			rc = slap_str2ad( arg, &member_ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					DYNLIST_USAGE
					"unable to find AttributeDescription #%d \"%s\"\n",
					i - 3, c->argv[ i ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}

			dlmp = (dynlist_map_t *)ch_calloc( 1, sizeof( dynlist_map_t ) );
			if ( dlm == NULL ) {
				dlm = dlmp;
				dlml = NULL;
			}
			dlmp->dlm_member_ad = member_ad;
			dlmp->dlm_mapped_ad = mapped_ad;
			dlmp->dlm_next = NULL;
		
			if ( dlml != NULL ) 
				dlml->dlm_next = dlmp;
			dlml = dlmp;
		}

		if ( c->valx > 0 ) {
			int	i;

			for ( i = 0, dlip = (dynlist_info_t **)&on->on_bi.bi_private;
				i < c->valx; i++ )
			{
				if ( *dlip == NULL ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						DYNLIST_USAGE
						"invalid index {%d}\n",
						c->valx );
					Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
						c->log, c->cr_msg, 0 );
					return 1;
				}
				dlip = &(*dlip)->dli_next;
			}
			dli_next = *dlip;

		} else {
			for ( dlip = (dynlist_info_t **)&on->on_bi.bi_private;
				*dlip; dlip = &(*dlip)->dli_next )
				/* goto last */;
		}

		*dlip = (dynlist_info_t *)ch_calloc( 1, sizeof( dynlist_info_t ) );

		(*dlip)->dli_oc = oc;
		(*dlip)->dli_ad = ad;
		(*dlip)->dli_dlm = dlm;
		(*dlip)->dli_next = dli_next;

		rc = dynlist_build_def_filter( *dlip );

		} break;

	case DL_ATTRPAIR_COMPAT:
		snprintf( c->cr_msg, sizeof( c->cr_msg ),
			"warning: \"attrpair\" only supported for limited "
			"backward compatibility with overlay \"dyngroup\"" );
		Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg, 0 );
		/* fallthru */

	case DL_ATTRPAIR: {
		dynlist_info_t		**dlip;
		ObjectClass		*oc = NULL;
		AttributeDescription	*ad = NULL,
					*member_ad = NULL;
		const char		*text;

		oc = oc_find( "groupOfURLs" );
		if ( oc == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"unable to find default ObjectClass \"groupOfURLs\"" );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		rc = slap_str2ad( c->argv[ 1 ], &member_ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"unable to find AttributeDescription \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		rc = slap_str2ad( c->argv[ 2 ], &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"\"dynlist-attrpair <member-ad> <URL-ad>\": "
				"unable to find AttributeDescription \"%s\"\n",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		if ( !is_at_subtype( ad->ad_type, slap_schema.si_ad_labeledURI->ad_type ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				DYNLIST_USAGE
				"AttributeDescription \"%s\" "
				"must be a subtype of \"labeledURI\"",
				c->argv[ 2 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		for ( dlip = (dynlist_info_t **)&on->on_bi.bi_private;
			*dlip; dlip = &(*dlip)->dli_next )
		{
			/* 
			 * The same URL attribute / member attribute pair
			 * cannot be repeated, but we enforce this only 
			 * when the member attribute is unique. Performing
			 * the check for multiple values would require
			 * sorting and comparing the lists, which is left
			 * as a future improvement
			 */
			if ( (*dlip)->dli_ad == ad &&
			     (*dlip)->dli_dlm->dlm_next == NULL &&
			     member_ad == (*dlip)->dli_dlm->dlm_member_ad ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"\"dynlist-attrpair <member-ad> <URL-ad>\": "
					"URL attributeDescription \"%s\" already mapped.\n",
					ad->ad_cname.bv_val );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
#if 0
				/* make it a warning... */
				return 1;
#endif
			}
		}

		*dlip = (dynlist_info_t *)ch_calloc( 1, sizeof( dynlist_info_t ) );

		(*dlip)->dli_oc = oc;
		(*dlip)->dli_ad = ad;
		(*dlip)->dli_dlm = (dynlist_map_t *)ch_calloc( 1, sizeof( dynlist_map_t ) );
		(*dlip)->dli_dlm->dlm_member_ad = member_ad;
		(*dlip)->dli_dlm->dlm_mapped_ad = NULL;

		rc = dynlist_build_def_filter( *dlip );

		} break;

	default:
		rc = 1;
		break;
	}

	return rc;
}
#endif

static int
dynlist_db_open(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	dynlist_info_t		*dli = (dynlist_info_t *)on->on_bi.bi_private;
	ObjectClass		*oc = NULL;
	AttributeDescription	*ad = NULL;
	const char	*text;
	int rc;

	if ( dli == NULL ) {
		dli = ch_calloc( 1, sizeof( dynlist_info_t ) );
		on->on_bi.bi_private = (void *)dli;
	}

	for ( ; dli; dli = dli->dli_next ) {
		if ( dli->dli_oc == NULL ) {
			if ( oc == NULL ) {
				oc = oc_find( "groupOfURLs" );
				if ( oc == NULL ) {
					snprintf( cr->msg, sizeof( cr->msg),
						"unable to fetch objectClass \"groupOfURLs\"" );
					Debug( LDAP_DEBUG_ANY, "dynlist_db_open: %s.\n", cr->msg, 0, 0 );
					return 1;
				}
			}

			dli->dli_oc = oc;
		}

		if ( dli->dli_ad == NULL ) {
			if ( ad == NULL ) {
				rc = slap_str2ad( "memberURL", &ad, &text );
				if ( rc != LDAP_SUCCESS ) {
					snprintf( cr->msg, sizeof( cr->msg),
						"unable to fetch attributeDescription \"memberURL\": %d (%s)",
						rc, text );
					Debug( LDAP_DEBUG_ANY, "dynlist_db_open: %s.\n", cr->msg, 0, 0 );
					return 1;
				}
			}
		
			dli->dli_ad = ad;			
		}

		if ( BER_BVISNULL( &dli->dli_default_filter ) ) {
			rc = dynlist_build_def_filter( dli );
			if ( rc != 0 ) {
				return rc;
			}
		}
	}

	if ( ad_dgIdentity == NULL ) {
		rc = slap_str2ad( "dgIdentity", &ad_dgIdentity, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( cr->msg, sizeof( cr->msg),
				"unable to fetch attributeDescription \"dgIdentity\": %d (%s)",
				rc, text );
			Debug( LDAP_DEBUG_ANY, "dynlist_db_open: %s\n", cr->msg, 0, 0 );
			/* Just a warning */
		}
	}

	if ( ad_dgAuthz == NULL ) {
		rc = slap_str2ad( "dgAuthz", &ad_dgAuthz, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( cr->msg, sizeof( cr->msg),
				"unable to fetch attributeDescription \"dgAuthz\": %d (%s)",
				rc, text );
			Debug( LDAP_DEBUG_ANY, "dynlist_db_open: %s\n", cr->msg, 0, 0 );
			/* Just a warning */
		}
	}

	return 0;
}

static int
dynlist_db_destroy(
	BackendDB	*be,
	ConfigReply	*cr )
{
	slap_overinst	*on = (slap_overinst *) be->bd_info;

	if ( on->on_bi.bi_private ) {
		dynlist_info_t	*dli = (dynlist_info_t *)on->on_bi.bi_private,
				*dli_next;

		for ( dli_next = dli; dli_next; dli = dli_next ) {
			dynlist_map_t *dlm;
			dynlist_map_t *dlm_next;

			dli_next = dli->dli_next;

			ch_free( dli->dli_default_filter.bv_val );
			dlm = dli->dli_dlm;
			while ( dlm != NULL ) {
				dlm_next = dlm->dlm_next;
				ch_free( dlm );
				dlm = dlm_next;
			}
			ch_free( dli );
		}
	}

	return 0;
}

static slap_overinst	dynlist = { { NULL } };
#ifdef TAKEOVER_DYNGROUP
static char		*obsolete_names[] = {
	"dyngroup",
	NULL
};
#endif

#if SLAPD_OVER_DYNLIST == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_DYNLIST == SLAPD_MOD_DYNAMIC */
int
dynlist_initialize(void)
{
#ifndef OL_2_2_COMPAT
	int	rc = 0;
#endif

	dynlist.on_bi.bi_type = "dynlist";

#ifdef TAKEOVER_DYNGROUP
	/* makes dynlist incompatible with dyngroup */
	dynlist.on_bi.bi_obsolete_names = obsolete_names;
#endif

#ifdef OL_2_2_COMPAT
	dynlist.on_bi.bi_db_config = dynlist_db_config;
#else
	dynlist.on_bi.bi_db_config = config_generic_wrapper;
#endif
	dynlist.on_bi.bi_db_open = dynlist_db_open;
	dynlist.on_bi.bi_db_destroy = dynlist_db_destroy;

	dynlist.on_response = dynlist_response;

#ifndef OL_2_2_COMPAT
	dynlist.on_bi.bi_cf_ocs = dlocs;

	rc = config_register_schema( dlcfg, dlocs );
	if ( rc ) {
		return rc;
	}
#endif

	return overlay_register( &dynlist );
}

#if SLAPD_OVER_DYNLIST == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return dynlist_initialize();
}
#endif

#endif /* SLAPD_OVER_DYNLIST */
