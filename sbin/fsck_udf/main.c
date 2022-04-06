/*	$NetBSD: main.c,v 1.1 2022/04/06 13:35:50 reinoud Exp $	*/

/*
 * Copyright (c) 2022 Reinoud Zandijk
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
 * Note to reader:
 *
 * fsck_udf uses the common udf_core.c file with newfs and makefs. It does use
 * some of the layout structure values but not all.
 */


#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.1 2022/04/06 13:35:50 reinoud Exp $");
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dirent.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>
#include <time.h>
#include <tzfile.h>
#include <math.h>
#include <assert.h>
#include <err.h>

#if !HAVE_NBTOOL_CONFIG_H
#define _EXPOSE_MMC
#include <sys/cdio.h>
#else
#include "udf/cdio_mmc_structs.h"
#endif

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/param.h>
#include <sys/queue.h>

#include <fs/udf/ecma167-udf.h>
#include <fs/udf/udf_mount.h>

#include "fsutil.h"
#include "exitvalues.h"
#include "udf_core.h"

/* Identifying myself */
#define IMPL_NAME		"*NetBSD fsck_udf 10.0"
#define APP_VERSION_MAIN	0
#define APP_VERSION_SUB		5

/* allocation walker actions */
#define AD_LOAD_FILE		(1<<0)
#define AD_SAVE_FILE		(1<<1)
#define AD_CHECK_FIDS		(1<<2)
#define AD_ADJUST_FIDS		(1<<3)
#define AD_GATHER_STATS		(1<<4)
#define AD_CHECK_USED		(1<<5)
#define AD_MARK_AS_USED		(1<<6)
#define AD_FIND_OVERLAP_PAIR	(1<<7)

struct udf_fsck_file_stats {
	uint64_t inf_len;
	uint64_t obj_size;
	uint64_t logblks_rec;
};


struct udf_fsck_fid_context {
	uint64_t fid_offset;
	uint64_t data_left;
};


/* basic node administration for passes */
#define FSCK_NODE_FLAG_HARDLINK		(1<< 0)	/* hardlink, for accounting */
#define FSCK_NODE_FLAG_DIRECTORY	(1<< 1)	/* is a normal directory */
#define FSCK_NODE_FLAG_HAS_STREAM_DIR	(1<< 2)	/* has a stream directory */
#define FSCK_NODE_FLAG_STREAM_ENTRY	(1<< 3)	/* is a stream file */
#define FSCK_NODE_FLAG_STREAM_DIR	(1<< 4)	/* is a stream directory */
#define FSCK_NODE_FLAG_OK(f)		(((f) >> 5) == 0)

#define FSCK_NODE_FLAG_KEEP		(1<< 5)	/* don't discard */
#define FSCK_NODE_FLAG_DIRTY		(1<< 6)	/* descriptor needs writeout */
#define FSCK_NODE_FLAG_REPAIRDIR	(1<< 7)	/* repair bad FID entries */
#define FSCK_NODE_FLAG_NEW_UNIQUE_ID	(1<< 8)	/* repair bad FID entries */
#define FSCK_NODE_FLAG_COPY_PARENT_ID	(1<< 9)	/* repair bad FID entries */
#define FSCK_NODE_FLAG_WIPE_STREAM_DIR	(1<<10)	/* wipe stream directory */
#define FSCK_NODE_FLAG_NOTFOUND		(1<<11)	/* FID pointing to garbage */
#define FSCK_NODE_FLAG_PAR_NOT_FOUND	(1<<12)	/* parent node not found! */
#define FSCK_NODE_FLAG_OVERLAP		(1<<13) /* node has overlaps */

#define FSCK_NODE_FLAG_STREAM (FSCK_NODE_FLAG_STREAM_ENTRY | FSCK_NODE_FLAG_STREAM_DIR)


#define	HASH_HASHBITS	5
#define	HASH_HASHSIZE	(1 << HASH_HASHBITS)
#define	HASH_HASHMASK	(HASH_HASHSIZE - 1)

/* fsck node for accounting checks */
struct udf_fsck_node {
	struct udf_fsck_node *parent;
	char *fname;

	struct long_ad	loc;
	struct long_ad	streamdir_loc;
	int		fsck_flags;

	int		link_count;
	int		found_link_count;
	uint64_t	unique_id;

	struct udf_fsck_file_stats declared;
	struct udf_fsck_file_stats found;

	uint8_t		*directory;		/* directory contents */

	LIST_ENTRY(udf_fsck_node) next_hash;
	TAILQ_ENTRY(udf_fsck_node) next;
};
TAILQ_HEAD(udf_fsck_node_list, udf_fsck_node) fs_nodes;
LIST_HEAD(udf_fsck_node_hash_list, udf_fsck_node) fs_nodes_hash[HASH_HASHSIZE];


/* fsck used space bitmap conflict list */
#define FSCK_OVERLAP_MAIN_NODE	(1<<0)
#define FSCK_OVERLAP_EXTALLOC	(1<<1)
#define FSCK_OVERLAP_EXTENT	(1<<2)

struct udf_fsck_overlap {
	struct udf_fsck_node *node;
	struct udf_fsck_node *node2;

	struct long_ad	loc;
	struct long_ad	loc2;

	int		flags;
	int		flags2;

	TAILQ_ENTRY(udf_fsck_overlap) next;
};
TAILQ_HEAD(udf_fsck_overlap_list, udf_fsck_overlap) fsck_overlaps;


/* backup of old read in free space bitmaps */
struct space_bitmap_desc *recorded_part_unalloc_bits[UDF_PARTITIONS];
uint32_t recorded_part_free[UDF_PARTITIONS];

/* shadow VAT build */
uint8_t *shadow_vat_contents;


/* options */
int alwaysno = 0;		/* assume "no" for all questions */
int alwaysyes = 0;		/* assume "yes" for all questions */
int search_older_vat = 0;	/* search for older VATs */
int force = 0;			/* do check even if its marked clean */
int preen = 0;			/* set when preening, doing automatic small repairs */
int rdonly = 0;			/* open device/image read-only */
int rdonly_flag = 0;		/* as passed on command line */
int heuristics = 0;		/* use heuristics to fix esoteric corruptions */
int target_session = 0;		/* offset to last session to check */


/* actions to undertake */
int undo_opening_session = 0;	/* trying to undo opening of last crippled session */
int open_integrity = 0;		/* should be open the integrity ie close later */
int vat_writeout = 0;		/* write out the VAT anyway */


/* SIGINFO */
static sig_atomic_t print_info = 0;		/* request for information on progress */


/* prototypes */
static void usage(void) __dead;
static int checkfilesys(char *given_dev);
static int ask(int def, const char *fmt, ...);
static int ask_noauto(int def, const char *fmt, ...);

static void udf_recursive_keep(struct udf_fsck_node *node);
static char *udf_node_path(struct udf_fsck_node *node);
static void udf_shadow_VAT_in_use(struct long_ad *loc);
static int udf_quick_check_fids(struct udf_fsck_node *node, union dscrptr *dscr);


/* --------------------------------------------------------------------- */

/* from bin/ls */
static void
printtime(time_t ftime)
{
	struct timespec clock;
        const char *longstring;
	time_t now;
        int i;

	clock_gettime(CLOCK_REALTIME, &clock);
	now = clock.tv_sec;

        if ((longstring = ctime(&ftime)) == NULL) {
                           /* 012345678901234567890123 */
                longstring = "????????????????????????";
        }
        for (i = 4; i < 11; ++i)
                (void)putchar(longstring[i]);

#define SIXMONTHS       ((DAYSPERNYEAR / 2) * SECSPERDAY)
        if (ftime + SIXMONTHS > now && ftime - SIXMONTHS < now)
                for (i = 11; i < 16; ++i)
                        (void)putchar(longstring[i]);
        else {
                (void)putchar(' ');
                for (i = 20; i < 24; ++i)
                        (void)putchar(longstring[i]);
        }
        (void)putchar(' ');
}


static void
udf_print_timestamp(const char *prefix, struct timestamp *timestamp, const char *suffix)
{
	struct timespec timespec;

	udf_timestamp_to_timespec(timestamp, &timespec);
	printf("%s", prefix);
	printtime(timespec.tv_sec);
	printf("%s", suffix);
}


static int
udf_compare_mtimes(struct timestamp *t1, struct timestamp *t2)
{
	struct timespec t1_tsp, t2_tsp;

	udf_timestamp_to_timespec(t1, &t1_tsp);
	udf_timestamp_to_timespec(t2, &t2_tsp);

	if (t1_tsp.tv_sec  < t2_tsp.tv_sec)
		return -1;
	if (t1_tsp.tv_sec  > t2_tsp.tv_sec)
		return  1;
	if (t1_tsp.tv_nsec < t2_tsp.tv_nsec)
		return -1;
	if (t1_tsp.tv_nsec > t2_tsp.tv_nsec)
		return  1;
	return 0;
}

/* --------------------------------------------------------------------- */

static int
udf_calc_node_hash(struct long_ad *icb)
{
	uint32_t lb_num = udf_rw32(icb->loc.lb_num);
	uint16_t vpart  = udf_rw16(icb->loc.part_num);

	return ((uint64_t) (vpart + lb_num * 257)) & HASH_HASHMASK;
}


static struct udf_fsck_node *
udf_node_lookup(struct long_ad *icb)
{
	struct udf_fsck_node *pos;
	int entry = udf_calc_node_hash(icb);

	pos = LIST_FIRST(&fs_nodes_hash[entry]);
	while (pos) {
		if (pos->loc.loc.part_num == icb->loc.part_num)
			if (pos->loc.loc.lb_num == icb->loc.lb_num)
				return pos;
		pos = LIST_NEXT(pos, next_hash);
	}
	return NULL;
}

/* --------------------------------------------------------------------- */

/* Note: only for VAT media since we don't allocate in bitmap */
static void
udf_wipe_and_reallocate(union dscrptr *dscrptr, int vpart_num, uint32_t *l_adp)
{
	struct file_entry    *fe  = &dscrptr->fe;
	struct extfile_entry *efe = &dscrptr->efe;
	struct desc_tag      *tag = &dscrptr->tag;
	struct icb_tag       *icb;
	struct long_ad        allocated;
	struct long_ad       *long_adp  = NULL;
	struct short_ad      *short_adp = NULL;
	uint64_t inf_len;
	uint32_t l_ea, l_ad;
	uint8_t *bpos;
	int bpos_start, ad_type, id;

	assert(context.format_flags & FORMAT_VAT);

	id = udf_rw16(tag->id);
	assert(id == TAGID_FENTRY || id == TAGID_EXTFENTRY);
	if (id == TAGID_FENTRY) {
		icb         = &fe->icbtag;
		inf_len     = udf_rw64(fe->inf_len);
		l_ea        = udf_rw32(fe->l_ea);
		bpos        = (uint8_t *) fe->data + l_ea;
		bpos_start  = offsetof(struct file_entry, data) + l_ea;
	} else {
		icb         = &efe->icbtag;
		inf_len     = udf_rw64(efe->inf_len);
		l_ea        = udf_rw32(efe->l_ea);
		bpos        = (uint8_t *) efe->data + l_ea;
		bpos_start  = offsetof(struct extfile_entry, data) + l_ea;
	}
	/* inf_len should be correct for one slot */
	assert(inf_len < UDF_EXT_MAXLEN);

	ad_type = udf_rw16(icb->flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
	if (ad_type == UDF_ICB_INTERN_ALLOC) {
		/* no action needed */
		return;
	}

	assert(vpart_num == context.data_part);
	udf_data_alloc(udf_bytes_to_sectors(inf_len), &allocated);
	memset(bpos, 0, context.sector_size - bpos_start);
	/* create one short_ad or one long_ad */
	if (ad_type == UDF_ICB_SHORT_ALLOC) {
		short_adp = (struct short_ad *) bpos;
		short_adp->len    = udf_rw64(inf_len);
		short_adp->lb_num = allocated.loc.lb_num;
		l_ad = sizeof(struct short_ad);
	} else {
		long_adp  = (struct long_ad  *) bpos;
		memcpy(long_adp, &allocated, sizeof(struct long_ad));
		long_adp->len = udf_rw64(inf_len);
		l_ad = sizeof(struct long_ad);
	}
	if (id == TAGID_FENTRY)
		fe->l_ad = udf_rw32(l_ad);
	else
		efe->l_ad = udf_rw32(l_ad);
	;
	*l_adp = l_ad;
}


static void
udf_copy_fid_verbatim(struct fileid_desc *sfid, struct fileid_desc *dfid,
		uint64_t dfpos, uint64_t drest)
{
	uint64_t endfid;
	uint32_t minlen, lb_rest, fidsize;

	if (udf_rw16(sfid->l_iu) == 0) {
		memcpy(dfid, sfid, udf_fidsize(sfid));
		return;
	}

	/* see if we can reduce its size */
	minlen = udf_fidsize(sfid) - udf_rw16(sfid->l_iu);

	/*
	 * OK, tricky part: we need to pad so the next descriptor header won't
	 * cross the sector boundary
	 */
	endfid = dfpos + minlen;
	lb_rest = context.sector_size - (endfid % context.sector_size);

	memcpy(dfid, sfid, UDF_FID_SIZE);
	if (lb_rest < sizeof(struct desc_tag)) {
		/* add at least 32 */
		dfid->l_iu = udf_rw16(32);
		udf_set_regid((struct regid *) dfid->data, context.impl_name);
		udf_add_impl_regid((struct regid *) dfid->data);

	}
	memcpy( dfid->data + udf_rw16(dfid->l_iu),
		sfid->data + udf_rw16(sfid->l_iu),
		minlen - UDF_FID_SIZE);

	fidsize = udf_fidsize(dfid);
	dfid->tag.desc_crc_len = udf_rw16(fidsize - UDF_DESC_TAG_LENGTH);
}


static int
udf_rebuild_fid_stream(struct udf_fsck_node *node, int64_t *rest_lenp)
{
	struct fileid_desc *sfid, *dfid;
	uint64_t inf_len;
	uint64_t sfpos, dfpos;
	int64_t srest, drest;
//	uint32_t sfid_len, dfid_len;
	uint8_t *directory, *rebuild_dir;
//	int namelen;
	int error, streaming, was_streaming, warned, error_in_stream;

	directory = node->directory;
	inf_len   = node->found.inf_len;

	rebuild_dir = calloc(1, inf_len);
	assert(rebuild_dir);

	sfpos  = 0;
	srest  = inf_len;

	dfpos  = 0;
	drest  = inf_len;

	error_in_stream = 0;
	streaming = 1;
	was_streaming = 1;
	warned = 0;
	while (srest > 0) {
		if (was_streaming & !streaming) {
			if (!warned) {
				pwarn("%s : BROKEN directory\n",
					udf_node_path(node));
				udf_recursive_keep(node);
				node->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
			}
			warned = 1;
			pwarn("%s : <directory resync>\n",
					udf_node_path(node));
		}
		was_streaming = streaming;

		assert(drest >= UDF_FID_SIZE);
		sfid = (struct fileid_desc *) (directory + sfpos);
		dfid = (struct fileid_desc *) (rebuild_dir + dfpos);

		/* check if we can read/salvage the next source fid */
		if (udf_rw16(sfid->tag.id) != TAGID_FID) {
			streaming = 0;
			sfpos += 4;
			srest -= 4;
			error_in_stream = 1;
			continue;
		}
		error = udf_check_tag(sfid);
		if (error) {
			/* unlikely to be recoverable */
			streaming = 0;
			sfpos += 4;
			srest -= 4;
			error_in_stream = 1;
			continue;
		}
		error = udf_check_tag_payload(
			(union dscrptr *) sfid,
			context.sector_size);
		if (!error) {
			streaming = 1;
			/* all OK, just copy verbatim, shrinking if possible */
			udf_copy_fid_verbatim(sfid, dfid, dfpos, drest);

			sfpos += udf_fidsize(sfid);
			srest -= udf_fidsize(sfid);

			dfpos += udf_fidsize(dfid);
			drest -= udf_fidsize(dfid);

			assert(udf_fidsize(sfid) == udf_fidsize(dfid));
			continue;
		}

		/*
		 * The hard part, we need to try to recover of what is
		 * deductible of the bad source fid. The tag itself is OK, but
		 * that doesn't say much; its contents can still be off.
		 */

		/* TODO NOT IMPLEMENTED YET, skip this entry the blunt way */
		streaming = 0;
		sfpos += 4;
		srest -= 4;
		error_in_stream = 1;
	}

	/* if we could shrink/fix the node, mark it for repair */
	if (error_in_stream) {
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
	}

	if (sfpos != dfpos)
		printf("%s: could save %ld bytes in directory\n", udf_node_path(node), sfpos - dfpos);

	memset(directory, 0, inf_len);
	memcpy(directory, rebuild_dir, dfpos);

	free(rebuild_dir);

	*rest_lenp = dfpos;
	return error_in_stream;
}


static int
udf_quick_check_fids_piece(uint8_t *piece, uint32_t piece_len,
		struct udf_fsck_fid_context *fid_context,
		uint32_t lb_num)
{
	int error;
	struct fileid_desc *fid;
	uint32_t location;
	uint32_t offset, fidsize;

	offset = fid_context->fid_offset % context.sector_size;
	while (fid_context->data_left && (offset < piece_len)) {
		fid = (struct fileid_desc *) (piece + offset);
		if (udf_rw16(fid->tag.id) == TAGID_FID) {
			error = udf_check_tag_payload(
					(union dscrptr *) fid,
					context.sector_size);
			if (error)
				return error;
		} else {
			return EINVAL;
		}
		assert(udf_rw16(fid->tag.id) == TAGID_FID);

		location = lb_num + offset / context.sector_size;

		if (udf_rw32(fid->tag.tag_loc) != location)
			return EINVAL;

		if (context.dscrver == 2) {
			/* compression IDs should be preserved in UDF < 2.00 */
			if (*(fid->data + udf_rw16(fid->l_iu)) > 16)
				return EINVAL;
		}

		fidsize      = udf_fidsize(fid);
		offset      += fidsize;
		fid_context->fid_offset += fidsize;
		fid_context->data_left  -= fidsize;
	}

	return 0;
}


static void
udf_fids_fixup(uint8_t *piece, uint32_t piece_len,
		struct udf_fsck_fid_context *fid_context,
		uint32_t lb_num)
{
	struct fileid_desc *fid;
	uint32_t location;
	uint32_t offset, fidsize;

	offset = fid_context->fid_offset % context.sector_size;
	while (fid_context->data_left && (offset < piece_len)) {

		fid = (struct fileid_desc *) (piece + offset);
		assert(udf_rw16(fid->tag.id) == TAGID_FID);

		location = lb_num + offset / context.sector_size;
		fid->tag.tag_loc = udf_rw32(location);

		udf_validate_tag_and_crc_sums((union dscrptr *) fid);

		fidsize      = udf_fidsize(fid);
		offset      += fidsize;
		fid_context->fid_offset += fidsize;
		fid_context->data_left  -= fidsize;
	}
}


/* NOTE returns non 0 for overlap, not an error code */
static int
udf_check_if_allocated(struct udf_fsck_node *node, int flags,
		uint32_t start_lb, int partnr, uint32_t piece_len)
{
	union dscrptr *dscr;
	struct udf_fsck_overlap *new_overlap;
	uint8_t *bpos;
	uint32_t cnt, bit;
	uint32_t blocks = udf_bytes_to_sectors(piece_len);
	int overlap = 0;

	/* account for space used on underlying partition */
#ifdef DEBUG
	printf("check allocated : node %p, flags %d, partnr %d, start_lb %d for %d blocks\n",
		node, flags, partnr, start_lb, blocks);
#endif

	switch (context.vtop_tp[partnr]) {
	case UDF_VTOP_TYPE_VIRT:
		/* nothing */
		break;
	case UDF_VTOP_TYPE_PHYS:
	case UDF_VTOP_TYPE_SPAREABLE:
	case UDF_VTOP_TYPE_META:
		if (context.part_unalloc_bits[context.vtop[partnr]] == NULL)
			break;
#ifdef DEBUG
		printf("checking allocation of %d+%d for being used\n", start_lb, blocks);
#endif
		dscr = (union dscrptr *) (context.part_unalloc_bits[partnr]);
		for (cnt = start_lb; cnt < start_lb + blocks; cnt++) {
			 bpos  = &dscr->sbd.data[cnt / 8];
			 bit   = cnt % 8;
			 /* only account for bits marked free */
			 if ((*bpos & (1 << bit)) == 0)
				 overlap++;
		}
		if (overlap == 0)
			break;

		/* overlap */
//		pwarn("%s allocation OVERLAP found, type %d\n",
//				udf_node_path(node), flags);
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_OVERLAP;

		new_overlap = calloc(1, sizeof(struct udf_fsck_overlap));
		assert(new_overlap);

		new_overlap->node              = node;
		new_overlap->node2             = NULL;
		new_overlap->flags             = flags;
		new_overlap->flags2            = 0;
		new_overlap->loc.len           = udf_rw32(piece_len);
		new_overlap->loc.loc.lb_num    = udf_rw32(start_lb);
		new_overlap->loc.loc.part_num  = udf_rw16(partnr);

		TAILQ_INSERT_TAIL(&fsck_overlaps, new_overlap, next);

		return overlap;
		break;
	default:
		errx(1, "internal error: bad mapping type %d in %s",
			context.vtop_tp[partnr], __func__);
	}
	/* no overlap */
	return 0;
}


/* NOTE returns non 0 for overlap, not an error code */
static void
udf_check_overlap_pair(struct udf_fsck_node *node, int flags,
		uint32_t start_lb, int partnr, uint32_t piece_len)
{
	struct udf_fsck_overlap *overlap;
	uint32_t ostart_lb, opiece_len, oblocks;
	uint32_t blocks = udf_bytes_to_sectors(piece_len);
	int opartnr;

	/* account for space used on underlying partition */
#ifdef DEBUG
	printf("check overlap pair : node %p, flags %d, partnr %d, start_lb %d for %d blocks\n",
		node, flags, partnr, start_lb, blocks);
#endif

	switch (context.vtop_tp[partnr]) {
	case UDF_VTOP_TYPE_VIRT:
		/* nothing */
		break;
	case UDF_VTOP_TYPE_PHYS:
	case UDF_VTOP_TYPE_SPAREABLE:
	case UDF_VTOP_TYPE_META:
		if (context.part_unalloc_bits[context.vtop[partnr]] == NULL)
			break;
#ifdef DEBUG
		printf("checking overlap of %d+%d for being used\n", start_lb, blocks);
#endif
		/* check all current overlaps with the piece we have here */
		TAILQ_FOREACH(overlap, &fsck_overlaps, next) {
			opiece_len = udf_rw32(overlap->loc.len);
			ostart_lb  = udf_rw32(overlap->loc.loc.lb_num);
			opartnr    = udf_rw16(overlap->loc.loc.part_num);
			oblocks    = udf_bytes_to_sectors(opiece_len);

			if (partnr != opartnr)
				continue;
			/* piece before overlap? */
			if (start_lb + blocks < ostart_lb)
				continue;
			/* piece after overlap? */
			if (start_lb > ostart_lb + oblocks)
				continue;

			/* overlap, mark conflict */
			overlap->node2             = node;
			overlap->flags2            = flags;
			overlap->loc2.len          = udf_rw32(piece_len);
			overlap->loc2.loc.lb_num   = udf_rw32(start_lb);
			overlap->loc2.loc.part_num = udf_rw16(partnr);

			udf_recursive_keep(node);
			node->fsck_flags |= FSCK_NODE_FLAG_OVERLAP;
		}
		return;
	default:
		errx(1, "internal error: bad mapping type %d in %s",
			context.vtop_tp[partnr], __func__);
	}
	/* no overlap */
	return;
}



static int
udf_process_ad(union dscrptr *dscrptr, int action, uint8_t **resultp,
	int vpart_num, uint64_t fpos,
	struct short_ad *short_adp, struct long_ad *long_adp, void *process_context)
{
	struct file_entry    *fe  = &dscrptr->fe;
	struct extfile_entry *efe = &dscrptr->efe;
	struct desc_tag      *tag = &dscrptr->tag;
	struct icb_tag  *icb;
	struct udf_fsck_file_stats *stats;
	uint64_t inf_len;
	uint32_t l_ea, piece_len, piece_alloc_len, piece_sectors, lb_num, flags;
	uint32_t dscr_lb_num;
	uint32_t i;
	uint8_t *bpos, *piece;
	int id, ad_type;
	int error, piece_error, return_error;

	assert(dscrptr);
	stats = (struct udf_fsck_file_stats *) process_context;

	id = udf_rw16(tag->id);
	assert(id == TAGID_FENTRY || id == TAGID_EXTFENTRY);
	if (id == TAGID_FENTRY) {
		icb         = &fe->icbtag;
		dscr_lb_num = udf_rw32(fe->tag.tag_loc);
		inf_len     = udf_rw64(fe->inf_len);
		l_ea        = udf_rw32(fe->l_ea);
		bpos        = (uint8_t *) fe->data + l_ea;
	} else {
		icb         = &efe->icbtag;
		dscr_lb_num = udf_rw32(efe->tag.tag_loc);
		inf_len     = udf_rw64(efe->inf_len);
		l_ea        = udf_rw32(efe->l_ea);
		bpos        = (uint8_t *) efe->data + l_ea;
	}

	lb_num = 0;
	piece_len = 0;

	ad_type = udf_rw16(icb->flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
	if (ad_type == UDF_ICB_INTERN_ALLOC) {
		piece_len = inf_len;
	}
	if (short_adp) {
		piece_len = udf_rw32(short_adp->len);
		lb_num    = udf_rw32(short_adp->lb_num);
	}
	if (long_adp) {
		piece_len = udf_rw32(long_adp->len);
		lb_num    = udf_rw32(long_adp->loc.lb_num);
		vpart_num = udf_rw16(long_adp->loc.part_num);
	}
	flags = UDF_EXT_FLAGS(piece_len);
	piece_len = UDF_EXT_LEN(piece_len);
	piece_alloc_len = UDF_ROUNDUP(piece_len, context.sector_size);
	piece_sectors   = piece_alloc_len / context.sector_size;

	return_error = 0;
	if (action & AD_GATHER_STATS) {
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			stats->inf_len     = piece_len;
			stats->obj_size    = piece_len;
			stats->logblks_rec = 0;
		}  else if (flags == UDF_EXT_ALLOCATED) {
			stats->inf_len     += piece_len;
			stats->obj_size    += piece_len;
			stats->logblks_rec += piece_sectors;
		} else if (flags == UDF_EXT_FREED) {
			stats->inf_len     += piece_len;
			stats->obj_size    += piece_len;
			stats->logblks_rec += piece_sectors;
		} else if (flags == UDF_EXT_FREE) {
			stats->inf_len     += piece_len;
			stats->obj_size    += piece_len;
		}
	}
	if (action & AD_LOAD_FILE) {
		uint32_t alloc_len;

		piece = calloc(1, piece_alloc_len);
		if (piece == NULL)
			return errno;
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			memcpy(piece, bpos, piece_len);
		} else if (flags == 0) {
			/* not empty */
			/* read sector by sector reading as much as possible */
			for (i = 0; i < piece_sectors; i++) {
				piece_error = udf_read_virt(
					piece + i * context.sector_size,
					lb_num + i, vpart_num, 1);
				if (piece_error)
					return_error = piece_error;
			}
		}

		alloc_len = UDF_ROUNDUP(fpos + piece_len, context.sector_size);
		error = reallocarr(resultp, 1, alloc_len);
		if (error) {
			/* fatal */
			free(piece);
			free(*resultp);
			return errno;
		}

		memcpy(*resultp + fpos, piece, piece_alloc_len);
		free(piece);
	}
	if (action & AD_ADJUST_FIDS) {
		piece = *resultp + fpos;
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			udf_fids_fixup(piece, piece_len, process_context,
				dscr_lb_num);
		} else if (flags == 0) {
			udf_fids_fixup(piece, piece_len, process_context,
				lb_num);
		}
	}
	if (action & AD_CHECK_FIDS) {
		piece = *resultp + fpos;
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			error = udf_quick_check_fids_piece(piece, piece_len,
				process_context, dscr_lb_num);
		} else if (flags == 0) {
			error = udf_quick_check_fids_piece(piece, piece_len,
				process_context, lb_num);
		}
		if (error)
			return error;
	}
	if (action & AD_SAVE_FILE) {
		/*
		 * Note: only used for directory contents.
		 */
		piece = *resultp + fpos;
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			memcpy(bpos, piece, piece_len);
			/* nothing */
		} else if (flags == 0) {
			/* not empty */
			error = udf_write_virt(
				piece, lb_num, vpart_num,
				piece_sectors);
			if (error) {
				pwarn("Got error writing piece\n");
				return error;
			}
		} else {
			/* allocated but not written piece, skip */
		}
	}
	if (action & AD_CHECK_USED) {
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			/* nothing */
		} else if (flags != UDF_EXT_FREE) {
			struct udf_fsck_node *node = process_context;
			(void) udf_check_if_allocated(
				node,
				FSCK_OVERLAP_EXTENT,
				lb_num, vpart_num,
				piece_len);
		}
	}
	if (action & AD_FIND_OVERLAP_PAIR) {
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			/* nothing */
		} else if (flags != UDF_EXT_FREE) {
			struct udf_fsck_node *node = process_context;
			udf_check_overlap_pair(
				node,
				FSCK_OVERLAP_EXTENT,
				lb_num, vpart_num,
				piece_len);
		}
	}
	if (action & AD_MARK_AS_USED) {
		if (ad_type == UDF_ICB_INTERN_ALLOC) {
			/* nothing */
		} else if (flags != UDF_EXT_FREE) {
			udf_mark_allocated(lb_num, vpart_num,
				udf_bytes_to_sectors(piece_len));
		}
	}

	return return_error;
}


static int
udf_process_file(union dscrptr *dscrptr, int vpart_num, uint8_t **resultp,
	int action, void *process_context)
{
	struct file_entry    *fe  = &dscrptr->fe;
	struct extfile_entry *efe = &dscrptr->efe;
	struct desc_tag      *tag = &dscrptr->tag;
	struct alloc_ext_entry *ext;
	struct icb_tag  *icb;
	struct long_ad  *long_adp  = NULL;
	struct short_ad *short_adp = NULL;
	union  dscrptr *extdscr = NULL;
	uint64_t fpos;
	uint32_t l_ad, l_ea, piece_len, lb_num, flags;
	uint8_t *bpos;
	int id, extid, ad_type, ad_len;
	int error;

	id = udf_rw16(tag->id);
	assert(id == TAGID_FENTRY || id == TAGID_EXTFENTRY);

	if (action & AD_CHECK_USED) {
		struct udf_fsck_node *node = process_context;
		(void) udf_check_if_allocated(
			node,
			FSCK_OVERLAP_MAIN_NODE,
			udf_rw32(node->loc.loc.lb_num),
			udf_rw16(node->loc.loc.part_num),
			context.sector_size);
		/* return error code? */
	}

	if (action & AD_FIND_OVERLAP_PAIR) {
		struct udf_fsck_node *node = process_context;
		udf_check_overlap_pair(
			node,
			FSCK_OVERLAP_MAIN_NODE,
			udf_rw32(node->loc.loc.lb_num),
			udf_rw16(node->loc.loc.part_num),
			context.sector_size);
		/* return error code? */
	}

	if (action & AD_MARK_AS_USED)
		udf_mark_allocated(udf_rw32(tag->tag_loc), vpart_num, 1);

	if (id == TAGID_FENTRY) {
		icb         = &fe->icbtag;
		l_ad   = udf_rw32(fe->l_ad);
		l_ea   = udf_rw32(fe->l_ea);
		bpos = (uint8_t *) fe->data + l_ea;
	} else {
		icb         = &efe->icbtag;
		l_ad   = udf_rw32(efe->l_ad);
		l_ea   = udf_rw32(efe->l_ea);
		bpos = (uint8_t *) efe->data + l_ea;
	}

	ad_type = udf_rw16(icb->flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
	if (ad_type == UDF_ICB_INTERN_ALLOC) {
		error = udf_process_ad(dscrptr, action, resultp, -1, 0,
				NULL, NULL, process_context);
		return error;
	}
	if ((ad_type != UDF_ICB_SHORT_ALLOC) &&
			(ad_type != UDF_ICB_LONG_ALLOC))
		return EINVAL;

	if (ad_type == UDF_ICB_SHORT_ALLOC)
		short_adp = (struct short_ad *) bpos;
	else
		long_adp  = (struct long_ad  *) bpos;
	;

	if (action & AD_SAVE_FILE) {
		/*
		 * Special case for writeout file/directory on recordable
		 * media. We write in one go so wipe and (re)allocate the
		 * entire space.
		 */
		if (context.format_flags & FORMAT_VAT)
			udf_wipe_and_reallocate(dscrptr, vpart_num, &l_ad);
	}

	fpos = 0;
	bpos = NULL;
	error = 0;
	while (l_ad) {
		if (ad_type == UDF_ICB_SHORT_ALLOC) {
			piece_len = udf_rw32(short_adp->len);
			lb_num    = udf_rw32(short_adp->lb_num);
			ad_len = sizeof(struct short_ad);
		} else /* UDF_ICB_LONG_ALLOC  */ {
			piece_len = udf_rw32(long_adp->len);
			lb_num    = udf_rw32(long_adp->loc.lb_num);
			vpart_num = udf_rw16(long_adp->loc.part_num);
			ad_len = sizeof(struct long_ad);
		}
		flags = UDF_EXT_FLAGS(piece_len);
		piece_len = UDF_EXT_LEN(piece_len);

		switch (flags) {
		default :
			error = udf_process_ad(dscrptr, action, resultp,
					vpart_num, fpos, short_adp, long_adp,
					process_context);
			break;
		case UDF_EXT_REDIRECT  :
			if (piece_len != context.sector_size) {
				/* should this be an error? */
				pwarn("Got extention redirect with wrong size %d\n",
					piece_len);
				error = EINVAL;
				break;
			}
			free(extdscr);
			error = udf_read_dscr_virt(lb_num, vpart_num, &extdscr);
			if (error)
				break;
			/* empty block is terminator */
			if (extdscr == NULL)
				return 0;
			ext = &extdscr->aee;
			extid = udf_rw16(ext->tag.id);
			if (extid != TAGID_ALLOCEXTENT) {
				pwarn("Corruption in allocated extents chain\n");
				/* corruption! */
				free(extdscr);
				errno = EINVAL;
				break;
			}

			if (action & AD_CHECK_USED) {
				(void) udf_check_if_allocated(
					(struct udf_fsck_node *) process_context,
					FSCK_OVERLAP_EXTALLOC,
					lb_num,
					vpart_num,
					context.sector_size);
				/* returning error code ? */
			}

			if (action & AD_FIND_OVERLAP_PAIR) {
				struct udf_fsck_node *node = process_context;
				udf_check_overlap_pair(
					node,
					FSCK_OVERLAP_EXTALLOC,
					lb_num,
					vpart_num,
					context.sector_size);
				/* return error code? */
			}

			if (action & AD_MARK_AS_USED)
				udf_mark_allocated(
					lb_num, vpart_num,
					1);
			/* TODO check for prev_entry? */
			l_ad = ext->l_ad;
			bpos = ext->data;
			if (ad_type == UDF_ICB_SHORT_ALLOC)
				short_adp = (struct short_ad *) bpos;
			else
				long_adp  = (struct long_ad  *) bpos;
			;
			continue;
		}
		if (error)
			break;

		if (long_adp)  long_adp++;
		if (short_adp) short_adp++;
		fpos += piece_len;
		bpos += piece_len;
		l_ad -= ad_len;
	}

	return error;
}


static int
udf_readin_file(union dscrptr *dscrptr, int vpart_num, uint8_t **resultp,
		struct udf_fsck_file_stats *statsp)
{
	struct udf_fsck_file_stats stats;
	int error;

	bzero(&stats, sizeof(stats));
	*resultp = NULL;
	error = udf_process_file(dscrptr, vpart_num, resultp,
			AD_LOAD_FILE | AD_GATHER_STATS, (void *) &stats);
	if (statsp)
		*statsp = stats;
	return error;
}

/* --------------------------------------------------------------------- */

#define MAX_BSIZE		(0x10000)
#define UDF_ISO_VRS_SIZE	(32*2048) /* 32 ISO `sectors' */

static void
udf_check_vrs9660(void)
{
	struct vrs_desc *vrs;
	uint8_t buffer[MAX_BSIZE];
	uint64_t rpos;
	uint8_t *pos;
	int max_sectors, sector, factor;
	int ret, ok;

	if (context.format_flags & FORMAT_TRACK512)
		return;

	/*
	 * location of iso9660 VRS is defined as first sector AFTER 32kb,
	 * minimum `sector size' 2048
	 */
	layout.iso9660_vrs = ((32*1024 + context.sector_size - 1) /
			context.sector_size);
	max_sectors = UDF_ISO_VRS_SIZE / 2048;
	factor = (2048 + context.sector_size -1) / context.sector_size;

	ok = 1;
	rpos = (uint64_t) layout.iso9660_vrs * context.sector_size;
	ret = pread(dev_fd, buffer, UDF_ISO_VRS_SIZE, rpos);
	if (ret == -1) {
		pwarn("Error reading in ISO9660 VRS\n");
		ok = 0;
	}
	if (ok && ((uint32_t) ret != UDF_ISO_VRS_SIZE)) {
		pwarn("Short read in ISO9660 VRS\n");
		ok = 0;
	}

	if (ok) {
		ok = 0;
		for (sector = 0; sector < max_sectors; sector++) {
			pos = buffer + sector * factor * context.sector_size;
			vrs = (struct vrs_desc *) pos;
			if (strncmp((const char *) vrs->identifier, VRS_BEA01, 5) == 0)
				ok  = 1;
			if (strncmp((const char *) vrs->identifier, VRS_NSR02, 5) == 0)
				ok |= 2;
			if (strncmp((const char *) vrs->identifier, VRS_NSR03, 5) == 0)
				ok |= 2;
			if (strncmp((const char *) vrs->identifier, VRS_TEA01, 5) == 0) {
				ok |= 4;
				break;
			}
		}
		if (ok != 7)
			ok = 0;
	}
	if (!ok) {
		pwarn("Error in ISO 9660 volume recognition sequence\n");
		if (context.format_flags & FORMAT_SEQUENTIAL) {
			pwarn("ISO 9660 volume recognition sequence can't be repaired "
			       "on SEQUENTIAL media\n");
		} else if (ask(0, "fix ISO 9660 volume recognition sequence")) {
			if (!rdonly)
				udf_write_iso9660_vrs();
		}
	}
}


/*
 * Read in disc and try to find basic properties like sector size, expected
 * UDF versions etc.
 */

static int
udf_find_anchor(int anum)
{
	uint8_t buffer[MAX_BSIZE];
	struct anchor_vdp *avdp = (struct anchor_vdp *) buffer;
	uint64_t rpos;
	uint32_t location;
	int sz_guess, ret;
	int error;

	location = layout.anchors[anum];

	/*
	 * Search ADVP by reading bigger and bigger sectors NOTE we can't use
	 * udf_read_phys yet since the sector size is not known yet
	 */
	sz_guess = mmc_discinfo.sector_size;	/* assume media is bigger */
	for (; sz_guess <= MAX_BSIZE; sz_guess += 512) {
		rpos = (uint64_t) location * sz_guess;
		ret = pread(dev_fd, buffer, sz_guess, rpos);
		if (ret == -1) {
			if (errno == ENODEV)
				return errno;
		} else if (ret != sz_guess) {
			/* most likely EOF, ignore */
		} else {
			error = udf_check_tag_and_location(buffer, location);
			if (!error) {
				if (udf_rw16(avdp->tag.id) != TAGID_ANCHOR)
					continue;
				error = udf_check_tag_payload(buffer, sz_guess);
				if (!error)
					break;
			}
		}
	}
	if (sz_guess > MAX_BSIZE)
		return -1;

	/* special case for disc images */
	if (mmc_discinfo.sector_size != (unsigned int) sz_guess) {
		emul_sectorsize = sz_guess;
		udf_update_discinfo();
	}
	context.sector_size = sz_guess;
	context.dscrver = udf_rw16(avdp->tag.descriptor_ver);

	context.anchors[anum] = calloc(1, context.sector_size);
	memcpy(context.anchors[anum], avdp, context.sector_size);

	context.min_udf = 0x102;
	context.max_udf = 0x150;
	if (context.dscrver > 2) {
		context.min_udf = 0x200;
		context.max_udf = 0x260;
	}
	return 0;
}


static int
udf_get_anchors(void)
{
	struct mmc_trackinfo ti;
	struct anchor_vdp *avdp;
	int need_fixup, error;

	memset(&layout, 0, sizeof(layout));
	memset(&ti, 0, sizeof(ti));

	/* search start */
	for (int i = 1; i <= mmc_discinfo.num_tracks; i++) {
		ti.tracknr = i;
		error = udf_update_trackinfo(&ti);
		assert(!error);
		if (ti.sessionnr == target_session)
			break;
	}
	/* support for track 512 */
	if (ti.flags & MMC_TRACKINFO_BLANK) 
		context.format_flags |= FORMAT_TRACK512;

	assert(!error);
	context.first_ti = ti;

	/* search end */
	for (int i = mmc_discinfo.num_tracks; i > 0; i--) {
		ti.tracknr = i;
		error = udf_update_trackinfo(&ti);
		assert(!error);
		if (ti.sessionnr == target_session)
			break;
	}
	context.last_ti = ti;

	layout.first_lba  = context.first_ti.track_start;
	layout.last_lba   = mmc_discinfo.last_possible_lba;
	layout.blockingnr = udf_get_blockingnr(&ti);

	layout.anchors[0] = layout.first_lba + 256;
	if (context.format_flags & FORMAT_TRACK512)
		layout.anchors[0] = layout.first_lba + 512;
	layout.anchors[1] = layout.last_lba - 256;
	layout.anchors[2] = layout.last_lba;

	need_fixup = 0;
	error = udf_find_anchor(0);
	if (error == ENODEV) {
		pwarn("Drive empty?\n");
		return errno;
	}
	if (error) {
		need_fixup = 1;
		if (!preen)
			pwarn("Anchor ADVP0 can't be found! Searching others\n");
		error = udf_find_anchor(2);
		if (error) {
			if (!preen)
				pwarn("Anchor ADVP2 can't be found! Searching ADVP1\n");
			/* this may be fidly, but search */
			error = udf_find_anchor(1);
			if (error) {
				if (!preen)
					pwarn("No valid anchors found!\n");
				/* TODO scan media for VDS? */
				return -1;
			}
		}
	}

	if (need_fixup) {
		if (context.format_flags & FORMAT_SEQUENTIAL) {
			pwarn("Missing primary anchor can't be resolved on "
			      "SEQUENTIAL media\n");
		} else if (ask(1, "Fixup missing anchors")) {
			pwarn("TODO fixup missing anchors\n");
			need_fixup = 0;
		}
		if (need_fixup)
			return -1;
	}
	if (!preen)
		printf("Filesystem sectorsize is %d bytes.\n\n",
			context.sector_size);

	/* update our last track info since our idea of sector size might have changed */
	(void) udf_update_trackinfo(&context.last_ti);

	/* sector size is now known */
	wrtrack_skew = context.last_ti.next_writable % layout.blockingnr;

	avdp = context.anchors[0];
	/* extract info from current anchor */
	layout.vds1      = udf_rw32(avdp->main_vds_ex.loc);
	layout.vds1_size = udf_rw32(avdp->main_vds_ex.len) / context.sector_size;
	layout.vds2      = udf_rw32(avdp->reserve_vds_ex.loc);
	layout.vds2_size = udf_rw32(avdp->reserve_vds_ex.len) / context.sector_size;

	return 0;
}


#define UDF_LVINT_HIST_CHUNK 32
static void
udf_retrieve_lvint(void) {
	union dscrptr *dscr;
	struct logvol_int_desc *lvint;
	struct udf_lvintq *trace;
	uint32_t lbnum, len, *pos;
	uint8_t *wpos;
	int num_partmappings;
	int error, cnt, trace_len;
	int sector_size = context.sector_size;

	len     = udf_rw32(context.logical_vol->integrity_seq_loc.len);
	lbnum   = udf_rw32(context.logical_vol->integrity_seq_loc.loc);
	layout.lvis = lbnum;
	layout.lvis_size = len / sector_size;

	udf_create_lvintd(UDF_INTEGRITY_OPEN);

	/* clean trace and history */
	memset(context.lvint_trace, 0,
	    UDF_LVDINT_SEGMENTS * sizeof(struct udf_lvintq));
	context.lvint_history_wpos = 0;
	context.lvint_history_len = UDF_LVINT_HIST_CHUNK;
	context.lvint_history = calloc(UDF_LVINT_HIST_CHUNK, sector_size);

	/* record the length on this segment */
	context.lvint_history_ondisc_len = (len / sector_size);

	trace_len    = 0;
	trace        = context.lvint_trace;
	trace->start = lbnum;
	trace->end   = lbnum + len/sector_size;
	trace->pos   = 0;
	trace->wpos  = 0;

	dscr  = NULL;
	error = 0;
	while (len) {
		trace->pos  = lbnum - trace->start;
		trace->wpos = trace->pos + 1;

		free(dscr);
		error = udf_read_dscr_phys(lbnum, &dscr);
		/* bad descriptors mean corruption, terminate */
		if (error)
			break;

		/* empty terminates */
		if (dscr == NULL) {
			trace->wpos = trace->pos;
			break;
		}

		/* we got a valid descriptor */
		if (udf_rw16(dscr->tag.id) == TAGID_TERM) {
			trace->wpos = trace->pos;
			break;
		}
		/* only logical volume integrity descriptors are valid */
		if (udf_rw16(dscr->tag.id) != TAGID_LOGVOL_INTEGRITY) {
			error = ENOENT;
			break;
		}
		lvint = &dscr->lvid;

		/* see if our history is long enough, with one spare */
		if (context.lvint_history_wpos+2 >= context.lvint_history_len) {
			int new_len = context.lvint_history_len +
				UDF_LVINT_HIST_CHUNK;
			if (reallocarr(&context.lvint_history,
					new_len, sector_size))
				err(FSCK_EXIT_CHECK_FAILED, "can't expand logvol history");
			context.lvint_history_len = new_len;
		}

		/* are we linking to a new piece? */
		if (lvint->next_extent.len) {
			len   = udf_rw32(lvint->next_extent.len);
			lbnum = udf_rw32(lvint->next_extent.loc);

			if (trace_len >= UDF_LVDINT_SEGMENTS-1) {
				/* IEK! segment link full... */
				pwarn("implementation limit: logical volume "
					"integrity segment list full\n");
				error = ENOMEM;
				break;
			}
			trace++;
			trace_len++;

			trace->start = lbnum;
			trace->end   = lbnum + len/sector_size;
			trace->pos   = 0;
			trace->wpos  = 0;

			context.lvint_history_ondisc_len += (len / sector_size);
		}

		/* record this found lvint; it is one sector long */
		wpos = context.lvint_history +
			context.lvint_history_wpos * sector_size;
		memcpy(wpos, dscr, sector_size);
		memcpy(context.logvol_integrity, dscr, sector_size);
		context.lvint_history_wpos++;

		/* proceed sequential */
		lbnum += 1;
		len   -= sector_size;
	}

	/* clean up the mess, esp. when there is an error */
	free(dscr);

	if (error) {
		if (!preen)
			printf("Error in logical volume integrity sequence\n");
		printf("Marking logical volume integrity OPEN\n");
		udf_update_lvintd(UDF_INTEGRITY_OPEN);
	}

	if (udf_rw16(context.logvol_info->min_udf_readver) > context.min_udf)
		context.min_udf   = udf_rw16(context.logvol_info->min_udf_readver);
	if (udf_rw16(context.logvol_info->min_udf_writever) > context.min_udf)
		context.min_udf   = udf_rw16(context.logvol_info->min_udf_writever);
	if (udf_rw16(context.logvol_info->max_udf_writever) < context.max_udf)
		context.max_udf   = udf_rw16(context.logvol_info->max_udf_writever);

	context.unique_id = udf_rw64(context.logvol_integrity->lvint_next_unique_id);

	/* fill in current size/free values */
	pos = &context.logvol_integrity->tables[0];
	num_partmappings = udf_rw32(context.logical_vol->n_pm);
	for (cnt = 0; cnt < num_partmappings; cnt++) {
		context.part_free[cnt] = udf_rw32(*pos);
		pos++;
	}
	/* leave the partition sizes alone; no idea why they are stated here */
	/* TODO sanity check the free space and partition sizes? */

/* XXX FAULT INJECTION POINT XXX */
//udf_update_lvintd(UDF_INTEGRITY_OPEN);

	if (!preen) {
		int ver;

		printf("\n");
		ver = udf_rw16(context.logvol_info->min_udf_readver);
		printf("Minimum read  version v%x.%02x\n", ver/0x100, ver&0xff);
		ver = udf_rw16(context.logvol_info->min_udf_writever);
		printf("Minimum write version v%x.%02x\n", ver/0x100, ver&0xff);
		ver = udf_rw16(context.logvol_info->max_udf_writever);
		printf("Maximum write version v%x.%02x\n", ver/0x100, ver&0xff);

		printf("\nLast logical volume integrity state is %s.\n",
			udf_rw32(context.logvol_integrity->integrity_type) ?
			"CLOSED" : "OPEN");
	}
}


static int
udf_writeout_lvint(void)
{
	union dscrptr *terminator;
	struct udf_lvintq *intq, *nintq;
	struct logvol_int_desc *lvint;
	uint32_t location;
	int wpos, num_avail;
	int sector_size = context.sector_size;
	int integrity_type, error;
	int next_present, end_slot, last_segment;

	/* only write out when its open */
	integrity_type = udf_rw32(context.logvol_integrity->integrity_type);
	if (integrity_type == UDF_INTEGRITY_CLOSED)
		return 0;

	if (!preen)
		printf("\n");
	if (!ask(1, "Write out modifications"))
		return 0;

	udf_allow_writing();

	/* close logical volume */
	udf_update_lvintd(UDF_INTEGRITY_CLOSED);

	/* do we need to lose some history? */
	if ((context.lvint_history_ondisc_len - context.lvint_history_wpos) < 2) {
		uint8_t *src, *dst;
		uint32_t size;

		dst = context.lvint_history;
		src = dst + sector_size;
		size = (context.lvint_history_wpos-2) * sector_size;
		memmove(dst, src, size);
		context.lvint_history_wpos -= 2;
	}

	/* write out complete trace just in case */
	wpos = 0;
	location = 0;
	for (int i = 0; i < UDF_LVDINT_SEGMENTS; i++) {
		intq = &context.lvint_trace[i];
		nintq = &context.lvint_trace[i+1];

		/* end of line? */
		if (intq->start == intq->end)
			break;
		num_avail = intq->end - intq->start;
		location  = intq->start;
		for (int sector = 0; sector < num_avail; sector++) {
			lvint = (struct logvol_int_desc *)
				(context.lvint_history + wpos * sector_size);
			memset(&lvint->next_extent, 0, sizeof(struct extent_ad));
			next_present = (wpos != context.lvint_history_wpos);
			end_slot     = (sector == num_avail -1);
			last_segment = (i == UDF_LVDINT_SEGMENTS-1);
			if (end_slot && next_present && !last_segment) {
				/* link to next segment */
				lvint->next_extent.len = udf_rw32(
					sector_size * (nintq->end - nintq->start));
				lvint->next_extent.loc = udf_rw32(nintq->start);
			}
			error = udf_write_dscr_phys((union dscrptr *) lvint, location, 1);
			assert(!error);
			wpos++;
			location++;
			if (wpos == context.lvint_history_wpos)
				break;
		}
	}

	/* at write pos, write out our integrity */
	assert(location);
	lvint = context.logvol_integrity;
	error = udf_write_dscr_phys((union dscrptr *) lvint, location, 1);
	assert(!error);
	wpos++;
	location++;

	/* write out terminator */
	terminator = calloc(1, context.sector_size);
	assert(terminator);
	udf_create_terminator(terminator, 0);

	/* same or increasing serial number: ECMA 3/7.2.5, 4/7.2.5, UDF 2.3.1.1. */
	terminator->tag.serial_num = lvint->tag.serial_num;

	error = udf_write_dscr_phys(terminator, location, 1);
	free(terminator);
	assert(!error);
	wpos++;
	location++;

	return 0;
}


static int
udf_readin_partitions_free_space(void)
{
	union dscrptr *dscr;
	struct part_desc *part;
	struct part_hdr_desc *phd;
	uint32_t bitmap_len, bitmap_lb;
	int cnt, tagid, error;

	/* XXX freed space bitmap ignored XXX */
	error = 0;
	for (cnt = 0; cnt < UDF_PARTITIONS; cnt++) {
		part = context.partitions[cnt];
		if (!part)
			continue;

		phd = &part->pd_part_hdr;
		bitmap_len = udf_rw32(phd->unalloc_space_bitmap.len);
		bitmap_lb  = udf_rw32(phd->unalloc_space_bitmap.lb_num);

		if (bitmap_len == 0) {
			error = 0;
			continue;
		}

		if (!preen)
			printf("Reading in free space map for partition %d\n", cnt);
		error = udf_read_dscr_virt(bitmap_lb, cnt, &dscr);
		if (error)
			break;
		if (!dscr) {
			error = ENOENT;
			break;
		}
		tagid = udf_rw16(dscr->tag.id);
		if (tagid != TAGID_SPACE_BITMAP) {
			pwarn("Unallocated space bitmap expected but got "
			      "tag %d\n", tagid);
			free(dscr);
			error = ENOENT;
			break;
		}
		if (udf_tagsize(dscr, context.sector_size) > bitmap_len) {
			pwarn("Warning, size of read in bitmap %d is "
			      "not equal to expected size %d\n",
			      udf_tagsize(dscr, context.sector_size),
			      bitmap_len);
		}
		context.part_unalloc_bits[cnt] = &dscr->sbd;
	}

	/* special case for metadata partitions */
	for (cnt = 0; cnt < UDF_PMAPS; cnt++) {
		if (context.vtop_tp[cnt] != UDF_VTOP_TYPE_META)
			continue;
		/* only if present */
		if (layout.meta_bitmap == 0xffffffff)
			continue;
		if (!preen)
			printf("Reading in free space map for partition %d\n", cnt);
		error = udf_readin_file(
				(union dscrptr *) context.meta_bitmap,
				context.vtop[cnt],
				(uint8_t **) &context.part_unalloc_bits[cnt],
				NULL);
		if (error) {
			free(context.part_unalloc_bits[cnt]);
			context.part_unalloc_bits[cnt] = NULL;
			pwarn("implementation limit: metadata bitmap file read error, "
			      "can't fix this up yet\n");
			return error;
		}
	}
	if (!preen)
		printf("\n");

	return error;
}


/* ------------------------- VAT support ------------------------- */

/*
 * Update logical volume name in all structures that keep a record of it. We
 * use memmove since each of them might be specified as a source.
 *
 * Note that it doesn't update the VAT structure!
 */

static void
udf_update_logvolname(char *logvol_id)
{
	struct logvol_desc     *lvd = NULL;
	struct fileset_desc    *fsd = NULL;
	struct udf_lv_info     *lvi = NULL;

	lvd = context.logical_vol;
	fsd = context.fileset_desc;
	if (context.implementation)
		lvi = &context.implementation->_impl_use.lv_info;

	/* logvol's id might be specified as original so use memmove here */
	memmove(lvd->logvol_id, logvol_id, 128);
	if (fsd)
		memmove(fsd->logvol_id, logvol_id, 128);
	if (lvi)
		memmove(lvi->logvol_id, logvol_id, 128);
}


static struct timestamp *
udf_file_mtime(union dscrptr *dscr)
{
	int tag_id = udf_rw16(dscr->tag.id);

	assert((tag_id == TAGID_FENTRY) || (tag_id == TAGID_EXTFENTRY));
	if (tag_id == TAGID_FENTRY)
		return &dscr->fe.mtime;
	else 
		return &dscr->efe.mtime;
	;
}


static void
udf_print_vat_details(union dscrptr *dscr)
{
	printf("\n");
	udf_print_timestamp("\tFound VAT timestamped at ",
		udf_file_mtime(dscr), "\n");
}


static int
udf_check_for_vat(union dscrptr *dscr)
{
	struct icb_tag   *icbtag;
	uint32_t  vat_length;
	int tag_id, filetype;

	tag_id = udf_rw16(dscr->tag.id);

	if ((tag_id != TAGID_FENTRY) && (tag_id != TAGID_EXTFENTRY))
		return ENOENT;

	if (tag_id == TAGID_FENTRY) {
		vat_length = udf_rw64(dscr->fe.inf_len);
		icbtag    = &dscr->fe.icbtag;
	} else {
		vat_length = udf_rw64(dscr->efe.inf_len);
		icbtag = &dscr->efe.icbtag;
	}
	filetype = icbtag->file_type;
	if ((filetype != 0) && (filetype != UDF_ICB_FILETYPE_VAT))
		return ENOENT;

	/* TODO sanity check vat length */
	vat_length = vat_length;

	return 0;
}


static int
udf_extract_vat(union dscrptr *dscr, uint8_t **vat_contents)
{
	struct udf_fsck_file_stats	 stats;
	struct icb_tag			*icbtag;
	struct timestamp		*mtime;
	struct udf_vat			*vat;
	struct udf_oldvat_tail		*oldvat_tl;
	struct udf_logvol_info		*lvinfo;
	struct impl_extattr_entry	*implext;
	struct vatlvext_extattr_entry	 lvext;
	const char *extstr = "*UDF VAT LVExtension";
	uint64_t vat_unique_id;
	uint64_t vat_length;
	uint32_t vat_entries, vat_offset;
	uint32_t offset, a_l;
	uint8_t *ea_start, *lvextpos;
	char *regid_name;
	int tag_id, filetype;
	int error;

	*vat_contents = NULL;
	lvinfo = context.logvol_info;

	/* read in VAT contents */
	error = udf_readin_file(dscr, context.data_part, vat_contents, &stats);
	if (error) {
		error = ENOENT;
		goto out;
	}

	/* tag_id already checked */
	tag_id = udf_rw16(dscr->tag.id);
	if (tag_id == TAGID_FENTRY) {
		vat_length    = udf_rw64(dscr->fe.inf_len);
		icbtag        = &dscr->fe.icbtag;
		mtime         = &dscr->fe.mtime;
		vat_unique_id = udf_rw64(dscr->fe.unique_id);
		ea_start      = dscr->fe.data;
	} else {
		vat_length    = udf_rw64(dscr->efe.inf_len);
		icbtag        = &dscr->efe.icbtag;
		mtime         = &dscr->efe.mtime;
		vat_unique_id = udf_rw64(dscr->efe.unique_id);
		ea_start      = dscr->efe.data;	/* for completion */
	}

	if (vat_length > stats.inf_len) {
		error = ENOENT;
		goto out;
	}

	/* file type already checked */
	filetype = icbtag->file_type;

	/* extract info from our VAT data */
	if (filetype == 0) {
		/* VAT 1.50 format */
		/* definition */
		vat_offset = 0;
		vat_entries = (vat_length-36)/4;
		oldvat_tl = (struct udf_oldvat_tail *)
			(*vat_contents + vat_entries * 4);
		regid_name = (char *) oldvat_tl->id.id;
		error = strncmp(regid_name, "*UDF Virtual Alloc Tbl", 22);
		if (error) {
			pwarn("Possible VAT 1.50 detected without tail\n");
			if (ask_noauto(0, "Accept anyway")) {
				vat_entries = vat_length/4;
				vat_writeout = 1;
				error = 0;
				goto ok;
			}
			pwarn("VAT format 1.50 rejected\n");
			error = ENOENT;
			goto out;
		}

		/*
		 * The following VAT extensions are optional and ignored but
		 * demand a clean VAT write out for sanity.
		 */
		error = udf_extattr_search_intern(dscr, 2048, extstr, &offset, &a_l);
		if (error) {
			/* VAT LVExtension extended attribute missing */
			vat_writeout = 1;
			goto ok;
		}

		implext = (struct impl_extattr_entry *) (ea_start + offset);
		error = udf_impl_extattr_check(implext);
		if (error) {
			/* VAT LVExtension checksum failed */
			vat_writeout = 1;
			goto ok;
		}

		/* paranoia */
		if (a_l != sizeof(*implext) -2 + udf_rw32(implext->iu_l) + sizeof(lvext)) {
			/* VAT LVExtension size doesn't compute */
			vat_writeout = 1;
			goto ok;
		}

		/*
		 * We have found our "VAT LVExtension attribute. BUT due to a
		 * bug in the specification it might not be word aligned so
		 * copy first to avoid panics on some machines (!!)
		 */
		lvextpos = implext->data + udf_rw32(implext->iu_l);
		memcpy(&lvext, lvextpos, sizeof(lvext));

		/* check if it was updated the last time */
		if (udf_rw64(lvext.unique_id_chk) == vat_unique_id) {
			lvinfo->num_files       = lvext.num_files;
			lvinfo->num_directories = lvext.num_directories;
			udf_update_logvolname(lvext.logvol_id);
		} else {
			/* VAT LVExtension out of date */
			vat_writeout = 1;
		}
	} else {
		/* VAT 2.xy format */
		/* definition */
		vat = (struct udf_vat *) (*vat_contents);
		vat_offset  = udf_rw16(vat->header_len);
		vat_entries = (vat_length - vat_offset)/4;

		if (heuristics) {
			if (vat->impl_use_len == 0) {
				uint32_t start_val;
				start_val = udf_rw32(*((uint32_t *) vat->data));
				if (start_val == 0x694d2a00) {
					/* "<0>*Mic"osoft Windows */
					pwarn("Heuristics found corrupted MS Windows VAT\n");
					if (ask(0, "Repair")) {
						vat->impl_use_len = udf_rw16(32);
						vat->header_len = udf_rw16(udf_rw16(vat->header_len) + 32);
						vat_offset += 32;
						vat_writeout = 1;
					}
				}
			}
		}
		assert(lvinfo);
		lvinfo->num_files        = vat->num_files;
		lvinfo->num_directories  = vat->num_directories;
		lvinfo->min_udf_readver  = vat->min_udf_readver;
		lvinfo->min_udf_writever = vat->min_udf_writever;
		lvinfo->max_udf_writever = vat->max_udf_writever;

		udf_update_logvolname(vat->logvol_id);
	}

/* XXX FAULT INJECTION POINT XXX */
//vat_writeout = 1;

ok:
	/* extra sanity checking */
	if (tag_id == TAGID_FENTRY) {
		/* nothing checked as yet */
	} else {
		/*
		 * The following VAT violations are ignored but demand a clean VAT
		 * writeout for sanity
		 */
		if (!is_zero(&dscr->efe.streamdir_icb, sizeof(struct long_ad))) {
			/* VAT specification violation:
			 * 	VAT has no cleared streamdir reference */
			vat_writeout = 1;
		}
		if (!is_zero(&dscr->efe.ex_attr_icb, sizeof(struct long_ad))) {
			/* VAT specification violation:
			 * 	VAT has no cleared extended attribute reference */
			vat_writeout = 1;
		}
		if (dscr->efe.obj_size != dscr->efe.inf_len) {
			/* VAT specification violation:
			 * 	VAT has invalid object size */
			vat_writeout = 1;
		}
	}

	if (!vat_writeout) {
		context.logvol_integrity->lvint_next_unique_id = udf_rw64(vat_unique_id);
		context.logvol_integrity->integrity_type = udf_rw32(UDF_INTEGRITY_CLOSED);
		context.logvol_integrity->time           = *mtime;
	}

	context.unique_id     = vat_unique_id;
	context.vat_allocated = UDF_ROUNDUP(vat_length, context.sector_size);
	context.vat_contents  = *vat_contents;
	context.vat_start     = vat_offset;
	context.vat_size      = vat_offset + vat_entries * 4;

out:
	if (error) {
		free(*vat_contents);
		*vat_contents = NULL;
	}

	return error;
}


#define VAT_BLK 256
static int
udf_search_vat(union udf_pmap *mapping, int log_part)
{
	union dscrptr *vat_candidate, *accepted_vat;
	struct part_desc *pdesc;
	struct mmc_trackinfo *ti, *ti_s;
	uint32_t part_start;
	uint32_t vat_loc, early_vat_loc, late_vat_loc, accepted_vat_loc;
	uint32_t first_possible_vat_location, last_possible_vat_location;
	uint8_t *vat_contents, *accepted_vat_contents;
	int num_tracks, tracknr, found_a_VAT, valid_loc, error;

	/*
	 * Start reading forward in blocks from the first possible vat
	 * location. If not found in this block, start again a bit before
	 * until we get a hit.
	 */

	/* get complete list of all our valid ranges */
	ti_s = calloc(mmc_discinfo.num_tracks, sizeof(struct mmc_trackinfo));
	for (tracknr = 1; tracknr <= mmc_discinfo.num_tracks; tracknr++) {
		ti = &ti_s[tracknr];
		ti->tracknr = tracknr;
		(void) udf_update_trackinfo(ti);
	}

	/* derive our very first track number our base partition covers */
	pdesc = context.partitions[context.data_part];
	part_start = udf_rw32(pdesc->start_loc);
	for (int cnt = 0; cnt < UDF_PARTITIONS; cnt++) {
		pdesc = context.partitions[cnt];
		if (!pdesc)
			continue;
		part_start = MIN(part_start, udf_rw32(pdesc->start_loc));
	}
	num_tracks = mmc_discinfo.num_tracks;
	for (tracknr = 1, ti = NULL; tracknr <= num_tracks; tracknr++) {
		ti = &ti_s[tracknr];
		if ((part_start >= ti->track_start) &&
				(part_start <= ti->track_start + ti->track_size))
			break;
	}
	context.first_ti_partition = *ti;

	first_possible_vat_location = context.first_ti_partition.track_start;
	last_possible_vat_location  = context.last_ti.track_start +
			context.last_ti.track_size -
			context.last_ti.free_blocks + 1;

	/* initial guess is around 16 sectors back */
	late_vat_loc = last_possible_vat_location;
	early_vat_loc = MAX(late_vat_loc - 16, first_possible_vat_location);

	if (!preen)
		printf("Full VAT range search from %d to %d\n",
			first_possible_vat_location,
			last_possible_vat_location);

	vat_writeout = 0;
	accepted_vat = NULL;
	accepted_vat_contents = NULL;
	accepted_vat_loc = 0;
	do {
		vat_loc = early_vat_loc;
		if (!preen) {
			printf("\tChecking range %8d to %8d\n",
					early_vat_loc, late_vat_loc);
			fflush(stdout);
		}
		found_a_VAT = 0;
		while (vat_loc <= late_vat_loc) {
			if (print_info) {
				pwarn("\nchecking for VAT in sector %8d\n", vat_loc);
				print_info = 0;
			}
			/* check if its in readable range */
			valid_loc = 0;
			for (tracknr = 1; tracknr <= num_tracks; tracknr++) {
				ti = &ti_s[tracknr];
				if (!(ti->flags & MMC_TRACKINFO_BLANK) &&
					((vat_loc >= ti->track_start) &&
					    (vat_loc <= ti->track_start + ti->track_size))) {
					valid_loc = 1;
					break;
				}
			}
			if (!valid_loc) {
				vat_loc++;
				continue;
			}

			error = udf_read_dscr_phys(vat_loc, &vat_candidate);
			if (!vat_candidate)
				error = ENOENT;
			if (!error)
				error = udf_check_for_vat(vat_candidate);
			if (error) {
				vat_loc++;	/* walk forward */
				continue;
			}

			if (accepted_vat) {
				/* check if newer vat time stamp is the same */
				if (udf_compare_mtimes(
						udf_file_mtime(vat_candidate),
						udf_file_mtime(accepted_vat)
						) == 0) {
					free(vat_candidate);
					vat_loc++;	/* walk forward */
					continue;
				}
			}

			/* check if its contents are OK */
			error = udf_extract_vat(
					vat_candidate, &vat_contents);
			if (error) {
				/* unlikely */
				// pwarn("Unreadable or malformed VAT encountered\n");
				free(vat_candidate);
				vat_loc++;
				continue;
			}
			/* accept new vat */
			free(accepted_vat);
			free(accepted_vat_contents);

			accepted_vat = vat_candidate;
			accepted_vat_contents = vat_contents;
			accepted_vat_loc = vat_loc;
			vat_candidate = NULL;
			vat_contents  = NULL;

			found_a_VAT = 1;

			vat_loc++;	/* walk forward */
		};

		if (found_a_VAT && accepted_vat) {
			/* VAT accepted */
			if (!preen)
				udf_print_vat_details(accepted_vat);
			if (vat_writeout)
				pwarn("\tVAT accepted but marked dirty\n");
			if (!preen && !vat_writeout)
				pwarn("\tLogical volume integrity state set to CLOSED\n");
			if (!search_older_vat)
				break;
			if (!ask_noauto(0, "\tSearch older VAT"))
				break;
			late_vat_loc  = accepted_vat_loc - 1;
		} else {
			late_vat_loc = early_vat_loc - 1;
		}
		early_vat_loc = first_possible_vat_location;
		if (late_vat_loc > VAT_BLK)
			early_vat_loc = MAX(early_vat_loc, late_vat_loc - VAT_BLK);
	} while (late_vat_loc > first_possible_vat_location);

	if (!preen)
		printf("\n");

	undo_opening_session = 0;

	if (!accepted_vat) {
		if ((context.last_ti.sessionnr > 1) && 
				ask_noauto(0, "Undo opening of last session")) {
			undo_opening_session = 1;
			pwarn("Undoing opening of last session not implemented!\n");
			error = ENOENT;
			goto error_out;
		} else {
			pwarn("No valid VAT found!\n");
			error = ENOENT;
			goto error_out;
		}
	}
	if (last_possible_vat_location - accepted_vat_loc > 16) {
		assert(accepted_vat);
		pwarn("Selected VAT is not the latest or not at the end of "
			"track.\n");
			vat_writeout = 1;
	}

/* XXX FAULT INJECTION POINT XXX */
//vat_writeout = 1;
//udf_update_lvintd(UDF_INTEGRITY_OPEN);

	return 0;

error_out:
	free(accepted_vat);
	free(accepted_vat_contents);

	return error;
}

/* ------------------------- sparables support ------------------------- */

static int
udf_read_spareables(union udf_pmap *mapping, int log_part)
{
	union dscrptr *dscr;
	struct part_map_spare *pms = &mapping->pms;
	uint32_t lb_num;
	int spar, error;

	for (spar = 0; spar < pms->n_st; spar++) {
		lb_num = pms->st_loc[spar];
		error = udf_read_dscr_phys(lb_num, &dscr);
		if (error && !preen)
			pwarn("Error reading spareable table %d\n", spar);
		if (!error && dscr) {
			if (udf_rw16(dscr->tag.id) == TAGID_SPARING_TABLE) {
				free(context.sparing_table);
				context.sparing_table = &dscr->spt;
				dscr = NULL;
				break;	/* we're done */
			}
		}
		free(dscr);
	}
	if (context.sparing_table == NULL)
		return ENOENT;
	return 0;
}

/* ------------------------- metadata support ------------------------- */

static bool
udf_metadata_node_supported(void)
{
	struct extfile_entry   *efe;
	struct short_ad        *short_ad;
	uint32_t len;
	uint32_t flags;
	uint8_t *data_pos;
	int dscr_size, l_ea, l_ad, icbflags, addr_type;

	/* we have to look into the file's allocation descriptors */

	efe = context.meta_file;
	dscr_size = sizeof(struct extfile_entry) - 1;
	l_ea = udf_rw32(efe->l_ea);
	l_ad = udf_rw32(efe->l_ad);

	icbflags = udf_rw16(efe->icbtag.flags);
	addr_type = icbflags & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
	if (addr_type != UDF_ICB_SHORT_ALLOC) {
		warnx("specification violation: metafile not using"
			"short allocs");
		return false;
	}

	data_pos = (uint8_t *) context.meta_file + dscr_size + l_ea;
	short_ad = (struct short_ad *) data_pos;
	while (l_ad > 0) {
		len      = udf_rw32(short_ad->len);
		flags    = UDF_EXT_FLAGS(len);
		if (flags == UDF_EXT_REDIRECT) {
			warnx("implementation limit: no support for "
			      "extent redirections in metadata file");
			return false;
		}
		short_ad++;
		l_ad -= sizeof(struct short_ad);
	}
	/* we passed all of them */
	return true;
}


static int
udf_read_metadata_nodes(union udf_pmap *mapping, int log_part)
{
	union dscrptr *dscr1, *dscr2, *dscr3;
	struct part_map_meta *pmm = &mapping->pmm;
	uint16_t raw_phys_part, phys_part;
	int tagid, file_type, error;

	/*
	 * BUGALERT: some rogue implementations use random physical
	 * partition numbers to break other implementations so lookup
	 * the number.
	 */

	raw_phys_part = udf_rw16(pmm->part_num);
	phys_part = udf_find_raw_phys(raw_phys_part);

	error = udf_read_dscr_virt(layout.meta_file, phys_part, &dscr1);
	if (!error) {
		tagid = udf_rw16(dscr1->tag.id);
		file_type = dscr1->efe.icbtag.file_type;
		if ((tagid != TAGID_EXTFENTRY) ||
				(file_type != UDF_ICB_FILETYPE_META_MAIN))
			error = ENOENT;
	}
	if (error) {
		pwarn("Bad primary metadata file descriptor\n");
		free(dscr1);
		dscr1 = NULL;
	}

	error = udf_read_dscr_virt(layout.meta_mirror, phys_part, &dscr2);
	if (!error) {
		tagid = udf_rw16(dscr2->tag.id);
		file_type = dscr2->efe.icbtag.file_type;
		if ((tagid != TAGID_EXTFENTRY) ||
				(file_type != UDF_ICB_FILETYPE_META_MIRROR))
			error = ENOENT;
	}
	if (error) {
		pwarn("Bad mirror metadata file descriptor\n");
		free(dscr2);
		dscr2 = NULL;
	}

	if ((dscr1 == NULL) && (dscr2 == NULL)) {
		pwarn("No valid metadata file descriptors found!\n");
		return -1;
	}

	error = 0;
	if ((dscr1 == NULL) && dscr2) {
		dscr1 = malloc(context.sector_size);
		memcpy(dscr1, dscr2, context.sector_size);
		dscr1->efe.icbtag.file_type = UDF_ICB_FILETYPE_META_MAIN;
		if (ask(1, "Fix up bad primary metadata file descriptor")) {
			error = udf_write_dscr_virt(dscr1,
					layout.meta_file, phys_part, 1);
		}
	}
	if (dscr1 && (dscr2 == NULL)) {
		dscr2 = malloc(context.sector_size);
		memcpy(dscr2, dscr1, context.sector_size);
		dscr2->efe.icbtag.file_type = UDF_ICB_FILETYPE_META_MIRROR;
		if (ask(1, "Fix up bad mirror metadata file descriptor")) {
			error = udf_write_dscr_virt(dscr2,
					layout.meta_mirror, phys_part, 1);
		}
	}
	if (error)
		pwarn("Copying metadata file descriptor failed, "
		      "trying to continue\n");

	context.meta_file   = &dscr1->efe;
	context.meta_mirror = &dscr2->efe;

	dscr3 = NULL;
	if (layout.meta_bitmap != 0xffffffff) {
		error = udf_read_dscr_virt(layout.meta_bitmap, phys_part, &dscr3);
		if (!error) {
			tagid = udf_rw16(dscr3->tag.id);
			file_type = dscr3->efe.icbtag.file_type;
			if ((tagid != TAGID_EXTFENTRY) ||
					(file_type != UDF_ICB_FILETYPE_META_BITMAP))
				error = ENOENT;
		}
		if (error) {
			pwarn("Bad metadata bitmap file descriptor\n");
			free(dscr3);
			dscr3 = NULL;
		}

		if (dscr3 == NULL) {
			pwarn("implementation limit: can't repair missing or "
			      "damaged metadata bitmap descriptor\n");
			return -1;
		}

		context.meta_bitmap = &dscr3->efe;
	}

	/* TODO early check if meta_file has allocation extent redirections */
	if (!udf_metadata_node_supported())
		return EINVAL;

	return 0;
}

/* ------------------------- VDS readin ------------------------- */

/* checks if the VDS information is correct and complete */
static int
udf_process_vds(void) {
	union dscrptr *dscr;
	union udf_pmap *mapping;
	struct part_desc *pdesc;
	struct long_ad fsd_loc;
	uint8_t *pmap_pos;
	char *domain_name, *map_name;
	const char *check_name;	
	int pmap_stype, pmap_size;
	int pmap_type, log_part, phys_part, raw_phys_part; //, maps_on;
	int n_pm, n_phys, n_virt, n_spar, n_meta;
	int len, error;

	/* we need at least an anchor (trivial, but for safety) */
	if (context.anchors[0] == NULL) {
		pwarn("sanity check: no anchors?\n");
		return EINVAL;
	}

	/* we need at least one primary and one logical volume descriptor */
	if ((context.primary_vol == NULL) || (context.logical_vol) == NULL) {
		pwarn("sanity check: missing primary or missing logical volume\n");
		return EINVAL;
	}

	/* we need at least one partition descriptor */
	if (context.partitions[0] == NULL) {
		pwarn("sanity check: missing partition descriptor\n");
		return EINVAL;
	}

	/* check logical volume sector size versus device sector size */
	if (udf_rw32(context.logical_vol->lb_size) != context.sector_size) {
		pwarn("sanity check: lb_size != sector size\n");
		return EINVAL;
	}

	/* check domain name, should never fail */
	domain_name = (char *) context.logical_vol->domain_id.id;
	if (strncmp(domain_name, "*OSTA UDF Compliant", 20)) {
		pwarn("sanity check: disc not OSTA UDF Compliant, aborting\n");
		return EINVAL;
	}

	/* retrieve logical volume integrity sequence */
	udf_retrieve_lvint();

	/* check if we support this disc, ie less or equal to 0x250 */
	if (udf_rw16(context.logvol_info->min_udf_writever) > 0x250) {
		pwarn("implementation limit: minimum write version UDF 2.60 "
		      "and on are not supported\n");
		return EINVAL;
	}

	/*
	 * check logvol mappings: effective virt->log partmap translation
	 * check and recording of the mapping results. Saves expensive
	 * strncmp() in tight places.
	 */
	n_pm = udf_rw32(context.logical_vol->n_pm);   /* num partmaps         */
	pmap_pos =  context.logical_vol->maps;

	if (n_pm > UDF_PMAPS) {
		pwarn("implementation limit: too many logvol mappings\n");
		return EINVAL;
	}

	/* count types and set partition numbers */
	context.data_part = context.metadata_part = context.fids_part = 0;
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
			context.data_part     = log_part;
			context.metadata_part = log_part;
			context.fids_part     = log_part;
			break;
		case 2: /* virtual/sparable/meta mapping */
			map_name  = (char *) mapping->pm2.part_id.id;
			/* volseq  = udf_rw16(mapping->pm2.vol_seq_num); */
			raw_phys_part = udf_rw16(mapping->pm2.part_num);
			pmap_type = UDF_VTOP_TYPE_UNKNOWN;
			len = UDF_REGID_ID_SIZE;

			check_name = "*UDF Virtual Partition";
			if (strncmp(map_name, check_name, len) == 0) {
				pmap_type = UDF_VTOP_TYPE_VIRT;
				n_virt++;
				context.metadata_part = log_part;
				context.format_flags |= FORMAT_VAT;
				break;
			}
			check_name = "*UDF Sparable Partition";
			if (strncmp(map_name, check_name, len) == 0) {
				pmap_type = UDF_VTOP_TYPE_SPAREABLE;
				n_spar++;
				layout.spareable_blockingnr = udf_rw16(mapping->pms.packet_len);

				context.data_part     = log_part;
				context.metadata_part = log_part;
				context.fids_part     = log_part;
				context.format_flags |= FORMAT_SPAREABLE;
				break;
			}
			check_name = "*UDF Metadata Partition";
			if (strncmp(map_name, check_name, len) == 0) {
				pmap_type = UDF_VTOP_TYPE_META;
				n_meta++;
				layout.meta_file	= udf_rw32(mapping->pmm.meta_file_lbn);
				layout.meta_mirror	= udf_rw32(mapping->pmm.meta_mirror_file_lbn);
				layout.meta_bitmap	= udf_rw32(mapping->pmm.meta_bitmap_file_lbn);
				layout.meta_blockingnr	= udf_rw32(mapping->pmm.alloc_unit_size);
				layout.meta_alignment	= udf_rw16(mapping->pmm.alignment_unit_size);
				/* XXX metadata_flags in mapping->pmm.flags? XXX */

				context.metadata_part = log_part;
				context.fids_part     = log_part;
				context.format_flags |= FORMAT_META;
				break;
			}
			break;
		default:
			return EINVAL;
		}

		/*
		 * BUGALERT: some rogue implementations use random physical
		 * partition numbers to break other implementations so lookup
		 * the number.
		 */
		phys_part = udf_find_raw_phys(raw_phys_part);

		if (phys_part == UDF_PARTITIONS) {
			pwarn("implementation limit: too many partitions\n");
			return EINVAL;
		}
		if (pmap_type == UDF_VTOP_TYPE_UNKNOWN) {
			pwarn("implementation limit: encountered unknown "
				"logvol mapping `%s`!\n", map_name);
			return EINVAL;
		}

		context.vtop   [log_part] = phys_part;
		context.vtop_tp[log_part] = pmap_type;

		pmap_pos += pmap_size;
	}
	/* not winning the beauty contest */
	context.vtop_tp[UDF_VTOP_RAWPART] = UDF_VTOP_TYPE_RAW;

	/* test some basic UDF assertions/requirements */
	if ((n_virt > 1) || (n_spar > 1) || (n_meta > 1)) {
		pwarn("Sanity check: format error, more than one "
		      "virtual, sparable or meta mapping\n");
		return EINVAL;
	}

	if (n_virt) {
		if ((n_phys == 0) || n_spar || n_meta) {
			pwarn("Sanity check: format error, no backing for "
			      "virtual partition\n");
			return EINVAL;
		}
	}
	if (n_spar + n_phys == 0) {
		pwarn("Sanity check: can't combine a sparable and a "
		      "physical partition\n");
		return EINVAL;
	}

	/* print format type as derived */
	if (!preen) {
		char bits[255];
		snprintb(bits, sizeof(bits), FORMAT_FLAGBITS, context.format_flags);
		printf("Format flags %s\n\n", bits);
	}

	/* read supporting tables */
	pmap_pos =  context.logical_vol->maps;
	for (log_part = 0; log_part < n_pm; log_part++) {
		mapping = (union udf_pmap *) pmap_pos;
		pmap_size  = pmap_pos[1];
		switch (context.vtop_tp[log_part]) {
		case UDF_VTOP_TYPE_PHYS :
			/* nothing */
			break;
		case UDF_VTOP_TYPE_VIRT :
			/* search and load VAT */
			error = udf_search_vat(mapping, log_part);
			if (error) {
				pwarn("Couldn't find virtual allocation table\n");
				return ENOENT;
			}
			break;
		case UDF_VTOP_TYPE_SPAREABLE :
			/* load one of the sparable tables */
			error = udf_read_spareables(mapping, log_part);
			if (error) {
				pwarn("Couldn't load sparable blocks tables\n");
				return ENOENT;
			}
			break;
		case UDF_VTOP_TYPE_META :
			/* load the associated file descriptors */
			error = udf_read_metadata_nodes(mapping, log_part);
			if (error) {
				pwarn("Couldn't read in the metadata descriptors\n");
				return ENOENT;
			}

			/*
			 * We have to extract the partition size from the meta
			 * data file length
			 */
			context.part_size[log_part] =
				udf_rw32(context.meta_file->inf_len) / context.sector_size;
			break;
		default:
			break;
		}
		pmap_pos += pmap_size;
	}

	/*
	 * Free/unallocated space bitmap readin delayed; the FS might be
	 * closed already; no need to read in copious amount of data only to
	 * not use it later.
	 *
	 * For now, extract partition sizes in our context
	 */
	for (int cnt = 0; cnt < UDF_PARTITIONS; cnt++) {
		pdesc = context.partitions[cnt];
		if (!pdesc)
			continue;

		context.part_size[cnt] = udf_rw32(pdesc->part_len);
		context.part_unalloc_bits[cnt] = NULL;
	}

	/* read file set descriptor */
	fsd_loc = context.logical_vol->lv_fsd_loc;
	error = udf_read_dscr_virt(
			udf_rw32(fsd_loc.loc.lb_num),
			udf_rw16(fsd_loc.loc.part_num), &dscr);
	if (error) {
		pwarn("Couldn't read in file set descriptor\n");
		pwarn("implementation limit: can't fix this\n");
		return ENOENT;
	}
	if (udf_rw16(dscr->tag.id) != TAGID_FSD) {
		pwarn("Expected fsd at (p %d, lb %d)\n",
				udf_rw16(fsd_loc.loc.part_num),
				udf_rw32(fsd_loc.loc.lb_num));
		pwarn("File set descriptor not pointing to a file set!\n");
		return ENOENT;
	}
	context.fileset_desc = &dscr->fsd;

	/* signal its OK for now */
	return 0;
}


#define UDF_UPDATE_DSCR(name, dscr) \
	if (name) {\
		free (name); \
		updated = 1; \
	} \
	name = calloc(1, dscr_size); \
	memcpy(name, dscr, dscr_size);

static void
udf_process_vds_descriptor(union dscrptr *dscr, int dscr_size) {
	struct pri_vol_desc *pri;
	struct logvol_desc *lvd;
	uint16_t raw_phys_part, phys_part;
	int updated = 0;

	switch (udf_rw16(dscr->tag.id)) {
	case TAGID_PRI_VOL :		/* primary partition */
		UDF_UPDATE_DSCR(context.primary_vol, dscr);
		pri = context.primary_vol;

		context.primary_name = malloc(32);
		context.volset_name  = malloc(128);

		udf_to_unix_name(context.volset_name, 32, pri->volset_id, 32,
			&pri->desc_charset);
		udf_to_unix_name(context.primary_name, 128, pri->vol_id, 128,
			&pri->desc_charset);

		if (!preen && !updated) {
			pwarn("Volume set       `%s`\n", context.volset_name);
			pwarn("Primary volume   `%s`\n", context.primary_name);
		}
		break;
	case TAGID_LOGVOL :		/* logical volume    */
		UDF_UPDATE_DSCR(context.logical_vol, dscr);
		/* could check lvd->domain_id */
		lvd = context.logical_vol;
		context.logvol_name = malloc(128);

		udf_to_unix_name(context.logvol_name, 128, lvd->logvol_id, 128,
			&lvd->desc_charset);

		if (!preen && !updated)
			pwarn("Logical volume   `%s`\n", context.logvol_name);
		break;
	case TAGID_UNALLOC_SPACE :	/* unallocated space */
		UDF_UPDATE_DSCR(context.unallocated, dscr);
		break;
	case TAGID_IMP_VOL :		/* implementation    */
		UDF_UPDATE_DSCR(context.implementation, dscr);
		break;
	case TAGID_PARTITION :		/* partition(s)	     */
		/* not much use if its not allocated */
		if ((udf_rw16(dscr->pd.flags) & UDF_PART_FLAG_ALLOCATED) == 0) {
			pwarn("Ignoring unallocated partition\n");
			break;
		}
		raw_phys_part = udf_rw16(dscr->pd.part_num);
		phys_part = udf_find_raw_phys(raw_phys_part);

		if (phys_part >= UDF_PARTITIONS) {
			pwarn("Too many physical partitions, ignoring\n");
			break;
		}
		UDF_UPDATE_DSCR(context.partitions[phys_part], dscr);
		break;
	case TAGID_TERM :		/* terminator        */
		break;
	case TAGID_VOL :		/* volume space ext  */
		pwarn("Ignoring VDS extender\n");
		break;
	default :
		pwarn("Unknown VDS type %d found, ignored\n",
			udf_rw16(dscr->tag.id));
	}
}


static void
udf_read_vds_extent(union dscrptr *dscr, int vds_size) {
	uint8_t *pos;
	int sector_size = context.sector_size;
	int dscr_size;

	pos = (uint8_t *) dscr;
	while (vds_size) {
		/* process the descriptor */
		dscr = (union dscrptr *) pos;

		/* empty block terminates */
		if (is_zero(dscr, sector_size))
			return;

		/* terminator terminates */
		if (udf_rw16(dscr->tag.id) == TAGID_TERM)
			return;

		if (udf_check_tag(dscr))
			pwarn("Bad descriptor sum in vds, ignoring\n");

		dscr_size = udf_tagsize(dscr, sector_size);
		if (udf_check_tag_payload(dscr, dscr_size))
			pwarn("Bad descriptor CRC in vds, ignoring\n");

		udf_process_vds_descriptor(dscr, dscr_size);

		pos      += dscr_size;
		vds_size -= dscr_size;
	}
}


static int
udf_copy_VDS_area(void *destbuf, void *srcbuf)
{
	pwarn("TODO implement VDS copy area, signalling success\n");
	return 0;
}


/* XXX why two buffers and not just read descritor by descriptor XXX */
static int
udf_check_VDS_areas(void) {
	union dscrptr *vds1_buf, *vds2_buf;
	int vds1_size, vds2_size;
	int error, error1, error2;

	vds1_size = layout.vds1_size * context.sector_size;
	vds2_size = layout.vds2_size * context.sector_size;
	vds1_buf = calloc(1, vds1_size);
	vds2_buf = calloc(1, vds2_size);
	assert(vds1_buf); assert(vds2_buf);

	error1 = udf_read_phys(vds1_buf, layout.vds1, layout.vds1_size);
	error2 = udf_read_phys(vds2_buf, layout.vds2, layout.vds2_size);

	if (error1 && error2) {
		pwarn("Can't read both volume descriptor areas!\n");
		return -1;
	}

	if (!error1) {
		/* retrieve data from VDS 1 */
		udf_read_vds_extent(vds1_buf, vds1_size);
		context.vds_buf  = vds1_buf;
		context.vds_size = vds1_size;
		free(vds2_buf);
	}
	if (!error2) {
		/* retrieve data from VDS 2 */
		udf_read_vds_extent(vds2_buf, vds2_size);
		context.vds_buf  = vds2_buf;
		context.vds_size = vds2_size;
		free(vds1_buf);
	}
	/* check if all is correct and complete */
	error = udf_process_vds();
	if (error)
		return error;

	/* TODO check if both area's are logically the same */
	error = 0;
	if (!error1 && error2) {
		/* first OK, second faulty */
		pwarn("Backup volume descriptor missing or damaged\n");
		if (context.format_flags & FORMAT_SEQUENTIAL) {
			pwarn("Can't fixup backup volume descriptor on "
			      "SEQUENTIAL media\n");
		} else if (ask(1, "Fixup backup volume descriptor")) {
			error = udf_copy_VDS_area(vds2_buf, vds1_buf);
			pwarn("\n");
		}
	}
	if (error1 && !error2) {
		/* second OK, first faulty */
		pwarn("Primary volume descriptor missing or damaged\n");
		if (context.format_flags & FORMAT_SEQUENTIAL) {
			pwarn("Can't fix up primary volume descriptor on "
			      "SEQUENTIAL media\n");
		} else if (ask(1, "Fix up primary volume descriptor")) {
			error = udf_copy_VDS_area(vds1_buf, vds2_buf);
		}
	}
	if (error)
		pwarn("copying VDS areas failed!\n");
	if (!preen)
		printf("\n");

	return error;
}

/* --------------------------------------------------------------------- */

static int
udf_prepare_writing(void)
{
	union dscrptr *zero_dscr, *dscr;
	struct mmc_trackinfo ti;
	uint32_t first_lba, loc;
	int sector_size = context.sector_size;
	int error;

	error = udf_prepare_disc();
	if (error) {
		pwarn("*** Preparing disc for writing failed!\n");
		return error;
	}

	/* if we are not on sequential media, we're done */
	if ((mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) == 0)
		return 0;
	assert(context.format_flags & FORMAT_VAT);

	/* if the disc is full, we drop back to read only */
	if (mmc_discinfo.disc_state == MMC_STATE_FULL)
		rdonly = 1;
	if (rdonly)
		return 0;

	/* check if we need to open the last track */
	ti.tracknr = mmc_discinfo.last_track_last_session;
	error = udf_update_trackinfo(&ti);
	if (error)
		return error;
	if (!(ti.flags & MMC_TRACKINFO_BLANK) && 
	     (ti.flags & MMC_TRACKINFO_NWA_VALID)) {
		/*
		 * Not closed; translate next_writable to a position relative to our
		 * backing partition
		 */
		context.alloc_pos[context.data_part] = ti.next_writable -
			udf_rw32(context.partitions[context.data_part]->start_loc);
		wrtrack_skew = ti.next_writable % layout.blockingnr;
		return 0;
	}
	assert(ti.flags & MMC_TRACKINFO_NWA_VALID);

	/* just in case */
	udf_suspend_writing();

	/* 'add' a new track */
	udf_update_discinfo();
	memset(&context.last_ti, 0, sizeof(struct mmc_trackinfo));
	context.last_ti.tracknr = mmc_discinfo.first_track_last_session;
	(void) udf_update_trackinfo(&context.last_ti);

	assert(mmc_discinfo.last_session_state == MMC_STATE_EMPTY);
	first_lba = context.last_ti.track_start;
	wrtrack_skew = context.last_ti.track_start % layout.blockingnr;

	/*
	 * location of iso9660 vrs is defined as first sector AFTER 32kb,
	 * minimum `sector size' 2048
	 */
	layout.iso9660_vrs = ((32*1024 + sector_size - 1) / sector_size)
		+ first_lba;

	/* anchor starts at specified offset in sectors */
	layout.anchors[0] = first_lba + 256;

	/* ready for appending, write preamble, we are using overwrite here! */
	if ((zero_dscr = calloc(1, context.sector_size)) == NULL)
		return ENOMEM;
	loc = first_lba;
	for (; loc < first_lba + 256; loc++) {
		if ((error = udf_write_sector(zero_dscr, loc))) {
			free(zero_dscr);
			return error;
		}
	}
	free(zero_dscr);

	/* write new ISO9660 volume recognition sequence */
	if ((error = udf_write_iso9660_vrs())) {
		pwarn("internal error: can't write iso966 VRS in new session!\n");
		rdonly = 1;
		return error;
	}

	/* write out our old anchor, VDS spaces will be reused */
	assert(context.anchors[0]);
	dscr = (union dscrptr *) context.anchors[0];
	loc  = layout.anchors[0];
	if ((error = udf_write_dscr_phys(dscr, loc, 1))) {
		pwarn("internal error: can't write anchor in new session!\n");
		rdonly = 1;
		return error;
	}

	context.alloc_pos[context.data_part] = first_lba + 257 -
		udf_rw32(context.partitions[context.data_part]->start_loc);

	return 0;
}


static int
udf_close_volume_vat(void)
{
	int integrity_type;

	/* only write out when its open */
	integrity_type = udf_rw32(context.logvol_integrity->integrity_type);
	if (integrity_type == UDF_INTEGRITY_CLOSED)
		return 0;

	if (!preen)
		printf("\n");
	if (!ask(1, "Write out modifications"))
		return 0;

	/* writeout our VAT contents */
	udf_allow_writing();
	return udf_writeout_VAT();
}


static int
udf_close_volume(void)
{
	struct part_desc       *part;
	struct part_hdr_desc   *phd;
	struct logvol_int_desc *lvid;
	struct udf_logvol_info *lvinfo;
	struct logvol_desc     *logvol;
	uint32_t bitmap_len, bitmap_lb, bitmap_numlb;
	int i, equal, error;

	lvid = context.logvol_integrity;
	logvol = context.logical_vol;
	lvinfo = context.logvol_info;
	assert(lvid);
	assert(logvol);
	assert(lvinfo);

	/* check our highest unique id */
	if (context.unique_id > udf_rw64(lvid->lvint_next_unique_id)) {
		pwarn("Last unique id updated from %ld to %ld : FIXED\n",
				udf_rw64(lvid->lvint_next_unique_id),
				context.unique_id);
		open_integrity = 1;
	}

	/* check file/directory counts */
	if (context.num_files != udf_rw32(lvinfo->num_files)) {
		pwarn("Number of files corrected from %d to %d : FIXED\n",
				udf_rw32(lvinfo->num_files),
				context.num_files);
		open_integrity = 1;
	}
	if (context.num_directories != udf_rw32(lvinfo->num_directories)) {
		pwarn("Number of directories corrected from %d to %d : FIXED\n",
				udf_rw32(lvinfo->num_directories),
				context.num_directories);
		open_integrity = 1;
	}

	if (vat_writeout)
		open_integrity = 1;

	if (open_integrity)
		udf_update_lvintd(UDF_INTEGRITY_OPEN);

	if (context.format_flags & FORMAT_VAT)
		return udf_close_volume_vat();

	/* adjust free space accounting! */
	for (i = 0; i < UDF_PARTITIONS; i++) {
		part = context.partitions[i];
		if (!part)
			continue;
		phd = &part->pd_part_hdr;
		bitmap_len = udf_rw32(phd->unalloc_space_bitmap.len);
		bitmap_lb  = udf_rw32(phd->unalloc_space_bitmap.lb_num);

		if (bitmap_len == 0) {
			error = 0;
			continue;
		}

		equal = memcmp( recorded_part_unalloc_bits[i],
				context.part_unalloc_bits[i],
				bitmap_len) == 0;

		if (!equal || (context.part_free[i] != recorded_part_free[i])) {
			if (!equal)
				pwarn("Calculated bitmap for partition %d not equal "
				      "to recorded one : FIXED\n", i);
			pwarn("Free space on partition %d corrected "
			      "from %d to %d blocks : FIXED\n", i,
			      recorded_part_free[i],
			      context.part_free[i]);

			/* write out updated free space map */
			pwarn("Updating unallocated bitmap for partition\n");
			if (!preen)
				printf("Writing free space map "
				       "for partition %d\n", i);
			error = 0;
			if (context.vtop_tp[i] == UDF_VTOP_TYPE_META) {
				if (context.meta_bitmap) {
					assert(i == context.metadata_part);
					error = udf_process_file(
						(union dscrptr *) context.meta_bitmap,
						context.data_part,
						(uint8_t **) &(context.part_unalloc_bits[i]),
						AD_SAVE_FILE, NULL);
				}
			} else {
				bitmap_numlb = udf_bytes_to_sectors(bitmap_len);
				error = udf_write_dscr_virt(
					(union dscrptr *) context.part_unalloc_bits[i],
					bitmap_lb,
					i,
					bitmap_numlb);
			}
			if (error)
				pwarn("Updating unallocated bitmap failed, "
				      "continuing\n");
			udf_update_lvintd(UDF_INTEGRITY_OPEN);
		}
	}

	/* write out the logical volume integrity sequence */
	error = udf_writeout_lvint();

	return error;
}

/* --------------------------------------------------------------------- */

/*
 * Main part of file system checking.
 *
 * Walk the entire directory tree and check all link counts and rebuild the
 * free space map (if present) on the go.
 */

static struct udf_fsck_node *
udf_new_fsck_node(struct udf_fsck_node *parent, struct long_ad *loc, char *fname)
{
	struct udf_fsck_node *this;
	this = calloc(1, sizeof(struct udf_fsck_node));
	if (!this)
		return NULL;

	this->parent = parent;
	this->fname = strdup(fname);
	this->loc = *loc;
	this->fsck_flags = 0;

	this->link_count = 0;
	this->found_link_count = 0;

	return this;
}


static void
udf_node_path_piece(char *pathname, struct udf_fsck_node *node)
{
	if (node->parent) {
		udf_node_path_piece(pathname, node->parent);
		if (node->fsck_flags & FSCK_NODE_FLAG_STREAM_DIR)
			strcat(pathname, "");
		else
			strcat(pathname, "/");
	}
	strcat(pathname, node->fname);
}


static char *
udf_node_path(struct udf_fsck_node *node)
{
	static char pathname[MAXPATHLEN + 10];

	strcpy(pathname, "`");
	if (node->parent)
		udf_node_path_piece(pathname, node);
	else
		strcat(pathname, "/");
	strcat(pathname, "'");

	return pathname;
}


static void
udf_recursive_keep(struct udf_fsck_node *node)
{
	while (node->parent) {
		node = node->parent;
		node->fsck_flags |= FSCK_NODE_FLAG_KEEP;
	}
}


static int
udf_quick_check_fids(struct udf_fsck_node *node, union dscrptr *dscr)
{
	struct udf_fsck_fid_context fid_context;
	int error;

	fid_context.fid_offset = 0;
	fid_context.data_left = node->found.inf_len;
	error = udf_process_file(dscr, context.fids_part,
			&node->directory,
			AD_CHECK_FIDS,
			&fid_context);

	return error;
}


/* read descriptor at node's location */
static int
udf_read_node_dscr(struct udf_fsck_node *node, union dscrptr **dscrptr)
{
	*dscrptr = NULL;
	return udf_read_dscr_virt(
			udf_rw32(node->loc.loc.lb_num),
			udf_rw16(node->loc.loc.part_num),
			dscrptr);
}


static int
udf_extract_node_info(struct udf_fsck_node *node, union dscrptr *dscr,
		int be_quiet)
{
	struct icb_tag       *icb = NULL;
	struct file_entry    *fe  = NULL;
	struct extfile_entry *efe = NULL;
	int ad_type, error;

	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe = (struct file_entry *) dscr;
		icb = &fe->icbtag;
		node->declared.inf_len     = udf_rw64(fe->inf_len);
		node->declared.obj_size    = udf_rw64(fe->inf_len);
		node->declared.logblks_rec = udf_rw64(fe->logblks_rec);
		node->link_count           = udf_rw16(fe->link_cnt);
		node->unique_id            = udf_rw64(fe->unique_id);

/* XXX FAULT INJECTION POINT XXX */
//if (fe->unique_id == 33) { return ENOENT;}

	}
	if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe = (struct extfile_entry *) dscr;
		icb = &efe->icbtag;
		node->declared.inf_len     = udf_rw64(efe->inf_len);
		node->declared.obj_size    = udf_rw64(efe->obj_size);
		node->declared.logblks_rec = udf_rw64(efe->logblks_rec);
		node->link_count           = udf_rw16(efe->link_cnt);
		node->unique_id            = udf_rw64(efe->unique_id);
		node->streamdir_loc = efe->streamdir_icb;
		if (node->streamdir_loc.len)
			node->fsck_flags |= FSCK_NODE_FLAG_HAS_STREAM_DIR;

/* XXX FAULT INJECTION POINT XXX */
//if (efe->unique_id == 0x891) { return ENOENT;}

	}

	if (!fe && !efe) {
//printf("NOT REFERENCING AN FE/EFE!\n");
		return ENOENT;
	}

	if (node->unique_id >= context.unique_id)
		context.unique_id = node->unique_id+1;

	ad_type = udf_rw16(icb->flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
	if ((ad_type != UDF_ICB_INTERN_ALLOC) &&
			(ad_type != UDF_ICB_SHORT_ALLOC) &&
			(ad_type != UDF_ICB_LONG_ALLOC)) {
		pwarn("%s : unknown allocation type\n",
				udf_node_path(node));
		return EINVAL;
	}

	bzero(&node->found, sizeof(node->found));
	error = udf_process_file(dscr, udf_rw16(node->loc.loc.part_num), NULL,
			AD_GATHER_STATS, (void *) &node->found);

	switch (icb->file_type) {
	case UDF_ICB_FILETYPE_RANDOMACCESS :
	case UDF_ICB_FILETYPE_BLOCKDEVICE :
	case UDF_ICB_FILETYPE_CHARDEVICE :
	case UDF_ICB_FILETYPE_FIFO :
	case UDF_ICB_FILETYPE_SOCKET :
	case UDF_ICB_FILETYPE_SYMLINK :
	case UDF_ICB_FILETYPE_REALTIME :
		break;
	default:
		/* unknown or unsupported file type, TODO clearing? */
		free(dscr);
		pwarn("%s : specification violation, unknown file type %d\n",
			udf_node_path(node), icb->file_type);
		return ENOENT;
	case UDF_ICB_FILETYPE_STREAMDIR :
	case UDF_ICB_FILETYPE_DIRECTORY :
		/* read in the directory contents */
		error = udf_readin_file(dscr, udf_rw16(node->loc.loc.part_num),
				&node->directory, NULL);

/* XXX FAULT INJECTION POINT XXX */
//if (dscr->efe.unique_id == 109) node->directory[125] = 0xff;
//if (dscr->efe.unique_id == 310) memset(node->directory+1024, 0, 300);

		if (error && !be_quiet) {
			pwarn("%s : directory has read errors\n",
				udf_node_path(node));
			if (ask(0, "Directory could be fixed or cleared. "
				   "Wipe defective directory")) {
				return ENOENT;
			}
			udf_recursive_keep(node);
			node->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
		}
		node->fsck_flags |= FSCK_NODE_FLAG_DIRECTORY;
		error = udf_quick_check_fids(node, dscr);
		if (error) {
			if (!(node->fsck_flags & FSCK_NODE_FLAG_REPAIRDIR))
				pwarn("%s : directory file entries need repair\n",
					udf_node_path(node));
			udf_recursive_keep(node);
			node->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
		}
	}

/* XXX FAULT INJECTION POINT XXX */
//if (fe->unique_id == 0) node->link_count++;
//if (efe->unique_id == 0) node->link_count++;
//if (efe->unique_id == 772) { node->declared.inf_len += 205; node->declared.obj_size -= 0; }

	return 0;
}


static void
udf_fixup_lengths_pass1(struct udf_fsck_node *node, union dscrptr *dscr)
{
	int64_t diff;

	/* file length check */
	diff = node->found.inf_len - node->declared.inf_len;
	if (diff) {
		pwarn("%s : recorded information length incorrect: "
			"%lu instead of declared %lu\n",
			udf_node_path(node),
			node->found.inf_len, node->declared.inf_len);
			node->declared.inf_len = node->found.inf_len;
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
	}

	/* recorded logical blocks count check */
	diff = node->found.logblks_rec - node->declared.logblks_rec;
	if (diff) {
		pwarn("%s : logical blocks recorded incorrect: "
		      "%lu instead of declared %lu, fixing\n",
			udf_node_path(node),
			node->found.logblks_rec, node->declared.logblks_rec);
		node->declared.logblks_rec = node->found.logblks_rec;
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
	}

	/* tally object sizes for streamdirs */
	node->found.obj_size = node->found.inf_len;
	if (node->fsck_flags & FSCK_NODE_FLAG_STREAM_ENTRY) {
		assert(node->parent);		/* streamdir itself */
		if (node->parent->parent)
			node->parent->parent->found.obj_size +=
				node->found.inf_len;
	}

	/* check descriptor CRC length */
	if (udf_rw16(dscr->tag.desc_crc_len) !=
			udf_tagsize(dscr, 1) - sizeof(struct desc_tag)) {
		pwarn("%s : node file descriptor CRC length mismatch; "
			"%d declared, %ld expected\n",
			udf_node_path(node), udf_rw16(dscr->tag.desc_crc_len),
			udf_tagsize(dscr, 1) - sizeof(struct desc_tag));
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
	}
}


static void
udf_node_pass1_add_entry(struct udf_fsck_node *node,
		struct fileid_desc *fid, struct dirent *dirent)
{
	struct udf_fsck_node *leaf_node;
	int entry;

	/* skip deleted FID entries */
	if (fid->file_char & UDF_FILE_CHAR_DEL)
		return;

	if (udf_rw32(fid->icb.loc.lb_num) == 0) {
		pwarn("%s : FileID entry `%s` has invalid location\n",
				udf_node_path(node), dirent->d_name);
		udf_recursive_keep(node);
		if (node->parent)
			node->parent->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
		return;
	}

	/* increase parent link count */
	if (fid->file_char & UDF_FILE_CHAR_PAR) {
		if (node->parent)
			node->parent->found_link_count++;
		return;
	}

	/* lookup if we already know this node */
	leaf_node = udf_node_lookup(&fid->icb);
	if (leaf_node) {
		/* got a hard link! */
		leaf_node->found_link_count++;
		return;
	}

	/* create new node */
	leaf_node = udf_new_fsck_node(
			node, &fid->icb, dirent->d_name);
	if (node->fsck_flags & FSCK_NODE_FLAG_STREAM_DIR)
		leaf_node->fsck_flags |= FSCK_NODE_FLAG_STREAM_ENTRY;

	TAILQ_INSERT_TAIL(&fs_nodes, leaf_node, next);
	entry = udf_calc_node_hash(&fid->icb);
	LIST_INSERT_HEAD(&fs_nodes_hash[entry], leaf_node, next_hash);
}


static void
udf_node_pass1_add_streamdir_entry(struct udf_fsck_node *node)
{
	struct udf_fsck_node *leaf_node;
	int entry;

	/* check for recursion */
	if (node->fsck_flags & FSCK_NODE_FLAG_STREAM) {
		/* recursive streams are not allowed by spec */
		pwarn("%s : specification violation, recursive stream dir\n",
			udf_node_path(node));
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_WIPE_STREAM_DIR;
		return;
	}

	/* lookup if we already know this node */
	leaf_node = udf_node_lookup(&node->streamdir_loc);
	if (leaf_node) {
		pwarn("%s : specification violation, hardlinked streamdir\n",
			udf_node_path(leaf_node));
		udf_recursive_keep(node);
		node->fsck_flags |= FSCK_NODE_FLAG_WIPE_STREAM_DIR;
		return;
	}

	/* create new node */
	leaf_node = udf_new_fsck_node(
			node, &node->streamdir_loc, strdup(""));
	leaf_node->fsck_flags |= FSCK_NODE_FLAG_STREAM_DIR;

	/* streamdirs have link count 0 : ECMA 4/14.9.6 */
	leaf_node->found_link_count--;

	/* insert in to lists */
	TAILQ_INSERT_TAIL(&fs_nodes, leaf_node, next);
	entry = udf_calc_node_hash(&node->streamdir_loc);
	LIST_INSERT_HEAD(&fs_nodes_hash[entry], leaf_node, next_hash);
}


static int
udf_process_node_pass1(struct udf_fsck_node *node, union dscrptr *dscr)
{
	struct fileid_desc *fid;
	struct dirent dirent;
	struct charspec osta_charspec;
	int64_t fpos, new_length, rest_len;
	uint32_t fid_len;
	uint8_t *bpos;
	int isdir;
	int error;

	isdir = node->fsck_flags & FSCK_NODE_FLAG_DIRECTORY;

	/* keep link count */
	node->found_link_count++;

	if (isdir) {
		assert(node->directory);
		udf_rebuild_fid_stream(node, &new_length);
		node->found.inf_len = new_length;
		rest_len = new_length;
	}

	udf_fixup_lengths_pass1(node, dscr);

	/* check UniqueID */
	if (node->parent) {
		if (node->fsck_flags & FSCK_NODE_FLAG_STREAM) {

/* XXX FAULT INJECTION POINT XXX */
//node->unique_id = 0xdeadbeefcafe;

			if (node->unique_id != node->parent->unique_id) {
				pwarn("%s : stream file/dir UniqueID mismatch "
				      "with parent\n",
						udf_node_path(node));
				/* do the work here prematurely for our siblings */
				udf_recursive_keep(node);
				node->unique_id = node->parent->unique_id;
				node->fsck_flags |= FSCK_NODE_FLAG_COPY_PARENT_ID |
					FSCK_NODE_FLAG_DIRTY;
				assert(node->parent);
				node->parent->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
			}
		} else if (node->unique_id < 16) {
			pwarn("%s : file has bad UniqueID\n",
					udf_node_path(node));
			udf_recursive_keep(node);
			node->fsck_flags |= FSCK_NODE_FLAG_NEW_UNIQUE_ID;
			assert(node->parent);
			node->parent->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
		}
	} else {
		/* rootdir */
		if (node->unique_id != 0) {
			pwarn("%s : has bad UniqueID, has to be zero\n",
					udf_node_path(node));
			udf_recursive_keep(node);
			node->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
		}
	}

	/* add streamdir if present */
	if (node->fsck_flags & FSCK_NODE_FLAG_HAS_STREAM_DIR)
		udf_node_pass1_add_streamdir_entry(node);

	/* add all children */
	if (isdir) {
		node->fsck_flags |= FSCK_NODE_FLAG_PAR_NOT_FOUND;
		rest_len = node->found.inf_len;

		/* walk trough all our FIDs in the directory stream */
		bpos = node->directory;
		fpos = 0;
		while (rest_len > 0) {
			fid = (struct fileid_desc *) bpos;
			fid_len = udf_fidsize(fid);

			/* get printable name */
			memset(&dirent, 0, sizeof(dirent));
			udf_osta_charset(&osta_charspec);
			udf_to_unix_name(dirent.d_name, NAME_MAX,
				(char *) fid->data + udf_rw16(fid->l_iu), fid->l_fi,
				&osta_charspec);
			dirent.d_namlen = strlen(dirent.d_name);

			/* '..' has no name, so provide one */
			if (fid->file_char & UDF_FILE_CHAR_PAR) {
				strcpy(dirent.d_name, "..");
				node->fsck_flags &= ~FSCK_NODE_FLAG_PAR_NOT_FOUND;
			}

			udf_node_pass1_add_entry(node, fid, &dirent);

			fpos += fid_len;
			bpos += fid_len;
			rest_len -= fid_len;
		}
	}

	error = udf_process_file(dscr, udf_rw16(node->loc.loc.part_num), NULL,
			AD_CHECK_USED, node);
	if (error) {
		pwarn("%s : internal error: checking for being allocated shouldn't fail\n",
			udf_node_path(node));
		return EINVAL;
	}
	/* file/directory is OK and referenced as its size won't change */
	error = udf_process_file(dscr, udf_rw16(node->loc.loc.part_num), NULL,
			AD_MARK_AS_USED, NULL);
	if (error) {
		pwarn("%s : internal error: marking allocated shouldn't fail\n",
			udf_node_path(node));
		return EINVAL;
	}
	return 0;
}


static void
udf_node_pass3_repairdir(struct udf_fsck_node *node, union dscrptr *dscr)
{
	struct fileid_desc *fid, *last_empty_fid;
	struct udf_fsck_node *file_node;
	struct udf_fsck_fid_context fid_context;
	struct dirent dirent;
	struct charspec osta_charspec;
	int64_t fpos, rest_len;
	uint32_t fid_len;
	uint8_t *bpos;
	int parent_missing;
	int error;

	pwarn("%s : fixing up directory\n", udf_node_path(node));
	assert(node->fsck_flags & FSCK_NODE_FLAG_DIRECTORY);

	rest_len = node->found.inf_len;

	udf_osta_charset(&osta_charspec);
	bpos = node->directory;
	fpos = 0;
	parent_missing = (node->fsck_flags & FSCK_NODE_FLAG_PAR_NOT_FOUND)? 1:0;

	last_empty_fid = NULL;
	while (rest_len > 0) {
		fid = (struct fileid_desc *) bpos;
		fid_len = udf_fidsize(fid);

		/* get printable name */
		memset(&dirent, 0, sizeof(dirent));
		udf_to_unix_name(dirent.d_name, NAME_MAX,
			(char *) fid->data + udf_rw16(fid->l_iu), fid->l_fi,
			&osta_charspec);
		dirent.d_namlen = strlen(dirent.d_name);

		/* '..' has no name, so provide one */
		if (fid->file_char & UDF_FILE_CHAR_PAR) {
			strcpy(dirent.d_name, "..");
		}

		/* only look up when not deleted */
		file_node = NULL;
		if ((fid->file_char & UDF_FILE_CHAR_DEL) == 0)
			file_node = udf_node_lookup(&fid->icb);

		/* if found */
		if (file_node) {
			/* delete files which couldn't be found */
			if (file_node && (file_node->fsck_flags & FSCK_NODE_FLAG_NOTFOUND)) {
				fid->file_char |= UDF_FILE_CHAR_DEL;
				memset(&fid->icb, 0, sizeof(struct long_ad));
			}

			/* fix up FID UniqueID errors */
			if (fid->icb.longad_uniqueid != file_node->unique_id)
				fid->icb.longad_uniqueid = udf_rw64(file_node->unique_id);
		} else {
			/* just mark it deleted if not found */
			fid->file_char |= UDF_FILE_CHAR_DEL;
		}

		if (fid->file_char & UDF_FILE_CHAR_DEL) {
			memset(&fid->icb, 0 , sizeof(struct long_ad));
			if (context.dscrver == 2) {
				uint8_t *cpos;
				/* compression IDs are preserved */
				cpos = (fid->data + udf_rw16(fid->l_iu));
				if (*cpos == 254)
					*cpos = 8;
				if (*cpos == 255)
					*cpos = 16;
			}
		}

		fpos += fid_len;
		bpos += fid_len;
		rest_len -= fid_len;
		assert(rest_len >= 0);
	}
	if (parent_missing) {
		/* this should be valid or we're in LALA land */
		assert(last_empty_fid);
		pwarn("%s : implementation limit, can't fix up missing parent node yet!\n",
			udf_node_path(node));
	}

	node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;

	fid_context.fid_offset = 0;
	fid_context.data_left = node->found.inf_len;
	error = udf_process_file(dscr, context.fids_part,
			&node->directory,
			AD_ADJUST_FIDS | AD_SAVE_FILE,
			&fid_context);
	if (error)
		pwarn("Failed to write out directory!\n");
}


static void
udf_node_pass3_writeout_update(struct udf_fsck_node *node, union dscrptr *dscr)
{
	struct file_entry    *fe  = NULL;
	struct extfile_entry *efe = NULL;
	int error;

	vat_writeout = 1;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe = (struct file_entry *) dscr;
		fe->inf_len      = udf_rw64(node->declared.inf_len);
		fe->logblks_rec  = udf_rw64(node->declared.logblks_rec);
		fe->link_cnt     = udf_rw16(node->link_count);
		fe->unique_id    = udf_rw64(node->unique_id);
	}
	if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe = (struct extfile_entry *) dscr;
		efe->inf_len     = udf_rw64(node->declared.inf_len);
		efe->obj_size    = udf_rw64(node->declared.obj_size);
		efe->logblks_rec = udf_rw64(node->declared.logblks_rec);
		efe->link_cnt    = udf_rw16(node->link_count);
		efe->unique_id   = udf_rw64(node->unique_id);
		/* streamdir directly cleared in dscr */
	}

	/* fixup CRC length (if needed) */
	dscr->tag.desc_crc_len = udf_tagsize(dscr, 1) - sizeof(struct desc_tag);

	pwarn("%s : updating node\n", udf_node_path(node));
	error = udf_write_dscr_virt(dscr, udf_rw32(node->loc.loc.lb_num),
			udf_rw16(node->loc.loc.part_num), 1);
	udf_shadow_VAT_in_use(&node->loc);
	if (error)
		pwarn("%s failed\n", __func__);
}


static void
udf_create_new_space_bitmaps_and_reset_freespace(void)
{
	struct space_bitmap_desc *sbd, *new_sbd;
	struct part_desc *part;
	struct part_hdr_desc *phd;
	uint32_t bitmap_len, bitmap_lb, bitmap_numlb;
	uint32_t cnt;
	int i, p, dscr_size;
	int error;

	/* copy recorded freespace info and clear counters */
	for (i = 0; i < UDF_PARTITIONS; i++) {
		recorded_part_free[i] = context.part_free[i];
		context.part_free[i]  = context.part_size[i];
	}

	/* clone existing bitmaps */
	for (i = 0; i < UDF_PARTITIONS; i++) {
		sbd = context.part_unalloc_bits[i];
		recorded_part_unalloc_bits[i] = sbd;
		if (sbd == NULL)
			continue;
		dscr_size = udf_tagsize((union dscrptr *) sbd,
				context.sector_size);
		new_sbd = calloc(1, dscr_size);
		memcpy(new_sbd, sbd, sizeof(struct space_bitmap_desc)-1);

		/* fill space with 0xff to indicate free */
		for (cnt = 0; cnt < udf_rw32(sbd->num_bytes); cnt++)
			new_sbd->data[cnt] = 0xff;

		context.part_unalloc_bits[i] = new_sbd;
	}

	/* allocate the space bitmaps themselves (normally one) */
	for (i = 0; i < UDF_PARTITIONS; i++) {
		part = context.partitions[i];
		if (!part)
			continue;

		phd = &part->pd_part_hdr;
		bitmap_len = udf_rw32(phd->unalloc_space_bitmap.len);
		bitmap_lb  = udf_rw32(phd->unalloc_space_bitmap.lb_num);
		if (bitmap_len == 0)
			continue;

		bitmap_numlb = udf_bytes_to_sectors(bitmap_len);
		sbd = context.part_unalloc_bits[i];
		assert(sbd);

		udf_mark_allocated(bitmap_lb, context.vtop[i], bitmap_numlb);
	}

	/* special case for metadata partition */
	if (context.format_flags & FORMAT_META) {
		i = context.metadata_part;
		p = context.vtop[i];
		assert(context.vtop_tp[i] == UDF_VTOP_TYPE_META);
		error = udf_process_file((union dscrptr *) context.meta_file,
			p, NULL, AD_MARK_AS_USED, NULL);
		error = udf_process_file((union dscrptr *) context.meta_mirror,
			p, NULL, AD_MARK_AS_USED, NULL);
		if (context.meta_bitmap) {
			error = udf_process_file(
				(union dscrptr *) context.meta_bitmap,
				p, NULL, AD_MARK_AS_USED, NULL);
			assert(error == 0);
		}
	}

	/* mark fsd allocation ! */
	udf_mark_allocated(udf_rw32(context.fileset_desc->tag.tag_loc),
		context.metadata_part, 1);
}


static void
udf_shadow_VAT_in_use(struct long_ad *loc)
{
	uint32_t i;
	uint8_t *vat_pos, *shadow_vat_pos;

	if (context.vtop_tp[context.metadata_part] != UDF_VTOP_TYPE_VIRT)
		return;

	i = udf_rw32(loc->loc.lb_num);
	vat_pos = context.vat_contents + context.vat_start + i*4;
	shadow_vat_pos = shadow_vat_contents + context.vat_start + i*4;
	/* keeping endian */
	*(uint32_t *) shadow_vat_pos = *(uint32_t *) vat_pos;
}


static void
udf_create_shadow_VAT(void)
{
	struct long_ad fsd_loc;
	uint32_t  vat_entries, i;
	uint8_t *vat_pos;

	if (context.vtop_tp[context.metadata_part] != UDF_VTOP_TYPE_VIRT)
		return;

	shadow_vat_contents = calloc(1, context.vat_allocated);
	assert(shadow_vat_contents);
	memcpy(shadow_vat_contents, context.vat_contents, context.vat_size);

	vat_entries = (context.vat_size - context.vat_start)/4;
	for (i = 0; i < vat_entries; i++) {
		vat_pos = shadow_vat_contents + context.vat_start + i*4;
		*(uint32_t *) vat_pos = udf_rw32(0xffffffff);
	}

	/*
	 * Record our FSD in this shadow VAT since its the only one outside
	 * the nodes.
	 */
	memset(&fsd_loc, 0, sizeof(struct long_ad));
	fsd_loc.loc.lb_num = context.fileset_desc->tag.tag_loc;
	udf_shadow_VAT_in_use(&fsd_loc);
}


static void
udf_check_shadow_VAT(void)
{
	uint32_t vat_entries, i;
	uint8_t *vat_pos, *shadow_vat_pos;
	int difference = 0;

	if (context.vtop_tp[context.metadata_part] != UDF_VTOP_TYPE_VIRT)
		return;

	vat_entries = (context.vat_size - context.vat_start)/4;
	for (i = 0; i < vat_entries; i++) {
		vat_pos = context.vat_contents + context.vat_start + i*4;
		shadow_vat_pos = shadow_vat_contents + context.vat_start + i*4;
		if (*(uint32_t *) vat_pos != *(uint32_t *) shadow_vat_pos) {
			difference++;
		}
	}
	memcpy(context.vat_contents, shadow_vat_contents, context.vat_size);
	if (difference) {
		if (!preen)
			printf("\t\t");
		pwarn("%d unused VAT entries cleaned\n", difference);
		vat_writeout = 1;
	}
}


static int
udf_check_directory_tree(void)
{
	union dscrptr *dscr;
	struct udf_fsck_node *root_node, *sys_stream_node;
	struct udf_fsck_node *cur_node, *next_node;
	struct long_ad root_icb, sys_stream_icb;
	bool dont_repair;
	int entry, error;

	assert(TAILQ_EMPTY(&fs_nodes));

	/* (re)init queues and hash lists */
	TAILQ_INIT(&fs_nodes);
	TAILQ_INIT(&fsck_overlaps);
	for (int i = 0; i < HASH_HASHSIZE; i++)
		LIST_INIT(&fs_nodes_hash[i]);

	/* create a new empty copy of the space bitmaps */
	udf_create_new_space_bitmaps_and_reset_freespace();
	udf_create_shadow_VAT();

	/* start from the root */
	root_icb       = context.fileset_desc->rootdir_icb;
	sys_stream_icb = context.fileset_desc->streamdir_icb;

	root_node = udf_new_fsck_node(NULL, &root_icb, strdup(""));
	assert(root_node);
	TAILQ_INSERT_TAIL(&fs_nodes, root_node, next);
	entry = udf_calc_node_hash(&root_node->loc);
	LIST_INSERT_HEAD(&fs_nodes_hash[entry], root_node, next_hash);

	sys_stream_node = NULL;
	if (sys_stream_icb.len) {
		sys_stream_node = udf_new_fsck_node(NULL, &sys_stream_icb, strdup("#"));
		assert(sys_stream_node);
		sys_stream_node->fsck_flags |= FSCK_NODE_FLAG_STREAM_DIR;

		TAILQ_INSERT_TAIL(&fs_nodes, sys_stream_node, next);
		entry = udf_calc_node_hash(&sys_stream_node->loc);
		LIST_INSERT_HEAD(&fs_nodes_hash[entry], sys_stream_node, next_hash);
	}

	/* pass 1 */
	if (!preen)
		printf("\tPass 1, reading in directory trees\n");

	context.unique_id = MAX(0x10, context.unique_id);
	TAILQ_FOREACH(cur_node, &fs_nodes, next) {
		/* read in node */
		error = udf_read_node_dscr(cur_node, &dscr);
		if (!error)
			error = udf_extract_node_info(cur_node, dscr, 0);
		if (error) {
			pwarn("%s : invalid reference or bad descriptor, DELETING\n",
				udf_node_path(cur_node));
			udf_recursive_keep(cur_node);
			cur_node->fsck_flags |= FSCK_NODE_FLAG_NOTFOUND;
			if (cur_node->parent) {
				if (cur_node->fsck_flags & FSCK_NODE_FLAG_STREAM_DIR)
					cur_node->parent->fsck_flags |=
						FSCK_NODE_FLAG_WIPE_STREAM_DIR;
				else
					cur_node->parent->fsck_flags |=
						FSCK_NODE_FLAG_REPAIRDIR;
				;
			}
			free(dscr);
			continue;
		}

		if (print_info) {
			pwarn("Processing %s\n", udf_node_path(cur_node));
			print_info = 0;
		}

		/* directory found in stream directory? */
		if (cur_node->parent &&
			(cur_node->parent->fsck_flags & FSCK_NODE_FLAG_STREAM_DIR) &&
			(cur_node->fsck_flags & FSCK_NODE_FLAG_DIRECTORY))
		{
			pwarn("%s : specification violation, directory in stream directory\n",
				udf_node_path(cur_node));
			if (ask(0, "Clear directory")) {
				udf_recursive_keep(cur_node);
				cur_node->fsck_flags |= FSCK_NODE_FLAG_NOTFOUND;
				cur_node->parent->fsck_flags |=
					FSCK_NODE_FLAG_REPAIRDIR;
				continue;
			}
		}
		error = udf_process_node_pass1(cur_node, dscr);
		free(dscr);

		if (error)
			return error;
	}

	/* pass 1b, if there is overlap, find matching pairs */
	dont_repair = false;
	if (!TAILQ_EMPTY(&fsck_overlaps)) {
		struct udf_fsck_overlap *overlap;

		dont_repair = true;
		pwarn("*** Overlaps detected! rescanning tree for matching pairs ***\n");
		TAILQ_FOREACH(cur_node, &fs_nodes, next) {
			if (cur_node->fsck_flags & FSCK_NODE_FLAG_NOTFOUND)
				continue;

			error = udf_read_node_dscr(cur_node, &dscr);
			/* should not fail differently */

			if (print_info) {
				pwarn("Processing %s\n", udf_node_path(cur_node));
				print_info = 0;
			}

			error = udf_process_file(
					dscr,
					udf_rw16(cur_node->loc.loc.part_num),
					NULL,
					AD_FIND_OVERLAP_PAIR,
					(void *) cur_node);
			/* shouldn't fail */

			free(dscr);
		}
		TAILQ_FOREACH(overlap, &fsck_overlaps, next) {
			pwarn("%s :overlaps with %s\n",
				udf_node_path(overlap->node),
				udf_node_path(overlap->node2));
		}
		if (!preen)
			printf("\n");
		pwarn("*** The following files/directories need to be copied/evacuated:\n");
		TAILQ_FOREACH(cur_node, &fs_nodes, next) {
			if (cur_node->fsck_flags & FSCK_NODE_FLAG_OVERLAP) {
				pwarn("%s : found OVERLAP, evacuate\n",
					udf_node_path(cur_node));
			}
		}
	}
	if (dont_repair) {
		if (!preen)
			printf("\n");
		pwarn("*** Skipping further repair, only updating free space map if needed\n");
		pwarn("*** After deep copying and/or evacuation of these files/directories,\n");
		pwarn("*** remove files/directories and re-run fsck_udf\n");
		error = udf_prepare_writing();
		if (error)
			return error;

		udf_update_lvintd(UDF_INTEGRITY_OPEN);
		return 0;
	}

	/* pass 2a, checking link counts, object sizes and count files/dirs */
	if (!preen)
		printf("\n\tPass 2, checking link counts, object sizes, stats and cleaning up\n");

	TAILQ_FOREACH_SAFE(cur_node, &fs_nodes, next, next_node) {
		/* not sane to process files/directories that are not found */
		if (cur_node->fsck_flags & FSCK_NODE_FLAG_NOTFOUND)
			continue;

		/* shadow VAT */
		udf_shadow_VAT_in_use(&cur_node->loc);

		/* link counts */
		if (cur_node->found_link_count != cur_node->link_count) {
			pwarn("%s : link count incorrect; "
			      "%u instead of declared %u : FIXED\n",
				udf_node_path(cur_node),
				cur_node->found_link_count, cur_node->link_count);
			cur_node->link_count = cur_node->found_link_count;
			udf_recursive_keep(cur_node);
			cur_node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
		}

		/* object sizes */
		if (cur_node->declared.obj_size != cur_node->found.obj_size) {
			pwarn("%s : recorded object size incorrect; "
			      "%lu instead of declared %lu\n",
				udf_node_path(cur_node),
				cur_node->found.obj_size, cur_node->declared.obj_size);
			cur_node->declared.obj_size = cur_node->found.obj_size;
			udf_recursive_keep(cur_node);
			cur_node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
		}

		/* XXX TODO XXX times */
		/* XXX TODO XXX extended attributes location for UDF < 1.50 */

		/* validity of UniqueID check */
		if (cur_node->parent) {
			if (cur_node->fsck_flags & FSCK_NODE_FLAG_NEW_UNIQUE_ID) {
				pwarn("%s : assigning new UniqueID\n",
					udf_node_path(cur_node));
				cur_node->unique_id = udf_rw64(context.unique_id);
				udf_advance_uniqueid();
				udf_recursive_keep(cur_node);
				cur_node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
				if (cur_node->fsck_flags & FSCK_NODE_FLAG_DIRECTORY)
					cur_node->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
				cur_node->parent->fsck_flags |= FSCK_NODE_FLAG_REPAIRDIR;
			}
			if (cur_node->fsck_flags & FSCK_NODE_FLAG_COPY_PARENT_ID) {
				/* work already done but make note to operator */
				pwarn("%s : fixing stream UniqueID to match parent\n",
					udf_node_path(cur_node));
			}
		} else {
			if (cur_node->unique_id != 0) {
				pwarn("%s : bad UniqueID, zeroing\n",
						udf_node_path(cur_node));
				cur_node->unique_id = 0;
				cur_node->fsck_flags |=
					FSCK_NODE_FLAG_DIRTY | FSCK_NODE_FLAG_REPAIRDIR;
			}
		}

		/* keep nodes in a repairing dir */
		if (cur_node->parent)
			if (cur_node->parent->fsck_flags & FSCK_NODE_FLAG_REPAIRDIR)
				cur_node->fsck_flags |= FSCK_NODE_FLAG_KEEP;

		/* stream directories and files in it are not included */
		if (!(cur_node->fsck_flags & FSCK_NODE_FLAG_STREAM)) {
			/* files / directories counting */
			int link_count = cur_node->found_link_count;

			/* stream directories don't count as link ECMA 4/14.9.6 */
			if (cur_node->fsck_flags & FSCK_NODE_FLAG_HAS_STREAM_DIR)
				link_count--;

			if (cur_node->fsck_flags & FSCK_NODE_FLAG_DIRECTORY)
				context.num_directories++;
			else 
				context.num_files += link_count;
			;
		}
	}

	/* pass 2b, cleaning */
	open_integrity = 0;
	TAILQ_FOREACH_SAFE(cur_node, &fs_nodes, next, next_node) {
		/* can we remove the node? (to save memory) */
		if (FSCK_NODE_FLAG_OK(cur_node->fsck_flags)) {
			TAILQ_REMOVE(&fs_nodes, cur_node, next);
			LIST_REMOVE(cur_node, next_hash);
			free(cur_node->directory);
			bzero(cur_node, sizeof(struct udf_fsck_node));
			free(cur_node);
		} else {
			/* else keep erroring node */
			open_integrity = 1;
		}
	}

	if (!preen)
		printf("\n\tPreparing disc for writing\n");
	error = udf_prepare_writing();
	if (error)
		return error;

	if (open_integrity)
		udf_update_lvintd(UDF_INTEGRITY_OPEN);

	/* pass 3 */
	if (!preen)
		printf("\n\tPass 3, fix errors\n");

	TAILQ_FOREACH_SAFE(cur_node, &fs_nodes, next, next_node) {
		/* not sane to process files/directories that are not found */
		if (cur_node->fsck_flags & FSCK_NODE_FLAG_NOTFOUND)
			continue;

		/* only interested in bad nodes */
		if (FSCK_NODE_FLAG_OK(cur_node->fsck_flags))
			continue;

		error = udf_read_node_dscr(cur_node, &dscr);
		/* should not fail differently */

		/* repair directories */
		if (cur_node->fsck_flags & FSCK_NODE_FLAG_REPAIRDIR)
			udf_node_pass3_repairdir(cur_node, dscr);

		/* remove invalid stream directories */
		if (cur_node->fsck_flags & FSCK_NODE_FLAG_WIPE_STREAM_DIR) {
			assert(udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY);
			bzero(&dscr->efe.streamdir_icb, sizeof(struct long_ad));
			cur_node->fsck_flags |= FSCK_NODE_FLAG_DIRTY;
		}

		if (cur_node->fsck_flags & FSCK_NODE_FLAG_DIRTY)
			udf_node_pass3_writeout_update(cur_node, dscr);
		free(dscr);
	}
	udf_check_shadow_VAT();

	return 0;
}


static void
udf_cleanup_after_check(void)
{
	struct udf_fsck_node *cur_node, *next_node;

	/* XXX yes, there are some small memory leaks here */

	/* clean old node info from previous checks */
	TAILQ_FOREACH_SAFE(cur_node, &fs_nodes, next, next_node) {
		TAILQ_REMOVE(&fs_nodes, cur_node, next);
		LIST_REMOVE(cur_node, next_hash);
		free(cur_node->directory);
		free(cur_node);
	}

	/* free partition related info */
	for (int i = 0; i < UDF_PARTITIONS; i++) {
		free(context.partitions[i]);
		free(context.part_unalloc_bits[i]);
		free(context.part_freed_bits[i]);
	}

	/* only free potentional big blobs */
	free(context.vat_contents);
	free(context.lvint_history);

	free(shadow_vat_contents);
	shadow_vat_contents = NULL;
}


static int
checkfilesys(char *given_dev)
{
	struct mmc_trackinfo ti;
	int open_flags;
	int error;

	udf_init_create_context();
	context.app_name         = "*NetBSD UDF";
	context.app_version_main = APP_VERSION_MAIN;
	context.app_version_sub  = APP_VERSION_SUB;
	context.impl_name        = IMPL_NAME;

	emul_mmc_profile  =  -1;	/* invalid->no emulation	*/
	emul_packetsize   =   1;	/* reasonable default		*/
	emul_sectorsize   = 512;	/* minimum allowed sector size	*/
	emul_size	  =   0;	/* empty			*/

	if (!preen)
		pwarn("** Checking UDF file system on %s\n", given_dev);

	/* reset sticky flags */
	rdonly = rdonly_flag;
	undo_opening_session = 0;	/* trying to undo opening of last crippled session */
	vat_writeout = 0;		/* to write out the VAT anyway */

	/* open disc device or emulated file */
	open_flags = rdonly ? O_RDONLY : O_RDWR;
	if (udf_opendisc(given_dev, open_flags)) {
		udf_closedisc();
		warnx("can't open %s", given_dev);
		return FSCK_EXIT_CHECK_FAILED;
	}

	if (!preen)
		pwarn("** Phase 1 - discovering format from disc\n\n");

	/* check if it is an empty disc or no disc in present */
	ti.tracknr = mmc_discinfo.first_track;
	error = udf_update_trackinfo(&ti);
	if (error || (ti.flags & MMC_TRACKINFO_BLANK)) {
		/* no use erroring out */
		pwarn("Empty disc\n");
		return FSCK_EXIT_OK;
	}

	context.format_flags = 0;
	if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL)
		context.format_flags |= FORMAT_SEQUENTIAL;

	if ((context.format_flags & FORMAT_SEQUENTIAL) &&
		    ((mmc_discinfo.disc_state == MMC_STATE_CLOSED) ||
		     (mmc_discinfo.disc_state == MMC_STATE_FULL))) {
		pwarn("Disc is closed or full, can't modify disc\n");
		rdonly = 1;
	}

	if (target_session) {
		context.create_new_session = 1;
		if (target_session < 0)
			target_session += mmc_discinfo.num_sessions;
	} else {
		target_session = mmc_discinfo.num_sessions;
		if (mmc_discinfo.last_session_state == MMC_STATE_EMPTY)
			target_session--;
	}

	error = udf_get_anchors();
	if (error) {
		udf_closedisc();
		pwarn("Failed to retrieve anchors; can't check file system\n");
		return FSCK_EXIT_CHECK_FAILED;
	}

	udf_check_vrs9660();

	/* get both VRS areas */
	error = udf_check_VDS_areas();
	if (error) {
		udf_closedisc();
		pwarn("Failure reading volume descriptors, disc might be toast\n");
		return FSCK_EXIT_CHECK_FAILED;
	}

	if (udf_rw32(context.logvol_integrity->integrity_type) ==
		UDF_INTEGRITY_CLOSED) {
		if (!force) {
			pwarn("** File system is clean; not checking\n");
			return FSCK_EXIT_OK;
		}
		pwarn("** File system is already clean\n");
		if (!preen)
			pwarn("\n");
	} else {
		pwarn("** File system not closed properly\n");
		if (!preen)
			printf("\n");
	}

	/*
	 * Only now read in free/unallocated space bitmap. If it reads in fine
	 * it doesn't mean its contents is valid though. Sets partition
	 * lengths too.
	 */
	error = udf_readin_partitions_free_space();
	if (error) {
		pwarn("Error during free space bitmap reading\n");
		udf_update_lvintd(UDF_INTEGRITY_OPEN);
	}

	if (!preen)
		pwarn("** Phase 2 - walking directory tree\n");

	udf_suspend_writing();
	error = udf_check_directory_tree();
	if (error) {
		if ((!rdonly) && ask(0, "Write out modifications made until now"))
			udf_allow_writing();
		else
			pwarn("** Aborting repair, not modifying disc\n");
		udf_closedisc();
		return FSCK_EXIT_CHECK_FAILED;
	}

	if (!preen)
		pwarn("\n** Phase 3 - closing volume if needed\n\n");

/* XXX FAULT INJECTION POINT XXX */
//udf_update_lvintd(UDF_INTEGRITY_OPEN);

	if (error && rdonly) {
		pwarn("** Aborting repair, nothing written, disc marked read-only\n");
	} else {
		error = udf_close_volume();
	}

	udf_closedisc();

	if (error)
		return FSCK_EXIT_CHECK_FAILED;
	return FSCK_EXIT_OK;
}


static void
usage(void)
{
    	(void)fprintf(stderr, "Usage: %s [-psSynfH] filesystem ... \n",
	    getprogname());
	exit(FSCK_EXIT_USAGE);
}


static void
got_siginfo(int signo)
{
	print_info = 1;
}


int
main(int argc, char **argv)
{
	int ret = FSCK_EXIT_OK, erg;
	int ch;

	while ((ch = getopt(argc, argv, "ps:SynfH")) != -1) {
		switch (ch) {
		case 'H':
			heuristics = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'n':
			rdonly_flag = alwaysno = 1;
			alwaysyes = preen = 0;
			break;
		case 'y':
			alwaysyes = 1;
			alwaysno = preen = 0;
			break;
		case 'p':
			/* small automatic repairs */
			preen = 1;
			alwaysyes = alwaysno = 0;
			break;
		case 's':
			/* session number or relative session */
			target_session = atoi(optarg);
			break;
		case 'S':		/* Search for older VATs */
			search_older_vat = 1;
			break;

		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (!argc)
		usage();

	/* TODO SIGINT and SIGQUIT catchers */
#if 0
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, catch);
	if (preen)
		(void) signal(SIGQUIT, catch);
#endif

	signal(SIGINFO, got_siginfo);

	while (--argc >= 0) {
		setcdevname(*argv, preen);
		erg = checkfilesys(*argv++);
		if (erg > ret)
			ret = erg;
		if (!preen)
			printf("\n");
		udf_cleanup_after_check();
	}

	return ret;
}


/*VARARGS*/
static int
ask(int def, const char *fmt, ...)
{
	va_list ap;

	char prompt[256];
	int c;

	va_start(ap, fmt);
	vsnprintf(prompt, sizeof(prompt), fmt, ap);
	va_end(ap);
	if (alwaysyes || rdonly) {
		pwarn("%s? %s\n", prompt, rdonly ? "no" : "yes");
		return !rdonly;
	}
	if (preen) {
		pwarn("%s? %s : (default)\n", prompt, def ? "yes" : "no");
		return def;
	}

	do {
		pwarn("%s? [yn] ", prompt);
		fflush(stdout);
		c = getchar();
		while (c != '\n' && getchar() != '\n')
			if (feof(stdin))
				return 0;
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	return c == 'y' || c == 'Y';
}


/*VARARGS*/
static int
ask_noauto(int def, const char *fmt, ...)
{
	va_list ap;

	char prompt[256];
	int c;

	va_start(ap, fmt);
	vsnprintf(prompt, sizeof(prompt), fmt, ap);
	va_end(ap);
#if 0
	if (preen) {
		pwarn("%s? %s : (default)\n", prompt, def ? "yes" : "no");
		return def;
	}
#endif

	do {
		pwarn("%s? [yn] ", prompt);
		fflush(stdout);
		c = getchar();
		while (c != '\n' && getchar() != '\n')
			if (feof(stdin))
				return 0;
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	return c == 'y' || c == 'Y';
}
