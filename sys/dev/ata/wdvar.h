/*	$NetBSD: wdvar.h,v 1.46 2017/11/03 13:01:26 mlelstv Exp $	*/

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_ATA_WDVAR_H_
#define	_DEV_ATA_WDVAR_H_

#ifdef _KERNEL_OPT
#include "opt_wd.h"
#endif

#include <dev/dkvar.h>
#include <sys/sysctl.h>

struct wd_softc {
	/* General disk infos */
	struct dk_softc sc_dksc;
	kmutex_t sc_lock;
	int sc_quirks;			/* any quirks drive might have */

	/* IDE disk soft states */
	struct ata_drive_datas *drvp; /* Our controller's infos */
	const struct ata_bustype *atabus;
	struct ataparams sc_params;/* drive characteristics found */
	int sc_flags;
/*
 * XXX Nothing resets this yet, but disk change sensing will when ATA-4 is
 * more fully implemented.
 */
#define WDF_LOADED	0x010 /* parameters loaded */
#define WDF_WAIT	0x020 /* waiting for resources */
#define WDF_LBA		0x040 /* using LBA mode */
#define WDF_LBA48	0x100 /* using 48-bit LBA mode */
#define WDF_FLUSH_PEND	0x200 /* cache flush waits for free xfer */
#define WDF_OPEN	0x400 /* device is open */
	uint64_t sc_capacity; /* full capacity of the device */
	uint64_t sc_capacity512; /* ... in DEV_BSIZE blocks */
	uint32_t sc_capacity28; /* capacity accessible with LBA28 commands */
	uint32_t sc_blksize; /* logical block size, in bytes */

#ifdef WD_SOFTBADSECT
	SLIST_HEAD(, disk_badsectors)	sc_bslist;
	u_int sc_bscount;
#endif

	/* Sysctl nodes specific for the disk */
	struct sysctllog *nodelog;
	int drv_max_tags;
#define WD_MAX_OPENINGS(wd)	\
	(MAX(1, MIN((wd)->drvp->drv_openings, (wd)->drv_max_tags)))
	bool drv_ncq;
#define WD_USE_NCQ(wd)	\
	((wd)->drv_ncq && ((wd)->drvp->drive_flags & ATA_DRIVE_NCQ))
	bool drv_ncq_prio;
#define WD_USE_NCQ_PRIO(wd) \
	((wd)->drv_ncq_prio && ((wd)->drvp->drive_flags & ATA_DRIVE_NCQ_PRIO))
#ifdef WD_CHAOS_MONKEY
	int drv_chaos_freq;		/* frequency of simulated bio errors */
	int drv_chaos_cnt;		/* count of processed bio read xfers */
#endif
};

#endif /* _DEV_ATA_WDVAR_H_ */
