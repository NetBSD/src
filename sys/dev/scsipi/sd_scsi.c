/*	$NetBSD: sd_scsi.c,v 1.36 2003/09/17 23:33:43 mycroft Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*
 * Originally written by Julian Elischer (julian@dialix.oz.au)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@dialix.oz.au) Sept 1992
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sd_scsi.c,v 1.36 2003/09/17 23:33:43 mycroft Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/dkio.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/sdvar.h>

int	sd_scsibus_match __P((struct device *, struct cfdata *, void *));
void	sd_scsibus_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(sd_scsibus, sizeof(struct sd_softc),
    sd_scsibus_match, sd_scsibus_attach, sddetach, sdactivate);

const struct scsipi_inquiry_pattern sd_scsibus_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,
	 "",         "",                 ""},
	{T_SIMPLE_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_SIMPLE_DIRECT, T_REMOV,
	 "",         "",                 ""},
};

struct sd_scsibus_mode_sense_data {
	/*
	 * XXX
	 * We are not going to parse this as-is -- it just has to be large
	 * enough.
	 */
	union {
		struct scsipi_mode_header small;
		struct scsipi_mode_header_big big;
	} header;
	struct scsi_blk_desc blk_desc;
	union scsi_disk_pages pages;
};

static int	sd_scsibus_mode_sense __P((struct sd_softc *,
		    u_int8_t, void *, size_t, int, int, int *));
static int	sd_scsibus_mode_select __P((struct sd_softc *,
		    u_int8_t, void *, size_t, int, int));
static int	sd_scsibus_get_capacity __P((struct sd_softc *,
		    struct disk_parms *, int));
static int	sd_scsibus_get_parms __P((struct sd_softc *,
		    struct disk_parms *, int));
static int	sd_scsibus_get_simplifiedparms __P((struct sd_softc *,
		    struct disk_parms *, int));
static int	sd_scsibus_flush __P((struct sd_softc *, int));
static int	sd_scsibus_getcache __P((struct sd_softc *, int *));
static int	sd_scsibus_setcache __P((struct sd_softc *, int));

const struct sd_ops sd_scsibus_ops = {
	sd_scsibus_get_parms,
	sd_scsibus_flush,
	sd_scsibus_getcache,
	sd_scsibus_setcache,
};

int
sd_scsibus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	if (scsipi_periph_bustype(sa->sa_periph) != SCSIPI_BUSTYPE_SCSI)
		return (0);

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)sd_scsibus_patterns,
	    sizeof(sd_scsibus_patterns) / sizeof(sd_scsibus_patterns[0]),
	    sizeof(sd_scsibus_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
sd_scsibus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sd_softc *sd = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	SC_DEBUG(periph, SCSIPI_DB2, ("sd_scsibus_attach: "));

	sd->type = (sa->sa_inqbuf.type & SID_TYPE);
	scsipi_strvis(sd->name, 16, sa->sa_inqbuf.product, 16);

	/*
	 * Note if this device is ancient.  This is used in sdminphys().
	 */
	if (periph->periph_version == 0)
		sd->flags |= SDF_ANCIENT;

	if (sd->type == T_SIMPLE_DIRECT)
		periph->periph_quirks |= PQUIRK_ONLYBIG | PQUIRK_NOBIGMODESENSE;

	sdattach(parent, sd, periph, &sd_scsibus_ops);
}

static int
sd_scsibus_mode_sense(sd, byte2, sense, size, page, flags, big)
	struct sd_softc *sd;
	u_int8_t byte2;
	void *sense;
	size_t size;
	int page, flags;
	int *big;
{

	if ((sd->sc_periph->periph_quirks & PQUIRK_ONLYBIG) &&
	    !(sd->sc_periph->periph_quirks & PQUIRK_NOBIGMODESENSE)) {
		*big = 1;
		return scsipi_mode_sense_big(sd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsipi_mode_header_big),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	} else {
		*big = 0;
		return scsipi_mode_sense(sd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsipi_mode_header),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	}
}

static int
sd_scsibus_mode_select(sd, byte2, sense, size, flags, big)
	struct sd_softc *sd;
	u_int8_t byte2;
	void *sense;
	size_t size;
	int flags, big;
{

	if (big) {
		struct scsipi_mode_header_big *header = sense;

		_lto2b(0, header->data_length);
		return scsipi_mode_select_big(sd->sc_periph, byte2, sense,
		    size + sizeof(struct scsipi_mode_header_big),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	} else {
		struct scsipi_mode_header *header = sense;

		header->data_length = 0;
		return scsipi_mode_select(sd->sc_periph, byte2, sense,
		    size + sizeof(struct scsipi_mode_header),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	}
}

static int
sd_scsibus_get_simplifiedparms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct {
		struct scsipi_mode_header header;
		/* no block descriptor */
		u_int8_t pg_code; /* page code (should be 6) */
		u_int8_t pg_length; /* page length (should be 11) */
		u_int8_t wcd; /* bit0: cache disable */
		u_int8_t lbs[2]; /* logical block size */
		u_int8_t size[5]; /* number of log. blocks */
		u_int8_t pp; /* power/performance */
		u_int8_t flags;
		u_int8_t resvd;
	} scsipi_sense;
	u_int64_t sectors;
	int error;

	/*
	 * scsipi_size (ie "read capacity") and mode sense page 6
	 * give the same information. Do both for now, and check
	 * for consistency.
	 * XXX probably differs for removable media
	 */
	dp->blksize = 512;
	if ((sectors = scsipi_size(sd->sc_periph, flags)) == 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	error = scsipi_mode_sense(sd->sc_periph, SMS_DBD, 6,
	    &scsipi_sense.header, sizeof(scsipi_sense),
	    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);

	if (error != 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	dp->blksize = _2btol(scsipi_sense.lbs);
	if (dp->blksize == 0) 
		dp->blksize = 512;

	/*
	 * Create a pseudo-geometry.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (dp->heads * dp->sectors);
	dp->disksize = _5btol(scsipi_sense.size);
	if (dp->disksize <= UINT32_MAX && dp->disksize != sectors) {
		printf("RBC size: mode sense=%llu, get cap=%llu\n",
		       (unsigned long long)dp->disksize,
		       (unsigned long long)sectors);
		dp->disksize = sectors;
	}
	dp->disksize512 = (dp->disksize * dp->blksize) / DEV_BSIZE;

	return (SDGP_RESULT_OK);
}

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
static int
sd_scsibus_get_capacity(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	u_int64_t sectors;
	int error;
#if 0
	int i;
	u_int8_t *p;
#endif

	dp->disksize = sectors = scsipi_size(sd->sc_periph, flags);
	if (sectors == 0) {
		struct scsipi_read_format_capacities scsipi_cmd;
		struct {
			struct scsipi_capacity_list_header header;
			struct scsipi_capacity_descriptor desc;
		} __attribute__((packed)) scsipi_result;

		memset(&scsipi_cmd, 0, sizeof(scsipi_cmd));
		memset(&scsipi_result, 0, sizeof(scsipi_result));
		scsipi_cmd.opcode = READ_FORMAT_CAPACITIES;
		_lto2b(sizeof(scsipi_result), scsipi_cmd.length);
		error = scsipi_command(sd->sc_periph, (void *)&scsipi_cmd,
		    sizeof(scsipi_cmd), (void *)&scsipi_result,
		    sizeof(scsipi_result), SDRETRIES, 20000,
		    NULL, flags | XS_CTL_DATA_IN | XS_CTL_DATA_ONSTACK /*|
		    XS_CTL_IGNORE_ILLEGAL_REQUEST*/);
		if (error || scsipi_result.header.length == 0)
			return (SDGP_RESULT_OFFLINE);

#if 0
printf("rfc: length=%d\n", scsipi_result.header.length);
printf("rfc result:"); for (i = sizeof(struct scsipi_capacity_list_header) + scsipi_result.header.length, p = (void *)&scsipi_result; i; i--, p++) printf(" %02x", *p); printf("\n");
#endif
		switch (scsipi_result.desc.byte5 & SCSIPI_CAP_DESC_CODE_MASK) {
		case SCSIPI_CAP_DESC_CODE_RESERVED:
		case SCSIPI_CAP_DESC_CODE_FORMATTED:
			break;

		case SCSIPI_CAP_DESC_CODE_UNFORMATTED:
			return (SDGP_RESULT_UNFORMATTED);

		case SCSIPI_CAP_DESC_CODE_NONE:
			return (SDGP_RESULT_OFFLINE);
		}

		dp->disksize = sectors = _4btol(scsipi_result.desc.nblks);
		if (sectors == 0)
			return (SDGP_RESULT_OFFLINE);		/* XXX? */

		dp->blksize = _3btol(scsipi_result.desc.blklen);
		if (dp->blksize == 0)
			dp->blksize = 512;
	} else {
		struct sd_scsibus_mode_sense_data scsipi_sense;
		int big, bsize;
		struct scsi_blk_desc *bdesc;

		memset(&scsipi_sense, 0, sizeof(scsipi_sense));
		error = sd_scsibus_mode_sense(sd, 0, &scsipi_sense,
		    sizeof(scsipi_sense.blk_desc), 0, flags | XS_CTL_SILENT, &big);
		if (!error) {
			if (big) {
				bdesc = (void *)(&scsipi_sense.header.big + 1);
				bsize = _2btol(scsipi_sense.header.big.blk_desc_len);
			} else {
				bdesc = (void *)(&scsipi_sense.header.small + 1);
				bsize = scsipi_sense.header.small.blk_desc_len;
			}

#if 0
printf("page 0 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 0 bsize=%d\n", bsize);
printf("page 0 ok\n");
#endif

			if (bsize >= 8) {
				dp->blksize = _3btol(bdesc->blklen);
				if (dp->blksize == 0)
					dp->blksize = 512;
			} else
				dp->blksize = 512;
		}
	}

	dp->disksize512 = (sectors * dp->blksize) / DEV_BSIZE;
	return (0);
}

static int
sd_scsibus_get_parms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct sd_scsibus_mode_sense_data scsipi_sense;
	int error;
	int big;
	union scsi_disk_pages *pages;
#if 0
	int i;
	u_int8_t *p;
#endif

	/*
	 * If offline, the SDEV_MEDIA_LOADED flag will be
	 * cleared by the caller if necessary.
	 */
	if (sd->type == T_SIMPLE_DIRECT)
		return (sd_scsibus_get_simplifiedparms(sd, dp, flags));

	error = sd_scsibus_get_capacity(sd, dp, flags);
	if (error)
		return (error);

	if (sd->type == T_OPTICAL)
		goto page0;

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_scsibus_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.blk_desc) +
	    sizeof(scsipi_sense.pages.rigid_geometry), 4,
	    flags | XS_CTL_SILENT, &big);
	if (!error) {
		if (big)
			pages = (void *)(&scsipi_sense.header.big + 1);
		else
			pages = (void *)(&scsipi_sense.header.small + 1);

#if 0
printf("page 4 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 4 pg_code=%d sense=%p/%p\n", pages->rigid_geometry.pg_code, &scsipi_sense, pages);
#endif

		if ((pages->rigid_geometry.pg_code & PGCODE_MASK) != 4)
			goto page5;
			
		SC_DEBUG(sd->sc_periph, SCSIPI_DB3,
		    ("%d cyls, %d heads, %d precomp, %d red_write, %d land_zone\n",
		    _3btol(pages->rigid_geometry.ncyl),
		    pages->rigid_geometry.nheads,
		    _2btol(pages->rigid_geometry.st_cyl_wp),
		    _2btol(pages->rigid_geometry.st_cyl_rwc),
		    _2btol(pages->rigid_geometry.land_zone)));

		/*
		 * KLUDGE!! (for zone recorded disks)
		 * give a number of sectors so that sec * trks * cyls
		 * is <= disk_size
		 * can lead to wasted space! THINK ABOUT THIS !
		 */
		dp->heads = pages->rigid_geometry.nheads;
		dp->cyls = _3btol(pages->rigid_geometry.ncyl);
		if (dp->heads == 0 || dp->cyls == 0)
			goto page5;
		dp->sectors = dp->disksize / (dp->heads * dp->cyls);	/* XXX */

		dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
		if (dp->rot_rate == 0)
			dp->rot_rate = 3600;

#if 0
printf("page 4 ok\n");
#endif
		goto blksize;
	}

page5:
	memset(&scsipi_sense, SMS_DBD, sizeof(scsipi_sense));
	error = sd_scsibus_mode_sense(sd, 0, &scsipi_sense,
	    sizeof(scsipi_sense.blk_desc) +
	    sizeof(scsipi_sense.pages.flex_geometry), 5,
	    flags | XS_CTL_SILENT, &big);
	if (!error) {
		if (big)
			pages = (void *)(&scsipi_sense.header.big + 1);
		else
			pages = (void *)(&scsipi_sense.header.small + 1);

#if 0
printf("page 5 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 5 pg_code=%d sense=%p/%p\n", pages->flex_geometry.pg_code, &scsipi_sense, pages);
#endif

		if ((pages->flex_geometry.pg_code & PGCODE_MASK) != 5)
			goto page0;
			
		SC_DEBUG(sd->sc_periph, SCSIPI_DB3,
		    ("%d cyls, %d heads, %d sec, %d bytes/sec\n",
		    _3btol(pages->flex_geometry.ncyl),
		    pages->flex_geometry.nheads,
		    pages->flex_geometry.ph_sec_tr,
		    _2btol(pages->flex_geometry.bytes_s)));

		dp->heads = pages->flex_geometry.nheads;
		dp->cyls = _2btol(pages->flex_geometry.ncyl);
		dp->sectors = pages->flex_geometry.ph_sec_tr;
		if (dp->heads == 0 || dp->cyls == 0 || dp->sectors == 0)
			goto page0;

		dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
		if (dp->rot_rate == 0)
			dp->rot_rate = 3600;

#if 0
printf("page 5 ok\n");
#endif
		goto blksize;
	}

page0:
	printf("%s: fabricating a geometry\n", sd->sc_dev.dv_xname);
	/* Try calling driver's method for figuring out geometry. */
	if (!sd->sc_periph->periph_channel->chan_adapter->adapt_getgeom ||
	    !(*sd->sc_periph->periph_channel->chan_adapter->adapt_getgeom)
		(sd->sc_periph, dp, dp->disksize)) {
		/*
		 * Use adaptec standard fictitious geometry
		 * this depends on which controller (e.g. 1542C is
		 * different. but we have to put SOMETHING here..)
		 */
		dp->heads = 64;
		dp->sectors = 32;
		dp->cyls = dp->disksize / (64 * 32);
	}
	dp->rot_rate = 3600;

blksize:
	return (SDGP_RESULT_OK);
}

static int
sd_scsibus_flush(sd, flags)
	struct sd_softc *sd;
	int flags;
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsi_synchronize_cache sync_cmd;

	/*
	 * If the device is SCSI-2, issue a SYNCHRONIZE CACHE.
	 * We issue with address 0 length 0, which should be
	 * interpreted by the device as "all remaining blocks
	 * starting at address 0".  We ignore ILLEGAL REQUEST
	 * in the event that the command is not supported by
	 * the device, and poll for completion so that we know
	 * that the cache has actually been flushed.
	 *
	 * Unless, that is, the device can't handle the SYNCHRONIZE CACHE
	 * command, as indicated by our quirks flags.
	 *
	 * XXX What about older devices?
	 */
	if (periph->periph_version >= 2 &&
	    (periph->periph_quirks & PQUIRK_NOSYNCCACHE) == 0) {
		sd->flags |= SDF_FLUSHING;
		memset(&sync_cmd, 0, sizeof(sync_cmd));
		sync_cmd.opcode = SCSI_SYNCHRONIZE_CACHE;

		return(scsipi_command(periph,
		       (struct scsipi_generic *)&sync_cmd, sizeof(sync_cmd),
		       NULL, 0, SDRETRIES, 100000, NULL,
		       flags|XS_CTL_IGNORE_ILLEGAL_REQUEST));
	} else
		return(0);
}

int
sd_scsibus_getcache(sd, bitsp)
	struct sd_softc *sd;
	int *bitsp;
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct sd_scsibus_mode_sense_data scsipi_sense;
	int error, bits = 0;
	int big;
	union scsi_disk_pages *pages;

	if (periph->periph_version < 2)
		return (EOPNOTSUPP);

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_scsibus_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params), 8, 0, &big);
	if (error)
		return (error);

	if (big)
		pages = (void *)(&scsipi_sense.header.big + 1);
	else
		pages = (void *)(&scsipi_sense.header.small + 1);

	if ((pages->caching_params.flags & CACHING_RCD) == 0)
		bits |= DKCACHE_READ;
	if (pages->caching_params.flags & CACHING_WCE)
		bits |= DKCACHE_WRITE;
	if (pages->caching_params.pg_code & PGCODE_PS)
		bits |= DKCACHE_SAVE;

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_scsibus_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params),
	    SMS_PAGE_CTRL_CHANGEABLE|8, 0, &big);
	if (error == 0) {
		if (big)
			pages = (void *)(&scsipi_sense.header.big + 1);
		else
			pages = (void *)(&scsipi_sense.header.small + 1);

		if (pages->caching_params.flags & CACHING_RCD)
			bits |= DKCACHE_RCHANGE;
		if (pages->caching_params.flags & CACHING_WCE)
			bits |= DKCACHE_WCHANGE;
	}

	*bitsp = bits;

	return (0);
}

int
sd_scsibus_setcache(sd, bits)
	struct sd_softc *sd;
	int bits;
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct sd_scsibus_mode_sense_data scsipi_sense;
	int error;
	uint8_t oflags, byte2 = 0;
	int big;
	union scsi_disk_pages *pages;

	if (periph->periph_version < 2)
		return (EOPNOTSUPP);

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_scsibus_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params), 8, 0, &big); 
	if (error)
		return (error);

	if (big)
		pages = (void *)(&scsipi_sense.header.big + 1);
	else
		pages = (void *)(&scsipi_sense.header.small + 1);

	oflags = pages->caching_params.flags;

	if (bits & DKCACHE_READ)
		pages->caching_params.flags &= ~CACHING_RCD;
	else
		pages->caching_params.flags |= CACHING_RCD;

	if (bits & DKCACHE_WRITE)
		pages->caching_params.flags |= CACHING_WCE;
	else
		pages->caching_params.flags &= ~CACHING_WCE;

	if (oflags == pages->caching_params.flags)
		return (0);

	pages->caching_params.pg_code &= PGCODE_MASK;

	if (bits & DKCACHE_SAVE)
		byte2 |= SMS_SP;

	return (sd_scsibus_mode_select(sd, byte2|SMS_PF, &scsipi_sense,
	    sizeof(struct scsipi_mode_page_header) +
	    pages->caching_params.pg_length, 0, big));
}
