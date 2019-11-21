/*	$NetBSD: miivar.h,v 1.68.4.1 2019/11/21 14:00:49 martin Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
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

#ifndef _DEV_MII_MIIVAR_H_
#define	_DEV_MII_MIIVAR_H_

#include <sys/queue.h>
#include <sys/callout.h>

#include <dev/mii/mii.h>
#include <dev/mii/mii_verbose.h>

/*
 * Media Independent Interface data structure definitions.
 */

struct ifnet;
struct mii_softc;

/*
 * Callbacks from MII layer into network interface device driver.
 */
typedef	int (*mii_readreg_t)(device_t, int, int, uint16_t *);
typedef	int (*mii_writereg_t)(device_t, int, int, uint16_t);
typedef	void (*mii_statchg_t)(struct ifnet *);

/*
 * A network interface driver has one of these structures in its softc.
 * It is the interface from the network interface driver to the MII
 * layer.
 */
struct mii_data {
	struct ifmedia mii_media;	/* media information */
	struct ifnet *mii_ifp;		/* pointer back to network interface */

	int mii_flags;			/* misc. flags; see below */

	/*
	 * For network interfaces with multiple PHYs, a list of all
	 * PHYs is required so they can all be notified when a media
	 * request is made.
	 */
	LIST_HEAD(mii_listhead, mii_softc) mii_phys;
	u_int mii_instance;

	/* PHY driver fills this in with active media status. */
	int mii_media_status;
	u_int mii_media_active;

	/* Calls from MII layer into network interface driver. */
	mii_readreg_t mii_readreg;
	mii_writereg_t mii_writereg;
	mii_statchg_t mii_statchg;
};
typedef struct mii_data mii_data_t;

/*
 * Functions provided by the PHY to perform various functions.
 */
struct mii_phy_funcs {
	int (*pf_service)(struct mii_softc *, struct mii_data *, int);
	void (*pf_status)(struct mii_softc *);
	void (*pf_reset)(struct mii_softc *);
};

/*
 * Requests that can be made to the downcall.
 */
#define	MII_TICK	1	/* once-per-second tick */
#define	MII_MEDIACHG	2	/* user changed media; perform the switch */
#define	MII_POLLSTAT	3	/* user requested media status; fill it in */
#define	MII_DOWN	4	/* interface is down */

/*
 * Each PHY driver's softc has one of these as the first member.
 * XXX This would be better named "phy_softc", but this is the name
 * XXX BSDI used, and we would like to have the same interface.
 */
struct mii_softc {
	device_t mii_dev;		/* generic device glue */

	LIST_ENTRY(mii_softc) mii_list;	/* entry on parent's PHY list */

	uint32_t mii_mpd_oui;		/* the PHY's OUI (MII_OUI())*/
	uint32_t mii_mpd_model;		/* the PHY's model (MII_MODEL())*/
	uint32_t mii_mpd_rev;		/* the PHY's revision (MII_REV())*/
	int mii_phy;			/* our MII address */
	int mii_offset;			/* first PHY, second PHY, etc. */
	u_int mii_inst;			/* instance for ifmedia */

	/* Our PHY functions. */
	const struct mii_phy_funcs *mii_funcs;

	struct mii_data *mii_pdata;	/* pointer to parent's mii_data */

	int mii_flags;			/* misc. flags; see below */
	uint16_t mii_capabilities;	/* capabilities from BMSR */
	uint16_t mii_extcapabilities;	/* extended capabilities from EXTSR */
	int mii_ticks;			/* MII_TICK counter */
	int mii_anegticks;		/* ticks before retrying aneg */

	struct callout mii_nway_ch;	/* NWAY callout */

	u_int mii_media_active;		/* last active media */
	int mii_media_status;		/* last active status */
};
typedef struct mii_softc mii_softc_t;

/* Default mii_anegticks values. */
#define	MII_ANEGTICKS		5
#define	MII_ANEGTICKS_GIGE	10

/* mii_flags */
#define	MIIF_INITDONE	0x0001		/* has been initialized (mii_data) */
#define	MIIF_NOISOLATE	0x0002		/* do not isolate the PHY */
#define	MIIF_NOLOOP	0x0004		/* no loopback capability */
#define	MIIF_DOINGAUTO	0x0008		/* doing autonegotiation (mii_softc) */
#define MIIF_AUTOTSLEEP	0x0010		/* use tsleep(), not callout() */
#define MIIF_HAVEFIBER	0x0020		/* from parent: has fiber interface */
#define	MIIF_HAVE_GTCR	0x0040		/* has 100base-T2/1000base-T CR */
#define	MIIF_IS_1000X	0x0080		/* is a 1000BASE-X device */
#define	MIIF_DOPAUSE	0x0100		/* advertise PAUSE capability */
#define	MIIF_IS_HPNA	0x0200		/* is a HomePNA device */
#define	MIIF_FORCEANEG	0x0400		/* force auto-negotiation */

#define	MIIF_INHERIT_MASK (MIIF_NOISOLATE | MIIF_NOLOOP | MIIF_AUTOTSLEEP)

/*
 * Special `locators' passed to mii_attach().  If one of these is not
 * an `any' value, we look for *that* PHY and configure it.  If both
 * are not `any', that is an error, and mii_attach() will panic.
 */
#define	MII_OFFSET_ANY		-1
#define	MII_PHY_ANY		-1

/*
 * Used to attach a PHY to a parent.
 */
struct mii_attach_args {
	struct mii_data *mii_data;	/* pointer to parent data */
	int mii_phyno;			/* MII address */
	uint16_t mii_id1;		/* PHY ID register 1 */
	uint16_t mii_id2;		/* PHY ID register 2 */
	uint16_t mii_capmask;		/* capability mask from BMSR */
	int mii_flags;			/* flags from parent */
};
typedef struct mii_attach_args mii_attach_args_t;

/*
 * Used to match a PHY.
 */
struct mii_phydesc {
	uint32_t mpd_oui;		/* the PHY's OUI */
	uint32_t mpd_model;		/* the PHY's model */
	const char *mpd_name;		/* the PHY's name */
};

#define MII_PHY_DESC(a, b) { MII_OUI_ ## a, MII_MODEL_ ## a ## _ ## b, \
        MII_STR_ ## a ## _ ## b }
#define MII_PHY_END     { 0, 0, NULL }

/*
 * An array of these structures map MII media types to BMCR/ANAR settings.
 */
struct mii_media {
	uint16_t mm_bmcr;		/* BMCR settings for this media */
	uint16_t mm_anar;		/* ANAR settings for this media */
	uint16_t mm_gtcr;		/* 100base-T2 or 1000base-T CR */
};

#define	MII_MEDIA_NONE		0
#define	MII_MEDIA_10_T		1
#define	MII_MEDIA_10_T_FDX	2
#define	MII_MEDIA_100_T4	3
#define	MII_MEDIA_100_TX	4
#define	MII_MEDIA_100_TX_FDX	5
#define	MII_MEDIA_1000_X	6
#define	MII_MEDIA_1000_X_FDX	7
#define	MII_MEDIA_1000_T	8
#define	MII_MEDIA_1000_T_FDX	9
#define	MII_NMEDIA		10

#ifdef _KERNEL

#define	PHY_READ(p, r, v)					    \
	(*(p)->mii_pdata->mii_readreg)(device_parent((p)->mii_dev), \
	    (p)->mii_phy, (r), (v))

#define	PHY_WRITE(p, r, v) \
	(*(p)->mii_pdata->mii_writereg)(device_parent((p)->mii_dev), \
	    (p)->mii_phy, (r), (v))

/*
 * Setup MDD indirect access. Set device address and register number.
 * "addr" variable takes an address ORed with the function (MMDACR_FN_*).
 *
 */
static inline int
MMD_INDIRECT(struct mii_softc *sc, uint16_t daddr, uint16_t regnum)
{
	int rv;

	/*
	 * Set the MMD device address and set the access mode (function)
	 * to address.
	 */
	if ((rv = PHY_WRITE(sc, MII_MMDACR, (daddr & ~MMDACR_FUNCMASK))) != 0)
		return rv;

	/* Set the register number */
	if ((rv = PHY_WRITE(sc, MII_MMDAADR, regnum)) != 0)
		return rv;

	/* Set the access mode (function) */
	rv = PHY_WRITE(sc, MII_MMDACR, daddr);

	return rv;
}

static inline int
MMD_INDIRECT_READ(struct mii_softc *sc, uint16_t daddr, uint16_t regnum,
    uint16_t *valp)
{
	int rv;

	if ((rv = MMD_INDIRECT(sc, daddr, regnum)) != 0)
		return rv;

	return PHY_READ(sc, MII_MMDAADR, valp);
}

static inline int
MMD_INDIRECT_WRITE(struct mii_softc *sc, uint16_t daddr, uint16_t regnum,
    uint16_t val)
{
	int rv;

	if ((rv = MMD_INDIRECT(sc, daddr, regnum)) != 0)
		return rv;

	return PHY_WRITE(sc, MII_MMDAADR, val);
}

#define	PHY_SERVICE(p, d, o) \
	(*(p)->mii_funcs->pf_service)((p), (d), (o))

#define	PHY_STATUS(p) \
	(*(p)->mii_funcs->pf_status)((p))

#define	PHY_RESET(p) \
	(*(p)->mii_funcs->pf_reset)((p))

void	mii_attach(device_t, struct mii_data *, int, int, int, int);
void	mii_detach(struct mii_data *, int, int);
bool	mii_phy_resume(device_t, const pmf_qual_t *);

int	mii_mediachg(struct mii_data *);
void	mii_tick(struct mii_data *);
void	mii_pollstat(struct mii_data *);
void	mii_down(struct mii_data *);
uint16_t mii_anar(struct ifmedia_entry *);

int	mii_ifmedia_change(struct mii_data *);

int	mii_phy_activate(device_t, enum devact);
int	mii_phy_detach(device_t, int);

const struct mii_phydesc *mii_phy_match(const struct mii_attach_args *,
	    const struct mii_phydesc *);

void	mii_phy_add_media(struct mii_softc *);
void	mii_phy_delete_media(struct mii_softc *);

void	mii_phy_setmedia(struct mii_softc *);
int	mii_phy_auto(struct mii_softc *, int);
void	mii_phy_reset(struct mii_softc *);
void	mii_phy_down(struct mii_softc *);
int	mii_phy_tick(struct mii_softc *);

void	mii_phy_status(struct mii_softc *);
void	mii_phy_update(struct mii_softc *, int);

u_int	mii_phy_flowstatus(struct mii_softc *);

void	ukphy_status(struct mii_softc *);

u_int	mii_oui(uint16_t, uint16_t);
#define	MII_OUI(id1, id2)	mii_oui(id1, id2)
#define	MII_MODEL(id2)		(((id2) & IDR2_MODEL) >> 4)
#define	MII_REV(id2)		((id2) & IDR2_REV)

#endif /* _KERNEL */

#endif /* _DEV_MII_MIIVAR_H_ */
