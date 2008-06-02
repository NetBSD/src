/*	$NetBSD: pf_lkm.c,v 1.3.70.1 2008/06/02 13:24:18 mjf Exp $	*/

/*
 *  Copyright (c) 2004 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *  by Peter Postma and Joel Wilsson.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pf_lkm.c,v 1.3.70.1 2008/06/02 13:24:18 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/lkm.h>

#include <net/if.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

int		pf_lkmentry(struct lkm_table *, int, int);
static int	pf_lkmload(struct lkm_table *, int);
static int	pf_lkmunload(struct lkm_table *, int);

extern void	pfattach(int);
extern void	pfdetach(void);
extern void	pflogattach(int);
extern void	pflogdetach(void);

extern const struct cdevsw pf_cdevsw;

MOD_DEV("pf", "pf", NULL, -1, &pf_cdevsw, -1);

int
pf_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	LKM_DISPATCH(lkmtp, cmd, ver, pf_lkmload, pf_lkmunload, lkm_nofunc);
}

static int
pf_lkmload(struct lkm_table *lkmtp, int cmd)
{
	if (lkmexists(lkmtp))
		return (EEXIST);

	pfattach(1);
	pflogattach(1);

	return (0);
}

static int
pf_lkmunload(struct lkm_table *lkmtp, int cmd)
{
	pfdetach();
	pflogdetach();

	return (0);
}
