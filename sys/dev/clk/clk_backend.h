/* $NetBSD: clk_backend.h,v 1.1.2.3 2017/08/28 17:52:01 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_CLK_CLK_BACKEND_H
#define _DEV_CLK_CLK_BACKEND_H

#include <dev/clk/clk.h>

struct clk_domain {
	const struct clk_funcs *funcs;
	void *priv;
};

struct clk {
	struct clk_domain *domain;
        const char *name;
        u_int flags;
#define CLK_SET_RATE_PARENT     0x01
};

struct clk_funcs {
	struct clk *(*get)(void *, const char *);
	void (*put)(void *, struct clk *);

	u_int (*get_rate)(void *, struct clk *);
	int (*set_rate)(void *, struct clk *, u_int);
	int (*enable)(void *, struct clk *);
	int (*disable)(void *, struct clk *);
	int (*set_parent)(void *, struct clk *, struct clk *);
	struct clk *(*get_parent)(void *, struct clk *);
};

#endif /* _DEV_CLK_CLK_BACKEND_H */
