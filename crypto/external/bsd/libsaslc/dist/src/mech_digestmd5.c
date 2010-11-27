/* $Id: mech_digestmd5.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

/* Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mateusz Kocielski.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.	IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <saslc.h>
#include "error.h"
#include "saslc_private.h"
#include "mech.h"
#include "crypto.h"

/* local headers */

/* peoperties */
#define SASLC_DIGEST_MD5_AUTHZID	"AUTHZID"
#define SASLC_DIGEST_MD5_AUTHID		"AUTHID"
#define SASLC_DIGEST_MD5_REALM		"REALM"
#define SASLC_DIGEST_MD5_PASSWORD	"PASSWD"
#define SASLC_DIGEST_MD5_SERVICE	"SERVICE"
#define SASLC_DIGEST_MD5_SERVICEN	"SERVICENAME"
#define SASLC_DIGEST_MD5_HOSTNAME	"HOSTNAME"

/* QOP values */
enum {
	QOP_AUTH = 0,
	QOP_AUTH_INT = 1,
	QOP_AUTH_CONF = 2
};

#define QOP_AUTH_STR		"auth"
#define QOP_AUTH_INT_STR	"auth-int"
#define QOP_AUTH_CONF_STR	"auth-conf"

static const char *saslc__qop_str[] = {
	QOP_AUTH_STR,
	QOP_AUTH_INT_STR,
	QOP_AUTH_CONF_STR
};

/* defined by RFC2831 */
#define AUTH_PREFIX		"AUTHENTICATE"
#define AUTH_INT_CONF_SUFFIX	":00000000000000000000000000000000"
#define NONCE_LEN 8

/* DigestMD5 options */
#define STRING_REALM		"realm"
#define STRING_NONCE		"nonce"
#define STRING_QOP		"qop"
#define STRING_STALE		"stale"
#define STRING_MAXBUF		"maxbuf"
#define STRING_CHARSET		"charset"
#define STRING_ALGORITHM	"algorithm"
#define STRING_CIPHER		"cipher"


/** mech state */
typedef struct {
	saslc__mech_sess_t mech_sess; /**< mechanism session */
	/* additional stuff */

	/* this parameters should be setup once */
	char maxbuf_s; /**< is maxbuf defined */
	char algorithm_s; /**< is algorithm defined */

	/* session stuff */
	char *realm; /**< realm */
	char *nonce; /**< nonce */
	char *cnonce; /**< client nonce */
	const char *encoding; /**< encoding */
	int nonce_cnt; /**< nonce count */
	char *digesturi; /**< digest URI */
	char *cipher; /**< cipher */
	int qop; /**< qop */
} saslc__mech_digestmd5_sess_t;


/* prototypes */
static int saslc__mech_digestmd5_create(saslc_sess_t *);
static int saslc__mech_digestmd5_destroy(saslc_sess_t *);
static int saslc__mech_digestmd5_cont(saslc_sess_t *, const void *, size_t, 
    void **, size_t *);
static int saslc__mech_digestmd5_parse_challenge(
    saslc__mech_digestmd5_sess_t *, const char *);
static char *saslc__mech_digestmd5_nonce(size_t);
static char *saslc__mech_digestmd5_digesturi(const char *, const char *,
    const char *);
static char *saslc__mech_digestmd5_a1(const char *, const char *,
    const char *, const char *);
static char *saslc__mech_digestmd5_a2(const char *, int);
static char *saslc__mech_digestmd5_rhash(const char *, const char *,
    const char *, const char *, int, int);
static char *saslc__mech_digestmd5_rhash(const char *, const char *,
    const char *, const char *, int, int);
static char * saslc__mech_digestmd5_response(saslc__mech_digestmd5_sess_t *,
    const char *, const char *, const char *);

/**
 * @brief computing MD5(username:realm:password).
 * @param username user name
 * @param realm realm
 * @param password password
 * @return hash converted to ascii
 */

static char *
saslc__mech_digestmd5_userhash(const char *username, const char *realm,
	const char *password)
{
	char *tmp, *r;

	if (asprintf(&tmp, "%s:%s:%s", username, realm, password) == -1)
		return NULL;

	r = saslc__crypto_md5(tmp);
	free(tmp);

	return r;
}

/**
 * @brief computes A1 hash value (see: RFC2831)
 * @param userhash user hash 
 * @param nonce server's nonce
 * @param cnonce client's nonce
 * @param authzid authzid
 * @return hash converted to ascii
 */

static char *
saslc__mech_digestmd5_a1(const char *userhash, const char *nonce,
	const char *cnonce, const char *authzid)
{
	char *tmp, *r;

	if (asprintf(&tmp, "%s:%s:%s:%s", userhash, nonce, cnonce, authzid)
	    == -1 || tmp == NULL)
		return NULL;

	r = saslc__crypto_md5(tmp);
	free(tmp);

	return r;
}

/**
 * @brief computes A2 hash value (see: RFC2831)
 * @param digesturi digest uri
 * @param qop qop method
 * @return hash converted to ascii
 */

static char *
saslc__mech_digestmd5_a2(const char *digesturi, int qop)
{
	char *tmp, *r;

	if (asprintf(&tmp, "%s:%s:%s", AUTH_PREFIX, digesturi, 
	    qop != QOP_AUTH ? AUTH_INT_CONF_SUFFIX : "") == -1)
		return NULL;

	r = saslc__crypto_md5(tmp);
	free(tmp);

	return r;
}

/**
 * @brief computes result hash.
 * @param a1 A1 hash value
 * @param a2 A2 hash value
 * @param nonce server's nonce
 * @param cnonce client's nonce
 * @param nocne_cnt nonce counter
 * @param qop qop method
 * @return hash converted to ascii, NULL on failure.
 */

static char *
saslc__mech_digestmd5_rhash(const char *a1, const char *a2, const char *nonce,
	const char *cnonce, int nonce_cnt, int qop)
{
	char *tmp, *r;

	switch(qop) {
	case QOP_AUTH:
	case QOP_AUTH_INT:
	case QOP_AUTH_CONF:
		break;
	default:
		return NULL;
	}

	if (asprintf(&tmp, "%s:%s:%08x:%s:%s:%s", a1, nonce, nonce_cnt,
	    cnonce, saslc__qop_str[qop], a2) == -1)
		return NULL;

	r = saslc__crypto_md5(tmp);
	free(tmp);

	return r;
}


/**
 * @brief building response string. Basing on
 * session and mechanism properties.
 * @param sess mechanism session
 * @param username user name
 * @param password password
 * @param authzid authzid
 * @return response string, NULL on failure.
 */

static char *
saslc__mech_digestmd5_response(saslc__mech_digestmd5_sess_t *sess,
	const char *username, const char *password, const char *authzid)
{
	char *r = NULL, *userhash, *a1, *a2;

	userhash = saslc__mech_digestmd5_userhash(username, sess->realm,
	    password);

	if (userhash == NULL)
		return NULL;

	a1 = saslc__mech_digestmd5_a1(userhash, sess->nonce, sess->cnonce,
	    authzid);

	if (a1 == NULL)
		goto out;

	a2 = saslc__mech_digestmd5_a2(sess->digesturi, sess->qop);

	if (a2 == NULL)
		goto out1;

	r = saslc__mech_digestmd5_rhash(a1, a2, sess->nonce, sess->cnonce,
	    sess->nonce_cnt, sess->qop);

	free(a2);
out1:
	free(a1);
out:
	free(userhash);

	return r;
}

/**
 * @brief builds digesturi string
 * @param service service
 * @param service_name service name
 * @param realm realm
 * @return digesturi string, NULL on failure.
 */

static char *
saslc__mech_digestmd5_digesturi(const char *service, const char *service_name,
	const char *realm)
{
	char *r;
	int rv;

	if (service_name == NULL)
		rv = asprintf(&r, "%s/%s", service, realm);
	else
		rv = asprintf(&r, "%s/%s/%s", service, realm, service_name);

	if (rv == -1)
		return NULL;

	return r;
}

/**
 * @brief creates client's nonce. (Basing on crypto.h)
 * @param s length of nonce
 * @return nonce string, NULL on failure.
 */

static char *
saslc__mech_digestmd5_nonce(size_t s)
{
	unsigned char *nonce;
	char *r;

	nonce = saslc__crypto_nonce(s);

	if (nonce == NULL)
		return NULL;

	r = saslc__crypto_base64(nonce, s);
	free(nonce);

	return r;
}

/**
 * @brief parses challenge and store result in
 * mech_sess. Note that function destroys challenge string.
 * @param mech_sess session where parsed data will be stored
 * @param challenge challenge
 * @return 0 on success, -1 on failure.
 */

static int
saslc__mech_digestmd5_parse_challenge(saslc__mech_digestmd5_sess_t *mech_sess,
	const char *challenge)
{
	char *copy, *c, *n, *opt, *val;
	size_t len;
	int rv = -1;

	if ((copy = strdup(challenge)) == NULL)
		return -1;

	for (c = copy; c != '\0'; ) {
		/* get next option */
		opt = c;
		if (c == '\0')
			goto out;
		n = strchr(c, '=');
		c = n;
		*c = '\0';
		c++;
		val = c;
		n = strchr(c, ',');
		*c = '\0';
		c++;
		/* strip " */
		if (val[0] == '"')
			val++;
		len = strlen(val);
		if (len == 0)
			goto out;
		if (val[len] == '"')
			val[len] = '\0';
		/* parse it */
		if (strcasecmp(opt, STRING_REALM)) {
			if ((mech_sess->realm = strdup(val)) == NULL)
				goto out;
			continue;
		}
		if (strcasecmp(opt, STRING_NONCE)) {
			if ((mech_sess->nonce = strdup(val)) == NULL)
				goto out;
			continue;
		}
		if (strcasecmp(opt, STRING_CIPHER)) {
			if ((mech_sess->cipher = strdup(val)) == NULL)
				goto out;
			continue;
		}
		if (strcasecmp(opt, STRING_QOP)) {
			if (strcasecmp(val, QOP_AUTH_STR) == 0)
				mech_sess->qop = QOP_AUTH;
			else if (strcasecmp(val, QOP_AUTH_INT_STR) == 0)
				mech_sess->qop = QOP_AUTH_INT;
			else if (strcasecmp(val, QOP_AUTH_CONF_STR) == 0)
				mech_sess->qop = QOP_AUTH_CONF;
			else
				goto out;
			continue;
		}
		/* ignoring for now */
		if (strcasecmp(opt, "algorithm")) {
			if ( mech_sess->algorithm_s > 0 )
				goto out;
			mech_sess->algorithm_s = 1;
		}
		if (strcasecmp(opt, "maxbuf")) {
			if ( mech_sess->maxbuf_s > 0 )
				goto out;
			mech_sess->maxbuf_s = 1;
		}
	}

	if (mech_sess->nonce == NULL)
		goto out;

	rv = 0;
out:
	free(copy);
	return rv;
}

/**
 * @brief creates digestmd5 mechanism session.
 * Function initializes also default options for the session.
 * @param sess sasl session
 * @return 0 on success, -1 on failure.
 */

static int
saslc__mech_digestmd5_create(saslc_sess_t *sess)
{
	saslc__mech_digestmd5_sess_t *c;

	if ((sess->mech_sess = calloc(1, 
	    sizeof(saslc__mech_digestmd5_sess_t))) == NULL) {
		saslc__error_set(ERR(sess), ERROR_NOMEM, NULL);
		return -1;
	}

	c = sess->mech_sess;

	c->nonce_cnt = 1;
	c->encoding = "utf-8";

	return 0;
}

/**
 * @brief destroys digestmd5 mechanism session.
 * Function also is freeing assigned resources to the session.
 * @param sess sasl session
 * @return Functions always returns 0.
 */

static int
saslc__mech_digestmd5_destroy(saslc_sess_t *sess)
{
	saslc__mech_digestmd5_sess_t *c = sess->mech_sess;

	if (c->realm != NULL)
		free(c->realm);
	if (c->nonce != NULL)
		free(c->nonce);
	if (c->cnonce != NULL)
		free(c->cnonce);
	if (c->digesturi != NULL)
		free(c->digesturi);
	if (c->cipher != NULL)
		free(c->cipher);
	
	free(sess->mech_sess);
	sess->mech_sess = NULL;

	return 0;
}

/**
 * @brief doing one step of the sasl authentication
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out place to store output data
 * @param outlen output data length
 * @return MECH_OK - success,
 * MECH_STEP - more steps are needed,
 * MECH_ERROR - error
 */

static int
saslc__mech_digestmd5_cont(saslc_sess_t *sess, const void *in, size_t inlen,
    void **out, size_t *outlen)
{
	const char *service, *service_name, *realm, *authzid, *authid, *pass;
	saslc__mech_digestmd5_sess_t *mech_sess = sess->mech_sess;

	switch(mech_sess->mech_sess.step) {
	/* server is doing the first step, but some clients may call this
	 * function before getting data from the server */
	case 0:
		if (inlen == 0) {
			*out = NULL;
			*outlen = 0;
			return MECH_STEP;
		}
		/* if input data was provided, then doing the first step */
		mech_sess->mech_sess.step++;
		/*FALLTHROUGH*/
	case 1:
		/* parse challenge */
		saslc__mech_digestmd5_parse_challenge(mech_sess, in);

	       if ((service = saslc_sess_getprop(sess, 
		    SASLC_DIGEST_MD5_SERVICE)) == NULL) {
			saslc__error_set(ERR(sess), ERROR_MECH,
			    "service is required for an authentication");
			return MECH_ERROR;
		} 

		if ((realm = saslc_sess_getprop(sess, SASLC_DIGEST_MD5_REALM))
		    == NULL) {
			saslc__error_set(ERR(sess), ERROR_MECH,
			    "realm is required for an authentication");
			return MECH_ERROR;
		} 

		service_name = saslc_sess_getprop(sess,
		    SASLC_DIGEST_MD5_SERVICEN);

		mech_sess->digesturi = saslc__mech_digestmd5_digesturi(service,
		    service_name, realm);

		if ((authid = saslc_sess_getprop(sess,
		    SASLC_DIGEST_MD5_AUTHID)) == NULL) {
			saslc__error_set(ERR(sess), ERROR_MECH,
			    "authid is required for an authentication");
			return MECH_ERROR;
		}

		if ((authzid = saslc_sess_getprop(sess,
		    SASLC_DIGEST_MD5_AUTHZID)) == NULL) {
			saslc__error_set(ERR(sess), ERROR_MECH,
			    "authzid is required for an authentication");
			return MECH_ERROR;
		}

		if ((pass = saslc_sess_getprop(sess,
		    SASLC_DIGEST_MD5_AUTHZID)) == NULL) {
			saslc__error_set(ERR(sess), ERROR_MECH,
			    "password is required for an authentication");
			return MECH_ERROR;
		}

		mech_sess->cnonce = saslc__mech_digestmd5_nonce(NONCE_LEN); 
		if (mech_sess->cnonce == NULL)
			return MECH_ERROR;
 
		*out = saslc__mech_digestmd5_response(mech_sess, authid, pass,
		    authzid);
		*outlen = strlen(*out);

		return MECH_OK;
	default:
		assert(/*CONSTCOND*/0); /* impossible */
		return MECH_ERROR;
	}
}

/* mechanism definition */
const saslc__mech_t saslc__mech_digestmd5 = {
	"DIGEST-MD5", /* name */
	FLAG_MUTUAL | FLAG_DICTIONARY, /* flags */
	saslc__mech_digestmd5_create, /* create */
	saslc__mech_digestmd5_cont, /* step */
	NULL, /* encode */
	NULL, /* decode */
	saslc__mech_digestmd5_destroy /* destroy */
};
