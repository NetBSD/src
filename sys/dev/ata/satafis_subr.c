/* $NetBSD: satafis_subr.c,v 1.2.2.5 2010/08/11 22:53:18 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: satafis_subr.c,v 1.2.2.5 2010/08/11 22:53:18 yamt Exp $");

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
	fis[rhd_features] = ata_c->r_features;
	fis[rhd_sector] = ata_c->r_sector;
	fis[rhd_cyl_lo] = ata_c->r_cyl & 0xff;
	fis[rhd_cyl_hi] = (ata_c->r_cyl >> 8) & 0xff;
	fis[rhd_dh] = ata_c->r_head & 0x0f;
	fis[rhd_seccnt] = ata_c->r_count;
}

void
satafis_rhd_construct_bio(struct ata_xfer *xfer, uint8_t *fis)
{
	struct ata_bio *ata_bio = xfer->c_cmd;
	int nblks;

	nblks = xfer->c_bcount / ata_bio->lp->d_secsize;

	memset(fis, 0, RHD_FISLEN);

	fis[fis_type] = RHD_FISTYPE;
	fis[rhd_c] = RHD_C;
	if (ata_bio->flags & ATA_LBA48) {
		fis[rhd_command] = (ata_bio->flags & ATA_READ) ?
		    WDCC_READDMA_EXT : WDCC_WRITEDMA_EXT;
	} else {
		fis[rhd_command] =
		    (ata_bio->flags & ATA_READ) ? WDCC_READDMA : WDCC_WRITEDMA;
	}
	fis[rhd_sector] = ata_bio->blkno & 0xff;
	fis[rhd_cyl_lo] = (ata_bio->blkno >> 8) & 0xff;
	fis[rhd_cyl_hi] = (ata_bio->blkno >> 16) & 0xff;
	if (ata_bio->flags & ATA_LBA48) {
		fis[rhd_dh] = WDSD_LBA;
		fis[rhd_sector_exp] = (ata_bio->blkno >> 24) & 0xff;
		fis[rhd_cyl_lo_exp] = (ata_bio->blkno >> 32) & 0xff;
		fis[rhd_cyl_hi_exp] = (ata_bio->blkno >> 40) & 0xff;
	} else {
		fis[rhd_dh] = ((ata_bio->blkno >> 24) & 0x0f) | WDSD_LBA;
	}
	fis[rhd_seccnt] = nblks & 0xff;
	fis[rhd_seccnt_exp] = (ata_bio->flags & ATA_LBA48) ?
	    ((nblks >> 8) & 0xff) : 0;
}

#if NATAPIBUS > 0
void
satafis_rhd_construct_atapi(struct ata_xfer *xfer, uint8_t *fis)
{
	memset(fis, 0, RHD_FISLEN);

	fis[fis_type] = RHD_FISTYPE;
	fis[rhd_c] = RHD_C;
	fis[rhd_command] = ATAPI_PKT_CMD;
	fis[rhd_features] = (xfer->c_flags & C_DMA) ?
	    ATAPI_PKT_CMD_FTRE_DMA : 0;

	return;
}
#endif /* NATAPIBUS */

void
satafis_rdh_parse(struct ata_channel *chp, const uint8_t *fis)
{
	chp->ch_status = fis[rdh_status];
	chp->ch_error = fis[rdh_error];
}

void
satafis_rdh_cmd_readreg(struct ata_command *ata_c, const uint8_t *fis)
{
	ata_c->r_command = fis[rdh_status];
	ata_c->r_features = fis[rdh_error];
	ata_c->r_error = fis[rdh_error];
	ata_c->r_sector = fis[rdh_sector];
	ata_c->r_cyl = fis[rdh_cyl_hi] << 8 | fis[rdh_cyl_lo];
	ata_c->r_head = fis[rdh_dh];
	ata_c->r_count = fis[rdh_seccnt];
}
