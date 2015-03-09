/*	$NetBSD: prototype.c,v 1.3 2015/03/09 03:43:02 riastradh Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: prototype.c,v 1.3 2015/03/09 03:43:02 riastradh Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/dtrace.h>
#include <sys/module.h>
#include <sys/systm.h>

static dtrace_provider_id_t prototype_id;

static void
prototype_enable(void *arg, dtrace_id_t id, void *parg)
{
	/* Enable the probe for id.  */
}

static void
prototype_disable(void *arg, dtrace_id_t id, void *parg)
{
	/* Disable the probe for id.  */
}

static void
prototype_getargdesc(void *arg, dtrace_id_t id, void *parg,
    dtrace_argdesc_t *desc)
{
	/* Get the argument descriptions for the probe id.  */
}

static uint64_t
prototype_getargval(void *arg, dtrace_id_t id, void *parg, int argno,
    int aframes)
{
	/* Get the value for the argno'th argument to the probe id.  */
}

static void
prototype_provide(void *arg, const dtrace_probedesc_t *desc)
{
	/*
	 * Create all probes for this provider.  Cookie passed to
	 * dtrace_probe_create will be passed on to prototype_destroy.
	 */
}

static void
prototype_destroy(void *arg, dtrace_id_t id, void *parg)
{
	/*
	 * Destroy a probe for this provider.  parg is cookie that was
	 * passed to dtrace_probe_create for this probe.
	 */
}

static dtrace_pattr_t prototype_attr = {
/* provider attributes */
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
/* module attributes */
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
/* function attributes */
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
/* name attributes */
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
/* arguments attributes */
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
};

static dtrace_pops_t prototype_pops = {
	prototype_provide,
	NULL,			/* provide_module (NYI) */
	prototype_enable,
	prototype_disable,
	NULL,			/* suspend (NYI) */
	NULL,			/* resume (NYI) */
	prototype_getargdesc,	/* optional */
	prototype_getargval,	/* optional */
	NULL,			/* usermode permissions check */
	prototype_destroy,
};

static int
prototype_init(void)
{
	int error;

	/* Set up any necessary state, or fail. */

	/* Register the dtrace provider.  */
	KASSERT(prototype_id == 0);
	error = dtrace_register("prototype", &prototype_attr, DTRACE_PRIV_USER,
	    NULL, &prototype_pops, NULL, &prototype_id);
	if (error) {
		printf("dtrace_prototype: failed to register dtrace provider"
		    ": %d\n", error);
		goto fail0;
	}
	KASSERT(prototype_id != 0);

	/* Success!  */
	return 0;

fail0:	KASSERT(error);
	return error;
}

static int
prototype_fini(void)
{
	int error;

	/*
	 * Deregister the dtrace provider, unless we already did but
	 * something below failed.
	 */
	if (prototype_id != 0) {
		error = dtrace_unregister(prototype_id);
		if (error) {
			printf("dtrace prototype"
			    ": failed to unregister dtrace provider: %d\n",
			    error);
			return error;
		}
		prototype_id = 0;
	}

	/* Tear down any necessary state, or fail.  */

	/* Success!  */
	return 0;
}

static int
dtrace_prototype_modcmd(modcmd_t cmd, void *data)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return prototype_init();
	case MODULE_CMD_FINI:
		return prototype_fini();
	default:
		return ENOTTY;
	}
}

MODULE(MODULE_CLASS_MISC, dtrace_prototype, "dtrace");
