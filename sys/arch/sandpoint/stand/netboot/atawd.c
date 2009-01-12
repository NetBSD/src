/* $NetBSD: atawd.c,v 1.9 2009/01/12 09:41:58 tsutsui Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/types.h>
#include <sys/param.h>

#include <sys/disklabel.h>
#include <sys/bootblock.h>
#include <dev/raidframe/raidframevar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ata/atareg.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <machine/stdarg.h>

#include "globals.h"

#if defined(_DEBUG)
#define DPRINTF(x)	printf x;
#else
#define DPRINTF(x)
#endif

struct atacdv {
	int (*match)(unsigned, void *);
	void *(*init)(unsigned, void *);
	unsigned chvalid;
	void *priv;
};

#define ATAC_DECL(xxx) \
    int xxx ## _match(unsigned, void *); \
    void * xxx ## _init(unsigned, void *)

ATAC_DECL(pciide);

static struct atacdv vatacdv[] = {
	{ pciide_match, pciide_init, 01 },
};
static int natacdv = sizeof(vatacdv)/sizeof(vatacdv[0]);
struct atacdv *atac;

void *disk[4];
int ndisk;

static int wd_get_params(struct wd_softc *);
static int wdgetdisklabel(struct wd_softc *);

int atac_init(unsigned);
int atac_probe(void *);
static int atac_wait_for_ready(struct atac_channel *); 
static int atac_exec_identify(struct wd_softc *, void *);
static int atac_read_block(struct wd_softc *, struct atac_command *);
static int atacommand(struct wd_softc *, struct atac_command *);
static int atac_exec_read(struct wd_softc *, int, daddr_t, void *);

#define WDC_TIMEOUT 2000000

int
wdopen(struct open_file *f, ...)
{
	va_list ap;
	int unit, part;
	struct wd_softc *wd;
	struct disklabel *lp;
	struct partition *pp;

	va_start(ap, f);
	unit = va_arg(ap, u_int);
	part = va_arg(ap, u_int);
	va_end(ap);

	if (unit >= ndisk)
		return ENXIO;
	wd = disk[unit];
	lp = &wd->sc_label;
	if (part >= lp->d_npartitions)
		return ENXIO;
	pp = &lp->d_partitions[part];
	if (pp->p_size == 0 || pp->p_fstype == FS_UNUSED)
		return ENXIO;
	wd->sc_part = part;
	f->f_devdata = wd;
	return 0;
}

int	
wdclose(struct open_file *f)
{

	f->f_devdata = NULL;
	return 0;
}	

int	    
wdstrategy(void *f, int rw, daddr_t dblk, size_t size, void *p, size_t *rsize)
{
	int i, nsect;
	daddr_t blkno;
	struct wd_softc *wd;
	struct partition *pp;
	uint8_t *buf;

	if (size == 0)
		return 0;

	if (rw != F_READ)
		return EOPNOTSUPP;

	buf = p;
	wd = f;
	pp = &wd->sc_label.d_partitions[wd->sc_part];

	nsect = howmany(size, wd->sc_label.d_secsize);
	blkno = dblk + pp->p_offset;
	if (pp->p_fstype == FS_RAID)
		blkno += RF_PROTECTED_SECTORS;

	for (i = 0; i < nsect; i++, blkno++) {
		int error;

		if ((error = atac_exec_read(wd, WDCC_READ, blkno, buf)) != 0)
			return error;

		buf += wd->sc_label.d_secsize;
	}

	*rsize = size;
	return 0;
}

int
parsefstype(void *data)
{
	struct wd_softc *wd = data;
	struct disklabel *lp = &wd->sc_label;
	struct partition *pp = &lp->d_partitions[wd->sc_part];

	return pp->p_fstype;
}

int
atac_init(unsigned tag)
{
	struct atacdv *dv;
	int n;

	for (n = 0; n < natacdv; n++) {
		dv = &vatacdv[n];
		if ((*dv->match)(tag, NULL) > 0)
			goto found;
	}
	return 0;
  found:
	atac = dv;
	atac->priv = (*dv->init)(tag, (void *)dv->chvalid);
	return 1;
}

int
atac_probe(void *atac)
{
	struct atac_softc *l = atac;
	struct wd_softc *wd;
	int i, error, chvalid;

	i = 0; error = 0;
	chvalid = l->chvalid;
	for (i = 0; chvalid != 0; i += 1) {
		if (chvalid & (01 << i)) {
			chvalid &= ~(01 << i);
#if 0
			error = diskprobe(atac);
			if (error != 0)
				continue;
#endif
			wd = alloc(sizeof(struct wd_softc));
			memset(wd, 0, sizeof(struct wd_softc));
			wd->sc_unit = ndisk;
			wd->sc_channel = &l->channel[i];
			disk[ndisk] = (void *)wd;
			error = wd_get_params(wd);
			if (error != 0)
				continue;
			error = wdgetdisklabel(wd);
			if (error != 0)
				continue;
			ndisk += 1;
		}
	}
	return error;
}

static int
wd_get_params(struct wd_softc *wd)
{
	int error;
	uint8_t *buf = wd->sc_buf;
	
	if ((error = atac_exec_identify(wd, buf)) != 0)
		return error;
	    
	wd->sc_params = *(struct ataparams *)buf;

	/* 48-bit LBA addressing */
	if ((wd->sc_params.atap_cmd2_en & ATA_CMD2_LBA48) != 0) {
		DPRINTF(("Drive supports LBA48.\n"));
		wd->sc_flags |= WDF_LBA48;
	}
   
	/* Prior to ATA-4, LBA was optional. */
	if ((wd->sc_params.atap_capabilities1 & WDC_CAP_LBA) != 0) {
		DPRINTF(("Drive supports LBA.\n"));
		wd->sc_flags |= WDF_LBA;
	}

	return 0;
}

static int
wdgetdisklabel(struct wd_softc *wd)
{
	char *msg;
	int sector, i, n;
	size_t rsize;
	struct mbr_partition *dp, *bsdp;
	struct disklabel *lp;
	uint8_t *buf = wd->sc_buf;
	
	lp = &wd->sc_label;
	memset(lp, 0, sizeof(struct disklabel));

	sector = 0;
	if (wdstrategy(wd, F_READ, MBR_BBSECTOR, DEV_BSIZE, buf, &rsize))
		return EOFFSET;

	dp = (struct mbr_partition *)(buf + MBR_PART_OFFSET);
	bsdp = NULL;
	for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
		if (dp->mbrp_type == MBR_PTYPE_NETBSD) {
			bsdp = dp;
			break;
		}
	}
	if (!bsdp) {
		/* generate fake disklabel */
		lp->d_secsize = DEV_BSIZE;
		lp->d_ntracks = wd->sc_params.atap_heads;
		lp->d_nsectors = wd->sc_params.atap_sectors;
		lp->d_ncylinders = wd->sc_params.atap_cylinders;
		lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
		if (strcmp((char *)wd->sc_params.atap_model, "ST506") == 0)
			lp->d_type = DTYPE_ST506;
		else
			lp->d_type = DTYPE_ESDI;
		strncpy(lp->d_typename, (char *)wd->sc_params.atap_model, 16);
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
		lp->d_magic = DISKMAGIC;
		lp->d_magic2 = DISKMAGIC;
		lp->d_checksum = dkcksum(lp);

		dp = (struct mbr_partition *)(buf + MBR_PART_OFFSET);
		n = 'e' - 'a';
		for (i = 0; i < MBR_PART_COUNT; i++, dp++) {
			if (dp->mbrp_type == MBR_PTYPE_UNUSED)
				continue;
			lp->d_partitions[n].p_offset = bswap32(dp->mbrp_start);
			lp->d_partitions[n].p_size = bswap32(dp->mbrp_size);
			switch (dp->mbrp_type) {
			case MBR_PTYPE_FAT12:
			case MBR_PTYPE_FAT16S:
			case MBR_PTYPE_FAT16B:
			case MBR_PTYPE_FAT32:
			case MBR_PTYPE_FAT32L:
			case MBR_PTYPE_FAT16L:
				lp->d_partitions[n].p_fstype = FS_MSDOS;
				break;
			case MBR_PTYPE_LNXEXT2:
				lp->d_partitions[n].p_fstype = FS_EX2FS;
				break;
			default:
				lp->d_partitions[n].p_fstype = FS_OTHER;
				break;
			}
			n += 1;
		}
		lp->d_npartitions = n;
	}
	else {
		sector = bswap32(bsdp->mbrp_start);
		if (wdstrategy(wd, F_READ, sector + LABELSECTOR, DEV_BSIZE,
		    buf, &rsize))
			return EOFFSET;
		msg = getdisklabel((char *)buf + LABELOFFSET, &wd->sc_label);
		if (msg != NULL)
			printf("wd%d: getdisklabel: %s\n", wd->sc_unit, msg);
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

static int
atac_wait_for_ready(struct atac_channel *chp)
{
	u_int timeout;
	
	for (timeout = WDC_TIMEOUT; timeout > 0; --timeout) {
		if ((WDC_READ_CMD(chp, wd_status) & (WDCS_BSY | WDCS_DRDY))
				== WDCS_DRDY)
			return 0;
	}	
	return ENXIO;
}

static int
atac_read_block(struct wd_softc *wd, struct atac_command *cmd)
{
	int i;
	struct atac_channel *chp = wd->sc_channel;
	uint16_t *ptr = (uint16_t *)cmd->data;
 
	if (ptr == NULL)
		return 0;
	for (i = cmd->bcount; i > 0; i -= sizeof(uint16_t))
		*ptr++ = WDC_READ_DATA(chp);
	return 0;
}

static int
atacommand(struct wd_softc *wd, struct atac_command *cmd)
{
	struct atac_channel *chp = wd->sc_channel;

	if (wd->sc_flags & WDF_LBA48) {
		WDC_WRITE_CMD(chp, wd_sdh, (cmd->drive << 4) | WDSD_LBA);

		/* previous */
		WDC_WRITE_CMD(chp, wd_features, 0);
		WDC_WRITE_CMD(chp, wd_seccnt, cmd->r_count >> 8);
		WDC_WRITE_CMD(chp, wd_lba_hi, cmd->r_blkno >> 40);
		WDC_WRITE_CMD(chp, wd_lba_mi, cmd->r_blkno >> 32);
		WDC_WRITE_CMD(chp, wd_lba_lo, cmd->r_blkno >> 24);

		/* current */
		WDC_WRITE_CMD(chp, wd_features, 0);
		WDC_WRITE_CMD(chp, wd_seccnt, cmd->r_count);
		WDC_WRITE_CMD(chp, wd_lba_hi, cmd->r_blkno >> 16);
		WDC_WRITE_CMD(chp, wd_lba_mi, cmd->r_blkno >> 8);
		WDC_WRITE_CMD(chp, wd_lba_lo, cmd->r_blkno);

		/* Send command. */
		WDC_WRITE_CMD(chp, wd_command, cmd->r_command);

		if (atac_wait_for_ready(chp) != 0)
			return ENXIO;

		if (WDC_READ_CMD(chp, wd_status) & WDCS_ERR) {
			printf("wd%d: error %x\n", chp->compatchan,
			    WDC_READ_CMD(chp, wd_error));
			return ENXIO;
		}
	}
	else {
		WDC_WRITE_CMD(chp, wd_precomp, cmd->r_precomp);
		WDC_WRITE_CMD(chp, wd_seccnt, cmd->r_count);
		WDC_WRITE_CMD(chp, wd_sector, cmd->r_sector);
		WDC_WRITE_CMD(chp, wd_cyl_lo, cmd->r_cyl);
		WDC_WRITE_CMD(chp, wd_cyl_hi, cmd->r_cyl >> 8);
		WDC_WRITE_CMD(chp, wd_sdh,
		    WDSD_IBM | (cmd->drive << 4) | cmd->r_head);
		WDC_WRITE_CMD(chp, wd_command, cmd->r_command);

	}
	if (atac_wait_for_ready(chp) != 0)
		return ENXIO;

	if (WDC_READ_CMD(chp, wd_status) & WDCS_ERR) {
		printf("wd%d: error %x\n", chp->compatchan,
		    WDC_READ_CMD(chp, wd_error));
		return ENXIO;
	}
	return 0;
}

static int
atac_exec_identify(struct wd_softc *wd, void *data)
{
	int error;
	struct atac_command *cmd;

	cmd = &wd->sc_command;
	memset(cmd, 0, sizeof(struct atac_command));

	cmd->r_command = WDCC_IDENTIFY;
	cmd->r_count = 1;
	cmd->data = data;
	cmd->drive = wd->sc_unit;
	cmd->bcount = wd->sc_label.d_secsize;

	if ((error = atacommand(wd, cmd)) != 0)
		return error;
	return atac_read_block(wd, cmd);
}

static int
atac_exec_read(struct wd_softc *wd, int exe, daddr_t blkno, void *data)
{
	int error;
	struct atac_command *cmd;

	cmd = &wd->sc_command;
	memset(cmd, 0, sizeof(struct atac_command));

	if (wd->sc_flags & WDF_LBA48)
		cmd->r_blkno = blkno;
	else if (wd->sc_flags & WDF_LBA) {
		cmd->r_sector = blkno & 0xff;
		cmd->r_cyl = (blkno >> 8) & 0xffff;
		cmd->r_head = (blkno >> 24) & 0x0f;
		cmd->r_head |= WDSD_LBA;
	}
	else {
		cmd->r_sector = 1 + (blkno % wd->sc_label.d_nsectors);
		blkno /= wd->sc_label.d_nsectors;
		cmd->r_head = blkno % wd->sc_label.d_ntracks;
		blkno /= wd->sc_label.d_ntracks;
		cmd->r_cyl = blkno;
		cmd->r_head |= WDSD_CHS;
	}
	cmd->r_command = exe;
	cmd->r_count = 1;
	cmd->data = data;
	cmd->drive = wd->sc_unit;
	cmd->bcount = wd->sc_label.d_secsize;

	if ((error = atacommand(wd, cmd)) != 0)
		return error;
	return atac_read_block(wd, cmd);
}
