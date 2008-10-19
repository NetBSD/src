/* $OpenLDAP: pkg/ldap/servers/slapd/overlays/constraint.c,v 1.2.2.8 2008/05/27 19:59:47 quanah Exp $ */
/* constraint.c - Overlay to constrain attributes to certain values */
/* 
 * Copyright 2003-2004 Hewlett-Packard Company
 * Copyright 2007 Emmanuel Dreyfus
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
/*
 * Authors: Neil Dunbar <neil.dunbar@hp.com>
 *			Emmannuel Dreyfus <manu@netbsd.org>
 */
#include "portable.h"

#ifdef SLAPD_OVER_CONSTRAINT

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <ac/regex.h>

#include "lutil.h"
#include "slap.h"
#include "config.h"

/*
 * This overlay limits the values which can be placed into an
 * attribute, over and above the limits placed by the schema.
 *
 * It traps only LDAP adds and modify commands (and only seeks to
 * control the add and modify value mods of a modify)
 */

#define REGEX_STR "regex"
#define URI_STR "uri"
#define SIZE_STR "size"
#define COUNT_STR "count"

/*
 * Linked list of attribute constraints which we should enforce.
 * This is probably a sub optimal structure - some form of sorted
 * array would be better if the number of attributes contrained is
 * likely to be much bigger than 4 or 5. We stick with a list for
 * the moment.
 */

typedef struct constraint {
	struct constraint *ap_next;
	AttributeDescription *ap;
	regex_t *re;
	LDAPURLDesc *lud;
	size_t size;
	size_t count;
	AttributeDescription **attrs;
	struct berval val; /* constraint value */
	struct berval dn;
	struct berval filter;
} constraint;

enum {
	CONSTRAINT_ATTRIBUTE = 1
};

static ConfigDriver constraint_cf_gen;

static ConfigTable constraintcfg[] = {
	{ "constraint_attribute", "attribute> (regex|uri) <value",
	  4, 4, 0, ARG_MAGIC | CONSTRAINT_ATTRIBUTE, constraint_cf_gen,
	  "( OLcfgOvAt:13.1 NAME 'olcConstraintAttribute' "
	  "DESC 'regular expression constraint for attribute' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs constraintocs[] = {
	{ "( OLcfgOvOc:13.1 "
	  "NAME 'olcConstraintConfig' "
	  "DESC 'Constraint overlay configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcConstraintAttribute ) )",
	  Cft_Overlay, constraintcfg },
	{ NULL, 0, NULL }
};

static void
constraint_free( constraint *cp )
{
	if (cp->re) {
		regfree(cp->re);
		ch_free(cp->re);
	}
	if (!BER_BVISNULL(&cp->val))
		ch_free(cp->val.bv_val);
	if (cp->lud)
		ldap_free_urldesc(cp->lud);
	if (cp->attrs)
		ch_free(cp->attrs);
	ch_free(cp);
}

static int
constraint_cf_gen( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)(c->bi);
	constraint *cn = on->on_bi.bi_private, *cp;
	struct berval bv;
	int i, rc = 0;
	constraint ap = { NULL, NULL, NULL	}, *a2 = NULL;
	const char *text = NULL;
	
	switch ( c->op ) {
	case SLAP_CONFIG_EMIT:
		switch (c->type) {
		case CONSTRAINT_ATTRIBUTE:
			for (cp=cn; cp; cp=cp->ap_next) {
				int len;
				char *s;
				char *tstr = NULL;

				len = cp->ap->ad_cname.bv_len + 3;
				if (cp->re) {
					len += STRLENOF(REGEX_STR);
					tstr = REGEX_STR;
				} else if (cp->lud) {
					len += STRLENOF(URI_STR);
					tstr = URI_STR;
				} else if (cp->size) {
					len += STRLENOF(SIZE_STR);
					tstr = SIZE_STR;
				} else if (cp->count) {
					len += STRLENOF(COUNT_STR);
					tstr = COUNT_STR;
				}
				len += cp->val.bv_len;

				s = ch_malloc(len);

				bv.bv_len = snprintf(s, len, "%s %s %s", cp->ap->ad_cname.bv_val,
						 tstr, cp->val.bv_val);
				bv.bv_val = s;
				rc = value_add_one( &c->rvalue_vals, &bv );
				if (rc) return rc;
				rc = value_add_one( &c->rvalue_nvals, &bv );
				if (rc) return rc;
				ch_free(s);
			}
			break;
		default:
			abort();
			break;
		}
		break;
	case LDAP_MOD_DELETE:
		switch (c->type) {
		case CONSTRAINT_ATTRIBUTE:
			if (!cn) break; /* nothing to do */
					
			if (c->valx < 0) {
				/* zap all constraints */
				while (cn) {
					cp = cn->ap_next;
					constraint_free( cn );
					cn = cp;
				}
						
				on->on_bi.bi_private = NULL;
			} else {
				constraint **cpp;
						
				/* zap constraint numbered 'valx' */
				for(i=0, cp = cn, cpp = &cn;
					(cp) && (i<c->valx);
					i++, cpp = &cp->ap_next, cp = *cpp);

				if (cp) {
					/* zap cp, and join cpp to cp->ap_next */
					*cpp = cp->ap_next;
					constraint_free( cp );
				}
				on->on_bi.bi_private = cn;
			}
			break;

		default:
			abort();
			break;
		}
		break;
	case SLAP_CONFIG_ADD:
	case LDAP_MOD_ADD:
		switch (c->type) {
		case CONSTRAINT_ATTRIBUTE:
			if ( slap_str2ad( c->argv[1], &ap.ap, &text ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"%s <%s>: %s\n", c->argv[0], c->argv[1], text );
				Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
					   "%s: %s\n", c->log, c->cr_msg, 0 );
				return( ARG_BAD_CONF );
			}

			if ( strcasecmp( c->argv[2], REGEX_STR ) == 0) {
				int err;
			
				ap.re = ch_malloc( sizeof(regex_t) );
				if ((err = regcomp( ap.re,
					c->argv[3], REG_EXTENDED )) != 0) {
					char errmsg[1024];
							
					regerror( err, ap.re, errmsg, sizeof(errmsg) );
					ch_free(ap.re);
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
					   "%s %s: Illegal regular expression \"%s\": Error %s",
					   c->argv[0], c->argv[1], c->argv[3], errmsg);
					Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
						"%s: %s\n", c->log, c->cr_msg, 0 );
					ap.re = NULL;
					return( ARG_BAD_CONF );
				}
				ber_str2bv( c->argv[3], 0, 1, &ap.val );
			} else if ( strcasecmp( c->argv[2], SIZE_STR ) == 0 ) {
				size_t size;

				if ( ( size = atoi(c->argv[3]) ) != 0 )
					ap.size = size;	
			} else if ( strcasecmp( c->argv[2], COUNT_STR ) == 0 ) {
				size_t count;

				if ( ( count = atoi(c->argv[3]) ) != 0 )
					ap.count = count;	
			} else if ( strcasecmp( c->argv[2], URI_STR ) == 0 ) {
				int err;
			
				err = ldap_url_parse(c->argv[3], &ap.lud);
				if ( err != LDAP_URL_SUCCESS ) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s %s: Invalid URI \"%s\"",
						c->argv[0], c->argv[1], c->argv[3]);
					Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
						"%s: %s\n", c->log, c->cr_msg, 0 );
					return( ARG_BAD_CONF );
				}

				if (ap.lud->lud_host != NULL) {
					snprintf( c->cr_msg, sizeof( c->cr_msg ),
						"%s %s: unsupported hostname in URI \"%s\"",
						c->argv[0], c->argv[1], c->argv[3]);
					Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
						"%s: %s\n", c->log, c->cr_msg, 0 );

					ldap_free_urldesc(ap.lud);

					return( ARG_BAD_CONF );
				}

				for ( i=0; ap.lud->lud_attrs[i]; i++);
				/* FIXME: This is worthless without at least one attr */
				if ( i ) {
					ap.attrs = ch_malloc( (i+1)*sizeof(AttributeDescription *));
					for ( i=0; ap.lud->lud_attrs[i]; i++) {
						ap.attrs[i] = NULL;
						if ( slap_str2ad( ap.lud->lud_attrs[i], &ap.attrs[i], &text ) ) {
							ch_free( ap.attrs );
							snprintf( c->cr_msg, sizeof( c->cr_msg ),
								"%s <%s>: %s\n", c->argv[0], ap.lud->lud_attrs[i], text );
							Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
								   "%s: %s\n", c->log, c->cr_msg, 0 );
							return( ARG_BAD_CONF );
						}
					}
					ap.attrs[i] = NULL;
				}

				if (ap.lud->lud_dn == NULL)
					ap.lud->lud_dn = ch_strdup("");

				if (ap.lud->lud_filter == NULL)
					ap.lud->lud_filter = ch_strdup("objectClass=*");

				ber_str2bv( c->argv[3], 0, 1, &ap.val );
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
				   "%s %s: Unknown constraint type: %s",
				   c->argv[0], c->argv[1], c->argv[2] );
				Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				   "%s: %s\n", c->log, c->cr_msg, 0 );
				return ( ARG_BAD_CONF );
			}

			a2 = ch_calloc( sizeof(constraint), 1 );
			a2->ap_next = on->on_bi.bi_private;
			a2->ap = ap.ap;
			a2->re = ap.re;
			a2->val = ap.val;
			a2->lud = ap.lud;
			a2->size = ap.size;
			a2->count = ap.count;
			if ( a2->lud ) {
				ber_str2bv(a2->lud->lud_dn, 0, 0, &a2->dn);
				ber_str2bv(a2->lud->lud_filter, 0, 0, &a2->filter);
			}
			a2->attrs = ap.attrs;
			on->on_bi.bi_private = a2;
			break;
		default:
			abort();
			break;
		}
		break;
	default:
		abort();
	}

	return rc;
}

static int
constraint_uri_cb( Operation *op, SlapReply *rs ) 
{
	if(rs->sr_type == REP_SEARCH) {
		int *foundp = op->o_callback->sc_private;

		*foundp = 1;

		Debug(LDAP_DEBUG_TRACE, "==> constraint_uri_cb <%s>\n",
			rs->sr_entry ? rs->sr_entry->e_name.bv_val : "UNKNOWN_DN", 0, 0);
	}
	return 0;
}

static int
constraint_violation( constraint *c, struct berval *bv, Operation *op, SlapReply *rs)
{
	if ((!c) || (!bv)) return 0;
	
	if ((c->re) &&
		(regexec(c->re, bv->bv_val, 0, NULL, 0) == REG_NOMATCH))
		return 1; /* regular expression violation */

	if ((c->size) && (bv->bv_len > c->size))
		return 1; /* size violation */

	if (c->lud) {
		Operation nop = *op;
		slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
		slap_callback cb;
		SlapReply nrs = { REP_RESULT };
		int i;
		int found;
		int rc;
		size_t len;
		struct berval filterstr;
		char *ptr;

		found = 0;

		nrs.sr_entry = NULL;
		nrs.sr_nentries = 0;

		cb.sc_next = NULL;
		cb.sc_response = constraint_uri_cb;
		cb.sc_cleanup = NULL;
		cb.sc_private = &found;

		nop.o_protocol = LDAP_VERSION3;
		nop.o_tag = LDAP_REQ_SEARCH;
		nop.o_time = slap_get_time();
		if (c->lud->lud_dn) {
			struct berval dn;

			ber_str2bv(c->lud->lud_dn, 0, 0, &dn);
			nop.o_req_dn = dn;
			nop.o_req_ndn = dn;
			nop.o_bd = select_backend(&nop.o_req_ndn, 1 );
			if (!nop.o_bd || !nop.o_bd->be_search) {
				return 1; /* unexpected error */
			}
		} else {
			nop.o_req_dn = nop.o_bd->be_nsuffix[0];
			nop.o_req_ndn = nop.o_bd->be_nsuffix[0];
			nop.o_bd = on->on_info->oi_origdb;
		}
		nop.o_do_not_cache = 1;
		nop.o_callback = &cb;

		nop.ors_scope = c->lud->lud_scope;
		nop.ors_deref = LDAP_DEREF_NEVER;
		nop.ors_slimit = SLAP_NO_LIMIT;
		nop.ors_tlimit = SLAP_NO_LIMIT;
		nop.ors_limit = NULL;

		nop.ors_attrsonly = 0;
		nop.ors_attrs = slap_anlist_no_attrs;

		len = STRLENOF("(&(") + 
			  c->filter.bv_len +
			  STRLENOF(")(|");

		for (i = 0; c->attrs[i]; i++) {
			len += STRLENOF("(") +
				   c->attrs[i]->ad_cname.bv_len +
				   STRLENOF("=") + 
				   bv->bv_len +
				   STRLENOF(")");
		}

		len += STRLENOF("))");
		filterstr.bv_len = len;
		filterstr.bv_val = op->o_tmpalloc(len + 1, op->o_tmpmemctx);

		ptr = filterstr.bv_val +
			snprintf(filterstr.bv_val, len, "(&(%s)(|", c->lud->lud_filter);
		for (i = 0; c->attrs[i]; i++) {
			*ptr++ = '(';
			ptr = lutil_strcopy( ptr, c->attrs[i]->ad_cname.bv_val );
			*ptr++ = '=';
			ptr = lutil_strcopy( ptr, bv->bv_val );
			*ptr++ = ')';
		}
		*ptr++ = ')';
		*ptr++ = ')';

		Debug(LDAP_DEBUG_TRACE, 
			"==> constraint_violation uri filter = %s\n",
			filterstr.bv_val, 0, 0);

		nop.ors_filterstr = filterstr;
		nop.ors_filter = str2filter_x(&nop, filterstr.bv_val);

		rc = nop.o_bd->be_search( &nop, &nrs );
		
		op->o_tmpfree(filterstr.bv_val, op->o_tmpmemctx);
		Debug(LDAP_DEBUG_TRACE, 
			"==> constraint_violation uri rc = %d, found = %d\n",
			rc, found, 0);

		if((rc != LDAP_SUCCESS) && (rc != LDAP_NO_SUCH_OBJECT)) {
			send_ldap_error(op, rs, rc, 
				"constraint_violation uri search failed");
			return 1; /* unexpected error */
		}

		if (!found)
			return 1; /* constraint violation */
			
	}
	
	return 0;
}

static char *
print_message( struct berval *errtext, AttributeDescription *a )
{
	char *ret;
	int sz;
	
	sz = errtext->bv_len + sizeof(" on ") + a->ad_cname.bv_len;
	ret = ch_malloc(sz);
	snprintf( ret, sz, "%s on %s", errtext->bv_val, a->ad_cname.bv_val );
	return ret;
}

static unsigned
constraint_count_attr(Entry *e, AttributeDescription *ad)
{
	struct Attribute *a;

	if ((a = attr_find(e->e_attrs, ad)) != NULL)
		return a->a_numvals;
	return 0;
}

static int
constraint_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Backend *be = op->o_bd;
	Attribute *a;
	constraint *c = on->on_bi.bi_private, *cp;
	BerVarray b = NULL;
	int i;
	struct berval rsv = BER_BVC("add breaks constraint");
	char *msg;

	if ((a = op->ora_e->e_attrs) == NULL) {
		op->o_bd->bd_info = (BackendInfo *)(on->on_info);
		send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
			"constraint_add() got null op.ora_e.e_attrs");
		return(rs->sr_err);
	}

	for(; a; a = a->a_next ) {
		/* we don't constrain operational attributes */
		if (is_at_operational(a->a_desc->ad_type)) continue;

		for(cp = c; cp; cp = cp->ap_next) {
			if (cp->ap != a->a_desc) continue;
			if ((b = a->a_vals) == NULL) continue;
				
			Debug(LDAP_DEBUG_TRACE, 
				"==> constraint_add, "
				"a->a_numvals = %d, cp->count = %d\n",
				a->a_numvals, cp->count, 0);

			if ((cp->count != 0) && (a->a_numvals > cp->count))
				goto add_violation;

			for(i=0; b[i].bv_val; i++) 
				if (constraint_violation( cp, &b[i], op, rs))
					goto add_violation;
		}
	}
	/* Default is to just fall through to the normal processing */
	return SLAP_CB_CONTINUE;

add_violation:
	op->o_bd->bd_info = (BackendInfo *)(on->on_info);
	msg = print_message( &rsv, a->a_desc );
	send_ldap_error(op, rs, LDAP_CONSTRAINT_VIOLATION, msg );
	ch_free(msg);
	return (rs->sr_err);
}


static int
constraint_modify( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Backend *be = op->o_bd;
	constraint *c = on->on_bi.bi_private, *cp;
	Entry *target_entry = NULL;
	Modifications *m;
	BerVarray b = NULL;
	int i;
	struct berval rsv = BER_BVC("modify breaks constraint");
	char *msg;
	
	Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "constraint_modify()", 0,0,0);
	if ((m = op->orm_modlist) == NULL) {
		op->o_bd->bd_info = (BackendInfo *)(on->on_info);
		send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
						"constraint_modify() got null orm_modlist");
		return(rs->sr_err);
	}

	/* Do we need to count attributes? */
	for(cp = c; cp; cp = cp->ap_next) {
		if (cp->count != 0) {
			int rc;

			op->o_bd = on->on_info->oi_origdb;
			rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &target_entry );
			op->o_bd = be;

			if (rc != 0 || target_entry == NULL) {
				Debug(LDAP_DEBUG_TRACE, 
					"==> constraint_modify rc = %d\n",
					rc, 0, 0);
				goto mod_violation;
			}
			break;
		}
	}
		
	for(;m; m = m->sml_next) {
		int ce = 0;

		/* Get this attribute count, if needed */
		if (target_entry)
			ce = constraint_count_attr(target_entry, m->sml_desc);

		if (is_at_operational( m->sml_desc->ad_type )) continue;
		if ((( m->sml_op & LDAP_MOD_OP ) != LDAP_MOD_ADD) &&
			(( m->sml_op & LDAP_MOD_OP ) != LDAP_MOD_REPLACE) &&
			(( m->sml_op & LDAP_MOD_OP ) != LDAP_MOD_DELETE))
			continue;
		/* we only care about ADD and REPLACE modifications */
		/* and DELETE are used to track attribute count */
		if ((( b = m->sml_values ) == NULL ) || (b[0].bv_val == NULL))
			continue;

		for(cp = c; cp; cp = cp->ap_next) {
			if (cp->ap != m->sml_desc) continue;
			
			if (cp->count != 0) {
				int ca;

				if (m->sml_op == LDAP_MOD_DELETE)
					ce = 0;

				for (ca = 0; b[ca].bv_val; ++ca);

				Debug(LDAP_DEBUG_TRACE, 
					"==> constraint_modify ce = %d, "
					"ca = %d, cp->count = %d\n",
					ce, ca, cp->count);

				if (m->sml_op == LDAP_MOD_ADD)
					if (ca + ce > cp->count)
						goto mod_violation;
				if (m->sml_op == LDAP_MOD_REPLACE) {
					if (ca > cp->count)
						goto mod_violation;
					ce = ca;
				}
			} 

			/* DELETE are to be ignored beyond this point */
			if (( m->sml_op & LDAP_MOD_OP ) == LDAP_MOD_DELETE)
				continue;

			for(i=0; b[i].bv_val; i++)
				if (constraint_violation( cp, &b[i], op, rs))
					goto mod_violation;
		}
	}
	
	if (target_entry) {
		op->o_bd = on->on_info->oi_origdb;
		be_entry_release_r(op, target_entry);
		op->o_bd = be;
	}
	return SLAP_CB_CONTINUE;
mod_violation:
	/* violation */
	if (target_entry) {
		op->o_bd = on->on_info->oi_origdb;
		be_entry_release_r(op, target_entry);
		op->o_bd = be;
	}
	op->o_bd->bd_info = (BackendInfo *)(on->on_info);
	msg = print_message( &rsv, m->sml_desc );
	send_ldap_error(op, rs, LDAP_CONSTRAINT_VIOLATION, msg );
	ch_free(msg);
	return (rs->sr_err);
}

static int
constraint_close(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	constraint *ap, *a2;

	for ( ap = on->on_bi.bi_private; ap; ap = a2 ) {
		a2 = ap->ap_next;
		constraint_free( ap );
	}

	return 0;
}

static slap_overinst constraint_ovl;

#if SLAPD_OVER_CONSTRAINT == SLAPD_MOD_DYNAMIC
static
#endif
int
constraint_initialize( void ) {
	int rc;

	constraint_ovl.on_bi.bi_type = "constraint";
	constraint_ovl.on_bi.bi_db_close = constraint_close;
	constraint_ovl.on_bi.bi_op_add = constraint_add;
	constraint_ovl.on_bi.bi_op_modify = constraint_modify;

	constraint_ovl.on_bi.bi_private = NULL;
	
	constraint_ovl.on_bi.bi_cf_ocs = constraintocs;
	rc = config_register_schema( constraintcfg, constraintocs );
	if (rc) return rc;
	
	return overlay_register( &constraint_ovl );
}

#if SLAPD_OVER_CONSTRAINT == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return constraint_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_CONSTRAINT) */

