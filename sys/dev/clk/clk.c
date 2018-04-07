/* $NetBSD: clk.c,v 1.2.12.1 2018/04/07 04:12:14 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: clk.c,v 1.2.12.1 2018/04/07 04:12:14 pgoyette Exp $");

#include <sys/param.h>

#include <dev/clk/clk.h>
#include <dev/clk/clk_backend.h>

struct clk *
clk_get(struct clk_domain *domain, const char *name)
{
	return domain->funcs->get(domain->priv, name);
}

void
clk_put(struct clk *clk)
{
	return clk->domain->funcs->put(clk->domain->priv, clk);
}

u_int
clk_get_rate(struct clk *clk)
{
	return clk->domain->funcs->get_rate(clk->domain->priv, clk);
}

int
clk_set_rate(struct clk *clk, u_int rate)
{
	if (clk->flags & CLK_SET_RATE_PARENT) {
		return clk_set_rate(clk_get_parent(clk), rate);
	} else if (clk->domain->funcs->set_rate) {
		return clk->domain->funcs->set_rate(clk->domain->priv,
		    clk, rate);
	} else {
		return EINVAL;
	}
}

u_int
clk_round_rate(struct clk *clk, u_int rate)
{
	if (clk->domain->funcs->round_rate) {
		return clk->domain->funcs->round_rate(clk->domain->priv,
		    clk, rate);
	}
	return 0;
}

int
clk_enable(struct clk *clk)
{
	if (clk->domain->funcs->enable)
		return clk->domain->funcs->enable(clk->domain->priv, clk);
	else
		return 0;
}

int
clk_disable(struct clk *clk)
{
	if (clk->domain->funcs->disable)
		return clk->domain->funcs->disable(clk->domain->priv, clk);
	else
		return EINVAL;
}

int
clk_set_parent(struct clk *clk, struct clk *parent_clk)
{
	if (clk->domain->funcs->set_parent)
		return clk->domain->funcs->set_parent(clk->domain->priv,
		    clk, parent_clk);
	else
		return EINVAL;
}

struct clk *
clk_get_parent(struct clk *clk)
{
	return clk->domain->funcs->get_parent(clk->domain->priv, clk);
}
