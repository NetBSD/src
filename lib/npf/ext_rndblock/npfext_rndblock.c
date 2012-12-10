/*	$NetBSD: npfext_rndblock.c,v 1.1 2012/12/10 00:32:24 rmind Exp $	*/

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
__RCSID("$NetBSD: npfext_rndblock.c,v 1.1 2012/12/10 00:32:24 rmind Exp $");

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <npf.h>

int		npfext_rndblock_init(void);
nl_ext_t *	npfext_rndblock_construct(const char *);
int		npfext_rndblock_param(nl_ext_t *, const char *, const char *);

int
npfext_rndblock_init(void)
{
	/* Nothing to initialise. */
	return 0;
}

nl_ext_t *
npfext_rndblock_construct(const char *name)
{
	assert(strcmp(name, "rndblock") == 0);
	return npf_ext_construct(name);
}

int
npfext_rndblock_param(nl_ext_t *ext, const char *param, const char *val)
{
	enum ptype { PARAM_U32 };
	static const struct param {
		const char *	name;
		enum ptype	type;
		signed long	min;
		signed long	max;
	} params[] = {
		{ "mod",	PARAM_U32,	1,	LONG_MAX	},
		{ "percentage",	PARAM_U32,	1,	9999		},
	};

	if (val == NULL) {
		return EINVAL;
	}
	for (unsigned i = 0; i < __arraycount(params); i++) {
		const char *name = params[i].name;
		long ival;

		if (strcmp(name, param) != 0) {
			continue;
		}

		/*
		 * Note: multiply by 100 and convert floating point to
		 * an integer, as 100% is based on 10000 in the kernel.
		 */
		ival = (i == 1) ? atof(val) * 100 : atol(val);
		if (ival < params[i].min || ival > params[i].max) {
			return EINVAL;
		}
		assert(params[i].type == PARAM_U32);
		npf_ext_param_u32(ext, name, ival);
		return 0;
	}

	/* Invalid parameter, if not found. */
	return EINVAL;
}
