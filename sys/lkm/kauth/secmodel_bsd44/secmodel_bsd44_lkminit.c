/* $NetBSD: secmodel_bsd44_lkminit.c,v 1.1.22.1 2008/01/09 01:56:55 matt Exp $ */

/*-
 * Copyright (c) 2007 Elad Efrat <elad@NetBSD.org>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: secmodel_bsd44_lkminit.c,v 1.1.22.1 2008/01/09 01:56:55 matt Exp $");

#include <sys/param.h>
#include <sys/lkm.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>

#include <secmodel/bsd44/bsd44.h>
#include <secmodel/bsd44/suser.h>
#include <secmodel/securelevel/securelevel.h>

int secmodel_bsd44_lkm_lkmentry(struct lkm_table *lkmtp, int, int);
static int secmodel_bsd44_lkm_load(struct lkm_table *, int);
static int secmodel_bsd44_lkm_unload(struct lkm_table *, int);

MOD_MISC("secmodel_bsd44_lkm");

static struct sysctllog *_secmodel_bsd44_lkm_log;

static int
secmodel_bsd44_lkm_load(struct lkm_table *lkmtp, int cmd)
{
	if(lkmexists(lkmtp))
		return (EEXIST);

	secmodel_bsd44_start();
	sysctl_security_bsd44_setup(&_secmodel_bsd44_lkm_log);
	sysctl_security_securelevel_setup(&_secmodel_bsd44_lkm_log);

	return (0);
}

static int
secmodel_bsd44_lkm_unload(struct lkm_table *lkmtp, int cmd)
{
	secmodel_bsd44_stop();
	sysctl_teardown(&_secmodel_bsd44_lkm_log);

	/* XXX synchronize listener exit... */

	return (0);
}

int
secmodel_bsd44_lkm_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, secmodel_bsd44_lkm_load,
	    secmodel_bsd44_lkm_unload, lkm_nofunc)
}
