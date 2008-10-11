/*	$NetBSD: uipc_accf.c,v 1.4 2008/10/11 16:39:07 tls Exp $	*/

/*-
 * Copyright (c) 2000 Paycounter, Inc.
 * Copyright (c) 2005 Robert N. M. Watson
 * Author: Alfred Perlstein <alfred@paycounter.com>, <alfred@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_accf.c,v 1.4 2008/10/11 16:39:07 tls Exp $");

#define ACCEPT_FILTER_MOD

#include "opt_inet.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/lkm.h>
#include <sys/mutex.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/queue.h>
#include <sys/once.h>

static kmutex_t accept_filter_mtx;
#define	ACCEPT_FILTER_LOCK()		mutex_spin_enter(&accept_filter_mtx)
#define	ACCEPT_FILTER_UNLOCK()		mutex_spin_exit(&accept_filter_mtx);
#define	SOCK_LOCK(so)			KASSERT(solocked(so));
#define	SOCK_UNLOCK(so)			KASSERT(solocked(so));

static SLIST_HEAD(, accept_filter) accept_filtlsthd =
	SLIST_HEAD_INITIALIZER(&accept_filtlsthd);

static int unloadable = 0;

/*
 * Names of Accept filter sysctl objects
 */

#define ACCFCTL_UNLOADABLE	1	/* Allow module to be unloaded */


SYSCTL_SETUP(sysctl_net_inet_accf_setup, "sysctl net.inet.accf subtree setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "accf",
		       SYSCTL_DESCR("Accept filters"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, SO_ACCEPTFILTER, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "unloadable",
		       SYSCTL_DESCR("Allow unload of accept filters "
				    "(not recommended)"),
		       NULL, 0, &unloadable, 0,
		       CTL_NET, PF_INET, SO_ACCEPTFILTER,
		       ACCFCTL_UNLOADABLE, CTL_EOL);
}

/*
 * Must be passed a kmem_malloc'd structure so we don't explode if the kld is
 * unloaded, we leak the struct on deallocation to deal with this, but if a
 * filter is loaded with the same name as a leaked one we re-use the entry.
 */
int
accept_filt_add(struct accept_filter *filt)
{
	struct accept_filter *p;

	ACCEPT_FILTER_LOCK();
	SLIST_FOREACH(p, &accept_filtlsthd, accf_next)
		if (strcmp(p->accf_name, filt->accf_name) == 0)  {
			if (p->accf_callback != NULL) {
				ACCEPT_FILTER_UNLOCK();
				return (EEXIST);
			} else {
				p->accf_callback = filt->accf_callback;
				ACCEPT_FILTER_UNLOCK();
				kmem_free(filt, sizeof(struct accept_filter));
				return (0);
			}
		}
				
	if (p == NULL)
		SLIST_INSERT_HEAD(&accept_filtlsthd, filt, accf_next);
	ACCEPT_FILTER_UNLOCK();
	return (0);
}

int
accept_filt_del(char *name)
{
	struct accept_filter *p;

	p = accept_filt_get(name);
	if (p == NULL)
		return (ENOENT);

	p->accf_callback = NULL;
	return (0);
}

struct accept_filter *
accept_filt_get(char *name)
{
	struct accept_filter *p;

	ACCEPT_FILTER_LOCK();
	SLIST_FOREACH(p, &accept_filtlsthd, accf_next)
		if (strcmp(p->accf_name, name) == 0)
			break;
	ACCEPT_FILTER_UNLOCK();

	return (p);
}

/*
 * Accept filter initialization routine.
 * This should be called only once.
 */

static int
accept_filter_init0(void)
{
	mutex_init(&accept_filter_mtx, MUTEX_DEFAULT, IPL_NET);

	return 0;
}

/*
 * Initialization routine: This can also be replaced with 
 * accept_filt_generic_mod_event for attaching new accept filter.
 */

void
accept_filter_init(void)
{
	static ONCE_DECL(accept_filter_init_once);

	RUN_ONCE(&accept_filter_init_once, accept_filter_init0);
}

int
accept_filt_generic_mod_event(struct lkm_table *lkmtp, int event, void *data)
{
	struct accept_filter *p;
	struct accept_filter *accfp = (struct accept_filter *) data;
	int error;

	switch (event) {
	case LKM_E_LOAD:
		accept_filter_init();
		p = kmem_alloc(sizeof(struct accept_filter), KM_SLEEP);
		bcopy(accfp, p, sizeof(*p));
		error = accept_filt_add(p);
		break;

	case LKM_E_UNLOAD:
		/*
		 * Do not support unloading yet. we don't keep track of
		 * refcounts and unloading an accept filter callback and then
		 * having it called is a bad thing.  A simple fix would be to
		 * track the refcount in the struct accept_filter.
		 */
		if (unloadable != 0) {
			error = accept_filt_del(accfp->accf_name);
		} else
			error = EOPNOTSUPP;
		break;

	case LKM_E_STAT:
		error = 0;
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}

	return (error);
}

int
do_getopt_accept_filter(struct socket *so, struct sockopt *sopt)
{
	struct accept_filter_arg afa;
	int error;

	SOCK_LOCK(so);
	if ((so->so_options & SO_ACCEPTCONN) == 0) {
		error = EINVAL;
		goto out;
	}
	if ((so->so_options & SO_ACCEPTFILTER) == 0) {
		error = EINVAL;
		goto out;
	}

	memset(&afa, 0, sizeof(afa));
	strcpy(afa.af_name, so->so_accf->so_accept_filter->accf_name);
	if (so->so_accf->so_accept_filter_str != NULL)
		strcpy(afa.af_arg, so->so_accf->so_accept_filter_str);
	error = sockopt_set(sopt, &afa, sizeof(afa));
out:
	SOCK_UNLOCK(so);
	return (error);
}

int
do_setopt_accept_filter(struct socket *so, const struct sockopt *sopt)
{
	struct accept_filter_arg afa;
	struct accept_filter *afp;
	struct so_accf *newaf;
	int error;

	/*
	 * Handle the simple delete case first.
	 */
	if (sopt == NULL || sopt->sopt_size == 0) {
		SOCK_LOCK(so);
		if ((so->so_options & SO_ACCEPTCONN) == 0) {
			SOCK_UNLOCK(so);
			return (EINVAL);
		}
		if (so->so_accf != NULL) {
			struct so_accf *af = so->so_accf;
			if (af->so_accept_filter != NULL &&
				af->so_accept_filter->accf_destroy != NULL) {
				af->so_accept_filter->accf_destroy(so);
			}
			if (af->so_accept_filter_str != NULL) {
				kmem_free(af->so_accept_filter_str,
					  sizeof(afa.af_name));
			}
			kmem_free(af, sizeof(*af));
			so->so_accf = NULL;
		}
		so->so_options &= ~SO_ACCEPTFILTER;
		SOCK_UNLOCK(so);
		return (0);
	}

	/*
	 * Pre-allocate any memory we may need later to avoid blocking at
	 * untimely moments.  This does not optimize for invalid arguments.
	 *
	 * XXX on NetBSD, we're called with the socket lock already held,
	 * XXX so we should not allow this allocation to block either.
	 */
	error = sockopt_get(sopt, &afa, sizeof(afa));
	if (error) {
		return (error);
	}
	afa.af_name[sizeof(afa.af_name)-1] = '\0';
	afa.af_arg[sizeof(afa.af_arg)-1] = '\0';
	afp = accept_filt_get(afa.af_name);
	if (afp == NULL) {
		return (ENOENT);
	}
	/*
	 * Allocate the new accept filter instance storage.  We may
	 * have to free it again later if we fail to attach it.  If
	 * attached properly, 'newaf' is NULLed to avoid a free()
	 * while in use.
	 */
	newaf = kmem_zalloc(sizeof(*newaf), KM_NOSLEEP);
	if (!newaf) {
		return ENOMEM;
	}
	if (afp->accf_create != NULL && afa.af_name[0] != '\0') {
		/*
		 * FreeBSD did a variable-size allocation here
		 * with the actual string length from afa.af_name
		 * but it is so short, why bother tracking it?
		 * XXX as others have noted, this is an API mistake;
		 * XXX accept_filter_arg should have a mandatory namelen.
		 * XXX (but it's a bit too late to fix that now)
		 */
		newaf->so_accept_filter_str =
		    kmem_alloc(sizeof(afa.af_name), KM_NOSLEEP);
		if (!newaf->so_accept_filter_str) {
			kmem_free(newaf, sizeof(*newaf));
			return ENOMEM;
		}
		strcpy(newaf->so_accept_filter_str, afa.af_name);
	}

	/*
	 * Require a listen socket; don't try to replace an existing filter
	 * without first removing it.
	 */
	SOCK_LOCK(so);
	if (((so->so_options & SO_ACCEPTCONN) == 0) ||
	    (so->so_accf != NULL)) {
		error = EINVAL;
		goto out;
	}

	/*
	 * Invoke the accf_create() method of the filter if required.  The
	 * socket mutex is held over this call, so create methods for filters
	 * can't block.
	 */
	if (afp->accf_create != NULL) {
		newaf->so_accept_filter_arg =
		    afp->accf_create(so, afa.af_arg);
		if (newaf->so_accept_filter_arg == NULL) {
			error = EINVAL;
			goto out;
		}
	}
	newaf->so_accept_filter = afp;
	so->so_accf = newaf;
	so->so_options |= SO_ACCEPTFILTER;
	newaf = NULL;
out:
	SOCK_UNLOCK(so);
	if (newaf != NULL) {
		if (newaf->so_accept_filter_str != NULL)
			kmem_free(newaf->so_accept_filter_str,
				  sizeof(afa.af_name));
		kmem_free(newaf, sizeof(*newaf));
	}
	return (error);
}
