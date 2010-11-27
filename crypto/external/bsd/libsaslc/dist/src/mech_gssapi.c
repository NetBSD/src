/* $Id: mech_gssapi.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

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

#include <stdio.h>
#include <string.h>
#include <gssapi/gssapi.h>
#include <assert.h>
#include <saslc.h>
#include "saslc_private.h"
#include "mech.h"

/* RFC2222 implementation
 * see: RFC2222 7.2.1 section
 */

/* local headers */

/* properties */
#define SASLC_GSSAPI_AUTHID	"AUTHID"
#define SASLC_GSSAPI_HOSTNAME	"HOSTNAME"
#define SASLC_GSSAPI_SERVICE	"SERVICE"

/* status */
enum {
	GSSAPI_INIT, /* initialization see: RFC2222 7.2.1 section */
	GSSAPI_UNWRAP, /* unwrap see: RFC2222 7.2.1 section */
	GSSAPI_AUTHENTICATED /* authenticated see: RFC2222 7.2.3 section */
};

typedef enum {
	LAYER_NONE = 1, /* no security layer */
	LAYER_INT = 2, /* integrity */
	LAYER_CONF = 4 /* privacy */
} saslc__mech_gssapi_sl_t;

/** gssapi mechanism session */
typedef struct {
	saslc__mech_sess_t mech_sess; /**< mechanism session */
	/* additional stuff */
	int status; /**< authentication status */
	gss_name_t name; /**< service\@hostname */
	gss_ctx_id_t context; /**< GSSAPI context */
	saslc__mech_gssapi_sl_t layer; /**< security layer */
} saslc__mech_gssapi_sess_t;

static int saslc__mech_gssapi_create(saslc_sess_t *);
static int saslc__mech_gssapi_destroy(saslc_sess_t *);
static int saslc__mech_gssapi_encode(saslc_sess_t *, const void *, size_t,
    void **, size_t *);
static int saslc__mech_gssapi_decode(saslc_sess_t *, const void *, size_t,
    void **, size_t *);
static int saslc__mech_gssapi_cont(saslc_sess_t *, const void *, size_t,
    void **, size_t *);

/**
 * @brief creates gssapi mechanism session.
 * Function initializes also default options for the session.
 * @param sess sasl session
 * @return 0 on success, -1 on failure.
 */

static int
saslc__mech_gssapi_create(saslc_sess_t *sess)
{
	saslc__mech_gssapi_sess_t *c;

	c = sess->mech_sess = calloc(1, sizeof(*c));
	if (c == NULL)
		return -1;

	sess->mech_sess = c;

	/* GSSAPI init stuff */
	c->context = GSS_C_NO_CONTEXT;
	c->name = GSS_C_NO_NAME;
	c->layer = LAYER_NONE;

	return 0;
}

/**
 * @brief destroys gssapi mechanism session.
 * Function also is freeing assigned resources to the session.
 * @param sess sasl session
 * @return Functions always returns 0.
 */

static int
saslc__mech_gssapi_destroy(saslc_sess_t *sess)
{
	saslc__mech_gssapi_sess_t *mech_sess = sess->mech_sess;
	OM_uint32 min_s;
	
	gss_release_name(&min_s, &mech_sess->name);

	free(sess->mech_sess);
	sess->mech_sess = NULL;

	return 0;
}


/**
 * @brief encodes data using negotiated security layer
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out place to store output data
 * @param outlen output data length
 * @return MECH_OK - success,
 * MECH_ERROR - error
 */

static int
saslc__mech_gssapi_encode(saslc_sess_t *sess, const void *in, size_t inlen,
    void **out, size_t *outlen)
{
	saslc__mech_gssapi_sess_t *mech_sess = sess->mech_sess;
	gss_buffer_desc input, output;	  
	OM_uint32 min_s, maj_s;

	if (mech_sess->status != GSSAPI_AUTHENTICATED)
		return MECH_ERROR;

	/* No layer was negotiated - just copy data */
	if (mech_sess->layer == LAYER_NONE) {
		*outlen = inlen;
		*out = calloc(*outlen, sizeof(char));
		memcpy(*out, in, *outlen);
		return MECH_OK;
	}

	input.value = (char *)(intptr_t)in;
	input.length = inlen;

	maj_s = gss_wrap(&min_s, mech_sess->context, 
	    mech_sess->layer, GSS_C_QOP_DEFAULT, &input, NULL,
	    &output);

	if (GSS_ERROR(maj_s))
		return MECH_ERROR;

	*outlen = output.length;
	if (*outlen > 0) {
		*out = calloc(*outlen, sizeof(char));
		if (*out == NULL) {
			saslc__error_set_errno(ERR(sess),
			    ERROR_NOMEM);
			return MECH_ERROR;
		}
		memcpy(*out, output.value, *outlen);
		gss_release_buffer(&min_s, &output);
	} else
		*out = NULL;

	return MECH_OK;
 }

/**
 * @brief decodes data using negotiated security layer
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out place to store output data
 * @param outlen output data length
 * @return MECH_OK - success,
 * MECH_ERROR - error
 */
	
static int
saslc__mech_gssapi_decode(saslc_sess_t *sess, const void *in, size_t inlen,
	void **out, size_t *outlen)
{
	saslc__mech_gssapi_sess_t *mech_sess = sess->mech_sess;
	gss_buffer_desc input, output;	  
	OM_uint32 min_s, maj_s;

	if (mech_sess->status != GSSAPI_AUTHENTICATED)
		return MECH_ERROR;

	/* No layer was negotiated - just copy data */
	if (mech_sess->layer == LAYER_NONE) {
		*outlen = inlen;
		*out = calloc(*outlen, sizeof(char));
		memcpy(*out, in, *outlen);
		return MECH_OK;
	}

	input.value = (char *)(intptr_t)in;
	input.length = inlen;

	maj_s = gss_unwrap(&min_s, mech_sess->context, &input, &output, NULL, 
	    NULL);

	if (GSS_ERROR(maj_s))
		return MECH_ERROR;

	*outlen = output.length;
	if (*outlen > 0) {
		*out = calloc(*outlen, sizeof(char));
		if (*out == NULL) {
			saslc__error_set_errno(ERR(sess),
			    ERROR_NOMEM);
			return MECH_ERROR;
		}
		memcpy(*out, output.value, *outlen);
		gss_release_buffer(&min_s, &output);
	} else
		*out = NULL;

	return MECH_OK;
}

/**
 * @brief doing one step of the sasl authentication
 *
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out place to store output data
 * @param outlen output data length
 * @return MECH_OK - success,
 * MECH_STEP - more steps are needed,
 * MECH_ERROR - error
 */

/*ARGSUSED*/
static int
saslc__mech_gssapi_cont(saslc_sess_t *sess, const void *in, size_t inlen,
    void **out, size_t *outlen)
{
	gss_buffer_t input_buf;
	gss_buffer_desc input, output, name;
	const char *hostname, *service, *authid;
	char *input_name;
	saslc__mech_gssapi_sess_t *mech_sess = sess->mech_sess;
	OM_uint32 min_s, maj_s;
	int len;

	switch(mech_sess->status) {
	case GSSAPI_INIT:
		/* setup name at the very begining */
		if (mech_sess->name == GSS_C_NO_NAME) {
			/* gss_import_name */
			if ((hostname = saslc_sess_getprop(sess,
			    SASLC_GSSAPI_HOSTNAME)) == NULL) {
				saslc__error_set(ERR(sess), ERROR_MECH,
				    "hostname is required for an "
				    "authentication");
				return MECH_ERROR;
			}
	
			if ((service = saslc_sess_getprop(sess,
				SASLC_GSSAPI_SERVICE)) == NULL) {
				saslc__error_set(ERR(sess), ERROR_MECH,
				    "service is required for an "
				    "authentication");
				return MECH_ERROR;
			}

			len = asprintf(&input_name, "%s@%s", service, hostname);
			if (len == -1) {
				saslc__error_set_errno(ERR(sess), ERROR_NOMEM);
				return MECH_ERROR;
			}
			name.value = input_name;
			name.length = len + 1;

			maj_s = gss_import_name(&min_s, &name,
			    GSS_C_NT_HOSTBASED_SERVICE, &mech_sess->name);

			free(input_name);

			if (GSS_ERROR(maj_s))
				return MECH_ERROR;

		}

		/* input data is passed */
		if (in != NULL && inlen > 0) {
			input.value = (char *)(intptr_t)in;
			input.length = inlen;
			input_buf = &input;
		} else {
			input_buf = GSS_C_NO_BUFFER;
		}

		maj_s = gss_init_sec_context(&min_s, GSS_C_NO_CREDENTIAL,
		    &mech_sess->context, mech_sess->name, GSS_C_NO_OID,
		    GSS_C_MUTUAL_FLAG | GSS_C_SEQUENCE_FLAG, 0,
		    GSS_C_NO_CHANNEL_BINDINGS, input_buf, NULL, &output, NULL,
		    NULL);

		switch(maj_s) {
		case GSS_S_COMPLETE:
			/* init complete, now do unwrap */
			mech_sess->status = GSSAPI_UNWRAP;
			/*FALLTHROUGH*/
		case GSS_S_CONTINUE_NEEDED:
			*outlen = output.length;
			if (*outlen > 0) {
				*out = calloc(*outlen, sizeof(char));
				if (*out == NULL) {
					saslc__error_set_errno(ERR(sess),
					    ERROR_NOMEM);
					return MECH_ERROR;
				}
				memcpy(*out, output.value, *outlen);
				gss_release_buffer(&min_s, &output);
			} else
				*out = NULL;
			return MECH_STEP;
		default:
			/* error occured */
			return MECH_ERROR;
		}
		
	case GSSAPI_UNWRAP:
		input.value = (char *)(intptr_t)in;
		input.length = inlen;
		
		maj_s = gss_unwrap(&min_s, mech_sess->context, &input,
		    &output, NULL, NULL);

		if ((authid = saslc_sess_getprop(sess, SASLC_GSSAPI_AUTHID))
			== NULL) {
			saslc__error_set(ERR(sess), ERROR_MECH,
			    "authid is required for an authentication");
			return MECH_ERROR;
		}

		len = asprintf(&input_name, "%c%c%c%c%s", 0, 0, 0, 0, authid);
		if (len == -1) {
			saslc__error_set_errno(ERR(sess), ERROR_NOMEM);
			return MECH_ERROR;
		}
		name.value = input_name;
		input.length = len + 1;
		gss_release_buffer(&min_s, &output);
		
		maj_s = gss_wrap(&min_s, mech_sess->context, 
		    0 /* FALSE - RFC2222 */, GSS_C_QOP_DEFAULT, &input, NULL,
		    &output);

		free(input.value);

		if (GSS_ERROR(maj_s))
			return MECH_ERROR;

		*outlen = output.length;
		if (*outlen > 0) {
			*out = calloc(*outlen, sizeof(char));
			if (*out == NULL) {
				saslc__error_set_errno(ERR(sess),
				    ERROR_NOMEM);
				return MECH_ERROR;
			}
			memcpy(*out, output.value, *outlen);
			gss_release_buffer(&min_s, &output);
		} else
			*out = NULL;
			
		mech_sess->status = GSSAPI_AUTHENTICATED;
		return MECH_OK;
	default:
		assert(/*CONSTCOND*/0); /* impossible */
		/* NOTREACHED */
		return MECH_ERROR;
	}
}

/* mechanism definition */
const saslc__mech_t saslc__mech_gssapi = {
	"GSSAPI", /* name */
	FLAG_NONE, /* flags */
	saslc__mech_gssapi_create, /* create */
	saslc__mech_gssapi_cont, /* step */
	saslc__mech_gssapi_encode, /* encode */
	saslc__mech_gssapi_decode, /* decode */
	saslc__mech_gssapi_destroy /* destroy */
};
