/* $NetBSD: satafis_subr.c,v 1.8 2017/10/07 16:05:32 jdolecek Exp $ */

/*-
 * Copyright (c) 2009 Jonathan A. Kollasch.
 * All rights reserved.
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

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: satafis_subr.c,v 1.8 2017/10/07 16:05:32 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/disklabel.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>

#include <dev/ata/satafisreg.h>
#include <dev/ata/satafisvar.h>

#include "atapibus.h"

void
satafis_rhd_construct_cmd(struct ata_command *ata_c, uint8_t *fis)
{

	memset(fis, 0, RHD_FISLEN);

	fis[fis_type] = RHD_FISTYPE;
	fis[rhd_c] = RHD_C;
	fis[rhd_command] = ata_c->r_command;
	fis[rhd_features0] = (ata_c->r_features >> 0) & 0xff;

	fis[rhd_lba0] = (ata_c->r_lba >> 0) & 0xff;
	fis[rhd_lba1] = (ata_c->r_lba >> 8) & 0xff;
	fis[rhd_lba2] = (ata_c->r_lba >> 16) & 0xff;
	if ((ata_c->flags & AT_LBA48) != 0) {
		fis[rhd_dh] = ata_c->r_device;
		fis[rhd_lba3] = (ata_c->r_lba >> 24) & 0xff;
		fis[rhd_lba4] = (ata_c->r_lba >> 32) & 0xff;
		fis[rhd_lba5] = (ata_c->r_lba >> 40) & 0xff;
		fis[rhd_features1] = (ata_c->r_features >> 8) & 0xff;
	} else {
		fis[rhd_dh] = (ata_c->r_device & 0xf0) |
		    ((ata_c->r_lba >> 24) & 0x0f);
	}

	fis[rhd_count0] = (ata_c->r_count >> 0) & 0xff;
	if ((ata_c->flags & AT_LBA48) != 0) {
		fis[rhd_count1] = (ata_c->r_count >> 8) & 0xff;
	}
}

void
satafis_rhd_construct_bio(struct ata_xfer *xfer, uint8_t *fis)
{
	struct ata_bio *ata_bio = &xfer->c_bio;
	struct ata_drive_datas *drvp = &xfer->c_chp->ch_drive[xfer->c_drive];
	uint16_t count, features;
	uint8_t device;

	count = xfer->c_bcount / drvp->lp->d_secsize;
	features = 0;
	device = WDSD_LBA;

	memset(fis, 0, RHD_FISLEN);

	fis[fis_type] = RHD_FISTYPE;
	fis[rhd_c] = RHD_C;
	if (ata_bio->flags & ATA_LBA48) {
		fis[rhd_command] = (ata_bio->flags & ATA_READ) ?
		    WDCC_READDMA_EXT : WDCC_WRITEDMA_EXT;

		atacmd_toncq(xfer, &fis[rhd_command], &count, &features,
		    &device);
	} else {
		fis[rhd_command] =
		    (ata_bio->flags & ATA_READ) ? WDCC_READDMA : WDCC_WRITEDMA;
	}
	fis[rhd_features0] = (features >> 0) & 0xff;

	fis[rhd_lba0] = (ata_bio->blkno >> 0) & 0xff;
	fis[rhd_lba1] = (ata_bio->blkno >> 8) & 0xff;
	fis[rhd_lba2] = (ata_bio->blkno >> 16) & 0xff;
	if ((ata_bio->flags & ATA_LBA48) != 0) {
		fis[rhd_dh] = device;
		fis[rhd_lba3] = (ata_bio->blkno >> 24) & 0xff;
		fis[rhd_lba4] = (ata_bio->blkno >> 32) & 0xff;
		fis[rhd_lba5] = (ata_bio->blkno >> 40) & 0xff;
		fis[rhd_features1] = (features >> 8) & 0xff;
	} else {
		fis[rhd_dh] = ((ata_bio->blkno >> 24) & 0x0f) |
		    (((ata_bio->flags & ATA_LBA) != 0) ? WDSD_LBA : 0);
	}

	fis[rhd_count0] = count & 0xff;
	if ((ata_bio->flags & ATA_LBA48) != 0) {
		fis[rhd_count1] = (count >> 8) & 0xff;
	}
}

#if NATAPIBUS > 0
void
satafis_rhd_construct_atapi(struct ata_xfer *xfer, uint8_t *fis)
{

	memset(fis, 0, RHD_FISLEN);

	fis[fis_type] = RHD_FISTYPE;
	fis[rhd_c] = RHD_C;
	fis[rhd_command] = ATAPI_PKT_CMD;
	fis[rhd_features0] = (xfer->c_flags & C_DMA) ?
	    ATAPI_PKT_CMD_FTRE_DMA : 0;
}
#endif /* NATAPIBUS */

int
satafis_rdh_parse(struct ata_channel *chp, const uint8_t *fis)
{

	return ATACH_ERR_ST(fis[rdh_error], fis[rdh_status]);
}

void
satafis_rdh_cmd_readreg(struct ata_command *ata_c, const uint8_t *fis)
{

	ata_c->r_lba = (uint64_t)fis[rdh_lba0] << 0;
	ata_c->r_lba |= (uint64_t)fis[rdh_lba1] << 8;
	ata_c->r_lba |= (uint64_t)fis[rdh_lba2] << 16;
	if ((ata_c->flags & AT_LBA48) != 0) {
		ata_c->r_lba |= (uint64_t)fis[rdh_lba3] << 24;
		ata_c->r_lba |= (uint64_t)fis[rdh_lba4] << 32;
		ata_c->r_lba |= (uint64_t)fis[rdh_lba5] << 40;
		ata_c->r_device = fis[rdh_dh];
	} else {
		ata_c->r_lba |= (uint64_t)(fis[rdh_dh] & 0x0f) << 24;
		ata_c->r_device = fis[rdh_dh] & 0xf0;
	}

	ata_c->r_count = fis[rdh_count0] << 0;
	if ((ata_c->flags & AT_LBA48) != 0) {
		ata_c->r_count |= fis[rdh_count1] << 8;
	}

	ata_c->r_error = fis[rdh_error];
	ata_c->r_status = fis[rdh_status];
}
