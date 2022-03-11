/*	$NetBSD: rf_netbsdkintf.c,v 1.403 2022/03/11 01:59:33 mrg Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2008-2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Greg Oster; Jason R. Thorpe.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cd.c 1.6 90/11/28$
 *
 *      @(#)cd.c        8.2 (Berkeley) 11/16/93
 */

/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Mark Holland, Jim Zelenka
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/***********************************************************
 *
 * rf_kintf.c -- the kernel interface routines for RAIDframe
 *
 ***********************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rf_netbsdkintf.c,v 1.403 2022/03/11 01:59:33 mrg Exp $");

#ifdef _KERNEL_OPT
#include "opt_raid_autoconfig.h"
#include "opt_compat_netbsd32.h"
#endif

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/disk.h>
#include <sys/device.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/reboot.h>
#include <sys/kauth.h>
#include <sys/module.h>
#include <sys/compat_stub.h>

#include <prop/proplib.h>

#include <dev/raidframe/raidframevar.h>
#include <dev/raidframe/raidframeio.h>
#include <dev/raidframe/rf_paritymap.h>

#include "rf_raid.h"
#include "rf_copyback.h"
#include "rf_dag.h"
#include "rf_dagflags.h"
#include "rf_desc.h"
#include "rf_diskqueue.h"
#include "rf_etimer.h"
#include "rf_general.h"
#include "rf_kintf.h"
#include "rf_options.h"
#include "rf_driver.h"
#include "rf_parityscan.h"
#include "rf_threadstuff.h"

#include "ioconf.h"

#ifdef DEBUG
int     rf_kdebug_level = 0;
#define db1_printf(a) if (rf_kdebug_level > 0) printf a
#else				/* DEBUG */
#define db1_printf(a) { }
#endif				/* DEBUG */

#if (RF_INCLUDE_PARITY_DECLUSTERING_DS > 0)
static rf_declare_mutex2(rf_sparet_wait_mutex);
static rf_declare_cond2(rf_sparet_wait_cv);
static rf_declare_cond2(rf_sparet_resp_cv);

static RF_SparetWait_t *rf_sparet_wait_queue;	/* requests to install a
						 * spare table */
static RF_SparetWait_t *rf_sparet_resp_queue;	/* responses from
						 * installation process */
#endif

const int rf_b_pass = (B_PHYS|B_RAW|B_MEDIA_FLAGS);

MALLOC_DEFINE(M_RAIDFRAME, "RAIDframe", "RAIDframe structures");

/* prototypes */
static void KernelWakeupFunc(struct buf *);
static void InitBP(struct buf *, struct vnode *, unsigned,
    dev_t, RF_SectorNum_t, RF_SectorCount_t, void *, void (*) (struct buf *),
    void *, int);
static void raidinit(struct raid_softc *);
static int raiddoaccess(RF_Raid_t *raidPtr, struct buf *bp);
static int rf_get_component_caches(RF_Raid_t *raidPtr, int *);

static int raid_match(device_t, cfdata_t, void *);
static void raid_attach(device_t, device_t, void *);
static int raid_detach(device_t, int);

static int raidread_component_area(dev_t, struct vnode *, void *, size_t,
    daddr_t, daddr_t);
static int raidwrite_component_area(dev_t, struct vnode *, void *, size_t,
    daddr_t, daddr_t, int);

static int raidwrite_component_label(unsigned,
    dev_t, struct vnode *, RF_ComponentLabel_t *);
static int raidread_component_label(unsigned,
    dev_t, struct vnode *, RF_ComponentLabel_t *);

static int raid_diskstart(device_t, struct buf *bp);
static int raid_dumpblocks(device_t, void *, daddr_t, int);
static int raid_lastclose(device_t);

static dev_type_open(raidopen);
static dev_type_close(raidclose);
static dev_type_read(raidread);
static dev_type_write(raidwrite);
static dev_type_ioctl(raidioctl);
static dev_type_strategy(raidstrategy);
static dev_type_dump(raiddump);
static dev_type_size(raidsize);

const struct bdevsw raid_bdevsw = {
	.d_open = raidopen,
	.d_close = raidclose,
	.d_strategy = raidstrategy,
	.d_ioctl = raidioctl,
	.d_dump = raiddump,
	.d_psize = raidsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw raid_cdevsw = {
	.d_open = raidopen,
	.d_close = raidclose,
	.d_read = raidread,
	.d_write = raidwrite,
	.d_ioctl = raidioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

static struct dkdriver rf_dkdriver = {
	.d_open = raidopen,
	.d_close = raidclose,
	.d_strategy = raidstrategy,
	.d_diskstart = raid_diskstart,
	.d_dumpblocks = raid_dumpblocks,
	.d_lastclose = raid_lastclose,
	.d_minphys = minphys
};

#define	raidunit(x)	DISKUNIT(x)
#define	raidsoftc(dev)	(((struct raid_softc *)device_private(dev))->sc_r.softc)

extern struct cfdriver raid_cd;
CFATTACH_DECL3_NEW(raid, sizeof(struct raid_softc),
    raid_match, raid_attach, raid_detach, NULL, NULL, NULL,
    DVF_DETACH_SHUTDOWN);

/* Internal representation of a rf_recon_req */
struct rf_recon_req_internal {
	RF_RowCol_t col;
	RF_ReconReqFlags_t flags;
	void   *raidPtr;
};

/*
 * Allow RAIDOUTSTANDING number of simultaneous IO's to this RAID device.
 * Be aware that large numbers can allow the driver to consume a lot of
 * kernel memory, especially on writes, and in degraded mode reads.
 *
 * For example: with a stripe width of 64 blocks (32k) and 5 disks,
 * a single 64K write will typically require 64K for the old data,
 * 64K for the old parity, and 64K for the new parity, for a total
 * of 192K (if the parity buffer is not re-used immediately).
 * Even it if is used immediately, that's still 128K, which when multiplied
 * by say 10 requests, is 1280K, *on top* of the 640K of incoming data.
 *
 * Now in degraded mode, for example, a 64K read on the above setup may
 * require data reconstruction, which will require *all* of the 4 remaining
 * disks to participate -- 4 * 32K/disk == 128K again.
 */

#ifndef RAIDOUTSTANDING
#define RAIDOUTSTANDING   6
#endif

#define RAIDLABELDEV(dev)	\
	(MAKEDISKDEV(major((dev)), raidunit((dev)), RAW_PART))

/* declared here, and made public, for the benefit of KVM stuff.. */

static int raidlock(struct raid_softc *);
static void raidunlock(struct raid_softc *);

static int raid_detach_unlocked(struct raid_softc *);

static void rf_markalldirty(RF_Raid_t *);
static void rf_set_geometry(struct raid_softc *, RF_Raid_t *);

static void rf_ReconThread(struct rf_recon_req_internal *);
static void rf_RewriteParityThread(RF_Raid_t *raidPtr);
static void rf_CopybackThread(RF_Raid_t *raidPtr);
static void rf_ReconstructInPlaceThread(struct rf_recon_req_internal *);
static int rf_autoconfig(device_t);
static int rf_rescan(void);
static void rf_buildroothack(RF_ConfigSet_t *);

static RF_AutoConfig_t *rf_find_raid_components(void);
static RF_ConfigSet_t *rf_create_auto_sets(RF_AutoConfig_t *);
static int rf_does_it_fit(RF_ConfigSet_t *,RF_AutoConfig_t *);
static void rf_create_configuration(RF_AutoConfig_t *,RF_Config_t *, RF_Raid_t *);
static int rf_set_autoconfig(RF_Raid_t *, int);
static int rf_set_rootpartition(RF_Raid_t *, int);
static void rf_release_all_vps(RF_ConfigSet_t *);
static void rf_cleanup_config_set(RF_ConfigSet_t *);
static int rf_have_enough_components(RF_ConfigSet_t *);
static struct raid_softc *rf_auto_config_set(RF_ConfigSet_t *);
static void rf_fix_old_label_size(RF_ComponentLabel_t *, uint64_t);

/*
 * Debugging, mostly.  Set to 0 to not allow autoconfig to take place.
 * Note that this is overridden by having RAID_AUTOCONFIG as an option
 * in the kernel config file.
 */
#ifdef RAID_AUTOCONFIG
int raidautoconfig = 1;
#else
int raidautoconfig = 0;
#endif
static bool raidautoconfigdone = false;

struct pool rf_alloclist_pool;   /* AllocList */

static LIST_HEAD(, raid_softc) raids = LIST_HEAD_INITIALIZER(raids);
static kmutex_t raid_lock;

static struct raid_softc *
raidcreate(int unit) {
	struct raid_softc *sc = kmem_zalloc(sizeof(*sc), KM_SLEEP);
	sc->sc_unit = unit;
	cv_init(&sc->sc_cv, "raidunit");
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	return sc;
}

static void
raiddestroy(struct raid_softc *sc) {
	cv_destroy(&sc->sc_cv);
	mutex_destroy(&sc->sc_mutex);
	kmem_free(sc, sizeof(*sc));
}

static struct raid_softc *
raidget(int unit, bool create) {
	struct raid_softc *sc;
	if (unit < 0) {
#ifdef DIAGNOSTIC
		panic("%s: unit %d!", __func__, unit);
#endif
		return NULL;
	}
	mutex_enter(&raid_lock);
	LIST_FOREACH(sc, &raids, sc_link) {
		if (sc->sc_unit == unit) {
			mutex_exit(&raid_lock);
			return sc;
		}
	}
	mutex_exit(&raid_lock);
	if (!create)
		return NULL;
	sc = raidcreate(unit);
	mutex_enter(&raid_lock);
	LIST_INSERT_HEAD(&raids, sc, sc_link);
	mutex_exit(&raid_lock);
	return sc;
}

static void
raidput(struct raid_softc *sc) {
	mutex_enter(&raid_lock);
	LIST_REMOVE(sc, sc_link);
	mutex_exit(&raid_lock);
	raiddestroy(sc);
}

void
raidattach(int num)
{

	/*
	 * Device attachment and associated initialization now occurs
	 * as part of the module initialization.
	 */
}

static int
rf_autoconfig(device_t self)
{
	RF_AutoConfig_t *ac_list;
	RF_ConfigSet_t *config_sets;

	if (!raidautoconfig || raidautoconfigdone == true)
		return 0;

	/* XXX This code can only be run once. */
	raidautoconfigdone = true;

#ifdef __HAVE_CPU_BOOTCONF
	/*
	 * 0. find the boot device if needed first so we can use it later
	 * this needs to be done before we autoconfigure any raid sets,
	 * because if we use wedges we are not going to be able to open
	 * the boot device later
	 */
	if (booted_device == NULL)
		cpu_bootconf();
#endif
	/* 1. locate all RAID components on the system */
	aprint_debug("Searching for RAID components...\n");
	ac_list = rf_find_raid_components();

	/* 2. Sort them into their respective sets. */
	config_sets = rf_create_auto_sets(ac_list);

	/*
	 * 3. Evaluate each set and configure the valid ones.
	 * This gets done in rf_buildroothack().
	 */
	rf_buildroothack(config_sets);

	return 1;
}

int
rf_inited(const struct raid_softc *rs) {
	return (rs->sc_flags & RAIDF_INITED) != 0;
}

RF_Raid_t *
rf_get_raid(struct raid_softc *rs) {
	return &rs->sc_r;
}

int
rf_get_unit(const struct raid_softc *rs) {
	return rs->sc_unit;
}

static int
rf_containsboot(RF_Raid_t *r, device_t bdv) {
	const char *bootname;
	size_t len;

	/* if bdv is NULL, the set can't contain it. exit early. */
	if (bdv == NULL)
		return 0;

	bootname = device_xname(bdv);
	len = strlen(bootname);

	for (int col = 0; col < r->numCol; col++) {
		const char *devname = r->Disks[col].devname;
		devname += sizeof("/dev/") - 1;
		if (strncmp(devname, "dk", 2) == 0) {
			const char *parent =
			    dkwedge_get_parent_name(r->Disks[col].dev);
			if (parent != NULL)
				devname = parent;
		}
		if (strncmp(devname, bootname, len) == 0) {
			struct raid_softc *sc = r->softc;
			aprint_debug("raid%d includes boot device %s\n",
			    sc->sc_unit, devname);
			return 1;
		}
	}
	return 0;
}

static int
rf_rescan(void)
{
	RF_AutoConfig_t *ac_list;
	RF_ConfigSet_t *config_sets, *cset, *next_cset;
	struct raid_softc *sc;
	int raid_added;
	
	ac_list = rf_find_raid_components();
	config_sets = rf_create_auto_sets(ac_list);

	raid_added = 1;
	while (raid_added > 0) {
		raid_added = 0;
		cset = config_sets;
		while (cset != NULL) {
			next_cset = cset->next;
			if (rf_have_enough_components(cset) &&
			    cset->ac->clabel->autoconfigure == 1) {
				sc = rf_auto_config_set(cset);
				if (sc != NULL) {
					aprint_debug("raid%d: configured ok, rootable %d\n",
						     sc->sc_unit, cset->rootable);
					/* We added one RAID set */
					raid_added++;
				} else {
					/* The autoconfig didn't work :( */
					aprint_debug("Autoconfig failed\n");
					rf_release_all_vps(cset);
				}
			} else {
				/* we're not autoconfiguring this set...
				   release the associated resources */
				rf_release_all_vps(cset);
			}
			/* cleanup */
			rf_cleanup_config_set(cset);
			cset = next_cset;
		}
		if (raid_added > 0) {
			/* We added at least one RAID set, so re-scan for recursive RAID */
			ac_list = rf_find_raid_components();
			config_sets = rf_create_auto_sets(ac_list);
		}
	}
	
	return 0;
}


static void
rf_buildroothack(RF_ConfigSet_t *config_sets)
{
	RF_AutoConfig_t *ac_list;
	RF_ConfigSet_t *cset;
	RF_ConfigSet_t *next_cset;
	int num_root;
	int raid_added;
	struct raid_softc *sc, *rsc;
	struct dk_softc *dksc = NULL;	/* XXX gcc -Os: may be used uninit. */

	sc = rsc = NULL;
	num_root = 0;

	raid_added = 1;
	while (raid_added > 0) {
		raid_added = 0;
		cset = config_sets;
		while (cset != NULL) {
			next_cset = cset->next;
			if (rf_have_enough_components(cset) &&
			    cset->ac->clabel->autoconfigure == 1) {
				sc = rf_auto_config_set(cset);
				if (sc != NULL) {
					aprint_debug("raid%d: configured ok, rootable %d\n",
						     sc->sc_unit, cset->rootable);
					/* We added one RAID set */
					raid_added++;
					if (cset->rootable) {
						rsc = sc;
						num_root++;
					}
				} else {
					/* The autoconfig didn't work :( */
					aprint_debug("Autoconfig failed\n");
					rf_release_all_vps(cset);
				}
			} else {
				/* we're not autoconfiguring this set...
				   release the associated resources */
				rf_release_all_vps(cset);
			}
			/* cleanup */
			rf_cleanup_config_set(cset);
			cset = next_cset;
		}
		if (raid_added > 0) {
			/* We added at least one RAID set, so re-scan for recursive RAID */
			ac_list = rf_find_raid_components();
			config_sets = rf_create_auto_sets(ac_list);
		}
	}
	
	/* if the user has specified what the root device should be
	   then we don't touch booted_device or boothowto... */

	if (rootspec != NULL) {
		aprint_debug("%s: rootspec %s\n", __func__, rootspec);
		return;
	}

	/* we found something bootable... */

	/*
	 * XXX: The following code assumes that the root raid
	 * is the first ('a') partition. This is about the best
	 * we can do with a BSD disklabel, but we might be able
	 * to do better with a GPT label, by setting a specified
	 * attribute to indicate the root partition. We can then
	 * stash the partition number in the r->root_partition
	 * high bits (the bottom 2 bits are already used). For
	 * now we just set booted_partition to 0 when we override
	 * root.
	 */
	if (num_root == 1) {
		device_t candidate_root;
		dksc = &rsc->sc_dksc;
		if (dksc->sc_dkdev.dk_nwedges != 0) {
			char cname[sizeof(cset->ac->devname)];
			/* XXX: assume partition 'a' first */
			snprintf(cname, sizeof(cname), "%s%c",
			    device_xname(dksc->sc_dev), 'a');
			candidate_root = dkwedge_find_by_wname(cname);
			aprint_debug("%s: candidate wedge root=%s\n", __func__,
			    cname);
			if (candidate_root == NULL) {
				/*
				 * If that is not found, because we don't use
				 * disklabel, return the first dk child
				 * XXX: we can skip the 'a' check above
				 * and always do this...
				 */
				size_t i = 0;
				candidate_root = dkwedge_find_by_parent(
				    device_xname(dksc->sc_dev), &i);
			}
			aprint_debug("%s: candidate wedge root=%p\n", __func__,
			    candidate_root);
		} else
			candidate_root = dksc->sc_dev;
		aprint_debug("%s: candidate root=%p booted_device=%p "
			     "root_partition=%d contains_boot=%d\n",
		    __func__, candidate_root, booted_device,
		    rsc->sc_r.root_partition,
		    rf_containsboot(&rsc->sc_r, booted_device));
		/* XXX the check for booted_device == NULL can probably be
		 * dropped, now that rf_containsboot handles that case.
		 */
		if (booted_device == NULL ||
		    rsc->sc_r.root_partition == 1 ||
		    rf_containsboot(&rsc->sc_r, booted_device)) {
			booted_device = candidate_root;
			booted_method = "raidframe/single";
			booted_partition = 0;	/* XXX assume 'a' */
			aprint_debug("%s: set booted_device=%s(%p)\n", __func__,
			    device_xname(booted_device), booted_device);
		}
	} else if (num_root > 1) {
		aprint_debug("%s: many roots=%d, %p\n", __func__, num_root,
		    booted_device);

		/*
		 * Maybe the MD code can help. If it cannot, then
		 * setroot() will discover that we have no
		 * booted_device and will ask the user if nothing was
		 * hardwired in the kernel config file
		 */
		if (booted_device == NULL)
			return;

		num_root = 0;
		mutex_enter(&raid_lock);
		LIST_FOREACH(sc, &raids, sc_link) {
			RF_Raid_t *r = &sc->sc_r;
			if (r->valid == 0)
				continue;

			if (r->root_partition == 0)
				continue;

			if (rf_containsboot(r, booted_device)) {
				num_root++;
				rsc = sc;
				dksc = &rsc->sc_dksc;
			}
		}
		mutex_exit(&raid_lock);

		if (num_root == 1) {
			booted_device = dksc->sc_dev;
			booted_method = "raidframe/multi";
			booted_partition = 0;	/* XXX assume 'a' */
		} else {
			/* we can't guess.. require the user to answer... */
			boothowto |= RB_ASKNAME;
		}
	}
}

static int
raidsize(dev_t dev)
{
	struct raid_softc *rs;
	struct dk_softc *dksc;
	unsigned int unit;

	unit = raidunit(dev);
	if ((rs = raidget(unit, false)) == NULL)
		return -1;
	dksc = &rs->sc_dksc;

	if ((rs->sc_flags & RAIDF_INITED) == 0)
		return -1;

	return dk_size(dksc, dev);
}

static int
raiddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	unsigned int unit;
	struct raid_softc *rs;
	struct dk_softc *dksc;

	unit = raidunit(dev);
	if ((rs = raidget(unit, false)) == NULL)
		return ENXIO;
	dksc = &rs->sc_dksc;

	if ((rs->sc_flags & RAIDF_INITED) == 0)
		return ENODEV;

        /*
           Note that blkno is relative to this particular partition.
           By adding adding RF_PROTECTED_SECTORS, we get a value that
	   is relative to the partition used for the underlying component.
        */
	blkno += RF_PROTECTED_SECTORS;

	return dk_dump(dksc, dev, blkno, va, size, DK_DUMP_RECURSIVE);
}

static int
raid_dumpblocks(device_t dev, void *va, daddr_t blkno, int nblk)
{
	struct raid_softc *rs = raidsoftc(dev);
	const struct bdevsw *bdev;
	RF_Raid_t *raidPtr;
	int     c, sparecol, j, scol, dumpto;
	int     error = 0;

	raidPtr = &rs->sc_r;

	/* we only support dumping to RAID 1 sets */
	if (raidPtr->Layout.numDataCol != 1 ||
	    raidPtr->Layout.numParityCol != 1)
		return EINVAL;

	if ((error = raidlock(rs)) != 0)
		return error;

	/* figure out what device is alive.. */

	/*
	   Look for a component to dump to.  The preference for the
	   component to dump to is as follows:
	   1) the first component
	   2) a used_spare of the first component
	   3) the second component
	   4) a used_spare of the second component
	*/

	dumpto = -1;
	for (c = 0; c < raidPtr->numCol; c++) {
		if (raidPtr->Disks[c].status == rf_ds_optimal) {
			/* this might be the one */
			dumpto = c;
			break;
		}
	}

	/*
	   At this point we have possibly selected a live component.
	   If we didn't find a live ocmponent, we now check to see
	   if there is a relevant spared component.
	*/

	for (c = 0; c < raidPtr->numSpare; c++) {
		sparecol = raidPtr->numCol + c;
		if (raidPtr->Disks[sparecol].status ==  rf_ds_used_spare) {
			/* How about this one? */
			scol = -1;
			for(j=0;j<raidPtr->numCol;j++) {
				if (raidPtr->Disks[j].spareCol == sparecol) {
					scol = j;
					break;
				}
			}
			if (scol == 0) {
				/*
				   We must have found a spared first
				   component!  We'll take that over
				   anything else found so far.  (We
				   couldn't have found a real first
				   component before, since this is a
				   used spare, and it's saying that
				   it's replacing the first
				   component.)  On reboot (with
				   autoconfiguration turned on)
				   sparecol will become the first
				   component (component0) of this set.
				*/
				dumpto = sparecol;
				break;
			} else if (scol != -1) {
				/*
				   Must be a spared second component.
				   We'll dump to that if we havn't found
				   anything else so far.
				*/
				if (dumpto == -1)
					dumpto = sparecol;
			}
		}
	}

	if (dumpto == -1) {
		/* we couldn't find any live components to dump to!?!?
		 */
		error = EINVAL;
		goto out;
	}

	bdev = bdevsw_lookup(raidPtr->Disks[dumpto].dev);
	if (bdev == NULL) {
		error = ENXIO;
		goto out;
	}

	error = (*bdev->d_dump)(raidPtr->Disks[dumpto].dev,
				blkno, va, nblk * raidPtr->bytesPerSector);

out:
	raidunlock(rs);

	return error;
}

/* ARGSUSED */
static int
raidopen(dev_t dev, int flags, int fmt,
    struct lwp *l)
{
	int     unit = raidunit(dev);
	struct raid_softc *rs;
	struct dk_softc *dksc;
	int     error = 0;
	int     part, pmask;

	if ((rs = raidget(unit, true)) == NULL)
		return ENXIO;
	if ((error = raidlock(rs)) != 0)
		return error;

	if ((rs->sc_flags & RAIDF_SHUTDOWN) != 0) {
		error = EBUSY;
		goto bad;
	}

	dksc = &rs->sc_dksc;

	part = DISKPART(dev);
	pmask = (1 << part);

	if (!DK_BUSY(dksc, pmask) &&
	    ((rs->sc_flags & RAIDF_INITED) != 0)) {
		/* First one... mark things as dirty... Note that we *MUST*
		 have done a configure before this.  I DO NOT WANT TO BE
		 SCRIBBLING TO RANDOM COMPONENTS UNTIL IT'S BEEN DETERMINED
		 THAT THEY BELONG TOGETHER!!!!! */
		/* XXX should check to see if we're only open for reading
		   here... If so, we needn't do this, but then need some
		   other way of keeping track of what's happened.. */

		rf_markalldirty(&rs->sc_r);
	}

	if ((rs->sc_flags & RAIDF_INITED) != 0)
		error = dk_open(dksc, dev, flags, fmt, l);

bad:
	raidunlock(rs);

	return error;


}

static int
raid_lastclose(device_t self)
{
	struct raid_softc *rs = raidsoftc(self);

	/* Last one... device is not unconfigured yet.
	   Device shutdown has taken care of setting the
	   clean bits if RAIDF_INITED is not set
	   mark things as clean... */

	rf_update_component_labels(&rs->sc_r,
	    RF_FINAL_COMPONENT_UPDATE);

	/* pass to unlocked code */
	if ((rs->sc_flags & RAIDF_SHUTDOWN) != 0)
		rs->sc_flags |= RAIDF_DETACH;

	return 0;
}

/* ARGSUSED */
static int
raidclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	int     unit = raidunit(dev);
	struct raid_softc *rs;
	struct dk_softc *dksc;
	cfdata_t cf;
	int     error = 0, do_detach = 0, do_put = 0;

	if ((rs = raidget(unit, false)) == NULL)
		return ENXIO;
	dksc = &rs->sc_dksc;

	if ((error = raidlock(rs)) != 0)
		return error;

	if ((rs->sc_flags & RAIDF_INITED) != 0) {
		error = dk_close(dksc, dev, flags, fmt, l);
		if ((rs->sc_flags & RAIDF_DETACH) != 0)
			do_detach = 1;
	} else if ((rs->sc_flags & RAIDF_SHUTDOWN) != 0)
		do_put = 1;

	raidunlock(rs);

	if (do_detach) {
		/* free the pseudo device attach bits */
		cf = device_cfdata(dksc->sc_dev);
		error = config_detach(dksc->sc_dev, 0);
		if (error == 0)
			free(cf, M_RAIDFRAME);
	} else if (do_put) {
		raidput(rs);
	}

	return error;

}

static void
raid_wakeup(RF_Raid_t *raidPtr)
{
	rf_lock_mutex2(raidPtr->iodone_lock);
	rf_signal_cond2(raidPtr->iodone_cv);
	rf_unlock_mutex2(raidPtr->iodone_lock);
}

static void
raidstrategy(struct buf *bp)
{
	unsigned int unit;
	struct raid_softc *rs;
	struct dk_softc *dksc;
	RF_Raid_t *raidPtr;

	unit = raidunit(bp->b_dev);
	if ((rs = raidget(unit, false)) == NULL) {
		bp->b_error = ENXIO;
		goto fail;
	}
	if ((rs->sc_flags & RAIDF_INITED) == 0) {
		bp->b_error = ENXIO;
		goto fail;
	}
	dksc = &rs->sc_dksc;
	raidPtr = &rs->sc_r;

	/* Queue IO only */
	if (dk_strategy_defer(dksc, bp))
		goto done;

	/* schedule the IO to happen at the next convenient time */
	raid_wakeup(raidPtr);

done:
	return;

fail:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

static int
raid_diskstart(device_t dev, struct buf *bp)
{
	struct raid_softc *rs = raidsoftc(dev);
	RF_Raid_t *raidPtr;

	raidPtr = &rs->sc_r;
	if (!raidPtr->valid) {
		db1_printf(("raid is not valid..\n"));
		return ENODEV;
	}

	/* XXX */
	bp->b_resid = 0;

	return raiddoaccess(raidPtr, bp);
}

void
raiddone(RF_Raid_t *raidPtr, struct buf *bp)
{
	struct raid_softc *rs;
	struct dk_softc *dksc;

	rs = raidPtr->softc;
	dksc = &rs->sc_dksc;

	dk_done(dksc, bp);

	rf_lock_mutex2(raidPtr->mutex);
	raidPtr->openings++;
	rf_unlock_mutex2(raidPtr->mutex);

	/* schedule more IO */
	raid_wakeup(raidPtr);
}

/* ARGSUSED */
static int
raidread(dev_t dev, struct uio *uio, int flags)
{
	int     unit = raidunit(dev);
	struct raid_softc *rs;

	if ((rs = raidget(unit, false)) == NULL)
		return ENXIO;

	if ((rs->sc_flags & RAIDF_INITED) == 0)
		return ENXIO;

	return physio(raidstrategy, NULL, dev, B_READ, minphys, uio);

}

/* ARGSUSED */
static int
raidwrite(dev_t dev, struct uio *uio, int flags)
{
	int     unit = raidunit(dev);
	struct raid_softc *rs;

	if ((rs = raidget(unit, false)) == NULL)
		return ENXIO;

	if ((rs->sc_flags & RAIDF_INITED) == 0)
		return ENXIO;

	return physio(raidstrategy, NULL, dev, B_WRITE, minphys, uio);

}

static int
raid_detach_unlocked(struct raid_softc *rs)
{
	struct dk_softc *dksc = &rs->sc_dksc;
	RF_Raid_t *raidPtr;
	int error;

	raidPtr = &rs->sc_r;

	if (DK_BUSY(dksc, 0) ||
	    raidPtr->recon_in_progress != 0 ||
	    raidPtr->parity_rewrite_in_progress != 0 ||
	    raidPtr->copyback_in_progress != 0)
		return EBUSY;

	if ((rs->sc_flags & RAIDF_INITED) == 0)
		return 0;

	rs->sc_flags &= ~RAIDF_SHUTDOWN;

	if ((error = rf_Shutdown(raidPtr)) != 0)
		return error;

	rs->sc_flags &= ~RAIDF_INITED;

	/* Kill off any queued buffers */
	dk_drain(dksc);
	bufq_free(dksc->sc_bufq);

	/* Detach the disk. */
	dkwedge_delall(&dksc->sc_dkdev);
	disk_detach(&dksc->sc_dkdev);
	disk_destroy(&dksc->sc_dkdev);
	dk_detach(dksc);

	return 0;
}

static bool
rf_must_be_initialized(const struct raid_softc *rs, u_long cmd)
{
	switch (cmd) {
	case RAIDFRAME_ADD_HOT_SPARE:
	case RAIDFRAME_CHECK_COPYBACK_STATUS:
	case RAIDFRAME_CHECK_COPYBACK_STATUS_EXT:
	case RAIDFRAME_CHECK_PARITY:
	case RAIDFRAME_CHECK_PARITYREWRITE_STATUS:
	case RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT:
	case RAIDFRAME_CHECK_RECON_STATUS:
	case RAIDFRAME_CHECK_RECON_STATUS_EXT:
	case RAIDFRAME_COPYBACK:
	case RAIDFRAME_DELETE_COMPONENT:
	case RAIDFRAME_FAIL_DISK:
	case RAIDFRAME_GET_ACCTOTALS:
	case RAIDFRAME_GET_COMPONENT_LABEL:
	case RAIDFRAME_GET_INFO:
	case RAIDFRAME_GET_SIZE:
	case RAIDFRAME_INCORPORATE_HOT_SPARE:
	case RAIDFRAME_INIT_LABELS:
	case RAIDFRAME_KEEP_ACCTOTALS:
	case RAIDFRAME_PARITYMAP_GET_DISABLE:
	case RAIDFRAME_PARITYMAP_SET_DISABLE:
	case RAIDFRAME_PARITYMAP_SET_PARAMS:
	case RAIDFRAME_PARITYMAP_STATUS:
	case RAIDFRAME_REBUILD_IN_PLACE:
	case RAIDFRAME_REMOVE_HOT_SPARE:
	case RAIDFRAME_RESET_ACCTOTALS:
	case RAIDFRAME_REWRITEPARITY:
	case RAIDFRAME_SET_AUTOCONFIG:
	case RAIDFRAME_SET_COMPONENT_LABEL:
	case RAIDFRAME_SET_ROOT:
		return (rs->sc_flags & RAIDF_INITED) == 0;
	}
	return false;
}

int
rf_fail_disk(RF_Raid_t *raidPtr, struct rf_recon_req *rr)
{
	struct rf_recon_req_internal *rrint;

	if (raidPtr->Layout.map->faultsTolerated == 0) {
		/* Can't do this on a RAID 0!! */
		return EINVAL;
	}

	if (rr->col < 0 || rr->col >= raidPtr->numCol) {
		/* bad column */
		return EINVAL;
	}

	rf_lock_mutex2(raidPtr->mutex);
	if (raidPtr->status == rf_rs_reconstructing) {
		/* you can't fail a disk while we're reconstructing! */
		/* XXX wrong for RAID6 */
		goto out;
	}
	if ((raidPtr->Disks[rr->col].status == rf_ds_optimal) &&
	    (raidPtr->numFailures > 0)) {
		/* some other component has failed.  Let's not make
		   things worse. XXX wrong for RAID6 */
		goto out;
	}
	if (raidPtr->Disks[rr->col].status == rf_ds_spared) {
		/* Can't fail a spared disk! */
		goto out;
	}
	rf_unlock_mutex2(raidPtr->mutex);

	/* make a copy of the recon request so that we don't rely on
	 * the user's buffer */
	rrint = RF_Malloc(sizeof(*rrint));
	if (rrint == NULL)
		return(ENOMEM);
	rrint->col = rr->col;
	rrint->flags = rr->flags;
	rrint->raidPtr = raidPtr;

	return RF_CREATE_THREAD(raidPtr->recon_thread, rf_ReconThread,
	    rrint, "raid_recon");
out:
	rf_unlock_mutex2(raidPtr->mutex);
	return EINVAL;
}

static int
rf_copyinspecificbuf(RF_Config_t *k_cfg)
{
	/* allocate a buffer for the layout-specific data, and copy it in */
	if (k_cfg->layoutSpecificSize == 0)
		return 0;

	if (k_cfg->layoutSpecificSize > 10000) {
	    /* sanity check */
	    return EINVAL;
	}

	u_char *specific_buf;
	specific_buf =  RF_Malloc(k_cfg->layoutSpecificSize);
	if (specific_buf == NULL)
		return ENOMEM;

	int retcode = copyin(k_cfg->layoutSpecific, specific_buf,
	    k_cfg->layoutSpecificSize);
	if (retcode) {
		RF_Free(specific_buf, k_cfg->layoutSpecificSize);
		db1_printf(("%s: retcode=%d copyin.2\n", __func__, retcode));
		return retcode;
	}

	k_cfg->layoutSpecific = specific_buf;
	return 0;
}

static int
rf_getConfiguration(struct raid_softc *rs, void *data, RF_Config_t **k_cfg)
{
	RF_Config_t *u_cfg = *((RF_Config_t **) data);

	if (rs->sc_r.valid) {
		/* There is a valid RAID set running on this unit! */
		printf("raid%d: Device already configured!\n", rs->sc_unit);
		return EINVAL;
	}

	/* copy-in the configuration information */
	/* data points to a pointer to the configuration structure */
	*k_cfg = RF_Malloc(sizeof(**k_cfg));
	if (*k_cfg == NULL) {
		return ENOMEM;
	}
	int retcode = copyin(u_cfg, *k_cfg, sizeof(RF_Config_t));
	if (retcode == 0)
		return 0;
	RF_Free(*k_cfg, sizeof(RF_Config_t));
	db1_printf(("%s: retcode=%d copyin.1\n", __func__, retcode));
	rs->sc_flags |= RAIDF_SHUTDOWN;
	return retcode;
}

int
rf_construct(struct raid_softc *rs, RF_Config_t *k_cfg)
{
	int retcode;
	RF_Raid_t *raidPtr = &rs->sc_r;

	rs->sc_flags &= ~RAIDF_SHUTDOWN;

	if ((retcode = rf_copyinspecificbuf(k_cfg)) != 0)
		goto out;

	/* should do some kind of sanity check on the configuration.
	 * Store the sum of all the bytes in the last byte? */

	/* configure the system */

	/*
	 * Clear the entire RAID descriptor, just to make sure
	 *  there is no stale data left in the case of a
	 *  reconfiguration
	 */
	memset(raidPtr, 0, sizeof(*raidPtr));
	raidPtr->softc = rs;
	raidPtr->raidid = rs->sc_unit;

	retcode = rf_Configure(raidPtr, k_cfg, NULL);

	if (retcode == 0) {
		/* allow this many simultaneous IO's to
		   this RAID device */
		raidPtr->openings = RAIDOUTSTANDING;

		raidinit(rs);
		raid_wakeup(raidPtr);
		rf_markalldirty(raidPtr);
	}

	/* free the buffers.  No return code here. */
	if (k_cfg->layoutSpecificSize) {
		RF_Free(k_cfg->layoutSpecific, k_cfg->layoutSpecificSize);
	}
out:
	RF_Free(k_cfg, sizeof(RF_Config_t));
	if (retcode) {
		/*
		 * If configuration failed, set sc_flags so that we
		 * will detach the device when we close it.
		 */
		rs->sc_flags |= RAIDF_SHUTDOWN;
	}
	return retcode;
}

#if RF_DISABLED
static int
rf_set_component_label(RF_Raid_t *raidPtr, RF_ComponentLabel_t *clabel)
{

	/* XXX check the label for valid stuff... */
	/* Note that some things *should not* get modified --
	   the user should be re-initing the labels instead of
	   trying to patch things.
	   */
#ifdef DEBUG
	int raidid = raidPtr->raidid;
	printf("raid%d: Got component label:\n", raidid);
	printf("raid%d: Version: %d\n", raidid, clabel->version);
	printf("raid%d: Serial Number: %d\n", raidid, clabel->serial_number);
	printf("raid%d: Mod counter: %d\n", raidid, clabel->mod_counter);
	printf("raid%d: Column: %d\n", raidid, clabel->column);
	printf("raid%d: Num Columns: %d\n", raidid, clabel->num_columns);
	printf("raid%d: Clean: %d\n", raidid, clabel->clean);
	printf("raid%d: Status: %d\n", raidid, clabel->status);
#endif	/* DEBUG */
	clabel->row = 0;
	int column = clabel->column;

	if ((column < 0) || (column >= raidPtr->numCol)) {
		return(EINVAL);
	}

	/* XXX this isn't allowed to do anything for now :-) */

	/* XXX and before it is, we need to fill in the rest
	   of the fields!?!?!?! */
	memcpy(raidget_component_label(raidPtr, column),
	    clabel, sizeof(*clabel));
	raidflush_component_label(raidPtr, column);
	return 0;
}
#endif

static int
rf_init_component_label(RF_Raid_t *raidPtr, RF_ComponentLabel_t *clabel)
{
	/*
	   we only want the serial number from
	   the above.  We get all the rest of the information
	   from the config that was used to create this RAID
	   set.
	   */

	raidPtr->serial_number = clabel->serial_number;

	for (int column = 0; column < raidPtr->numCol; column++) {
		RF_RaidDisk_t *diskPtr = &raidPtr->Disks[column];
		if (RF_DEAD_DISK(diskPtr->status))
			continue;
		RF_ComponentLabel_t *ci_label = raidget_component_label(
		    raidPtr, column);
		/* Zeroing this is important. */
		memset(ci_label, 0, sizeof(*ci_label));
		raid_init_component_label(raidPtr, ci_label);
		ci_label->serial_number = raidPtr->serial_number;
		ci_label->row = 0; /* we dont' pretend to support more */
		rf_component_label_set_partitionsize(ci_label,
		    diskPtr->partitionSize);
		ci_label->column = column;
		raidflush_component_label(raidPtr, column);
		/* XXXjld what about the spares? */
	}

	return 0;
}

static int
rf_rebuild_in_place(RF_Raid_t *raidPtr, RF_SingleComponent_t *componentPtr)
{

	if (raidPtr->Layout.map->faultsTolerated == 0) {
		/* Can't do this on a RAID 0!! */
		return EINVAL;
	}

	if (raidPtr->recon_in_progress == 1) {
		/* a reconstruct is already in progress! */
		return EINVAL;
	}

	RF_SingleComponent_t component;
	memcpy(&component, componentPtr, sizeof(RF_SingleComponent_t));
	component.row = 0; /* we don't support any more */
	int column = component.column;

	if ((column < 0) || (column >= raidPtr->numCol)) {
		return EINVAL;
	}

	rf_lock_mutex2(raidPtr->mutex);
	if ((raidPtr->Disks[column].status == rf_ds_optimal) &&
	    (raidPtr->numFailures > 0)) {
		/* XXX 0 above shouldn't be constant!!! */
		/* some component other than this has failed.
		   Let's not make things worse than they already
		   are... */
		printf("raid%d: Unable to reconstruct to disk at:\n",
		       raidPtr->raidid);
		printf("raid%d:     Col: %d   Too many failures.\n",
		       raidPtr->raidid, column);
		rf_unlock_mutex2(raidPtr->mutex);
		return EINVAL;
	}

	if (raidPtr->Disks[column].status == rf_ds_reconstructing) {
		printf("raid%d: Unable to reconstruct to disk at:\n",
		       raidPtr->raidid);
		printf("raid%d:    Col: %d   "
		    "Reconstruction already occurring!\n",
		    raidPtr->raidid, column);

		rf_unlock_mutex2(raidPtr->mutex);
		return EINVAL;
	}

	if (raidPtr->Disks[column].status == rf_ds_spared) {
		rf_unlock_mutex2(raidPtr->mutex);
		return EINVAL;
	}

	rf_unlock_mutex2(raidPtr->mutex);

	struct rf_recon_req_internal *rrint;
	rrint = RF_Malloc(sizeof(*rrint));
	if (rrint == NULL)
		return ENOMEM;

	rrint->col = column;
	rrint->raidPtr = raidPtr;

	return RF_CREATE_THREAD(raidPtr->recon_thread,
	    rf_ReconstructInPlaceThread, rrint, "raid_reconip");
}

static int
rf_check_recon_status(RF_Raid_t *raidPtr, int *data)
{
	/*
	 * This makes no sense on a RAID 0, or if we are not reconstructing
	 * so tell the user it's done.
	 */
	if (raidPtr->Layout.map->faultsTolerated == 0 ||
	    raidPtr->status != rf_rs_reconstructing) {
		*data = 100;
		return 0;
	}
	if (raidPtr->reconControl->numRUsTotal == 0) {
		*data = 0;
		return 0;
	}
	*data = (raidPtr->reconControl->numRUsComplete * 100
	    / raidPtr->reconControl->numRUsTotal);
	return 0;
}

static int
raidioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int     unit = raidunit(dev);
	int     part, pmask;
	struct raid_softc *rs;
	struct dk_softc *dksc;
	RF_Config_t *k_cfg;
	RF_Raid_t *raidPtr;
	RF_AccTotals_t *totals;
	RF_SingleComponent_t component;
	RF_DeviceConfig_t *d_cfg, *ucfgp;
	int retcode = 0;
	int column;
	RF_ComponentLabel_t *clabel;
	RF_SingleComponent_t *sparePtr,*componentPtr;
	int d;

	if ((rs = raidget(unit, false)) == NULL)
		return ENXIO;

	dksc = &rs->sc_dksc;
	raidPtr = &rs->sc_r;

	db1_printf(("raidioctl: %d %d %d %lu\n", (int) dev,
	    (int) DISKPART(dev), (int) unit, cmd));

	/* Must be initialized for these... */
	if (rf_must_be_initialized(rs, cmd))
		return ENXIO;

	switch (cmd) {
		/* configure the system */
	case RAIDFRAME_CONFIGURE:
		if ((retcode = rf_getConfiguration(rs, data, &k_cfg)) != 0)
			return retcode;
		return rf_construct(rs, k_cfg);

		/* shutdown the system */
	case RAIDFRAME_SHUTDOWN:

		part = DISKPART(dev);
		pmask = (1 << part);

		if ((retcode = raidlock(rs)) != 0)
			return retcode;

		if (DK_BUSY(dksc, pmask) ||
		    raidPtr->recon_in_progress != 0 ||
		    raidPtr->parity_rewrite_in_progress != 0 ||
		    raidPtr->copyback_in_progress != 0)
			retcode = EBUSY;
		else {
			/* detach and free on close */
			rs->sc_flags |= RAIDF_SHUTDOWN;
			retcode = 0;
		}

		raidunlock(rs);

		return retcode;
	case RAIDFRAME_GET_COMPONENT_LABEL:
		return rf_get_component_label(raidPtr, data);

#if RF_DISABLED
	case RAIDFRAME_SET_COMPONENT_LABEL:
		return rf_set_component_label(raidPtr, data);
#endif

	case RAIDFRAME_INIT_LABELS:
		return rf_init_component_label(raidPtr, data);

	case RAIDFRAME_SET_AUTOCONFIG:
		d = rf_set_autoconfig(raidPtr, *(int *) data);
		printf("raid%d: New autoconfig value is: %d\n",
		       raidPtr->raidid, d);
		*(int *) data = d;
		return retcode;

	case RAIDFRAME_SET_ROOT:
		d = rf_set_rootpartition(raidPtr, *(int *) data);
		printf("raid%d: New rootpartition value is: %d\n",
		       raidPtr->raidid, d);
		*(int *) data = d;
		return retcode;

		/* initialize all parity */
	case RAIDFRAME_REWRITEPARITY:

		if (raidPtr->Layout.map->faultsTolerated == 0) {
			/* Parity for RAID 0 is trivially correct */
			raidPtr->parity_good = RF_RAID_CLEAN;
			return 0;
		}

		if (raidPtr->parity_rewrite_in_progress == 1) {
			/* Re-write is already in progress! */
			return EINVAL;
		}

		return RF_CREATE_THREAD(raidPtr->parity_rewrite_thread,
		    rf_RewriteParityThread, raidPtr,"raid_parity");

	case RAIDFRAME_ADD_HOT_SPARE:
		sparePtr = (RF_SingleComponent_t *) data;
		memcpy(&component, sparePtr, sizeof(RF_SingleComponent_t));
		return rf_add_hot_spare(raidPtr, &component);

	case RAIDFRAME_REMOVE_HOT_SPARE:
		return retcode;

	case RAIDFRAME_DELETE_COMPONENT:
		componentPtr = (RF_SingleComponent_t *)data;
		memcpy(&component, componentPtr, sizeof(RF_SingleComponent_t));
		return rf_delete_component(raidPtr, &component);

	case RAIDFRAME_INCORPORATE_HOT_SPARE:
		componentPtr = (RF_SingleComponent_t *)data;
		memcpy(&component, componentPtr, sizeof(RF_SingleComponent_t));
		return rf_incorporate_hot_spare(raidPtr, &component);

	case RAIDFRAME_REBUILD_IN_PLACE:
		return rf_rebuild_in_place(raidPtr, data);
		
	case RAIDFRAME_GET_INFO:
		ucfgp = *(RF_DeviceConfig_t **)data;
		d_cfg = RF_Malloc(sizeof(*d_cfg));
		if (d_cfg == NULL)
			return ENOMEM;
		retcode = rf_get_info(raidPtr, d_cfg);
		if (retcode == 0) {
			retcode = copyout(d_cfg, ucfgp, sizeof(*d_cfg));
		}
		RF_Free(d_cfg, sizeof(RF_DeviceConfig_t));
		return retcode;

	case RAIDFRAME_CHECK_PARITY:
		*(int *) data = raidPtr->parity_good;
		return 0;

	case RAIDFRAME_PARITYMAP_STATUS:
		if (rf_paritymap_ineligible(raidPtr))
			return EINVAL;
		rf_paritymap_status(raidPtr->parity_map, data);
		return 0;

	case RAIDFRAME_PARITYMAP_SET_PARAMS:
		if (rf_paritymap_ineligible(raidPtr))
			return EINVAL;
		if (raidPtr->parity_map == NULL)
			return ENOENT; /* ??? */
		if (rf_paritymap_set_params(raidPtr->parity_map, data, 1) != 0)
			return EINVAL;
		return 0;

	case RAIDFRAME_PARITYMAP_GET_DISABLE:
		if (rf_paritymap_ineligible(raidPtr))
			return EINVAL;
		*(int *) data = rf_paritymap_get_disable(raidPtr);
		return 0;

	case RAIDFRAME_PARITYMAP_SET_DISABLE:
		if (rf_paritymap_ineligible(raidPtr))
			return EINVAL;
		rf_paritymap_set_disable(raidPtr, *(int *)data);
		/* XXX should errors be passed up? */
		return 0;

	case RAIDFRAME_RESCAN:
		return rf_rescan();

	case RAIDFRAME_RESET_ACCTOTALS:
		memset(&raidPtr->acc_totals, 0, sizeof(raidPtr->acc_totals));
		return 0;

	case RAIDFRAME_GET_ACCTOTALS:
		totals = (RF_AccTotals_t *) data;
		*totals = raidPtr->acc_totals;
		return 0;

	case RAIDFRAME_KEEP_ACCTOTALS:
		raidPtr->keep_acc_totals = *(int *)data;
		return 0;

	case RAIDFRAME_GET_SIZE:
		*(int *) data = raidPtr->totalSectors;
		return 0;

	case RAIDFRAME_FAIL_DISK:
		return rf_fail_disk(raidPtr, data);

		/* invoke a copyback operation after recon on whatever disk
		 * needs it, if any */
	case RAIDFRAME_COPYBACK:

		if (raidPtr->Layout.map->faultsTolerated == 0) {
			/* This makes no sense on a RAID 0!! */
			return EINVAL;
		}

		if (raidPtr->copyback_in_progress == 1) {
			/* Copyback is already in progress! */
			return EINVAL;
		}

		return RF_CREATE_THREAD(raidPtr->copyback_thread,
		    rf_CopybackThread, raidPtr, "raid_copyback");

		/* return the percentage completion of reconstruction */
	case RAIDFRAME_CHECK_RECON_STATUS:
		return rf_check_recon_status(raidPtr, data);

	case RAIDFRAME_CHECK_RECON_STATUS_EXT:
		rf_check_recon_status_ext(raidPtr, data);
		return 0;

	case RAIDFRAME_CHECK_PARITYREWRITE_STATUS:
		if (raidPtr->Layout.map->faultsTolerated == 0) {
			/* This makes no sense on a RAID 0, so tell the
			   user it's done. */
			*(int *) data = 100;
			return 0;
		}
		if (raidPtr->parity_rewrite_in_progress == 1) {
			*(int *) data = 100 *
				raidPtr->parity_rewrite_stripes_done /
				raidPtr->Layout.numStripe;
		} else {
			*(int *) data = 100;
		}
		return 0;

	case RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT:
		rf_check_parityrewrite_status_ext(raidPtr, data);
		return 0;

	case RAIDFRAME_CHECK_COPYBACK_STATUS:
		if (raidPtr->Layout.map->faultsTolerated == 0) {
			/* This makes no sense on a RAID 0 */
			*(int *) data = 100;
			return 0;
		}
		if (raidPtr->copyback_in_progress == 1) {
			*(int *) data = 100 * raidPtr->copyback_stripes_done /
				raidPtr->Layout.numStripe;
		} else {
			*(int *) data = 100;
		}
		return 0;

	case RAIDFRAME_CHECK_COPYBACK_STATUS_EXT:
		rf_check_copyback_status_ext(raidPtr, data);
		return 0;

	case RAIDFRAME_SET_LAST_UNIT:
		for (column = 0; column < raidPtr->numCol; column++)
			if (raidPtr->Disks[column].status != rf_ds_optimal)
				return EBUSY;

		for (column = 0; column < raidPtr->numCol; column++) {
			clabel = raidget_component_label(raidPtr, column);
			clabel->last_unit = *(int *)data;
			raidflush_component_label(raidPtr, column);
		}
		rs->sc_cflags |= RAIDF_UNIT_CHANGED;
		return 0;

		/* the sparetable daemon calls this to wait for the kernel to
		 * need a spare table. this ioctl does not return until a
		 * spare table is needed. XXX -- calling mpsleep here in the
		 * ioctl code is almost certainly wrong and evil. -- XXX XXX
		 * -- I should either compute the spare table in the kernel,
		 * or have a different -- XXX XXX -- interface (a different
		 * character device) for delivering the table     -- XXX */
#if RF_DISABLED
	case RAIDFRAME_SPARET_WAIT:
		rf_lock_mutex2(rf_sparet_wait_mutex);
		while (!rf_sparet_wait_queue)
			rf_wait_cond2(rf_sparet_wait_cv, rf_sparet_wait_mutex);
		RF_SparetWait_t *waitreq = rf_sparet_wait_queue;
		rf_sparet_wait_queue = rf_sparet_wait_queue->next;
		rf_unlock_mutex2(rf_sparet_wait_mutex);

		/* structure assignment */
		*((RF_SparetWait_t *) data) = *waitreq;

		RF_Free(waitreq, sizeof(*waitreq));
		return 0;

		/* wakes up a process waiting on SPARET_WAIT and puts an error
		 * code in it that will cause the dameon to exit */
	case RAIDFRAME_ABORT_SPARET_WAIT:
		waitreq = RF_Malloc(sizeof(*waitreq));
		waitreq->fcol = -1;
		rf_lock_mutex2(rf_sparet_wait_mutex);
		waitreq->next = rf_sparet_wait_queue;
		rf_sparet_wait_queue = waitreq;
		rf_broadcast_cond2(rf_sparet_wait_cv);
		rf_unlock_mutex2(rf_sparet_wait_mutex);
		return 0;

		/* used by the spare table daemon to deliver a spare table
		 * into the kernel */
	case RAIDFRAME_SEND_SPARET:

		/* install the spare table */
		retcode = rf_SetSpareTable(raidPtr, *(void **) data);

		/* respond to the requestor.  the return status of the spare
		 * table installation is passed in the "fcol" field */
		waitred = RF_Malloc(sizeof(*waitreq));
		waitreq->fcol = retcode;
		rf_lock_mutex2(rf_sparet_wait_mutex);
		waitreq->next = rf_sparet_resp_queue;
		rf_sparet_resp_queue = waitreq;
		rf_broadcast_cond2(rf_sparet_resp_cv);
		rf_unlock_mutex2(rf_sparet_wait_mutex);

		return retcode;
#endif
	default:
		/*
		 * Don't bother trying to load compat modules
		 * if it is not our ioctl. This is more efficient
		 * and makes rump tests not depend on compat code
		 */
		if (IOCGROUP(cmd) != 'r')
			break;
#ifdef _LP64
		if ((l->l_proc->p_flag & PK_32) != 0) {
			module_autoload("compat_netbsd32_raid",
			    MODULE_CLASS_EXEC);
			MODULE_HOOK_CALL(raidframe_netbsd32_ioctl_hook,
			    (rs, cmd, data), enosys(), retcode);
			if (retcode != EPASSTHROUGH)
				return retcode;
		}
#endif
		module_autoload("compat_raid_80", MODULE_CLASS_EXEC);
		MODULE_HOOK_CALL(raidframe_ioctl_80_hook,
		    (rs, cmd, data), enosys(), retcode);
		if (retcode != EPASSTHROUGH)
			return retcode;

		module_autoload("compat_raid_50", MODULE_CLASS_EXEC);
		MODULE_HOOK_CALL(raidframe_ioctl_50_hook,
		    (rs, cmd, data), enosys(), retcode);
		if (retcode != EPASSTHROUGH)
			return retcode;
		break; /* fall through to the os-specific code below */

	}

	if (!raidPtr->valid)
		return EINVAL;

	/*
	 * Add support for "regular" device ioctls here.
	 */

	switch (cmd) {
	case DIOCGCACHE:
		retcode = rf_get_component_caches(raidPtr, (int *)data);
		break;

	case DIOCCACHESYNC:
		retcode = rf_sync_component_caches(raidPtr, *(int *)data);
		break;

	default:
		retcode = dk_ioctl(dksc, dev, cmd, data, flag, l);
		break;
	}

	return retcode;

}


/* raidinit -- complete the rest of the initialization for the
   RAIDframe device.  */


static void
raidinit(struct raid_softc *rs)
{
	cfdata_t cf;
	unsigned int unit;
	struct dk_softc *dksc = &rs->sc_dksc;
	RF_Raid_t *raidPtr = &rs->sc_r;
	device_t dev;

	unit = raidPtr->raidid;

	/* XXX doesn't check bounds. */
	snprintf(rs->sc_xname, sizeof(rs->sc_xname), "raid%u", unit);

	/* attach the pseudo device */
	cf = malloc(sizeof(*cf), M_RAIDFRAME, M_WAITOK);
	cf->cf_name = raid_cd.cd_name;
	cf->cf_atname = raid_cd.cd_name;
	cf->cf_unit = unit;
	cf->cf_fstate = FSTATE_STAR;

	dev = config_attach_pseudo(cf);
	if (dev == NULL) {
		printf("raid%d: config_attach_pseudo failed\n",
		    raidPtr->raidid);
		free(cf, M_RAIDFRAME);
		return;
	}

	/* provide a backpointer to the real softc */
	raidsoftc(dev) = rs;

	/* disk_attach actually creates space for the CPU disklabel, among
	 * other things, so it's critical to call this *BEFORE* we try putzing
	 * with disklabels. */
	dk_init(dksc, dev, DKTYPE_RAID);
	disk_init(&dksc->sc_dkdev, rs->sc_xname, &rf_dkdriver);

	/* XXX There may be a weird interaction here between this, and
	 * protectedSectors, as used in RAIDframe.  */

	rs->sc_size = raidPtr->totalSectors;

	/* Attach dk and disk subsystems */
	dk_attach(dksc);
	disk_attach(&dksc->sc_dkdev);
	rf_set_geometry(rs, raidPtr);

	bufq_alloc(&dksc->sc_bufq, "fcfs", BUFQ_SORT_RAWBLOCK);

	/* mark unit as usuable */
	rs->sc_flags |= RAIDF_INITED;

	dkwedge_discover(&dksc->sc_dkdev);
}

#if (RF_INCLUDE_PARITY_DECLUSTERING_DS > 0)
/* wake up the daemon & tell it to get us a spare table
 * XXX
 * the entries in the queues should be tagged with the raidPtr
 * so that in the extremely rare case that two recons happen at once,
 * we know for which device were requesting a spare table
 * XXX
 *
 * XXX This code is not currently used. GO
 */
int
rf_GetSpareTableFromDaemon(RF_SparetWait_t *req)
{
	int     retcode;

	rf_lock_mutex2(rf_sparet_wait_mutex);
	req->next = rf_sparet_wait_queue;
	rf_sparet_wait_queue = req;
	rf_broadcast_cond2(rf_sparet_wait_cv);

	/* mpsleep unlocks the mutex */
	while (!rf_sparet_resp_queue) {
		rf_wait_cond2(rf_sparet_resp_cv, rf_sparet_wait_mutex);
	}
	req = rf_sparet_resp_queue;
	rf_sparet_resp_queue = req->next;
	rf_unlock_mutex2(rf_sparet_wait_mutex);

	retcode = req->fcol;
	RF_Free(req, sizeof(*req));	/* this is not the same req as we
					 * alloc'd */
	return retcode;
}
#endif

/* a wrapper around rf_DoAccess that extracts appropriate info from the
 * bp & passes it down.
 * any calls originating in the kernel must use non-blocking I/O
 * do some extra sanity checking to return "appropriate" error values for
 * certain conditions (to make some standard utilities work)
 *
 * Formerly known as: rf_DoAccessKernel
 */
void
raidstart(RF_Raid_t *raidPtr)
{
	struct raid_softc *rs;
	struct dk_softc *dksc;

	rs = raidPtr->softc;
	dksc = &rs->sc_dksc;
	/* quick check to see if anything has died recently */
	rf_lock_mutex2(raidPtr->mutex);
	if (raidPtr->numNewFailures > 0) {
		rf_unlock_mutex2(raidPtr->mutex);
		rf_update_component_labels(raidPtr,
					   RF_NORMAL_COMPONENT_UPDATE);
		rf_lock_mutex2(raidPtr->mutex);
		raidPtr->numNewFailures--;
	}
	rf_unlock_mutex2(raidPtr->mutex);

	if ((rs->sc_flags & RAIDF_INITED) == 0) {
		printf("raid%d: raidstart not ready\n", raidPtr->raidid);
		return;
	}

	dk_start(dksc, NULL);
}

static int
raiddoaccess(RF_Raid_t *raidPtr, struct buf *bp)
{
	RF_SectorCount_t num_blocks, pb, sum;
	RF_RaidAddr_t raid_addr;
	daddr_t blocknum;
	int rc;

	rf_lock_mutex2(raidPtr->mutex);
	if (raidPtr->openings == 0) {
		rf_unlock_mutex2(raidPtr->mutex);
		return EAGAIN;
	}
	rf_unlock_mutex2(raidPtr->mutex);

	blocknum = bp->b_rawblkno;

	db1_printf(("Blocks: %d, %d\n", (int) bp->b_blkno,
		    (int) blocknum));

	db1_printf(("bp->b_bcount = %d\n", (int) bp->b_bcount));
	db1_printf(("bp->b_resid = %d\n", (int) bp->b_resid));

	/* *THIS* is where we adjust what block we're going to...
	 * but DO NOT TOUCH bp->b_blkno!!! */
	raid_addr = blocknum;

	num_blocks = bp->b_bcount >> raidPtr->logBytesPerSector;
	pb = (bp->b_bcount & raidPtr->sectorMask) ? 1 : 0;
	sum = raid_addr + num_blocks + pb;
	if (1 || rf_debugKernelAccess) {
		db1_printf(("raid_addr=%d sum=%d num_blocks=%d(+%d) (%d)\n",
			    (int) raid_addr, (int) sum, (int) num_blocks,
			    (int) pb, (int) bp->b_resid));
	}
	if ((sum > raidPtr->totalSectors) || (sum < raid_addr)
	    || (sum < num_blocks) || (sum < pb)) {
		rc = ENOSPC;
		goto done;
	}
	/*
	 * XXX rf_DoAccess() should do this, not just DoAccessKernel()
	 */

	if (bp->b_bcount & raidPtr->sectorMask) {
		rc = ENOSPC;
		goto done;
	}
	db1_printf(("Calling DoAccess..\n"));


	rf_lock_mutex2(raidPtr->mutex);
	raidPtr->openings--;
	rf_unlock_mutex2(raidPtr->mutex);

	/* don't ever condition on bp->b_flags & B_WRITE.
	 * always condition on B_READ instead */

	rc = rf_DoAccess(raidPtr, (bp->b_flags & B_READ) ?
			 RF_IO_TYPE_READ : RF_IO_TYPE_WRITE,
			 raid_addr, num_blocks,
			 bp->b_data, bp, RF_DAG_NONBLOCKING_IO);

done:
	return rc;
}

/* invoke an I/O from kernel mode.  Disk queue should be locked upon entry */

int
rf_DispatchKernelIO(RF_DiskQueue_t *queue, RF_DiskQueueData_t *req)
{
	int     op = (req->type == RF_IO_TYPE_READ) ? B_READ : B_WRITE;
	struct buf *bp;

	req->queue = queue;
	bp = req->bp;

	switch (req->type) {
	case RF_IO_TYPE_NOP:	/* used primarily to unlock a locked queue */
		/* XXX need to do something extra here.. */
		/* I'm leaving this in, as I've never actually seen it used,
		 * and I'd like folks to report it... GO */
		printf("%s: WAKEUP CALLED\n", __func__);
		queue->numOutstanding++;

		bp->b_flags = 0;
		bp->b_private = req;

		KernelWakeupFunc(bp);
		break;

	case RF_IO_TYPE_READ:
	case RF_IO_TYPE_WRITE:
#if RF_ACC_TRACE > 0
		if (req->tracerec) {
			RF_ETIMER_START(req->tracerec->timer);
		}
#endif
		InitBP(bp, queue->rf_cinfo->ci_vp,
		    op, queue->rf_cinfo->ci_dev,
		    req->sectorOffset, req->numSector,
		    req->buf, KernelWakeupFunc, (void *) req,
		    queue->raidPtr->logBytesPerSector);

		if (rf_debugKernelAccess) {
			db1_printf(("dispatch: bp->b_blkno = %ld\n",
				(long) bp->b_blkno));
		}
		queue->numOutstanding++;
		queue->last_deq_sector = req->sectorOffset;
		/* acc wouldn't have been let in if there were any pending
		 * reqs at any other priority */
		queue->curPriority = req->priority;

		db1_printf(("Going for %c to unit %d col %d\n",
			    req->type, queue->raidPtr->raidid,
			    queue->col));
		db1_printf(("sector %d count %d (%d bytes) %d\n",
			(int) req->sectorOffset, (int) req->numSector,
			(int) (req->numSector <<
			    queue->raidPtr->logBytesPerSector),
			(int) queue->raidPtr->logBytesPerSector));

		/*
		 * XXX: drop lock here since this can block at
		 * least with backing SCSI devices.  Retake it
		 * to minimize fuss with calling interfaces.
		 */

		RF_UNLOCK_QUEUE_MUTEX(queue, "unusedparam");
		bdev_strategy(bp);
		RF_LOCK_QUEUE_MUTEX(queue, "unusedparam");
		break;

	default:
		panic("bad req->type in rf_DispatchKernelIO");
	}
	db1_printf(("Exiting from DispatchKernelIO\n"));

	return 0;
}
/* this is the callback function associated with a I/O invoked from
   kernel code.
 */
static void
KernelWakeupFunc(struct buf *bp)
{
	RF_DiskQueueData_t *req = NULL;
	RF_DiskQueue_t *queue;

	db1_printf(("recovering the request queue:\n"));

	req = bp->b_private;

	queue = (RF_DiskQueue_t *) req->queue;

	rf_lock_mutex2(queue->raidPtr->iodone_lock);

#if RF_ACC_TRACE > 0
	if (req->tracerec) {
		RF_ETIMER_STOP(req->tracerec->timer);
		RF_ETIMER_EVAL(req->tracerec->timer);
		rf_lock_mutex2(rf_tracing_mutex);
		req->tracerec->diskwait_us += RF_ETIMER_VAL_US(req->tracerec->timer);
		req->tracerec->phys_io_us += RF_ETIMER_VAL_US(req->tracerec->timer);
		req->tracerec->num_phys_ios++;
		rf_unlock_mutex2(rf_tracing_mutex);
	}
#endif

	/* XXX Ok, let's get aggressive... If b_error is set, let's go
	 * ballistic, and mark the component as hosed... */

	if (bp->b_error != 0) {
		/* Mark the disk as dead */
		/* but only mark it once... */
		/* and only if it wouldn't leave this RAID set
		   completely broken */
		if (((queue->raidPtr->Disks[queue->col].status ==
		      rf_ds_optimal) ||
		     (queue->raidPtr->Disks[queue->col].status ==
		      rf_ds_used_spare)) &&
		     (queue->raidPtr->numFailures <
		      queue->raidPtr->Layout.map->faultsTolerated)) {
			printf("raid%d: IO Error (%d). Marking %s as failed.\n",
			       queue->raidPtr->raidid,
			       bp->b_error,
			       queue->raidPtr->Disks[queue->col].devname);
			queue->raidPtr->Disks[queue->col].status =
			    rf_ds_failed;
			queue->raidPtr->status = rf_rs_degraded;
			queue->raidPtr->numFailures++;
			queue->raidPtr->numNewFailures++;
		} else {	/* Disk is already dead... */
			/* printf("Disk already marked as dead!\n"); */
		}

	}

	/* Fill in the error value */
	req->error = bp->b_error;

	/* Drop this one on the "finished" queue... */
	TAILQ_INSERT_TAIL(&(queue->raidPtr->iodone), req, iodone_entries);

	/* Let the raidio thread know there is work to be done. */
	rf_signal_cond2(queue->raidPtr->iodone_cv);

	rf_unlock_mutex2(queue->raidPtr->iodone_lock);
}


/*
 * initialize a buf structure for doing an I/O in the kernel.
 */
static void
InitBP(struct buf *bp, struct vnode *b_vp, unsigned rw_flag, dev_t dev,
       RF_SectorNum_t startSect, RF_SectorCount_t numSect, void *bf,
       void (*cbFunc) (struct buf *), void *cbArg, int logBytesPerSector)
{
	bp->b_flags = rw_flag | (bp->b_flags & rf_b_pass);
	bp->b_oflags = 0;
	bp->b_cflags = 0;
	bp->b_bcount = numSect << logBytesPerSector;
	bp->b_bufsize = bp->b_bcount;
	bp->b_error = 0;
	bp->b_dev = dev;
	bp->b_data = bf;
	bp->b_blkno = startSect << logBytesPerSector >> DEV_BSHIFT;
	bp->b_resid = bp->b_bcount;	/* XXX is this right!??!?!! */
	if (bp->b_bcount == 0) {
		panic("bp->b_bcount is zero in InitBP!!");
	}
	bp->b_iodone = cbFunc;
	bp->b_private = cbArg;
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 * (Hmm... where have we seen this warning before :->  GO )
 */
static int
raidlock(struct raid_softc *rs)
{
	int     error;

	error = 0;
	mutex_enter(&rs->sc_mutex);
	while ((rs->sc_flags & RAIDF_LOCKED) != 0) {
		rs->sc_flags |= RAIDF_WANTED;
		error = cv_wait_sig(&rs->sc_cv, &rs->sc_mutex);
		if (error != 0)
			goto done;
	}
	rs->sc_flags |= RAIDF_LOCKED;
done:
	mutex_exit(&rs->sc_mutex);
	return error;
}
/*
 * Unlock and wake up any waiters.
 */
static void
raidunlock(struct raid_softc *rs)
{

	mutex_enter(&rs->sc_mutex);
	rs->sc_flags &= ~RAIDF_LOCKED;
	if ((rs->sc_flags & RAIDF_WANTED) != 0) {
		rs->sc_flags &= ~RAIDF_WANTED;
		cv_broadcast(&rs->sc_cv);
	}
	mutex_exit(&rs->sc_mutex);
}


#define RF_COMPONENT_INFO_OFFSET  16384 /* bytes */
#define RF_COMPONENT_INFO_SIZE     1024 /* bytes */
#define RF_PARITY_MAP_SIZE   RF_PARITYMAP_NBYTE

static daddr_t
rf_component_info_offset(void)
{

	return RF_COMPONENT_INFO_OFFSET;
}

static daddr_t
rf_component_info_size(unsigned secsize)
{
	daddr_t info_size;

	KASSERT(secsize);
	if (secsize > RF_COMPONENT_INFO_SIZE)
		info_size = secsize;
	else
		info_size = RF_COMPONENT_INFO_SIZE;

	return info_size;
}

static daddr_t
rf_parity_map_offset(RF_Raid_t *raidPtr)
{
	daddr_t map_offset;

	KASSERT(raidPtr->bytesPerSector);
	if (raidPtr->bytesPerSector > RF_COMPONENT_INFO_SIZE)
		map_offset = raidPtr->bytesPerSector;
	else
		map_offset = RF_COMPONENT_INFO_SIZE;
	map_offset += rf_component_info_offset();

	return map_offset;
}

static daddr_t
rf_parity_map_size(RF_Raid_t *raidPtr)
{
	daddr_t map_size;

	if (raidPtr->bytesPerSector > RF_PARITY_MAP_SIZE)
		map_size = raidPtr->bytesPerSector;
	else
		map_size = RF_PARITY_MAP_SIZE;

	return map_size;
}

int
raidmarkclean(RF_Raid_t *raidPtr, RF_RowCol_t col)
{
	RF_ComponentLabel_t *clabel;

	clabel = raidget_component_label(raidPtr, col);
	clabel->clean = RF_RAID_CLEAN;
	raidflush_component_label(raidPtr, col);
	return(0);
}


int
raidmarkdirty(RF_Raid_t *raidPtr, RF_RowCol_t col)
{
	RF_ComponentLabel_t *clabel;

	clabel = raidget_component_label(raidPtr, col);
	clabel->clean = RF_RAID_DIRTY;
	raidflush_component_label(raidPtr, col);
	return(0);
}

int
raidfetch_component_label(RF_Raid_t *raidPtr, RF_RowCol_t col)
{
	KASSERT(raidPtr->bytesPerSector);

	return raidread_component_label(raidPtr->bytesPerSector,
	    raidPtr->Disks[col].dev,
	    raidPtr->raid_cinfo[col].ci_vp,
	    &raidPtr->raid_cinfo[col].ci_label);
}

RF_ComponentLabel_t *
raidget_component_label(RF_Raid_t *raidPtr, RF_RowCol_t col)
{
	return &raidPtr->raid_cinfo[col].ci_label;
}

int
raidflush_component_label(RF_Raid_t *raidPtr, RF_RowCol_t col)
{
	RF_ComponentLabel_t *label;

	label = &raidPtr->raid_cinfo[col].ci_label;
	label->mod_counter = raidPtr->mod_counter;
#ifndef RF_NO_PARITY_MAP
	label->parity_map_modcount = label->mod_counter;
#endif
	return raidwrite_component_label(raidPtr->bytesPerSector,
	    raidPtr->Disks[col].dev,
	    raidPtr->raid_cinfo[col].ci_vp, label);
}

/*
 * Swap the label endianness.
 *
 * Everything in the component label is 4-byte-swapped except the version,
 * which is kept in the byte-swapped version at all times, and indicates
 * for the writer that a swap is necessary.
 *
 * For reads it is expected that out_label == clabel, but writes expect
 * separate labels so only the re-swapped label is written out to disk,
 * leaving the swapped-except-version internally.
 *
 * Only support swapping label version 2.
 */
static void
rf_swap_label(RF_ComponentLabel_t *clabel, RF_ComponentLabel_t *out_label)
{
	int	*in, *out, *in_last;

	KASSERT(clabel->version == bswap32(RF_COMPONENT_LABEL_VERSION));

	/* Don't swap the label, but do copy it. */
	out_label->version = clabel->version;

	in = &clabel->serial_number;
	in_last = &clabel->future_use2[42];
	out = &out_label->serial_number;

	for (; in < in_last; in++, out++)
		*out = bswap32(*in);
}

static int
raidread_component_label(unsigned secsize, dev_t dev, struct vnode *b_vp,
    RF_ComponentLabel_t *clabel)
{
	int error;

	error = raidread_component_area(dev, b_vp, clabel,
	    sizeof(RF_ComponentLabel_t),
	    rf_component_info_offset(),
	    rf_component_info_size(secsize));

	if (error == 0 &&
	    clabel->version == bswap32(RF_COMPONENT_LABEL_VERSION)) {
		rf_swap_label(clabel, clabel);
	}

	return error;
}

/* ARGSUSED */
static int
raidread_component_area(dev_t dev, struct vnode *b_vp, void *data,
    size_t msize, daddr_t offset, daddr_t dsize)
{
	struct buf *bp;
	int error;

	/* XXX should probably ensure that we don't try to do this if
	   someone has changed rf_protected_sectors. */

	if (b_vp == NULL) {
		/* For whatever reason, this component is not valid.
		   Don't try to read a component label from it. */
		return(EINVAL);
	}

	/* get a block of the appropriate size... */
	bp = geteblk((int)dsize);
	bp->b_dev = dev;

	/* get our ducks in a row for the read */
	bp->b_blkno = offset / DEV_BSIZE;
	bp->b_bcount = dsize;
	bp->b_flags |= B_READ;
 	bp->b_resid = dsize;

	bdev_strategy(bp);
	error = biowait(bp);

	if (!error) {
		memcpy(data, bp->b_data, msize);
	}

	brelse(bp, 0);
	return(error);
}

static int
raidwrite_component_label(unsigned secsize, dev_t dev, struct vnode *b_vp,
    RF_ComponentLabel_t *clabel)
{
	RF_ComponentLabel_t *clabel_write = clabel;
	RF_ComponentLabel_t lclabel;
	int error;

	if (clabel->version == bswap32(RF_COMPONENT_LABEL_VERSION)) {
		clabel_write = &lclabel;
		rf_swap_label(clabel, clabel_write);
	}
	error = raidwrite_component_area(dev, b_vp, clabel_write,
	    sizeof(RF_ComponentLabel_t),
	    rf_component_info_offset(),
	    rf_component_info_size(secsize), 0);

	return error;
}

/* ARGSUSED */
static int
raidwrite_component_area(dev_t dev, struct vnode *b_vp, void *data,
    size_t msize, daddr_t offset, daddr_t dsize, int asyncp)
{
	struct buf *bp;
	int error;

	/* get a block of the appropriate size... */
	bp = geteblk((int)dsize);
	bp->b_dev = dev;

	/* get our ducks in a row for the write */
	bp->b_blkno = offset / DEV_BSIZE;
	bp->b_bcount = dsize;
	bp->b_flags |= B_WRITE | (asyncp ? B_ASYNC : 0);
 	bp->b_resid = dsize;

	memset(bp->b_data, 0, dsize);
	memcpy(bp->b_data, data, msize);

	bdev_strategy(bp);
	if (asyncp)
		return 0;
	error = biowait(bp);
	brelse(bp, 0);
	if (error) {
#if 1
		printf("Failed to write RAID component info!\n");
#endif
	}

	return(error);
}

void
rf_paritymap_kern_write(RF_Raid_t *raidPtr, struct rf_paritymap_ondisk *map)
{
	int c;

	for (c = 0; c < raidPtr->numCol; c++) {
		/* Skip dead disks. */
		if (RF_DEAD_DISK(raidPtr->Disks[c].status))
			continue;
		/* XXXjld: what if an error occurs here? */
		raidwrite_component_area(raidPtr->Disks[c].dev,
		    raidPtr->raid_cinfo[c].ci_vp, map,
		    RF_PARITYMAP_NBYTE,
		    rf_parity_map_offset(raidPtr),
		    rf_parity_map_size(raidPtr), 0);
	}
}

void
rf_paritymap_kern_read(RF_Raid_t *raidPtr, struct rf_paritymap_ondisk *map)
{
	struct rf_paritymap_ondisk tmp;
	int c,first;

	first=1;
	for (c = 0; c < raidPtr->numCol; c++) {
		/* Skip dead disks. */
		if (RF_DEAD_DISK(raidPtr->Disks[c].status))
			continue;
		raidread_component_area(raidPtr->Disks[c].dev,
		    raidPtr->raid_cinfo[c].ci_vp, &tmp,
		    RF_PARITYMAP_NBYTE,
		    rf_parity_map_offset(raidPtr),
		    rf_parity_map_size(raidPtr));
		if (first) {
			memcpy(map, &tmp, sizeof(*map));
			first = 0;
		} else {
			rf_paritymap_merge(map, &tmp);
		}
	}
}

void
rf_markalldirty(RF_Raid_t *raidPtr)
{
	RF_ComponentLabel_t *clabel;
	int sparecol;
	int c;
	int j;
	int scol = -1;

	raidPtr->mod_counter++;
	for (c = 0; c < raidPtr->numCol; c++) {
		/* we don't want to touch (at all) a disk that has
		   failed */
		if (!RF_DEAD_DISK(raidPtr->Disks[c].status)) {
			clabel = raidget_component_label(raidPtr, c);
			if (clabel->status == rf_ds_spared) {
				/* XXX do something special...
				   but whatever you do, don't
				   try to access it!! */
			} else {
				raidmarkdirty(raidPtr, c);
			}
		}
	}

	for( c = 0; c < raidPtr->numSpare ; c++) {
		sparecol = raidPtr->numCol + c;
		if (raidPtr->Disks[sparecol].status == rf_ds_used_spare) {
			/*

			   we claim this disk is "optimal" if it's
			   rf_ds_used_spare, as that means it should be
			   directly substitutable for the disk it replaced.
			   We note that too...

			 */

			for(j=0;j<raidPtr->numCol;j++) {
				if (raidPtr->Disks[j].spareCol == sparecol) {
					scol = j;
					break;
				}
			}

			clabel = raidget_component_label(raidPtr, sparecol);
			/* make sure status is noted */

			raid_init_component_label(raidPtr, clabel);

			clabel->row = 0;
			clabel->column = scol;
			/* Note: we *don't* change status from rf_ds_used_spare
			   to rf_ds_optimal */
			/* clabel.status = rf_ds_optimal; */

			raidmarkdirty(raidPtr, sparecol);
		}
	}
}


void
rf_update_component_labels(RF_Raid_t *raidPtr, int final)
{
	RF_ComponentLabel_t *clabel;
	int sparecol;
	int c;
	int j;
	int scol;
	struct raid_softc *rs = raidPtr->softc;

	scol = -1;

	/* XXX should do extra checks to make sure things really are clean,
	   rather than blindly setting the clean bit... */

	raidPtr->mod_counter++;

	for (c = 0; c < raidPtr->numCol; c++) {
		if (raidPtr->Disks[c].status == rf_ds_optimal) {
			clabel = raidget_component_label(raidPtr, c);
			/* make sure status is noted */
			clabel->status = rf_ds_optimal;

			/* note what unit we are configured as */
			if ((rs->sc_cflags & RAIDF_UNIT_CHANGED) == 0)
				clabel->last_unit = raidPtr->raidid;

			raidflush_component_label(raidPtr, c);
			if (final == RF_FINAL_COMPONENT_UPDATE) {
				if (raidPtr->parity_good == RF_RAID_CLEAN) {
					raidmarkclean(raidPtr, c);
				}
			}
		}
		/* else we don't touch it.. */
	}

	for( c = 0; c < raidPtr->numSpare ; c++) {
		sparecol = raidPtr->numCol + c;
		/* Need to ensure that the reconstruct actually completed! */
		if (raidPtr->Disks[sparecol].status == rf_ds_used_spare) {
			/*

			   we claim this disk is "optimal" if it's
			   rf_ds_used_spare, as that means it should be
			   directly substitutable for the disk it replaced.
			   We note that too...

			 */

			for(j=0;j<raidPtr->numCol;j++) {
				if (raidPtr->Disks[j].spareCol == sparecol) {
					scol = j;
					break;
				}
			}

			/* XXX shouldn't *really* need this... */
			clabel = raidget_component_label(raidPtr, sparecol);
			/* make sure status is noted */

			raid_init_component_label(raidPtr, clabel);

			clabel->column = scol;
			clabel->status = rf_ds_optimal;
			if ((rs->sc_cflags & RAIDF_UNIT_CHANGED) == 0)
				clabel->last_unit = raidPtr->raidid;

			raidflush_component_label(raidPtr, sparecol);
			if (final == RF_FINAL_COMPONENT_UPDATE) {
				if (raidPtr->parity_good == RF_RAID_CLEAN) {
					raidmarkclean(raidPtr, sparecol);
				}
			}
		}
	}
}

void
rf_close_component(RF_Raid_t *raidPtr, struct vnode *vp, int auto_configured)
{

	if (vp != NULL) {
		if (auto_configured == 1) {
			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			VOP_CLOSE(vp, FREAD | FWRITE, NOCRED);
			vput(vp);

		} else {
			(void) vn_close(vp, FREAD | FWRITE, curlwp->l_cred);
		}
	}
}


void
rf_UnconfigureVnodes(RF_Raid_t *raidPtr)
{
	int r,c;
	struct vnode *vp;
	int acd;


	/* We take this opportunity to close the vnodes like we should.. */

	for (c = 0; c < raidPtr->numCol; c++) {
		vp = raidPtr->raid_cinfo[c].ci_vp;
		acd = raidPtr->Disks[c].auto_configured;
		rf_close_component(raidPtr, vp, acd);
		raidPtr->raid_cinfo[c].ci_vp = NULL;
		raidPtr->Disks[c].auto_configured = 0;
	}

	for (r = 0; r < raidPtr->numSpare; r++) {
		vp = raidPtr->raid_cinfo[raidPtr->numCol + r].ci_vp;
		acd = raidPtr->Disks[raidPtr->numCol + r].auto_configured;
		rf_close_component(raidPtr, vp, acd);
		raidPtr->raid_cinfo[raidPtr->numCol + r].ci_vp = NULL;
		raidPtr->Disks[raidPtr->numCol + r].auto_configured = 0;
	}
}


static void
rf_ReconThread(struct rf_recon_req_internal *req)
{
	int     s;
	RF_Raid_t *raidPtr;

	s = splbio();
	raidPtr = (RF_Raid_t *) req->raidPtr;
	raidPtr->recon_in_progress = 1;

	if (req->flags & RF_FDFLAGS_RECON_FORCE) {
		raidPtr->forceRecon = 1;
	}
	
	rf_FailDisk((RF_Raid_t *) req->raidPtr, req->col,
		    ((req->flags & RF_FDFLAGS_RECON) ? 1 : 0));

	if (req->flags & RF_FDFLAGS_RECON_FORCE) {
		raidPtr->forceRecon = 0;
	}

	RF_Free(req, sizeof(*req));

	raidPtr->recon_in_progress = 0;
	splx(s);

	/* That's all... */
	kthread_exit(0);	/* does not return */
}

static void
rf_RewriteParityThread(RF_Raid_t *raidPtr)
{
	int retcode;
	int s;

	raidPtr->parity_rewrite_stripes_done = 0;
	raidPtr->parity_rewrite_in_progress = 1;
	s = splbio();
	retcode = rf_RewriteParity(raidPtr);
	splx(s);
	if (retcode) {
		printf("raid%d: Error re-writing parity (%d)!\n",
		    raidPtr->raidid, retcode);
	} else {
		/* set the clean bit!  If we shutdown correctly,
		   the clean bit on each component label will get
		   set */
		raidPtr->parity_good = RF_RAID_CLEAN;
	}
	raidPtr->parity_rewrite_in_progress = 0;

	/* Anyone waiting for us to stop?  If so, inform them... */
	if (raidPtr->waitShutdown) {
		rf_lock_mutex2(raidPtr->rad_lock);
		cv_broadcast(&raidPtr->parity_rewrite_cv);
		rf_unlock_mutex2(raidPtr->rad_lock);
	}

	/* That's all... */
	kthread_exit(0);	/* does not return */
}


static void
rf_CopybackThread(RF_Raid_t *raidPtr)
{
	int s;

	raidPtr->copyback_in_progress = 1;
	s = splbio();
	rf_CopybackReconstructedData(raidPtr);
	splx(s);
	raidPtr->copyback_in_progress = 0;

	/* That's all... */
	kthread_exit(0);	/* does not return */
}


static void
rf_ReconstructInPlaceThread(struct rf_recon_req_internal *req)
{
	int s;
	RF_Raid_t *raidPtr;

	s = splbio();
	raidPtr = req->raidPtr;
	raidPtr->recon_in_progress = 1;

	if (req->flags & RF_FDFLAGS_RECON_FORCE) {
		raidPtr->forceRecon = 1;
	}

	rf_ReconstructInPlace(raidPtr, req->col);

	if (req->flags & RF_FDFLAGS_RECON_FORCE) {
		raidPtr->forceRecon = 0;
	}

	RF_Free(req, sizeof(*req));
	raidPtr->recon_in_progress = 0;
	splx(s);

	/* That's all... */
	kthread_exit(0);	/* does not return */
}

static RF_AutoConfig_t *
rf_get_component(RF_AutoConfig_t *ac_list, dev_t dev, struct vnode *vp,
    const char *cname, RF_SectorCount_t size, uint64_t numsecs,
    unsigned secsize)
{
	int good_one = 0;
	RF_ComponentLabel_t *clabel;
	RF_AutoConfig_t *ac;

	clabel = malloc(sizeof(RF_ComponentLabel_t), M_RAIDFRAME, M_WAITOK);

	if (!raidread_component_label(secsize, dev, vp, clabel)) {
		/* Got the label.  Does it look reasonable? */
		if (rf_reasonable_label(clabel, numsecs) &&
		    (rf_component_label_partitionsize(clabel) <= size)) {
#ifdef DEBUG
			printf("Component on: %s: %llu\n",
				cname, (unsigned long long)size);
			rf_print_component_label(clabel);
#endif
			/* if it's reasonable, add it, else ignore it. */
			ac = malloc(sizeof(RF_AutoConfig_t), M_RAIDFRAME,
				M_WAITOK);
			strlcpy(ac->devname, cname, sizeof(ac->devname));
			ac->dev = dev;
			ac->vp = vp;
			ac->clabel = clabel;
			ac->next = ac_list;
			ac_list = ac;
			good_one = 1;
		}
	}
	if (!good_one) {
		/* cleanup */
		free(clabel, M_RAIDFRAME);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
		VOP_CLOSE(vp, FREAD | FWRITE, NOCRED);
		vput(vp);
	}
	return ac_list;
}

static RF_AutoConfig_t *
rf_find_raid_components(void)
{
	struct vnode *vp;
	struct disklabel label;
	device_t dv;
	deviter_t di;
	dev_t dev;
	int bmajor, bminor, wedge, rf_part_found;
	int error;
	int i;
	RF_AutoConfig_t *ac_list;
	uint64_t numsecs;
	unsigned secsize;
	int dowedges;

	/* initialize the AutoConfig list */
	ac_list = NULL;

	/*
	 * we begin by trolling through *all* the devices on the system *twice*
	 * first we scan for wedges, second for other devices. This avoids
	 * using a raw partition instead of a wedge that covers the whole disk
	 */

	for (dowedges=1; dowedges>=0; --dowedges) {
		for (dv = deviter_first(&di, DEVITER_F_ROOT_FIRST); dv != NULL;
		     dv = deviter_next(&di)) {

			/* we are only interested in disks */
			if (device_class(dv) != DV_DISK)
				continue;

			/* we don't care about floppies */
			if (device_is_a(dv, "fd")) {
				continue;
			}

			/* we don't care about CDs. */
			if (device_is_a(dv, "cd")) {
				continue;
			}

			/* we don't care about md. */
			if (device_is_a(dv, "md")) {
				continue;
			}

			/* hdfd is the Atari/Hades floppy driver */
			if (device_is_a(dv, "hdfd")) {
				continue;
			}

			/* fdisa is the Atari/Milan floppy driver */
			if (device_is_a(dv, "fdisa")) {
				continue;
			}

			/* we don't care about spiflash */
			if (device_is_a(dv, "spiflash")) {
				continue;
			}

			/* are we in the wedges pass ? */
			wedge = device_is_a(dv, "dk");
			if (wedge != dowedges) {
				continue;
			}

			/* need to find the device_name_to_block_device_major stuff */
			bmajor = devsw_name2blk(device_xname(dv), NULL, 0);

			rf_part_found = 0; /*No raid partition as yet*/

			/* get a vnode for the raw partition of this disk */
			bminor = minor(device_unit(dv));
			dev = wedge ? makedev(bmajor, bminor) :
			    MAKEDISKDEV(bmajor, bminor, RAW_PART);
			if (bdevvp(dev, &vp))
				panic("RAID can't alloc vnode");

			vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
			error = VOP_OPEN(vp, FREAD | FSILENT, NOCRED);

			if (error) {
				/* "Who cares."  Continue looking
				   for something that exists*/
				vput(vp);
				continue;
			}

			error = getdisksize(vp, &numsecs, &secsize);
			if (error) {
				/*
				 * Pseudo devices like vnd and cgd can be
				 * opened but may still need some configuration.
				 * Ignore these quietly.
				 */
				if (error != ENXIO)
					printf("RAIDframe: can't get disk size"
					    " for dev %s (%d)\n",
					    device_xname(dv), error);
				VOP_CLOSE(vp, FREAD | FWRITE, NOCRED);
				vput(vp);
				continue;
			}
			if (wedge) {
				struct dkwedge_info dkw;
				error = VOP_IOCTL(vp, DIOCGWEDGEINFO, &dkw, FREAD,
				    NOCRED);
				if (error) {
					printf("RAIDframe: can't get wedge info for "
					    "dev %s (%d)\n", device_xname(dv), error);
					VOP_CLOSE(vp, FREAD | FWRITE, NOCRED);
					vput(vp);
					continue;
				}

				if (strcmp(dkw.dkw_ptype, DKW_PTYPE_RAIDFRAME) != 0) {
					VOP_CLOSE(vp, FREAD | FWRITE, NOCRED);
					vput(vp);
					continue;
				}

				VOP_UNLOCK(vp);
				ac_list = rf_get_component(ac_list, dev, vp,
				    device_xname(dv), dkw.dkw_size, numsecs, secsize);
				rf_part_found = 1; /*There is a raid component on this disk*/
				continue;
			}

			/* Ok, the disk exists.  Go get the disklabel. */
			error = VOP_IOCTL(vp, DIOCGDINFO, &label, FREAD, NOCRED);
			if (error) {
				/*
				 * XXX can't happen - open() would
				 * have errored out (or faked up one)
				 */
				if (error != ENOTTY)
					printf("RAIDframe: can't get label for dev "
					    "%s (%d)\n", device_xname(dv), error);
			}

			/* don't need this any more.  We'll allocate it again
			   a little later if we really do... */
			VOP_CLOSE(vp, FREAD | FWRITE, NOCRED);
			vput(vp);

			if (error)
				continue;

			rf_part_found = 0; /*No raid partitions yet*/
			for (i = 0; i < label.d_npartitions; i++) {
				char cname[sizeof(ac_list->devname)];

				/* We only support partitions marked as RAID */
				if (label.d_partitions[i].p_fstype != FS_RAID)
					continue;

				dev = MAKEDISKDEV(bmajor, device_unit(dv), i);
				if (bdevvp(dev, &vp))
					panic("RAID can't alloc vnode");

				vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
				error = VOP_OPEN(vp, FREAD, NOCRED);
				if (error) {
					/* Not quite a 'whatever'.  In
					 * this situation we know 
					 * there is a FS_RAID
					 * partition, but we can't
					 * open it.  The most likely
					 * reason is that the
					 * partition is already in
					 * use by another RAID set.
					 * So note that we've already
					 * found a partition on this
					 * disk so we don't attempt
					 * to use the raw disk later. */
					rf_part_found = 1;
					vput(vp);
					continue;
				}
				VOP_UNLOCK(vp);
				snprintf(cname, sizeof(cname), "%s%c",
				    device_xname(dv), 'a' + i);
				ac_list = rf_get_component(ac_list, dev, vp, cname,
					label.d_partitions[i].p_size, numsecs, secsize);
				rf_part_found = 1; /*There is at least one raid partition on this disk*/
			}

			/*
			 *If there is no raid component on this disk, either in a
			 *disklabel or inside a wedge, check the raw partition as well,
			 *as it is possible to configure raid components on raw disk
			 *devices.
			 */

			if (!rf_part_found) {
				char cname[sizeof(ac_list->devname)];

				dev = MAKEDISKDEV(bmajor, device_unit(dv), RAW_PART);
				if (bdevvp(dev, &vp))
					panic("RAID can't alloc vnode");

				vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

				error = VOP_OPEN(vp, FREAD, NOCRED);
				if (error) {
					/* Whatever... */
					vput(vp);
					continue;
				}
				VOP_UNLOCK(vp);
				snprintf(cname, sizeof(cname), "%s%c",
				    device_xname(dv), 'a' + RAW_PART);
				ac_list = rf_get_component(ac_list, dev, vp, cname,
					label.d_partitions[RAW_PART].p_size, numsecs, secsize);
			}
		}
		deviter_release(&di);
	}
	return ac_list;
}

int
rf_reasonable_label(RF_ComponentLabel_t *clabel, uint64_t numsecs)
{

	if ((clabel->version==RF_COMPONENT_LABEL_VERSION_1 ||
	     clabel->version==RF_COMPONENT_LABEL_VERSION ||
	     clabel->version == bswap32(RF_COMPONENT_LABEL_VERSION)) &&
	    (clabel->clean == RF_RAID_CLEAN ||
	     clabel->clean == RF_RAID_DIRTY) &&
	    clabel->row >=0 &&
	    clabel->column >= 0 &&
	    clabel->num_rows > 0 &&
	    clabel->num_columns > 0 &&
	    clabel->row < clabel->num_rows &&
	    clabel->column < clabel->num_columns &&
	    clabel->blockSize > 0 &&
	    /*
	     * numBlocksHi may contain garbage, but it is ok since
	     * the type is unsigned.  If it is really garbage,
	     * rf_fix_old_label_size() will fix it.
	     */
	    rf_component_label_numblocks(clabel) > 0) {
		/*
		 * label looks reasonable enough...
		 * let's make sure it has no old garbage.
		 */
		if (numsecs)
			rf_fix_old_label_size(clabel, numsecs);
		return(1);
	}
	return(0);
}


/*
 * For reasons yet unknown, some old component labels have garbage in
 * the newer numBlocksHi region, and this causes lossage.  Since those
 * disks will also have numsecs set to less than 32 bits of sectors,
 * we can determine when this corruption has occurred, and fix it.
 *
 * The exact same problem, with the same unknown reason, happens to
 * the partitionSizeHi member as well.
 */
static void
rf_fix_old_label_size(RF_ComponentLabel_t *clabel, uint64_t numsecs)
{

	if (numsecs < ((uint64_t)1 << 32)) {
		if (clabel->numBlocksHi) {
			printf("WARNING: total sectors < 32 bits, yet "
			       "numBlocksHi set\n"
			       "WARNING: resetting numBlocksHi to zero.\n");
			clabel->numBlocksHi = 0;
		}

		if (clabel->partitionSizeHi) {
			printf("WARNING: total sectors < 32 bits, yet "
			       "partitionSizeHi set\n"
			       "WARNING: resetting partitionSizeHi to zero.\n");
			clabel->partitionSizeHi = 0;
		}
	}
}


#ifdef DEBUG
void
rf_print_component_label(RF_ComponentLabel_t *clabel)
{
	uint64_t numBlocks;
	static const char *rp[] = {
	    "No", "Force", "Soft", "*invalid*"
	};


	numBlocks = rf_component_label_numblocks(clabel);

	printf("   Row: %d Column: %d Num Rows: %d Num Columns: %d\n",
	       clabel->row, clabel->column,
	       clabel->num_rows, clabel->num_columns);
	printf("   Version: %d Serial Number: %d Mod Counter: %d\n",
	       clabel->version, clabel->serial_number,
	       clabel->mod_counter);
	printf("   Clean: %s Status: %d\n",
	       clabel->clean ? "Yes" : "No", clabel->status);
	printf("   sectPerSU: %d SUsPerPU: %d SUsPerRU: %d\n",
	       clabel->sectPerSU, clabel->SUsPerPU, clabel->SUsPerRU);
	printf("   RAID Level: %c  blocksize: %d numBlocks: %"PRIu64"\n",
	       (char) clabel->parityConfig, clabel->blockSize, numBlocks);
	printf("   Autoconfig: %s\n", clabel->autoconfigure ? "Yes" : "No");
	printf("   Root partition: %s\n", rp[clabel->root_partition & 3]);
	printf("   Last configured as: raid%d\n", clabel->last_unit);
#if 0
	   printf("   Config order: %d\n", clabel->config_order);
#endif

}
#endif

static RF_ConfigSet_t *
rf_create_auto_sets(RF_AutoConfig_t *ac_list)
{
	RF_AutoConfig_t *ac;
	RF_ConfigSet_t *config_sets;
	RF_ConfigSet_t *cset;
	RF_AutoConfig_t *ac_next;


	config_sets = NULL;

	/* Go through the AutoConfig list, and figure out which components
	   belong to what sets.  */
	ac = ac_list;
	while(ac!=NULL) {
		/* we're going to putz with ac->next, so save it here
		   for use at the end of the loop */
		ac_next = ac->next;

		if (config_sets == NULL) {
			/* will need at least this one... */
			config_sets = malloc(sizeof(RF_ConfigSet_t),
				       M_RAIDFRAME, M_WAITOK);
			/* this one is easy :) */
			config_sets->ac = ac;
			config_sets->next = NULL;
			config_sets->rootable = 0;
			ac->next = NULL;
		} else {
			/* which set does this component fit into? */
			cset = config_sets;
			while(cset!=NULL) {
				if (rf_does_it_fit(cset, ac)) {
					/* looks like it matches... */
					ac->next = cset->ac;
					cset->ac = ac;
					break;
				}
				cset = cset->next;
			}
			if (cset==NULL) {
				/* didn't find a match above... new set..*/
				cset = malloc(sizeof(RF_ConfigSet_t),
					       M_RAIDFRAME, M_WAITOK);
				cset->ac = ac;
				ac->next = NULL;
				cset->next = config_sets;
				cset->rootable = 0;
				config_sets = cset;
			}
		}
		ac = ac_next;
	}


	return(config_sets);
}

static int
rf_does_it_fit(RF_ConfigSet_t *cset, RF_AutoConfig_t *ac)
{
	RF_ComponentLabel_t *clabel1, *clabel2;

	/* If this one matches the *first* one in the set, that's good
	   enough, since the other members of the set would have been
	   through here too... */
	/* note that we are not checking partitionSize here..

	   Note that we are also not checking the mod_counters here.
	   If everything else matches except the mod_counter, that's
	   good enough for this test.  We will deal with the mod_counters
	   a little later in the autoconfiguration process.

	    (clabel1->mod_counter == clabel2->mod_counter) &&

	   The reason we don't check for this is that failed disks
	   will have lower modification counts.  If those disks are
	   not added to the set they used to belong to, then they will
	   form their own set, which may result in 2 different sets,
	   for example, competing to be configured at raid0, and
	   perhaps competing to be the root filesystem set.  If the
	   wrong ones get configured, or both attempt to become /,
	   weird behaviour and or serious lossage will occur.  Thus we
	   need to bring them into the fold here, and kick them out at
	   a later point.

	*/

	clabel1 = cset->ac->clabel;
	clabel2 = ac->clabel;
	if ((clabel1->version == clabel2->version) &&
	    (clabel1->serial_number == clabel2->serial_number) &&
	    (clabel1->num_rows == clabel2->num_rows) &&
	    (clabel1->num_columns == clabel2->num_columns) &&
	    (clabel1->sectPerSU == clabel2->sectPerSU) &&
	    (clabel1->SUsPerPU == clabel2->SUsPerPU) &&
	    (clabel1->SUsPerRU == clabel2->SUsPerRU) &&
	    (clabel1->parityConfig == clabel2->parityConfig) &&
	    (clabel1->maxOutstanding == clabel2->maxOutstanding) &&
	    (clabel1->blockSize == clabel2->blockSize) &&
	    rf_component_label_numblocks(clabel1) ==
	    rf_component_label_numblocks(clabel2) &&
	    (clabel1->autoconfigure == clabel2->autoconfigure) &&
	    (clabel1->root_partition == clabel2->root_partition) &&
	    (clabel1->last_unit == clabel2->last_unit) &&
	    (clabel1->config_order == clabel2->config_order)) {
		/* if it get's here, it almost *has* to be a match */
	} else {
		/* it's not consistent with somebody in the set..
		   punt */
		return(0);
	}
	/* all was fine.. it must fit... */
	return(1);
}

static int
rf_have_enough_components(RF_ConfigSet_t *cset)
{
	RF_AutoConfig_t *ac;
	RF_AutoConfig_t *auto_config;
	RF_ComponentLabel_t *clabel;
	int c;
	int num_cols;
	int num_missing;
	int mod_counter;
	int mod_counter_found;
	int even_pair_failed;
	char parity_type;


	/* check to see that we have enough 'live' components
	   of this set.  If so, we can configure it if necessary */

	num_cols = cset->ac->clabel->num_columns;
	parity_type = cset->ac->clabel->parityConfig;

	/* XXX Check for duplicate components!?!?!? */

	/* Determine what the mod_counter is supposed to be for this set. */

	mod_counter_found = 0;
	mod_counter = 0;
	ac = cset->ac;
	while(ac!=NULL) {
		if (mod_counter_found==0) {
			mod_counter = ac->clabel->mod_counter;
			mod_counter_found = 1;
		} else {
			if (ac->clabel->mod_counter > mod_counter) {
				mod_counter = ac->clabel->mod_counter;
			}
		}
		ac = ac->next;
	}

	num_missing = 0;
	auto_config = cset->ac;

	even_pair_failed = 0;
	for(c=0; c<num_cols; c++) {
		ac = auto_config;
		while(ac!=NULL) {
			if ((ac->clabel->column == c) &&
			    (ac->clabel->mod_counter == mod_counter)) {
				/* it's this one... */
#ifdef DEBUG
				printf("Found: %s at %d\n",
				       ac->devname,c);
#endif
				break;
			}
			ac=ac->next;
		}
		if (ac==NULL) {
				/* Didn't find one here! */
				/* special case for RAID 1, especially
				   where there are more than 2
				   components (where RAIDframe treats
				   things a little differently :( ) */
			if (parity_type == '1') {
				if (c%2 == 0) { /* even component */
					even_pair_failed = 1;
				} else { /* odd component.  If
					    we're failed, and
					    so is the even
					    component, it's
					    "Good Night, Charlie" */
					if (even_pair_failed == 1) {
						return(0);
					}
				}
			} else {
				/* normal accounting */
				num_missing++;
			}
		}
		if ((parity_type == '1') && (c%2 == 1)) {
				/* Just did an even component, and we didn't
				   bail.. reset the even_pair_failed flag,
				   and go on to the next component.... */
			even_pair_failed = 0;
		}
	}

	clabel = cset->ac->clabel;

	if (((clabel->parityConfig == '0') && (num_missing > 0)) ||
	    ((clabel->parityConfig == '4') && (num_missing > 1)) ||
	    ((clabel->parityConfig == '5') && (num_missing > 1))) {
		/* XXX this needs to be made *much* more general */
		/* Too many failures */
		return(0);
	}
	/* otherwise, all is well, and we've got enough to take a kick
	   at autoconfiguring this set */
	return(1);
}

static void
rf_create_configuration(RF_AutoConfig_t *ac, RF_Config_t *config,
			RF_Raid_t *raidPtr)
{
	RF_ComponentLabel_t *clabel;
	int i;

	clabel = ac->clabel;

	/* 1. Fill in the common stuff */
	config->numCol = clabel->num_columns;
	config->numSpare = 0; /* XXX should this be set here? */
	config->sectPerSU = clabel->sectPerSU;
	config->SUsPerPU = clabel->SUsPerPU;
	config->SUsPerRU = clabel->SUsPerRU;
	config->parityConfig = clabel->parityConfig;
	/* XXX... */
	strcpy(config->diskQueueType,"fifo");
	config->maxOutstandingDiskReqs = clabel->maxOutstanding;
	config->layoutSpecificSize = 0; /* XXX ?? */

	while(ac!=NULL) {
		/* row/col values will be in range due to the checks
		   in reasonable_label() */
		strcpy(config->devnames[0][ac->clabel->column],
		       ac->devname);
		ac = ac->next;
	}

	for(i=0;i<RF_MAXDBGV;i++) {
		config->debugVars[i][0] = 0;
	}
}

static int
rf_set_autoconfig(RF_Raid_t *raidPtr, int new_value)
{
	RF_ComponentLabel_t *clabel;
	int column;
	int sparecol;

	raidPtr->autoconfigure = new_value;

	for(column=0; column<raidPtr->numCol; column++) {
		if (raidPtr->Disks[column].status == rf_ds_optimal) {
			clabel = raidget_component_label(raidPtr, column);
			clabel->autoconfigure = new_value;
			raidflush_component_label(raidPtr, column);
		}
	}
	for(column = 0; column < raidPtr->numSpare ; column++) {
		sparecol = raidPtr->numCol + column;
		if (raidPtr->Disks[sparecol].status == rf_ds_used_spare) {
			clabel = raidget_component_label(raidPtr, sparecol);
			clabel->autoconfigure = new_value;
			raidflush_component_label(raidPtr, sparecol);
		}
	}
	return(new_value);
}

static int
rf_set_rootpartition(RF_Raid_t *raidPtr, int new_value)
{
	RF_ComponentLabel_t *clabel;
	int column;
	int sparecol;

	raidPtr->root_partition = new_value;
	for(column=0; column<raidPtr->numCol; column++) {
		if (raidPtr->Disks[column].status == rf_ds_optimal) {
			clabel = raidget_component_label(raidPtr, column);
			clabel->root_partition = new_value;
			raidflush_component_label(raidPtr, column);
		}
	}
	for(column = 0; column < raidPtr->numSpare ; column++) {
		sparecol = raidPtr->numCol + column;
		if (raidPtr->Disks[sparecol].status == rf_ds_used_spare) {
			clabel = raidget_component_label(raidPtr, sparecol);
			clabel->root_partition = new_value;
			raidflush_component_label(raidPtr, sparecol);
		}
	}
	return(new_value);
}

static void
rf_release_all_vps(RF_ConfigSet_t *cset)
{
	RF_AutoConfig_t *ac;

	ac = cset->ac;
	while(ac!=NULL) {
		/* Close the vp, and give it back */
		if (ac->vp) {
			vn_lock(ac->vp, LK_EXCLUSIVE | LK_RETRY);
			VOP_CLOSE(ac->vp, FREAD | FWRITE, NOCRED);
			vput(ac->vp);
			ac->vp = NULL;
		}
		ac = ac->next;
	}
}


static void
rf_cleanup_config_set(RF_ConfigSet_t *cset)
{
	RF_AutoConfig_t *ac;
	RF_AutoConfig_t *next_ac;

	ac = cset->ac;
	while(ac!=NULL) {
		next_ac = ac->next;
		/* nuke the label */
		free(ac->clabel, M_RAIDFRAME);
		/* cleanup the config structure */
		free(ac, M_RAIDFRAME);
		/* "next.." */
		ac = next_ac;
	}
	/* and, finally, nuke the config set */
	free(cset, M_RAIDFRAME);
}


void
raid_init_component_label(RF_Raid_t *raidPtr, RF_ComponentLabel_t *clabel)
{
	/* avoid over-writing byteswapped version. */
	if (clabel->version != bswap32(RF_COMPONENT_LABEL_VERSION))
		clabel->version = RF_COMPONENT_LABEL_VERSION;
	clabel->serial_number = raidPtr->serial_number;
	clabel->mod_counter = raidPtr->mod_counter;

	clabel->num_rows = 1;
	clabel->num_columns = raidPtr->numCol;
	clabel->clean = RF_RAID_DIRTY; /* not clean */
	clabel->status = rf_ds_optimal; /* "It's good!" */

	clabel->sectPerSU = raidPtr->Layout.sectorsPerStripeUnit;
	clabel->SUsPerPU = raidPtr->Layout.SUsPerPU;
	clabel->SUsPerRU = raidPtr->Layout.SUsPerRU;

	clabel->blockSize = raidPtr->bytesPerSector;
	rf_component_label_set_numblocks(clabel, raidPtr->sectorsPerDisk);

	/* XXX not portable */
	clabel->parityConfig = raidPtr->Layout.map->parityConfig;
	clabel->maxOutstanding = raidPtr->maxOutstanding;
	clabel->autoconfigure = raidPtr->autoconfigure;
	clabel->root_partition = raidPtr->root_partition;
	clabel->last_unit = raidPtr->raidid;
	clabel->config_order = raidPtr->config_order;

#ifndef RF_NO_PARITY_MAP
	rf_paritymap_init_label(raidPtr->parity_map, clabel);
#endif
}

static struct raid_softc *
rf_auto_config_set(RF_ConfigSet_t *cset)
{
	RF_Raid_t *raidPtr;
	RF_Config_t *config;
	int raidID;
	struct raid_softc *sc;

#ifdef DEBUG
	printf("RAID autoconfigure\n");
#endif

	/* 1. Create a config structure */
	config = malloc(sizeof(*config), M_RAIDFRAME, M_WAITOK|M_ZERO);

	/*
	   2. Figure out what RAID ID this one is supposed to live at
	   See if we can get the same RAID dev that it was configured
	   on last time..
	*/

	raidID = cset->ac->clabel->last_unit;
	for (sc = raidget(raidID, false); sc && sc->sc_r.valid != 0;
	     sc = raidget(++raidID, false))
		continue;
#ifdef DEBUG
	printf("Configuring raid%d:\n",raidID);
#endif

	if (sc == NULL)
		sc = raidget(raidID, true);
	raidPtr = &sc->sc_r;

	/* XXX all this stuff should be done SOMEWHERE ELSE! */
	raidPtr->softc = sc;
	raidPtr->raidid = raidID;
	raidPtr->openings = RAIDOUTSTANDING;

	/* 3. Build the configuration structure */
	rf_create_configuration(cset->ac, config, raidPtr);

	/* 4. Do the configuration */
	if (rf_Configure(raidPtr, config, cset->ac) == 0) {
		raidinit(sc);

		rf_markalldirty(raidPtr);
		raidPtr->autoconfigure = 1; /* XXX do this here? */
		switch (cset->ac->clabel->root_partition) {
		case 1:	/* Force Root */
		case 2:	/* Soft Root: root when boot partition part of raid */
			/*
			 * everything configured just fine.  Make a note
			 * that this set is eligible to be root,
			 * or forced to be root
			 */
			cset->rootable = cset->ac->clabel->root_partition;
			/* XXX do this here? */
			raidPtr->root_partition = cset->rootable;
			break;
		default:
			break;
		}
	} else {
		raidput(sc);
		sc = NULL;
	}

	/* 5. Cleanup */
	free(config, M_RAIDFRAME);
	return sc;
}

void
rf_pool_init(RF_Raid_t *raidPtr, char *w_chan, struct pool *p, size_t size, const char *pool_name,
	     size_t xmin, size_t xmax)
{

	/* Format: raid%d_foo */
	snprintf(w_chan, RF_MAX_POOLNAMELEN, "raid%d_%s", raidPtr->raidid, pool_name);
	
	pool_init(p, size, 0, 0, 0, w_chan, NULL, IPL_BIO);
	pool_sethiwat(p, xmax);
	pool_prime(p, xmin);
}


/*
 * rf_buf_queue_check(RF_Raid_t raidPtr) -- looks into the buffer queue
 * to see if there is IO pending and if that IO could possibly be done
 * for a given RAID set.  Returns 0 if IO is waiting and can be done, 1
 * otherwise.
 *
 */
int
rf_buf_queue_check(RF_Raid_t *raidPtr)
{
	struct raid_softc *rs;
	struct dk_softc *dksc;

	rs = raidPtr->softc;
	dksc = &rs->sc_dksc;

	if ((rs->sc_flags & RAIDF_INITED) == 0)
		return 1;

	if (dk_strategy_pending(dksc) && raidPtr->openings > 0) {
		/* there is work to do */
		return 0;
	}
	/* default is nothing to do */
	return 1;
}

int
rf_getdisksize(struct vnode *vp, RF_RaidDisk_t *diskPtr)
{
	uint64_t numsecs;
	unsigned secsize;
	int error;

	error = getdisksize(vp, &numsecs, &secsize);
	if (error == 0) {
		diskPtr->blockSize = secsize;
		diskPtr->numBlocks = numsecs - rf_protectedSectors;
		diskPtr->partitionSize = numsecs;
		return 0;
	}
	return error;
}

static int
raid_match(device_t self, cfdata_t cfdata, void *aux)
{
	return 1;
}

static void
raid_attach(device_t parent, device_t self, void *aux)
{
}


static int
raid_detach(device_t self, int flags)
{
	int error;
	struct raid_softc *rs = raidsoftc(self);

	if (rs == NULL)
		return ENXIO;

	if ((error = raidlock(rs)) != 0)
		return error;

	error = raid_detach_unlocked(rs);

	raidunlock(rs);

	/* XXX raid can be referenced here */

	if (error)
		return error;

	/* Free the softc */
	raidput(rs);

	return 0;
}

static void
rf_set_geometry(struct raid_softc *rs, RF_Raid_t *raidPtr)
{
	struct dk_softc *dksc = &rs->sc_dksc;
	struct disk_geom *dg = &dksc->sc_dkdev.dk_geom;

	memset(dg, 0, sizeof(*dg));

	dg->dg_secperunit = raidPtr->totalSectors;
	dg->dg_secsize = raidPtr->bytesPerSector;
	dg->dg_nsectors = raidPtr->Layout.dataSectorsPerStripe;
	dg->dg_ntracks = 4 * raidPtr->numCol;

	disk_set_info(dksc->sc_dev, &dksc->sc_dkdev, NULL);
}

/*
 * Get cache info for all the components (including spares).
 * Returns intersection of all the cache flags of all disks, or first
 * error if any encountered.
 * XXXfua feature flags can change as spares are added - lock down somehow
 */
static int
rf_get_component_caches(RF_Raid_t *raidPtr, int *data)
{
	int c;
	int error;
	int dkwhole = 0, dkpart;

	for (c = 0; c < raidPtr->numCol + raidPtr->numSpare; c++) {
		/*
		 * Check any non-dead disk, even when currently being
		 * reconstructed.
		 */
		if (!RF_DEAD_DISK(raidPtr->Disks[c].status)
		    || raidPtr->Disks[c].status == rf_ds_reconstructing) {
			error = VOP_IOCTL(raidPtr->raid_cinfo[c].ci_vp,
			    DIOCGCACHE, &dkpart, FREAD, NOCRED);
			if (error) {
				if (error != ENODEV) {
					printf("raid%d: get cache for component %s failed\n",
					    raidPtr->raidid,
					    raidPtr->Disks[c].devname);
				}

				return error;
			}

			if (c == 0)
				dkwhole = dkpart;
			else
				dkwhole = DKCACHE_COMBINE(dkwhole, dkpart);
		}
	}

	*data = dkwhole;

	return 0;
}

/*
 * Implement forwarding of the DIOCCACHESYNC ioctl to each of the components.
 * We end up returning whatever error was returned by the first cache flush
 * that fails.
 */

static int
rf_sync_component_cache(RF_Raid_t *raidPtr, int c, int force)
{
	int e = 0;
	for (int i = 0; i < 5; i++) {
		e = VOP_IOCTL(raidPtr->raid_cinfo[c].ci_vp, DIOCCACHESYNC,
		    &force, FWRITE, NOCRED);
		if (!e || e == ENODEV)
			return e;
		printf("raid%d: cache flush[%d] to component %s failed (%d)\n",
		    raidPtr->raidid, i, raidPtr->Disks[c].devname, e);
	}
	return e;
}

int
rf_sync_component_caches(RF_Raid_t *raidPtr, int force)
{
	int c, error;

	error = 0;
	for (c = 0; c < raidPtr->numCol; c++) {
		if (raidPtr->Disks[c].status == rf_ds_optimal) {
			int e = rf_sync_component_cache(raidPtr, c, force);
			if (e && !error)
				error = e;
		}
	}

	for (c = 0; c < raidPtr->numSpare ; c++) {
		int sparecol = raidPtr->numCol + c;
		/* Need to ensure that the reconstruct actually completed! */
		if (raidPtr->Disks[sparecol].status == rf_ds_used_spare) {
			int e = rf_sync_component_cache(raidPtr, sparecol,
			    force);
			if (e && !error)
				error = e;
		}
	}
	return error;
}

/* Fill in info with the current status */
void
rf_check_recon_status_ext(RF_Raid_t *raidPtr, RF_ProgressInfo_t *info)
{

	memset(info, 0, sizeof(*info));

	if (raidPtr->status != rf_rs_reconstructing) {
		info->total = 100;
		info->completed = 100;
	} else {
		info->total = raidPtr->reconControl->numRUsTotal;
		info->completed = raidPtr->reconControl->numRUsComplete;
	}
	info->remaining = info->total - info->completed;
}

/* Fill in info with the current status */
void
rf_check_parityrewrite_status_ext(RF_Raid_t *raidPtr, RF_ProgressInfo_t *info)
{

	memset(info, 0, sizeof(*info));

	if (raidPtr->parity_rewrite_in_progress == 1) {
		info->total = raidPtr->Layout.numStripe;
		info->completed = raidPtr->parity_rewrite_stripes_done;
	} else {
		info->completed = 100;
		info->total = 100;
	}
	info->remaining = info->total - info->completed;
}

/* Fill in info with the current status */
void
rf_check_copyback_status_ext(RF_Raid_t *raidPtr, RF_ProgressInfo_t *info)
{

	memset(info, 0, sizeof(*info));

	if (raidPtr->copyback_in_progress == 1) {
		info->total = raidPtr->Layout.numStripe;
		info->completed = raidPtr->copyback_stripes_done;
		info->remaining = info->total - info->completed;
	} else {
		info->remaining = 0;
		info->completed = 100;
		info->total = 100;
	}
}

/* Fill in config with the current info */
int
rf_get_info(RF_Raid_t *raidPtr, RF_DeviceConfig_t *config)
{
	int	d, i, j;

	if (!raidPtr->valid)
		return ENODEV;
	config->cols = raidPtr->numCol;
	config->ndevs = raidPtr->numCol;
	if (config->ndevs >= RF_MAX_DISKS)
		return ENOMEM;
	config->nspares = raidPtr->numSpare;
	if (config->nspares >= RF_MAX_DISKS)
		return ENOMEM;
	config->maxqdepth = raidPtr->maxQueueDepth;
	d = 0;
	for (j = 0; j < config->cols; j++) {
		config->devs[d] = raidPtr->Disks[j];
		d++;
	}
	for (j = config->cols, i = 0; i < config->nspares; i++, j++) {
		config->spares[i] = raidPtr->Disks[j];
		if (config->spares[i].status == rf_ds_rebuilding_spare) {
			/* XXX: raidctl(8) expects to see this as a used spare */
			config->spares[i].status = rf_ds_used_spare;
		}
	}
	return 0;
}

int
rf_get_component_label(RF_Raid_t *raidPtr, void *data)
{
	RF_ComponentLabel_t *clabel = (RF_ComponentLabel_t *)data;
	RF_ComponentLabel_t *raid_clabel;
	int column = clabel->column;

	if ((column < 0) || (column >= raidPtr->numCol + raidPtr->numSpare))
		return EINVAL;
	raid_clabel = raidget_component_label(raidPtr, column);
	memcpy(clabel, raid_clabel, sizeof *clabel);
	/* Fix-up for userland. */
	if (clabel->version == bswap32(RF_COMPONENT_LABEL_VERSION))
		clabel->version = RF_COMPONENT_LABEL_VERSION;

	return 0;
}

/*
 * Module interface
 */

MODULE(MODULE_CLASS_DRIVER, raid, "dk_subr,bufq_fcfs");

#ifdef _MODULE
CFDRIVER_DECL(raid, DV_DISK, NULL);
#endif

static int raid_modcmd(modcmd_t, void *);
static int raid_modcmd_init(void);
static int raid_modcmd_fini(void);

static int
raid_modcmd(modcmd_t cmd, void *data)
{
	int error;

	error = 0;
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = raid_modcmd_init();
		break;
	case MODULE_CMD_FINI:
		error = raid_modcmd_fini();
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}

static int
raid_modcmd_init(void)
{
	int error;
	int bmajor, cmajor;

	mutex_init(&raid_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_enter(&raid_lock);
#if (RF_INCLUDE_PARITY_DECLUSTERING_DS > 0)
	rf_init_mutex2(rf_sparet_wait_mutex, IPL_VM);
	rf_init_cond2(rf_sparet_wait_cv, "sparetw");
	rf_init_cond2(rf_sparet_resp_cv, "rfgst");

	rf_sparet_wait_queue = rf_sparet_resp_queue = NULL;
#endif

	bmajor = cmajor = -1;
	error = devsw_attach("raid", &raid_bdevsw, &bmajor,
	    &raid_cdevsw, &cmajor);
	if (error != 0 && error != EEXIST) {
		aprint_error("%s: devsw_attach failed %d\n", __func__, error);
		mutex_exit(&raid_lock);
		return error;
	}
#ifdef _MODULE
	error = config_cfdriver_attach(&raid_cd);
	if (error != 0) {
		aprint_error("%s: config_cfdriver_attach failed %d\n",
		    __func__, error);
		devsw_detach(&raid_bdevsw, &raid_cdevsw);
		mutex_exit(&raid_lock);
		return error;
	}
#endif
	error = config_cfattach_attach(raid_cd.cd_name, &raid_ca);
	if (error != 0) {
		aprint_error("%s: config_cfattach_attach failed %d\n",
		    __func__, error);
#ifdef _MODULE
		config_cfdriver_detach(&raid_cd);
#endif
		devsw_detach(&raid_bdevsw, &raid_cdevsw);
		mutex_exit(&raid_lock);
		return error;
	}

	raidautoconfigdone = false;

	mutex_exit(&raid_lock);

	if (error == 0) {
		if (rf_BootRaidframe(true) == 0)
			aprint_verbose("Kernelized RAIDframe activated\n");
		else
			panic("Serious error activating RAID!!");
	}

	/*
	 * Register a finalizer which will be used to auto-config RAID
	 * sets once all real hardware devices have been found.
	 */
	error = config_finalize_register(NULL, rf_autoconfig);
	if (error != 0) {
		aprint_error("WARNING: unable to register RAIDframe "
		    "finalizer\n");
		error = 0;
	}

	return error;
}

static int
raid_modcmd_fini(void)
{
	int error;

	mutex_enter(&raid_lock);

	/* Don't allow unload if raid device(s) exist.  */
	if (!LIST_EMPTY(&raids)) {
		mutex_exit(&raid_lock);
		return EBUSY;
	}

	error = config_cfattach_detach(raid_cd.cd_name, &raid_ca);
	if (error != 0) {
		aprint_error("%s: cannot detach cfattach\n",__func__);
		mutex_exit(&raid_lock);
		return error;
	}
#ifdef _MODULE
	error = config_cfdriver_detach(&raid_cd);
	if (error != 0) {
		aprint_error("%s: cannot detach cfdriver\n",__func__);
		config_cfattach_attach(raid_cd.cd_name, &raid_ca);
		mutex_exit(&raid_lock);
		return error;
	}
#endif
	error = devsw_detach(&raid_bdevsw, &raid_cdevsw);
	if (error != 0) {
		aprint_error("%s: cannot detach devsw\n",__func__);
#ifdef _MODULE
		config_cfdriver_attach(&raid_cd);
#endif
		config_cfattach_attach(raid_cd.cd_name, &raid_ca);
		mutex_exit(&raid_lock);
		return error;
	}
	rf_BootRaidframe(false);
#if (RF_INCLUDE_PARITY_DECLUSTERING_DS > 0)
	rf_destroy_mutex2(rf_sparet_wait_mutex);
	rf_destroy_cond2(rf_sparet_wait_cv);
	rf_destroy_cond2(rf_sparet_resp_cv);
#endif
	mutex_exit(&raid_lock);
	mutex_destroy(&raid_lock);

	return error;
}
