/* $NetBSD: newfs_udf.c,v 1.8 2009/09/17 10:37:28 reinoud Exp $ */

/*
 * Copyright (c) 2006, 2008 Reinoud Zandijk
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
 * 
 */

/*
 * TODO
 * - implement metadata formatting for BD-R
 * - implement support for a read-only companion partition?
 */

#define _EXPOSE_MMC
#if 0
# define DEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>
#include <time.h>
#include <assert.h>
#include <err.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/cdio.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/param.h>
#include <sys/queue.h>

#include <fs/udf/ecma167-udf.h>
#include <fs/udf/udf_mount.h>

#include "mountprog.h"
#include "udf_create.h"

/* general settings */
#define UDF_512_TRACK	0	/* NOT recommended */
#define UDF_META_PERC  20	/* picked */


/* prototypes */
int newfs_udf(int argc, char **argv);
static void usage(void) __attribute__((__noreturn__));

int udf_derive_format(int req_en, int req_dis, int force);
int udf_proces_names(void);
int udf_do_newfs(void);

/* Identifying myself */
#define APP_NAME		"*NetBSD newfs"
#define APP_VERSION_MAIN	0
#define APP_VERSION_SUB		3
#define IMPL_NAME		"*NetBSD userland UDF"


/* global variables describing disc and format requests */
int	 fd;				/* device: file descriptor */
char	*dev;				/* device: name		   */
struct mmc_discinfo mmc_discinfo;	/* device: disc info	   */

char	*format_str;			/* format: string representation */
int	 format_flags;			/* format: attribute flags	 */
int	 media_accesstype;		/* derived from current mmc cap  */
int	 check_surface;			/* for rewritables               */

int	 wrtrack_skew;
int	 meta_perc = UDF_META_PERC;
float	 meta_fract = (float) UDF_META_PERC / 100.0;


/* shared structure between udf_create.c users */
struct udf_create_context context;
struct udf_disclayout     layout;


/* queue for temporary storage of sectors to be written out */
struct wrsect {
	uint32_t  sectornr;
	uint8_t	 *sector_data;
	TAILQ_ENTRY(wrsect) next;
};

/* write queue and track blocking skew */
TAILQ_HEAD(wrsect_list, wrsect) write_queue;


/* --------------------------------------------------------------------- */

/*
 * write queue implementation
 */

static int
udf_write_sector(void *sector, uint32_t location)
{
	struct wrsect *pos, *seekpos;


	/* search location */
	TAILQ_FOREACH_REVERSE(seekpos, &write_queue, wrsect_list, next) {
		if (seekpos->sectornr <= location)
			break;
	}
	if ((seekpos == NULL) || (seekpos->sectornr != location)) {
		pos = calloc(1, sizeof(struct wrsect));
		if (pos == NULL)
			return ENOMEM;
		/* allocate space for copy of sector data */
		pos->sector_data = calloc(1, context.sector_size);
		if (pos->sector_data == NULL)
			return ENOMEM;
		pos->sectornr = location;

		if (seekpos) {
			TAILQ_INSERT_AFTER(&write_queue, seekpos, pos, next);
		} else {
			TAILQ_INSERT_HEAD(&write_queue, pos, next);
		}	
	} else {
		pos = seekpos;
	}
	memcpy(pos->sector_data, sector, context.sector_size);

	return 0;
}


/*
 * Now all write requests are queued in the TAILQ, write them out to the
 * disc/file image. Special care needs to be taken for devices that are only
 * strict overwritable i.e. only in packet size chunks
 *
 * XXX support for growing vnd?
 */

static int
writeout_write_queue(void)
{
	struct wrsect *pos;
	uint64_t offset;
	uint32_t line_len, line_offset;
	uint32_t line_start, new_line_start, relpos;
	uint32_t blockingnr;
	uint8_t *linebuf, *adr;

	blockingnr  = layout.blockingnr;
	line_len    = blockingnr   * context.sector_size;
	line_offset = wrtrack_skew * context.sector_size;

	linebuf     = malloc(line_len);
	if (linebuf == NULL)
		return ENOMEM;

	pos = TAILQ_FIRST(&write_queue);
	bzero(linebuf, line_len);

	/*
	 * Always writing out in whole lines now; this is slightly wastefull
	 * on logical overwrite volumes but it reduces complexity and the loss
	 * is near zero compared to disc size.
	 */
	line_start = (pos->sectornr - wrtrack_skew) / blockingnr;
	TAILQ_FOREACH(pos, &write_queue, next) {
		new_line_start = (pos->sectornr - wrtrack_skew) / blockingnr;
		if (new_line_start != line_start) {
			/* write out */
			offset = (uint64_t) line_start * line_len + line_offset;
#ifdef DEBUG
			printf("WRITEOUT %08"PRIu64" + %02d -- "
				"[%08"PRIu64"..%08"PRIu64"]\n",
				offset / context.sector_size, blockingnr,
				offset / context.sector_size,
				offset / context.sector_size + blockingnr-1);
#endif
			if (pwrite(fd, linebuf, line_len, offset) < 0) {
				perror("Writing failed");
				return errno;
			}
			line_start = new_line_start;
			bzero(linebuf, line_len);
		}

		relpos = (pos->sectornr - wrtrack_skew) % blockingnr;
		adr = linebuf + relpos * context.sector_size;
		memcpy(adr, pos->sector_data, context.sector_size);
	}
	/* writeout last chunk */
	offset = (uint64_t) line_start * line_len + line_offset;
#ifdef DEBUG
	printf("WRITEOUT %08"PRIu64" + %02d -- [%08"PRIu64"..%08"PRIu64"]\n",
		offset / context.sector_size, blockingnr,
		offset / context.sector_size,
		offset / context.sector_size + blockingnr-1);
#endif
	if (pwrite(fd, linebuf, line_len, offset) < 0) {
		perror("Writing failed");
		return errno;
	}

	/* success */
	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * mmc_discinfo and mmc_trackinfo readers modified from origional in udf main
 * code in sys/fs/udf/
 */

#ifdef DEBUG
static void
udf_dump_discinfo(struct mmc_discinfo *di)
{
	char bits[128];

	printf("Device/media info  :\n");
	printf("\tMMC profile        0x%02x\n", di->mmc_profile);
	printf("\tderived class      %d\n", di->mmc_class);
	printf("\tsector size        %d\n", di->sector_size);
	printf("\tdisc state         %d\n", di->disc_state);
	printf("\tlast ses state     %d\n", di->last_session_state);
	printf("\tbg format state    %d\n", di->bg_format_state);
	printf("\tfrst track         %d\n", di->first_track);
	printf("\tfst on last ses    %d\n", di->first_track_last_session);
	printf("\tlst on last ses    %d\n", di->last_track_last_session);
	printf("\tlink block penalty %d\n", di->link_block_penalty);
	snprintb(bits, sizeof(bits), MMC_DFLAGS_FLAGBITS, (uint64_t) di->disc_flags);
	printf("\tdisc flags         %s\n", bits);
	printf("\tdisc id            %x\n", di->disc_id);
	printf("\tdisc barcode       %"PRIx64"\n", di->disc_barcode);

	printf("\tnum sessions       %d\n", di->num_sessions);
	printf("\tnum tracks         %d\n", di->num_tracks);

	snprintb(bits, sizeof(bits), MMC_CAP_FLAGBITS, di->mmc_cur);
	printf("\tcapabilities cur   %s\n", bits);
	snprintb(bits, sizeof(bits), MMC_CAP_FLAGBITS, di->mmc_cap);
	printf("\tcapabilities cap   %s\n", bits);
	printf("\n");
	printf("\tlast_possible_lba  %d\n", di->last_possible_lba);
	printf("\n");
}
#else
#define udf_dump_discinfo(a);
#endif

/* --------------------------------------------------------------------- */

static int
udf_update_discinfo(struct mmc_discinfo *di)
{
	struct disklabel  disklab;
	struct partition *dp;
	struct stat st;
	int partnr, error;

	memset(di, 0, sizeof(struct mmc_discinfo));

	/* check if we're on a MMC capable device, i.e. CD/DVD */
	error = ioctl(fd, MMCGETDISCINFO, di);
	if (error == 0)
		return 0;

	/*
	 * disc partition support; note we can't use DIOCGPART in userland so
	 * get disc label and use the stat info to get the partition number.
	 */
	if (ioctl(fd, DIOCGDINFO, &disklab) == -1) {
		/* failed to get disclabel! */
		perror("disklabel");
		return errno;
	}

	/* get disk partition it refers to */
	fstat(fd, &st);
	partnr = DISKPART(st.st_rdev);
	dp = &disklab.d_partitions[partnr];

	/* set up a disc info profile for partitions */
	di->mmc_profile		= 0x01;	/* disc type */
	di->mmc_class		= MMC_CLASS_DISC;
	di->disc_state		= MMC_STATE_CLOSED;
	di->last_session_state	= MMC_STATE_CLOSED;
	di->bg_format_state	= MMC_BGFSTATE_COMPLETED;
	di->link_block_penalty	= 0;

	di->mmc_cur     = MMC_CAP_RECORDABLE | MMC_CAP_REWRITABLE |
		MMC_CAP_ZEROLINKBLK | MMC_CAP_HW_DEFECTFREE;
	di->mmc_cap    = di->mmc_cur;
	di->disc_flags = MMC_DFLAGS_UNRESTRICTED;

	/* TODO problem with last_possible_lba on resizable VND; request */
	if (dp->p_size == 0) {
		perror("faulty disklabel partition returned, check label\n");
		return EIO;
	}
	di->last_possible_lba = dp->p_size - 1;
	di->sector_size       = disklab.d_secsize;

	di->num_sessions = 1;
	di->num_tracks   = 1;

	di->first_track  = 1;
	di->first_track_last_session = di->last_track_last_session = 1;

	return 0;
}


static int
udf_update_trackinfo(struct mmc_discinfo *di, struct mmc_trackinfo *ti)
{
	int error, class;

	class = di->mmc_class;
	if (class != MMC_CLASS_DISC) {
		/* tracknr specified in struct ti */
		error = ioctl(fd, MMCGETTRACKINFO, ti);
		return error;
	}

	/* discs partition support */
	if (ti->tracknr != 1)
		return EIO;

	/* create fake ti (TODO check for resized vnds) */
	ti->sessionnr  = 1;

	ti->track_mode = 0;	/* XXX */
	ti->data_mode  = 0;	/* XXX */
	ti->flags = MMC_TRACKINFO_LRA_VALID | MMC_TRACKINFO_NWA_VALID;

	ti->track_start    = 0;
	ti->packet_size    = 1;

	/* TODO support for resizable vnd */
	ti->track_size    = di->last_possible_lba;
	ti->next_writable = di->last_possible_lba;
	ti->last_recorded = ti->next_writable;
	ti->free_blocks   = 0;

	return 0;
}


static int
udf_setup_writeparams(struct mmc_discinfo *di)
{
	struct mmc_writeparams mmc_writeparams;
	int error;

	if (di->mmc_class == MMC_CLASS_DISC)
		return 0;

	/*
	 * only CD burning normally needs setting up, but other disc types
	 * might need other settings to be made. The MMC framework will set up
	 * the nessisary recording parameters according to the disc
	 * characteristics read in. Modifications can be made in the discinfo
	 * structure passed to change the nature of the disc.
	 */
	memset(&mmc_writeparams, 0, sizeof(struct mmc_writeparams));
	mmc_writeparams.mmc_class  = di->mmc_class;
	mmc_writeparams.mmc_cur    = di->mmc_cur;

	/*
	 * UDF dictates first track to determine track mode for the whole
	 * disc. [UDF 1.50/6.10.1.1, UDF 1.50/6.10.2.1]
	 * To prevent problems with a `reserved' track in front we start with
	 * the 2nd track and if that is not valid, go for the 1st.
	 */
	mmc_writeparams.tracknr = 2;
	mmc_writeparams.data_mode  = MMC_DATAMODE_DEFAULT;	/* XA disc */
	mmc_writeparams.track_mode = MMC_TRACKMODE_DEFAULT;	/* data */

	error = ioctl(fd, MMCSETUPWRITEPARAMS, &mmc_writeparams);
	if (error) {
		mmc_writeparams.tracknr = 1;
		error = ioctl(fd, MMCSETUPWRITEPARAMS, &mmc_writeparams);
	}
	return error;
}


static void
udf_synchronise_caches(void)
{
	struct mmc_op mmc_op;

	bzero(&mmc_op, sizeof(struct mmc_op));
	mmc_op.operation = MMC_OP_SYNCHRONISECACHE;

	/* this device might not know this ioct, so just be ignorant */
	(void) ioctl(fd, MMCOP, &mmc_op);
}

/* --------------------------------------------------------------------- */

static int
udf_write_dscr_phys(union dscrptr *dscr, uint32_t location,
	uint32_t sects)
{
	uint32_t phys, cnt;
	uint8_t *bpos;
	int error;

	dscr->tag.tag_loc = udf_rw32(location);
	(void) udf_validate_tag_and_crc_sums(dscr);

	for (cnt = 0; cnt < sects; cnt++) {
		bpos  = (uint8_t *) dscr;
		bpos += context.sector_size * cnt;

		phys = location + cnt;
		error = udf_write_sector(bpos, phys);
		if (error)
			return error;
	}
	return 0;
}


static int
udf_write_dscr_virt(union dscrptr *dscr, uint32_t location, uint32_t vpart,
	uint32_t sects)
{
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct extattrhdr_desc *extattrhdr;
	uint32_t phys, cnt;
	uint8_t *bpos;
	int error;

	extattrhdr = NULL;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe = (struct file_entry *) dscr;
		if (udf_rw32(fe->l_ea) > 0)
			extattrhdr = (struct extattrhdr_desc *) fe->data;
	}
	if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe = (struct extfile_entry *) dscr;
		if (udf_rw32(efe->l_ea) > 0)
			extattrhdr = (struct extattrhdr_desc *) efe->data;
	}
	if (extattrhdr) {
		extattrhdr->tag.tag_loc = udf_rw32(location);
		udf_validate_tag_and_crc_sums((union dscrptr *) extattrhdr);
	}

	dscr->tag.tag_loc = udf_rw32(location);
	udf_validate_tag_and_crc_sums(dscr);

	for (cnt = 0; cnt < sects; cnt++) {
		bpos  = (uint8_t *) dscr;
		bpos += context.sector_size * cnt;

		/* NOTE linear mapping assumed in the ranges used */
		phys = context.vtop_offset[vpart] + location + cnt;

		error = udf_write_sector(bpos, phys);
		if (error)
			return error;
	}
	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * udf_derive_format derives the format_flags from the disc's mmc_discinfo.
 * The resulting flags uniquely define a disc format. Note there are at least
 * 7 distinct format types defined in UDF.
 */

#define UDF_VERSION(a) \
	(((a) == 0x100) || ((a) == 0x102) || ((a) == 0x150) || ((a) == 0x200) || \
	 ((a) == 0x201) || ((a) == 0x250) || ((a) == 0x260))

int
udf_derive_format(int req_enable, int req_disable, int force)
{
	/* disc writability, formatted, appendable */
	if ((mmc_discinfo.mmc_cur & MMC_CAP_RECORDABLE) == 0) {
		(void)printf("Can't newfs readonly device\n");
		return EROFS;
	}
	if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) {
		/* sequentials need sessions appended */
		if (mmc_discinfo.disc_state == MMC_STATE_CLOSED) {
			(void)printf("Can't append session to a closed disc\n");
			return EROFS;
		}
		if ((mmc_discinfo.disc_state != MMC_STATE_EMPTY) && !force) {
			(void)printf("Disc not empty! Use -F to force "
			    "initialisation\n");
			return EROFS;
		}
	} else {
		/* check if disc (being) formatted or has been started on */
		if (mmc_discinfo.disc_state == MMC_STATE_EMPTY) {
			(void)printf("Disc is not formatted\n");
			return EROFS;
		}
	}

	/* determine UDF format */
	format_flags = 0;
	if (mmc_discinfo.mmc_cur & MMC_CAP_REWRITABLE) {
		/* all rewritable media */
		format_flags |= FORMAT_REWRITABLE;
		if (context.min_udf >= 0x0250) {
			/* standard dictates meta as default */
			format_flags |= FORMAT_META;
		}

		if ((mmc_discinfo.mmc_cur & MMC_CAP_HW_DEFECTFREE) == 0) {
			/* sparables for defect management */
			if (context.min_udf >= 0x150)
				format_flags |= FORMAT_SPARABLE;
		}
	} else {
		/* all once recordable media */
		format_flags |= FORMAT_WRITEONCE;
		if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) {
			format_flags |= FORMAT_SEQUENTIAL;

			if (mmc_discinfo.mmc_cur & MMC_CAP_PSEUDOOVERWRITE) {
				/* logical overwritable */
				format_flags |= FORMAT_LOW;
			} else {
				/* have to use VAT for overwriting */
				format_flags |= FORMAT_VAT;
			}
		} else {
			/* rare WORM devices, but BluRay has one, strat4096 */
			format_flags |= FORMAT_WORM;
		}
	}

	/* enable/disable requests */
	if (req_disable & FORMAT_META) {
		format_flags &= ~FORMAT_META;
		req_disable  &= ~FORMAT_META;
	}
	if (req_disable || req_enable) {
		(void)printf("Internal error\n");
		(void)printf("\tunrecognised enable/disable req.\n");
		return EIO;
	}
	if ((format_flags && FORMAT_VAT) && UDF_512_TRACK)
		format_flags |= FORMAT_TRACK512;

	/* determine partition/media access type */
	media_accesstype = UDF_ACCESSTYPE_NOT_SPECIFIED;
	if (mmc_discinfo.mmc_cur & MMC_CAP_REWRITABLE) {
		media_accesstype = UDF_ACCESSTYPE_OVERWRITABLE;
		if (mmc_discinfo.mmc_cur & MMC_CAP_ERASABLE)
			media_accesstype = UDF_ACCESSTYPE_REWRITEABLE;
	} else {
		/* all once recordable media */
		media_accesstype = UDF_ACCESSTYPE_WRITE_ONCE;
	}
	if (mmc_discinfo.mmc_cur & MMC_CAP_PSEUDOOVERWRITE)
		media_accesstype = UDF_ACCESSTYPE_PSEUDO_OVERWITE;

	/* adjust minimum version limits */
	if (format_flags & FORMAT_VAT)
		context.min_udf = MAX(context.min_udf, 0x0150);
	if (format_flags & FORMAT_SPARABLE)
		context.min_udf = MAX(context.min_udf, 0x0150);
	if (format_flags & FORMAT_META)
		context.min_udf = MAX(context.min_udf, 0x0250);
	if (format_flags & FORMAT_LOW)
		context.min_udf = MAX(context.min_udf, 0x0260);

	/* adjust maximum version limits not to tease or break things */
	if (!(format_flags & FORMAT_META) && (context.max_udf > 0x200))
		context.max_udf = 0x201;

	if ((format_flags & (FORMAT_VAT | FORMAT_SPARABLE)) == 0)
		if (context.max_udf <= 0x150)
			context.min_udf = 0x102;

	/* limit Ecma 167 descriptor if possible/needed */
	context.dscrver = 3;
	if ((context.min_udf < 0x200) || (context.max_udf < 0x200)) {
		context.dscrver = 2;
		context.max_udf = 0x150;	/* last version < 0x200 */
	}

	/* is it possible ? */
	if (context.min_udf > context.max_udf) {
		(void)printf("Initialisation prohibited by specified maximum "
		    "UDF version 0x%04x. Minimum version required 0x%04x\n",
		    context.max_udf, context.min_udf);
		return EPERM;
	}

	if (!UDF_VERSION(context.min_udf) || !UDF_VERSION(context.max_udf)) {
		printf("Choose UDF version numbers from "
			"0x102, 0x150, 0x200, 0x201, 0x250 and 0x260\n");
		printf("Default version is 0x201\n");
		return EPERM;
	}

	return 0;
}

#undef UDF_VERSION


/* --------------------------------------------------------------------- */

int
udf_proces_names(void)
{
	uint32_t primary_nr;
	uint64_t volset_nr;

	if (context.logvol_name == NULL)
		context.logvol_name = strdup("anonymous");
	if (context.primary_name == NULL) {
		if (mmc_discinfo.disc_flags & MMC_DFLAGS_DISCIDVALID) {
			primary_nr = mmc_discinfo.disc_id;
		} else {
			primary_nr = (uint32_t) random();
		}
		context.primary_name = calloc(32, 1);
		sprintf(context.primary_name, "%08"PRIx32, primary_nr);
	}
	if (context.volset_name == NULL) {
		if (mmc_discinfo.disc_flags & MMC_DFLAGS_BARCODEVALID) {
			volset_nr = mmc_discinfo.disc_barcode;
		} else {
			volset_nr  =  (uint32_t) random();
			volset_nr |= ((uint64_t) random()) << 32;
		}
		context.volset_name = calloc(128,1);
		sprintf(context.volset_name, "%016"PRIx64, volset_nr);
	}
	if (context.fileset_name == NULL)
		context.fileset_name = strdup("anonymous");

	/* check passed/created identifiers */
	if (strlen(context.logvol_name)  > 128) {
		(void)printf("Logical volume name too long\n");
		return EINVAL;
	}
	if (strlen(context.primary_name) >  32) {
		(void)printf("Primary volume name too long\n");
		return EINVAL;
	}
	if (strlen(context.volset_name)  > 128) {
		(void)printf("Volume set name too long\n");
		return EINVAL;
	}
	if (strlen(context.fileset_name) > 32) {
		(void)printf("Fileset name too long\n");
		return EINVAL;
	}

	/* signal all OK */
	return 0;
}

/* --------------------------------------------------------------------- */

static int
udf_prepare_disc(void)
{
	struct mmc_trackinfo ti;
	struct mmc_op        op;
	int tracknr, error;

	/* If the last track is damaged, repair it */
	ti.tracknr = mmc_discinfo.last_track_last_session;
	error = udf_update_trackinfo(&mmc_discinfo, &ti);
	if (error)
		return error;

	if (ti.flags & MMC_TRACKINFO_DAMAGED) {
		/*
		 * Need to repair last track before anything can be done.
		 * this is an optional command, so ignore its error but report
		 * warning.
		 */
		memset(&op, 0, sizeof(op));
		op.operation   = MMC_OP_REPAIRTRACK;
		op.mmc_profile = mmc_discinfo.mmc_profile;
		op.tracknr     = ti.tracknr;
		error = ioctl(fd, MMCOP, &op);

		if (error)
			(void)printf("Drive can't explicitly repair last "
				"damaged track, but it might autorepair\n");
	}
	/* last track (if any) might not be damaged now, operations are ok now */

	/* setup write parameters from discinfo */
	error = udf_setup_writeparams(&mmc_discinfo);
	if (error)
		return error;

	/* if the drive is not sequential, we're done */
	if ((mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) == 0)
		return 0;

#ifdef notyet
	/* if last track is not the reserved but an empty track, unreserve it */
	if (ti.flags & MMC_TRACKINFO_BLANK) {
		if (ti.flags & MMC_TRACKINFO_RESERVED == 0) {
			memset(&op, 0, sizeof(op));
			op.operation   = MMC_OP_UNRESERVETRACK;
			op.mmc_profile = mmc_discinfo.mmc_profile;
			op.tracknr     = ti.tracknr;
			error = ioctl(fd, MMCOP, &op);
			if (error)
				return error;

			/* update discinfo since it changed by the operation */
			error = udf_update_discinfo(&mmc_discinfo);
			if (error)
				return error;
		}
	}
#endif

	/* close the last session if its still open */
	if (mmc_discinfo.last_session_state == MMC_STATE_INCOMPLETE) {
		printf("Closing last open session if present\n");
		/* close all associated tracks */
		tracknr = mmc_discinfo.first_track_last_session;
		while (tracknr <= mmc_discinfo.last_track_last_session) {
			ti.tracknr = tracknr;
			error = udf_update_trackinfo(&mmc_discinfo, &ti);
			if (error)
				return error;
			printf("\tClosing open track %d\n", tracknr);
			memset(&op, 0, sizeof(op));
			op.operation   = MMC_OP_CLOSETRACK;
			op.mmc_profile = mmc_discinfo.mmc_profile;
			op.tracknr     = tracknr;
			error = ioctl(fd, MMCOP, &op);
			if (error)
				return error;
			tracknr ++;
		}
		printf("Closing session\n");
		memset(&op, 0, sizeof(op));
		op.operation   = MMC_OP_CLOSESESSION;
		op.mmc_profile = mmc_discinfo.mmc_profile;
		op.sessionnr   = mmc_discinfo.num_sessions;
		error = ioctl(fd, MMCOP, &op);
		if (error)
			return error;

		/* update discinfo since it changed by the operations */
		error = udf_update_discinfo(&mmc_discinfo);
		if (error)
			return error;
	}

	if (format_flags & FORMAT_TRACK512) {
		/* get last track again */
		ti.tracknr = mmc_discinfo.last_track_last_session;
		error = udf_update_trackinfo(&mmc_discinfo, &ti);
		if (error)
			return error;

		/* Split up the space at 512 for iso cd9660 hooking */
		memset(&op, 0, sizeof(op));
		op.operation   = MMC_OP_RESERVETRACK_NWA;	/* UPTO nwa */
		op.mmc_profile = mmc_discinfo.mmc_profile;
		op.extent      = 512;				/* size */
		error = ioctl(fd, MMCOP, &op);
		if (error)
			return error;
	}

	return 0;
}

/* --------------------------------------------------------------------- */

static int
udf_surface_check(void)
{
	uint32_t loc, block_bytes;
	uint32_t sector_size, blockingnr, bpos;
	uint8_t *buffer;
	int error, num_errors;

	sector_size = context.sector_size;
	blockingnr  = layout.blockingnr;

	block_bytes = layout.blockingnr * sector_size;
	if ((buffer = malloc(block_bytes)) == NULL)
		return ENOMEM;

	/* set all one to not kill Flash memory? */
	for (bpos = 0; bpos < block_bytes; bpos++)
		buffer[bpos] = 0x00;

	printf("\nChecking disc surface : phase 1 - writing\n");
	num_errors = 0;
	loc = layout.first_lba;
	while (loc <= layout.last_lba) {
		/* write blockingnr sectors */
		error = pwrite(fd, buffer, block_bytes, loc*sector_size);
		printf("   %08d + %d (%02d %%)\r", loc, blockingnr,
			(int)((100.0 * loc)/layout.last_lba));
		fflush(stdout);
		if (error == -1) {
			/* block is bad */
			printf("BAD block at %08d + %d         \n",
				loc, layout.blockingnr);
			if ((error = udf_register_bad_block(loc)))
				return error;
			num_errors ++;
		}
		loc += layout.blockingnr;
	}

	printf("\nChecking disc surface : phase 2 - reading\n");
	num_errors = 0;
	loc = layout.first_lba;
	while (loc <= layout.last_lba) {
		/* read blockingnr sectors */
		error = pread(fd, buffer, block_bytes, loc*sector_size);
		printf("   %08d + %d (%02d %%)\r", loc, blockingnr,
			(int)((100.0 * loc)/layout.last_lba));
		fflush(stdout);
		if (error == -1) {
			/* block is bad */
			printf("BAD block at %08d + %d         \n",
				loc, layout.blockingnr);
			if ((error = udf_register_bad_block(loc)))
				return error;
			num_errors ++;
		}
		loc += layout.blockingnr;
	}
	printf("Scan complete : %d bad blocks found\n", num_errors);
	free(buffer);

	return 0;
}

/* --------------------------------------------------------------------- */

static int
udf_write_iso9660_vrs(void)
{
	struct vrs_desc *iso9660_vrs_desc;
	uint32_t pos;
	int error, cnt, dpos;

	/* create ISO/Ecma-167 identification descriptors */
	if ((iso9660_vrs_desc = calloc(1, context.sector_size)) == NULL)
		return ENOMEM;

	/*
	 * All UDF formats should have their ISO/Ecma-167 descriptors written
	 * except when not possible due to track reservation in the case of
	 * VAT
	 */
	if ((format_flags & FORMAT_TRACK512) == 0) {
		dpos = (2048 + context.sector_size - 1) / context.sector_size;

		/* wipe at least 6 times 2048 byte `sectors' */
		for (cnt = 0; cnt < 6 *dpos; cnt++) {
			pos = layout.iso9660_vrs + cnt;
			if ((error = udf_write_sector(iso9660_vrs_desc, pos)))
				return error;
		}

		/* common VRS fields in all written out ISO descriptors */
		iso9660_vrs_desc->struct_type = 0;
		iso9660_vrs_desc->version     = 1;
		pos = layout.iso9660_vrs;

		/* BEA01, NSR[23], TEA01 */
		memcpy(iso9660_vrs_desc->identifier, "BEA01", 5);
		if ((error = udf_write_sector(iso9660_vrs_desc, pos)))
			return error;
		pos += dpos;

		if (context.dscrver == 2)
			memcpy(iso9660_vrs_desc->identifier, "NSR02", 5);
		else
			memcpy(iso9660_vrs_desc->identifier, "NSR03", 5);
		;
		if ((error = udf_write_sector(iso9660_vrs_desc, pos)))
			return error;
		pos += dpos;

		memcpy(iso9660_vrs_desc->identifier, "TEA01", 5);
		if ((error = udf_write_sector(iso9660_vrs_desc, pos)))
			return error;
	}

	/* return success */
	return 0;
}


/* --------------------------------------------------------------------- */

/*
 * Main function that creates and writes out disc contents based on the
 * format_flags's that uniquely define the type of disc to create.
 */

int
udf_do_newfs(void)
{
	union dscrptr *zero_dscr;
	union dscrptr *terminator_dscr;
	union dscrptr *root_dscr;
	union dscrptr *vat_dscr;
	union dscrptr *dscr;
	struct mmc_trackinfo ti;
	uint32_t sparable_blocks;
	uint32_t sector_size, blockingnr;
	uint32_t cnt, loc, len;
	int sectcopy;
	int error, integrity_type;
	int data_part, metadata_part;

	/* init */
	sector_size = mmc_discinfo.sector_size;

	/* determine span/size */
	ti.tracknr = mmc_discinfo.first_track_last_session;
	error = udf_update_trackinfo(&mmc_discinfo, &ti);
	if (error)
		return error;

	if (mmc_discinfo.sector_size < context.sector_size) {
		fprintf(stderr, "Impossible to format: sectorsize too small\n");
		return EIO;
	}
	context.sector_size = sector_size;

	/* determine blockingnr */
	blockingnr = ti.packet_size;
	if (blockingnr <= 1) {
		/* paranoia on blockingnr */
		switch (mmc_discinfo.mmc_profile) {
		case 0x09 : /* CD-R    */
		case 0x0a : /* CD-RW   */
			blockingnr = 32;	/* UDF requirement */
			break;
		case 0x11 : /* DVD-R (DL) */
		case 0x1b : /* DVD+R      */
		case 0x2b : /* DVD+R Dual layer */
		case 0x13 : /* DVD-RW restricted overwrite */
		case 0x14 : /* DVD-RW sequential */
			blockingnr = 16;	/* SCSI definition */
			break;
		case 0x41 : /* BD-R Sequential recording (SRM) */
		case 0x51 : /* HD DVD-R   */
			blockingnr = 32;	/* SCSI definition */
			break;
		default:
			break;
		}

	}
	if (blockingnr <= 0) {
		printf("Can't fixup blockingnumber for device "
			"type %d\n", mmc_discinfo.mmc_profile);

		printf("Device is not returning valid blocking"
			" number and media type is unknown.\n");

		return EINVAL;
	}

	/* setup sector writeout queue's */
	TAILQ_INIT(&write_queue);
	wrtrack_skew = ti.track_start % blockingnr;

	if (mmc_discinfo.mmc_class == MMC_CLASS_CD) {
		/* not too much for CD-RW, still 20MiB */
		sparable_blocks = 32;
	} else {
		/* take a value for DVD*RW mainly, BD is `defect free' */
		sparable_blocks = 512;
	}

	/* get layout */
	error = udf_calculate_disc_layout(format_flags, context.min_udf,
		wrtrack_skew,
		ti.track_start, mmc_discinfo.last_possible_lba,
		sector_size, blockingnr, sparable_blocks,
		meta_fract);

	/* cache partition for we need it often */
	data_part     = context.data_part;
	metadata_part = context.metadata_part;

	/* Create sparing table descriptor if applicable */
	if (format_flags & FORMAT_SPARABLE) {
		if ((error = udf_create_sparing_tabled()))
			return error;

		if (check_surface) {
			if ((error = udf_surface_check()))
				return error;
		}
	}

	/* Create a generic terminator descriptor */
	terminator_dscr = calloc(1, sector_size);
	if (terminator_dscr == NULL)
		return ENOMEM;
	udf_create_terminator(terminator_dscr, 0);

	/*
	 * Start with wipeout of VRS1 upto start of partition. This allows
	 * formatting for sequentials with the track reservation and it 
	 * cleans old rubbish on rewritables. For sequentuals without the
	 * track reservation all is wiped from track start.
	 */
	if ((zero_dscr = calloc(1, context.sector_size)) == NULL)
		return ENOMEM;

	loc = (format_flags & FORMAT_TRACK512) ? layout.vds1 : ti.track_start;
	for (; loc < layout.part_start_lba; loc++) {
		if ((error = udf_write_sector(zero_dscr, loc)))
			return error;
	}

	/* Create anchors */
	for (cnt = 0; cnt < 3; cnt++) {
		if ((error = udf_create_anchor(cnt)))
			return error;
	}

	/* 
	 * Create the two Volume Descriptor Sets (VDS) each containing the
	 * following descriptors : primary volume, partition space,
	 * unallocated space, logical volume, implementation use and the
	 * terminator
	 */

	/* start of volume recognision sequence building */
	context.vds_seq = 0;

	/* Create primary volume descriptor */
	if ((error = udf_create_primaryd()))
		return error;

	/* Create partition descriptor */
	if ((error = udf_create_partitiond(context.data_part, media_accesstype)))
		return error;

	/* Create unallocated space descriptor */
	if ((error = udf_create_unalloc_spaced()))
		return error;

	/* Create logical volume descriptor */
	if ((error = udf_create_logical_dscr(format_flags)))
		return error;

	/* Create implementation use descriptor */
	/* TODO input of fields 1,2,3 and passing them */
	if ((error = udf_create_impvold(NULL, NULL, NULL)))
		return error;

	/* write out what we've created so far */

	/* writeout iso9660 vrs */
	if ((error = udf_write_iso9660_vrs()))
		return error;

	/* Writeout anchors */
	for (cnt = 0; cnt < 3; cnt++) {
		dscr = (union dscrptr *) context.anchors[cnt];
		loc  = layout.anchors[cnt];
		if ((error = udf_write_dscr_phys(dscr, loc, 1)))
			return error;

		/* sequential media has only one anchor */
		if (format_flags & FORMAT_SEQUENTIAL)
			break;
	}

	/* write out main and secondary VRS */
	for (sectcopy = 1; sectcopy <= 2; sectcopy++) {
		loc = (sectcopy == 1) ? layout.vds1 : layout.vds2;

		/* primary volume descriptor */
		dscr = (union dscrptr *) context.primary_vol;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* partition descriptor(s) */
		for (cnt = 0; cnt < UDF_PARTITIONS; cnt++) {
			dscr = (union dscrptr *) context.partitions[cnt];
			if (dscr) {
				error = udf_write_dscr_phys(dscr, loc, 1);
				if (error)
					return error;
				loc++;
			}
		}

		/* unallocated space descriptor */
		dscr = (union dscrptr *) context.unallocated;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* logical volume descriptor */
		dscr = (union dscrptr *) context.logical_vol;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* implementation use descriptor */
		dscr = (union dscrptr *) context.implementation;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;

		/* terminator descriptor */
		error = udf_write_dscr_phys(terminator_dscr, loc, 1);
		if (error)
			return error;
		loc++;
	}

	/* writeout the two sparable table descriptors (if needed) */
	if (format_flags & FORMAT_SPARABLE) {
		for (sectcopy = 1; sectcopy <= 2; sectcopy++) {
			loc  = (sectcopy == 1) ? layout.spt_1 : layout.spt_2;
			dscr = (union dscrptr *) context.sparing_table;
			len  = layout.sparing_table_dscr_lbas;

			/* writeout */
			error = udf_write_dscr_phys(dscr, loc, len);
			if (error)
				return error;
		}
	}

	/*
	 * Create unallocated space bitmap descriptor. Sequential recorded
	 * media report their own free/used space; no free/used space tables
	 * should be recorded for these.
	 */
	if ((format_flags & FORMAT_SEQUENTIAL) == 0) {
		error = udf_create_space_bitmap(
				layout.alloc_bitmap_dscr_size,
				layout.part_size_lba,
				&context.part_unalloc_bits[data_part]);
		if (error)
			return error;
		/* TODO: freed space bitmap if applicable */

		/* mark space allocated for the unallocated space bitmap */
		udf_mark_allocated(layout.unalloc_space, data_part,
			layout.alloc_bitmap_dscr_size);
	}

	/*
	 * Create metadata partition file entries and allocate and init their
	 * space and free space maps.
	 */
	if (format_flags & FORMAT_META) {
		error = udf_create_space_bitmap(
				layout.meta_bitmap_dscr_size,
				layout.meta_part_size_lba,
				&context.part_unalloc_bits[metadata_part]);
		if (error)
			return error;
	
		error = udf_create_meta_files();
		if (error)
			return error;

		/* mark space allocated for meta partition and its bitmap */
		udf_mark_allocated(layout.meta_file,   data_part, 1);
		udf_mark_allocated(layout.meta_mirror, data_part, 1);
		udf_mark_allocated(layout.meta_bitmap, data_part, 1);
		udf_mark_allocated(layout.meta_part_start_lba, data_part,
			layout.meta_part_size_lba);

		/* mark space allocated for the unallocated space bitmap */
		udf_mark_allocated(layout.meta_bitmap_space, data_part,
			layout.meta_bitmap_dscr_size);
	}

	/* create logical volume integrity descriptor */
	context.num_files = 0;
	context.num_directories = 0;
	integrity_type = UDF_INTEGRITY_OPEN;
	if ((error = udf_create_lvintd(integrity_type)))
		return error;

	/* create FSD */
	if ((error = udf_create_fsd()))
		return error;
	udf_mark_allocated(layout.fsd, metadata_part, 1);

	/* create root directory */
	assert(context.unique_id == 0x10);
	context.unique_id = 0;
	if ((error = udf_create_new_rootdir(&root_dscr)))
		return error;
	udf_mark_allocated(layout.rootdir, metadata_part, 1);

	/* writeout FSD + rootdir */
	dscr = (union dscrptr *) context.fileset_desc;
	error = udf_write_dscr_virt(dscr, layout.fsd, metadata_part, 1);
	if (error)
		return error;

	error = udf_write_dscr_virt(root_dscr, layout.rootdir, metadata_part, 1);
	if (error)
		return error;

	/* writeout initial open integrity sequence + terminator */
	loc = layout.lvis;
	dscr = (union dscrptr *) context.logvol_integrity;
	error = udf_write_dscr_phys(dscr, loc, 1);
	if (error)
		return error;
	loc++;
	error = udf_write_dscr_phys(terminator_dscr, loc, 1);
	if (error)
		return error;


	/* XXX the place to add more files */


	if ((format_flags & FORMAT_SEQUENTIAL) == 0) {
		/* update lvint and mark it closed */
		udf_update_lvintd(UDF_INTEGRITY_CLOSED);

		/* overwrite initial terminator */
		loc = layout.lvis+1;
		dscr = (union dscrptr *) context.logvol_integrity;
		error = udf_write_dscr_phys(dscr, loc, 1);
		if (error)
			return error;
		loc++;
	
		/* mark end of integrity desciptor sequence again */
		error = udf_write_dscr_phys(terminator_dscr, loc, 1);
		if (error)
			return error;
	}

	/* write out unallocated space bitmap on non sequential media */
	if ((format_flags & FORMAT_SEQUENTIAL) == 0) {
		/* writeout unallocated space bitmap */
		loc  = layout.unalloc_space;
		dscr = (union dscrptr *) (context.part_unalloc_bits[data_part]);
		len  = layout.alloc_bitmap_dscr_size;
		error = udf_write_dscr_virt(dscr, loc, data_part, len);
		if (error)
			return error;
	}

	if (format_flags & FORMAT_META) {
		loc = layout.meta_file;
		dscr = (union dscrptr *) context.meta_file;
		error = udf_write_dscr_virt(dscr, loc, data_part, 1);
		if (error)
			return error;
	
		loc = layout.meta_mirror;
		dscr = (union dscrptr *) context.meta_mirror;
		error = udf_write_dscr_virt(dscr, loc, data_part, 1);
		if (error)
			return error;

		loc = layout.meta_bitmap;
		dscr = (union dscrptr *) context.meta_bitmap;
		error = udf_write_dscr_virt(dscr, loc, data_part, 1);
		if (error)
			return error;

		/* writeout unallocated space bitmap */
		loc  = layout.meta_bitmap_space;
		dscr = (union dscrptr *) (context.part_unalloc_bits[metadata_part]);
		len  = layout.meta_bitmap_dscr_size;
		error = udf_write_dscr_virt(dscr, loc, data_part, len);
		if (error)
			return error;
	}

	/* create a VAT and account for FSD+root */
	vat_dscr = NULL;
	if (format_flags & FORMAT_VAT) {
		/* update lvint to reflect the newest values (no writeout) */
		udf_update_lvintd(UDF_INTEGRITY_CLOSED);

		error = udf_create_new_VAT(&vat_dscr);
		if (error)
			return error;

		loc = layout.vat;
		error = udf_write_dscr_virt(vat_dscr, loc, metadata_part, 1);
		if (error)
			return error;
	}

	/* write out sectors */
	if ((error = writeout_write_queue()))
		return error;

	/* done */
	return 0;
}

/* --------------------------------------------------------------------- */

/* version can be specified as 0xabc or a.bc */
static int
parse_udfversion(const char *pos, uint32_t *version) {
	int hex = 0;
	char c1, c2, c3, c4;

	*version = 0;
	if (*pos == '0') {
		pos++;
		/* expect hex format */
		hex = 1;
		if (*pos++ != 'x')
			return 1;
	}

	c1 = *pos++;
	if (c1 < '0' || c1 > '9')
		return 1;
	c1 -= '0';

	c2 = *pos++;
	if (!hex) {
		if (c2 != '.')
			return 1;
		c2 = *pos++;
	}
	if (c2 < '0' || c2 > '9')
		return 1;
	c2 -= '0';

	c3 = *pos++;
	if (c3 < '0' || c3 > '9')
		return 1;
	c3 -= '0';

	c4 = *pos++;
	if (c4 != 0)
		return 1;

	*version = c1 * 0x100 + c2 * 0x10 + c3;
	return 0;
}


static int
a_udf_version(const char *s, const char *id_type)
{
	uint32_t version;

	if (parse_udfversion(s, &version))
		errx(1, "unknown %s id %s; specify as hex or float", id_type, s);
	return version;
}

/* --------------------------------------------------------------------- */

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-cFM] [-L loglabel] "
	    "[-P discid] [-S setlabel] [-s size] [-p perc] "
	    "[-t gmtoff] [-v min_udf] [-V max_udf] special\n", getprogname());
	exit(EXIT_FAILURE);
}


int
main(int argc, char **argv)
{
	struct tm *tm;
	struct stat st;
	time_t now;
	char  scrap[255];
	int ch, req_enable, req_disable, force;
	int error;

	setprogname(argv[0]);

	/* initialise */
	format_str    = strdup("");
	req_enable    = req_disable = 0;
	format_flags  = FORMAT_INVALID;
	force         = 0;
	check_surface = 0;

	srandom((unsigned long) time(NULL));
	udf_init_create_context();
	context.app_name  = APP_NAME;
	context.impl_name = IMPL_NAME;
	context.app_version_main = APP_VERSION_MAIN;
	context.app_version_sub  = APP_VERSION_SUB;

	/* minimum and maximum UDF versions we advise */
	context.min_udf = 0x201;
	context.max_udf = 0x201;

	/* use user's time zone as default */
	(void)time(&now);
	tm = localtime(&now);
	context.gmtoff = tm->tm_gmtoff;

	/* process options */
	while ((ch = getopt(argc, argv, "cFL:Mp:P:s:S:t:v:V:")) != -1) {
		switch (ch) {
		case 'c' :
			check_surface = 1;
			break;
		case 'F' :
			force = 1;
			break;
		case 'L' :
			if (context.logvol_name) free(context.logvol_name);
			context.logvol_name = strdup(optarg);
			break;
		case 'M' :
			req_disable |= FORMAT_META;
			break;
		case 'p' :
			meta_perc = a_num(optarg, "meta_perc");
			/* limit to `sensible` values */
			meta_perc = MIN(meta_perc, 99);
			meta_perc = MAX(meta_perc, 1);
			meta_fract = (float) meta_perc/100.0;
			break;
		case 'v' :
			context.min_udf = a_udf_version(optarg, "min_udf");
			if (context.min_udf > context.max_udf)
				context.max_udf = context.min_udf;
			break;
		case 'V' :
			context.max_udf = a_udf_version(optarg, "max_udf");
			if (context.min_udf > context.max_udf)
				context.min_udf = context.max_udf;
			break;
		case 'P' :
			context.primary_name = strdup(optarg);
			break;
		case 's' :
			/* TODO size argument; recordable emulation */
			break;
		case 'S' :
			if (context.volset_name) free(context.volset_name);
			context.volset_name = strdup(optarg);
			break;
		case 't' :
			/* time zone overide */
			context.gmtoff = a_num(optarg, "gmtoff");
			break;
		default  :
			usage();
			/* NOTREACHED */
		}
	}

	if (optind + 1 != argc)
		usage();

	/* get device and directory specifier */
	dev = argv[optind];

	/* open device */
	if ((fd = open(dev, O_RDWR, 0)) == -1) {
		perror("can't open device");
		return EXIT_FAILURE;
	}

	/* stat the device */
	if (fstat(fd, &st) != 0) {
		perror("can't stat the device");
		close(fd);
		return EXIT_FAILURE;
	}

	/* Formatting can only be done on raw devices */
	if (!S_ISCHR(st.st_mode)) {
		printf("%s is not a raw device\n", dev);
		close(fd);
		return EXIT_FAILURE;
	}

	/* just in case something went wrong, synchronise the drive's cache */
	udf_synchronise_caches();

	/* get disc information */
	error = udf_update_discinfo(&mmc_discinfo);
	if (error) {
		perror("can't retrieve discinfo");
		close(fd);
		return EXIT_FAILURE;
	}

	/* derive disc identifiers when not specified and check given */
	error = udf_proces_names();
	if (error) {
		/* error message has been printed */
		close(fd);
		return EXIT_FAILURE;
	}

	/* derive newfs disc format from disc profile */
	error = udf_derive_format(req_enable, req_disable, force);
	if (error)  {
		/* error message has been printed */
		close(fd);
		return EXIT_FAILURE;
	}

	udf_dump_discinfo(&mmc_discinfo);
	printf("Formatting disc compatible with UDF version %x to %x\n\n",
		context.min_udf, context.max_udf);
	(void)snprintb(scrap, sizeof(scrap), FORMAT_FLAGBITS,
	    (uint64_t) format_flags);
	printf("UDF properties       %s\n", scrap);
	printf("Volume set          `%s'\n", context.volset_name);
	printf("Primary volume      `%s`\n", context.primary_name);
	printf("Logical volume      `%s`\n", context.logvol_name);
	if (format_flags & FORMAT_META)
		printf("Metadata percentage  %d %%\n", meta_perc);
	printf("\n");

	/* prepare disc if nessisary (recordables mainly) */
	error = udf_prepare_disc();
	if (error) {
		perror("preparing disc failed");
		close(fd);
		return EXIT_FAILURE;
	};

	/* set up administration */
	error = udf_do_newfs();

	/* in any case, synchronise the drive's cache to prevent lockups */
	udf_synchronise_caches();

	close(fd);
	if (error)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/* --------------------------------------------------------------------- */

