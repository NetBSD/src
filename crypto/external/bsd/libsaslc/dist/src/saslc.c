/* $Id: saslc.c,v 1.1.1.1.2.1 2011/02/08 16:18:31 bouyer Exp $ */

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
 * PURPOSE ARE DISCLAIMED.      IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
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
#include <assert.h>
#include "dict.h"
#include "parser.h"
#include "saslc_private.h"
#include "mech.h"
#include "error.h"

/* local headers */

static bool saslc__valid_appname(const char *);

/**
 * @brief checks if application name is valid
 * @param appname application name
 * @return true if application name is valid, false otherwise
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
 * @return saslc context
 */

saslc_t * 
saslc_alloc(void)
{
        return calloc(1, sizeof(saslc_t));
}

/**
 * @brief initializes sasl context, basing on application name function
 * parses configuration files, sets up default peroperties and creates
 * mechanisms list for the context.
 * @param ctx sasl context
 * @param appname application name, NULL could be used for generic aplication
 * @return 0 on success, -1 otherwise.
 */

int
saslc_init(saslc_t *ctx, const char *appname)
{
        memset(ctx, 0, sizeof(*ctx));

        LIST_INIT(&ctx->sessions);

        ctx->prop = saslc__dict_create();

        if (ctx->prop == NULL)
                return -1;

        /* appname */
        if (appname != NULL) { 
                /* check if appname is valid */
                if (saslc__valid_appname(appname) == false) {
                        saslc__error_set(ERR(ctx), ERROR_BADARG,
                            "application name is not permited");
                        goto error;
                }

                ctx->appname = strdup(appname);
                if (ctx->appname == NULL) {
                        saslc__error_set_errno(ERR(ctx), ERROR_NOMEM);
                        goto error;
                }
        } else
                ctx->appname = NULL;

        /* mechanisms list */
        if (saslc__mech_list_create(ctx) == -1)
                goto error;

        /* parse configuration files */
        if (saslc__parser_config(ctx) == -1)
                /* errno is set up by parser */
                goto error;

        return 0;

error:
        if (ctx->appname != NULL)
                free((void *)ctx->appname);
        if (ctx->prop != NULL)
                saslc__dict_destroy(ctx->prop);

        ctx->appname = NULL;
        ctx->prop = NULL;

        return -1;
}

/**
 * @brief gets string message of last error.
 * @param ctx context
 * @return error string
 */

const char *
saslc_strerror(saslc_t *ctx)
{
        return saslc__error_get_strerror(ERR(ctx));
}

/**
 * @brief destroys and deallocate resources used by the context.
 * @param ctx context
 * @param destroy_sessions indicates if all existing sessions assigned to
 * the context (if any) should be destroyed
 * @return 0 on success, -1 on failure
 */

int
saslc_end(saslc_t *ctx, bool destroy_sessions)
{
        /* check if there're any assigned sessions */
        if (!LIST_EMPTY(&ctx->sessions) && destroy_sessions == false) {
                saslc__error_set(ERR(ctx), ERROR_GENERAL,
                    "context has got assigned active sessions");
                return -1;
        }

        /* destroy all assigned sessions (note that if any nodes are assigned,
         * then destroy_sessions == true) */
        assert(LIST_EMPTY(&ctx->sessions) || destroy_sessions == true);
        while (!LIST_EMPTY(&ctx->sessions))
                saslc_sess_end(LIST_FIRST(&ctx->sessions));

        /* mechanism list */
        if (!LIST_EMPTY(&ctx->mechanisms))
                saslc__mech_list_destroy(&ctx->mechanisms);

        /* properties */
        if (ctx->prop != NULL)
                saslc__dict_destroy(ctx->prop);

        /* application name */
        free(ctx->appname);

        /* free context */
        free(ctx);

        return 0;
}
