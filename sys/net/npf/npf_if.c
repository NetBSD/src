/*-
 * Copyright (c) 2019 Mindaugas Rasiukevicius <rmind at noxt eu>
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
 * NPF network interface handling.
 *
 * NPF uses its own interface IDs (npf-if-id).  These IDs start from 1.
 * Zero is reserved to indicate "no interface" case or an interface of
 * no interest (i.e. not registered).
 *
 * This module provides an interface to primarily handle the following:
 *
 * - Bind a symbolic interface name to NPF interface ID.
 * - Associate NPF interface ID when the network interface is attached.
 *
 * When NPF configuration is (re)loaded, each referenced network interface
 * name is registered with a unique ID.  If the network interface is already
 * attached, then the ID is associated with it immediately; otherwise, IDs
 * are associated/disassociated on interface events which are monitored
 * using pfil(9) hooks.
 *
 * To avoid race conditions when an active NPF configuration is updated or
 * interfaces are detached/attached, the interface names are never removed
 * and therefore IDs are never re-assigned.  The only point when interface
 * names and IDs are cleared is when the configuration is flushed.
 *
 * A linear counter is used for IDs.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_if.c,v 1.13 2020/05/30 14:16:56 rmind Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kmem.h>
#include <net/if.h>
#endif

#include "npf_impl.h"

typedef struct npf_ifmap {
	char		ifname[IFNAMSIZ + 1];
} npf_ifmap_t;

#define	NPF_IFMAP_NOID			(0U)
#define	NPF_IFMAP_SLOT2ID(npf, slot)	((npf)->ifmap_off + (slot) + 1)
#define	NPF_IFMAP_ID2SLOT(npf, id)	\
    ((id) - atomic_load_relaxed(&(npf)->ifmap_off) - 1)

void
npf_ifmap_init(npf_t *npf, const npf_ifops_t *ifops)
{
	const size_t nbytes = sizeof(npf_ifmap_t) * NPF_MAX_IFMAP;

	KASSERT(ifops != NULL);
	ifops->flush(npf, (void *)(uintptr_t)0);

	mutex_init(&npf->ifmap_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	npf->ifmap = kmem_zalloc(nbytes, KM_SLEEP);
	npf->ifmap_cnt = 0;
	npf->ifmap_off = 0;
	npf->ifops = ifops;
}

void
npf_ifmap_fini(npf_t *npf)
{
	const size_t nbytes = sizeof(npf_ifmap_t) * NPF_MAX_IFMAP;
	mutex_destroy(&npf->ifmap_lock);
	kmem_free(npf->ifmap, nbytes);
}

static unsigned
npf_ifmap_lookup(npf_t *npf, const char *ifname)
{
	KASSERT(mutex_owned(&npf->ifmap_lock));

	for (unsigned i = 0; i < npf->ifmap_cnt; i++) {
		npf_ifmap_t *ifmap = &npf->ifmap[i];

		if (strcmp(ifmap->ifname, ifname) == 0) {
			return NPF_IFMAP_SLOT2ID(npf, i);
		}
	}
	return NPF_IFMAP_NOID;
}

/*
 * npf_ifmap_register: register an interface name; return an assigned
 * NPF network ID on success (non-zero).
 *
 * This routine is mostly called on NPF configuration (re)load for the
 * interfaces names referenced by the rules.
 */
unsigned
npf_ifmap_register(npf_t *npf, const char *ifname)
{
	npf_ifmap_t *ifmap;
	unsigned id, i;
	ifnet_t *ifp;

	mutex_enter(&npf->ifmap_lock);
	if ((id = npf_ifmap_lookup(npf, ifname)) != NPF_IFMAP_NOID) {
		goto out;
	}
	if (npf->ifmap_cnt == NPF_MAX_IFMAP) {
		printf("npf_ifmap_new: out of slots; bump NPF_MAX_IFMAP\n");
		id = NPF_IFMAP_NOID;
		goto out;
	}
	KASSERT(npf->ifmap_cnt < NPF_MAX_IFMAP);

	/* Allocate a new slot and convert and assign an ID. */
	i = npf->ifmap_cnt++;
	ifmap = &npf->ifmap[i];
	strlcpy(ifmap->ifname, ifname, IFNAMSIZ);
	id = NPF_IFMAP_SLOT2ID(npf, i);

	if ((ifp = npf->ifops->lookup(npf, ifname)) != NULL) {
		npf->ifops->setmeta(npf, ifp, (void *)(uintptr_t)id);
	}
out:
	mutex_exit(&npf->ifmap_lock);
	return id;
}

void
npf_ifmap_flush(npf_t *npf)
{
	mutex_enter(&npf->ifmap_lock);
	npf->ifops->flush(npf, (void *)(uintptr_t)NPF_IFMAP_NOID);
	for (unsigned i = 0; i < npf->ifmap_cnt; i++) {
		npf->ifmap[i].ifname[0] = '\0';
	}
	npf->ifmap_cnt = 0;

	/*
	 * Reset the ID counter if reaching the overflow; this is not
	 * realistic, but we maintain correctness.
	 */
	if (npf->ifmap_off < (UINT_MAX - NPF_MAX_IFMAP)) {
		npf->ifmap_off += NPF_MAX_IFMAP;
	} else {
		npf->ifmap_off = 0;
	}
	mutex_exit(&npf->ifmap_lock);
}

/*
 * npf_ifmap_getid: get the ID for the given network interface.
 *
 * => This routine is typically called from the packet handler when
 *    matching whether the packet is on particular network interface.
 *
 * => This routine is lock-free; if the NPF configuration is flushed
 *    while the packet is in-flight, the ID will not match because we
 *    keep the IDs linear.
 */
unsigned
npf_ifmap_getid(npf_t *npf, const ifnet_t *ifp)
{
	const unsigned id = (uintptr_t)npf->ifops->getmeta(npf, ifp);
	return id;
}

/*
 * npf_ifmap_copylogname: this function is toxic; it can return garbage
 * as we don't lock, but it is only used temporarily and only for logging.
 */
void
npf_ifmap_copylogname(npf_t *npf, unsigned id, char *buf, size_t len)
{
	const unsigned i = NPF_IFMAP_ID2SLOT(npf, id);

	membar_consumer();

	if (id != NPF_IFMAP_NOID && i < NPF_MAX_IFMAP) {
		/*
		 * Lock-free access is safe as there is an extra byte
		 * with a permanent NUL terminator at the end.
		 */
		const npf_ifmap_t *ifmap = &npf->ifmap[i];
		strlcpy(buf, ifmap->ifname, MIN(len, IFNAMSIZ));
	} else {
		strlcpy(buf, "???", len);
	}
}

void
npf_ifmap_copyname(npf_t *npf, unsigned id, char *buf, size_t len)
{
	mutex_enter(&npf->ifmap_lock);
	npf_ifmap_copylogname(npf, id, buf, len);
	mutex_exit(&npf->ifmap_lock);
}

__dso_public void
npfk_ifmap_attach(npf_t *npf, ifnet_t *ifp)
{
	const npf_ifops_t *ifops = npf->ifops;
	unsigned id;

	mutex_enter(&npf->ifmap_lock);
	id = npf_ifmap_lookup(npf, ifops->getname(npf, ifp));
	ifops->setmeta(npf, ifp, (void *)(uintptr_t)id);
	mutex_exit(&npf->ifmap_lock);
}

__dso_public void
npfk_ifmap_detach(npf_t *npf, ifnet_t *ifp)
{
	/* Diagnostic. */
	mutex_enter(&npf->ifmap_lock);
	npf->ifops->setmeta(npf, ifp, (void *)(uintptr_t)NPF_IFMAP_NOID);
	mutex_exit(&npf->ifmap_lock);
}
