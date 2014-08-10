/*	$NetBSD: sdt.c,v 1.8.18.1 2014/08/10 06:50:28 tls Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by CoyotePoint Systems, Inc. It was developed under contract to 
 * CoyotePoint by Darran Hunt.
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

#ifdef _KERNEL_OPT
#include "opt_dtrace.h"
#endif

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mutex.h>

#include <sys/dtrace.h>

#define KDTRACE_HOOKS
#include <sys/sdt.h>

#undef SDT_DEBUG

static dev_type_open(sdt_open);

static int	sdt_unload(void);
static void	sdt_getargdesc(void *, dtrace_id_t, void *, dtrace_argdesc_t *);
static void	sdt_provide(void *, const dtrace_probedesc_t *);
static void	sdt_destroy(void *, dtrace_id_t, void *);
static int	sdt_enable(void *, dtrace_id_t, void *);
static void	sdt_disable(void *, dtrace_id_t, void *);
static void	sdt_load(void *);

static const struct cdevsw sdt_cdevsw = {
	sdt_open, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter, nodiscard,
	D_OTHER
};

static dtrace_pops_t sdt_pops = {
	sdt_provide,
	NULL,
	sdt_enable,
	sdt_disable,
	NULL,
	NULL,
	sdt_getargdesc,
	NULL,
	NULL,
	sdt_destroy
};

#ifdef notyet
static struct cdev		*sdt_cdev;
#endif

/*
 * Provider and probe definitions 
 */

/*
 * proc provider
 */

/* declare all probes belonging to the provider */
SDT_PROBE_DECLARE(proc,,,create);
SDT_PROBE_DECLARE(proc,,,exec);
SDT_PROBE_DECLARE(proc,,,exec_success);
SDT_PROBE_DECLARE(proc,,,exec_failure);
SDT_PROBE_DECLARE(proc,,,signal_send);
SDT_PROBE_DECLARE(proc,,,signal_discard);
SDT_PROBE_DECLARE(proc,,,signal_clear);
SDT_PROBE_DECLARE(proc,,,signal_handle);
SDT_PROBE_DECLARE(proc,,,lwp_create);
SDT_PROBE_DECLARE(proc,,,lwp_start);
SDT_PROBE_DECLARE(proc,,,lwp_exit);

/* define the provider */
static sdt_provider_t proc_provider = {
	"proc",		/* provider name */
	0,		/* registered ID - leave as 0 */
	{
		{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
		{ DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_CLASS_UNKNOWN },
		{ DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_CLASS_ISA },
		{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
		{ DTRACE_STABILITY_PRIVATE,  DTRACE_STABILITY_PRIVATE,  DTRACE_CLASS_ISA },
	},

	/* list all probes belonging to the provider */
	{ 
		&SDT_NAME(proc,,,create),
		&SDT_NAME(proc,,,exec),
		&SDT_NAME(proc,,,exec_success),
		&SDT_NAME(proc,,,exec_failure),
		&SDT_NAME(proc,,,signal_send),
		&SDT_NAME(proc,,,signal_discard),
		&SDT_NAME(proc,,,signal_clear),
		&SDT_NAME(proc,,,signal_handle),
		&SDT_NAME(proc,,,lwp_create),
		&SDT_NAME(proc,,,lwp_start),
		&SDT_NAME(proc,,,lwp_exit),
		NULL				/* NULL terminated list */
	}
};

/* list of local providers to register with DTrace */
static sdt_provider_t *sdt_providers[] = {
	&proc_provider,
	NULL		/* NULL terminated list */
};

static sdt_provider_t **sdt_list = NULL;	/* registered provider list */
static kmutex_t sdt_mutex;
static int sdt_count = 0;	/* number of registered providers */

static void
sdt_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{
	sdt_probe_t *sprobe = parg;

#ifdef SDT_DEBUG
	printf("sdt: %s probe %d\n", __func__, id);
	printf("%s: probe %d (%s:%s:%s:%s).%d\n",
		__func__, id,
		sprobe->provider,
		sprobe->module,
		sprobe->function,
		sprobe->name,
		desc->dtargd_ndx);
#endif

	/* provide up to 5 arguments  */
	if ((desc->dtargd_ndx < SDT_MAX_ARGS) &&
		(sprobe->argv[desc->dtargd_ndx] != NULL)) {
		strncpy(desc->dtargd_native, sprobe->argv[desc->dtargd_ndx],
			sizeof(desc->dtargd_native));
		desc->dtargd_mapping = desc->dtargd_ndx;
		if (sprobe->argx[desc->dtargd_ndx] != NULL) {
			strncpy(desc->dtargd_xlate, sprobe->argx[desc->dtargd_ndx],
			    sizeof(desc->dtargd_xlate));
		}
#ifdef SDT_DEBUG
		printf("%s: probe %d (%s:%s:%s:%s).%d = %s\n",
			__func__, id,
			sprobe->provider,
			sprobe->module,
			sprobe->function,
			sprobe->name,
			desc->dtargd_ndx,
			sprobe->argv[desc->dtargd_ndx]);
#endif
	} else {
#ifdef SDT_DEBUG
		printf("%s: probe %d (%s:%s:%s:%s).%d = NULL\n",
			__func__, id,
			sprobe->provider,
			sprobe->module,
			sprobe->function,
			sprobe->name,
			desc->dtargd_ndx);
		desc->dtargd_ndx = DTRACE_ARGNONE;
#endif
	}
}

static void
sdt_provide(void *arg, const dtrace_probedesc_t *desc)
{
	sdt_provider_t *sprov = arg;
	int res;
	int ind;
	int num_probes = 0;

#ifdef SDT_DEBUG
	if (desc == NULL) {
		printf("sdt: provide null\n");
	} else {
		printf("sdt: provide %d %02x:%02x:%02x:%02x\n",
		    desc->dtpd_id,
		    desc->dtpd_provider[0],
		    desc->dtpd_mod[0],
		    desc->dtpd_func[0],
		    desc->dtpd_name[0]);
	}
#endif

	for (ind = 0; sprov->probes[ind] != NULL; ind++) {
	    	if (sprov->probes[ind]->created == 0) {
			res = dtrace_probe_create(sprov->id,
				sprov->probes[ind]->module,
				sprov->probes[ind]->function,
				sprov->probes[ind]->name,
				0, sprov->probes[ind]);
			sprov->probes[ind]->id = res;
#ifdef SDT_DEBUG
			printf("%s: dtrace_probe_create[%d] res=%d\n",
				__func__, ind, res);
#endif
			sprov->probes[ind]->created = 1;
			num_probes++;
		}
	}

#ifdef SDT_DEBUG
	printf("sdt: %s num_probes %d\n", __func__, ind);
#endif

}

static void
sdt_destroy(void *arg, dtrace_id_t id, void *parg)
{
	sdt_provider_t *sprov = arg;
	int ind;

#ifdef SDT_DEBUG
	printf("sdt: %s\n", __func__);
#endif

	for (ind = 0; sprov->probes[ind] != NULL; ind++) {
	    	if (sprov->probes[ind]->id == id) {
#ifdef SDT_DEBUG
		    	printf("%s: destroying probe %d (%s:%s:%s:%s)\n",
				__func__, id,
				sprov->probes[ind]->provider,
				sprov->probes[ind]->module,
				sprov->probes[ind]->function,
				sprov->probes[ind]->name);
#endif
			sprov->probes[ind]->enabled = 0;
			sprov->probes[ind]->created = 0;
			sprov->probes[ind]->id = 0;
			break;
		}
	}
}

static int
sdt_enable(void *arg, dtrace_id_t id, void *parg)
{
	sdt_provider_t *sprov = arg;
	int ind;

#ifdef SDT_DEBUG
	printf("sdt: %s\n", __func__);
#endif

	for (ind = 0; sprov->probes[ind] != NULL; ind++) {
	    	if (sprov->probes[ind]->id == id) {
#ifdef SDT_DEBUG
		    	printf("%s: enabling probe %d (%s:%s:%s:%s)\n",
				__func__, id,
				sprov->probes[ind]->provider,
				sprov->probes[ind]->module,
				sprov->probes[ind]->function,
				sprov->probes[ind]->name);
#endif
			sprov->probes[ind]->enabled = 1;
			break;
		}
	}

	return 0;
}

static void
sdt_disable(void *arg, dtrace_id_t id, void *parg)
{
	sdt_provider_t *sprov = arg;
	int ind;

#ifdef SDT_DEBUG
	printf("sdt: %s\n", __func__);
#endif

	for (ind = 0; sprov->probes[ind] != NULL; ind++) {
	    	if (sprov->probes[ind]->id == id) {
#ifdef SDT_DEBUG
		    	printf("%s: disabling probe %d (%s:%s:%s:%s)\n",
				__func__, id,
				sprov->probes[ind]->provider,
				sprov->probes[ind]->module,
				sprov->probes[ind]->function,
				sprov->probes[ind]->name);
#endif
			sprov->probes[ind]->enabled = 0;
			break;
		}
	}
}

int
sdt_register(sdt_provider_t *prov)
{
	int ind;
	int res;

	/* make sure the provider is not already registered */
	for (ind = 0; ind < sdt_count; ind++) {
		if (strncmp(sdt_list[ind]->name, prov->name,
		    SDT_MAX_NAME_SIZE) == 0) {
			printf("sdt: provider %s already registered\n", prov->name);
			return -1;
		}
	}

	/* register the new provider */
	if ((res = dtrace_register(prov->name, 
			    &prov->attr, DTRACE_PRIV_USER,
			    NULL, &sdt_pops, prov,
			    &(prov->id))) != 0) {
		printf("sdt: failed to register %s res = %d\n",
			prov->name, res);
		return -1;
	}

	sdt_list[sdt_count++] = prov;

	return 0;
}

int
sdt_unregister(sdt_provider_t *prov)
{
	int ind;
	int res;

	/* find the provider reference */
	for (ind = 0; ind < sdt_count; ind++) {
		if (sdt_list[ind] == prov) {
			res = dtrace_unregister(sdt_list[ind]->id);
			if (res != 0) {
				printf(
				    "sdt: failed to unregister provider %s\n",
				    sdt_list[ind]->name);
			}
			/* remove provider from list */
			sdt_list[ind] = sdt_list[--sdt_count];
			return 0;
		}
	}

	/* provider not found */
	printf("sdt: provider %s not found\n", prov->name);

	return 0;
}

static void
sdt_load(void *dummy)
{
	int ind;

#ifdef SDT_DEBUG
	printf("sdt: %s\n", __func__);
#endif

	sdt_init(dtrace_probe);

	sdt_list = kmem_alloc(sizeof(sdt_provider_t *) * SDT_MAX_PROVIDER,
	    KM_SLEEP);

	mutex_init(&sdt_mutex, "sdt_mutex", MUTEX_DEFAULT, NULL);

	sdt_count = 0;

	if (sdt_list == NULL) {
		printf("sdt: failed to alloc provider list\n");
		return;
	}

	for (ind = 0; sdt_providers[ind] != NULL; ind++) {
	    	if (sdt_count >= SDT_MAX_PROVIDER) {
			printf("sdt: too many providers\n");
			break;
		}
		sdt_register(sdt_providers[ind]);

#ifdef SDT_DEBUG
		printf("sdt: registered %s id = 0x%x\n",
			sdt_providers[ind]->name,
			sdt_providers[ind]->id);
#endif
	}
}


static int
sdt_unload(void)
{
	int error = 0;
	int res = 0;
	int ind;

#ifdef SDT_DEBUG
	printf("sdt: %s\n", __func__);
#endif

	for (ind = 0; ind < sdt_count; ind++) {
		if ((res = dtrace_unregister(sdt_list[ind]->id)) != 0) {
#ifdef SDT_DEBUG
			printf("%s: failed to unregister %s error = %d\n",
			    sdt_list[ind]->name, res);
#endif
			error = res;
		} else {
#ifdef SDT_DEBUG
			printf("sdt: unregistered %s id = %d\n",
			    sdt_list[ind]->name,
			    sdt_list[ind]->id);
#endif
		}
	}

	kmem_free(sdt_list, sizeof(sdt_provider_t *) * SDT_MAX_PROVIDER);
	mutex_destroy(&sdt_mutex);
	sdt_exit();
	return (error);
}

static int
sdt_modcmd(modcmd_t cmd, void *data)
{
	int bmajor = -1, cmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		sdt_load(NULL);
		return devsw_attach("sdt", NULL, &bmajor,
		    &sdt_cdevsw, &cmajor);
	case MODULE_CMD_FINI:
		sdt_unload();
		return devsw_detach(NULL, &sdt_cdevsw);
	default:
		return ENOTTY;
	}
}

static int
sdt_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	return (0);
}

MODULE(MODULE_CLASS_MISC, sdt, "dtrace");
