/*	$NetBSD: mii.c,v 1.58 2022/08/14 20:34:26 riastradh Exp $	*/

/*-
 * Copyright (c) 1998, 2000, 2020 The NetBSD Foundation, Inc.
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
 * MII bus layer, glues MII-capable network interface drivers to sharable
 * PHY drivers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mii.c,v 1.58 2022/08/14 20:34:26 riastradh Exp $");

#define	__IFMEDIA_PRIVATE

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include "locators.h"

static int	mii_print(void *, const char *);

/*
 * Helper function used by network interface drivers, attaches PHYs
 * to the network interface driver parent.
 */
void
mii_attach(device_t parent, struct mii_data *mii, int capmask,
    int phyloc, int offloc, int flags)
{
	struct mii_attach_args ma;
	struct mii_softc *child;
	int offset = 0;
	uint16_t bmsr;
	int phymin, phymax;
	int locs[MIICF_NLOCS];
	int rv;

	KASSERT(mii->mii_media.ifm_lock != NULL);

	if (phyloc != MII_PHY_ANY && offloc != MII_OFFSET_ANY)
		panic("mii_attach: phyloc and offloc specified");

	if (phyloc == MII_PHY_ANY) {
		phymin = 0;
		phymax = MII_NPHY - 1;
	} else
		phymin = phymax = phyloc;

	mii_lock(mii);

	if ((mii->mii_flags & MIIF_INITDONE) == 0) {
		LIST_INIT(&mii->mii_phys);
		cv_init(&mii->mii_probe_cv, "mii_attach");
		mii->mii_flags |= MIIF_INITDONE;
	}

	/*
	 * Probing temporarily unlocks the MII; wait until anyone
	 * else who might be doing this to finish.
	 */
	mii->mii_probe_waiters++;
	for (;;) {
		if (mii->mii_flags & MIIF_EXITING) {
			mii->mii_probe_waiters--;
			cv_signal(&mii->mii_probe_cv);
			mii_unlock(mii);
			return;
		}
		if ((mii->mii_flags & MIIF_PROBING) == 0)
			break;
		cv_wait(&mii->mii_probe_cv, mii->mii_media.ifm_lock);
	}
	mii->mii_probe_waiters--;

	/* ...and old others off. */
	mii->mii_flags |= MIIF_PROBING;

	for (ma.mii_phyno = phymin; ma.mii_phyno <= phymax; ma.mii_phyno++) {
		/*
		 * Make sure we haven't already configured a PHY at this
		 * address.  This allows mii_attach() to be called
		 * multiple times.
		 */
		LIST_FOREACH(child, &mii->mii_phys, mii_list) {
			if (child->mii_phy == ma.mii_phyno) {
				/*
				 * Yes, there is already something
				 * configured at this address.
				 */
				offset++;
				continue;
			}
		}

		/*
		 * Check to see if there is a PHY at this address.  Note,
		 * many braindead PHYs report 0/0 in their ID registers,
		 * so we test for media in the BMSR.
		 */
		bmsr = 0;
		rv = (*mii->mii_readreg)(parent, ma.mii_phyno, MII_BMSR,
		    &bmsr);
		if ((rv != 0) || bmsr == 0 || bmsr == 0xffff ||
		    (bmsr & (BMSR_EXTSTAT | BMSR_MEDIAMASK)) == 0) {
			/* Assume no PHY at this address. */
			continue;
		}

		/*
		 * There is a PHY at this address.  If we were given an
		 * `offset' locator, skip this PHY if it doesn't match.
		 */
		if (offloc != MII_OFFSET_ANY && offloc != offset) {
			offset++;
			continue;
		}

		/*
		 * Extract the IDs.  Braindead PHYs will be handled by
		 * the `ukphy' driver, as we have no ID information to
		 * match on.
		 */
		ma.mii_id1 = ma.mii_id2 = 0;
		rv = (*mii->mii_readreg)(parent, ma.mii_phyno,
		    MII_PHYIDR1, &ma.mii_id1);
		rv |= (*mii->mii_readreg)(parent, ma.mii_phyno,
		    MII_PHYIDR2, &ma.mii_id2);
		if (rv != 0)
			continue;

		ma.mii_data = mii;
		ma.mii_capmask = capmask;
		ma.mii_flags = flags | (mii->mii_flags & MIIF_INHERIT_MASK);

		locs[MIICF_PHY] = ma.mii_phyno;

		mii_unlock(mii);

		child = device_private(
		    config_found(parent, &ma, mii_print,
				 CFARGS(.submatch = config_stdsubmatch,
					.iattr = "mii",
					.locators = locs)));
		if (child) {
			/* Link it up in the parent's MII data. */
			if (child->mii_flags & MIIF_AUTOTSLEEP)
				cv_init(&child->mii_nway_cv, "miiauto");
			else
				callout_init(&child->mii_nway_ch, 0);
			mii_lock(mii);
			LIST_INSERT_HEAD(&mii->mii_phys, child, mii_list);
			child->mii_offset = offset;
			mii->mii_instance++;
		} else {
			mii_lock(mii);
		}
		offset++;
	}

	/* All done! */
	mii->mii_flags &= ~MIIF_PROBING;
	cv_signal(&mii->mii_probe_cv);
	mii_unlock(mii);
}

void
mii_detach(struct mii_data *mii, int phyloc, int offloc)
{
	struct mii_softc *child, *nchild;
	bool exiting = false;

	if (phyloc != MII_PHY_ANY && offloc != MII_PHY_ANY)
		panic("mii_detach: phyloc and offloc specified");

	mii_lock(mii);

	if ((mii->mii_flags & MIIF_INITDONE) == 0 ||
	    (mii->mii_flags & MIIF_EXITING) != 0) {
		mii_unlock(mii);
		return;
	}

	/*
	 * XXX This is probably not the best hueristic.  Consider
	 * XXX adding an argument to mii_detach().
	 */
	if (phyloc == MII_PHY_ANY && MII_PHY_ANY == MII_OFFSET_ANY)
		exiting = true;

	if (exiting) {
		mii->mii_flags |= MIIF_EXITING;
		cv_broadcast(&mii->mii_probe_cv);
	}

	/* Wait for everyone else to get out. */
	mii->mii_probe_waiters++;
	for (;;) {
		/*
		 * If we've been waiting to do a less-than-exit, and
		 * *someone else* initiated an exit, then get out.
		 */
		if (!exiting && (mii->mii_flags & MIIF_EXITING) != 0) {
			mii->mii_probe_waiters--;
			cv_signal(&mii->mii_probe_cv);
			mii_unlock(mii);
			return;
		}
		if ((mii->mii_flags & MIIF_PROBING) == 0 &&
		    (!exiting || (exiting && mii->mii_probe_waiters == 1)))
			break;
		cv_wait(&mii->mii_probe_cv, mii->mii_media.ifm_lock);
	}
	mii->mii_probe_waiters--;

	if (!exiting)
		mii->mii_flags |= MIIF_PROBING;

	LIST_FOREACH_SAFE(child, &mii->mii_phys, mii_list, nchild) {
		if (phyloc != MII_PHY_ANY || offloc != MII_OFFSET_ANY) {
			if (phyloc != MII_PHY_ANY &&
			    phyloc != child->mii_phy)
				continue;
			if (offloc != MII_OFFSET_ANY &&
			    offloc != child->mii_offset)
				continue;
		}
		LIST_REMOVE(child, mii_list);
		mii_unlock(mii);
		(void)config_detach(child->mii_dev, DETACH_FORCE);
		mii_lock(mii);
	}

	if (exiting) {
		cv_destroy(&mii->mii_probe_cv);
	} else {
		mii->mii_flags &= ~MIIF_PROBING;
		cv_signal(&mii->mii_probe_cv);
	}

	mii_unlock(mii);
}

static int
mii_print(void *aux, const char *pnp)
{
	struct mii_attach_args *ma = aux;

	if (pnp != NULL)
		aprint_normal("OUI 0x%06x model 0x%04x rev %d at %s",
		    MII_OUI(ma->mii_id1, ma->mii_id2), MII_MODEL(ma->mii_id2),
		    MII_REV(ma->mii_id2), pnp);

	aprint_normal(" phy %d", ma->mii_phyno);
	return UNCONF;
}

static inline int
phy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{

	if (!device_is_active(sc->mii_dev))
		return ENXIO;
	return PHY_SERVICE(sc, mii, cmd);
}

int
mii_ifmedia_change(struct mii_data *mii)
{

	KASSERT(mii_locked(mii) ||
		ifmedia_islegacy(&mii->mii_media));
	return ifmedia_change(&mii->mii_media, mii->mii_ifp);
}

/*
 * Media changed; notify all PHYs.
 */
int
mii_mediachg(struct mii_data *mii)
{
	struct mii_softc *child;
	int rv = 0;

	IFMEDIA_LOCK_FOR_LEGACY(&mii->mii_media);
	KASSERT(mii_locked(mii));

	mii->mii_media_status = 0;
	mii->mii_media_active = IFM_NONE;

	LIST_FOREACH(child, &mii->mii_phys, mii_list) {
		rv = phy_service(child, mii, MII_MEDIACHG);
		if (rv)
			break;
	}
	IFMEDIA_UNLOCK_FOR_LEGACY(&mii->mii_media);
	return rv;
}

/*
 * Call the PHY tick routines, used during autonegotiation.
 */
void
mii_tick(struct mii_data *mii)
{
	struct mii_softc *child;

	IFMEDIA_LOCK_FOR_LEGACY(&mii->mii_media);
	KASSERT(mii_locked(mii));

	LIST_FOREACH(child, &mii->mii_phys, mii_list)
		(void)phy_service(child, mii, MII_TICK);

	IFMEDIA_UNLOCK_FOR_LEGACY(&mii->mii_media);
}

/*
 * Get media status from PHYs.
 */
void
mii_pollstat(struct mii_data *mii)
{
	struct mii_softc *child;

	IFMEDIA_LOCK_FOR_LEGACY(&mii->mii_media);
	KASSERT(mii_locked(mii));

	mii->mii_media_status = 0;
	mii->mii_media_active = IFM_NONE;

	LIST_FOREACH(child, &mii->mii_phys, mii_list)
		(void)phy_service(child, mii, MII_POLLSTAT);

	IFMEDIA_UNLOCK_FOR_LEGACY(&mii->mii_media);
}

/*
 * Inform the PHYs that the interface is down.
 */
void
mii_down(struct mii_data *mii)
{
	struct mii_softc *child;

	IFMEDIA_LOCK_FOR_LEGACY(&mii->mii_media);
	KASSERT(mii_locked(mii));

	LIST_FOREACH(child, &mii->mii_phys, mii_list)
		(void)phy_service(child, mii, MII_DOWN);

	IFMEDIA_UNLOCK_FOR_LEGACY(&mii->mii_media);
}

static unsigned char
bitreverse(unsigned char x)
{
	static const unsigned char nibbletab[16] = {
		0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
	};

	return ((nibbletab[x & 15] << 4) | nibbletab[x >> 4]);
}

u_int
mii_oui(uint16_t id1, uint16_t id2)
{
	u_int h;

	h = (id1 << 6) | (id2 >> 10);

	return ((bitreverse(h >> 16) << 16) |
		(bitreverse((h >> 8) & 255) << 8) |
		bitreverse(h & 255));
}
