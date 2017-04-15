/*	$NetBSD: wdvar.h,v 1.43.4.2 2017/04/15 12:01:23 jdolecek Exp $	*/

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
#include "opt_wd_softbadsect.h"
#endif

#include <sys/rndsource.h>

struct wd_softc {
	/* General disk infos */
	device_t sc_dev;
	struct disk sc_dk;
	struct bufq_state *sc_q;
	struct callout sc_restart_ch;
	kmutex_t sc_lock;
	int sc_quirks;			/* any quirks drive might have */

	/* IDE disk soft states */
	struct ata_drive_datas *drvp; /* Our controller's infos */
	const struct ata_bustype *atabus;
	struct ataparams sc_params;/* drive characteristics found */
	int sc_flags;
#define	WDF_WLABEL	0x004 /* label is writable */
#define	WDF_LABELLING	0x008 /* writing label */
/*
 * XXX Nothing resets this yet, but disk change sensing will when ATA-4 is
 * more fully implemented.
 */
#define WDF_LOADED	0x010 /* parameters loaded */
#define WDF_WAIT	0x020 /* waiting for resources */
#define WDF_LBA		0x040 /* using LBA mode */
#define WDF_KLABEL	0x080 /* retain label after 'full' close */
#define WDF_LBA48	0x100 /* using 48-bit LBA mode */
	uint64_t sc_capacity; /* full capacity of the device */
	uint64_t sc_capacity512; /* ... in DEV_BSIZE blocks */
	uint32_t sc_capacity28; /* capacity accessible with LBA28 commands */
	uint32_t sc_blksize; /* logical block size, in bytes */

#ifdef WD_SOFTBADSECT
	SLIST_HEAD(, disk_badsectors)	sc_bslist;
	u_int sc_bscount;
#endif
	krndsource_t	rnd_source;

	LIST_HEAD(, wd_ioctl) wi_head;

	STAILQ_HEAD(, ata_xfer) xfer_restart;
};

#endif /* _DEV_ATA_WDVAR_H_ */
