/*	$NetBSD: npfext_normalize.c,v 1.1 2013/03/10 21:49:26 christos Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: npfext_normalize.c,v 1.1 2013/03/10 21:49:26 christos Exp $");

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <npf.h>

int		npfext_normalize_init(void);
nl_ext_t *	npfext_normalize_construct(const char *);
int		npfext_normalize_param(nl_ext_t *, const char *, const char *);

int
npfext_normalize_init(void)
{
	/* Nothing to initialize. */
	return 0;
}

nl_ext_t *
npfext_normalize_construct(const char *name)
{
	assert(strcmp(name, "normalize") == 0);
	return npf_ext_construct(name);
}

int
npfext_normalize_param(nl_ext_t *ext, const char *param, const char *val)
{
	enum ptype {
		PARAM_BOOL,
		PARAM_U32
	};
	static const struct param {
		const char *	name;
		enum ptype	type;
		bool		reqval;
	} params[] = {
		{ "random-id",	PARAM_BOOL,	false	},
		{ "no-df",	PARAM_BOOL,	false	},
		{ "min-ttl",	PARAM_U32,	true	},
		{ "max-mss",	PARAM_U32,	true	},
	};

	for (unsigned i = 0; i < __arraycount(params); i++) {
		const char *name = params[i].name;

		if (strcmp(name, param) != 0) {
			continue;
		}
		if (val == NULL && params[i].reqval) {
			return EINVAL;
		}

		switch (params[i].type) {
		case PARAM_BOOL:
			npf_ext_param_bool(ext, name, true);
			break;
		case PARAM_U32:
			npf_ext_param_u32(ext, name, atol(val));
			break;
		default:
			assert(false);
		}
		return 0;
	}

	/* Invalid parameter, if not found. */
	return EINVAL;
}
