/* $Id: mech.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

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
#include <stdlib.h>
#include <string.h>
#include <saslc.h>
#include <sys/queue.h>
#include "saslc_private.h"
#include "dict.h"
#include "mech.h"
#include "error.h"

/* local headers */

/* mechanisms */
extern const saslc__mech_t saslc__mech_anonymous;
extern const saslc__mech_t saslc__mech_crammd5;
extern const saslc__mech_t saslc__mech_digestmd5;
extern const saslc__mech_t saslc__mech_external;
extern const saslc__mech_t saslc__mech_gssapi;
extern const saslc__mech_t saslc__mech_login;
extern const saslc__mech_t saslc__mech_plain;

const saslc__mech_t *saslc__mechanisms[] = {
	&saslc__mech_anonymous,
	&saslc__mech_crammd5,
	&saslc__mech_digestmd5,
	&saslc__mech_external,
	&saslc__mech_gssapi,
	&saslc__mech_login,
	&saslc__mech_plain,
	NULL
};

/**
 * @brief creates lists of the mechanisms for the context,
 * function only creates list, it means that it's not assigning mechanism_list
 * with the context.
 * @param ctx context
 * @return pointer to head od the list, NULL if allocation failed
 */

saslc__mech_list_t	*
saslc__mech_list_create(saslc_t *ctx)
{
	saslc__mech_list_t *head = NULL;
	saslc__mech_list_node_t *node = NULL;
	size_t i;

	if ((head = calloc(1, sizeof(*head))) == NULL) {
		saslc__error_set_errno(ERR(ctx), ERROR_NOMEM);
		return NULL;
	}

	for (i = 0; saslc__mechanisms[i] != NULL; i++) {	
		if ((node = calloc(1, sizeof(*node))) == NULL)
			goto error;

		if ((node->prop = saslc__dict_create()) == NULL) {
			free(node);
			goto error;
		}

		node->mech = saslc__mechanisms[i];

		LIST_INSERT_HEAD(head, node, nodes);
	}

	return head;

error:
	saslc__error_set_errno(ERR(ctx), ERROR_NOMEM);
	saslc__mech_list_destroy(head);
	return NULL;
}

/**
 * @brief gets mechanism from the list using name
 * @param list mechanisms list
 * @param mech_name mechanism name
 * @return pointer to the mechanism, NULL if mechanism was not found
 */

saslc__mech_list_node_t *
saslc__mech_list_get(saslc__mech_list_t *list, const char *mech_name)
{
	saslc__mech_list_node_t *node;

	for (node = list->lh_first; node != NULL; node = node->nodes.le_next)
		if (strcasecmp(node->mech->name, mech_name) == 0)
			return node;

	return NULL;
}

/**
 * @brief destroys and deallocates mechanism list
 * @param list mechanisms list
 */

void
saslc__mech_list_destroy(saslc__mech_list_t *list)
{
	saslc__mech_list_node_t *node;

	while((node = list->lh_first) != NULL) {	
		if (node != NULL)
			saslc__dict_destroy(node->prop);
		LIST_REMOVE(node, nodes);
		free(node);
	}

	free(list);
}

/**
 * @brief doing copy of the session property, on error sets 
 * error message for the session. Copied data is located in *out and *outlen
 * @param sess sasl session
 * @param out out buffer for the session property copy
 * @param outlen length of the session property copy
 * @param name name of the property
 * @param error_msg error messages set on failure
 * @return MECH_OK - on success
 * MECH_ERROR - on failure
 */

int
saslc__mech_strdup(saslc_sess_t *sess, char **out, size_t *outlen,
    const char *name, const char *error_msg)
{
	const char *value; /* property value */

	/* get value */
	if ((value = saslc_sess_getprop(sess, name)) == NULL) {
		saslc__error_set(ERR(sess), ERROR_MECH, error_msg);
		return MECH_ERROR;
	}

	/* copy value */
	if ((*out = strdup(value)) == NULL) {
		saslc__error_set_errno(ERR(sess), ERROR_NOMEM);
		return MECH_ERROR;
	}

	if (outlen != NULL)
		*outlen = saslc__dict_get_len(sess->prop, name);

	return MECH_OK;
}

/**
 * @brief generic session create function, this
 * function is suitable for the most mechanisms.
 * @return 0 on success, -1 on failure
 */

int
saslc__mech_generic_create(saslc_sess_t *sess)
{
	if ((sess->mech_sess = calloc(1, sizeof(saslc__mech_sess_t))) == NULL) {
		saslc__error_set(ERR(sess), ERROR_NOMEM, NULL);
		return -1;
	}

	return 0;
}

/**
 * @brief generic session destroy function, this
 * function is suitable for the most mechanisms.
 * @return function always returns 0
 */

int
saslc__mech_generic_destroy(saslc_sess_t *sess)
{
	free(sess->mech_sess);
	sess->mech_sess = NULL;

	return 0;
}
