/* $Id: saslc.c,v 1.1.1.1 2010/11/27 21:23:59 agc Exp $ */

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

#include <saslc.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "dict.h"
#include "parser.h"
#include "saslc_private.h"
#include "mech.h"
#include "error.h"

/* local headers */

static bool saslc__valid_appname(const char *);

/**
 * @brief checks if application name is legal
 * @param appname application name
 * @return true if application name is legal, false otherwise
 */

static bool
saslc__valid_appname(const char *appname)
{
	const char *p;

	for (p = appname; *p; p++)
		if (!isalnum((unsigned char)*p))
			return false;

	return true;
}


/**
 * @brief allocates new saslc context
 * @return pointer to the saslc context
 */

saslc_t * 
saslc_alloc(void)
{
	return calloc(1, sizeof(saslc_t));
}

/**
 * @brief initializes sasl context, basing on application name function
 * parses configuration files, sets up default peroperties, creates
 * mechanisms list for the context.
 * @param ctx sasl context
 * @param appname application name, NULL could be used for generic aplication
 * @return 0 on success, -1 otherwise.
 */

int
saslc_init(saslc_t *ctx, const char *appname)
{

	/* reference counter */
	ctx->refcnt = 0;
	ctx->prop = saslc__dict_create();

	/* appname */
	if (appname != NULL) { 
		/* check if appname is valid */
		if (saslc__valid_appname(appname) == false) {
			saslc__error_set(ERR(ctx), ERROR_BADARG,
			    "application name is not permited");
			free(ctx->prop);
			ctx->prop = NULL;
			return -1;
		}

		if ((ctx->appname = strdup(appname)) == NULL) {
			saslc__error_set_errno(ERR(ctx), ERROR_NOMEM);
			free(ctx->prop);
			ctx->prop = NULL;
			return -1;
		}
	} else
		ctx->appname = NULL;

	/* mechanisms list */
	ctx->mechanisms = saslc__mech_list_create(ctx);

	if (ctx->mechanisms == NULL)
		return -1;

	/* @param configuration default properties */
	if (saslc__parser_config(ctx) < 0) {
		free((void *)(intptr_t)ctx->appname);
		ctx->appname = NULL;
		saslc__dict_destroy(ctx->prop);
		ctx->prop = NULL;
		saslc__mech_list_destroy(ctx->mechanisms);
		ctx->mechanisms = NULL;
		return -1;
	}

	return 0;
}

/**
 * @brief gets string message of the error.
 * @param ctx context
 * @return pointer to the error message.
 */

const char *
saslc_strerror(saslc_t *ctx)
{
	return saslc__error_get_strerror(ERR(ctx));
}

/**
 * @brief destroys and deallocate resources used by the context.
 * Context shouldn't have got any sessions assigned to it.
 * @param ctx context
 * @return 0 on success, -1 on failure.
 */

int
saslc_end(saslc_t *ctx)
{
	if (ctx->refcnt > 0) {
		saslc__error_set(ERR(ctx), ERROR_GENERAL,
		    "context has got assigned active sessions");
		return -1;
	}

	/* mechanism list */
	if (ctx->mechanisms != NULL)
		saslc__mech_list_destroy(ctx->mechanisms);

	/* properties */
	if (ctx->prop != NULL)
		saslc__dict_destroy(ctx->prop);

	/* application name */
	if (ctx->appname != NULL)
		free((void *)(intptr_t)ctx->appname);

	/* free context */
	free(ctx);

	return 0;
}
