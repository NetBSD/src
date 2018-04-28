/* $NetBSD: clk.c,v 1.4 2018/04/28 15:20:33 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: clk.c,v 1.4 2018/04/28 15:20:33 jmcneill Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>

#include <dev/clk/clk.h>
#include <dev/clk/clk_backend.h>

static struct sysctllog *clk_log;
static const struct sysctlnode *clk_node;

static int
create_clk_node(void)
{
	const struct sysctlnode *hw_node;
	int error;

	if (clk_node)
		return 0;

	error = sysctl_createv(&clk_log, 0, NULL, &hw_node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (error)
		return error;

	error = sysctl_createv(&clk_log, 0, &hw_node, &clk_node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "clk", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return error;

	return 0;
}

static int
create_domain_node(struct clk_domain *domain)
{
	int error;

	if (domain->node)
		return 0;

	error = create_clk_node();
	if (error)
		return error;

	error = sysctl_createv(&clk_log, 0, &clk_node, &domain->node,
	    0, CTLTYPE_NODE, domain->name, NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		return error;

	return 0;
}

static int
clk_sysctl_rate_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct clk *clk;
	uint64_t rate;

	node = *rnode;
	clk = node.sysctl_data;
	node.sysctl_data = &rate;

	rate = clk_get_rate(clk);

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
clk_sysctl_parent_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct clk *clk, *clk_parent;

	node = *rnode;
	clk = node.sysctl_data;

	clk_parent = clk_get_parent(clk);
	if (clk_parent && clk_parent->name)
		node.sysctl_data = __UNCONST(clk_parent->name);
	else
		node.sysctl_data = __UNCONST("?");

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
clk_sysctl_parent_domain_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct clk *clk, *clk_parent;

	node = *rnode;
	clk = node.sysctl_data;

	clk_parent = clk_get_parent(clk);
	if (clk_parent && clk_parent->domain && clk_parent->domain->name)
		node.sysctl_data = __UNCONST(clk_parent->domain->name);
	else
		node.sysctl_data = __UNCONST("?");

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

int
clk_attach(struct clk *clk)
{
	const struct sysctlnode *node;
	struct clk_domain *domain = clk->domain;
	int error;

	KASSERT(domain != NULL);

	if (!domain->name || !clk->name) {
		/* Names are required to create sysctl nodes */
		return 0;
	}

	error = create_domain_node(domain);
	if (error != 0)
		goto sysctl_failed;

	error = sysctl_createv(&clk_log, 0, &domain->node, &node,
	    CTLFLAG_PRIVATE, CTLTYPE_NODE, clk->name, NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

	error = sysctl_createv(&clk_log, 0, &node, NULL,
	    CTLFLAG_PRIVATE, CTLTYPE_QUAD, "rate", NULL,
	    clk_sysctl_rate_helper, 0, (void *)clk, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

	error = sysctl_createv(&clk_log, 0, &node, NULL,
	    CTLFLAG_PRIVATE, CTLTYPE_STRING, "parent", NULL,
	    clk_sysctl_parent_helper, 0, (void *)clk, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

	error = sysctl_createv(&clk_log, 0, &node, NULL,
	    CTLFLAG_PRIVATE, CTLTYPE_STRING, "parent_domain", NULL,
	    clk_sysctl_parent_domain_helper, 0, (void *)clk, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

sysctl_failed:
	if (error)
		aprint_error("%s: failed to create sysctl node for %s: %d\n",
		    domain->name, clk->name, error);
	return error;
}

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
	if (clk->domain->funcs->get_parent)
		return clk->domain->funcs->get_parent(clk->domain->priv, clk);
	else
		return NULL;
}
