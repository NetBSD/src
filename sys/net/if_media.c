/*	$NetBSD: if_media.c,v 1.54 2022/09/03 02:47:59 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1997
 *	Jonathan Stone and Jason R. Thorpe.  All rights reserved.
 *
 * This software is derived from information provided by Matt Thomas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jonathan Stone
 *	and Jason R. Thorpe for the NetBSD Project.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
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

/*
 * BSD/OS-compatible network interface media selection.
 *
 * Where it is safe to do so, this code strays slightly from the BSD/OS
 * design.  Software which uses the API (device drivers, basically)
 * shouldn't notice any difference.
 *
 * Many thanks to Matt Thomas for providing the information necessary
 * to implement this interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_media.c,v 1.54 2022/09/03 02:47:59 thorpej Exp $");

#define	__IFMEDIA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/kmem.h>

#include <net/if.h>
#include <net/if_media.h>

static void	ifmedia_status(struct ifmedia *, struct ifnet *,
		    struct ifmediareq *);
static struct ifmedia_entry *
		ifmedia_match_locked(struct ifmedia *, u_int, u_int);

/*
 * Compile-time options:
 * IFMEDIA_DEBUG:
 *	Turn on implementation-level debug printfs.
 * 	Useful for debugging newly-ported drivers.
 */

#ifdef IFMEDIA_DEBUG
int	ifmedia_debug = 0;
static	void ifmedia_printword(int);
#endif

/*
 * We need to implement a recursive mutex to handle the un-converted
 * driver case.  For a fully MP-safe driver, the media lock will be
 * held before calling any of the entry points that require it.  However,
 * this is not necessarily the case for a driver that hasn't yet been
 * converted, and the entry point calls may be nested (for example
 * mii_ifmedia_change -> ether_mediachange -> mii_mediachg).  Luckily,
 * the nesting won't be very deep, and 4 nested holds should be plenty.
 */
#define	IFM_L_OWNLOCK		0x01
#define	IFM_L_COUNT_MASK	0x3UL
#define	IFM_L_CPU_MASK		~(IFM_L_COUNT_MASK)

void
ifmedia_lock_for_legacy(struct ifmedia *ifm)
{
	uintptr_t cnt = IFM_L_OWNLOCK;
	uintptr_t ci;

	if (mutex_tryenter(ifm->ifm_lock)) {
		goto gotit;
	}

	kpreempt_disable();
	ci = (uintptr_t)curcpu();
	if ((ifm->ifm_legacy & IFM_L_CPU_MASK) == ci) {
		cnt = ifm->ifm_legacy & IFM_L_COUNT_MASK;
		KASSERT(cnt < IFM_L_COUNT_MASK);
		cnt++;
		kpreempt_enable();
		goto gotit;
	}
	kpreempt_enable();

	mutex_enter(ifm->ifm_lock);
 gotit:
	KASSERT(kpreempt_disabled());
	ci = (uintptr_t)curcpu();
	KASSERT((ci & IFM_L_CPU_MASK) == ci);
	ifm->ifm_legacy = ci | cnt;
}

void
ifmedia_unlock_for_legacy(struct ifmedia *ifm)
{
	uintptr_t cnt;
	uintptr_t ci = (uintptr_t)curcpu();

	KASSERT(kpreempt_disabled());
	KASSERT((ifm->ifm_legacy & IFM_L_CPU_MASK) == ci);
	cnt = ifm->ifm_legacy & IFM_L_COUNT_MASK;
	KASSERT(cnt != 0);
	if (cnt == IFM_L_OWNLOCK) {
		ifm->ifm_legacy = IFM_L_OWNLOCK;
		mutex_exit(ifm->ifm_lock);
		return;
	}
	cnt--;
	ifm->ifm_legacy = ci | cnt;
}

/*
 * Initialize if_media struct for a specific interface instance.
 */
void
ifmedia_init_with_lock(struct ifmedia *ifm, int dontcare_mask,
    ifm_change_cb_t change_callback, ifm_stat_cb_t status_callback,
    kmutex_t *lock)
{

	/*
	 * XXX Would really like to assert:
	 *
	 *	!if_is_mpsafe(ifp) || ((if_is_mpsafe(ifp) && lock != NULL)
	 *
	 * ...but we don't have access to the ifnet here.
	 */

	TAILQ_INIT(&ifm->ifm_list);
	ifm->ifm_cur = NULL;
	ifm->ifm_media = IFM_NONE;
	ifm->ifm_mask = dontcare_mask;		/* IF don't-care bits */
	ifm->ifm_change = change_callback;
	ifm->ifm_status = status_callback;
	ifm->ifm_legacy = 0;

	if (lock == NULL) {
		/*
		 * This is to support drivers that are not yet MP-safe
		 * with regard to the ifmedia layer.  In these cases,
		 * we supply the lock and we ensure it's taken upon entry
		 * to various routines that expect it to be held.  When
		 * we do this, we expect that the driver is in general a
		 * non-MP-safe driver and has already gone to splnet().
		 */
		lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NET);
		ifm->ifm_legacy = IFM_L_OWNLOCK;
	}
	ifm->ifm_lock = lock;
}

void
ifmedia_init(struct ifmedia *ifm, int dontcare_mask,
    ifm_change_cb_t change_callback, ifm_stat_cb_t status_callback)
{
	ifmedia_init_with_lock(ifm, dontcare_mask, change_callback,
	    status_callback, NULL);
}

/*
 * Free resources associated with an ifmedia.
 */
void
ifmedia_fini(struct ifmedia *ifm)
{

	ifmedia_removeall(ifm);

	if (ifm->ifm_legacy) {
		KASSERT(ifm->ifm_legacy == IFM_L_OWNLOCK);
		mutex_obj_free(ifm->ifm_lock);
	}
	ifm->ifm_legacy = 0;
	ifm->ifm_lock = NULL;
}

int
ifmedia_change(struct ifmedia *ifm, struct ifnet *ifp)
{
	int rv;

	IFMEDIA_LOCK_FOR_LEGACY(ifm);
	KASSERT(ifmedia_locked(ifm));
	if (ifm->ifm_change)
		rv = (*ifm->ifm_change)(ifp);
	else
		rv = -1;
	IFMEDIA_UNLOCK_FOR_LEGACY(ifm);

	return rv;
}

static void
ifmedia_status(struct ifmedia *ifm, struct ifnet *ifp, struct ifmediareq *ifmr)
{

	KASSERT(ifmedia_locked(ifm));
	if (ifm->ifm_status == NULL)
		return;
	(*ifm->ifm_status)(ifp, ifmr);
}

/*
 * Add a media configuration to the list of supported media
 * for a specific interface instance.
 */
static void
ifmedia_add_entry(struct ifmedia *ifm, int mword, int data, void *aux,
    struct ifmedia_entry *entry)
{

#ifdef IFMEDIA_DEBUG
	if (ifmedia_debug) {
		if (ifm == NULL) {
			printf("ifmedia_add: null ifm\n");
			return;
		}
		printf("Adding entry for ");
		ifmedia_printword(mword);
	}
#endif

	entry->ifm_media = mword;
	entry->ifm_data = data;
	entry->ifm_aux = aux;
	TAILQ_INSERT_TAIL(&ifm->ifm_list, entry, ifm_list);
}

void
ifmedia_add(struct ifmedia *ifm, int mword, int data, void *aux)
{
	struct ifmedia_entry *entry;

	entry = kmem_zalloc(sizeof(*entry), KM_SLEEP);
	ifmedia_lock(ifm);
	ifmedia_add_entry(ifm, mword, data, aux, entry);
	ifmedia_unlock(ifm);
}

/*
 * Add an array of media configurations to the list of
 * supported media for a specific interface instance.
 */
void
ifmedia_list_add(struct ifmedia *ifm, struct ifmedia_entry *lp, int count)
{
	int i;

	for (i = 0; i < count; i++)
		ifmedia_add(ifm, lp[i].ifm_media, lp[i].ifm_data,
		    lp[i].ifm_aux);
}

/*
 * Set the default active media.
 *
 * Called by device-specific code which is assumed to have already
 * selected the default media in hardware.  We do _not_ call the
 * media-change callback.
 */
void
ifmedia_set(struct ifmedia *ifm, int target)
{
	struct ifmedia_entry *match, *entry = NULL;

	ifmedia_lock(ifm);
	match = ifmedia_match_locked(ifm, target, ifm->ifm_mask);

	/*
	 * If we didn't find the requested media, then we try to fall
	 * back to target-type (IFM_ETHER, e.g.) | IFM_NONE.  If that's
	 * not on the list, then we add it and set the media to it.
	 *
	 * Since ifmedia_set is almost always called with IFM_AUTO or
	 * with a known-good media, this really should only occur if we:
	 *
	 * a) didn't find any PHYs, or
	 * b) didn't find an autoselect option on the PHY when the
	 *    parent ethernet driver expected to.
	 *
	 * In either case, it makes sense to select no media.
	 */
	if (match == NULL) {
		printf("ifmedia_set: no match for 0x%x/0x%x\n",
		    target, ~ifm->ifm_mask);
		target = (target & IFM_NMASK) | IFM_NONE;
		match = ifmedia_match_locked(ifm, target, ifm->ifm_mask);
		if (match == NULL) {
			ifmedia_unlock(ifm);
			entry = kmem_zalloc(sizeof(*entry), KM_SLEEP);
			ifmedia_lock(ifm);
			match = ifmedia_match_locked(ifm, target,
			    ifm->ifm_mask);
			if (match == NULL) {
				ifmedia_add_entry(ifm, target, 0, NULL, entry);
				entry = NULL;
			}
			match = ifmedia_match_locked(ifm, target,
			    ifm->ifm_mask);
			if (match == NULL)
				panic("ifmedia_set failed");
		}
	}
	ifm->ifm_cur = match;
	ifmedia_unlock(ifm);

	if (entry)
		kmem_free(entry, sizeof(*entry));

#ifdef IFMEDIA_DEBUG
	if (ifmedia_debug) {
		printf("ifmedia_set: target ");
		ifmedia_printword(target);
		printf("ifmedia_set: setting to ");
		ifmedia_printword(ifm->ifm_cur->ifm_media);
	}
#endif
}

static int
ifmedia_getwords(struct ifmedia * const ifm, int *words, int maxwords)
{
	struct ifmedia_entry *ep;
	int nwords = 0;

	KASSERT(ifmedia_locked(ifm));

	TAILQ_FOREACH(ep, &ifm->ifm_list, ifm_list) {
		if (words != NULL && nwords < maxwords) {
			words[nwords] = ep->ifm_media;
		}
		nwords++;
	}

	return nwords;
}

#define	IFMEDIA_IOCTL_LOCK(ifm)						\
do {									\
	if (ifmedia_islegacy(ifm))					\
		ifmedia_lock_for_legacy(ifm);				\
	else								\
		ifmedia_lock(ifm);					\
} while (/*CONSTCOND*/0)

#define	IFMEDIA_IOCTL_UNLOCK(ifm)					\
do {									\
	if (ifmedia_islegacy(ifm))					\
		ifmedia_unlock_for_legacy(ifm);				\
	else								\
		ifmedia_unlock(ifm);					\
} while (/*CONSTCOND*/0)

/*
 * Device-independent media ioctl support function.
 */
int
ifmedia_ioctl(struct ifnet *ifp, struct ifreq *ifr, struct ifmedia *ifm,
    u_long cmd)
{
	struct ifmedia_entry *match;
	struct ifmediareq *ifmr = (struct ifmediareq *)ifr;
	int error = 0;

	if (ifp == NULL || ifr == NULL || ifm == NULL)
		return EINVAL;

	KERNEL_LOCK_UNLESS_IFP_MPSAFE(ifp);

	switch (cmd) {
	case SIOCSIFMEDIA:	/* Set the current media. */
	{
		struct ifmedia_entry *oldentry;
		u_int oldmedia;
		u_int newmedia = ifr->ifr_media;

		IFMEDIA_IOCTL_LOCK(ifm);

		match = ifmedia_match_locked(ifm, newmedia, ifm->ifm_mask);
		if (match == NULL) {
#ifdef IFMEDIA_DEBUG
			if (ifmedia_debug) {
				printf("ifmedia_ioctl: no media found for "
				    "0x%08x\n", newmedia);
			}
#endif
			IFMEDIA_IOCTL_UNLOCK(ifm);
			error = EINVAL;
			break;
		}

		/*
		 * If no change, we're done.
		 * XXX Automedia may involve software intervention.
		 *     Keep going in case the connected media changed.
		 *     Similarly, if best match changed (kernel debugger?).
		 */
		if ((IFM_SUBTYPE(newmedia) != IFM_AUTO) &&
		    (newmedia == ifm->ifm_media) && (match == ifm->ifm_cur)) {
			IFMEDIA_IOCTL_UNLOCK(ifm);
			break;
		}

		/*
		 * We found a match, now make the driver switch to it.
		 * Make sure to preserve our old media type in case the
		 * driver can't switch.
		 */
#ifdef IFMEDIA_DEBUG
		if (ifmedia_debug) {
			printf("ifmedia_ioctl: switching %s to ",
			    ifp->if_xname);
			ifmedia_printword(match->ifm_media);
		}
#endif
		oldentry = ifm->ifm_cur;
		oldmedia = ifm->ifm_media;
		ifm->ifm_cur = match;
		ifm->ifm_media = newmedia;
		error = ifmedia_change(ifm, ifp);
		if (error) {
			ifm->ifm_cur = oldentry;
			ifm->ifm_media = oldmedia;
		}
		IFMEDIA_IOCTL_UNLOCK(ifm);
		break;
	}

	/* Get list of available media and current media on interface. */
	case SIOCGIFMEDIA:
	{
		int nwords1, nwords2;

		if (ifmr->ifm_count < 0) {
			error = EINVAL;
			break;
		}

		IFMEDIA_IOCTL_LOCK(ifm);
		ifmr->ifm_active = ifmr->ifm_current = ifm->ifm_cur ?
		    ifm->ifm_cur->ifm_media : IFM_NONE;
		ifmr->ifm_mask = ifm->ifm_mask;
		ifmr->ifm_status = 0;
		ifmedia_status(ifm, ifp, ifmr);

		/*
		 * Count them so we know how much is the max we'll
		 * need.
		 */
		nwords1 = nwords2 = ifmedia_getwords(ifm, NULL, 0);
		IFMEDIA_IOCTL_UNLOCK(ifm);

		if (ifmr->ifm_count != 0) {
			int maxwords = MIN(nwords1, ifmr->ifm_count);
			int *kptr = kmem_zalloc(maxwords * sizeof(int),
			    KM_SLEEP);

			ifmedia_lock(ifm);
			nwords2 = ifmedia_getwords(ifm, kptr, maxwords);
			ifmedia_unlock(ifm);
			error = copyout(kptr, ifmr->ifm_ulist,
			    maxwords * sizeof(int));
			if (error == 0 && nwords2 > nwords1)
				error = E2BIG;	/* oops! */
			kmem_free(kptr, maxwords * sizeof(int));
		}
		/* Update with the real number */
		ifmr->ifm_count = nwords2;
		break;
	}

	default:
		error = EINVAL;
		break;
	}

	KERNEL_UNLOCK_UNLESS_IFP_MPSAFE(ifp);

	return error;
}

/*
 * Find media entry matching a given ifm word.
 */
static struct ifmedia_entry *
ifmedia_match_locked(struct ifmedia *ifm, u_int target, u_int mask)
{
	struct ifmedia_entry *match, *next;

	match = NULL;
	mask = ~mask;

	TAILQ_FOREACH(next, &ifm->ifm_list, ifm_list) {
		if ((next->ifm_media & mask) == (target & mask)) {
			if (match) {
#if defined(IFMEDIA_DEBUG) || defined(DIAGNOSTIC)
				printf("ifmedia_match: multiple match for "
				    "0x%x/0x%x, selected instance %d\n",
				    target, mask, IFM_INST(match->ifm_media));
#endif
				break;
			}
			match = next;
		}
	}

	return match;
}

struct ifmedia_entry *
ifmedia_match(struct ifmedia *ifm, u_int target, u_int mask)
{
	struct ifmedia_entry *match;

	/*
	 * N.B. We expect the caller is responsible fot the lifecycle
	 * of the media entries.  Use with extreme caution.
	 */

	ifmedia_lock(ifm);
	match = ifmedia_match_locked(ifm, target, mask);
	ifmedia_unlock(ifm);
	return match;
}

/*
 * Delete all media for a given instance.
 */
void
ifmedia_delete_instance(struct ifmedia *ifm, u_int inst)
{
	struct ifmedia_entry *ife, *nife;
	TAILQ_HEAD(, ifmedia_entry) dead_entries;

	TAILQ_INIT(&dead_entries);

	ifmedia_lock(ifm);
	TAILQ_FOREACH_SAFE(ife, &ifm->ifm_list, ifm_list, nife) {
		if (inst == IFM_INST_ANY ||
		    inst == IFM_INST(ife->ifm_media)) {
			if (ifm->ifm_cur == ife) {
				ifm->ifm_cur = NULL;
				ifm->ifm_media = IFM_NONE;
			}
			TAILQ_REMOVE(&ifm->ifm_list, ife, ifm_list);
			TAILQ_INSERT_TAIL(&dead_entries, ife, ifm_list);
		}
	}
	ifmedia_unlock(ifm);

	TAILQ_FOREACH_SAFE(ife, &dead_entries, ifm_list, nife) {
		TAILQ_REMOVE(&dead_entries, ife, ifm_list);
		kmem_free(ife, sizeof(*ife));
	}
}

void
ifmedia_removeall(struct ifmedia *ifm)
{

	ifmedia_delete_instance(ifm, IFM_INST_ANY);
}

/*
 * Compute the interface `baudrate' from the media, for the interface
 * metrics (used by routing daemons).
 */
static const struct ifmedia_baudrate ifmedia_baudrate_descriptions[] =
    IFM_BAUDRATE_DESCRIPTIONS;

uint64_t
ifmedia_baudrate(int mword)
{
	int i;

	for (i = 0; ifmedia_baudrate_descriptions[i].ifmb_word != 0; i++) {
		if (IFM_TYPE_SUBTYPE_MATCH(mword,
		    ifmedia_baudrate_descriptions[i].ifmb_word))
			return ifmedia_baudrate_descriptions[i].ifmb_baudrate;
	}

	/* Not known. */
	return 0;
}

#ifdef IFMEDIA_DEBUG

static const struct ifmedia_description ifm_type_descriptions[] =
    IFM_TYPE_DESCRIPTIONS;

static const struct ifmedia_description ifm_subtype_descriptions[] =
    IFM_SUBTYPE_DESCRIPTIONS;

static const struct ifmedia_description ifm_option_descriptions[] =
    IFM_OPTION_DESCRIPTIONS;

/*
 * print a media word.
 */
static void
ifmedia_printword(int ifmw)
{
	const struct ifmedia_description *desc;
	int seen_option = 0;

	/* Print the top-level interface type. */
	for (desc = ifm_type_descriptions; desc->ifmt_string != NULL;
	     desc++) {
		if (IFM_TYPE(ifmw) == desc->ifmt_word)
			break;
	}
	if (desc->ifmt_string == NULL)
		printf("<unknown type> ");
	else
		printf("%s ", desc->ifmt_string);

	/* Print the subtype. */
	for (desc = ifm_subtype_descriptions; desc->ifmt_string != NULL;
	     desc++) {
		if (IFM_TYPE_MATCH(desc->ifmt_word, ifmw) &&
		    IFM_SUBTYPE(desc->ifmt_word) == IFM_SUBTYPE(ifmw))
			break;
	}
	if (desc->ifmt_string == NULL)
		printf("<unknown subtype>");
	else
		printf("%s", desc->ifmt_string);

	/* Print any options. */
	for (desc = ifm_option_descriptions; desc->ifmt_string != NULL;
	     desc++) {
		if (IFM_TYPE_MATCH(desc->ifmt_word, ifmw) &&
		    (ifmw & desc->ifmt_word) != 0 &&
		    (seen_option & IFM_OPTIONS(desc->ifmt_word)) == 0) {
			if (seen_option == 0)
				printf(" <");
			printf("%s%s", seen_option ? "," : "",
			    desc->ifmt_string);
			seen_option |= IFM_OPTIONS(desc->ifmt_word);
		}
	}
	printf("%s\n", seen_option ? ">" : "");
}

#endif /* IFMEDIA_DEBUG */
