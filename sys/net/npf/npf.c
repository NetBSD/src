/*	$NetBSD: npf.c,v 1.1 2010/08/22 18:56:22 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

/*
 * NPF main: dynamic load/initialisation and unload routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.1 2010/08/22 18:56:22 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/socketvar.h>
#include <sys/uio.h>

#include "npf_impl.h"

/*
 * Module and device structures.
 */
MODULE(MODULE_CLASS_MISC, npf, NULL);

void		npfattach(int);

static int	npf_dev_open(dev_t, int, int, lwp_t *);
static int	npf_dev_close(dev_t, int, int, lwp_t *);
static int	npf_dev_ioctl(dev_t, u_long, void *, int, lwp_t *);
static int	npf_dev_poll(dev_t, int, lwp_t *);
static int	npf_dev_read(dev_t, struct uio *, int);

const struct cdevsw npf_cdevsw = {
	npf_dev_open, npf_dev_close, npf_dev_read, nowrite, npf_dev_ioctl,
	nostop, notty, npf_dev_poll, nommap, nokqfilter, D_OTHER | D_MPSAFE
};

static int
npf_init(void)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
#endif
	int error;

	/*
	 * Initialise ruleset, tables and session structures.
	 */

	error = npf_ruleset_sysinit();
	if (error)
		return error;

	error = npf_tableset_sysinit();
	if (error) {
		npf_ruleset_sysfini();
		return error;
	}

	error = npf_session_sysinit();
	if (error) {
		npf_tableset_sysfini();
		npf_ruleset_sysfini();
		return error;
	}
	npf_nat_sysinit();
	npf_alg_sysinit();

#ifdef _MODULE
	/* Attach /dev/npf device. */
	error = devsw_attach("npf", NULL, &bmajor, &npf_cdevsw, &cmajor);
	if (error) {
		npf_nat_sysfini();
		npf_session_sysfini();
		npf_tableset_sysfini();
		npf_ruleset_sysfini();
	}
#endif
	return error;
}

static int
npf_fini(void)
{

#ifdef _MODULE
	/* At first, detach device and remove pfil hooks. */
	devsw_detach(NULL, &npf_cdevsw);
#endif
	npf_nat_sysfini();
	npf_alg_sysfini();
	npf_session_sysfini();
	npf_tableset_sysfini();
	npf_ruleset_sysfini();

	return 0;
}

/*
 * Module interface.
 */
static int
npf_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return npf_init();
	case MODULE_CMD_FINI:
		return npf_fini();
	default:
		return ENOTTY;
	}
	return 0;
}

void
npfattach(int nunits)
{

	/* Void. */
}

static int
npf_dev_open(dev_t dev, int flag, int mode, lwp_t *l)
{

	/* Available only for super-user. */
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER, NULL)) {
		return EPERM;
	}
	return 0;
}

static int
npf_dev_close(dev_t dev, int flag, int mode, lwp_t *l)
{

	return 0;
}

static int
npf_dev_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	int error;

	/* Available only for super-user. */
	if (kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER, NULL)) {
		return EPERM;
	}

	switch (cmd) {
	case IOC_NPF_VERSION:
		*(int *)data = NPF_VERSION;
		error = 0;
		break;
	case IOC_NPF_SWITCH:
		error = npfctl_switch(data);
		break;
	case IOC_NPF_RELOAD:
		error = npfctl_reload(cmd, data);
		break;
	case IOC_NPF_TABLE:
		error = npfctl_table(data);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}

static int
npf_dev_poll(dev_t dev, int events, lwp_t *l)
{

	return ENOTSUP;
}

static int
npf_dev_read(dev_t dev, struct uio *uio, int flag)
{

	return ENOTSUP;
}
