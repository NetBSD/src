/*	$NetBSD: remoteauth.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

/* $OpenLDAP$ */
/* remoteauth.c - Overlay to delegate bind processing to a remote server */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2021 The OpenLDAP Foundation.
 * Portions Copyright 2017-2021 Ondřej Kuzník, Symas Corporation.
 * Portions Copyright 2004-2017 Howard Chu, Symas Corporation.
 * Portions Copyright 2004 Hewlett-Packard Company.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: remoteauth.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#include <ldap.h>
#if SLAPD_MODULES
#define LIBLTDL_DLL_IMPORT /* Win32: don't re-export libltdl's symbols */
#include <ltdl.h>
#endif
#include <ac/errno.h>
#include <ac/time.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include "lutil.h"
#include "slap.h"
#include "slap-config.h"

#ifndef UP_STR
#define UP_STR "userPassword"
#endif /* UP_STR */

#ifndef LDAP_PREFIX
#define LDAP_PREFIX "ldap://"
#endif /* LDAP_PREFIX */

#ifndef FILE_PREFIX
#define FILE_PREFIX "file://"
#endif /* LDAP_PREFIX */

typedef struct _ad_info {
	struct _ad_info *next;
	char *domain;
	char *realm;
} ad_info;

typedef struct _ad_pin {
	struct _ad_pin *next;
	char *hostname;
	char *pin;
} ad_pin;

typedef struct _ad_private {
	char *dn;
	AttributeDescription *dn_ad;
	char *domain_attr;
	AttributeDescription *domain_ad;

	AttributeDescription *up_ad;
	ad_info *mappings;

	char *default_realm;
	char *default_domain;

	int up_set;
	int retry_count;
	int store_on_success;

	ad_pin *pins;
	slap_bindconf ad_tls;
} ad_private;

enum {
	REMOTE_AUTH_MAPPING = 1,
	REMOTE_AUTH_DN_ATTRIBUTE,
	REMOTE_AUTH_DOMAIN_ATTRIBUTE,
	REMOTE_AUTH_DEFAULT_DOMAIN,
	REMOTE_AUTH_DEFAULT_REALM,
	REMOTE_AUTH_CACERT_DIR,
	REMOTE_AUTH_CACERT_FILE,
	REMOTE_AUTH_VALIDATE_CERTS,
	REMOTE_AUTH_RETRY_COUNT,
	REMOTE_AUTH_TLS,
	REMOTE_AUTH_TLS_PIN,
	REMOTE_AUTH_STORE_ON_SUCCESS,
};

static ConfigDriver remoteauth_cf_gen;

static ConfigTable remoteauthcfg[] = {
	{ "remoteauth_mapping", "mapping between domain and realm", 2, 3, 0,
		ARG_MAGIC|REMOTE_AUTH_MAPPING,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.1 NAME 'olcRemoteAuthMapping' "
			"DESC 'Mapping from domain name to server' "
			"SYNTAX OMsDirectoryString )",
		NULL, NULL
	},
	{ "remoteauth_dn_attribute", "Attribute to use as AD bind DN", 2, 2, 0,
		ARG_MAGIC|REMOTE_AUTH_DN_ATTRIBUTE,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.2 NAME 'olcRemoteAuthDNAttribute' "
			"DESC 'Attribute in entry to use as bind DN for AD' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL
	},
	{ "remoteauth_domain_attribute", "Attribute to use as domain determinant", 2, 2, 0,
		ARG_MAGIC|REMOTE_AUTH_DOMAIN_ATTRIBUTE,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.3 NAME 'olcRemoteAuthDomainAttribute' "
			"DESC 'Attribute in entry to determine windows domain' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL
	},
	{ "remoteauth_default_domain", "Default Windows domain", 2, 2, 0,
		ARG_MAGIC|REMOTE_AUTH_DEFAULT_DOMAIN,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.4 NAME 'olcRemoteAuthDefaultDomain' "
			"DESC 'Default Windows domain to use' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL
	},
	{ "remoteauth_default_realm", "Default AD realm", 2, 2, 0,
		ARG_MAGIC|REMOTE_AUTH_DEFAULT_REALM,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.5 NAME 'olcRemoteAuthDefaultRealm' "
			"DESC 'Default AD realm to use' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL
	},
	{ "remoteauth_store", "on|off", 1, 2, 0,
		ARG_OFFSET|ARG_ON_OFF|REMOTE_AUTH_STORE_ON_SUCCESS,
		(void *)offsetof(ad_private, store_on_success),
		"( OLcfgOvAt:24.6 NAME 'olcRemoteAuthStore' "
			"DESC 'Store password locally on success' "
			"SYNTAX OMsBoolean SINGLE-VALUE )",
		NULL, NULL
	},
	{ "remoteauth_retry_count", "integer", 2, 2, 0,
		ARG_OFFSET|ARG_UINT|REMOTE_AUTH_RETRY_COUNT,
		(void *)offsetof(ad_private, retry_count),
		"( OLcfgOvAt:24.7 NAME 'olcRemoteAuthRetryCount' "
			"DESC 'Number of retries attempted' "
			"SYNTAX OMsInteger SINGLE-VALUE )",
		NULL, { .v_uint = 3 }
	},
	{ "remoteauth_tls", "tls settings", 2, 0, 0,
		ARG_MAGIC|REMOTE_AUTH_TLS,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.8 NAME 'olcRemoteAuthTLS' "
			"DESC 'StartTLS settings' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )",
		NULL, NULL
	},
	{ "remoteauth_tls_peerkey_hash", "mapping between hostnames and their public key hash", 3, 3, 0,
		ARG_MAGIC|REMOTE_AUTH_TLS_PIN,
		remoteauth_cf_gen,
		"( OLcfgOvAt:24.9 NAME 'olcRemoteAuthTLSPeerkeyHash' "
			"DESC 'StartTLS hostname to public key pin mapping file' "
			"SYNTAX OMsDirectoryString )",
		NULL, NULL
	},

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED, NULL }
};

static ConfigOCs remoteauthocs[] = {
	{ "( OLcfgOvOc:24.1 "
		"NAME 'olcRemoteAuthCfg' "
		"DESC 'Remote Directory passthough authentication configuration' "
		"SUP olcOverlayConfig "
		"MUST olcRemoteAuthTLS "
		"MAY ( olcRemoteAuthMapping $ olcRemoteAuthDNAttribute $ "
		"      olcRemoteAuthDomainAttribute $ olcRemoteAuthDefaultDomain $ "
		"      olcRemoteAuthDefaultRealm $ olcRemoteAuthStore $ "
		"      olcRemoteAuthRetryCount $ olcRemoteAuthTLSPeerkeyHash ) )",
		Cft_Overlay, remoteauthcfg },
	{ NULL, 0, NULL }
};

static int
remoteauth_cf_gen( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	ad_private *ad = (ad_private *)on->on_bi.bi_private;
	struct berval bv;
	int i, rc = 0;
	ad_info *map;
	const char *text = NULL;

	switch ( c->op ) {
		case SLAP_CONFIG_EMIT:
			switch ( c->type ) {
				case REMOTE_AUTH_MAPPING:
					for ( map = ad->mappings; map; map = map->next ) {
						char *str;

						str = ch_malloc( strlen( map->domain ) +
								strlen( map->realm ) + 2 );
						sprintf( str, "%s %s", map->domain, map->realm );
						ber_str2bv( str, strlen( str ), 1, &bv );
						ch_free( str );
						rc = value_add_one( &c->rvalue_vals, &bv );
						if ( rc ) return rc;
						rc = value_add_one( &c->rvalue_nvals, &bv );
						if ( rc ) return rc;
					}
					break;
				case REMOTE_AUTH_DN_ATTRIBUTE:
					if ( ad->dn )
						value_add_one( &c->rvalue_vals, &ad->dn_ad->ad_cname );
					break;
				case REMOTE_AUTH_DOMAIN_ATTRIBUTE:
					if ( ad->domain_attr )
						value_add_one(
								&c->rvalue_vals, &ad->domain_ad->ad_cname );
					break;
				case REMOTE_AUTH_DEFAULT_DOMAIN:
					if ( ad->default_domain ) {
						ber_str2bv( ad->default_domain, 0, 1, &bv );
						value_add_one( &c->rvalue_vals, &bv );
					}
					break;
				case REMOTE_AUTH_DEFAULT_REALM:
					if ( ad->default_realm ) {
						ber_str2bv( ad->default_realm, 0, 1, &bv );
						value_add_one( &c->rvalue_vals, &bv );
					}
					break;
				case REMOTE_AUTH_TLS:
					bindconf_tls_unparse( &ad->ad_tls, &bv );

					for ( i = 0; isspace( (unsigned char) bv.bv_val[ i ] ); i++ )
						/* count spaces */ ;

					if ( i ) {
						bv.bv_len -= i;
						AC_MEMCPY( bv.bv_val, &bv.bv_val[ i ],
							bv.bv_len + 1 );
					}

					value_add_one( &c->rvalue_vals, &bv );
					break;
				case REMOTE_AUTH_TLS_PIN: {
					ad_pin *pin = ad->pins;
					for ( pin = ad->pins; pin; pin = pin->next ) {
						bv.bv_val = ch_malloc( strlen( pin->hostname ) +
								strlen( pin->pin ) + 2 );
						bv.bv_len = sprintf(
								bv.bv_val, "%s %s", pin->hostname, pin->pin );
						rc = value_add_one( &c->rvalue_vals, &bv );
						if ( rc ) return rc;
						rc = value_add_one( &c->rvalue_nvals, &bv );
						if ( rc ) return rc;
					}
				} break;

				default:
					abort();
			}
			break;
		case LDAP_MOD_DELETE:
			switch ( c->type ) {
				case REMOTE_AUTH_MAPPING:
					if ( c->valx < 0 ) {
						/* delete all mappings */
						while ( ad->mappings ) {
							map = ad->mappings;
							ad->mappings = ad->mappings->next;
							ch_free( map->domain );
							ch_free( map->realm );
							ch_free( map );
						}
					} else {
						/* delete a specific mapping indicated by 'valx'*/
						ad_info *pmap = NULL;

						for ( map = ad->mappings, i = 0;
								( map ) && ( i < c->valx );
								pmap = map, map = map->next, i++ )
							;

						if ( pmap ) {
							pmap->next = map->next;
							map->next = NULL;

							ch_free( map->domain );
							ch_free( map->realm );
							ch_free( map );
						} else if ( ad->mappings ) {
							/* delete the first item in the list */
							map = ad->mappings;
							ad->mappings = map->next;
							ch_free( map->domain );
							ch_free( map->realm );
							ch_free( map );
						}
					}
					break;
				case REMOTE_AUTH_DN_ATTRIBUTE:
					if ( ad->dn ) {
						ch_free( ad->dn );
						ad->dn = NULL; /* Don't free AttributeDescription */
					}
					break;
				case REMOTE_AUTH_DOMAIN_ATTRIBUTE:
					if ( ad->domain_attr ) {
						ch_free( ad->domain_attr );
						/* Don't free AttributeDescription */
						ad->domain_attr = NULL;
					}
					break;
				case REMOTE_AUTH_DEFAULT_DOMAIN:
					if ( ad->default_domain ) {
						ch_free( ad->default_domain );
						ad->default_domain = NULL;
					}
					break;
				case REMOTE_AUTH_DEFAULT_REALM:
					if ( ad->default_realm ) {
						ch_free( ad->default_realm );
						ad->default_realm = NULL;
					}
					break;
				case REMOTE_AUTH_TLS:
					/* MUST + SINGLE-VALUE -> this is a replace */
					bindconf_free( &ad->ad_tls );
					break;
				case REMOTE_AUTH_TLS_PIN:
					while ( ad->pins ) {
						ad_pin *pin = ad->pins;
						ad->pins = ad->pins->next;
						ch_free( pin->hostname );
						ch_free( pin->pin );
						ch_free( pin );
					}
					break;
				/* ARG_OFFSET */
				case REMOTE_AUTH_STORE_ON_SUCCESS:
				case REMOTE_AUTH_RETRY_COUNT:
					abort();
					break;
				default:
					abort();
			}
			break;
		case SLAP_CONFIG_ADD:
		case LDAP_MOD_ADD:
			switch ( c->type ) {
				case REMOTE_AUTH_MAPPING:
					/* add mapping to head of list */
					map = ch_malloc( sizeof(ad_info) );
					map->domain = ber_strdup( c->argv[1] );
					map->realm = ber_strdup( c->argv[2] );
					map->next = ad->mappings;
					ad->mappings = map;

					break;
				case REMOTE_AUTH_DN_ATTRIBUTE:
					if ( slap_str2ad( c->argv[1], &ad->dn_ad, &text ) ==
							LDAP_SUCCESS ) {
						ad->dn = ber_strdup( ad->dn_ad->ad_cname.bv_val );
					} else {
						strncpy( c->cr_msg, text, sizeof(c->cr_msg) );
						c->cr_msg[sizeof(c->cr_msg) - 1] = '\0';
						Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg );
						rc = ARG_BAD_CONF;
					}
					break;
				case REMOTE_AUTH_DOMAIN_ATTRIBUTE:
					if ( slap_str2ad( c->argv[1], &ad->domain_ad, &text ) ==
							LDAP_SUCCESS ) {
						ad->domain_attr =
								ber_strdup( ad->domain_ad->ad_cname.bv_val );
					} else {
						strncpy( c->cr_msg, text, sizeof(c->cr_msg) );
						c->cr_msg[sizeof(c->cr_msg) - 1] = '\0';
						Debug( LDAP_DEBUG_ANY, "%s: %s.\n", c->log, c->cr_msg );
						rc = ARG_BAD_CONF;
					}
					break;
				case REMOTE_AUTH_DEFAULT_DOMAIN:
					if ( ad->default_domain ) {
						ch_free( ad->default_domain );
						ad->default_domain = NULL;
					}
					ad->default_domain = ber_strdup( c->argv[1] );
					break;
				case REMOTE_AUTH_DEFAULT_REALM:
					if ( ad->default_realm ) {
						ch_free( ad->default_realm );
						ad->default_realm = NULL;
					}
					ad->default_realm = ber_strdup( c->argv[1] );
					break;
				case REMOTE_AUTH_TLS:
					for ( i=1; i < c->argc; i++ ) {
						if ( bindconf_tls_parse( c->argv[i], &ad->ad_tls ) ) {
							rc = 1;
							break;
						}
					}
					bindconf_tls_defaults( &ad->ad_tls );
					break;
				case REMOTE_AUTH_TLS_PIN: {
					ad_pin *pin = ch_calloc( 1, sizeof(ad_pin) );

					pin->hostname = ber_strdup( c->argv[1] );
					pin->pin = ber_strdup( c->argv[2] );
					pin->next = ad->pins;
					ad->pins = pin;
				} break;
				/* ARG_OFFSET */
				case REMOTE_AUTH_STORE_ON_SUCCESS:
				case REMOTE_AUTH_RETRY_COUNT:
					abort();
					break;
				default:
					abort();
			}
			break;
		default:
			abort();
	}

	return rc;
}

static char *
get_realm(
		const char *domain,
		ad_info *mappings,
		const char *default_realm,
		int *isfile )
{
	ad_info *ai;
	char *dom = NULL, *ch, *ret = NULL;

	if ( isfile ) *isfile = 0;

	if ( !domain ) {
		ret = default_realm ? ch_strdup( default_realm ) : NULL;
		goto exit;
	}

	/* munge any DOMAIN\user or DOMAIN:user values into just DOMAIN */

	ch = strchr( domain, '\\' );
	if ( !ch ) ch = strchr( domain, ':' );

	if ( ch ) {
		dom = ch_malloc( ch - domain + 1 );
		strncpy( dom, domain, ch - domain );
		dom[ch - domain] = '\0';
	} else {
		dom = ch_strdup( domain );
	}

	for ( ai = mappings; ai; ai = ai->next )
		if ( strcasecmp( ai->domain, dom ) == 0 ) {
			ret = ch_strdup( ai->realm );
			break;
		}

	if ( !ai )
		ret = default_realm ? ch_strdup( default_realm ) :
							  NULL; /* no mapping found */
exit:
	if ( dom ) ch_free( dom );
	if ( ret &&
			( strncasecmp( ret, FILE_PREFIX, strlen( FILE_PREFIX ) ) == 0 ) ) {
		char *p;

		p = ret;
		ret = ch_strdup( p + strlen( FILE_PREFIX ) );
		ch_free( p );
		if ( isfile ) *isfile = 1;
	}

	return ret;
}

static char *
get_ldap_url( const char *realm, int isfile )
{
	char *ldap_url = NULL;
	FILE *fp;

	if ( !realm ) return NULL;

	if ( !isfile ) {
		if ( strstr( realm, "://" ) ) {
			return ch_strdup( realm );
		}

		ldap_url = ch_malloc( 1 + strlen( LDAP_PREFIX ) + strlen( realm ) );
		sprintf( ldap_url, "%s%s", LDAP_PREFIX, realm );
		return ldap_url;
	}

	fp = fopen( realm, "r" );
	if ( !fp ) {
		char ebuf[128];
		int saved_errno = errno;
		Debug( LDAP_DEBUG_TRACE, "remoteauth: "
				"Unable to open realm file (%s)\n",
				sock_errstr( saved_errno, ebuf, sizeof(ebuf) ) );
		return NULL;
	}
	/*
		 * Read each line in the file and return a URL of the form
		 * "ldap://<line1> ldap://<line2> ... ldap://<lineN>"
		 * which can be passed to ldap_initialize.
		 */
	while ( !feof( fp ) ) {
		char line[512], *p;

		p = fgets( line, sizeof(line), fp );
		if ( !p ) continue;

		/* terminate line at first whitespace */
		for ( p = line; *p; p++ )
			if ( isspace( *p ) ) {
				*p = '\0';
				break;
			}

		if ( ldap_url ) {
			char *nu;

			nu = ch_malloc( strlen( ldap_url ) + 2 + strlen( LDAP_PREFIX ) +
					strlen( line ) );

			if ( strstr( line, "://" ) ) {
				sprintf( nu, "%s %s", ldap_url, line );
			} else {
				sprintf( nu, "%s %s%s", ldap_url, LDAP_PREFIX, line );
			}
			ch_free( ldap_url );
			ldap_url = nu;
		} else {
			ldap_url = ch_malloc( 1 + strlen( line ) + strlen( LDAP_PREFIX ) );
			if ( strstr( line, "://" ) ) {
				strcpy( ldap_url, line );
			} else {
				sprintf( ldap_url, "%s%s", LDAP_PREFIX, line );
			}
		}
	}

	fclose( fp );

	return ldap_url;
}

static void
trace_remoteauth_parameters( ad_private *ap )
{
	ad_info *pad_info;
	struct berval bv;

	if ( !ap ) return;

	Debug( LDAP_DEBUG_TRACE, "remoteauth_dn_attribute: %s\n",
			ap->dn ? ap->dn : "NULL" );

	Debug( LDAP_DEBUG_TRACE, "remoteauth_domain_attribute: %s\n",
			ap->domain_attr ? ap->domain_attr : "NULL" );

	Debug( LDAP_DEBUG_TRACE, "remoteauth_default_realm: %s\n",
			ap->default_realm ? ap->default_realm : "NULL" );

	Debug( LDAP_DEBUG_TRACE, "remoteauth_default_domain: %s\n",
			ap->default_domain ? ap->default_domain : "NULL" );

	Debug( LDAP_DEBUG_TRACE, "remoteauth_retry_count: %d\n", ap->retry_count );

	bindconf_tls_unparse( &ap->ad_tls, &bv );
	Debug( LDAP_DEBUG_TRACE, "remoteauth_tls:%s\n", bv.bv_val );
	ch_free( bv.bv_val );

	pad_info = ap->mappings;
	while ( pad_info ) {
		Debug( LDAP_DEBUG_TRACE, "remoteauth_mappings(%s,%s)\n",
				pad_info->domain ? pad_info->domain : "NULL",
				pad_info->realm ? pad_info->realm : "NULL" );
		pad_info = pad_info->next;
	}

	return;
}

static int
remoteauth_conn_cb(
		LDAP *ld,
		Sockbuf *sb,
		LDAPURLDesc *srv,
		struct sockaddr *addr,
		struct ldap_conncb *ctx )
{
	ad_private *ap = ctx->lc_arg;
	ad_pin *pin = NULL;
	char *host;

	host = srv->lud_host;
	if ( !host || !*host ) {
		host = "localhost";
	}

	for ( pin = ap->pins; pin; pin = pin->next ) {
		if ( !strcasecmp( host, pin->hostname ) ) break;
	}

	if ( pin ) {
		int rc = ldap_set_option( ld, LDAP_OPT_X_TLS_PEERKEY_HASH, pin->pin );
		if ( rc == LDAP_SUCCESS ) {
			return 0;
		}

		Debug( LDAP_DEBUG_TRACE, "remoteauth_conn_cb: "
				"TLS Peerkey hash could not be set to '%s': %d\n",
				pin->pin, rc );
	} else {
		Debug( LDAP_DEBUG_TRACE, "remoteauth_conn_cb: "
				"No TLS Peerkey hash found for host '%s'\n",
				host );
	}

	return -1;
}

static void
remoteauth_conn_delcb( LDAP *ld, Sockbuf *sb, struct ldap_conncb *ctx )
{
	return;
}

static int
remoteauth_bind( Operation *op, SlapReply *rs )
{
	Entry *e;
	int rc;
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	ad_private *ap = (ad_private *)on->on_bi.bi_private;
	Attribute *a_dom, *a_dn;
	struct ldap_conncb ad_conncb = { .lc_add = remoteauth_conn_cb,
			.lc_del = remoteauth_conn_delcb,
			.lc_arg = ap };
	struct berval dn = { 0 };
	char *dom_val, *realm = NULL;
	char *ldap_url = NULL;
	LDAP *ld = NULL;
	int protocol = LDAP_VERSION3, isfile = 0;
	int tries = 0;

	if ( LogTest( LDAP_DEBUG_TRACE ) ) {
		trace_remoteauth_parameters( ap );
	}

	if ( op->orb_method != LDAP_AUTH_SIMPLE )
		return SLAP_CB_CONTINUE; /* only do password auth */

	/* Can't handle root via this mechanism */
	if ( be_isroot_dn( op->o_bd, &op->o_req_ndn ) ) return SLAP_CB_CONTINUE;

	if ( !ap->up_set ) {
		const char *txt = NULL;

		if ( slap_str2ad( UP_STR, &ap->up_ad, &txt ) )
			Debug( LDAP_DEBUG_TRACE, "remoteauth_bind: "
					"userPassword attr undefined: %s\n",
					txt ? txt : "" );
		ap->up_set = 1;
	}

	if ( !ap->up_ad ) {
		Debug( LDAP_DEBUG_TRACE, "remoteauth_bind: "
				"password attribute not configured\n" );
		return SLAP_CB_CONTINUE; /* userPassword not defined */
	}

	if ( !ap->dn ) {
		Debug( LDAP_DEBUG_TRACE, "remoteauth_bind: "
				"remote DN attribute not configured\n" );
		return SLAP_CB_CONTINUE; /* no mapped DN attribute */
	}

	if ( !ap->domain_attr ) {
		Debug( LDAP_DEBUG_TRACE, "remoteauth_bind: "
				"domain attribute not configured\n" );
		return SLAP_CB_CONTINUE; /* no way to know domain */
	}

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &e );
	if ( rc != LDAP_SUCCESS ) return SLAP_CB_CONTINUE;

	rc = SLAP_CB_CONTINUE;
	/* if userPassword is defined in entry, skip to the end */
	if ( attr_find( e->e_attrs, ap->up_ad ) ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"user has a password, skipping\n",
				op->o_log_prefix );
		goto exit;
	}

	a_dom = attr_find( e->e_attrs, ap->domain_ad );
	if ( !a_dom )
		dom_val = ap->default_domain;
	else {
		dom_val = a_dom->a_vals[0].bv_val;
	}

	if ( !dom_val ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"user has no domain nor do we have a default, skipping\n",
				op->o_log_prefix );
		goto exit; /* user has no domain */
	}

	realm = get_realm( dom_val, ap->mappings, ap->default_realm, &isfile );
	if ( !realm ) goto exit;

	a_dn = attr_find( e->e_attrs, ap->dn_ad );
	if ( !a_dn ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"no remote DN found on user\n",
				op->o_log_prefix );
		goto exit; /* user has no DN for the other directory */
	}

	ber_dupbv_x( &dn, a_dn->a_vals, op->o_tmpmemctx );
	be_entry_release_r( op, e );
	e = NULL;

	Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
			"(realm, dn) = (%s, %s)\n",
			op->o_log_prefix, realm, dn.bv_val );

	ldap_url = get_ldap_url( realm, isfile );
	if ( !ldap_url ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"No LDAP URL obtained\n",
				op->o_log_prefix );
		goto exit;
	}

retry:
	rc = ldap_initialize( &ld, ldap_url );
	if ( rc ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"Cannot initialize %s: %s\n",
				op->o_log_prefix, ldap_url, ldap_err2string( rc ) );
		goto exit; /* user has no DN for the other directory */
	}

	ldap_set_option( ld, LDAP_OPT_PROTOCOL_VERSION, &protocol );

#ifdef HAVE_TLS
	rc = bindconf_tls_set( &ap->ad_tls, ld );
	if ( rc ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"bindconf_tls_set failed\n",
				op->o_log_prefix );
		goto exit;
	}

	if ( ap->pins ) {
		if ( (rc = ldap_set_option( ld, LDAP_OPT_CONNECT_CB, &ad_conncb )) !=
				LDAP_SUCCESS ) {
			goto exit;
		}
	}

	if ( (rc = ldap_connect( ld )) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"Cannot connect to %s: %s\n",
				op->o_log_prefix, ldap_url, ldap_err2string( rc ) );
		goto exit;
	}

	if ( ap->ad_tls.sb_tls && !ldap_tls_inplace( ld ) ) {
		if ( (rc = ldap_start_tls_s( ld, NULL, NULL )) != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
					"LDAP TLS failed %s: %s\n",
					op->o_log_prefix, ldap_url, ldap_err2string( rc ) );
			goto exit;
		}
	}

#endif /* HAVE_TLS */

	rc = ldap_sasl_bind_s( ld, dn.bv_val, LDAP_SASL_SIMPLE,
			&op->oq_bind.rb_cred, NULL, NULL, NULL );
	if ( rc == LDAP_SUCCESS ) {
		if ( ap->store_on_success ) {
			const char *txt;

			Operation op2 = *op;
			SlapReply r2 = { REP_RESULT };
			slap_callback cb = { NULL, slap_null_cb, NULL, NULL };
			Modifications m = {};

			op2.o_tag = LDAP_REQ_MODIFY;
			op2.o_callback = &cb;
			op2.orm_modlist = &m;
			op2.orm_no_opattrs = 0;
			op2.o_dn = op->o_bd->be_rootdn;
			op2.o_ndn = op->o_bd->be_rootndn;

			m.sml_op = LDAP_MOD_ADD;
			m.sml_flags = 0;
			m.sml_next = NULL;
			m.sml_type = ap->up_ad->ad_cname;
			m.sml_desc = ap->up_ad;
			m.sml_numvals = 1;
			m.sml_values = op->o_tmpcalloc(
					sizeof(struct berval), 2, op->o_tmpmemctx );

			slap_passwd_hash( &op->oq_bind.rb_cred, &m.sml_values[0], &txt );
			if ( m.sml_values[0].bv_val == NULL ) {
				Debug( LDAP_DEBUG_ANY, "%s remoteauth_bind: "
						"password hashing for '%s' failed, storing password in "
						"plain text\n",
						op->o_log_prefix, op->o_req_dn.bv_val );
				ber_dupbv( &m.sml_values[0], &op->oq_bind.rb_cred );
			}

			/*
			 * If this server is a shadow use the frontend to perform this
			 * modify. That will trigger the update referral, which can then be
			 * forwarded by the chain overlay. Obviously the updateref and
			 * chain overlay must be configured appropriately for this to be
			 * useful.
			 */
			if ( SLAP_SHADOW(op->o_bd) ) {
				op2.o_bd = frontendDB;
			} else {
				op2.o_bd->bd_info = (BackendInfo *)on->on_info;
			}

			if ( op2.o_bd->be_modify( &op2, &r2 ) != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_ANY, "%s remoteauth_bind: "
						"attempt to store password in entry '%s' failed, "
						"ignoring\n",
						op->o_log_prefix, op->o_req_dn.bv_val );
			}
			ch_free( m.sml_values[0].bv_val );
		}
		goto exit;
	}

	if ( rc == LDAP_INVALID_CREDENTIALS ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"ldap_sasl_bind_s (%s) failed: invalid credentials\n",
				op->o_log_prefix, ldap_url );
		goto exit;
	}

	if ( tries < ap->retry_count ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"ldap_sasl_bind_s failed %s: %s (try #%d)\n",
				op->o_log_prefix, ldap_url, ldap_err2string( rc ), tries );
		if ( ld ) ldap_unbind_ext_s( ld, NULL, NULL );
		tries++;
		goto retry;
	} else
		goto exit;

exit:
	if ( dn.bv_val ) {
		op->o_tmpfree( dn.bv_val, op->o_tmpmemctx );
	}
	if ( e ) {
		be_entry_release_r( op, e );
	}
	if ( ld ) ldap_unbind_ext_s( ld, NULL, NULL );
	if ( ldap_url ) ch_free( ldap_url );
	if ( realm ) ch_free( realm );
	if ( rc == SLAP_CB_CONTINUE ) {
		Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
				"continue\n", op->o_log_prefix );
		return rc;
	} else {
		/* for rc == 0, frontend sends result */
		if ( rc ) {
			if ( rc > 0 ) {
				Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
						"failed\n", op->o_log_prefix );
				send_ldap_error( op, rs, rc, "remoteauth_bind failed" );
			} else {
				Debug( LDAP_DEBUG_TRACE, "%s remoteauth_bind: "
						"operations error\n", op->o_log_prefix );
				send_ldap_error( op, rs, LDAP_OPERATIONS_ERROR,
						"remoteauth_bind operations error" );
			}
		}

		return rs->sr_err;
	}
}

static int
remoteauth_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	ad_private *ap;

	if ( SLAP_ISGLOBALOVERLAY(be) ) {
		Debug( LDAP_DEBUG_ANY, "remoteauth_db_init: "
				"remoteauth overlay must be instantiated within a "
				"database.\n" );
		return 1;
	}

	ap = ch_calloc( 1, sizeof(ad_private) );

	ap->dn = NULL;
	ap->dn_ad = NULL;
	ap->domain_attr = NULL;
	ap->domain_ad = NULL;

	ap->up_ad = NULL;
	ap->mappings = NULL;

	ap->default_realm = NULL;
	ap->default_domain = NULL;

	ap->pins = NULL;

	ap->up_set = 0;
	ap->retry_count = 3;

	on->on_bi.bi_private = ap;

	return LDAP_SUCCESS;
}

static int
remoteauth_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	ad_private *ap = (ad_private *)on->on_bi.bi_private;
	ad_info *ai = ap->mappings;

	while ( ai ) {
		if ( ai->domain ) ch_free( ai->domain );
		if ( ai->realm ) ch_free( ai->realm );
		ai = ai->next;
	}

	if ( ap->dn ) ch_free( ap->dn );
	if ( ap->default_domain ) ch_free( ap->default_domain );
	if ( ap->default_realm ) ch_free( ap->default_realm );

	bindconf_free( &ap->ad_tls );

	ch_free( ap );

	return 0;
}

static slap_overinst remoteauth;

int
remoteauth_initialize( void )
{
	int rc;

	remoteauth.on_bi.bi_type = "remoteauth";
	remoteauth.on_bi.bi_flags = SLAPO_BFLAG_SINGLE;

	remoteauth.on_bi.bi_cf_ocs = remoteauthocs;
	rc = config_register_schema( remoteauthcfg, remoteauthocs );
	if ( rc ) return rc;

	remoteauth.on_bi.bi_db_init = remoteauth_db_init;
	remoteauth.on_bi.bi_db_destroy = remoteauth_db_destroy;
	remoteauth.on_bi.bi_op_bind = remoteauth_bind;

	return overlay_register( &remoteauth );
}

#if SLAPD_OVER_ACCESSLOG == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return remoteauth_initialize();
}
#endif
