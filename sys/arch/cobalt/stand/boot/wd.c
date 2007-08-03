/*	$NetBSD: wd.c,v 1.7 2007/08/03 13:15:57 tsutsui Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/stdint.h>

#include <lib/libsa/stand.h>

#include <machine/param.h>
#include <machine/stdarg.h>
#include <dev/raidframe/raidframevar.h>		/* For RF_PROTECTED_SECTORS */

#include "boot.h"
#include "wdvar.h"

static int  wd_get_params(struct wd_softc *wd);
static int  wdgetdisklabel(struct wd_softc *wd);
static void wdgetdefaultlabel(struct wd_softc *wd, struct disklabel *lp);

/*
 * Get drive parameters through 'device identify' command.
 */
int
wd_get_params(struct wd_softc *wd)
{
	int error;
	uint8_t buf[DEV_BSIZE];

	if ((error = wdc_exec_identify(wd, buf)) != 0)
		return error;

	wd->sc_params = *(struct ataparams *)buf;

	/* 48-bit LBA addressing */
	if ((wd->sc_params.atap_cmd2_en & ATA_CMD2_LBA48) != 0) {
		DPRINTF(("Drive supports LBA48.\n"));
#if defined(_ENABLE_LBA48)
		wd->sc_flags |= WDF_LBA48;
#endif
	}

	/* Prior to ATA-4, LBA was optional. */
	if ((wd->sc_params.atap_capabilities1 & WDC_CAP_LBA) != 0) {
		DPRINTF(("Drive supports LBA.\n"));
		wd->sc_flags |= WDF_LBA;
	}

	return 0;
}

/*
 * Initialize disk label to the default value.
 */
void
wdgetdefaultlabel(struct wd_softc *wd, struct disklabel *lp)
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = DEV_BSIZE;
	lp->d_ntracks = wd->sc_params.atap_heads;
	lp->d_nsectors = wd->sc_params.atap_sectors;
	lp->d_ncylinders = wd->sc_params.atap_cylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	if (strcmp(wd->sc_params.atap_model, "ST506") == 0)
		lp->d_type = DTYPE_ST506;
	else
		lp->d_type = DTYPE_ESDI;

	strncpy(lp->d_typename, wd->sc_params.atap_model, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	if (wd->sc_capacity > UINT32_MAX)
		lp->d_secperunit = UINT32_MAX;
	else
		lp->d_secperunit = wd->sc_capacity;
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = MAXPARTITIONS;	/* RAW_PART + 1 ??? */

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Read disk label from the device.
 */
int
wdgetdisklabel(struct wd_softc *wd)
{
	char *msg;
	int sector;
	size_t rsize;
	struct disklabel *lp;
	uint8_t buf[DEV_BSIZE];

	wdgetdefaultlabel(wd, &wd->sc_label);

	/*
	 * Find NetBSD Partition in DOS partition table.
	 */
	sector = 0;
	if (wdstrategy(wd, F_READ, MBR_BBSECTOR, DEV_BSIZE, buf, &rsize))
		return EOFFSET;

	if (*(uint16_t *)&buf[MBR_MAGIC_OFFSET] == MBR_MAGIC) {
		int i;
		struct mbr_partition *mp;

		/*
		 * Lookup NetBSD slice. If there is none, go ahead
		 * and try to read the disklabel off sector #0.
		 */
		mp = (struct mbr_partition *)&buf[MBR_PART_OFFSET];
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (mp[i].mbrp_type == MBR_PTYPE_NETBSD) {
				sector = mp[i].mbrp_start;
				break;
			}
		}
	}

	if (wdstrategy(wd, F_READ, sector + LABELSECTOR, DEV_BSIZE,
	    buf, &rsize))
		return EOFFSET;

	if ((msg = getdisklabel(buf + LABELOFFSET, &wd->sc_label)))
		printf("wd%d: getdisklabel: %s\n", wd->sc_unit, msg);

	lp = &wd->sc_label;

	/* check partition */
	if ((wd->sc_part >= lp->d_npartitions) ||
	    (lp->d_partitions[wd->sc_part].p_fstype == FS_UNUSED)) {
		DPRINTF(("illegal partition\n"));
		return EPART;
	}

	DPRINTF(("label info: d_secsize %d, d_nsectors %d, d_ncylinders %d,"
	    "d_ntracks %d, d_secpercyl %d\n",
	    wd->sc_label.d_secsize,
	    wd->sc_label.d_nsectors,
	    wd->sc_label.d_ncylinders,
	    wd->sc_label.d_ntracks,
	    wd->sc_label.d_secpercyl));

	return 0;
}

/*
 * Open device (read drive parameters and disklabel)
 */
int
wdopen(struct open_file *f, ...)
{
	int error;
	va_list ap;
	u_int unit, part;
	struct wd_softc *wd;

	va_start(ap, f);
	unit = va_arg(ap, u_int);
	part = va_arg(ap, u_int);
	va_end(ap);

	DPRINTF(("wdopen: %d:%d\n", unit, part));

	wd = alloc(sizeof(struct wd_softc));
	if (wd == NULL)
		return ENOMEM;

	memset(wd, 0, sizeof(struct wd_softc));

	if (wdc_init(wd, &unit) != 0)
		return (ENXIO);

	wd->sc_part = part;
	wd->sc_unit = unit;

	if ((error = wd_get_params(wd)) != 0)
		return error;

	if ((error = wdgetdisklabel(wd)) != 0)
		return error;

	f->f_devdata = wd;
	return 0;
}

/*
 * Close device.
 */
int
wdclose(struct open_file *f)
{

	return 0;
}

/*
 * Read some data.
 */
int
wdstrategy(void *f, int rw, daddr_t dblk, size_t size, void *buf, size_t *rsize)
{
	int i, nsect;
	daddr_t blkno;
	struct wd_softc *wd;
	struct partition *pp;

	if (size == 0)
		return 0;
    
	if (rw != F_READ)
		return EOPNOTSUPP;

	wd = f;
	pp = &wd->sc_label.d_partitions[wd->sc_part];

	nsect = howmany(size, wd->sc_label.d_secsize);
	blkno = dblk + pp->p_offset;
	if (pp->p_fstype == FS_RAID)
		blkno += RF_PROTECTED_SECTORS;

	for (i = 0; i < nsect; i++, blkno++) {
		int error;

		if ((error = wdc_exec_read(wd, WDCC_READ, blkno, buf)) != 0)
			return error;

		buf += wd->sc_label.d_secsize;
	}

	*rsize = size;
	return 0;
}
