/*	$NetBSD: sd.c,v 1.1.6.2 2017/12/03 11:36:38 jdolecek Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stdint.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/param.h>

#include "boot.h"
#include "sdvar.h"

#ifdef DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

#define SD_DEFAULT_BLKSIZE	512


struct sd_mode_sense_data {
	struct scsi_mode_parameter_header_6 header;
	struct scsi_general_block_descriptor blk_desc;
	union scsi_disk_pages pages;
};

static int sd_validate_blksize(int);
static uint64_t sd_read_capacity(struct sd_softc *, int *);
static int sd_get_simplifiedparms(struct sd_softc *);
static int sd_get_capacity(struct sd_softc *);
static int sd_get_parms_page4(struct sd_softc *, struct disk_parms *);
static int sd_get_parms_page5(struct sd_softc *, struct disk_parms *);
static int sd_get_parms(struct sd_softc *);
static void sdgetdefaultlabel(struct sd_softc *, struct disklabel *);
static int sdgetdisklabel(struct sd_softc *);

int sdopen(struct open_file *, ...);
int sdclose(struct open_file *);
int sdstrategy(void *, int, daddr_t, size_t, void *, size_t *);


static int
sd_validate_blksize(int len)
{

	switch (len) {
	case 256:
	case 512:
	case 1024:
	case 2048:
	case 4096:
		return 1;
	}
	return 0;
}

/*
 * sd_read_capacity:
 *
 *	Find out from the device what its capacity is.
 */
static uint64_t
sd_read_capacity(struct sd_softc *sd, int *blksize)
{
	union {
		struct scsipi_read_capacity_10 cmd;
		struct scsipi_read_capacity_16 cmd16;
	} cmd;
	union {
		struct scsipi_read_capacity_10_data data;
		struct scsipi_read_capacity_16_data data16;
	} data;
	uint64_t rv;

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd.opcode = READ_CAPACITY_10;

	/*
	 * If the command works, interpret the result as a 4 byte
	 * number of blocks
	 */
	rv = 0;
	memset(&data, 0, sizeof(data.data));
	if (scsi_command(sd, (void *)&cmd.cmd, sizeof(cmd.cmd),
	    (void *)&data, sizeof(data.data)) != 0)
		goto out;

	if (_4btol(data.data.addr) != 0xffffffff) {
		*blksize = _4btol(data.data.length);
		rv = _4btol(data.data.addr) + 1;
		goto out;
	}

	/*
	 * Device is larger than can be reflected by READ CAPACITY (10).
	 * Try READ CAPACITY (16).
	 */

	memset(&cmd, 0, sizeof(cmd));
	cmd.cmd16.opcode = READ_CAPACITY_16;
	cmd.cmd16.byte2 = SRC16_SERVICE_ACTION;
	_lto4b(sizeof(data.data16), cmd.cmd16.len);

	memset(&data, 0, sizeof(data.data16));
	if (scsi_command(sd, (void *)&cmd.cmd16, sizeof(cmd.cmd16),
	    (void *)&data, sizeof(data.data16)) != 0)
		goto out;

	*blksize = _4btol(data.data16.length);
	rv = _8btol(data.data16.addr) + 1;

 out:
	return rv;
}

static int
sd_get_simplifiedparms(struct sd_softc *sd)
{
	struct {
		struct scsi_mode_parameter_header_6 header;
		/* no block descriptor */
		uint8_t pg_code; /* page code (should be 6) */
		uint8_t pg_length; /* page length (should be 11) */
		uint8_t wcd; /* bit0: cache disable */
		uint8_t lbs[2]; /* logical block size */
		uint8_t size[5]; /* number of log. blocks */
		uint8_t pp; /* power/performance */
		uint8_t flags;
		uint8_t resvd;
	} scsipi_sense;
	struct disk_parms *dp = &sd->sc_params;
	uint64_t blocks;
	int error, blksize;

	/*
	 * sd_read_capacity (ie "read capacity") and mode sense page 6
	 * give the same information. Do both for now, and check
	 * for consistency.
	 * XXX probably differs for removable media
	 */
	dp->blksize = SD_DEFAULT_BLKSIZE;
	if ((blocks = sd_read_capacity(sd, &blksize)) == 0)
		return SDGP_RESULT_OFFLINE;		/* XXX? */

	error = scsi_mode_sense(sd, SMS_DBD, 6,
	    &scsipi_sense.header, sizeof(scsipi_sense));

	if (error != 0)
		return SDGP_RESULT_OFFLINE;		/* XXX? */

	dp->blksize = blksize;
	if (!sd_validate_blksize(dp->blksize))
		dp->blksize = _2btol(scsipi_sense.lbs);
	if (!sd_validate_blksize(dp->blksize))
		dp->blksize = SD_DEFAULT_BLKSIZE;

	/*
	 * Create a pseudo-geometry.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = blocks / (dp->heads * dp->sectors);
	dp->disksize = _5btol(scsipi_sense.size);
	if (dp->disksize <= UINT32_MAX && dp->disksize != blocks) {
		printf("RBC size: mode sense=%llu, get cap=%llu\n",
		       (unsigned long long)dp->disksize,
		       (unsigned long long)blocks);
		dp->disksize = blocks;
	}
	dp->disksize512 = (dp->disksize * dp->blksize) / DEV_BSIZE;

	return SDGP_RESULT_OK;
}

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
static int
sd_get_capacity(struct sd_softc *sd)
{
	struct disk_parms *dp = &sd->sc_params;
	uint64_t blocks;
	int error, blksize;

	dp->disksize = blocks = sd_read_capacity(sd, &blksize);
	if (blocks == 0) {
		struct scsipi_read_format_capacities cmd;
		struct {
			struct scsipi_capacity_list_header header;
			struct scsipi_capacity_descriptor desc;
		} __packed data;

		memset(&cmd, 0, sizeof(cmd));
		memset(&data, 0, sizeof(data));
		cmd.opcode = READ_FORMAT_CAPACITIES;
		_lto2b(sizeof(data), cmd.length);

		error = scsi_command(sd,
		    (void *)&cmd, sizeof(cmd), (void *)&data, sizeof(data));
		if (error == EFTYPE)
			/* Medium Format Corrupted, handle as not formatted */
			return SDGP_RESULT_UNFORMATTED;
		if (error || data.header.length == 0)
			return SDGP_RESULT_OFFLINE;

		switch (data.desc.byte5 & SCSIPI_CAP_DESC_CODE_MASK) {
		case SCSIPI_CAP_DESC_CODE_RESERVED:
		case SCSIPI_CAP_DESC_CODE_FORMATTED:
			break;

		case SCSIPI_CAP_DESC_CODE_UNFORMATTED:
			return SDGP_RESULT_UNFORMATTED;

		case SCSIPI_CAP_DESC_CODE_NONE:
			return SDGP_RESULT_OFFLINE;
		}

		dp->disksize = blocks = _4btol(data.desc.nblks);
		if (blocks == 0)
			return SDGP_RESULT_OFFLINE;		/* XXX? */

		blksize = _3btol(data.desc.blklen);

	} else if (!sd_validate_blksize(blksize)) {
		struct sd_mode_sense_data scsipi_sense;
		int bsize;

		memset(&scsipi_sense, 0, sizeof(scsipi_sense));
		error = scsi_mode_sense(sd, 0, 0, &scsipi_sense.header,
		    sizeof(struct scsi_mode_parameter_header_6) +
						sizeof(scsipi_sense.blk_desc));
		if (!error) {
			bsize = scsipi_sense.header.blk_desc_len;

			if (bsize >= 8)
				blksize = _3btol(scsipi_sense.blk_desc.blklen);
		}
	}

	if (!sd_validate_blksize(blksize))
		blksize = SD_DEFAULT_BLKSIZE;

	dp->blksize = blksize;
	dp->disksize512 = (blocks * dp->blksize) / DEV_BSIZE;
	return 0;
}

static int
sd_get_parms_page4(struct sd_softc *sd, struct disk_parms *dp)
{
	struct sd_mode_sense_data scsipi_sense;
	union scsi_disk_pages *pages;
	size_t poffset;
	int byte2, error;

	byte2 = SMS_DBD;
again:
	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = scsi_mode_sense(sd, byte2, 4, &scsipi_sense.header,
	    (byte2 ? 0 : sizeof(scsipi_sense.blk_desc)) +
				sizeof(scsipi_sense.pages.rigid_geometry));
	if (error) {
		if (byte2 == SMS_DBD) {
			/* No result; try once more with DBD off */
			byte2 = 0;
			goto again;
		}
		return error;
	}

	poffset = sizeof(scsipi_sense.header);
	poffset += scsipi_sense.header.blk_desc_len;

	if (poffset > sizeof(scsipi_sense) - sizeof(pages->rigid_geometry))
		return ERESTART;

	pages = (void *)((u_long)&scsipi_sense + poffset);
#if 0
	{
		size_t i;
		u_int8_t *p;

		printf("page 4 sense:");
		for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i;
		    i--, p++)
			printf(" %02x", *p);
		printf("\n");
		printf("page 4 pg_code=%d sense=%p/%p\n",
		    pages->rigid_geometry.pg_code, &scsipi_sense, pages);
	}
#endif

	if ((pages->rigid_geometry.pg_code & PGCODE_MASK) != 4)
		return ERESTART;

	/*
	 * KLUDGE!! (for zone recorded disks)
	 * give a number of sectors so that sec * trks * cyls
	 * is <= disk_size
	 * can lead to wasted space! THINK ABOUT THIS !
	 */
	dp->heads = pages->rigid_geometry.nheads;
	dp->cyls = _3btol(pages->rigid_geometry.ncyl);
	if (dp->heads == 0 || dp->cyls == 0)
		return ERESTART;
	dp->sectors = dp->disksize / (dp->heads * dp->cyls);	/* XXX */

	dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
	if (dp->rot_rate == 0)
		dp->rot_rate = 3600;

#if 0
printf("page 4 ok\n");
#endif
	return 0;
}

static int
sd_get_parms_page5(struct sd_softc *sd, struct disk_parms *dp)
{
	struct sd_mode_sense_data scsipi_sense;
	union scsi_disk_pages *pages;
	size_t poffset;
	int byte2, error;

	byte2 = SMS_DBD;
again:
	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = scsi_mode_sense(sd, 0, 5, &scsipi_sense.header,
	    (byte2 ? 0 : sizeof(scsipi_sense.blk_desc)) +
				sizeof(scsipi_sense.pages.flex_geometry));
	if (error) {
		if (byte2 == SMS_DBD) {
			/* No result; try once more with DBD off */
			byte2 = 0;
			goto again;
		}
		return error;
	}

	poffset = sizeof(scsipi_sense.header);
	poffset += scsipi_sense.header.blk_desc_len;

	if (poffset > sizeof(scsipi_sense) - sizeof(pages->flex_geometry))
		return ERESTART;

	pages = (void *)((u_long)&scsipi_sense + poffset);
#if 0
	{
		size_t i;
		u_int8_t *p;

		printf("page 5 sense:");
		for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i;
		    i--, p++)
			printf(" %02x", *p);
		printf("\n");
		printf("page 5 pg_code=%d sense=%p/%p\n",
		    pages->flex_geometry.pg_code, &scsipi_sense, pages);
	}
#endif

	if ((pages->flex_geometry.pg_code & PGCODE_MASK) != 5)
		return ERESTART;

	dp->heads = pages->flex_geometry.nheads;
	dp->cyls = _2btol(pages->flex_geometry.ncyl);
	dp->sectors = pages->flex_geometry.ph_sec_tr;
	if (dp->heads == 0 || dp->cyls == 0 || dp->sectors == 0)
		return ERESTART;

	dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
	if (dp->rot_rate == 0)
		dp->rot_rate = 3600;

#if 0
printf("page 5 ok\n");
#endif
	return 0;
}

static int
sd_get_parms(struct sd_softc *sd)
{
	struct disk_parms *dp = &sd->sc_params;
	int error;

	/*
	 * If offline, the SDEV_MEDIA_LOADED flag will be
	 * cleared by the caller if necessary.
	 */
	if (sd->sc_type == T_SIMPLE_DIRECT) {
		error = sd_get_simplifiedparms(sd);
		if (error)
			return error;
		goto ok;
	}

	error = sd_get_capacity(sd);
	if (error)
		return error;

	if (sd->sc_type == T_OPTICAL)
		goto page0;

	if (sd->sc_flags & FLAGS_REMOVABLE) {
		if (!sd_get_parms_page5(sd, dp) ||
		    !sd_get_parms_page4(sd, dp))
			goto ok;
	} else {
		if (!sd_get_parms_page4(sd, dp) ||
		    !sd_get_parms_page5(sd, dp))
			goto ok;
	}

page0:
	printf("fabricating a geometry\n");
	/* Try calling driver's method for figuring out geometry. */
	/*
	 * Use adaptec standard fictitious geometry
	 * this depends on which controller (e.g. 1542C is
	 * different. but we have to put SOMETHING here..)
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = dp->disksize / (64 * 32);
	dp->rot_rate = 3600;

ok:
	DPRINTF(("disksize = %" PRId64 ", disksize512 = %" PRId64 ".\n",
	    dp->disksize, dp->disksize512));

	return 0;
}

static void
sdgetdefaultlabel(struct sd_softc *sd, struct disklabel *lp)
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = sd->sc_params.blksize;
	lp->d_ntracks = sd->sc_params.heads;
	lp->d_nsectors = sd->sc_params.sectors;
	lp->d_ncylinders = sd->sc_params.cyls;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	lp->d_type = DKTYPE_SCSI;

	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = sd->sc_params.disksize;
	lp->d_rpm = sd->sc_params.rot_rate;
	lp->d_interleave = 1;
	lp->d_flags = (sd->sc_flags & FLAGS_REMOVABLE) ? D_REMOVABLE : 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = lp->d_secperunit;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Load the label information on the named device.
 */
static int
sdgetdisklabel(struct sd_softc *sd)
{
	struct mbr_sector *mbr;
	struct mbr_partition *mp;
	struct disklabel *lp = &sd->sc_label;
	size_t rsize;
	int sector, i;
	char *msg;
	uint8_t buf[DEV_BSIZE];

	sdgetdefaultlabel(sd, lp);

	if (lp->d_secpercyl == 0) {
		lp->d_secpercyl = 100;
		/* as long as it's not 0 - readdisklabel divides by it (?) */
	}

	/*
	 * Find NetBSD Partition in DOS partition table.
	 */
	sector = 0;
	if (sdstrategy(sd, F_READ, MBR_BBSECTOR, DEV_BSIZE, buf, &rsize))
		return EOFFSET;

	mbr = (struct mbr_sector *)buf;
	if (mbr->mbr_magic == htole16(MBR_MAGIC)) {
		/*
		 * Lookup NetBSD slice. If there is none, go ahead
		 * and try to read the disklabel off sector #0.
		 */
		mp = mbr->mbr_parts;
		for (i = 0; i < MBR_PART_COUNT; i++) {
			if (mp[i].mbrp_type == MBR_PTYPE_NETBSD) {
				sector = le32toh(mp[i].mbrp_start);
				break;
			}
		}
	}

	if (sdstrategy(sd, F_READ, sector + LABELSECTOR, DEV_BSIZE,
	    buf, &rsize))
		return EOFFSET;

	msg = getdisklabel((const char *)buf + LABELOFFSET, &sd->sc_label);
	if (msg)
		printf("sd(%d,%d,%d,...): getdisklabel: %s\n",
		    sd->sc_bus, sd->sc_target, sd->sc_lun, msg);

	/* check partition */
	if ((sd->sc_part >= lp->d_npartitions) ||
	    (lp->d_partitions[sd->sc_part].p_fstype == FS_UNUSED)) {
		DPRINTF(("illegal partition\n"));
		return EPART;
	}

	DPRINTF(("label info: d_secsize %d, d_nsectors %d, d_ncylinders %d,"
	    " d_ntracks %d, d_secpercyl %d\n",
	    sd->sc_label.d_secsize,
	    sd->sc_label.d_nsectors,
	    sd->sc_label.d_ncylinders,
	    sd->sc_label.d_ntracks,
	    sd->sc_label.d_secpercyl));

	return 0;
}

/*
 * Open device (read drive parameters and disklabel)
 */
int
sdopen(struct open_file *f, ...)
{
	struct sd_softc *sd;
	struct scsi_test_unit_ready cmd;
	struct scsipi_inquiry_data *inqbuf;
	u_int bus, target, lun, part;
	int error;
	char buf[SCSIPI_INQUIRY_LENGTH_SCSI2];
	va_list ap;

	va_start(ap, f);
	bus = 0;
	target = va_arg(ap, u_int);
	lun = va_arg(ap, u_int);
	part = va_arg(ap, u_int);
	va_end(ap);

	DPRINTF(("sdopen: sd(%d,%d,%d)\n", target, lun, part));

	sd = alloc(sizeof(struct sd_softc));
	if (sd == NULL)
		return ENOMEM;

	memset(sd, 0, sizeof(struct sd_softc));

	sd->sc_part = part;
	sd->sc_lun = lun;
	sd->sc_target = target;
	sd->sc_bus = bus;

	if ((error = scsi_inquire(sd, sizeof(buf), buf)) != 0)
		return error;

	inqbuf = (struct scsipi_inquiry_data *)buf;

	sd->sc_type = inqbuf->device & SID_TYPE;

	/*
	 * Determine the operating mode capabilities of the device.
	 */
	if ((inqbuf->version & SID_ANSII) >= 2) {
//		if ((inqbuf->flags3 & SID_CmdQue) != 0)
//			sd->sc_cap |= PERIPH_CAP_TQING;
		if ((inqbuf->flags3 & SID_Sync) != 0)
			sd->sc_cap |= PERIPH_CAP_SYNC;

		/* SPC-2 */
		if ((inqbuf->version & SID_ANSII) >= 3) {
			/*
			 * Report ST clocking though CAP_WIDExx/CAP_SYNC.
			 * If the device only supports DT, clear these
			 * flags (DT implies SYNC and WIDE)
			 */
			switch (inqbuf->flags4 & SID_Clocking) {
			case SID_CLOCKING_DT_ONLY:
				sd->sc_cap &= ~PERIPH_CAP_SYNC;
				break;
			}
		}
	}
	sd->sc_flags =
	    (inqbuf->dev_qual2 & SID_REMOVABLE) ? FLAGS_REMOVABLE : 0;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_TEST_UNIT_READY;
	if ((error = scsi_command(sd, (void *)&cmd, sizeof(cmd), NULL, 0)) != 0)
		return error;

	if (sd->sc_flags & FLAGS_REMOVABLE) {
		printf("XXXXX: removable device found. will not support\n");
	}
	if (!(sd->sc_flags & FLAGS_MEDIA_LOADED))
		sd->sc_flags |= FLAGS_MEDIA_LOADED;

	if ((error = sd_get_parms(sd)) != 0)
		return error;

	strncpy(sd->sc_label.d_typename, inqbuf->product, 16);
	if ((error = sdgetdisklabel(sd)) != 0)
		return error;

	f->f_devdata = sd;
	return 0;
}

/*
 * Close device.
 */
int
sdclose(struct open_file *f)
{

	return 0;
}

/*
 * Read some data.
 */
int
sdstrategy(void *f, int rw, daddr_t dblk, size_t size, void *p, size_t *rsize)
{
	struct sd_softc *sd;
	struct disklabel *lp;
	struct partition *pp;
	struct scsipi_generic *cmdp;
	struct scsipi_rw_16 cmd16;
	struct scsipi_rw_10 cmd_big;
	struct scsi_rw_6 cmd_small;
	daddr_t blkno;
	int cmdlen, nsect, i;
	uint8_t *buf;

	if (size == 0)
		return 0;
    
	if (rw != F_READ)
		return EOPNOTSUPP;

	buf = p;
	sd = f;
	lp = &sd->sc_label;
	pp = &lp->d_partitions[sd->sc_part];

	if (!(sd->sc_flags & FLAGS_MEDIA_LOADED))
		return EIO;

	nsect = howmany(size, lp->d_secsize);
	blkno = dblk + pp->p_offset;

	for (i = 0; i < nsect; i++, blkno++) {
		int error;

		/*
		 * Fill out the scsi command.  Use the smallest CDB possible
		 * (6-byte, 10-byte, or 16-byte).
		 */
		if ((blkno & 0x1fffff) == blkno) {
			/* 6-byte CDB */
			memset(&cmd_small, 0, sizeof(cmd_small));
			cmd_small.opcode = SCSI_READ_6_COMMAND;
			_lto3b(blkno, cmd_small.addr);
			cmd_small.length = 1;
			cmdlen = sizeof(cmd_small);
			cmdp = (struct scsipi_generic *)&cmd_small;
		} else if ((blkno & 0xffffffff) == blkno) {
			/* 10-byte CDB */
			memset(&cmd_big, 0, sizeof(cmd_big));
			cmd_big.opcode = READ_10;
			_lto4b(blkno, cmd_big.addr);
			_lto2b(1, cmd_big.length);
			cmdlen = sizeof(cmd_big);
			cmdp = (struct scsipi_generic *)&cmd_big;
		} else {
			/* 16-byte CDB */
			memset(&cmd16, 0, sizeof(cmd16));
			cmd16.opcode = READ_16;
			_lto8b(blkno, cmd16.addr);
			_lto4b(1, cmd16.length);
			cmdlen = sizeof(cmd16);
			cmdp = (struct scsipi_generic *)&cmd16;
		}

		error = scsi_command(sd, cmdp, cmdlen, buf, lp->d_secsize);
		if (error)
			return error;

		buf += lp->d_secsize;
	}

	*rsize = size;
	return 0;
}
