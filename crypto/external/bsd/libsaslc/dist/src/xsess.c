/* $Id: xsess.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
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
#include <saslc.h>
#include <assert.h>
#include "dict.h"
#include "error.h"
#include "mech.h"
#include "saslc_private.h"

/* local headers */

static const saslc__mech_t *
saslc__sess_choose_mech(saslc_t *, const char *);

/**
 * @brief chooses best mechanism form the mechs list for
 * the sasl session.
 * @param ctx sasl context
 * @param mechs comma separated list of mechanisms eg. "PLAIN,LOGIN", note that
 * this function is not case sensitive
 * @return pointer to the mech on success, NULL if none mechanism was chose
 */

static const saslc__mech_t *
saslc__sess_choose_mech(saslc_t *ctx, const char *mechs)
{
	char *c, *e;
	const char *mech_name;
	const saslc__mech_list_node_t *m = NULL;

	if ((c = strdup(mechs)) == NULL) {
		saslc__error_set_errno(ERR(ctx), ERROR_NOMEM);
		return NULL;
	}

	mech_name = c;
	e = c;

	/* check if mechs parameter is a list of the mechanisms */
	if (strchr(e, ',') != NULL) {
		while ((e = strchr(e, ',')) != NULL) {
			*e = '\0';
			if ((m = saslc__mech_list_get(ctx->mechanisms,
			    mech_name)) != NULL)
				goto out;
			e++;
			mech_name = e;
		}
	}
	
	m = saslc__mech_list_get(ctx->mechanisms, mech_name);
out:
	free(c);
	return m != NULL ? m->mech : NULL;
}

/**
 * @brief sasl session initializaion. Function initializes session
 * property dictionary, chooses best mechanism, creates mech session.
 * @param ctx sasl context
 * @param mechs comma separated list of mechanisms eg. "PLAIN,LOGIN", note that
 * this function is not case sensitive
 * @return pointer to the sasl session on success, NULL on failure
 */

saslc_sess_t	*
saslc_sess_init(saslc_t *ctx, const char *mechs)
{
	saslc_sess_t *sess;

	if ((sess = calloc(1, sizeof(*sess))) == NULL) {
		saslc__error_set_errno(ERR(ctx), ERROR_NOMEM);
		return NULL;
	}

	/* mechanism initialization */
	if ((sess->mech = saslc__sess_choose_mech(ctx, mechs)) == NULL) {
		free(sess);
		saslc__error_set(ERR(ctx), ERROR_MECH,
		    "mechanism is not supported");
		return NULL;
	}

	/* create mechanism session */
	if (sess->mech->create(sess) < 0) {
		free(sess);
		return NULL;
	}

	/* properties */
	if ((sess->prop = saslc__dict_create()) == NULL) {
		free(sess);
		saslc__error_set(ERR(ctx), ERROR_NOMEM, NULL);
		return NULL;
	}

	sess->context = ctx;
	ctx->refcnt++;

	return sess;
}

/**
 * @brief ends sasl session, destroys and deallocates internal
 * resources
 * @param sess sasl session
 */

void
saslc_sess_end(saslc_sess_t *sess)
{
	sess->mech->destroy(sess);
	saslc__dict_destroy(sess->prop);
	sess->context->refcnt--;
	free(sess);
}

/**
 * @brief sets property for the session. If property already
 * exists in the session, then previous value is replaced by the new value.
 * @param sess sasl session
 * @param name property name
 * @param value proprty value
 * @return 0 on success, -1 on failure
 */

int
saslc_sess_setprop(saslc_sess_t *sess, const char *name, const char *value)
{
	/* check if key exists, if so then remove it from dictionary */

	if (saslc__dict_get(sess->prop, name) != NULL) {
		if (saslc__dict_remove(sess->prop, name) != DICT_OK)
			assert(/*CONSTCOND*/0);
	}

	switch (saslc__dict_insert(sess->prop, name, value)) {
	case DICT_VALBAD:
		saslc__error_set(ERR(sess), ERROR_BADARG, "bad value");
		return -1;
	case DICT_KEYINVALID:
		saslc__error_set(ERR(sess), ERROR_BADARG, "bad name");
		return -1;
	case DICT_NOMEM:
		saslc__error_set(ERR(sess), ERROR_NOMEM, NULL);
		return -1;
	case DICT_OK:
		break;
	case DICT_KEYEXISTS:
	default:
		assert(/*CONSTCOND*/0); /* impossible */
	}

	return 0;
}

/**
 * @brief gets property from the session. Dictionaries are used
 * in following order: session dictionary, context dictionary (global
 * configuration), mechanism dicionary.
 * @param sess sasl session
 * @param name property name
 * @return property value on success, NULL on failure.
 */

const char	*
saslc_sess_getprop(saslc_sess_t *sess, const char *name)
{
	const char *r;
	saslc__mech_list_node_t *m;

	/* get property from the session dictionary */
	if ((r = saslc__dict_get(sess->prop, name)) != NULL)
		return r;

	/* get property from the context dictionary */
	if ((r = saslc__dict_get(sess->context->prop, name)) != NULL)
		return r;
	
	/* get property form the mechanism dictionary */
	if ((m = saslc__mech_list_get(sess->context->mechanisms,
	    sess->mech->name)) == NULL)
		return NULL;

	return saslc__dict_get(m->prop, name);
}

/**
 * @brief do one step of the sasl authentication, input data
 * and its lenght are stored in in and inlen, output is stored in out and
 * outlen. This function is and wrapper for mechanism step functions.
 * Additionaly it checks if session is not already authorized and handles
 * steps mech_sess structure.
 * @param sess saslc session
 * @param in input data
 * @param inlen input data length
 * @param out output data
 * @param outlen output data length
 * @return MECH_OK - on success, no more steps are needed
 * MECH_ERROR - on error, additionaly errno in sess is setup
 * MECH_STEP - more steps are needed
 */

int
saslc_sess_cont(saslc_sess_t *sess, const void *in, size_t inlen, void **out,
	size_t *outlen)
{
	int r;
	saslc__mech_sess_t *ms = sess->mech_sess;

	if (ms->status == STATUS_AUTHENTICATED) {
		saslc__error_set(ERR(sess), ERROR_MECH,
		    "session authenticated");
		return MECH_ERROR;
	}

	r = sess->mech->cont(sess, in, inlen, out, outlen);

	if (r == MECH_OK)
		ms->status = STATUS_AUTHENTICATED;

	ms->step++;
	return r;
}

/**
 * @brief encodes data using method established during the
 * authentication. Input data is stored in in and inlen and output data
 * is stored in out and outlen.
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out output data
 * @param outlen output data length
 * @return 0 on success, -1 on failure
 */

int
saslc_sess_encode(saslc_sess_t *sess, const void *in, size_t inlen,
	void **out, size_t *outlen)
{
	if (sess->mech->encode == NULL) {
		saslc__error_set(ERR(sess), ERROR_MECH,
		    "security layer is not supported by mechnism");
		return -1;
	}

	return sess->mech->encode(sess, in, inlen, out, outlen);
}

/**
 * @brief decodes data using method established during the
 * authentication. Input data is stored in in and inlen and output data
 * is stored in out and outlen.
 * @param sess sasl session
 * @param in input data
 * @param inlen input data length
 * @param out output data
 * @param outlen output data length
 * @return 0 on success, -1 on failure
 */

int
saslc_sess_decode(saslc_sess_t *sess, const void *in, size_t inlen,
    void **out, size_t *outlen)
{
	if (sess->mech->decode == NULL) {
		saslc__error_set(ERR(sess), ERROR_MECH,
		    "security layer is not supported by mechnism");
		return -1;
	}

	return sess->mech->decode(sess, in, inlen, out, outlen);
}

/**
 * @brief gets string message of the error
 * @param sess sasl session
 * @return pointer to the error message
 */

const char *
saslc_sess_strerror(saslc_sess_t *sess)
{
	return saslc__error_get_strerror(ERR(sess));
}

/**
 * @brief gets name of the mechanism used in sasl session
 * @param sess sasl session
 * @return pointer to the mechanism name
 */

const char *
saslc_sess_strmech(saslc_sess_t *sess)
{
	return sess->mech->name;
}
