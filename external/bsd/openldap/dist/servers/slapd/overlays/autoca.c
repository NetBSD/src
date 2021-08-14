/*	$NetBSD: autoca.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

/* autoca.c - Automatic Certificate Authority */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2021 The OpenLDAP Foundation.
 * Copyright 2009-2018 by Howard Chu.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: autoca.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#ifdef SLAPD_OVER_AUTOCA

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "slap-config.h"

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

/* Starting with OpenSSL 1.1.0, rsa.h is no longer included in
 * x509.h, so we need to explicitly include it for the
 * call to EVP_PKEY_CTX_set_rsa_keygen_bits
 */

#if OPENSSL_VERSION_NUMBER >= 0x10100000
#include <openssl/rsa.h>
#define X509_get_notBefore(x)	X509_getm_notBefore(x)
#define X509_get_notAfter(x)	X509_getm_notAfter(x)
#endif

/* This overlay implements a certificate authority that can generate
 * certificates automatically for any entry in the directory.
 * On startup it generates a self-signed CA cert for the directory's
 * suffix entry and uses this to sign all other certs that it generates.
 * User and server certs are generated on demand, using a Search request.
 */

#define LBER_TAG_OID        ((ber_tag_t) 0x06UL)
#define LBER_TAG_UTF8       ((ber_tag_t) 0x0cUL)

#define KEYBITS	2048
#define MIN_KEYBITS	512

#define ACA_SCHEMA_ROOT	"1.3.6.1.4.1.4203.666.11.11"

#define ACA_SCHEMA_AT ACA_SCHEMA_ROOT ".1"
#define ACA_SCHEMA_OC ACA_SCHEMA_ROOT ".2"

static AttributeDescription *ad_caCert, *ad_caPkey, *ad_usrCert, *ad_usrPkey;
static AttributeDescription *ad_mail, *ad_ipaddr;
static ObjectClass *oc_caObj, *oc_usrObj;

static char *aca_attrs[] = {
	"( " ACA_SCHEMA_AT ".1 NAME 'cAPrivateKey' "
		"DESC 'X.509 CA private key, use ;binary' "
		"SUP pKCS8PrivateKey )",
	"( " ACA_SCHEMA_AT ".2 NAME 'userPrivateKey' "
		"DESC 'X.509 user private key, use ;binary' "
		"SUP pKCS8PrivateKey )",
	NULL
};

static struct {
	char *at;
	AttributeDescription **ad;
} aca_attr2[] = {
	{ "cACertificate;binary", &ad_caCert },
	{ "cAPrivateKey;binary", &ad_caPkey },
	{ "userCertificate;binary", &ad_usrCert },
	{ "userPrivateKey;binary", &ad_usrPkey },
	{ "mail", &ad_mail },
	{ NULL }
};

static struct {
	char *ot;
	ObjectClass **oc;
} aca_ocs[] = {
	{ "( " ACA_SCHEMA_OC ".1 NAME 'autoCA' "
		"DESC 'Automated PKI certificate authority' "
		"SUP pkiCA AUXILIARY "
		"MAY cAPrivateKey )", &oc_caObj },
	{ "( " ACA_SCHEMA_OC ".2 NAME 'autoCAuser' "
		"DESC 'Automated PKI CA user' "
		"SUP pkiUser AUXILIARY "
		"MAY userPrivateKey )", &oc_usrObj },
	{ NULL }
};

typedef struct autoca_info {
	X509 *ai_cert;
	EVP_PKEY *ai_pkey;
	ObjectClass *ai_usrclass;
	ObjectClass *ai_srvclass;
	struct berval ai_localdn;
	struct berval ai_localndn;
	int ai_usrkeybits;
	int ai_srvkeybits;
	int ai_cakeybits;
	int ai_usrdays;
	int ai_srvdays;
	int ai_cadays;
} autoca_info;

/* Rewrite an LDAP DN in DER form
 * Input must be valid DN, therefore no error checking is done here.
 */
static int autoca_dnbv2der( Operation *op, struct berval *bv, struct berval *der )
{
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	LDAPDN dn;
	LDAPRDN rdn;
	LDAPAVA *ava;
	AttributeDescription *ad;
	int irdn, iava;

	ldap_bv2dn_x( bv, &dn, LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx );

	ber_init2( ber, NULL, LBER_USE_DER );
	ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

	/* count RDNs, we need them in reverse order */
	for (irdn = 0; dn[irdn]; irdn++);
	irdn--;

	/* DN is a SEQuence of RDNs */
	ber_start_seq( ber, LBER_SEQUENCE );
	for (; irdn >=0; irdn--)
	{
		/* RDN is a SET of AVAs */
		ber_start_set( ber, LBER_SET );
		rdn = dn[irdn];
		for (iava = 0; rdn[iava]; iava++)
		{
			const char *text;
			char oid[1024];
			struct berval bvo = { sizeof(oid), oid };
			struct berval bva;

			/* AVA is a SEQuence of attr and value */
			ber_start_seq( ber, LBER_SEQUENCE );
			ava = rdn[iava];
			ad = NULL;
			slap_bv2ad( &ava->la_attr, &ad, &text );
			ber_str2bv( ad->ad_type->sat_oid, 0, 0, &bva );
			ber_encode_oid( &bva, &bvo );
			ber_put_berval( ber, &bvo, LBER_TAG_OID );
			ber_put_berval( ber, &ava->la_value, LBER_TAG_UTF8 );
			ber_put_seq( ber );
		}
		ber_put_set( ber );
	}
	ber_put_seq( ber );
	ber_flatten2( ber, der, 0 );
	ldap_dnfree_x( dn, op->o_tmpmemctx );
	return 0;
}

static int autoca_genpkey(int bits, EVP_PKEY **pkey)
{
	EVP_PKEY_CTX *kctx;
	int rc;

	kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
	if (kctx == NULL)
		return -1;
	if (EVP_PKEY_keygen_init(kctx) <= 0)
	{
		EVP_PKEY_CTX_free(kctx);
		return -1;
	}
	if (EVP_PKEY_CTX_set_rsa_keygen_bits(kctx, bits) <= 0)
	{
		EVP_PKEY_CTX_free(kctx);
		return -1;
	}
	rc = EVP_PKEY_keygen(kctx, pkey);
	EVP_PKEY_CTX_free(kctx);
	return rc;
}

static int autoca_signcert(X509 *cert, EVP_PKEY *pkey)
{
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	EVP_PKEY_CTX *pkctx = NULL;
	int rc = -1;

	if ( ctx == NULL )
		return -1;
	if (EVP_DigestSignInit(ctx, &pkctx, NULL, NULL, pkey))
	{
		rc = X509_sign_ctx(cert, ctx);
	}
	EVP_MD_CTX_destroy(ctx);
	return rc;
}

#define SERIAL_BITS	64	/* should be less than 160 */

typedef struct myext {
	char *name;
	char *value;
} myext;

static myext CAexts[] = {
	{ "subjectKeyIdentifier", "hash" },
	{ "authorityKeyIdentifier", "keyid:always,issuer" },
	{ "basicConstraints", "critical,CA:true" },
	{ "keyUsage", "digitalSignature,cRLSign,keyCertSign" },
	{ "nsComment", "OpenLDAP automatic certificate" },
	{ NULL }
};

static myext usrExts[] = {
	{ "subjectKeyIdentifier", "hash" },
	{ "authorityKeyIdentifier", "keyid:always,issuer" },
	{ "basicConstraints", "CA:false" },
	{ "keyUsage", "digitalSignature,nonRepudiation,keyEncipherment" },
	{ "extendedKeyUsage", "clientAuth,emailProtection,codeSigning" },
	{ "nsComment", "OpenLDAP automatic certificate" },
	{ NULL }
};

static myext srvExts[] = {
	{ "subjectKeyIdentifier", "hash" },
	{ "authorityKeyIdentifier", "keyid:always,issuer" },
	{ "basicConstraints", "CA:false" },
	{ "keyUsage", "digitalSignature,keyEncipherment" },
	{ "extendedKeyUsage", "serverAuth,clientAuth" },
	{ "nsComment", "OpenLDAP automatic certificate" },
	{ NULL }
};

typedef struct genargs {
	X509 *issuer_cert;
	EVP_PKEY *issuer_pkey;
	struct berval *subjectDN;
	myext *cert_exts;
	myext *more_exts;
	X509 *newcert;
	EVP_PKEY *newpkey;
	struct berval dercert;
	struct berval derpkey;
	int keybits;
	int days;
} genargs;

static int autoca_gencert( Operation *op, genargs *args )
{
	X509_NAME *subj_name, *issuer_name;
	X509 *subj_cert;
	struct berval derdn;
	unsigned char *pp;
	EVP_PKEY *evpk = NULL;
	int rc;

	if ((subj_cert = X509_new()) == NULL)
		return -1;

	autoca_dnbv2der( op, args->subjectDN, &derdn );
	pp = (unsigned char *)derdn.bv_val;
	subj_name = d2i_X509_NAME( NULL, (const unsigned char **)&pp, derdn.bv_len );
	op->o_tmpfree( derdn.bv_val, op->o_tmpmemctx );
	if ( subj_name == NULL )
	{
fail1:
		X509_free( subj_cert );
		return -1;
	}

	rc = autoca_genpkey( args->keybits, &evpk );
	if ( rc <= 0 )
	{
fail2:
		if ( subj_name ) X509_NAME_free( subj_name );
		goto fail1;
	}
	/* encode DER in PKCS#8 */
	{
		PKCS8_PRIV_KEY_INFO *p8inf;
		if (( p8inf = EVP_PKEY2PKCS8( evpk )) == NULL )
			goto fail2;
		args->derpkey.bv_len = i2d_PKCS8_PRIV_KEY_INFO( p8inf, NULL );
		args->derpkey.bv_val = op->o_tmpalloc( args->derpkey.bv_len, op->o_tmpmemctx );
		pp = (unsigned char *)args->derpkey.bv_val;
		i2d_PKCS8_PRIV_KEY_INFO( p8inf, &pp );
		PKCS8_PRIV_KEY_INFO_free( p8inf );
	}
	args->newpkey = evpk;

	/* set random serial */
	{
		BIGNUM *bn = BN_new();
		if ( bn == NULL )
		{
fail3:
			EVP_PKEY_free( evpk );
			goto fail2;
		}
		if (!BN_pseudo_rand(bn, SERIAL_BITS, 0, 0))
		{
			BN_free( bn );
			goto fail3;
		}
		if (!BN_to_ASN1_INTEGER(bn, X509_get_serialNumber(subj_cert)))
		{
			BN_free( bn );
			goto fail3;
		}
		BN_free(bn);
	}
	if (args->issuer_cert) {
		issuer_name = X509_get_subject_name(args->issuer_cert);
	} else {
		issuer_name = subj_name;
		args->issuer_cert = subj_cert;
		args->issuer_pkey = evpk;
	}
	if (!X509_set_version(subj_cert, 2) ||	/* set version to V3 */
		!X509_set_issuer_name(subj_cert, issuer_name) ||
		!X509_set_subject_name(subj_cert, subj_name) ||
		!X509_gmtime_adj(X509_get_notBefore(subj_cert), 0) ||
		!X509_time_adj_ex(X509_get_notAfter(subj_cert), args->days, 0, NULL) ||
		!X509_set_pubkey(subj_cert, evpk))
	{
		goto fail3;
	}
	X509_NAME_free(subj_name);
	subj_name = NULL;

	/* set cert extensions */
	{
		X509V3_CTX ctx;
		X509_EXTENSION *ext;
		int i;

		X509V3_set_ctx(&ctx, args->issuer_cert, subj_cert, NULL, NULL, 0);
		for (i=0; args->cert_exts[i].name; i++) {
			ext = X509V3_EXT_nconf(NULL, &ctx, args->cert_exts[i].name, args->cert_exts[i].value);
			if ( ext == NULL )
				goto fail3;
			rc = X509_add_ext(subj_cert, ext, -1);
			X509_EXTENSION_free(ext);
			if ( !rc )
				goto fail3;
		}
		if (args->more_exts) {
			for (i=0; args->more_exts[i].name; i++) {
				ext = X509V3_EXT_nconf(NULL, &ctx, args->more_exts[i].name, args->more_exts[i].value);
				if ( ext == NULL )
					goto fail3;
				rc = X509_add_ext(subj_cert, ext, -1);
				X509_EXTENSION_free(ext);
				if ( !rc )
					goto fail3;
			}
		}
	}
	rc = autoca_signcert( subj_cert, args->issuer_pkey );
	if ( rc < 0 )
		goto fail3;
	args->dercert.bv_len = i2d_X509( subj_cert, NULL );
	args->dercert.bv_val = op->o_tmpalloc( args->dercert.bv_len, op->o_tmpmemctx );
	pp = (unsigned char *)args->dercert.bv_val;
	i2d_X509( subj_cert, &pp );
	args->newcert = subj_cert;
	return 0;
}

typedef struct saveargs {
	ObjectClass *oc;
	struct berval *dercert;
	struct berval *derpkey;
	slap_overinst *on;
	struct berval *dn;
	struct berval *ndn;
	int isca;
} saveargs;

static int autoca_savecert( Operation *op, saveargs *args )
{
	Modifications mod[3], *mp = mod;
	struct berval bvs[6], *bp = bvs;
	BackendInfo *bi;
	slap_callback cb = {0};
	SlapReply rs = {REP_RESULT};

	if ( args->oc ) {
		mp->sml_numvals = 1;
		mp->sml_values = bp;
		mp->sml_nvalues = NULL;
		mp->sml_desc = slap_schema.si_ad_objectClass;
		mp->sml_op = LDAP_MOD_ADD;
		mp->sml_flags = SLAP_MOD_INTERNAL;
		*bp++ = args->oc->soc_cname;
		BER_BVZERO( bp );
		bp++;
		mp->sml_next = mp+1;
		mp++;
	}
	mp->sml_numvals = 1;
	mp->sml_values = bp;
	mp->sml_nvalues = NULL;
	mp->sml_desc = args->isca ? ad_caCert : ad_usrCert;
	mp->sml_op = LDAP_MOD_REPLACE;
	mp->sml_flags = SLAP_MOD_INTERNAL;
	*bp++ = *args->dercert;
	BER_BVZERO( bp );
	bp++;
	mp->sml_next = mp+1;
	mp++;

	mp->sml_numvals = 1;
	mp->sml_values = bp;
	mp->sml_nvalues = NULL;
	mp->sml_desc = args->isca ? ad_caPkey : ad_usrPkey;
	mp->sml_op = LDAP_MOD_ADD;
	mp->sml_flags = SLAP_MOD_INTERNAL;
	*bp++ = *args->derpkey;
	BER_BVZERO( bp );
	mp->sml_next = NULL;

	cb.sc_response = slap_null_cb;
	bi = op->o_bd->bd_info;
	op->o_bd->bd_info = args->on->on_info->oi_orig;
	op->o_tag = LDAP_REQ_MODIFY;
	op->o_callback = &cb;
	op->orm_modlist = mod;
	op->orm_no_opattrs = 1;
	op->o_req_dn = *args->dn;
	op->o_req_ndn = *args->ndn;
	op->o_bd->be_modify( op, &rs );
	op->o_bd->bd_info = bi;
	return rs.sr_err;
}

static const struct berval configDN = BER_BVC("cn=config");

/* must run as a pool thread to avoid cn=config deadlock */
static void *
autoca_setca_task( void *ctx, void *arg )
{
	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	struct berval *cacert = arg;
	Modifications mod;
	struct berval bvs[2];
	slap_callback cb = {0};
	SlapReply rs = {REP_RESULT};
	const char *text;

	connection_fake_init( &conn, &opbuf, ctx );
	op = &opbuf.ob_op;

	mod.sml_numvals = 1;
	mod.sml_values = bvs;
	mod.sml_nvalues = NULL;
	mod.sml_desc = NULL;
	if ( slap_str2ad( "olcTLSCACertificate;binary", &mod.sml_desc, &text ))
		goto leave;
	mod.sml_op = LDAP_MOD_REPLACE;
	mod.sml_flags = SLAP_MOD_INTERNAL;
	bvs[0] = *cacert;
	BER_BVZERO( &bvs[1] );
	mod.sml_next = NULL;

	cb.sc_response = slap_null_cb;
	op->o_bd = select_backend( (struct berval *)&configDN, 0 );
	if ( !op->o_bd )
		goto leave;

	op->o_tag = LDAP_REQ_MODIFY;
	op->o_callback = &cb;
	op->orm_modlist = &mod;
	op->orm_no_opattrs = 1;
	op->o_req_dn = configDN;
	op->o_req_ndn = configDN;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	op->o_bd->be_modify( op, &rs );
leave:
	ch_free( arg );
	return NULL;
}

static int
autoca_setca( struct berval *cacert )
{
	struct berval *bv = ch_malloc( sizeof(struct berval) + cacert->bv_len );
	bv->bv_len = cacert->bv_len;
	bv->bv_val = (char *)(bv+1);
	AC_MEMCPY( bv->bv_val, cacert->bv_val, bv->bv_len );
	return ldap_pvt_thread_pool_submit( &connection_pool, autoca_setca_task, bv );
}

static int
autoca_setlocal( Operation *op, struct berval *cert, struct berval *pkey )
{
	Modifications mod[2];
	struct berval bvs[4];
	slap_callback cb = {0};
	SlapReply rs = {REP_RESULT};
	const char *text;

	mod[0].sml_numvals = 1;
	mod[0].sml_values = bvs;
	mod[0].sml_nvalues = NULL;
	mod[0].sml_desc = NULL;
	if ( slap_str2ad( "olcTLSCertificate;binary", &mod[0].sml_desc, &text ))
		return -1;
	mod[0].sml_op = LDAP_MOD_REPLACE;
	mod[0].sml_flags = SLAP_MOD_INTERNAL;
	bvs[0] = *cert;
	BER_BVZERO( &bvs[1] );
	mod[0].sml_next = &mod[1];

	mod[1].sml_numvals = 1;
	mod[1].sml_values = &bvs[2];
	mod[1].sml_nvalues = NULL;
	mod[1].sml_desc = NULL;
	if ( slap_str2ad( "olcTLSCertificateKey;binary", &mod[1].sml_desc, &text ))
		return -1;
	mod[1].sml_op = LDAP_MOD_REPLACE;
	mod[1].sml_flags = SLAP_MOD_INTERNAL;
	bvs[2] = *pkey;
	BER_BVZERO( &bvs[3] );
	mod[1].sml_next = NULL;

	cb.sc_response = slap_null_cb;
	op->o_bd = select_backend( (struct berval *)&configDN, 0 );
	if ( !op->o_bd )
		return -1;

	op->o_tag = LDAP_REQ_MODIFY;
	op->o_callback = &cb;
	op->orm_modlist = mod;
	op->orm_no_opattrs = 1;
	op->o_req_dn = configDN;
	op->o_req_ndn = configDN;
	op->o_dn = op->o_bd->be_rootdn;
	op->o_ndn = op->o_bd->be_rootndn;
	op->o_bd->be_modify( op, &rs );
	return rs.sr_err;
}

enum {
	ACA_USRCLASS = 1,
	ACA_SRVCLASS,
	ACA_USRKEYBITS,
	ACA_SRVKEYBITS,
	ACA_CAKEYBITS,
	ACA_USRDAYS,
	ACA_SRVDAYS,
	ACA_CADAYS,
	ACA_LOCALDN
};

static int autoca_cf( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	autoca_info *ai = on->on_bi.bi_private;
	int rc = 0;

	switch( c->op ) {
	case SLAP_CONFIG_EMIT:
		switch( c->type ) {
		case ACA_USRCLASS:
			if ( ai->ai_usrclass ) {
				c->value_string = ch_strdup( ai->ai_usrclass->soc_cname.bv_val );
			} else {
				rc = 1;
			}
			break;
		case ACA_SRVCLASS:
			if ( ai->ai_srvclass ) {
				c->value_string = ch_strdup( ai->ai_srvclass->soc_cname.bv_val );
			} else {
				rc = 1;
			}
			break;
		case ACA_USRKEYBITS:
			c->value_int = ai->ai_usrkeybits;
			break;
		case ACA_SRVKEYBITS:
			c->value_int = ai->ai_srvkeybits;
			break;
		case ACA_CAKEYBITS:
			c->value_int = ai->ai_cakeybits;
			break;
		case ACA_USRDAYS:
			c->value_int = ai->ai_usrdays;
			break;
		case ACA_SRVDAYS:
			c->value_int = ai->ai_srvdays;
			break;
		case ACA_CADAYS:
			c->value_int = ai->ai_cadays;
			break;
		case ACA_LOCALDN:
			if ( !BER_BVISNULL( &ai->ai_localdn )) {
				rc = value_add_one( &c->rvalue_vals, &ai->ai_localdn );
			} else {
				rc = 1;
			}
			break;
		}
		break;
	case LDAP_MOD_DELETE:
		switch( c->type ) {
		case ACA_USRCLASS:
			ai->ai_usrclass = NULL;
			break;
		case ACA_SRVCLASS:
			ai->ai_srvclass = NULL;
			break;
		case ACA_LOCALDN:
			if ( ai->ai_localdn.bv_val ) {
				ch_free( ai->ai_localdn.bv_val );
				ch_free( ai->ai_localndn.bv_val );
				BER_BVZERO( &ai->ai_localdn );
				BER_BVZERO( &ai->ai_localndn );
			}
			break;
		/* single-valued attrs, all no-ops */
		}
		break;
	case SLAP_CONFIG_ADD:
	case LDAP_MOD_ADD:
		switch( c->type ) {
		case ACA_USRCLASS:
			{
				ObjectClass *oc = oc_find( c->value_string );
				if ( oc )
					ai->ai_usrclass = oc;
				else
					rc = 1;
			}
			break;
		case ACA_SRVCLASS:
			{
				ObjectClass *oc = oc_find( c->value_string );
				if ( oc )
					ai->ai_srvclass = oc;
				else
					rc = 1;
			}
		case ACA_USRKEYBITS:
			if ( c->value_int < MIN_KEYBITS )
				rc = 1;
			else
				ai->ai_usrkeybits = c->value_int;
			break;
		case ACA_SRVKEYBITS:
			if ( c->value_int < MIN_KEYBITS )
				rc = 1;
			else
				ai->ai_srvkeybits = c->value_int;
			break;
		case ACA_CAKEYBITS:
			if ( c->value_int < MIN_KEYBITS )
				rc = 1;
			else
				ai->ai_cakeybits = c->value_int;
			break;
		case ACA_USRDAYS:
			ai->ai_usrdays = c->value_int;
			break;
		case ACA_SRVDAYS:
			ai->ai_srvdays = c->value_int;
			break;
		case ACA_CADAYS:
			ai->ai_cadays = c->value_int;
			break;
		case ACA_LOCALDN:
			if ( c->be->be_nsuffix == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"suffix must be set" );
				Debug( LDAP_DEBUG_CONFIG, "autoca_config: %s\n",
					c->cr_msg );
				rc = ARG_BAD_CONF;
				break;
			}
			if ( !dnIsSuffix( &c->value_ndn, c->be->be_nsuffix )) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"DN is not a subordinate of backend" );
				Debug( LDAP_DEBUG_CONFIG, "autoca_config: %s\n",
					c->cr_msg );
				rc = ARG_BAD_CONF;
				break;
			}
			if ( ai->ai_localdn.bv_val ) {
				ch_free( ai->ai_localdn.bv_val );
				ch_free( ai->ai_localndn.bv_val );
			}
			ai->ai_localdn = c->value_dn;
			ai->ai_localndn = c->value_ndn;
		}
	}
	return rc;
}

static ConfigTable autoca_cfg[] = {
	{ "userClass", "objectclass", 2, 2, 0,
	  ARG_STRING|ARG_MAGIC|ACA_USRCLASS, autoca_cf,
	  "( OLcfgOvAt:22.1 NAME 'olcAutoCAuserClass' "
	  "DESC 'ObjectClass of user entries' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "serverClass", "objectclass", 2, 2, 0,
	  ARG_STRING|ARG_MAGIC|ACA_SRVCLASS, autoca_cf,
	  "( OLcfgOvAt:22.2 NAME 'olcAutoCAserverClass' "
	  "DESC 'ObjectClass of server entries' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "userKeybits", "integer", 2, 2, 0,
	  ARG_INT|ARG_MAGIC|ACA_USRKEYBITS, autoca_cf,
	  "( OLcfgOvAt:22.3 NAME 'olcAutoCAuserKeybits' "
	  "DESC 'Size of PrivateKey for user entries' "
	  "EQUALITY integerMatch "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "serverKeybits", "integer", 2, 2, 0,
	  ARG_INT|ARG_MAGIC|ACA_SRVKEYBITS, autoca_cf,
	  "( OLcfgOvAt:22.4 NAME 'olcAutoCAserverKeybits' "
	  "DESC 'Size of PrivateKey for server entries' "
	  "EQUALITY integerMatch "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "caKeybits", "integer", 2, 2, 0,
	  ARG_INT|ARG_MAGIC|ACA_CAKEYBITS, autoca_cf,
	  "( OLcfgOvAt:22.5 NAME 'olcAutoCAKeybits' "
	  "DESC 'Size of PrivateKey for CA certificate' "
	  "EQUALITY integerMatch "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "userDays", "integer", 2, 2, 0,
	  ARG_INT|ARG_MAGIC|ACA_USRDAYS, autoca_cf,
	  "( OLcfgOvAt:22.6 NAME 'olcAutoCAuserDays' "
	  "DESC 'Lifetime of user certificates in days' "
	  "EQUALITY integerMatch "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "serverDays", "integer", 2, 2, 0,
	  ARG_INT|ARG_MAGIC|ACA_SRVDAYS, autoca_cf,
	  "( OLcfgOvAt:22.7 NAME 'olcAutoCAserverDays' "
	  "DESC 'Lifetime of server certificates in days' "
	  "EQUALITY integerMatch "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "caDays", "integer", 2, 2, 0,
	  ARG_INT|ARG_MAGIC|ACA_CADAYS, autoca_cf,
	  "( OLcfgOvAt:22.8 NAME 'olcAutoCADays' "
	  "DESC 'Lifetime of CA certificate in days' "
	  "EQUALITY integerMatch "
	  "SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "localdn", "dn", 2, 2, 0,
	  ARG_DN|ARG_QUOTE|ARG_MAGIC|ACA_LOCALDN, autoca_cf,
	  "( OLcfgOvAt:22.9 NAME 'olcAutoCAlocalDN' "
	  "DESC 'DN of local server cert' "
	  "EQUALITY distinguishedNameMatch "
	  "SYNTAX OMsDN SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs autoca_ocs[] = {
	{ "( OLcfgOvOc:22.1 "
	  "NAME 'olcAutoCAConfig' "
	  "DESC 'AutoCA configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcAutoCAuserClass $ olcAutoCAserverClass $ "
	   "olcAutoCAuserKeybits $ olcAutoCAserverKeybits $ olcAutoCAKeyBits $ "
	   "olcAutoCAuserDays $ olcAutoCAserverDays $ olcAutoCADays $ "
	   "olcAutoCAlocalDN ) )",
	  Cft_Overlay, autoca_cfg },
	{ NULL, 0, NULL }
};

static int
autoca_op_response(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = op->o_callback->sc_private;
	autoca_info *ai = on->on_bi.bi_private;
	Attribute *a;
	int isusr = 0;

	if (rs->sr_type != REP_SEARCH)
		return SLAP_CB_CONTINUE;

	/* If root or self */
	if ( !be_isroot( op ) &&
		!dn_match( &rs->sr_entry->e_nname, &op->o_ndn ))
		return SLAP_CB_CONTINUE;

	isusr = is_entry_objectclass( rs->sr_entry, ai->ai_usrclass, SLAP_OCF_CHECK_SUP );
	if ( !isusr )
	{
		if (!is_entry_objectclass( rs->sr_entry, ai->ai_srvclass, SLAP_OCF_CHECK_SUP ))
			return SLAP_CB_CONTINUE;
	}
	a = attr_find( rs->sr_entry->e_attrs, ad_usrPkey );
	if ( !a )
	{
		Operation op2;
		genargs args;
		saveargs arg2;
		myext extras[2];
		int rc;

		args.issuer_cert = ai->ai_cert;
		args.issuer_pkey = ai->ai_pkey;
		args.subjectDN = &rs->sr_entry->e_name;
		args.more_exts = NULL;
		if ( isusr )
		{
			args.cert_exts = usrExts;
			args.keybits = ai->ai_usrkeybits;
			args.days = ai->ai_usrdays;
			a = attr_find( rs->sr_entry->e_attrs, ad_mail );
			if ( a )
			{
				extras[0].name = "subjectAltName";
				extras[1].name = NULL;
				extras[0].value = op->o_tmpalloc( sizeof("email:") + a->a_vals[0].bv_len, op->o_tmpmemctx );
				sprintf(extras[0].value, "email:%s", a->a_vals[0].bv_val);
				args.more_exts = extras;
			}
		} else
		{
			args.cert_exts = srvExts;
			args.keybits = ai->ai_srvkeybits;
			args.days = ai->ai_srvdays;
			if ( ad_ipaddr && (a = attr_find( rs->sr_entry->e_attrs, ad_ipaddr )))
			{
				extras[0].name = "subjectAltName";
				extras[1].name = NULL;
				extras[0].value = op->o_tmpalloc( sizeof("IP:") + a->a_vals[0].bv_len, op->o_tmpmemctx );
				sprintf(extras[0].value, "IP:%s", a->a_vals[0].bv_val);
				args.more_exts = extras;
			}
		}
		rc = autoca_gencert( op, &args );
		if ( rc )
			return SLAP_CB_CONTINUE;
		X509_free( args.newcert );
		EVP_PKEY_free( args.newpkey );

		if ( is_entry_objectclass( rs->sr_entry, oc_usrObj, 0 ))
			arg2.oc = NULL;
		else
			arg2.oc = oc_usrObj;
		if ( !( rs->sr_flags & REP_ENTRY_MODIFIABLE ))
		{
			Entry *e = entry_dup( rs->sr_entry );
			rs_replace_entry( op, rs, on, e );
			rs->sr_flags |= REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED;
		}
		arg2.dercert = &args.dercert;
		arg2.derpkey = &args.derpkey;
		arg2.on = on;
		arg2.dn = &rs->sr_entry->e_name;
		arg2.ndn = &rs->sr_entry->e_nname;
		arg2.isca = 0;
		op2 = *op;
		rc = autoca_savecert( &op2, &arg2 );
		if ( !rc )
		{
			/* If this is our cert DN, configure it */
			if ( dn_match( &rs->sr_entry->e_nname, &ai->ai_localndn ))
				autoca_setlocal( &op2, &args.dercert, &args.derpkey );
			attr_merge_one( rs->sr_entry, ad_usrCert, &args.dercert, NULL );
			attr_merge_one( rs->sr_entry, ad_usrPkey, &args.derpkey, NULL );
		}
		op->o_tmpfree( args.dercert.bv_val, op->o_tmpmemctx );
		op->o_tmpfree( args.derpkey.bv_val, op->o_tmpmemctx );
	}

	return SLAP_CB_CONTINUE;
}

static int
autoca_op_search(
	Operation *op,
	SlapReply *rs
)
{
	/* we only act on a search that returns just our cert/key attrs */
	if ( op->ors_attrs && op->ors_attrs[0].an_desc == ad_usrCert &&
		op->ors_attrs[1].an_desc == ad_usrPkey &&
		op->ors_attrs[2].an_name.bv_val == NULL )
	{
		slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
		slap_callback *sc = op->o_tmpcalloc( 1, sizeof(slap_callback), op->o_tmpmemctx );
		sc->sc_response = autoca_op_response;
		sc->sc_private = on;
		sc->sc_next = op->o_callback;
		op->o_callback = sc;
	}
	return SLAP_CB_CONTINUE;
}

static int
autoca_db_init(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	autoca_info *ai;

	ai = ch_calloc(1, sizeof(autoca_info));
	on->on_bi.bi_private = ai;

	/* set defaults */
	ai->ai_usrclass = oc_find( "person" );
	ai->ai_srvclass = oc_find( "ipHost" );
	ai->ai_usrkeybits = KEYBITS;
	ai->ai_srvkeybits = KEYBITS;
	ai->ai_cakeybits = KEYBITS;
	ai->ai_usrdays = 365;	/* 1 year */
	ai->ai_srvdays = 1826;	/* 5 years */
	ai->ai_cadays = 3652;	/* 10 years */
	return 0;
}

static int
autoca_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	autoca_info *ai = on->on_bi.bi_private;

	if ( ai->ai_cert )
		X509_free( ai->ai_cert );
	if ( ai->ai_pkey )
		EVP_PKEY_free( ai->ai_pkey );
	ch_free( ai );

	return 0;
}

static int
autoca_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	autoca_info *ai = on->on_bi.bi_private;

	Connection conn = { 0 };
	OperationBuffer opbuf;
	Operation *op;
	void *thrctx;
	Entry *e = NULL;
	Attribute *a;
	int rc;

	if (slapMode & SLAP_TOOL_MODE)
		return 0;

	if ( ! *aca_attr2[0].ad ) {
		int i, code;
		const char *text;

		for ( i=0; aca_attr2[i].at; i++ ) {
			code = slap_str2ad( aca_attr2[i].at, aca_attr2[i].ad, &text );
			if ( code ) return code;
		}

		/* Schema may not be loaded, ignore if missing */
		slap_str2ad( "ipHostNumber", &ad_ipaddr, &text );

		for ( i=0; aca_ocs[i].ot; i++ ) {
			code = register_oc( aca_ocs[i].ot, aca_ocs[i].oc, 0 );
			if ( code ) return code;
		}
	}

	thrctx = ldap_pvt_thread_pool_context();
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op = &opbuf.ob_op;
	op->o_bd = be;
	op->o_dn = be->be_rootdn;
	op->o_ndn = be->be_rootndn;
	rc = overlay_entry_get_ov( op, be->be_nsuffix, NULL, 
		NULL, 0, &e, on );

	if ( e ) {
		int gotoc = 0, gotat = 0;
		if ( is_entry_objectclass( e, oc_caObj, 0 )) {
			gotoc = 1;
			a = attr_find( e->e_attrs, ad_caPkey );
			if ( a ) {
				const unsigned char *pp;
				pp = (unsigned char *)a->a_vals[0].bv_val;
				ai->ai_pkey = d2i_AutoPrivateKey( NULL, &pp, a->a_vals[0].bv_len );
				if ( ai->ai_pkey )
				{
					a = attr_find( e->e_attrs, ad_caCert );
					if ( a )
					{
						pp = (unsigned char *)a->a_vals[0].bv_val;
						ai->ai_cert = d2i_X509( NULL, &pp, a->a_vals[0].bv_len );
						/* If TLS wasn't configured yet, set this as our CA */
						if ( !slap_tls_ctx )
							autoca_setca( a->a_vals );
					}
				}
				gotat = 1;
			}
		}
		overlay_entry_release_ov( op, e, 0, on );
		/* generate attrs, store... */
		if ( !gotat ) {
			genargs args;
			saveargs arg2;

			args.issuer_cert = NULL;
			args.issuer_pkey = NULL;
			args.subjectDN = &be->be_suffix[0];
			args.cert_exts = CAexts;
			args.more_exts = NULL;
			args.keybits = ai->ai_cakeybits;
			args.days = ai->ai_cadays;

			rc = autoca_gencert( op, &args );
			if ( rc )
				return -1;

			ai->ai_cert = args.newcert;
			ai->ai_pkey = args.newpkey;

			arg2.dn = be->be_suffix;
			arg2.ndn = be->be_nsuffix;
			arg2.isca = 1;
			if ( !gotoc )
				arg2.oc = oc_caObj;
			else
				arg2.oc = NULL;
			arg2.on = on;
			arg2.dercert = &args.dercert;
			arg2.derpkey = &args.derpkey;

			autoca_savecert( op, &arg2 );

			/* If TLS wasn't configured yet, set this as our CA */
			if ( !slap_tls_ctx )
				autoca_setca( &args.dercert );

			op->o_tmpfree( args.dercert.bv_val, op->o_tmpmemctx );
			op->o_tmpfree( args.derpkey.bv_val, op->o_tmpmemctx );
		}
	}

	return 0;
}

static slap_overinst autoca;

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

int autoca_initialize() {
	int i, code;

	autoca.on_bi.bi_type = "autoca";
	autoca.on_bi.bi_flags = SLAPO_BFLAG_SINGLE;
	autoca.on_bi.bi_db_init = autoca_db_init;
	autoca.on_bi.bi_db_destroy = autoca_db_destroy;
	autoca.on_bi.bi_db_open = autoca_db_open;
	autoca.on_bi.bi_op_search = autoca_op_search;

	autoca.on_bi.bi_cf_ocs = autoca_ocs;
	code = config_register_schema( autoca_cfg, autoca_ocs );
	if ( code ) return code;

	for ( i=0; aca_attrs[i]; i++ ) {
		code = register_at( aca_attrs[i], NULL, 0 );
		if ( code ) return code;
	}

	return overlay_register( &autoca );
}

#if SLAPD_OVER_AUTOCA == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return autoca_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_AUTOCA) */
