/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 *
 * $FreeBSD: head/sys/cddl/dev/sdt/sdt.c 297771 2016-04-10 01:24:27Z markj $
 *
 */

/*
 * This file contains a reimplementation of the statically-defined tracing (SDT)
 * framework for DTrace. Probes and SDT providers are defined using the macros
 * in sys/sdt.h, which append all the needed structures to linker sets. When
 * this module is loaded, it iterates over all of the loaded modules and
 * registers probes and providers with the DTrace framework based on the
 * contents of these linker sets.
 *
 * A list of SDT providers is maintained here since a provider may span multiple
 * modules. When a kernel module is unloaded, a provider defined in that module
 * is unregistered only if no other modules refer to it. The DTrace framework is
 * responsible for destroying individual probes when a kernel module is
 * unloaded; in particular, probes may not span multiple kernel modules.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sdt.c,v 1.18.12.1 2018/06/25 07:25:15 pgoyette Exp $");

#include <sys/cdefs.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <sys/conf.h>
#ifdef __FreeBSD__
#include <sys/eventhandler.h>
#endif
#include <sys/kernel.h>
#include <sys/syslimits.h>
#ifdef __FreeBSD__
#include <sys/linker.h>
#include <sys/linker_set.h>
#endif
#include <sys/lock.h>
#ifdef __FreeBSD__
#include <sys/lockstat.h>
#endif
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/queue.h>
#define KDTRACE_HOOKS
#include <sys/sdt.h>

#include <sys/dtrace.h>
#include <sys/dtrace_bsd.h>

/* DTrace methods. */
static void	sdt_getargdesc(void *, dtrace_id_t, void *, dtrace_argdesc_t *);
static void	sdt_provide_probes(void *, dtrace_probedesc_t *);
static void	sdt_destroy(void *, dtrace_id_t, void *);
static int	sdt_enable(void *, dtrace_id_t, void *);
static void	sdt_disable(void *, dtrace_id_t, void *);

static void	sdt_load(void);
static int	sdt_unload(void);
static void	sdt_create_provider(struct sdt_provider *);
static void	sdt_create_probe(struct sdt_probe *);
#ifdef __FreeBSD__
static void	sdt_kld_load(void *, struct linker_file *);
static void	sdt_kld_unload_try(void *, struct linker_file *, int *);
#endif

MALLOC_DECLARE(M_SDT);
MALLOC_DEFINE(M_SDT, "SDT", "DTrace SDT providers");
#define SDT_KASSERT(cond, msg)	KASSERT(cond)

static dtrace_pattr_t sdt_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t sdt_pops = {
	sdt_provide_probes,
	NULL,
	sdt_enable,
	sdt_disable,
	NULL,
	NULL,
	sdt_getargdesc,
	NULL,
	NULL,
	sdt_destroy,
};

#ifdef __NetBSD__
static int
sdt_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	return 0;
}

static const struct cdevsw sdt_cdevsw = {
	.d_open		= sdt_open,
	.d_close	= noclose,
	.d_read		= noread,
	.d_write	= nowrite,
	.d_ioctl	= noioctl,
	.d_stop		= nostop,
	.d_tty		= notty,
	.d_poll		= nopoll,
	.d_mmap		= nommap,
	.d_kqfilter	= nokqfilter,
	.d_discard	= nodiscard,
	.d_flag		= D_OTHER
};
#endif

static TAILQ_HEAD(, sdt_provider) sdt_prov_list;

#ifdef __FreeBSD__
static eventhandler_tag	sdt_kld_load_tag;
static eventhandler_tag	sdt_kld_unload_try_tag;
#endif

#ifdef __NetBSD__
static char *
strdup(const char *s, const struct malloc_type *m)
{
	size_t l = strlen(s) + 1;
	char *d = malloc(l, m, M_WAITOK);
	memcpy(d, s, l);
	return d;
}
#endif

static void
sdt_create_provider(struct sdt_provider *prov)
{
	struct sdt_provider *curr, *newprov;

	TAILQ_FOREACH(curr, &sdt_prov_list, prov_entry)
		if (strcmp(prov->name, curr->name) == 0) {
			/* The provider has already been defined. */
			curr->sdt_refs++;
			return;
		}

	/*
	 * Make a copy of prov so that we don't lose fields if its module is
	 * unloaded but the provider isn't destroyed. This could happen with
	 * a provider that spans multiple modules.
	 */
	newprov = malloc(sizeof(*newprov), M_SDT, M_WAITOK | M_ZERO);
	newprov->name = strdup(prov->name, M_SDT);
	prov->sdt_refs = newprov->sdt_refs = 1;

	TAILQ_INSERT_TAIL(&sdt_prov_list, newprov, prov_entry);

	(void)dtrace_register(newprov->name, &sdt_attr, DTRACE_PRIV_USER, NULL,
	    &sdt_pops, NULL, (dtrace_provider_id_t *)&newprov->id);
	prov->id = newprov->id;
}

static void
sdt_create_probe(struct sdt_probe *probe)
{
	struct sdt_provider *prov;
	char mod[DTRACE_MODNAMELEN];
	char func[DTRACE_FUNCNAMELEN];
	char name[DTRACE_NAMELEN];
	const char *from;
	char *to;
	size_t len;

	if (probe->version != (int)sizeof(*probe)) {
		printf("ignoring probe %p, version %u expected %u\n",
		    probe, probe->version, (int)sizeof(*probe));
		return;
	}

	TAILQ_FOREACH(prov, &sdt_prov_list, prov_entry)
		if (strcmp(prov->name, probe->prov->name) == 0)
			break;

	SDT_KASSERT(prov != NULL, ("probe defined without a provider"));

#ifdef __FreeBSD__
	/* If no module name was specified, use the module filename. */
	if (*probe->mod == 0) {
		len = strlcpy(mod, probe->sdtp_lf->filename, sizeof(mod));
		if (len > 3 && strcmp(mod + len - 3, ".ko") == 0)
			mod[len - 3] = '\0';
	} else
#endif
		strlcpy(mod, probe->mod, sizeof(mod));

	/*
	 * Unfortunately this is necessary because the Solaris DTrace
	 * code mixes consts and non-consts with casts to override
	 * the incompatibilies. On FreeBSD, we use strict warnings
	 * in the C compiler, so we have to respect const vs non-const.
	 */
	strlcpy(func, probe->func, sizeof(func));
	if (func[0] == '\0')
		strcpy(func, "none");

	from = probe->name;
	to = name;
	for (len = 0; len < (sizeof(name) - 1) && *from != '\0';
	    len++, from++, to++) {
		if (from[0] == '_' && from[1] == '_') {
			*to = '-';
			from++;
		} else
			*to = *from;
	}
	*to = '\0';

	if (dtrace_probe_lookup(prov->id, mod, func, name) != DTRACE_IDNONE)
		return;

	(void)dtrace_probe_create(prov->id, mod, func, name, 1, probe);
}

/*
 * Probes are created through the SDT module load/unload hook, so this function
 * has nothing to do. It only exists because the DTrace provider framework
 * requires one of provide_probes and provide_module to be defined.
 */
static void
sdt_provide_probes(void *arg, dtrace_probedesc_t *desc)
{
}

static int
sdt_enable(void *arg __unused, dtrace_id_t id, void *parg)
{
	struct sdt_probe *probe = parg;

	probe->id = id;
#ifdef __FreeBSD__
	probe->sdtp_lf->nenabled++;
	if (strcmp(probe->prov->name, "lockstat") == 0)
		lockstat_enabled++;
#endif
	return 1;
}

static void
sdt_disable(void *arg __unused, dtrace_id_t id, void *parg)
{
	struct sdt_probe *probe = parg;

#ifdef __FreeBSD__
	SDT_KASSERT(probe->sdtp_lf->nenabled > 0, ("no probes enabled"));
	if (strcmp(probe->prov->name, "lockstat") == 0)
		lockstat_enabled--;
	probe->sdtp_lf->nenabled--;
#endif
	probe->id = 0;
}

static void
sdt_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{
	struct sdt_argtype *argtype;
	struct sdt_probe *probe = parg;

	if (desc->dtargd_ndx >= probe->n_args) {
		desc->dtargd_ndx = DTRACE_ARGNONE;
		return;
	}

	TAILQ_FOREACH(argtype, &probe->argtype_list, argtype_entry) {
		if (desc->dtargd_ndx == argtype->ndx) {
			desc->dtargd_mapping = desc->dtargd_ndx;
			if (argtype->type == NULL) {
				desc->dtargd_native[0] = '\0';
				desc->dtargd_xlate[0] = '\0';
				continue;
			}
			strlcpy(desc->dtargd_native, argtype->type,
			    sizeof(desc->dtargd_native));
			if (argtype->xtype != NULL)
				strlcpy(desc->dtargd_xlate, argtype->xtype,
				    sizeof(desc->dtargd_xlate));
		}
	}
}

static void
sdt_destroy(void *arg, dtrace_id_t id, void *parg)
{
}

#ifdef __FreeBSD__
/*
 * Called from the kernel linker when a module is loaded, before
 * dtrace_module_loaded() is called. This is done so that it's possible to
 * register new providers when modules are loaded. The DTrace framework
 * explicitly disallows calling into the framework from the provide_module
 * provider method, so we cannot do this there.
 */
static void
sdt_kld_load(void *arg __unused, struct linker_file *lf)
{
	struct sdt_provider **prov, **begin, **end;
	struct sdt_probe **probe, **p_begin, **p_end;
	struct sdt_argtype **argtype, **a_begin, **a_end;

	if (linker_file_lookup_set(lf, "sdt_providers_set", &begin, &end,
	    NULL) == 0) {
		for (prov = begin; prov < end; prov++)
			sdt_create_provider(*prov);
	}

	if (linker_file_lookup_set(lf, "sdt_probes_set", &p_begin, &p_end,
	    NULL) == 0) {
		for (probe = p_begin; probe < p_end; probe++) {
			(*probe)->sdtp_lf = lf;
			sdt_create_probe(*probe);
			TAILQ_INIT(&(*probe)->argtype_list);
		}
	}

	if (linker_file_lookup_set(lf, "sdt_argtypes_set", &a_begin, &a_end,
	    NULL) == 0) {
		for (argtype = a_begin; argtype < a_end; argtype++) {
			(*argtype)->probe->n_args++;
			TAILQ_INSERT_TAIL(&(*argtype)->probe->argtype_list,
			    *argtype, argtype_entry);
		}
	}
}

static void
sdt_kld_unload_try(void *arg __unused, struct linker_file *lf, int *error)
{
	struct sdt_provider *prov, **curr, **begin, **end, *tmp;

	if (*error != 0)
		/* We already have an error, so don't do anything. */
		return;
	else if (linker_file_lookup_set(lf, "sdt_providers_set", &begin, &end,
	    NULL))
		/* No DTrace providers are declared in this file. */
		return;

	/*
	 * Go through all the providers declared in this linker file and
	 * unregister any that aren't declared in another loaded file.
	 */
	for (curr = begin; curr < end; curr++) {
		TAILQ_FOREACH_SAFE(prov, &sdt_prov_list, prov_entry, tmp) {
			if (strcmp(prov->name, (*curr)->name) != 0)
				continue;

			if (prov->sdt_refs == 1) {
				if (dtrace_unregister(prov->id) != 0) {
					*error = 1;
					return;
				}
				TAILQ_REMOVE(&sdt_prov_list, prov, prov_entry);
				free(prov->name, M_SDT);
				free(prov, M_SDT);
			} else
				prov->sdt_refs--;
			break;
		}
	}
}

static int
sdt_linker_file_cb(linker_file_t lf, void *arg __unused)
{

	sdt_kld_load(NULL, lf);

	return (0);
}
#endif

#ifdef __NetBSD__
/*
 * weak symbols don't work in kernel modules; link set end symbols are
 * weak by default, so we kill that.
 */
#undef __weak
#define __weak
__link_set_decl(sdt_providers_set, struct sdt_provider);
__link_set_decl(sdt_probes_set, struct sdt_probe);
__link_set_decl(sdt_argtypes_set, struct sdt_argtype);

/*
 * Unfortunately we don't have linker set functions and event handlers
 * to support loading and unloading probes in modules... Currently if
 * modules have probes, if the modules are loaded when sdt is loaded
 * they will work, but they will crash unloading.
 */
static void
sdt_link_set_load(void)
{
	struct sdt_provider * const *provider;
	struct sdt_probe * const *probe;
	struct sdt_argtype * const *argtype;

	__link_set_foreach(provider, sdt_providers_set) {
		sdt_create_provider(*provider);
	}

	__link_set_foreach(probe, sdt_probes_set) {
		(*probe)->sdtp_lf = NULL;	// XXX: we don't support it
		sdt_create_probe(*probe);
		TAILQ_INIT(&(*probe)->argtype_list);
	}

	__link_set_foreach(argtype, sdt_argtypes_set) {
		(*argtype)->probe->n_args++;
		TAILQ_INSERT_TAIL(&(*argtype)->probe->argtype_list,
		    *argtype, argtype_entry);
	}
}

static void
sdt_link_set_unload(void)
{
	struct sdt_provider * const *curr, *prov, *tmp;

	/*
	 * Go through all the providers declared in this linker file and
	 * unregister any that aren't declared in another loaded file.
	 */
	__link_set_foreach(curr, sdt_providers_set) {
		TAILQ_FOREACH_SAFE(prov, &sdt_prov_list, prov_entry, tmp) {
			if (strcmp(prov->name, (*curr)->name) != 0)
				continue;

			if (prov->sdt_refs == 1) {
				if (dtrace_unregister(prov->id) != 0) {
					return;
				}
				TAILQ_REMOVE(&sdt_prov_list, prov, prov_entry);
				free(__UNCONST(prov->name), M_SDT);
				free(prov, M_SDT);
			} else
				prov->sdt_refs--;
			break;
		}
	}
}
#endif


static void
sdt_load(void)
{
	TAILQ_INIT(&sdt_prov_list);

#ifdef __FreeBSD__
	sdt_probe_func = dtrace_probe;

	sdt_kld_load_tag = EVENTHANDLER_REGISTER(kld_load, sdt_kld_load, NULL,
	    EVENTHANDLER_PRI_ANY);
	sdt_kld_unload_try_tag = EVENTHANDLER_REGISTER(kld_unload_try,
	    sdt_kld_unload_try, NULL, EVENTHANDLER_PRI_ANY);

	/* Pick up probes from the kernel and already-loaded linker files. */
	linker_file_foreach(sdt_linker_file_cb, NULL);
#endif
#ifdef __NetBSD__
	sdt_init(dtrace_probe);
	sdt_link_set_load();
#endif
}

static int
sdt_unload(void)
{
	struct sdt_provider *prov, *tmp;
	int ret;

#ifdef __FreeBSD__
	EVENTHANDLER_DEREGISTER(kld_load, sdt_kld_load_tag);
	EVENTHANDLER_DEREGISTER(kld_unload_try, sdt_kld_unload_try_tag);

	sdt_probe_func = sdt_probe_stub;
#endif

#ifdef __NetBSD__
	sdt_exit();

	sdt_link_set_unload();
#endif
	TAILQ_FOREACH_SAFE(prov, &sdt_prov_list, prov_entry, tmp) {
		ret = dtrace_unregister(prov->id);
		if (ret != 0)
			return (ret);
		TAILQ_REMOVE(&sdt_prov_list, prov, prov_entry);
		free(__UNCONST(prov->name), M_SDT);
		free(prov, M_SDT);
	}

	return (0);
}

#ifdef __FreeBSD__
static int
sdt_modevent(module_t mod __unused, int type, void *data __unused)
{

	switch (type) {
	case MOD_LOAD:
	case MOD_UNLOAD:
	case MOD_SHUTDOWN:
		return (0);
	default:
		return (EOPNOTSUPP);
	}
}

SYSINIT(sdt_load, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY, sdt_load, NULL);
SYSUNINIT(sdt_unload, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY, sdt_unload, NULL);

DEV_MODULE(sdt, sdt_modevent, NULL);
MODULE_VERSION(sdt, 1);
MODULE_DEPEND(sdt, dtrace, 1, 1, 1);
#endif

#ifdef __NetBSD__
static int
dtrace_sdt_modcmd(modcmd_t cmd, void *data)
{
	int bmajor = -1, cmajor = -1;
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		sdt_load();
		return devsw_attach("sdt", NULL, &bmajor,
		    &sdt_cdevsw, &cmajor);
	case MODULE_CMD_FINI:
		error = sdt_unload();
		if (error != 0)
			return error;
		return devsw_detach(NULL, &sdt_cdevsw);
	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
	default:
		error = EOPNOTSUPP;
		break;
	}
	return error;
}

MODULE(MODULE_CLASS_MISC, dtrace_sdt, "dtrace");
#endif
