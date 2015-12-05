/* $NetBSD: clk.c,v 1.1 2015/12/05 13:31:07 jmcneill Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clk.c,v 1.1 2015/12/05 13:31:07 jmcneill Exp $");

#include <sys/param.h>

#include <dev/clk/clk.h>
#include <dev/clk/clk_backend.h>

static const char *clk_backend = NULL;
static const struct clk_funcs *clk_funcs = NULL;
static void *clk_priv = NULL;

int
clk_backend_register(const char *name, const struct clk_funcs *funcs, void *priv)
{
	KASSERT(clk_funcs == NULL);

	clk_backend = name;
	clk_funcs = funcs;
	clk_priv = priv;

	return 0;
}

struct clk *
clk_get(const char *name)
{
	if (clk_funcs == NULL)
		return NULL;
	return clk_funcs->get(clk_priv, name);
}

void
clk_put(struct clk *clk)
{
	return clk_funcs->put(clk_priv, clk);
}

u_int
clk_get_rate(struct clk *clk)
{
	return clk_funcs->get_rate(clk_priv, clk);
}

int
clk_set_rate(struct clk *clk, u_int rate)
{
	if (clk->flags & CLK_SET_RATE_PARENT) {
		return clk_set_rate(clk_get_parent(clk), rate);
	} else {
		return clk_funcs->set_rate(clk_priv, clk, rate);
	}
}

int
clk_enable(struct clk *clk)
{
	return clk_funcs->enable(clk_priv, clk);
}

int
clk_disable(struct clk *clk)
{
	return clk_funcs->disable(clk_priv, clk);
}

int
clk_set_parent(struct clk *clk, struct clk *parent_clk)
{
	return clk_funcs->set_parent(clk_priv, clk, parent_clk);
}

struct clk *
clk_get_parent(struct clk *clk)
{
	return clk_funcs->get_parent(clk_priv, clk);
}
