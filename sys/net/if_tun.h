/*
 * Copyright (c) 1988, Julian Onions.
 *
 * This source may be freely distributed, however I would be interested
 * in any changes that are made.
 *
 * from: $Header: if_tnreg.h,v 1.1.2.1 1992/07/16 22:39:16 friedl Exp
 * $Id: if_tun.h,v 1.1.2.2 1993/11/14 20:07:24 deraadt Exp $
 */

#ifndef _NET_IF_TUN_H_
#define _NET_IF_TUN_H_

struct tun_softc {
	u_short	tun_flags;		/* misc flags */
#define	TUN_OPEN	0x0001
#define	TUN_INITED	0x0002
#define	TUN_RCOLL	0x0004
#define	TUN_IASET	0x0008
#define	TUN_DSTADDR	0x0010
#define	TUN_READY	0x0020
#define	TUN_RWAIT	0x0040
#define	TUN_ASYNC	0x0080
#define	TUN_NBIO	0x0100
	struct	ifnet tun_if;		/* the interface */
	int	tun_pgrp;		/* the process group - if any */
	struct	selinfo	tun_rsel;	/* read select */
	struct	selinfo	tun_wsel;	/* write select (not used) */
};

/* Maximum packet size */
#define	TUNMTU		0x1000

/* ioctl's for get/set debug */
#define	TUNSDEBUG	_IOW('t', 90, int)
#define	TUNGDEBUG	_IOR('t', 89, int)

#endif /* !_NET_IF_TUN_H_ */
