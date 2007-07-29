/* $NetBSD: udf_subr.c,v 1.36.10.2 2007/07/29 13:31:12 ad Exp $ */

/*
 * Copyright (c) 2006 Reinoud Zandijk
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
#ifndef lint
__RCSID("$NetBSD: udf_subr.c,v 1.36.10.2 2007/07/29 13:31:12 ad Exp $");
#endif /* not lint */


#if defined(_KERNEL_OPT)
#include "opt_quota.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <miscfs/genfs/genfs_node.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <dev/clock_subr.h>

#include <fs/udf/ecma167-udf.h>
#include <fs/udf/udf_mount.h>

#include "udf.h"
#include "udf_subr.h"
#include "udf_bswap.h"


#define VTOI(vnode) ((struct udf_node *) vnode->v_data)


/* predefines */


#if 0
{
	int i, j, dlen;
	uint8_t *blob;

	blob = (uint8_t *) fid;
	dlen = file_size - (*offset);

	printf("blob = %p\n", blob);
	printf("dump of %d bytes\n", dlen);

	for (i = 0; i < dlen; i+ = 16) {
		printf("%04x ", i);
		for (j = 0; j < 16; j++) {
			if (i+j < dlen) {
				printf("%02x ", blob[i+j]);
			} else {
				printf("   ");
			}
		}
		for (j = 0; j < 16; j++) {
			if (i+j < dlen) {
				if (blob[i+j]>32 && blob[i+j]! = 127) {
					printf("%c", blob[i+j]);
				} else {
					printf(".");
				}
			}
		}
		printf("\n");
	}
	printf("\n");
}
Debugger();
#endif


/* --------------------------------------------------------------------- */

/* STUB */

static int
udf_bread(struct udf_mount *ump, uint32_t sector, struct buf **bpp)
{
	int sector_size = ump->discinfo.sector_size;
	int blks = sector_size / DEV_BSIZE;

	/* NOTE bread() checks if block is in cache or not */
	return bread(ump->devvp, sector*blks, sector_size, NOCRED, bpp);
}


/* --------------------------------------------------------------------- */

/*
 * Check if the blob starts with a good UDF tag. Tags are protected by a
 * checksum over the reader except one byte at position 4 that is the checksum
 * itself.
 */

int
udf_check_tag(void *blob)
{
	struct desc_tag *tag = blob;
	uint8_t *pos, sum, cnt;

	/* check TAG header checksum */
	pos = (uint8_t *) tag;
	sum = 0;

	for(cnt = 0; cnt < 16; cnt++) {
		if (cnt != 4)
			sum += *pos;
		pos++;
	}
	if (sum != tag->cksum) {
		/* bad tag header checksum; this is not a valid tag */
		return EINVAL;
	}

	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * check tag payload will check descriptor CRC as specified.
 * If the descriptor is too short, it will return EIO otherwise EINVAL.
 */

int
udf_check_tag_payload(void *blob, uint32_t max_length)
{
	struct desc_tag *tag = blob;
	uint16_t crc, crc_len;

	crc_len = udf_rw16(tag->desc_crc_len);

	/* check payload CRC if applicable */
	if (crc_len == 0)
		return 0;

	if (crc_len > max_length)
		return EIO;

	crc = udf_cksum(((uint8_t *) tag) + UDF_DESC_TAG_LENGTH, crc_len);
	if (crc != udf_rw16(tag->desc_crc)) {
		/* bad payload CRC; this is a broken tag */
		return EINVAL;
	}

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_validate_tag_sum(void *blob)
{
	struct desc_tag *tag = blob;
	uint8_t *pos, sum, cnt;

	/* calculate TAG header checksum */
	pos = (uint8_t *) tag;
	sum = 0;

	for(cnt = 0; cnt < 16; cnt++) {
		if (cnt != 4) sum += *pos;
		pos++;
	}
	tag->cksum = sum;	/* 8 bit */

	return 0;
}

/* --------------------------------------------------------------------- */

/* assumes sector number of descriptor to be saved already present */

int
udf_validate_tag_and_crc_sums(void *blob)
{
	struct desc_tag *tag  = blob;
	uint8_t         *btag = (uint8_t *) tag;
	uint16_t crc, crc_len;

	crc_len = udf_rw16(tag->desc_crc_len);

	/* check payload CRC if applicable */
	if (crc_len > 0) {
		crc = udf_cksum(btag + UDF_DESC_TAG_LENGTH, crc_len);
		tag->desc_crc = udf_rw16(crc);
	}

	/* calculate TAG header checksum */
	return udf_validate_tag_sum(blob);
}

/* --------------------------------------------------------------------- */

/*
 * XXX note the different semantics from udfclient: for FIDs it still rounds
 * up to sectors. Use udf_fidsize() for a correct length.
 */

int
udf_tagsize(union dscrptr *dscr, uint32_t udf_sector_size)
{
	uint32_t size, tag_id, num_secs, elmsz;

	tag_id = udf_rw16(dscr->tag.id);

	switch (tag_id) {
	case TAGID_LOGVOL :
		size  = sizeof(struct logvol_desc) - 1;
		size += udf_rw32(dscr->lvd.mt_l);
		break;
	case TAGID_UNALLOC_SPACE :
		elmsz = sizeof(struct extent_ad);
		size  = sizeof(struct unalloc_sp_desc) - elmsz;
		size += udf_rw32(dscr->usd.alloc_desc_num) * elmsz;
		break;
	case TAGID_FID :
		size = UDF_FID_SIZE + dscr->fid.l_fi + udf_rw16(dscr->fid.l_iu);
		size = (size + 3) & ~3;
		break;
	case TAGID_LOGVOL_INTEGRITY :
		size  = sizeof(struct logvol_int_desc) - sizeof(uint32_t);
		size += udf_rw32(dscr->lvid.l_iu);
		size += (2 * udf_rw32(dscr->lvid.num_part) * sizeof(uint32_t));
		break;
	case TAGID_SPACE_BITMAP :
		size  = sizeof(struct space_bitmap_desc) - 1;
		size += udf_rw32(dscr->sbd.num_bytes);
		break;
	case TAGID_SPARING_TABLE :
		elmsz = sizeof(struct spare_map_entry);
		size  = sizeof(struct udf_sparing_table) - elmsz;
		size += udf_rw16(dscr->spt.rt_l) * elmsz;
		break;
	case TAGID_FENTRY :
		size  = sizeof(struct file_entry);
		size += udf_rw32(dscr->fe.l_ea) + udf_rw32(dscr->fe.l_ad)-1;
		break;
	case TAGID_EXTFENTRY :
		size  = sizeof(struct extfile_entry);
		size += udf_rw32(dscr->efe.l_ea) + udf_rw32(dscr->efe.l_ad)-1;
		break;
	case TAGID_FSD :
		size  = sizeof(struct fileset_desc);
		break;
	default :
		size = sizeof(union dscrptr);
		break;
	}

	if ((size == 0) || (udf_sector_size == 0)) return 0;

	/* round up in sectors */
	num_secs = (size + udf_sector_size -1) / udf_sector_size;
	return num_secs * udf_sector_size;
}


static int
udf_fidsize(struct fileid_desc *fid, uint32_t udf_sector_size)
{
	uint32_t size;

	if (udf_rw16(fid->tag.id) != TAGID_FID)
		panic("got udf_fidsize on non FID\n");

	size = UDF_FID_SIZE + fid->l_fi + udf_rw16(fid->l_iu);
	size = (size + 3) & ~3;

	return size;
}

/* --------------------------------------------------------------------- */

/*
 * Problem with read_descriptor are long descriptors spanning more than one
 * sector. Luckily long descriptors can't be in `logical space'.
 *
 * Size of allocated piece is returned in multiple of sector size due to 
 * udf_calc_udf_malloc_size().
 */

int
udf_read_descriptor(struct udf_mount *ump, uint32_t sector,
		    struct malloc_type *mtype, union dscrptr **dstp)
{
	union dscrptr *src, *dst;
	struct buf *bp;
	uint8_t *pos;
	int blks, blk, dscrlen;
	int i, error, sector_size;

	sector_size = ump->discinfo.sector_size;

	*dstp = dst = NULL;
	dscrlen = sector_size;

	/* read initial piece */
	error = udf_bread(ump, sector, &bp);
	DPRINTFIF(DESCRIPTOR, error, ("read error (%d)\n", error));

	if (!error) {
		/* check if its a valid tag */
		error = udf_check_tag(bp->b_data);
		if (error) {
			/* check if its an empty block */
			pos = bp->b_data;
			for (i = 0; i < sector_size; i++, pos++) {
				if (*pos) break;
			}
			if (i == sector_size) {
				/* return no error but with no dscrptr */
				/* dispose first block */
				brelse(bp);
				return 0;
			}
		}
	}
	DPRINTFIF(DESCRIPTOR, error, ("bad tag checksum\n"));
	if (!error) {
		src = (union dscrptr *) bp->b_data;
		dscrlen = udf_tagsize(src, sector_size);
		dst = malloc(dscrlen, mtype, M_WAITOK);
		memcpy(dst, src, sector_size);
	}
	/* dispose first block */
	bp->b_flags |= B_AGE;
	brelse(bp);

	if (!error && (dscrlen > sector_size)) {
		DPRINTF(DESCRIPTOR, ("multi block descriptor read\n"));
		/*
		 * Read the rest of descriptor. Since it is only used at mount
		 * time its overdone to define and use a specific udf_breadn
		 * for this alone.
		 */
		blks = (dscrlen + sector_size -1) / sector_size;
		for (blk = 1; blk < blks; blk++) {
			error = udf_bread(ump, sector + blk, &bp);
			if (error) {
				brelse(bp);
				break;
			}
			pos = (uint8_t *) dst + blk*sector_size;
			memcpy(pos, bp->b_data, sector_size);

			/* dispose block */
			bp->b_flags |= B_AGE;
			brelse(bp);
		}
		DPRINTFIF(DESCRIPTOR, error, ("read error on multi (%d)\n",
		    error));
	}
	if (!error) {
		error = udf_check_tag_payload(dst, dscrlen);
		DPRINTFIF(DESCRIPTOR, error, ("bad payload check sum\n"));
	}
	if (error && dst) {
		free(dst, mtype);
		dst = NULL;
	}
	*dstp = dst;

	return error;
}

/* --------------------------------------------------------------------- */
#ifdef DEBUG
static void
udf_dump_discinfo(struct udf_mount *ump)
{
	char   bits[128];
	struct mmc_discinfo *di = &ump->discinfo;

	if ((udf_verbose & UDF_DEBUG_VOLUMES) == 0)
		return;

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
	bitmask_snprintf(di->disc_flags, MMC_DFLAGS_FLAGBITS, bits,
		sizeof(bits));
	printf("\tdisc flags         %s\n", bits);
	printf("\tdisc id            %x\n", di->disc_id);
	printf("\tdisc barcode       %"PRIx64"\n", di->disc_barcode);

	printf("\tnum sessions       %d\n", di->num_sessions);
	printf("\tnum tracks         %d\n", di->num_tracks);

	bitmask_snprintf(di->mmc_cur, MMC_CAP_FLAGBITS, bits, sizeof(bits));
	printf("\tcapabilities cur   %s\n", bits);
	bitmask_snprintf(di->mmc_cap, MMC_CAP_FLAGBITS, bits, sizeof(bits));
	printf("\tcapabilities cap   %s\n", bits);
}
#else
#define udf_dump_discinfo(a);
#endif

/* not called often */
int
udf_update_discinfo(struct udf_mount *ump)
{
	struct vnode *devvp = ump->devvp;
	struct partinfo dpart;
	struct mmc_discinfo *di;
	int error;

	DPRINTF(VOLUMES, ("read/update disc info\n"));
	di = &ump->discinfo;
	memset(di, 0, sizeof(struct mmc_discinfo));

	/* check if we're on a MMC capable device, i.e. CD/DVD */
	error = VOP_IOCTL(devvp, MMCGETDISCINFO, di, FKIOCTL, NOCRED, NULL);
	if (error == 0) {
		udf_dump_discinfo(ump);
		return 0;
	}

	/* disc partition support */
	error = VOP_IOCTL(devvp, DIOCGPART, &dpart, FREAD, NOCRED, NULL);
	if (error)
		return ENODEV;

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
	di->last_possible_lba = dpart.part->p_size;
	di->sector_size       = dpart.disklab->d_secsize;
	di->blockingnr        = 1;

	di->num_sessions = 1;
	di->num_tracks   = 1;

	di->first_track  = 1;
	di->first_track_last_session = di->last_track_last_session = 1;

	udf_dump_discinfo(ump);
	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_update_trackinfo(struct udf_mount *ump, struct mmc_trackinfo *ti)
{
	struct vnode *devvp = ump->devvp;
	struct mmc_discinfo *di = &ump->discinfo;
	int error, class;

	DPRINTF(VOLUMES, ("read track info\n"));

	class = di->mmc_class;
	if (class != MMC_CLASS_DISC) {
		/* tracknr specified in struct ti */
		error = VOP_IOCTL(devvp, MMCGETTRACKINFO, ti, FKIOCTL,
			NOCRED, NULL);
		return error;
	}

	/* disc partition support */
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

/* --------------------------------------------------------------------- */

/* track/session searching for mounting */

static int
udf_search_tracks(struct udf_mount *ump, struct udf_args *args,
		  int *first_tracknr, int *last_tracknr)
{
	struct mmc_trackinfo trackinfo;
	uint32_t tracknr, start_track, num_tracks;
	int error;

	/* if negative, sessionnr is relative to last session */
	if (args->sessionnr < 0) {
		args->sessionnr += ump->discinfo.num_sessions;
		/* sanity */
		if (args->sessionnr < 0)
			args->sessionnr = 0;
	}

	/* sanity */
	if (args->sessionnr > ump->discinfo.num_sessions)
		args->sessionnr = ump->discinfo.num_sessions;

	/* search the tracks for this session, zero session nr indicates last */
	if (args->sessionnr == 0) {
		args->sessionnr = ump->discinfo.num_sessions;
		if (ump->discinfo.last_session_state == MMC_STATE_EMPTY) {
			args->sessionnr--;
		}
	}

	/* search the first and last track of the specified session */
	num_tracks  = ump->discinfo.num_tracks;
	start_track = ump->discinfo.first_track;

	/* search for first track of this session */
	for (tracknr = start_track; tracknr <= num_tracks; tracknr++) {
		/* get track info */
		trackinfo.tracknr = tracknr;
		error = udf_update_trackinfo(ump, &trackinfo);
		if (error)
			return error;

		if (trackinfo.sessionnr == args->sessionnr)
			break;
	}
	*first_tracknr = tracknr;

	/* search for last track of this session */
	for (;tracknr <= num_tracks; tracknr++) {
		/* get track info */
		trackinfo.tracknr = tracknr;
		error = udf_update_trackinfo(ump, &trackinfo);
		if (error || (trackinfo.sessionnr != args->sessionnr)) {
			tracknr--;
			break;
		}
	}
	if (tracknr > num_tracks)
		tracknr--;

	*last_tracknr = tracknr;

	assert(*last_tracknr >= *first_tracknr);
	return 0;
}

/* --------------------------------------------------------------------- */

static int
udf_read_anchor(struct udf_mount *ump, uint32_t sector, struct anchor_vdp **dst)
{
	int error;

	error = udf_read_descriptor(ump, sector, M_UDFVOLD,
			(union dscrptr **) dst);
	if (!error) {
		/* blank terminator blocks are not allowed here */
		if (*dst == NULL)
			return ENOENT;
		if (udf_rw16((*dst)->tag.id) != TAGID_ANCHOR) {
			error = ENOENT;
			free(*dst, M_UDFVOLD);
			*dst = NULL;
			DPRINTF(VOLUMES, ("Not an anchor\n"));
		}
	}

	return error;
}


int
udf_read_anchors(struct udf_mount *ump, struct udf_args *args)
{
	struct mmc_trackinfo first_track;
	struct mmc_trackinfo last_track;
	struct anchor_vdp **anchorsp;
	uint32_t track_start;
	uint32_t track_end;
	uint32_t positions[4];
	int first_tracknr, last_tracknr;
	int error, anch, ok, first_anchor;

	/* search the first and last track of the specified session */
	error = udf_search_tracks(ump, args, &first_tracknr, &last_tracknr);
	if (!error) {
		first_track.tracknr = first_tracknr;
		error = udf_update_trackinfo(ump, &first_track);
	}
	if (!error) {
		last_track.tracknr = last_tracknr;
		error = udf_update_trackinfo(ump, &last_track);
	}
	if (error) {
		printf("UDF mount: reading disc geometry failed\n");
		return 0;
	}

	track_start = first_track.track_start;

	/* `end' is not as straitforward as start. */
	track_end =   last_track.track_start
		    + last_track.track_size - last_track.free_blocks - 1;

	if (ump->discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) {
		/* end of track is not straitforward here */
		if (last_track.flags & MMC_TRACKINFO_LRA_VALID)
			track_end = last_track.last_recorded;
		else if (last_track.flags & MMC_TRACKINFO_NWA_VALID)
			track_end = last_track.next_writable
				    - ump->discinfo.link_block_penalty;
	}

	/* its no use reading a blank track */
	first_anchor = 0;
	if (first_track.flags & MMC_TRACKINFO_BLANK)
		first_anchor = 1;

	/* read anchors start+256, start+512, end-256, end */
	positions[0] = track_start+256;
	positions[1] =   track_end-256;
	positions[2] =   track_end;
	positions[3] = track_start+512;	/* [UDF 2.60/6.11.2] */
	/* XXX shouldn't +512 be prefered above +256 for compat with Roxio CD */

	ok = 0;
	anchorsp = ump->anchors;
	for (anch = first_anchor; anch < 4; anch++) {
		DPRINTF(VOLUMES, ("Read anchor %d at sector %d\n", anch,
		    positions[anch]));
		error = udf_read_anchor(ump, positions[anch], anchorsp);
		if (!error) {
			anchorsp++;
			ok++;
		}
	}

	/* VATs are only recorded on sequential media, but initialise */
	ump->first_possible_vat_location = track_start + 256 + 1;
	ump->last_possible_vat_location  = track_end
		+ ump->discinfo.blockingnr;

	return ok;
}

/* --------------------------------------------------------------------- */

/* we dont try to be smart; we just record the parts */
#define UDF_UPDATE_DSCR(name, dscr) \
	if (name) \
		free(name, M_UDFVOLD); \
	name = dscr;

static int
udf_process_vds_descriptor(struct udf_mount *ump, union dscrptr *dscr)
{
	struct part_desc *part;
	uint16_t phys_part, raw_phys_part;

	DPRINTF(VOLUMES, ("\tprocessing VDS descr %d\n",
	    udf_rw16(dscr->tag.id)));
	switch (udf_rw16(dscr->tag.id)) {
	case TAGID_PRI_VOL :		/* primary partition		*/
		UDF_UPDATE_DSCR(ump->primary_vol, &dscr->pvd);
		break;
	case TAGID_LOGVOL :		/* logical volume		*/
		UDF_UPDATE_DSCR(ump->logical_vol, &dscr->lvd);
		break;
	case TAGID_UNALLOC_SPACE :	/* unallocated space		*/
		UDF_UPDATE_DSCR(ump->unallocated, &dscr->usd);
		break;
	case TAGID_IMP_VOL :		/* implementation		*/
		/* XXX do we care about multiple impl. descr ? */
		UDF_UPDATE_DSCR(ump->implementation, &dscr->ivd);
		break;
	case TAGID_PARTITION :		/* physical partition		*/
		/* not much use if its not allocated */
		if ((udf_rw16(dscr->pd.flags) & UDF_PART_FLAG_ALLOCATED) == 0) {
			free(dscr, M_UDFVOLD);
			break;
		}

		/*
		 * BUGALERT: some rogue implementations use random physical
		 * partion numbers to break other implementations so lookup
		 * the number.
		 */
		raw_phys_part = udf_rw16(dscr->pd.part_num);
		for (phys_part = 0; phys_part < UDF_PARTITIONS; phys_part++) {
			part = ump->partitions[phys_part];
			if (part == NULL)
				break;
			if (udf_rw16(part->part_num) == raw_phys_part)
				break;
		}
		if (phys_part == UDF_PARTITIONS) {
			free(dscr, M_UDFVOLD);
			return EINVAL;
		}

		UDF_UPDATE_DSCR(ump->partitions[phys_part], &dscr->pd);
		break;
	case TAGID_VOL :		/* volume space extender; rare	*/
		DPRINTF(VOLUMES, ("VDS extender ignored\n"));
		free(dscr, M_UDFVOLD);
		break;
	default :
		DPRINTF(VOLUMES, ("Unhandled VDS type %d\n",
		    udf_rw16(dscr->tag.id)));
		free(dscr, M_UDFVOLD);
	}

	return 0;
}
#undef UDF_UPDATE_DSCR

/* --------------------------------------------------------------------- */

static int
udf_read_vds_extent(struct udf_mount *ump, uint32_t loc, uint32_t len)
{
	union dscrptr *dscr;
	uint32_t sector_size, dscr_size;
	int error;

	sector_size = ump->discinfo.sector_size;

	/* loc is sectornr, len is in bytes */
	error = EIO;
	while (len) {
		error = udf_read_descriptor(ump, loc, M_UDFVOLD, &dscr);
		if (error)
			return error;

		/* blank block is a terminator */
		if (dscr == NULL)
			return 0;

		/* TERM descriptor is a terminator */
		if (udf_rw16(dscr->tag.id) == TAGID_TERM) {
			free(dscr, M_UDFVOLD);
			return 0;
		}

		/* process all others */
		dscr_size = udf_tagsize(dscr, sector_size);
		error = udf_process_vds_descriptor(ump, dscr);
		if (error) {
			free(dscr, M_UDFVOLD);
			break;
		}
		assert((dscr_size % sector_size) == 0);

		len -= dscr_size;
		loc += dscr_size / sector_size;
	}

	return error;
}


int
udf_read_vds_space(struct udf_mount *ump)
{
	struct anchor_vdp *anchor, *anchor2;
	size_t size;
	uint32_t main_loc, main_len;
	uint32_t reserve_loc, reserve_len;
	int error;

	/*
	 * read in VDS space provided by the anchors; if one descriptor read
	 * fails, try the mirror sector.
	 *
	 * check if 2nd anchor is different from 1st; if so, go for 2nd. This
	 * avoids the `compatibility features' of DirectCD that may confuse
	 * stuff completely.
	 */

	anchor  = ump->anchors[0];
	anchor2 = ump->anchors[1];
	assert(anchor);

	if (anchor2) {
		size = sizeof(struct extent_ad);
		if (memcmp(&anchor->main_vds_ex, &anchor2->main_vds_ex, size))
			anchor = anchor2;
		/* reserve is specified to be a literal copy of main */
	}

	main_loc    = udf_rw32(anchor->main_vds_ex.loc);
	main_len    = udf_rw32(anchor->main_vds_ex.len);

	reserve_loc = udf_rw32(anchor->reserve_vds_ex.loc);
	reserve_len = udf_rw32(anchor->reserve_vds_ex.len);

	error = udf_read_vds_extent(ump, main_loc, main_len);
	if (error) {
		printf("UDF mount: reading in reserve VDS extent\n");
		error = udf_read_vds_extent(ump, reserve_loc, reserve_len);
	}

	return error;
}

/* --------------------------------------------------------------------- */

/*
 * Read in the logical volume integrity sequence pointed to by our logical
 * volume descriptor. Its a sequence that can be extended using fields in the
 * integrity descriptor itself. On sequential media only one is found, on
 * rewritable media a sequence of descriptors can be found as a form of
 * history keeping and on non sequential write-once media the chain is vital
 * to allow more and more descriptors to be written. The last descriptor
 * written in an extent needs to claim space for a new extent.
 */

static int
udf_retrieve_lvint(struct udf_mount *ump, struct logvol_int_desc **lvintp)
{
	union dscrptr *dscr;
	struct logvol_int_desc *lvint;
	uint32_t sector_size, sector, len;
	int dscr_type, error;

	sector_size = ump->discinfo.sector_size;
	len    = udf_rw32(ump->logical_vol->integrity_seq_loc.len);
	sector = udf_rw32(ump->logical_vol->integrity_seq_loc.loc);

	lvint = NULL;
	dscr  = NULL;
	error = 0;
	while (len) {
		/* read in our integrity descriptor */
		error = udf_read_descriptor(ump, sector, M_UDFVOLD, &dscr);
		if (!error) {
			if (dscr == NULL)
				break;		/* empty terminates */
			dscr_type = udf_rw16(dscr->tag.id);
			if (dscr_type == TAGID_TERM) {
				break;		/* clean terminator */
			}
			if (dscr_type != TAGID_LOGVOL_INTEGRITY) {
				/* fatal... corrupt disc */
				error = ENOENT;
				break;
			}
			if (lvint)
				free(lvint, M_UDFVOLD);
			lvint = &dscr->lvid;
			dscr = NULL;
		} /* else hope for the best... maybe the next is ok */

		DPRINTFIF(VOLUMES, lvint, ("logvol integrity read, state %s\n",
		    udf_rw32(lvint->integrity_type) ? "CLOSED" : "OPEN"));

		/* proceed sequential */
		sector += 1;
		len    -= sector_size;

		/* are we linking to a new piece? */
		if (lvint->next_extent.len) {
			len    = udf_rw32(lvint->next_extent.len);
			sector = udf_rw32(lvint->next_extent.loc);
		}
	}

	/* clean up the mess, esp. when there is an error */
	if (dscr)
		free(dscr, M_UDFVOLD);

	if (error && lvint) {
		free(lvint, M_UDFVOLD);
		lvint = NULL;
	}

	if (!lvint)
		error = ENOENT;

	*lvintp = lvint;
	return error;
}

/* --------------------------------------------------------------------- */

/*
 * Checks if ump's vds information is correct and complete
 */

int
udf_process_vds(struct udf_mount *ump, struct udf_args *args)
{
	union udf_pmap *mapping;
	struct logvol_int_desc *lvint;
	struct udf_logvol_info *lvinfo;
	struct part_desc *part;
	uint32_t n_pm, mt_l;
	uint8_t *pmap_pos;
	char *domain_name, *map_name;
	const char *check_name;
	int pmap_stype, pmap_size;
	int pmap_type, log_part, phys_part, raw_phys_part;
	int n_phys, n_virt, n_spar, n_meta;
	int len, error;

	if (ump == NULL)
		return ENOENT;

	/* we need at least an anchor (trivial, but for safety) */
	if (ump->anchors[0] == NULL)
		return EINVAL;

	/* we need at least one primary and one logical volume descriptor */
	if ((ump->primary_vol == NULL) || (ump->logical_vol) == NULL)
		return EINVAL;

	/* we need at least one partition descriptor */
	if (ump->partitions[0] == NULL)
		return EINVAL;

	/* check logical volume sector size verses device sector size */
	if (udf_rw32(ump->logical_vol->lb_size) != ump->discinfo.sector_size) {
		printf("UDF mount: format violation, lb_size != sector size\n");
		return EINVAL;
	}

	domain_name = ump->logical_vol->domain_id.id;
	if (strncmp(domain_name, "*OSTA UDF Compliant", 20)) {
		printf("mount_udf: disc not OSTA UDF Compliant, aborting\n");
		return EINVAL;
	}

	/* retrieve logical volume integrity sequence */
	error = udf_retrieve_lvint(ump, &ump->logvol_integrity);

	/*
	 * We need at least one logvol integrity descriptor recorded.  Note
	 * that its OK to have an open logical volume integrity here. The VAT
	 * will close/update the integrity.
	 */
	if (ump->logvol_integrity == NULL)
		return EINVAL;

	/* process derived structures */
	n_pm   = udf_rw32(ump->logical_vol->n_pm);   /* num partmaps         */
	lvint  = ump->logvol_integrity;
	lvinfo = (struct udf_logvol_info *) (&lvint->tables[2 * n_pm]);
	ump->logvol_info = lvinfo;

	/* TODO check udf versions? */

	/*
	 * check logvol mappings: effective virt->log partmap translation
	 * check and recording of the mapping results. Saves expensive
	 * strncmp() in tight places.
	 */
	DPRINTF(VOLUMES, ("checking logvol mappings\n"));
	n_pm = udf_rw32(ump->logical_vol->n_pm);   /* num partmaps         */
	mt_l = udf_rw32(ump->logical_vol->mt_l);   /* partmaps data length */
	pmap_pos =  ump->logical_vol->maps;

	if (n_pm > UDF_PMAPS) {
		printf("UDF mount: too many mappings\n");
		return EINVAL;
	}

	n_phys = n_virt = n_spar = n_meta = 0;
	for (log_part = 0; log_part < n_pm; log_part++) {
		mapping = (union udf_pmap *) pmap_pos;
		pmap_stype = pmap_pos[0];
		pmap_size  = pmap_pos[1];
		switch (pmap_stype) {
		case 1:	/* physical mapping */
			/* volseq    = udf_rw16(mapping->pm1.vol_seq_num); */
			raw_phys_part = udf_rw16(mapping->pm1.part_num);
			pmap_type = UDF_VTOP_TYPE_PHYS;
			n_phys++;
			break;
		case 2: /* virtual/sparable/meta mapping */
			map_name  = mapping->pm2.part_id.id;
			/* volseq  = udf_rw16(mapping->pm2.vol_seq_num); */
			raw_phys_part = udf_rw16(mapping->pm2.part_num);
			pmap_type = UDF_VTOP_TYPE_UNKNOWN;
			len = UDF_REGID_ID_SIZE;

			check_name = "*UDF Virtual Partition";
			if (strncmp(map_name, check_name, len) == 0) {
				pmap_type = UDF_VTOP_TYPE_VIRT;
				n_virt++;
				break;
			}
			check_name = "*UDF Sparable Partition";
			if (strncmp(map_name, check_name, len) == 0) {
				pmap_type = UDF_VTOP_TYPE_SPARABLE;
				n_spar++;
				break;
			}
			check_name = "*UDF Metadata Partition";
			if (strncmp(map_name, check_name, len) == 0) {
				pmap_type = UDF_VTOP_TYPE_META;
				n_meta++;
				break;
			}
			break;
		default:
			return EINVAL;
		}

		/*
		 * BUGALERT: some rogue implementations use random physical
		 * partion numbers to break other implementations so lookup
		 * the number.
		 */
		for (phys_part = 0; phys_part < UDF_PARTITIONS; phys_part++) {
			part = ump->partitions[phys_part];
			if (part == NULL)
				continue;
			if (udf_rw16(part->part_num) == raw_phys_part)
				break;
		}

		DPRINTF(VOLUMES, ("\t%d -> %d(%d) type %d\n", log_part,
		    raw_phys_part, phys_part, pmap_type));
	
		if (phys_part == UDF_PARTITIONS)
			return EINVAL;
		if (pmap_type == UDF_VTOP_TYPE_UNKNOWN)
			return EINVAL;

		ump->vtop   [log_part] = phys_part;
		ump->vtop_tp[log_part] = pmap_type;

		pmap_pos += pmap_size;
	}
	/* not winning the beauty contest */
	ump->vtop_tp[UDF_VTOP_RAWPART] = UDF_VTOP_TYPE_RAW;

	/* test some basic UDF assertions/requirements */
	if ((n_virt > 1) || (n_spar > 1) || (n_meta > 1))
		return EINVAL;

	if (n_virt) {
		if ((n_phys == 0) || n_spar || n_meta)
			return EINVAL;
	}
	if (n_spar + n_phys == 0)
		return EINVAL;

	/* vat's can only be on a sequential media */
	ump->data_alloc = UDF_ALLOC_SPACEMAP;
	if (n_virt)
		ump->data_alloc = UDF_ALLOC_SEQUENTIAL;

	ump->meta_alloc = UDF_ALLOC_SPACEMAP;
	if (n_virt)
		ump->meta_alloc = UDF_ALLOC_VAT;
	if (n_meta)
		ump->meta_alloc = UDF_ALLOC_METABITMAP;

	/* special cases for pseudo-overwrite */
	if (ump->discinfo.mmc_cur & MMC_CAP_PSEUDOOVERWRITE) {
		ump->data_alloc = UDF_ALLOC_SEQUENTIAL;
		if (n_meta) {
			ump->meta_alloc = UDF_ALLOC_METASEQUENTIAL;
		} else {
			ump->meta_alloc = UDF_ALLOC_RELAXEDSEQUENTIAL;
		}
	}

	DPRINTF(VOLUMES, ("\tdata alloc scheme %d, meta alloc scheme %d\n",
	    ump->data_alloc, ump->meta_alloc));
	/* TODO determine partitions to write data and metadata ? */

	/* signal its OK for now */
	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * Read in complete VAT file and check if its indeed a VAT file descriptor
 */

static int
udf_check_for_vat(struct udf_node *vat_node)
{
	struct udf_mount *ump;
	struct icb_tag   *icbtag;
	struct timestamp *mtime;
	struct regid     *regid;
	struct udf_vat   *vat;
	struct udf_logvol_info *lvinfo;
	uint32_t  vat_length, alloc_length;
	uint32_t  vat_offset, vat_entries;
	uint32_t  sector_size;
	uint32_t  sectors;
	uint32_t *raw_vat;
	char     *regid_name;
	int filetype;
	int error;

	/* vat_length is really 64 bits though impossible */

	DPRINTF(VOLUMES, ("Checking for VAT\n"));
	if (!vat_node)
		return ENOENT;

	/* get mount info */
	ump = vat_node->ump;

	/* check assertions */
	assert(vat_node->fe || vat_node->efe);
	assert(ump->logvol_integrity);

	/* get information from fe/efe */
	if (vat_node->fe) {
		vat_length = udf_rw64(vat_node->fe->inf_len);
		icbtag = &vat_node->fe->icbtag;
		mtime  = &vat_node->fe->mtime;
	} else {
		vat_length = udf_rw64(vat_node->efe->inf_len);
		icbtag = &vat_node->efe->icbtag;
		mtime  = &vat_node->efe->mtime;
	}

	/* Check icb filetype! it has to be 0 or UDF_ICB_FILETYPE_VAT */
	filetype = icbtag->file_type;
	if ((filetype != 0) && (filetype != UDF_ICB_FILETYPE_VAT))
		return ENOENT;

	DPRINTF(VOLUMES, ("\tPossible VAT length %d\n", vat_length));
	/* place a sanity check on the length; currently 1Mb in size */
	if (vat_length > 1*1024*1024)
		return ENOENT;

	/* get sector size */
	sector_size = vat_node->ump->discinfo.sector_size;

	/* calculate how many sectors to read in and how much to allocate */
	sectors = (vat_length + sector_size -1) / sector_size;
	alloc_length = (sectors + 2) * sector_size;

	/* try to allocate the space */
	ump->vat_table_alloc_length = alloc_length;
	ump->vat_table = malloc(alloc_length, M_UDFVOLD, M_CANFAIL | M_WAITOK);
	if (!ump->vat_table)
		return ENOMEM;		/* impossible to allocate */
	DPRINTF(VOLUMES, ("\talloced fine\n"));

	/* read it in! */
	raw_vat = (uint32_t *) ump->vat_table;
	error = udf_read_file_extent(vat_node, 0, sectors, (uint8_t *) raw_vat);
	if (error) {
		DPRINTF(VOLUMES, ("\tread failed : %d\n", error));
		/* not completely readable... :( bomb out */
		free(ump->vat_table, M_UDFVOLD);
		ump->vat_table = NULL;
		return error;
	}
	DPRINTF(VOLUMES, ("VAT read in fine!\n"));

	/*
	 * check contents of the file if its the old 1.50 VAT table format.
	 * Its notoriously broken and allthough some implementations support an
	 * extention as defined in the UDF 1.50 errata document, its doubtfull
	 * to be useable since a lot of implementations don't maintain it.
	 */
	lvinfo = ump->logvol_info;

	if (filetype == 0) {
		/* definition */
		vat_offset  = 0;
		vat_entries = (vat_length-36)/4;

		/* check 1.50 VAT */
		regid = (struct regid *) (raw_vat + vat_entries);
		regid_name = (char *) regid->id;
		error = strncmp(regid_name, "*UDF Virtual Alloc Tbl", 22);
		if (error) {
			DPRINTF(VOLUMES, ("VAT format 1.50 rejected\n"));
			free(ump->vat_table, M_UDFVOLD);
			ump->vat_table = NULL;
			return ENOENT;
		}
		/* TODO update LVID from "*UDF VAT LVExtension" ext. attr. */
	} else {
		vat = (struct udf_vat *) raw_vat;

		/* definition */
		vat_offset  = vat->header_len;
		vat_entries = (vat_length - vat_offset)/4;

		assert(lvinfo);
		lvinfo->num_files        = vat->num_files;
		lvinfo->num_directories  = vat->num_directories;
		lvinfo->min_udf_readver  = vat->min_udf_readver;
		lvinfo->min_udf_writever = vat->min_udf_writever;
		lvinfo->max_udf_writever = vat->max_udf_writever;
	}

	ump->vat_offset  = vat_offset;
	ump->vat_entries = vat_entries;

	DPRINTF(VOLUMES, ("VAT format accepted, marking it closed\n"));
	ump->logvol_integrity->integrity_type = udf_rw32(UDF_INTEGRITY_CLOSED);
	ump->logvol_integrity->time           = *mtime;

	return 0;	/* success! */
}

/* --------------------------------------------------------------------- */

static int
udf_search_vat(struct udf_mount *ump, union udf_pmap *mapping)
{
	struct udf_node *vat_node;
	struct long_ad	 icb_loc;
	uint32_t early_vat_loc, late_vat_loc, vat_loc;
	int error;

	/* mapping info not needed */
	mapping = mapping;

	vat_loc = ump->last_possible_vat_location;
	early_vat_loc = vat_loc - 2 * ump->discinfo.blockingnr;
	early_vat_loc = MAX(early_vat_loc, ump->first_possible_vat_location);
	late_vat_loc  = vat_loc + 1024;

	/* TODO first search last sector? */
	do {
		DPRINTF(VOLUMES, ("Checking for VAT at sector %d\n", vat_loc));
		icb_loc.loc.part_num = udf_rw16(UDF_VTOP_RAWPART);
		icb_loc.loc.lb_num   = udf_rw32(vat_loc);

		error = udf_get_node(ump, &icb_loc, &vat_node);
		if (!error) error = udf_check_for_vat(vat_node);
		if (!error) break;
		if (vat_node) {
			vput(vat_node->vnode);
			udf_dispose_node(vat_node);
		}
		vat_loc--;	/* walk backwards */
	} while (vat_loc >= early_vat_loc);

	/* we don't need our VAT node anymore */
	if (vat_node) {
		vput(vat_node->vnode);
		udf_dispose_node(vat_node);
	}

	return error;
}

/* --------------------------------------------------------------------- */

static int
udf_read_sparables(struct udf_mount *ump, union udf_pmap *mapping)
{
	union dscrptr *dscr;
	struct part_map_spare *pms = &mapping->pms;
	uint32_t lb_num;
	int spar, error;

	/*
	 * The partition mapping passed on to us specifies the information we
	 * need to locate and initialise the sparable partition mapping
	 * information we need.
	 */

	DPRINTF(VOLUMES, ("Read sparable table\n"));
	ump->sparable_packet_len = udf_rw16(pms->packet_len);
	for (spar = 0; spar < pms->n_st; spar++) {
		lb_num = pms->st_loc[spar];
		DPRINTF(VOLUMES, ("Checking for sparing table %d\n", lb_num));
		error = udf_read_descriptor(ump, lb_num, M_UDFVOLD, &dscr);
		if (!error && dscr) {
			if (udf_rw16(dscr->tag.id) == TAGID_SPARING_TABLE) {
				if (ump->sparing_table)
					free(ump->sparing_table, M_UDFVOLD);
				ump->sparing_table = &dscr->spt;
				dscr = NULL;
				DPRINTF(VOLUMES,
				    ("Sparing table accepted (%d entries)\n",
				     udf_rw16(ump->sparing_table->rt_l)));
				break;	/* we're done */
			}
		}
		if (dscr)
			free(dscr, M_UDFVOLD);
	}

	if (ump->sparing_table)
		return 0;

	return ENOENT;
}

/* --------------------------------------------------------------------- */

#define UDF_SET_SYSTEMFILE(vp) \
	simple_lock(&(vp)->v_interlock);	\
	(vp)->v_flag |= VSYSTEM;		\
	simple_unlock(&(vp)->v_interlock);\
	vref(vp);			\
	vput(vp);			\

static int
udf_read_metadata_files(struct udf_mount *ump, union udf_pmap *mapping)
{
	struct part_map_meta *pmm = &mapping->pmm;
	struct long_ad	 icb_loc;
	struct vnode *vp;
	int error;

	DPRINTF(VOLUMES, ("Reading in Metadata files\n"));
	icb_loc.loc.part_num = pmm->part_num;
	icb_loc.loc.lb_num   = pmm->meta_file_lbn;
	DPRINTF(VOLUMES, ("Metadata file\n"));
	error = udf_get_node(ump, &icb_loc, &ump->metadata_file);
	if (ump->metadata_file) {
		vp = ump->metadata_file->vnode;
		UDF_SET_SYSTEMFILE(vp);
	}

	icb_loc.loc.lb_num   = pmm->meta_mirror_file_lbn;
	if (icb_loc.loc.lb_num != -1) {
		DPRINTF(VOLUMES, ("Metadata copy file\n"));
		error = udf_get_node(ump, &icb_loc, &ump->metadatamirror_file);
		if (ump->metadatamirror_file) {
			vp = ump->metadatamirror_file->vnode;
			UDF_SET_SYSTEMFILE(vp);
		}
	}

	icb_loc.loc.lb_num   = pmm->meta_bitmap_file_lbn;
	if (icb_loc.loc.lb_num != -1) {
		DPRINTF(VOLUMES, ("Metadata bitmap file\n"));
		error = udf_get_node(ump, &icb_loc, &ump->metadatabitmap_file);
		if (ump->metadatabitmap_file) {
			vp = ump->metadatabitmap_file->vnode;
			UDF_SET_SYSTEMFILE(vp);
		}
	}

	/* if we're mounting read-only we relax the requirements */
	if (ump->vfs_mountp->mnt_flag & MNT_RDONLY) {
		error = EFAULT;
		if (ump->metadata_file)
			error = 0;
		if ((ump->metadata_file == NULL) && (ump->metadatamirror_file)) {
			printf( "udf mount: Metadata file not readable, "
				"substituting Metadata copy file\n");
			ump->metadata_file = ump->metadatamirror_file;
			ump->metadatamirror_file = NULL;
			error = 0;
		}
	} else {
		/* mounting read/write */
		DPRINTF(VOLUMES, ("udf mount: read only file system\n"));
		error = EROFS;
	}
	DPRINTFIF(VOLUMES, error, ("udf mount: failed to read "
				   "metadata files\n"));
	return error;
}
#undef UDF_SET_SYSTEMFILE

/* --------------------------------------------------------------------- */

int
udf_read_vds_tables(struct udf_mount *ump, struct udf_args *args)
{
	union udf_pmap *mapping;
	uint32_t n_pm, mt_l;
	uint32_t log_part;
	uint8_t *pmap_pos;
	int pmap_size;
	int error;

	/* We have to iterate again over the part mappings for locations   */
	n_pm = udf_rw32(ump->logical_vol->n_pm);   /* num partmaps         */
	mt_l = udf_rw32(ump->logical_vol->mt_l);   /* partmaps data length */
	pmap_pos =  ump->logical_vol->maps;

	for (log_part = 0; log_part < n_pm; log_part++) {
		mapping = (union udf_pmap *) pmap_pos;
		switch (ump->vtop_tp[log_part]) {
		case UDF_VTOP_TYPE_PHYS :
			/* nothing */
			break;
		case UDF_VTOP_TYPE_VIRT :
			/* search and load VAT */
			error = udf_search_vat(ump, mapping);
			if (error)
				return ENOENT;
			break;
		case UDF_VTOP_TYPE_SPARABLE :
			/* load one of the sparable tables */
			error = udf_read_sparables(ump, mapping);
			if (error)
				return ENOENT;
			break;
		case UDF_VTOP_TYPE_META :
			/* load the associated file descriptors */
			error = udf_read_metadata_files(ump, mapping);
			if (error)
				return ENOENT;
			break;
		default:
			break;
		}
		pmap_size  = pmap_pos[1];
		pmap_pos  += pmap_size;
	}

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_read_rootdirs(struct udf_mount *ump, struct udf_args *args)
{
	struct udf_node *rootdir_node, *streamdir_node;
	union dscrptr *dscr;
	struct long_ad  fsd_loc, *dir_loc;
	uint32_t lb_num, dummy;
	uint32_t fsd_len;
	int dscr_type;
	int error;

	/* TODO implement FSD reading in separate function like integrity? */
	/* get fileset descriptor sequence */
	fsd_loc = ump->logical_vol->lv_fsd_loc;
	fsd_len = udf_rw32(fsd_loc.len);

	dscr  = NULL;
	error = 0;
	while (fsd_len || error) {
		DPRINTF(VOLUMES, ("fsd_len = %d\n", fsd_len));
		/* translate fsd_loc to lb_num */
		error = udf_translate_vtop(ump, &fsd_loc, &lb_num, &dummy);
		if (error)
			break;
		DPRINTF(VOLUMES, ("Reading FSD at lb %d\n", lb_num));
		error = udf_read_descriptor(ump, lb_num, M_UDFVOLD, &dscr);
		/* end markers */
		if (error || (dscr == NULL))
			break;

		/* analyse */
		dscr_type = udf_rw16(dscr->tag.id);
		if (dscr_type == TAGID_TERM)
			break;
		if (dscr_type != TAGID_FSD) {
			free(dscr, M_UDFVOLD);
			return ENOENT;
		}

		/*
		 * TODO check for multiple fileset descriptors; its only
		 * picking the last now. Also check for FSD
		 * correctness/interpretability
		 */

		/* update */
		if (ump->fileset_desc) {
			free(ump->fileset_desc, M_UDFVOLD);
		}
		ump->fileset_desc = &dscr->fsd;
		dscr = NULL;

		/* continue to the next fsd */
		fsd_len -= ump->discinfo.sector_size;
		fsd_loc.loc.lb_num = udf_rw32(udf_rw32(fsd_loc.loc.lb_num)+1);

		/* follow up to fsd->next_ex (long_ad) if its not null */
		if (udf_rw32(ump->fileset_desc->next_ex.len)) {
			DPRINTF(VOLUMES, ("follow up FSD extent\n"));
			fsd_loc = ump->fileset_desc->next_ex;
			fsd_len = udf_rw32(ump->fileset_desc->next_ex.len);
		}
	}
	if (dscr)
		free(dscr, M_UDFVOLD);

	/* there has to be one */
	if (ump->fileset_desc == NULL)
		return ENOENT;

	DPRINTF(VOLUMES, ("FSD read in fine\n"));

	/*
	 * Now the FSD is known, read in the rootdirectory and if one exists,
	 * the system stream dir. Some files in the system streamdir are not
	 * wanted in this implementation since they are not maintained. If
	 * writing is enabled we'll delete these files if they exist.
	 */

	rootdir_node = streamdir_node = NULL;
	dir_loc = NULL;

	/* try to read in the rootdir */
	dir_loc = &ump->fileset_desc->rootdir_icb;
	error = udf_get_node(ump, dir_loc, &rootdir_node);
	if (error)
		return ENOENT;

	/* aparently it read in fine */

	/*
	 * Try the system stream directory; not very likely in the ones we
	 * test, but for completeness.
	 */
	dir_loc = &ump->fileset_desc->streamdir_icb;
	if (udf_rw32(dir_loc->len)) {
		error = udf_get_node(ump, dir_loc, &streamdir_node);
		if (error)
			printf("udf mount: streamdir defined but ignored\n");
		if (!error) {
			/*
			 * TODO process streamdir `baddies' i.e. files we dont
			 * want if R/W
			 */
		}
	}

	DPRINTF(VOLUMES, ("Rootdir(s) read in fine\n"));

	/* release the vnodes again; they'll be auto-recycled later */
	if (streamdir_node) {
		vput(streamdir_node->vnode);
	}
	if (rootdir_node) {
		vput(rootdir_node->vnode);
	}

	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_translate_vtop(struct udf_mount *ump, struct long_ad *icb_loc,
		   uint32_t *lb_numres, uint32_t *extres)
{
	struct part_desc       *pdesc;
	struct spare_map_entry *sme;
	struct file_entry      *fe;
	struct extfile_entry   *efe;
	struct short_ad        *s_ad;
	struct long_ad         *l_ad;
	uint64_t  cur_offset;
	uint32_t *trans;
	uint32_t  lb_num, plb_num, lb_rel, lb_packet;
	uint32_t  sector_size, len, alloclen;
	uint8_t *pos;
	int rel, vpart, part, addr_type, icblen, icbflags, flags;

	assert(ump && icb_loc && lb_numres);

	vpart  = udf_rw16(icb_loc->loc.part_num);
	lb_num = udf_rw32(icb_loc->loc.lb_num);
	if (vpart < 0 || vpart > UDF_VTOP_RAWPART)
		return EINVAL;

	part = ump->vtop[vpart];
	pdesc = ump->partitions[part];

	switch (ump->vtop_tp[vpart]) {
	case UDF_VTOP_TYPE_RAW :
		/* 1:1 to the end of the device */
		*lb_numres = lb_num;
		*extres = INT_MAX;
		return 0;
	case UDF_VTOP_TYPE_PHYS :
		/* transform into its disc logical block */
		if (lb_num > udf_rw32(pdesc->part_len))
			return EINVAL;
		*lb_numres = lb_num + udf_rw32(pdesc->start_loc);

		/* extent from here to the end of the partition */
		*extres = udf_rw32(pdesc->part_len) - lb_num;
		return 0;
	case UDF_VTOP_TYPE_VIRT :
		/* only maps one sector, lookup in VAT */
		if (lb_num >= ump->vat_entries)		/* XXX > or >= ? */
			return EINVAL;
	
		/* lookup in virtual allocation table */
		trans  = (uint32_t *) (ump->vat_table + ump->vat_offset);
		lb_num = udf_rw32(trans[lb_num]);

		/* transform into its disc logical block */
		if (lb_num > udf_rw32(pdesc->part_len))
			return EINVAL;
		*lb_numres = lb_num + udf_rw32(pdesc->start_loc);

		/* just one logical block */
		*extres = 1;
		return 0;
	case UDF_VTOP_TYPE_SPARABLE :
		/* check if the packet containing the lb_num is remapped */
		lb_packet = lb_num / ump->sparable_packet_len;
		lb_rel    = lb_num % ump->sparable_packet_len;

		for (rel = 0; rel < udf_rw16(ump->sparing_table->rt_l); rel++) {
			sme = &ump->sparing_table->entries[rel];
			if (lb_packet == udf_rw32(sme->org)) {
				/* NOTE maps to absolute disc logical block! */
				*lb_numres = udf_rw32(sme->map) + lb_rel;
				*extres    = ump->sparable_packet_len - lb_rel;
				return 0;
			}
		}

		/* transform into its disc logical block */
		if (lb_num > udf_rw32(pdesc->part_len))
			return EINVAL;
		*lb_numres = lb_num + udf_rw32(pdesc->start_loc);

		/* rest of block */
		*extres = ump->sparable_packet_len - lb_rel;
		return 0;
	case UDF_VTOP_TYPE_META :
		/* we have to look into the file's allocation descriptors */
		/* free after udf_translate_file_extent() */
		/* XXX sector size or lb_size? */
		sector_size = ump->discinfo.sector_size;
		/* XXX should we claim exclusive access to the metafile ? */
		fe  = ump->metadata_file->fe;
		efe = ump->metadata_file->efe;
		if (fe) {
			alloclen = udf_rw32(fe->l_ad);
			pos      = &fe->data[0] + udf_rw32(fe->l_ea);
			icbflags = udf_rw16(fe->icbtag.flags);
		} else {
			assert(efe);
			alloclen = udf_rw32(efe->l_ad);
			pos      = &efe->data[0] + udf_rw32(efe->l_ea);
			icbflags = udf_rw16(efe->icbtag.flags);
		}
		addr_type = icbflags & UDF_ICB_TAG_FLAGS_ALLOC_MASK;

		cur_offset = 0;
		while (alloclen) {
			if (addr_type == UDF_ICB_SHORT_ALLOC) {
				icblen = sizeof(struct short_ad);
				s_ad   = (struct short_ad *) pos;
				len        = udf_rw32(s_ad->len);
				plb_num    = udf_rw32(s_ad->lb_num);
			} else {
				/* should not be present, but why not */
				icblen = sizeof(struct long_ad);
				l_ad   = (struct long_ad *) pos;
				len        = udf_rw32(l_ad->len);
				plb_num    = udf_rw32(l_ad->loc.lb_num);
				/* pvpart_num = udf_rw16(l_ad->loc.part_num); */
			}
			/* process extent */
			flags   = UDF_EXT_FLAGS(len);
			len     = UDF_EXT_LEN(len);

			if (cur_offset + len > lb_num * sector_size) {
				if (flags != UDF_EXT_ALLOCATED)
					return EINVAL;
				lb_rel = lb_num - cur_offset / sector_size;
				/* remainder of this extent */
				*lb_numres = plb_num + lb_rel + 
					udf_rw32(pdesc->start_loc);
				*extres = (len / sector_size) - lb_rel;
				return 0;
			}
			cur_offset += len;
			pos        += icblen;
			alloclen   -= icblen;
		}
		/* not found */
		DPRINTF(TRANSLATE, ("Metadata partition translation failed\n"));
		return EINVAL;
	default:
		printf("UDF vtop translation scheme %d unimplemented yet\n",
			ump->vtop_tp[vpart]);
	}

	return EINVAL;
}

/* --------------------------------------------------------------------- */

/* To make absolutely sure we are NOT returning zero, add one :) */

long
udf_calchash(struct long_ad *icbptr)
{
	/* ought to be enough since each mountpoint has its own chain */
	return udf_rw32(icbptr->loc.lb_num) + 1;
}

/* --------------------------------------------------------------------- */

static struct udf_node *
udf_hashget(struct udf_mount *ump, struct long_ad *icbptr)
{
	struct udf_node *unp;
	struct vnode *vp;
	uint32_t hashline;

loop:
	simple_lock(&ump->ihash_slock);

	hashline = udf_calchash(icbptr) & UDF_INODE_HASHMASK;
	LIST_FOREACH(unp, &ump->udf_nodes[hashline], hashchain) {
		assert(unp);
		if (unp->loc.loc.lb_num   == icbptr->loc.lb_num &&
		    unp->loc.loc.part_num == icbptr->loc.part_num) {
			vp = unp->vnode;
			assert(vp);
			simple_lock(&vp->v_interlock);
			simple_unlock(&ump->ihash_slock);
			if (vget(vp, LK_EXCLUSIVE | LK_INTERLOCK))
				goto loop;
			return unp;
		}
	}
	simple_unlock(&ump->ihash_slock);

	return NULL;
}

/* --------------------------------------------------------------------- */

static void
udf_hashins(struct udf_node *unp)
{
	struct udf_mount *ump;
	uint32_t hashline;

	ump = unp->ump;
	simple_lock(&ump->ihash_slock);

	hashline = udf_calchash(&unp->loc) & UDF_INODE_HASHMASK;
	LIST_INSERT_HEAD(&ump->udf_nodes[hashline], unp, hashchain);

	simple_unlock(&ump->ihash_slock);
}

/* --------------------------------------------------------------------- */

static void
udf_hashrem(struct udf_node *unp) 
{
	struct udf_mount *ump;

	ump = unp->ump;
	simple_lock(&ump->ihash_slock);

	LIST_REMOVE(unp, hashchain);

	simple_unlock(&ump->ihash_slock);
}

/* --------------------------------------------------------------------- */

int
udf_dispose_locked_node(struct udf_node *node)
{
	if (!node)
		return 0;
	if (node->vnode)
		VOP_UNLOCK(node->vnode, 0);
	return udf_dispose_node(node);
}

/* --------------------------------------------------------------------- */

int
udf_dispose_node(struct udf_node *node)
{
	struct vnode *vp;

	DPRINTF(NODE, ("udf_dispose_node called on node %p\n", node));
	if (!node) {
		DPRINTF(NODE, ("UDF: Dispose node on node NULL, ignoring\n"));
		return 0;
	}

	vp  = node->vnode;

	/* TODO extended attributes and streamdir */

	/* remove from our hash lookup table */
	udf_hashrem(node);

	/* destroy genfs structures */
	genfs_node_destroy(vp);

	/* dissociate our udf_node from the vnode */
	vp->v_data = NULL;

	/* free associated memory and the node itself */
	if (node->fe)
		pool_put(node->ump->desc_pool, node->fe);
	if (node->efe)
		pool_put(node->ump->desc_pool, node->efe);
	pool_put(&udf_node_pool, node);

	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * Genfs interfacing
 *
 * static const struct genfs_ops udffs_genfsops = {
 * 	.gop_size = genfs_size,
 * 		size of transfers
 * 	.gop_alloc = udf_gop_alloc,
 * 		unknown
 * 	.gop_write = genfs_gop_write,
 * 		putpages interface code
 * 	.gop_markupdate = udf_gop_markupdate,
 * 		set update/modify flags etc.
 * }
 */

/*
 * Genfs interface. These four functions are the only ones defined though not
 * documented... great.... why is chosen for the `.' initialisers i dont know
 * but other filingsystems seem to use it this way.
 */

static int
udf_gop_alloc(struct vnode *vp, off_t off,
    off_t len, int flags, kauth_cred_t cred)
{
	return 0;
}


static void
udf_gop_markupdate(struct vnode *vp, int flags)
{
	struct udf_node *udf_node = VTOI(vp);
	u_long mask;

	udf_node = udf_node;	/* shut up gcc */

	mask = 0;
#ifdef notyet
	if ((flags & GOP_UPDATE_ACCESSED) != 0) {
		mask = UDF_SET_ACCESS;
	}
	if ((flags & GOP_UPDATE_MODIFIED) != 0) {
		mask |= UDF_SET_UPDATE;
	}
	if (mask) {
		udf_node->update_flag |= mask;
	}
#endif
	/* msdosfs doesn't do it, but shouldn't we update the times here? */
}


static const struct genfs_ops udf_genfsops = {
	.gop_size = genfs_size,
	.gop_alloc = udf_gop_alloc,
	.gop_write = genfs_gop_write,
	.gop_markupdate = udf_gop_markupdate,
};

/* --------------------------------------------------------------------- */

/*
 * Each node can have an attached streamdir node though not
 * recursively. These are otherwise known as named substreams/named
 * extended attributes that have no size limitations.
 *
 * `Normal' extended attributes are indicated with a number and are recorded
 * in either the fe/efe descriptor itself for small descriptors or recorded in
 * the attached extended attribute file. Since this file can get fragmented,
 * care ought to be taken.
 */

int
udf_get_node(struct udf_mount *ump, struct long_ad *node_icb_loc,
	     struct udf_node **noderes)
{
	union dscrptr   *dscr, *tmpdscr;
	struct udf_node *node;
	struct vnode    *nvp;
	struct long_ad   icb_loc;
	extern int (**udf_vnodeop_p)(void *);
	uint64_t file_size;
	uint32_t lb_size, sector, dummy;
	int udf_file_type, dscr_type, strat, strat4096, needs_indirect;
	int error;

	DPRINTF(NODE, ("udf_get_node called\n"));
	*noderes = node = NULL;

	/* lock to disallow simultanious creation of same node */
	lockmgr(&ump->get_node_lock, LK_EXCLUSIVE, NULL);

	DPRINTF(NODE, ("\tlookup in hash table\n"));
	/* lookup in hash table */
	assert(ump);
	assert(node_icb_loc);
	node = udf_hashget(ump, node_icb_loc);
	if (node) {
		DPRINTF(NODE, ("\tgot it from the hash!\n"));
		/* vnode is returned locked */
		*noderes = node;
		lockmgr(&ump->get_node_lock, LK_RELEASE, NULL);
		return 0;
	}

	/* garbage check: translate node_icb_loc to sectornr */
	error = udf_translate_vtop(ump, node_icb_loc, &sector, &dummy);
	if (error) {
		/* no use, this will fail anyway */
		lockmgr(&ump->get_node_lock, LK_RELEASE, NULL);
		return EINVAL;
	}

	/* build node (do initialise!) */
	node = pool_get(&udf_node_pool, PR_WAITOK);
	memset(node, 0, sizeof(struct udf_node));

	DPRINTF(NODE, ("\tget new vnode\n"));
	/* give it a vnode */
	error = getnewvnode(VT_UDF, ump->vfs_mountp, udf_vnodeop_p, &nvp);
        if (error) {
		pool_put(&udf_node_pool, node);
		lockmgr(&ump->get_node_lock, LK_RELEASE, NULL);
		return error;
	}

	/* always return locked vnode */
	if ((error = vn_lock(nvp, LK_EXCLUSIVE | LK_RETRY))) {
		/* recycle vnode and unlock; simultanious will fail too */
		ungetnewvnode(nvp);
		lockmgr(&ump->get_node_lock, LK_RELEASE, NULL);
		return error;
	}

	/* initialise crosslinks, note location of fe/efe for hashing */
	node->ump    =  ump;
	node->vnode  =  nvp;
	nvp->v_data  =  node;
	node->loc    = *node_icb_loc;
	node->lockf  =  0;

	/* insert into the hash lookup */
	udf_hashins(node);

	/* safe to unlock, the entry is in the hash table, vnode is locked */
	lockmgr(&ump->get_node_lock, LK_RELEASE, NULL);

	icb_loc = *node_icb_loc;
	needs_indirect = 0;
	strat4096 = 0;
	udf_file_type = UDF_ICB_FILETYPE_UNKNOWN;
	file_size = 0;
	lb_size = udf_rw32(ump->logical_vol->lb_size);

	do {
		error = udf_translate_vtop(ump, &icb_loc, &sector, &dummy);
		if (error)
			break;

		/* try to read in fe/efe */
		error = udf_read_descriptor(ump, sector, M_UDFTEMP, &tmpdscr);

		/* blank sector marks end of sequence, check this */
		if ((tmpdscr == NULL) &&  (!strat4096))
			error = ENOENT;

		/* break if read error or blank sector */
		if (error || (tmpdscr == NULL))
			break;

		/* process descriptor based on the descriptor type */
		dscr_type = udf_rw16(tmpdscr->tag.id);

		/* if dealing with an indirect entry, follow the link */
		if (dscr_type == TAGID_INDIRECT_ENTRY) {
			needs_indirect = 0;
			icb_loc = tmpdscr->inde.indirect_icb;
			free(tmpdscr, M_UDFTEMP);
			continue;
		}

		/* only file entries and extended file entries allowed here */
		if ((dscr_type != TAGID_FENTRY) &&
		    (dscr_type != TAGID_EXTFENTRY)) {
			free(tmpdscr, M_UDFTEMP);
			error = ENOENT;
			break;
		}

		/* get descriptor space from our pool */
		KASSERT(udf_tagsize(tmpdscr, lb_size) == lb_size);

		dscr = pool_get(ump->desc_pool, PR_WAITOK);
		memcpy(dscr, tmpdscr, lb_size);
		free(tmpdscr, M_UDFTEMP);

		/* record and process/update (ext)fentry */
		if (dscr_type == TAGID_FENTRY) {
			if (node->fe)
				pool_put(ump->desc_pool, node->fe);
			node->fe  = &dscr->fe;
			strat = udf_rw16(node->fe->icbtag.strat_type);
			udf_file_type = node->fe->icbtag.file_type;
			file_size = udf_rw64(node->fe->inf_len);
		} else {
			if (node->efe)
				pool_put(ump->desc_pool, node->efe);
			node->efe = &dscr->efe;
			strat = udf_rw16(node->efe->icbtag.strat_type);
			udf_file_type = node->efe->icbtag.file_type;
			file_size = udf_rw64(node->efe->inf_len);
		}

		/* check recording strategy (structure) */

		/*
		 * Strategy 4096 is a daisy linked chain terminating with an
		 * unrecorded sector or a TERM descriptor. The next
		 * descriptor is to be found in the sector that follows the
		 * current sector.
		 */
		if (strat == 4096) {
			strat4096 = 1;
			needs_indirect = 1;

			icb_loc.loc.lb_num = udf_rw32(icb_loc.loc.lb_num) + 1;
		}

		/*
		 * Strategy 4 is the normal strategy and terminates, but if
		 * we're in strategy 4096, we can't have strategy 4 mixed in
		 */

		if (strat == 4) {
			if (strat4096) {
				error = EINVAL;
				break;
			}
			break;		/* done */
		}
	} while (!error);

	if (error) {
		/* recycle udf_node */
		udf_dispose_node(node);

		/* recycle vnode */
		nvp->v_data = NULL;
		ungetnewvnode(nvp);

		return EINVAL;		/* error code ok? */
	}

	/* post process and initialise node */

	/* assert no references to dscr anymore beyong this point */
	assert((node->fe) || (node->efe));
	dscr = NULL;

	/*
	 * Record where to record an updated version of the descriptor. If
	 * there is a sequence of indirect entries, icb_loc will have been
	 * updated. Its the write disipline to allocate new space and to make
	 * sure the chain is maintained.
	 *
	 * `needs_indirect' flags if the next location is to be filled with
	 * with an indirect entry.
	 */
	node->next_loc = icb_loc;
	node->needs_indirect = needs_indirect;

	/*
	 * Translate UDF filetypes into vnode types.
	 *
	 * Systemfiles like the meta main and mirror files are not treated as
	 * normal files, so we type them as having no type. UDF dictates that
	 * they are not allowed to be visible.
	 */

	/* TODO specfs, fifofs etc etc. vnops setting */
	switch (udf_file_type) {
	case UDF_ICB_FILETYPE_DIRECTORY :
	case UDF_ICB_FILETYPE_STREAMDIR :
		nvp->v_type = VDIR;
		break;
	case UDF_ICB_FILETYPE_BLOCKDEVICE :
		nvp->v_type = VBLK;
		break;
	case UDF_ICB_FILETYPE_CHARDEVICE :
		nvp->v_type = VCHR;
		break;
	case UDF_ICB_FILETYPE_SYMLINK :
		nvp->v_type = VLNK;
		break;
	case UDF_ICB_FILETYPE_VAT :
	case UDF_ICB_FILETYPE_META_MAIN :
	case UDF_ICB_FILETYPE_META_MIRROR :
		nvp->v_type = VNON;
		break;
	case UDF_ICB_FILETYPE_RANDOMACCESS :
	case UDF_ICB_FILETYPE_REALTIME :
		nvp->v_type = VREG;
		break;
	default:
		/* YIKES, either a block/char device, fifo or something else */
		nvp->v_type = VNON;
	}

	/* initialise genfs */
	genfs_node_init(nvp, &udf_genfsops);

	/* don't forget to set vnode's v_size */
	uvm_vnp_setsize(nvp, file_size);

	/* TODO ext attr and streamdir nodes */

	*noderes = node;

	return 0;
}

/* --------------------------------------------------------------------- */

/* UDF<->unix converters */

/* --------------------------------------------------------------------- */

static mode_t
udf_perm_to_unix_mode(uint32_t perm)
{
	mode_t mode;

	mode  = ((perm & UDF_FENTRY_PERM_USER_MASK)      );
	mode |= ((perm & UDF_FENTRY_PERM_GRP_MASK  ) >> 2);
	mode |= ((perm & UDF_FENTRY_PERM_OWNER_MASK) >> 4);

	return mode;
}

/* --------------------------------------------------------------------- */

#ifdef notyet
static uint32_t
unix_mode_to_udf_perm(mode_t mode)
{
	uint32_t perm;
	
	perm  = ((mode & S_IRWXO)     );
	perm |= ((mode & S_IRWXG) << 2);
	perm |= ((mode & S_IRWXU) << 4);
	perm |= ((mode & S_IWOTH) << 3);
	perm |= ((mode & S_IWGRP) << 5);
	perm |= ((mode & S_IWUSR) << 7);

	return perm;
}
#endif

/* --------------------------------------------------------------------- */

static uint32_t
udf_icb_to_unix_filetype(uint32_t icbftype)
{
	switch (icbftype) {
	case UDF_ICB_FILETYPE_DIRECTORY :
	case UDF_ICB_FILETYPE_STREAMDIR :
		return S_IFDIR;
	case UDF_ICB_FILETYPE_FIFO :
		return S_IFIFO;
	case UDF_ICB_FILETYPE_CHARDEVICE :
		return S_IFCHR;
	case UDF_ICB_FILETYPE_BLOCKDEVICE :
		return S_IFBLK;
	case UDF_ICB_FILETYPE_RANDOMACCESS :
	case UDF_ICB_FILETYPE_REALTIME :
		return S_IFREG;
	case UDF_ICB_FILETYPE_SYMLINK :
		return S_IFLNK;
	case UDF_ICB_FILETYPE_SOCKET :
		return S_IFSOCK;
	}
	/* no idea what this is */
	return 0;
}

/* --------------------------------------------------------------------- */

/* TODO KNF-ify */

void
udf_to_unix_name(char *result, char *id, int len, struct charspec *chsp)
{
	uint16_t *raw_name, *unix_name;
	uint16_t *inchp, ch;
	uint8_t	 *outchp;
	int       ucode_chars, nice_uchars;

	raw_name = malloc(2048 * sizeof(uint16_t), M_UDFTEMP, M_WAITOK);
	unix_name = raw_name + 1024;			/* split space in half */
	assert(sizeof(char) == sizeof(uint8_t));
	outchp = (uint8_t *) result;
	if ((chsp->type == 0) && (strcmp((char*) chsp->inf, "OSTA Compressed Unicode") == 0)) {
		*raw_name = *unix_name = 0;
		ucode_chars = udf_UncompressUnicode(len, (uint8_t *) id, raw_name);
		ucode_chars = MIN(ucode_chars, UnicodeLength((unicode_t *) raw_name));
		nice_uchars = UDFTransName(unix_name, raw_name, ucode_chars);
		for (inchp = unix_name; nice_uchars>0; inchp++, nice_uchars--) {
			ch = *inchp;
			/* XXX sloppy unicode -> latin */
			*outchp++ = ch & 255;
			if (!ch) break;
		}
		*outchp++ = 0;
	} else {
		/* assume 8bit char length byte latin-1 */
		assert(*id == 8);
		strncpy((char *) result, (char *) (id+1), strlen((char *) (id+1)));
	}
	free(raw_name, M_UDFTEMP);
}

/* --------------------------------------------------------------------- */

/* TODO KNF-ify */

void
unix_to_udf_name(char *result, char *name,
		 uint8_t *result_len, struct charspec *chsp)
{
	uint16_t *raw_name;
	int       udf_chars, name_len;
	char     *inchp;
	uint16_t *outchp;

	raw_name = malloc(1024, M_UDFTEMP, M_WAITOK);
	/* convert latin-1 or whatever to unicode-16 */
	*raw_name = 0;
	name_len  = 0;
	inchp  = name;
	outchp = raw_name;
	while (*inchp) {
		*outchp++ = (uint16_t) (*inchp++);
		name_len++;
	}

	if ((chsp->type == 0) && (strcmp((char *) chsp->inf, "OSTA Compressed Unicode") == 0)) {
		udf_chars = udf_CompressUnicode(name_len, 8, (unicode_t *) raw_name, (byte *) result);
	} else {
		/* XXX assume 8bit char length byte latin-1 */
		*result++ = 8; udf_chars = 1;
		strncpy(result, name + 1, strlen(name+1));
		udf_chars += strlen(name);
	}
	*result_len = udf_chars;
	free(raw_name, M_UDFTEMP);
}

/* --------------------------------------------------------------------- */

void
udf_timestamp_to_timespec(struct udf_mount *ump,
			  struct timestamp *timestamp,
			  struct timespec  *timespec)
{
	struct clock_ymdhms ymdhms;
	uint32_t usecs, secs, nsecs;
	uint16_t tz;

	/* fill in ymdhms structure from timestamp */
	memset(&ymdhms, 0, sizeof(ymdhms));
	ymdhms.dt_year = udf_rw16(timestamp->year);
	ymdhms.dt_mon  = timestamp->month;
	ymdhms.dt_day  = timestamp->day;
	ymdhms.dt_wday = 0; /* ? */
	ymdhms.dt_hour = timestamp->hour;
	ymdhms.dt_min  = timestamp->minute;
	ymdhms.dt_sec  = timestamp->second;

	secs = clock_ymdhms_to_secs(&ymdhms);
	usecs = timestamp->usec +
		100*timestamp->hund_usec + 10000*timestamp->centisec;
	nsecs = usecs * 1000;

	/*
	 * Calculate the time zone.  The timezone is 12 bit signed 2's
	 * compliment, so we gotta do some extra magic to handle it right.
	 */
	tz  = udf_rw16(timestamp->type_tz);
	tz &= 0x0fff;			/* only lower 12 bits are significant */
	if (tz & 0x0800)		/* sign extention */
		tz |= 0xf000;

	/* TODO check timezone conversion */
	/* check if we are specified a timezone to convert */
	if (udf_rw16(timestamp->type_tz) & 0x1000) {
		if ((int16_t) tz != -2047)
			secs -= (int16_t) tz * 60;
	} else {
		secs -= ump->mount_args.gmtoff;
	}

	timespec->tv_sec  = secs;
	timespec->tv_nsec = nsecs;
}

/* --------------------------------------------------------------------- */

/*
 * Attribute and filetypes converters with get/set pairs
 */

uint32_t
udf_getaccessmode(struct udf_node *udf_node)
{
	struct file_entry *fe;
	struct extfile_entry *efe;
	uint32_t udf_perm, icbftype;
	uint32_t mode, ftype;
	uint16_t icbflags;

	if (udf_node->fe) {
		fe = udf_node->fe;
		udf_perm = udf_rw32(fe->perm);
		icbftype = fe->icbtag.file_type;
		icbflags = udf_rw16(fe->icbtag.flags);
	} else {
		assert(udf_node->efe);
		efe = udf_node->efe;
		udf_perm = udf_rw32(efe->perm);
		icbftype = efe->icbtag.file_type;
		icbflags = udf_rw16(efe->icbtag.flags);
	}

	mode  = udf_perm_to_unix_mode(udf_perm);
	ftype = udf_icb_to_unix_filetype(icbftype);

	/* set suid, sgid, sticky from flags in fe/efe */
	if (icbflags & UDF_ICB_TAG_FLAGS_SETUID)
		mode |= S_ISUID;
	if (icbflags & UDF_ICB_TAG_FLAGS_SETGID)
		mode |= S_ISGID;
	if (icbflags & UDF_ICB_TAG_FLAGS_STICKY)
		mode |= S_ISVTX;

	return mode | ftype;
}

/* --------------------------------------------------------------------- */

/*
 * Directory read and manipulation functions
 */

int 
udf_lookup_name_in_dir(struct vnode *vp, const char *name, int namelen,
		       struct long_ad *icb_loc)
{
	struct udf_node  *dir_node = VTOI(vp);
	struct file_entry    *fe;
	struct extfile_entry *efe;
	struct fileid_desc *fid;
	struct dirent dirent;
	uint64_t file_size, diroffset;
	uint32_t lb_size;
	int found, error;

	/* get directory filesize */
	if (dir_node->fe) {
		fe = dir_node->fe;
		file_size = udf_rw64(fe->inf_len);
	} else {
		assert(dir_node->efe);
		efe = dir_node->efe;
		file_size = udf_rw64(efe->inf_len);
	}

	/* allocate temporary space for fid */
	lb_size = udf_rw32(dir_node->ump->logical_vol->lb_size);
	fid = malloc(lb_size, M_TEMP, M_WAITOK);

	found = 0;
	diroffset = dir_node->last_diroffset;

	/*
	 * if the directory is trunced or if we have never visited it yet,
	 * start at the end.
	 */
	if ((diroffset >= file_size) || (diroffset == 0)) {
		diroffset = dir_node->last_diroffset = file_size;
	}

	while (!found) {
		/* if at the end, go trough zero */
		if (diroffset >= file_size)
			diroffset = 0;

		/* transfer a new fid/dirent */
		error = udf_read_fid_stream(vp, &diroffset, fid, &dirent);
		if (error)
			break;

		/* skip deleted entries */
		if ((fid->file_char & UDF_FILE_CHAR_DEL) == 0) {
			if ((strlen(dirent.d_name) == namelen) &&
			    (strncmp(dirent.d_name, name, namelen) == 0)) {
				found = 1;
				*icb_loc = fid->icb;
			}
		}

		if (diroffset == dir_node->last_diroffset) {
			/* we have cycled */
			break;
		}
	}
	free(fid, M_TEMP);
	dir_node->last_diroffset = diroffset;

	return found;
}

/* --------------------------------------------------------------------- */

/*
 * Read one fid and process it into a dirent and advance to the next (*fid)
 * has to be allocated a logical block in size, (*dirent) struct dirent length
 */

int
udf_read_fid_stream(struct vnode *vp, uint64_t *offset,
		    struct fileid_desc *fid, struct dirent *dirent)
{
	struct udf_node  *dir_node = VTOI(vp);
	struct udf_mount *ump = dir_node->ump;
	struct file_entry    *fe;
	struct extfile_entry *efe;
	struct uio    dir_uio;
	struct iovec  dir_iovec;
	uint32_t      entry_length, lb_size;
	uint64_t      file_size;
	char         *fid_name;
	int           enough, error;

	assert(fid);
	assert(dirent);
	assert(dir_node);
	assert(offset);
	assert(*offset != 1);

	DPRINTF(FIDS, ("read_fid_stream called\n"));
	/* check if we're past the end of the directory */
	if (dir_node->fe) {
		fe = dir_node->fe;
		file_size = udf_rw64(fe->inf_len);
	} else {
		assert(dir_node->efe);
		efe = dir_node->efe;
		file_size = udf_rw64(efe->inf_len);
	}
	if (*offset >= file_size)
		return EINVAL;

	/* get maximum length of FID descriptor */
	lb_size = udf_rw32(ump->logical_vol->lb_size);

	/* initialise return values */
	entry_length = 0;
	memset(dirent, 0, sizeof(struct dirent));
	memset(fid, 0, lb_size);

	/* TODO use vn_rdwr instead of creating our own uio */
	/* read part of the directory */
	memset(&dir_uio, 0, sizeof(struct uio));
	dir_uio.uio_rw     = UIO_READ;	/* read into this space */
	dir_uio.uio_iovcnt = 1;
	dir_uio.uio_iov    = &dir_iovec;
	UIO_SETUP_SYSSPACE(&dir_uio);
	dir_iovec.iov_base = fid;
	dir_iovec.iov_len  = lb_size;
	dir_uio.uio_offset = *offset;

	/* limit length of read in piece */
	dir_uio.uio_resid  = MIN(file_size - (*offset), lb_size);

	/* read the part into the fid space */
	error = VOP_READ(vp, &dir_uio, IO_ALTSEMANTICS, NOCRED);
	if (error)
		return error;

	/*
	 * Check if we got a whole descriptor.
	 * XXX Try to `resync' directory stream when something is very wrong.
	 *
	 */
	enough = (dir_uio.uio_offset - (*offset) >= UDF_FID_SIZE);
	if (!enough) {
		/* short dir ... */
		return EIO;
	}

	/* check if our FID header is OK */
	error = udf_check_tag(fid);
	DPRINTFIF(FIDS, error, ("read fids: tag check failed\n"));
	if (!error) {
		if (udf_rw16(fid->tag.id) != TAGID_FID)
			error = ENOENT;
	}
	DPRINTFIF(FIDS, !error, ("\ttag checked ok: got TAGID_FID\n"));

	/* check for length */
	if (!error) {
		entry_length = udf_fidsize(fid, lb_size);
		enough = (dir_uio.uio_offset - (*offset) >= entry_length);
	}
	DPRINTFIF(FIDS, !error, ("\tentry_length = %d, enough = %s\n",
	    entry_length, enough?"yes":"no"));

	if (!enough) {
		/* short dir ... bomb out */
		return EIO;
	}

	/* check FID contents */
	if (!error) {
		error = udf_check_tag_payload((union dscrptr *) fid, lb_size);
		DPRINTF(FIDS, ("\tpayload checked ok\n"));
	}
	if (error) {
		/* note that is sometimes a bit quick to report */
		printf("BROKEN DIRECTORY ENTRY\n");
		/* RESYNC? */
		/* TODO: use udf_resync_fid_stream */
		return EIO;
	}
	DPRINTF(FIDS, ("\tinterpret FID\n"));

	/* we got a whole and valid descriptor! */

	/* create resulting dirent structure */
	fid_name = (char *) fid->data + udf_rw16(fid->l_iu);
	udf_to_unix_name(dirent->d_name,
		fid_name, fid->l_fi, &ump->logical_vol->desc_charset);

	/* '..' has no name, so provide one */
	if (fid->file_char & UDF_FILE_CHAR_PAR)
		strcpy(dirent->d_name, "..");

	dirent->d_fileno = udf_calchash(&fid->icb);	/* inode hash XXX */
	dirent->d_namlen = strlen(dirent->d_name);
	dirent->d_reclen = _DIRENT_SIZE(dirent);

	/*
	 * Note that its not worth trying to go for the filetypes now... its
	 * too expensive too
	 */
	dirent->d_type = DT_UNKNOWN;

	/* initial guess for filetype we can make */
	if (fid->file_char & UDF_FILE_CHAR_DIR)
		dirent->d_type = DT_DIR;

	/* advance */
	*offset += entry_length;

	return error;
}

/* --------------------------------------------------------------------- */

/*
 * block based file reading and writing
 */

static int
udf_read_internal(struct udf_node *node, uint8_t *blob)
{
	struct udf_mount *ump;
	struct file_entry *fe;
	struct extfile_entry *efe;
	uint64_t inflen;
	uint32_t sector_size;
	uint8_t  *pos;
	int icbflags, addr_type;

	/* shut up gcc */
	inflen = addr_type = icbflags = 0;
	pos = NULL;

	/* get extent and do some paranoia checks */
	ump = node->ump;
	sector_size = ump->discinfo.sector_size;

	fe  = node->fe;
	efe = node->efe;
	if (fe) {
		inflen   = udf_rw64(fe->inf_len);
		pos      = &fe->data[0] + udf_rw32(fe->l_ea);
		icbflags = udf_rw16(fe->icbtag.flags);
	}
	if (efe) {
		inflen   = udf_rw64(efe->inf_len);
		pos      = &efe->data[0] + udf_rw32(efe->l_ea);
		icbflags = udf_rw16(efe->icbtag.flags);
	}
	addr_type = icbflags & UDF_ICB_TAG_FLAGS_ALLOC_MASK;

	assert(addr_type == UDF_ICB_INTERN_ALLOC);
	assert(inflen < sector_size);

	/* copy out info */
	memset(blob, 0, sector_size);
	memcpy(blob, pos, inflen);

	return 0;
}

/* --------------------------------------------------------------------- */

/*
 * Read file extent reads an extent specified in sectors from the file. It is
 * sector based; i.e. no `fancy' offsets.
 */

int
udf_read_file_extent(struct udf_node *node,
		     uint32_t from, uint32_t sectors,
		     uint8_t *blob)
{
	struct buf buf;
	uint32_t sector_size;

	BUF_INIT(&buf);

	sector_size = node->ump->discinfo.sector_size;

	buf.b_bufsize = sectors * sector_size;
	buf.b_data    = blob;
	buf.b_bcount  = buf.b_bufsize;
	buf.b_resid   = buf.b_bcount;
	buf.b_flags   = B_BUSY | B_READ;
	buf.b_vp      = node->vnode;
	buf.b_proc    = NULL;

	buf.b_blkno  = from;
	buf.b_lblkno = 0;
	BIO_SETPRIO(&buf, BPRIO_TIMELIMITED);

	udf_read_filebuf(node, &buf);
	return biowait(&buf);
}


/* --------------------------------------------------------------------- */

/*
 * Read file extent in the buffer.
 *
 * The splitup of the extent into separate request-buffers is to minimise
 * copying around as much as possible.
 */


/* maximum of 128 translations (!) (64 kb in 512 byte sectors) */
#define FILEBUFSECT 128

void
udf_read_filebuf(struct udf_node *node, struct buf *buf)
{
	struct buf *nestbuf;
	uint64_t   *mapping;
	uint64_t    run_start;
	uint32_t    sector_size;
	uint32_t    buf_offset, sector, rbuflen, rblk;
	uint8_t    *buf_pos;
	int error, run_length;

	uint32_t  from;
	uint32_t  sectors;

	sector_size = node->ump->discinfo.sector_size;

	from    = buf->b_blkno;
	sectors = buf->b_bcount / sector_size;

	/* assure we have enough translation slots */
	KASSERT(buf->b_bcount / sector_size <= FILEBUFSECT);
	KASSERT(MAXPHYS / sector_size <= FILEBUFSECT);

	if (sectors > FILEBUFSECT) {
		printf("udf_read_filebuf: implementation limit on bufsize\n");
		buf->b_error  = EIO;
		biodone(buf);
		return;
	}

	mapping = malloc(sizeof(*mapping) * FILEBUFSECT, M_TEMP, M_WAITOK);

	error = 0;
	DPRINTF(READ, ("\ttranslate %d-%d\n", from, sectors));
	error = udf_translate_file_extent(node, from, sectors, mapping);
	if (error) {
		buf->b_error  = error;
		biodone(buf);
		goto out;
	}
	DPRINTF(READ, ("\ttranslate extent went OK\n"));

	/* pre-check if internal or parts are zero */
	if (*mapping == UDF_TRANS_INTERN) {
		error = udf_read_internal(node, (uint8_t *) buf->b_data);
		if (error) {
			buf->b_error  = error;
		}
		biodone(buf);
		goto out;
	}
	DPRINTF(READ, ("\tnot intern\n"));

	/* request read-in of data from disc scheduler */
	buf->b_resid = buf->b_bcount;
	for (sector = 0; sector < sectors; sector++) {
		buf_offset = sector * sector_size;
		buf_pos    = (uint8_t *) buf->b_data + buf_offset;
		DPRINTF(READ, ("\tprocessing rel sector %d\n", sector));

		switch (mapping[sector]) {
		case UDF_TRANS_UNMAPPED:
		case UDF_TRANS_ZERO:
			/* copy zero sector */
			memset(buf_pos, 0, sector_size);
			DPRINTF(READ, ("\treturning zero sector\n"));
			nestiobuf_done(buf, sector_size, 0);
			break;
		default :
			DPRINTF(READ, ("\tread sector "
			    "%"PRIu64"\n", mapping[sector]));

			run_start  = mapping[sector];
			run_length = 1;
			while (sector < sectors-1) {
				if (mapping[sector+1] != mapping[sector]+1)
					break;
				run_length++;
				sector++;
			}

			/*
			 * nest an iobuf and mark it for async reading. Since
			 * we're using nested buffers, they can't be cached by
			 * design.
			 */
			rbuflen = run_length * sector_size;
			rblk    = run_start  * (sector_size/DEV_BSIZE);

			nestbuf = getiobuf();
			nestiobuf_setup(buf, nestbuf, buf_offset, rbuflen);
			/* nestbuf is B_ASYNC */

			/* CD schedules on raw blkno */
			nestbuf->b_blkno    = rblk;
			nestbuf->b_proc     = NULL;
			nestbuf->b_cylinder = 0;
			nestbuf->b_rawblkno = rblk;
			VOP_STRATEGY(node->ump->devvp, nestbuf);
		}
	}
out:
	DPRINTF(READ, ("\tend of read_filebuf\n"));
	free(mapping, M_TEMP);
	return;
}
#undef FILEBUFSECT


/* --------------------------------------------------------------------- */

/*
 * Translate an extent (in sectors) into sector numbers; used for read and
 * write operations. DOESNT't check extents.
 */

int
udf_translate_file_extent(struct udf_node *node,
		          uint32_t from, uint32_t pages,
			  uint64_t *map)
{
	struct udf_mount *ump;
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct short_ad *s_ad;
	struct long_ad  *l_ad, t_ad;
	uint64_t transsec;
	uint32_t sector_size, transsec32;
	uint32_t overlap, translen;
	uint32_t vpart_num, lb_num, len, alloclen;
	uint8_t *pos;
	int error, flags, addr_type, icblen, icbflags;

	if (!node)
		return ENOENT;

	/* shut up gcc */
	alloclen = addr_type = icbflags = 0;
	pos = NULL;

	/* do the work */
	ump = node->ump;
	sector_size = ump->discinfo.sector_size;
	fe  = node->fe;
	efe = node->efe;
	if (fe) {
		alloclen = udf_rw32(fe->l_ad);
		pos      = &fe->data[0] + udf_rw32(fe->l_ea);
		icbflags = udf_rw16(fe->icbtag.flags);
	}
	if (efe) {
		alloclen = udf_rw32(efe->l_ad);
		pos      = &efe->data[0] + udf_rw32(efe->l_ea);
		icbflags = udf_rw16(efe->icbtag.flags);
	}
	addr_type = icbflags & UDF_ICB_TAG_FLAGS_ALLOC_MASK;

	DPRINTF(TRANSLATE, ("udf trans: alloc_len = %d, addr_type %d, "
	    "fe %p, efe %p\n", alloclen, addr_type, fe, efe));

	vpart_num = udf_rw16(node->loc.loc.part_num);
	lb_num = len = icblen = 0;	/* shut up gcc */
	while (pages && alloclen) {
		DPRINTF(TRANSLATE, ("\taddr_type %d\n", addr_type));
		switch (addr_type) {
		case UDF_ICB_INTERN_ALLOC :
			/* TODO check extents? */
			*map = UDF_TRANS_INTERN;
			return 0;
		case UDF_ICB_SHORT_ALLOC :
			icblen = sizeof(struct short_ad);
			s_ad   = (struct short_ad *) pos;
			len       = udf_rw32(s_ad->len);
			lb_num    = udf_rw32(s_ad->lb_num);
			break;
		case UDF_ICB_LONG_ALLOC  :
			icblen = sizeof(struct long_ad);
			l_ad   = (struct long_ad *) pos;
			len       = udf_rw32(l_ad->len);
			lb_num    = udf_rw32(l_ad->loc.lb_num);
			vpart_num = udf_rw16(l_ad->loc.part_num);
			DPRINTFIF(TRANSLATE,
			    (l_ad->impl.im_used.flags &
			     UDF_ADIMP_FLAGS_EXTENT_ERASED),
			    ("UDF: got an `extent erased' flag in long_ad\n"));
			break;
		default:
			/* can't be here */
			return EINVAL;	/* for sure */
		}

		/* process extent */
		flags   = UDF_EXT_FLAGS(len);
		len     = UDF_EXT_LEN(len);

		overlap = (len + sector_size -1) / sector_size;
		if (from) {
			if (from > overlap) {
				from -= overlap;
				overlap = 0;
			} else {
				lb_num  += from;	/* advance in extent */
				overlap -= from;
				from = 0;
			}
		}

		overlap = MIN(overlap, pages);
		while (overlap) {
			switch (flags) {
			case UDF_EXT_REDIRECT :
				/* no support for allocation extentions yet */
				/* TODO support for allocation extention */
				return ENOENT;
			case UDF_EXT_FREED :
			case UDF_EXT_FREE :
				transsec = UDF_TRANS_ZERO;
				translen = overlap;
				while (overlap && pages && translen) {
					*map++ = transsec;
					lb_num++;
					overlap--; pages--; translen--;
				}
				break;
			case UDF_EXT_ALLOCATED :
				t_ad.loc.lb_num   = udf_rw32(lb_num);
				t_ad.loc.part_num = udf_rw16(vpart_num);
				error = udf_translate_vtop(ump,
						&t_ad, &transsec32, &translen);
				transsec = transsec32;
				if (error)
					return error;
				while (overlap && pages && translen) {
					*map++ = transsec;
					lb_num++; transsec++;
					overlap--; pages--; translen--;
				}
				break;
			}
		}
		pos      += icblen;
		alloclen -= icblen;
	}
	return 0;
}

/* --------------------------------------------------------------------- */

