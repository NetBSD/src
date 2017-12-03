/*	$NetBSD: npf_if.c,v 1.4.4.3 2017/12/03 11:39:03 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
 * NPF network interface handling module.
 *
 * NPF uses its own interface IDs (npf-if-id).  When NPF configuration is
 * (re)loaded, each required interface name is registered and a matching
 * network interface gets an ID assigned.  If an interface is not present,
 * it gets an ID on attach.
 *
 * IDs start from 1.  Zero is reserved to indicate "no interface" case or
 * an interface of no interest (i.e. not registered).
 *
 * The IDs are mapped synchronously based on interface events which are
 * monitored using pfil(9) hooks.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_if.c,v 1.4.4.3 2017/12/03 11:39:03 jdolecek Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <net/if.h>
#endif

#include "npf_impl.h"

typedef struct npf_ifmap {
	char		n_ifname[IFNAMSIZ];
} npf_ifmap_t;

void
npf_ifmap_init(npf_t *npf, const npf_ifops_t *ifops)
{
	const size_t nbytes = sizeof(npf_ifmap_t) * NPF_MAX_IFMAP;

	KASSERT(ifops != NULL);
	ifops->flush((void *)(uintptr_t)0);

	npf->ifmap = kmem_zalloc(nbytes, KM_SLEEP);
	npf->ifmap_cnt = 0;
	npf->ifops = ifops;
}

void
npf_ifmap_fini(npf_t *npf)
{
	const size_t nbytes = sizeof(npf_ifmap_t) * NPF_MAX_IFMAP;
	kmem_free(npf->ifmap, nbytes);
}

static u_int
npf_ifmap_new(npf_t *npf)
{
	KASSERT(npf_config_locked_p(npf));

	for (u_int i = 0; i < npf->ifmap_cnt; i++)
		if (npf->ifmap[i].n_ifname[0] == '\0')
			return i + 1;

	if (npf->ifmap_cnt == NPF_MAX_IFMAP) {
		printf("npf_ifmap_new: out of slots; bump NPF_MAX_IFMAP\n");
		return 0;
	}
	return ++npf->ifmap_cnt;
}

static u_int
npf_ifmap_lookup(npf_t *npf, const char *ifname)
{
	KASSERT(npf_config_locked_p(npf));

	for (u_int i = 0; i < npf->ifmap_cnt; i++) {
		npf_ifmap_t *nim = &npf->ifmap[i];

		if (nim->n_ifname[0] && strcmp(nim->n_ifname, ifname) == 0)
			return i + 1;
	}
	return 0;
}

u_int
npf_ifmap_register(npf_t *npf, const char *ifname)
{
	npf_ifmap_t *nim;
	ifnet_t *ifp;
	u_int i;

	npf_config_enter(npf);
	if ((i = npf_ifmap_lookup(npf, ifname)) != 0) {
		goto out;
	}
	if ((i = npf_ifmap_new(npf)) == 0) {
		goto out;
	}
	nim = &npf->ifmap[i - 1];
	strlcpy(nim->n_ifname, ifname, IFNAMSIZ);

	if ((ifp = npf->ifops->lookup(ifname)) != NULL) {
		npf->ifops->setmeta(ifp, (void *)(uintptr_t)i);
	}
out:
	npf_config_exit(npf);
	return i;
}

void
npf_ifmap_flush(npf_t *npf)
{
	KASSERT(npf_config_locked_p(npf));

	for (u_int i = 0; i < npf->ifmap_cnt; i++) {
		npf->ifmap[i].n_ifname[0] = '\0';
	}
	npf->ifmap_cnt = 0;
	npf->ifops->flush((void *)(uintptr_t)0);
}

u_int
npf_ifmap_getid(npf_t *npf, const ifnet_t *ifp)
{
	const u_int i = (uintptr_t)npf->ifops->getmeta(ifp);
	KASSERT(i <= npf->ifmap_cnt);
	return i;
}

/*
 * This function is toxic; it can return garbage since we don't
 * lock, but it is only used temporarily and only for logging.
 */
void
npf_ifmap_copyname(npf_t *npf, u_int id, char *buf, size_t len)
{
	if (id > 0 && id < npf->ifmap_cnt)
		strlcpy(buf, npf->ifmap[id - 1].n_ifname,
		    MIN(len, sizeof(npf->ifmap[id - 1].n_ifname)));
	else
		strlcpy(buf, "???", len);
}

const char *
npf_ifmap_getname(npf_t *npf, const u_int id)
{
	const char *ifname;

	KASSERT(npf_config_locked_p(npf));
	KASSERT(id > 0 && id <= npf->ifmap_cnt);

	ifname = npf->ifmap[id - 1].n_ifname;
	KASSERT(ifname[0] != '\0');
	return ifname;
}

__dso_public void
npf_ifmap_attach(npf_t *npf, ifnet_t *ifp)
{
	const npf_ifops_t *ifops = npf->ifops;
	u_int i;

	npf_config_enter(npf);
	i = npf_ifmap_lookup(npf, ifops->getname(ifp));
	ifops->setmeta(ifp, (void *)(uintptr_t)i);
	npf_config_exit(npf);
}

__dso_public void
npf_ifmap_detach(npf_t *npf, ifnet_t *ifp)
{
	/* Diagnostic. */
	npf_config_enter(npf);
	npf->ifops->setmeta(ifp, (void *)(uintptr_t)0);
	npf_config_exit(npf);
}
