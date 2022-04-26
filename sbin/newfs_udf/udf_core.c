/* $NetBSD: udf_core.c,v 1.9 2022/04/26 15:11:42 reinoud Exp $ */

/*
 * Copyright (c) 2006, 2008, 2021, 2022 Reinoud Zandijk
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
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: udf_core.c,v 1.9 2022/04/26 15:11:42 reinoud Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <util.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#include "newfs_udf.h"
#include "unicode.h"
#include "udf_core.h"


/* disk partition support */
#if !HAVE_NBTOOL_CONFIG_H
#include "../fsck/partutil.h"
#include "../fsck/partutil.c"
#endif


/* queue for temporary storage of sectors to be written out */
struct wrpacket {
	uint64_t  start_sectornr;
	uint8_t	 *packet_data;
	uint64_t  present;
	TAILQ_ENTRY(wrpacket) next;
};


/* global variables describing disc and format requests */
struct udf_create_context context;
struct udf_disclayout     layout;


int		 dev_fd_rdonly;		/* device: open readonly!	*/
int		 dev_fd;		/* device: file descriptor	*/
struct stat	 dev_fd_stat;	  	/* device: last stat info	*/
char		*dev_name;		/* device: name			*/
int	 	 emul_mmc_profile;	/* for files			*/
int	 	 emul_packetsize;	/* for discs and files		*/
int		 emul_sectorsize;	/* for files			*/
off_t		 emul_size;		/* for files			*/

struct mmc_discinfo mmc_discinfo;	/* device: disc info		*/
union dscrptr *terminator_dscr;		/* generic terminator descriptor*/


/* write queue and track blocking skew */
TAILQ_HEAD(wrpacket_list, wrpacket) write_queue;
int	  write_queuelen;
int	  write_queue_suspend;
uint32_t  wrtrack_skew;			/* offset for writing sector0	*/

static void udf_init_writequeue(int write_strategy);
static int  udf_writeout_writequeue(bool complete);

/*
 * NOTE that there is some overlap between this code and the udf kernel fs.
 * This is intentionally though it might better be factored out one day.
 */

void
udf_init_create_context(void)
{
	/* clear */
	memset(&context, 0, sizeof(struct udf_create_context));

	/* fill with defaults currently known */
	context.dscrver   = 3;
	context.min_udf   = 0x0102;
	context.max_udf   = 0x0250;
	context.serialnum = 1;		/* default */

	context.gmtoff        = 0;
	context.meta_perc     = UDF_META_PERC;
	context.check_surface = 0;
	context.create_new_session  = 0;

	context.sector_size      = 512;	/* minimum for UDF */
	context.media_accesstype = UDF_ACCESSTYPE_NOT_SPECIFIED;
	context.format_flags     = FORMAT_INVALID;
	context.write_strategy   = UDF_WRITE_PACKET;

	context.logvol_name  = NULL;
	context.primary_name = NULL;
	context.volset_name  = NULL;
	context.fileset_name = NULL;

	/* most basic identification */
	context.app_name	 = "*NetBSD";
	context.app_version_main = 0;
	context.app_version_sub  = 0;
	context.impl_name        = "*NetBSD";

	context.vds_seq = 0;	/* first one starts with zero */

	/* Minimum value of 16 : UDF 3.2.1.1, 3.3.3.4. */
	context.unique_id       = 0x10;

	context.num_files       = 0;
	context.num_directories = 0;

	context.data_part          = 0;
	context.metadata_part      = 0;
}


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


/*
 * Parse a given string for an udf version.
 * May exit.
 */
int
a_udf_version(const char *s, const char *id_type)
{
	uint32_t version;

	if (parse_udfversion(s, &version))
		errx(1, "unknown %s version %s; specify as hex or float", id_type, s);
	switch (version) {
		case 0x102:
		case 0x150:
		case 0x200:
		case 0x201:
		case 0x250:
			break;
		case 0x260:
			/* we don't support this one */
			errx(1, "UDF version 0x260 is not supported");
			break;
		default:
			errx(1, "unknown %s version %s, choose from "
				"0x102, 0x150, 0x200, 0x201, 0x250",
				id_type, s);
	}
	return version;
}


static uint32_t
udf_space_bitmap_len(uint32_t part_size)
{
	return  sizeof(struct space_bitmap_desc)-1 +
		part_size/8;
}


uint32_t
udf_bytes_to_sectors(uint64_t bytes)
{
	uint32_t sector_size = context.sector_size;
	return (bytes + sector_size -1) / sector_size;
}


void
udf_dump_layout(void) {
#ifdef DEBUG
	int format_flags = context.format_flags;
	int sector_size  = context.sector_size;

	printf("Summary so far\n");
	printf("\tiso9660_vrs\t\t%d\n", layout.iso9660_vrs);
	printf("\tanchor0\t\t\t%d\n", layout.anchors[0]);
	printf("\tanchor1\t\t\t%d\n", layout.anchors[1]);
	printf("\tanchor2\t\t\t%d\n", layout.anchors[2]);
	printf("\tvds1_size\t\t%d\n", layout.vds1_size);
	printf("\tvds2_size\t\t%d\n", layout.vds2_size);
	printf("\tvds1\t\t\t%d\n", layout.vds1);
	printf("\tvds2\t\t\t%d\n", layout.vds2);
	printf("\tlvis_size\t\t%d\n", layout.lvis_size);
	printf("\tlvis\t\t\t%d\n", layout.lvis);
	if (format_flags & FORMAT_SPAREABLE) {
		printf("\tspareable size\t\t%d\n", layout.spareable_area_size);
		printf("\tspareable\t\t%d\n", layout.spareable_area);
	}
	printf("\tpartition start lba\t%d\n", layout.part_start_lba);
	printf("\tpartition size\t\t%ld KiB, %ld MiB\n",
		((uint64_t) layout.part_size_lba * sector_size) / 1024,
		((uint64_t) layout.part_size_lba * sector_size) / (1024*1024));
	if ((format_flags & FORMAT_SEQUENTIAL) == 0) {
		printf("\tpart bitmap start\t%d\n",   layout.unalloc_space);
		printf("\t\tfor %d lba\n", layout.alloc_bitmap_dscr_size);
	}
	if (format_flags & FORMAT_META) {
		printf("\tmeta blockingnr\t\t%d\n", layout.meta_blockingnr);
		printf("\tmeta alignment\t\t%d\n",  layout.meta_alignment);
		printf("\tmeta size\t\t%ld KiB, %ld MiB\n",
			((uint64_t) layout.meta_part_size_lba * sector_size) / 1024,
			((uint64_t) layout.meta_part_size_lba * sector_size) / (1024*1024));
		printf("\tmeta file\t\t%d\n", layout.meta_file);
		printf("\tmeta mirror\t\t%d\n", layout.meta_mirror);
		printf("\tmeta bitmap\t\t%d\n", layout.meta_bitmap);
		printf("\tmeta bitmap start\t%d\n", layout.meta_bitmap_space);
		printf("\t\tfor %d lba\n", layout.meta_bitmap_dscr_size);
		printf("\tmeta space start\t%d\n",  layout.meta_part_start_lba);
		printf("\t\tfor %d lba\n", layout.meta_part_size_lba);
	}
	printf("\n");
#endif
}


int
udf_calculate_disc_layout(int min_udf,
	uint32_t first_lba, uint32_t last_lba,
	uint32_t sector_size, uint32_t blockingnr)
{
	uint64_t kbsize, bytes;
	uint32_t spareable_blockingnr;
	uint32_t align_blockingnr;
	uint32_t pos, mpos;
	int	 format_flags = context.format_flags;

	/* clear */
	memset(&layout, 0, sizeof(layout));

	/* fill with parameters */
	layout.wrtrack_skew    = wrtrack_skew;
	layout.first_lba       = first_lba;
	layout.last_lba        = last_lba;
	layout.blockingnr      = blockingnr;
	layout.spareable_blocks = udf_spareable_blocks();

	/* start disc layouting */

	/*
	 * location of iso9660 vrs is defined as first sector AFTER 32kb,
	 * minimum `sector size' 2048
	 */
	layout.iso9660_vrs = ((32*1024 + sector_size - 1) / sector_size)
		+ first_lba;

	/* anchor starts at specified offset in sectors */
	layout.anchors[0] = first_lba + 256;
	if (format_flags & FORMAT_TRACK512)
		layout.anchors[0] = first_lba + 512;
	layout.anchors[1] = last_lba - 256;
	layout.anchors[2] = last_lba;

	/* update workable space */
	first_lba = layout.anchors[0] + blockingnr;
	last_lba  = layout.anchors[1] - 1;

	/* XXX rest of anchor packet can be added to unallocated space descr */

	/* reserve space for VRS and VRS copy and associated tables */
	layout.vds1_size = MAX(16, blockingnr);     /* UDF 2.2.3.1+2 */
	layout.vds1 = first_lba;
	first_lba += layout.vds1_size;              /* next packet */

	layout.vds2_size = layout.vds1_size;
	if (format_flags & FORMAT_SEQUENTIAL) {
		/* for sequential, append them ASAP */
		layout.vds2 = first_lba;
		first_lba += layout.vds2_size;
	} else {
		layout.vds2 = layout.anchors[1] +1 - layout.vds2_size;
		last_lba = layout.vds2 - 1;
	}

	/*
	 * Reserve space for logvol integrity sequence, at least 8192 bytes
	 * for overwritable and rewritable media UDF 2.2.4.6, ECMA 3/10.6.12.
	 */
	layout.lvis_size = MAX(8192.0/sector_size, 2 * blockingnr);
	if (layout.lvis_size * sector_size < 8192)
		layout.lvis_size++;
	if (format_flags & FORMAT_VAT)
		layout.lvis_size = 2;
	if (format_flags & FORMAT_WORM)
		layout.lvis_size = 64 * blockingnr;

	/* TODO skip bad blocks in LVID sequence */
	layout.lvis = first_lba;
	first_lba += layout.lvis_size;

	/* initial guess of UDF partition size */
	layout.part_start_lba = first_lba;
	layout.part_size_lba = last_lba - layout.part_start_lba;

	/* all non sequential media needs an unallocated space bitmap */
	layout.alloc_bitmap_dscr_size = 0;
	if ((format_flags & (FORMAT_SEQUENTIAL | FORMAT_READONLY)) == 0) {
		bytes = udf_space_bitmap_len(layout.part_size_lba);
		layout.alloc_bitmap_dscr_size = udf_bytes_to_sectors(bytes);

		/* XXX freed space map when applicable */
	}

	spareable_blockingnr = udf_spareable_blockingnr();
	align_blockingnr = blockingnr;

	if (format_flags & (FORMAT_SPAREABLE | FORMAT_META))
		align_blockingnr = spareable_blockingnr;

	layout.align_blockingnr    = align_blockingnr;
	layout.spareable_blockingnr = spareable_blockingnr;

	/*
	 * Align partition LBA space to blocking granularity. Not strictly
	 * necessary for non spareables but safer for the VRS data since it is
	 * updated sporadically
	 */

#ifdef DEBUG
	printf("Lost %lu slack sectors at start\n", UDF_ROUNDUP(
		first_lba, align_blockingnr) -
		first_lba);
	printf("Lost %lu slack sectors at end\n",
		last_lba - UDF_ROUNDDOWN(
		last_lba, align_blockingnr));
#endif

	first_lba = UDF_ROUNDUP(first_lba, align_blockingnr);
	last_lba  = UDF_ROUNDDOWN(last_lba, align_blockingnr);

	if ((format_flags & FORMAT_SPAREABLE) == 0)
		layout.spareable_blocks = 0;

	if (format_flags & FORMAT_SPAREABLE) {
		layout.spareable_area_size =
			layout.spareable_blocks * spareable_blockingnr;

		/* a sparing table descriptor is a whole blockingnr sectors */
		layout.sparing_table_dscr_lbas = spareable_blockingnr;

		/* place the descriptors at the start and end of the area */
		layout.spt_1 = first_lba;
		first_lba += layout.sparing_table_dscr_lbas;

		layout.spt_2 = last_lba - layout.sparing_table_dscr_lbas;
		last_lba -= layout.sparing_table_dscr_lbas;

		/* allocate spareable section */
		layout.spareable_area = first_lba;
		first_lba += layout.spareable_area_size;
	}

	/* update guess of UDF partition size */
	layout.part_start_lba = first_lba;
	layout.part_size_lba = last_lba - layout.part_start_lba;

	/* determine partition selection for data and metadata */
	context.data_part     = 0;
	context.metadata_part = context.data_part;
	if ((format_flags & FORMAT_VAT) || (format_flags & FORMAT_META))
		context.metadata_part = context.data_part + 1;
	context.fids_part = context.metadata_part;
	if (format_flags & FORMAT_VAT)
		context.fids_part = context.data_part;

	/*
	 * Pick fixed logical space sector numbers for main FSD, rootdir and
	 * unallocated space. The reason for this pre-allocation is that they
	 * are referenced in the volume descriptor sequence and hence can't be
	 * allocated later.
	 */
	pos = 0;
	layout.unalloc_space = pos;
	pos += layout.alloc_bitmap_dscr_size;

	/* claim metadata descriptors and partition space [UDF 2.2.10] */
	if (format_flags & FORMAT_META) {
		/* note: all in backing partition space */
		layout.meta_file   = pos++;
		layout.meta_bitmap = 0xffffffff;
		if (!(context.format_flags & FORMAT_READONLY))
			layout.meta_bitmap = pos++;
		layout.meta_mirror = layout.part_size_lba-1;
		layout.meta_alignment  = MAX(blockingnr, spareable_blockingnr);
		layout.meta_blockingnr = MAX(layout.meta_alignment, 32);

		/* calculate our partition length and store in sectors */
		layout.meta_part_size_lba = layout.part_size_lba *
			((float) context.meta_perc / 100.0);
		layout.meta_part_size_lba = MAX(layout.meta_part_size_lba, 32);
		layout.meta_part_size_lba =
			UDF_ROUNDDOWN(layout.meta_part_size_lba, layout.meta_blockingnr);

		if (!(context.format_flags & FORMAT_READONLY)) {
			/* metadata partition free space bitmap */
			bytes = udf_space_bitmap_len(layout.meta_part_size_lba);
			layout.meta_bitmap_dscr_size = udf_bytes_to_sectors(bytes);

			layout.meta_bitmap_space = pos;
			pos += layout.meta_bitmap_dscr_size;
		}

		layout.meta_part_start_lba  = UDF_ROUNDUP(pos, layout.meta_alignment);
		pos = layout.meta_part_start_lba + layout.meta_part_size_lba;
	}

	if (context.metadata_part == context.data_part) {
		mpos = pos;
		layout.fsd           = mpos;	mpos += 1;
		layout.rootdir       = mpos;
		pos = mpos;
	} else {
		mpos = 0;
		layout.fsd           = mpos;	mpos += 1;
		layout.rootdir       = mpos;
	}

	/* pos and mpos now refer to the rootdir block */
	context.alloc_pos[context.data_part] = pos;
	context.alloc_pos[context.metadata_part] = mpos;

	udf_dump_layout();

	kbsize = (uint64_t) last_lba * sector_size;
	printf("Total space on this medium approx. "
			"%"PRIu64" KiB, %"PRIu64" MiB\n",
			kbsize/1024, kbsize/(1024*1024));
	kbsize = (uint64_t)(layout.part_size_lba - layout.alloc_bitmap_dscr_size
		- layout.meta_bitmap_dscr_size) * sector_size;
	printf("Recordable free space on this volume approx.  "
			"%"PRIu64" KiB, %"PRIu64" MiB\n\n",
			kbsize/1024, kbsize/(1024*1024));

	return 0;
}


/*
 * Check if the blob starts with a good UDF tag. Tags are protected by a
 * checksum over the header, except one byte at position 4 that is the
 * checksum itself.
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


/*
 * check tag payload will check descriptor CRC as specified.
 * If the descriptor is too long, it will return EIO otherwise EINVAL.
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


int
udf_check_tag_and_location(void *blob, uint32_t location)
{
	struct desc_tag *tag = blob;

	if (udf_check_tag(blob))
		return 1;
	if (udf_rw32(tag->tag_loc) != location)
		return 1;
	return 0;
}


int
udf_validate_tag_sum(union dscrptr *dscr)
{
	struct desc_tag *tag = &dscr->tag;
	uint8_t *pos, sum, cnt;

	/* calculate TAG header checksum */
	pos = (uint8_t *) tag;
	sum = 0;

	for (cnt = 0; cnt < 16; cnt++) {
		if (cnt != 4) sum += *pos;
		pos++;
	};
	tag->cksum = sum;	/* 8 bit */

	return 0;
}


/* assumes sector number of descriptor to be already present */
int
udf_validate_tag_and_crc_sums(union dscrptr *dscr)
{
	struct desc_tag *tag = &dscr->tag;
	uint16_t crc;

	/* check payload CRC if applicable */
	if (udf_rw16(tag->desc_crc_len) > 0) {
		crc = udf_cksum(((uint8_t *) tag) + UDF_DESC_TAG_LENGTH,
			udf_rw16(tag->desc_crc_len));
		tag->desc_crc = udf_rw16(crc);
	};

	/* calculate TAG header checksum */
	return udf_validate_tag_sum(dscr);
}


void
udf_inittag(struct desc_tag *tag, int tagid, uint32_t loc)
{
	tag->id 		= udf_rw16(tagid);
	tag->descriptor_ver	= udf_rw16(context.dscrver);
	tag->cksum		= 0;
	tag->reserved		= 0;
	tag->serial_num		= udf_rw16(context.serialnum);
	tag->tag_loc            = udf_rw32(loc);
}


int
udf_create_anchor(int num)
{
	struct anchor_vdp *avdp;
	uint32_t vds1_extent_len = layout.vds1_size * context.sector_size;
	uint32_t vds2_extent_len = layout.vds2_size * context.sector_size;

	avdp = context.anchors[num];
	if (!avdp)
		if ((avdp = calloc(1, context.sector_size)) == NULL)
			return ENOMEM;

	udf_inittag(&avdp->tag, TAGID_ANCHOR, layout.anchors[num]);

	avdp->main_vds_ex.loc = udf_rw32(layout.vds1);
	avdp->main_vds_ex.len = udf_rw32(vds1_extent_len);

	avdp->reserve_vds_ex.loc = udf_rw32(layout.vds2);
	avdp->reserve_vds_ex.len = udf_rw32(vds2_extent_len);

	/* CRC length for an anchor is 512 - tag length; defined in Ecma 167 */
	avdp->tag.desc_crc_len = udf_rw16(512-UDF_DESC_TAG_LENGTH);

	context.anchors[num] = avdp;
	return 0;
}


void
udf_create_terminator(union dscrptr *dscr, uint32_t loc)
{
	memset(dscr, 0, context.sector_size);
	udf_inittag(&dscr->tag, TAGID_TERM, loc);

	/* CRC length for an anchor is 512 - tag length; defined in Ecma 167 */
	dscr->tag.desc_crc_len = udf_rw16(512-UDF_DESC_TAG_LENGTH);
}


void
udf_osta_charset(struct charspec *charspec)
{
	memset(charspec, 0, sizeof(*charspec));
	charspec->type = 0;
	strcpy((char *) charspec->inf, "OSTA Compressed Unicode");
}


/* ---- shared from kernel's udf_subr.c, slightly modified ---- */
void
udf_to_unix_name(char *result, int result_len, char *id, int len,
	struct charspec *chsp)
{
	uint16_t   *raw_name, *unix_name;
	uint16_t   *inchp, ch;
	char	   *outchp;
	const char *osta_id = "OSTA Compressed Unicode";
	int         ucode_chars, nice_uchars, is_osta_typ0, nout;

	raw_name = malloc(2048 * sizeof(uint16_t));
	assert(raw_name);

	unix_name = raw_name + 1024;			/* split space in half */
	assert(sizeof(char) == sizeof(uint8_t));
	outchp = result;

	is_osta_typ0  = (chsp->type == 0);
	is_osta_typ0 &= (strcmp((char *) chsp->inf, osta_id) == 0);
	if (is_osta_typ0) {
		/* TODO clean up */
		*raw_name = *unix_name = 0;
		ucode_chars = udf_UncompressUnicode(len, (uint8_t *) id, raw_name);
		ucode_chars = MIN(ucode_chars, UnicodeLength((unicode_t *) raw_name));
		nice_uchars = UDFTransName(unix_name, raw_name, ucode_chars);
		/* output UTF8 */
		for (inchp = unix_name; nice_uchars>0; inchp++, nice_uchars--) {
			ch = *inchp;
			nout = wput_utf8(outchp, result_len, ch);
			outchp += nout; result_len -= nout;
			if (!ch) break;
		}
		*outchp++ = 0;
	} else {
		/* assume 8bit char length byte latin-1 */
		assert(*id == 8);
		assert(strlen((char *) (id+1)) <= NAME_MAX);
		memcpy((char *) result, (char *) (id+1), strlen((char *) (id+1)));
	}
	free(raw_name);
}


void
unix_to_udf_name(char *result, uint8_t *result_len, char const *name, int name_len,
	struct charspec *chsp)
{
	uint16_t   *raw_name;
	uint16_t   *outchp;
	const char *inchp;
	const char *osta_id = "OSTA Compressed Unicode";
	int         udf_chars, is_osta_typ0, bits;
	size_t      cnt;

	/* allocate temporary unicode-16 buffer */
	raw_name = malloc(1024);
	assert(raw_name);

	/* convert utf8 to unicode-16 */
	*raw_name = 0;
	inchp  = name;
	outchp = raw_name;
	bits = 8;
	for (cnt = name_len, udf_chars = 0; cnt;) {
		*outchp = wget_utf8(&inchp, &cnt);
		if (*outchp > 0xff)
			bits=16;
		outchp++;
		udf_chars++;
	}
	/* null terminate just in case */
	*outchp++ = 0;

	is_osta_typ0  = (chsp->type == 0);
	is_osta_typ0 &= (strcmp((char *) chsp->inf, osta_id) == 0);
	if (is_osta_typ0) {
		udf_chars = udf_CompressUnicode(udf_chars, bits,
				(unicode_t *) raw_name,
				(byte *) result);
	} else {
		printf("unix to udf name: no CHSP0 ?\n");
		/* XXX assume 8bit char length byte latin-1 */
		*result++ = 8; udf_chars = 1;
		strncpy(result, name + 1, name_len);
		udf_chars += name_len;
	}
	*result_len = udf_chars;
	free(raw_name);
}


/* first call udf_set_regid and then the suffix */
void
udf_set_regid(struct regid *regid, char const *name)
{
	memset(regid, 0, sizeof(*regid));
	regid->flags    = 0;		/* not dirty and not protected */
	strcpy((char *) regid->id, name);
}


void
udf_add_domain_regid(struct regid *regid)
{
	uint16_t *ver;

	ver  = (uint16_t *) regid->id_suffix;
	*ver = udf_rw16(context.min_udf);
}


void
udf_add_udf_regid(struct regid *regid)
{
	uint16_t *ver;

	ver  = (uint16_t *) regid->id_suffix;
	*ver = udf_rw16(context.min_udf);

	regid->id_suffix[2] = 4;	/* unix */
	regid->id_suffix[3] = 8;	/* NetBSD */
}


void
udf_add_impl_regid(struct regid *regid)
{
	regid->id_suffix[0] = 4;	/* unix */
	regid->id_suffix[1] = 8;	/* NetBSD */
}


void
udf_add_app_regid(struct regid *regid)
{
	regid->id_suffix[0] = context.app_version_main;
	regid->id_suffix[1] = context.app_version_sub;
}


/*
 * Timestamp to timespec conversion code is taken with small modifications
 * from FreeBSD /sys/fs/udf by Scott Long <scottl@freebsd.org>
 */

static int mon_lens[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static int
udf_isaleapyear(int year)
{
	int i;

	i = (year % 4) ? 0 : 1;
	i &= (year % 100) ? 1 : 0;
	i |= (year % 400) ? 0 : 1;

	return i;
}


void
udf_timestamp_to_timespec(struct timestamp *timestamp, struct timespec *timespec)
{
	uint32_t usecs, secs, nsecs;
	uint16_t tz;
	int i, lpyear, daysinyear, year;

	timespec->tv_sec  = secs  = 0;
	timespec->tv_nsec = nsecs = 0;

       /*
	* DirectCD seems to like using bogus year values.
	* Distrust time->month especially, since it will be used for an array
	* index.
	*/
	year = udf_rw16(timestamp->year);
	if ((year < 1970) || (timestamp->month > 12)) {
		return;
	}

	/* Calculate the time and day */
	usecs = timestamp->usec + 100*timestamp->hund_usec + 10000*timestamp->centisec;
	nsecs = usecs * 1000;
	secs  = timestamp->second;
	secs += timestamp->minute * 60;
	secs += timestamp->hour * 3600;
	secs += (timestamp->day-1) * 3600 * 24;			/* day : 1-31 */

	/* Calclulate the month */
	lpyear = udf_isaleapyear(year);
	for (i = 1; i < timestamp->month; i++)
		secs += mon_lens[lpyear][i-1] * 3600 * 24;	/* month: 1-12 */

	for (i = 1970; i < year; i++) {
		daysinyear = udf_isaleapyear(i) + 365 ;
		secs += daysinyear * 3600 * 24;
	}

	/*
	 * Calculate the time zone.  The timezone is 12 bit signed 2's
	 * compliment, so we gotta do some extra magic to handle it right.
	 */
	tz  = udf_rw16(timestamp->type_tz);
	tz &= 0x0fff;				/* only lower 12 bits are significant */
	if (tz & 0x0800)			/* sign extention */
		tz |= 0xf000;

	/* TODO check timezone conversion */
#if 1
	/* check if we are specified a timezone to convert */
	if (udf_rw16(timestamp->type_tz) & 0x1000)
		if ((int16_t) tz != -2047)
			secs -= (int16_t) tz * 60;
#endif
	timespec->tv_sec  = secs;
	timespec->tv_nsec = nsecs;
}


/*
 * Fill in timestamp structure based on clock_gettime(). Time is reported back
 * as a time_t accompanied with a nano second field.
 *
 * The husec, usec and csec could be relaxed in type.
 */
void
udf_timespec_to_timestamp(struct timespec *timespec, struct timestamp *timestamp)
{
	struct tm tm;
	uint64_t husec, usec, csec;

	memset(timestamp, 0, sizeof(*timestamp));
	gmtime_r(&timespec->tv_sec, &tm);

	/*
	 * Time type and time zone : see ECMA 1/7.3, UDF 2., 2.1.4.1, 3.1.1.
	 *
	 * Lower 12 bits are two complement signed timezone offset if bit 12
	 * (method 1) is clear. Otherwise if bit 12 is set, specify timezone
	 * offset to -2047 i.e. unsigned `zero'
	 */

	/* set method 1 for CUT/GMT */
	timestamp->type_tz	= udf_rw16((1<<12) + 0);
	timestamp->year		= udf_rw16(tm.tm_year + 1900);
	timestamp->month	= tm.tm_mon + 1;	/* `tm' uses 0..11 for months */
	timestamp->day		= tm.tm_mday;
	timestamp->hour		= tm.tm_hour;
	timestamp->minute	= tm.tm_min;
	timestamp->second	= tm.tm_sec;

	usec   = (timespec->tv_nsec + 500) / 1000;	/* round */
	husec  =   usec / 100;
	usec  -=  husec * 100;				/* only 0-99 in usec  */
	csec   =  husec / 100;				/* only 0-99 in csec  */
	husec -=   csec * 100;				/* only 0-99 in husec */

	/* in rare cases there is overflow in csec */
	csec  = MIN(99, csec);
	husec = MIN(99, husec);
	usec  = MIN(99, usec);

	timestamp->centisec	= csec;
	timestamp->hund_usec	= husec;
	timestamp->usec		= usec;
}


static void
udf_set_timestamp(struct timestamp *timestamp, time_t value)
{
	struct timespec t;

	memset(&t, 0, sizeof(struct timespec));
	t.tv_sec  = value;
	t.tv_nsec = 0;
	udf_timespec_to_timestamp(&t, timestamp);
}


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

/* end of copied code */


void
udf_encode_osta_id(char *osta_id, uint16_t len, char *text)
{
	struct charspec osta_charspec;
	uint8_t result_len;

	memset(osta_id, 0, len);
	if (!text || (strlen(text) == 0)) return;

	udf_osta_charset(&osta_charspec);
	unix_to_udf_name(osta_id, &result_len, text, strlen(text),
		&osta_charspec);

	/* Ecma 167/7.2.13 states that length is recorded in the last byte */
	osta_id[len-1] = strlen(text)+1;
}


void
udf_set_timestamp_now(struct timestamp *timestamp)
{
	struct timespec now;

#ifdef CLOCK_REALTIME
	(void)clock_gettime(CLOCK_REALTIME, &now);
#else
	struct timeval time_of_day;

	(void)gettimeofday(&time_of_day, NULL);
	now.tv_sec = time_of_day.tv_sec;
	now.tv_nsec = time_of_day.tv_usec * 1000;
#endif
	udf_timespec_to_timestamp(&now, timestamp);
}


int
udf_create_primaryd(void)
{
	struct pri_vol_desc *pri;
	uint16_t crclen;

	pri = calloc(1, context.sector_size);
	if (pri == NULL)
		return ENOMEM;

	memset(pri, 0, context.sector_size);
	udf_inittag(&pri->tag, TAGID_PRI_VOL, /* loc */ 0);
	pri->seq_num = udf_rw32(context.vds_seq); context.vds_seq++;

	pri->pvd_num = udf_rw32(0);		/* default serial */
	udf_encode_osta_id(pri->vol_id, 32, context.primary_name);

	/* set defaults for single disc volumes as UDF prescribes */
	pri->vds_num      = udf_rw16(1);
	pri->max_vol_seq  = udf_rw16(1);
	pri->ichg_lvl     = udf_rw16(2);
	pri->max_ichg_lvl = udf_rw16(3);
	pri->flags        = udf_rw16(0);

	pri->charset_list     = udf_rw32(1);	/* only CS0 */
	pri->max_charset_list = udf_rw32(1);	/* only CS0 */

	udf_encode_osta_id(pri->volset_id, 128, context.volset_name);
	udf_osta_charset(&pri->desc_charset);
	udf_osta_charset(&pri->explanatory_charset);

	udf_set_regid(&pri->app_id, context.app_name);
	udf_add_app_regid(&pri->app_id);

	udf_set_regid(&pri->imp_id, context.impl_name);
	udf_add_impl_regid(&pri->imp_id);

	udf_set_timestamp_now(&pri->time);

	crclen = sizeof(struct pri_vol_desc) - UDF_DESC_TAG_LENGTH;
	pri->tag.desc_crc_len = udf_rw16(crclen);

	context.primary_vol = pri;

	return 0;
}


/*
 * BUGALERT: some rogue implementations use random physical partition
 * numbers to break other implementations so lookup the number.
 */

uint16_t
udf_find_raw_phys(uint16_t raw_phys_part)
{
	struct part_desc *part;
	uint16_t phys_part;

	for (phys_part = 0; phys_part < UDF_PARTITIONS; phys_part++) {
		part = context.partitions[phys_part];
		if (part == NULL)
			break;
		if (udf_rw16(part->part_num) == raw_phys_part)
			break;
	}
	return phys_part;
}


/* XXX no support for unallocated or freed space tables yet (!) */
int
udf_create_partitiond(int part_num)
{
	struct part_desc     *pd;
	struct part_hdr_desc *phd;
	uint32_t sector_size, bitmap_bytes;
	uint16_t crclen;
	int part_accesstype = context.media_accesstype;

	sector_size = context.sector_size;
	bitmap_bytes = layout.alloc_bitmap_dscr_size * sector_size;

	if (context.partitions[part_num])
		errx(1, "internal error, partition %d already defined in %s",
			part_num, __func__);

	pd = calloc(1, context.sector_size);
	if (pd == NULL)
		return ENOMEM;
	phd = &pd->_impl_use.part_hdr;

	udf_inittag(&pd->tag, TAGID_PARTITION, /* loc */ 0);
	pd->seq_num  = udf_rw32(context.vds_seq); context.vds_seq++;

	pd->flags    = udf_rw16(1);		/* allocated */
	pd->part_num = udf_rw16(part_num);	/* only one physical partition */

	if (context.dscrver == 2) {
		udf_set_regid(&pd->contents, "+NSR02");
	} else {
		udf_set_regid(&pd->contents, "+NSR03");
	}
	udf_add_app_regid(&pd->contents);

	phd->unalloc_space_bitmap.len    = udf_rw32(bitmap_bytes);
	phd->unalloc_space_bitmap.lb_num = udf_rw32(layout.unalloc_space);

	if (layout.freed_space) {
		phd->freed_space_bitmap.len    = udf_rw32(bitmap_bytes);
		phd->freed_space_bitmap.lb_num = udf_rw32(layout.freed_space);
	}

	pd->access_type = udf_rw32(part_accesstype);
	pd->start_loc   = udf_rw32(layout.part_start_lba);
	pd->part_len    = udf_rw32(layout.part_size_lba);

	udf_set_regid(&pd->imp_id, context.impl_name);
	udf_add_impl_regid(&pd->imp_id);

	crclen = sizeof(struct part_desc) - UDF_DESC_TAG_LENGTH;
	pd->tag.desc_crc_len = udf_rw16(crclen);

	context.partitions[part_num] = pd;

	return 0;
}


int
udf_create_unalloc_spaced(void)
{
	struct unalloc_sp_desc *usd;
	uint16_t crclen;

	usd = calloc(1, context.sector_size);
	if (usd == NULL)
		return ENOMEM;

	udf_inittag(&usd->tag, TAGID_UNALLOC_SPACE, /* loc */ 0);
	usd->seq_num  = udf_rw32(context.vds_seq); context.vds_seq++;

	/* no default entries */
	usd->alloc_desc_num = udf_rw32(0);		/* no entries */

	crclen  = sizeof(struct unalloc_sp_desc) - sizeof(struct extent_ad);
	crclen -= UDF_DESC_TAG_LENGTH;
	usd->tag.desc_crc_len = udf_rw16(crclen);

	context.unallocated = usd;

	return 0;
}


static int
udf_create_base_logical_dscr(void)
{
	struct logvol_desc *lvd;
	uint32_t sector_size;
	uint16_t crclen;

	sector_size = context.sector_size;

	lvd = calloc(1, sector_size);
	if (lvd == NULL)
		return ENOMEM;

	udf_inittag(&lvd->tag, TAGID_LOGVOL, /* loc */ 0);
	lvd->seq_num  = udf_rw32(context.vds_seq); context.vds_seq++;

	udf_osta_charset(&lvd->desc_charset);
	udf_encode_osta_id(lvd->logvol_id, 128, context.logvol_name);
	lvd->lb_size = udf_rw32(sector_size);

	udf_set_regid(&lvd->domain_id, "*OSTA UDF Compliant");
	udf_add_domain_regid(&lvd->domain_id);

	/* no partition mappings/entries yet */
	lvd->mt_l = udf_rw32(0);
	lvd->n_pm = udf_rw32(0);

	udf_set_regid(&lvd->imp_id, context.impl_name);
	udf_add_impl_regid(&lvd->imp_id);

	lvd->integrity_seq_loc.loc = udf_rw32(layout.lvis);
	lvd->integrity_seq_loc.len = udf_rw32(layout.lvis_size * sector_size);

	/* just one fsd for now */
	lvd->lv_fsd_loc.len = udf_rw32(sector_size);
	lvd->lv_fsd_loc.loc.part_num = udf_rw16(context.metadata_part);
	lvd->lv_fsd_loc.loc.lb_num   = udf_rw32(layout.fsd);

	crclen  = sizeof(struct logvol_desc) - 1 - UDF_DESC_TAG_LENGTH;
	lvd->tag.desc_crc_len = udf_rw16(crclen);

	context.logical_vol = lvd;
	context.vtop_tp[UDF_VTOP_RAWPART]     = UDF_VTOP_TYPE_RAW;

	return 0;
}


static void
udf_add_logvol_part_physical(uint16_t phys_part)
{
	struct logvol_desc *logvol = context.logical_vol;
	union  udf_pmap *pmap;
	uint8_t         *pmap_pos;
	uint16_t crclen;
	uint32_t pmap1_size, log_part;

	log_part = udf_rw32(logvol->n_pm);
	pmap_pos = logvol->maps + udf_rw32(logvol->mt_l);
	pmap1_size = sizeof(struct part_map_1);

	pmap = (union udf_pmap *) pmap_pos;
	pmap->pm1.type        = 1;
	pmap->pm1.len         = sizeof(struct part_map_1);
	pmap->pm1.vol_seq_num = udf_rw16(1);		/* no multi-volume */
	pmap->pm1.part_num    = udf_rw16(phys_part);

	context.vtop       [log_part] = phys_part;
	context.vtop_tp    [log_part] = UDF_VTOP_TYPE_PHYS;
	context.part_size[log_part] = layout.part_size_lba;
	context.part_free[log_part] = layout.part_size_lba;

	/* increment number of partitions and length */
	logvol->n_pm = udf_rw32(log_part + 1);
	logvol->mt_l = udf_rw32(udf_rw32(logvol->mt_l) + pmap1_size);

	crclen = udf_rw16(logvol->tag.desc_crc_len) + pmap1_size;
	logvol->tag.desc_crc_len = udf_rw16(crclen);
}


static void
udf_add_logvol_part_virtual(uint16_t phys_part)
{
	union  udf_pmap *pmap;
	struct logvol_desc *logvol = context.logical_vol;
	uint8_t *pmap_pos;
	uint16_t crclen;
	uint32_t pmapv_size, log_part;

	log_part = udf_rw32(logvol->n_pm);
	pmap_pos = logvol->maps + udf_rw32(logvol->mt_l);
	pmapv_size = sizeof(struct part_map_2);

	pmap = (union udf_pmap *) pmap_pos;
	pmap->pmv.type        = 2;
	pmap->pmv.len         = pmapv_size;

	udf_set_regid(&pmap->pmv.id, "*UDF Virtual Partition");
	udf_add_udf_regid(&pmap->pmv.id);

	pmap->pmv.vol_seq_num = udf_rw16(1);		/* no multi-volume */
	pmap->pmv.part_num    = udf_rw16(phys_part);

	context.vtop       [log_part] = phys_part;
	context.vtop_tp    [log_part] = UDF_VTOP_TYPE_VIRT;
	context.part_size[log_part] = 0xffffffff;
	context.part_free[log_part] = 0xffffffff;

	/* increment number of partitions and length */
	logvol->n_pm = udf_rw32(log_part + 1);
	logvol->mt_l = udf_rw32(udf_rw32(logvol->mt_l) + pmapv_size);

	crclen = udf_rw16(logvol->tag.desc_crc_len) + pmapv_size;
	logvol->tag.desc_crc_len = udf_rw16(crclen);
}


/* sparing table size is in bytes */
static void
udf_add_logvol_part_spareable(uint16_t phys_part)
{
	union  udf_pmap *pmap;
	struct logvol_desc *logvol = context.logical_vol;
	uint32_t *st_pos, spareable_bytes, pmaps_size;
	uint8_t  *pmap_pos, num;
	uint16_t crclen;
	uint32_t log_part;

	log_part = udf_rw32(logvol->n_pm);
	pmap_pos = logvol->maps + udf_rw32(logvol->mt_l);
	pmaps_size = sizeof(struct part_map_2);
	spareable_bytes = layout.spareable_area_size * context.sector_size;

	pmap = (union udf_pmap *) pmap_pos;
	pmap->pms.type        = 2;
	pmap->pms.len         = pmaps_size;

	udf_set_regid(&pmap->pmv.id, "*UDF Sparable Partition");
	udf_add_udf_regid(&pmap->pmv.id);

	pmap->pms.vol_seq_num = udf_rw16(1);		/* no multi-volume */
	pmap->pms.part_num    = udf_rw16(phys_part);

	pmap->pms.packet_len  = udf_rw16(layout.spareable_blockingnr);
	pmap->pms.st_size     = udf_rw32(spareable_bytes);

	/* enter spare tables  */
	st_pos = &pmap->pms.st_loc[0];
	*st_pos++ = udf_rw32(layout.spt_1);
	*st_pos++ = udf_rw32(layout.spt_2);

	num = 2;
	if (layout.spt_2 == 0) num--;
	if (layout.spt_1 == 0) num--;
	pmap->pms.n_st = num;		/* 8 bit */

	context.vtop       [log_part] = phys_part;
	context.vtop_tp    [log_part] = UDF_VTOP_TYPE_SPAREABLE;
	context.part_size[log_part] = layout.part_size_lba;
	context.part_free[log_part] = layout.part_size_lba;

	/* increment number of partitions and length */
	logvol->n_pm = udf_rw32(log_part + 1);
	logvol->mt_l = udf_rw32(udf_rw32(logvol->mt_l) + pmaps_size);

	crclen = udf_rw16(logvol->tag.desc_crc_len) + pmaps_size;
	logvol->tag.desc_crc_len = udf_rw16(crclen);
}


int
udf_create_sparing_tabled(void)
{
	struct udf_sparing_table *spt;
	struct spare_map_entry   *sme;
	uint32_t loc, cnt;
	uint32_t crclen;	/* XXX: should be 16; need to detect overflow */

	spt = calloc(context.sector_size, layout.sparing_table_dscr_lbas);
	if (spt == NULL)
		return ENOMEM;

	/* a sparing table descriptor is a whole spareable_blockingnr sectors */
	udf_inittag(&spt->tag, TAGID_SPARING_TABLE, /* loc */ 0);

	udf_set_regid(&spt->id, "*UDF Sparing Table");
	udf_add_udf_regid(&spt->id);

	spt->rt_l    = udf_rw16(layout.spareable_blocks);
	spt->seq_num = udf_rw32(0);			/* first generation */

	for (cnt = 0; cnt < layout.spareable_blocks; cnt++) {
		sme = &spt->entries[cnt];
		loc = layout.spareable_area + cnt * layout.spareable_blockingnr;
		sme->org = udf_rw32(0xffffffff);	/* open for reloc */
		sme->map = udf_rw32(loc);
	}

	/* calculate crc len for actual size */
	crclen  = sizeof(struct udf_sparing_table) - UDF_DESC_TAG_LENGTH;
	crclen += (layout.spareable_blocks-1) * sizeof(struct spare_map_entry);

	assert(crclen <= UINT16_MAX);
	spt->tag.desc_crc_len = udf_rw16((uint16_t)crclen);

	context.sparing_table = spt;

	return 0;
}


static void
udf_add_logvol_part_meta(uint16_t phys_part)
{
	union  udf_pmap *pmap;
	struct logvol_desc *logvol = context.logical_vol;
	uint8_t *pmap_pos;
	uint32_t pmapv_size, log_part;
	uint16_t crclen;

	log_part = udf_rw32(logvol->n_pm);
	pmap_pos = logvol->maps + udf_rw32(logvol->mt_l);
	pmapv_size = sizeof(struct part_map_2);

	pmap = (union udf_pmap *) pmap_pos;
	pmap->pmm.type        = 2;
	pmap->pmm.len         = pmapv_size;

	udf_set_regid(&pmap->pmm.id, "*UDF Metadata Partition");
	udf_add_udf_regid(&pmap->pmm.id);

	pmap->pmm.vol_seq_num = udf_rw16(1);		/* no multi-volume */
	pmap->pmm.part_num    = udf_rw16(phys_part);

	/* fill in meta data file(s) and alloc/alignment unit sizes */
	pmap->pmm.meta_file_lbn        = udf_rw32(layout.meta_file);
	pmap->pmm.meta_mirror_file_lbn = udf_rw32(layout.meta_mirror);
	pmap->pmm.meta_bitmap_file_lbn = udf_rw32(layout.meta_bitmap);
	pmap->pmm.alloc_unit_size      = udf_rw32(layout.meta_blockingnr);
	pmap->pmm.alignment_unit_size  = udf_rw16(layout.meta_alignment);
	pmap->pmm.flags                = 0; /* METADATA_DUPLICATED */

	context.vtop       [log_part] = phys_part;
	context.vtop_tp    [log_part] = UDF_VTOP_TYPE_META;
	context.part_size[log_part] = layout.meta_part_size_lba;
	context.part_free[log_part] = layout.meta_part_size_lba;

	/* increment number of partitions and length */
	logvol->n_pm = udf_rw32(log_part + 1);
	logvol->mt_l = udf_rw32(udf_rw32(logvol->mt_l) + pmapv_size);

	crclen = udf_rw16(logvol->tag.desc_crc_len) + pmapv_size;
	logvol->tag.desc_crc_len = udf_rw16(crclen);
}


int
udf_create_logical_dscr(void)
{
	int error;

	if ((error = udf_create_base_logical_dscr()))
		return error;

	/* we pass data_part for there might be a read-only part one day */
	if (context.format_flags & FORMAT_SPAREABLE) {
		/* spareable partition mapping has no physical mapping */
		udf_add_logvol_part_spareable(context.data_part);
	} else {
		udf_add_logvol_part_physical(context.data_part);
	}

	if (context.format_flags & FORMAT_VAT) {
		/* add VAT virtual mapping; reflects on datapart */
		udf_add_logvol_part_virtual(context.data_part);
	}
	if (context.format_flags & FORMAT_META) {
		/* add META data mapping; reflects on datapart */
		udf_add_logvol_part_meta(context.data_part);
	}

	return 0;
}


int
udf_create_impvold(char *field1, char *field2, char *field3)
{
	struct impvol_desc *ivd;
	struct udf_lv_info *lvi;
	uint16_t crclen;

	ivd = calloc(1, context.sector_size);
	if (ivd == NULL)
		return ENOMEM;
	lvi = &ivd->_impl_use.lv_info;

	udf_inittag(&ivd->tag, TAGID_IMP_VOL, /* loc */ 0);
	ivd->seq_num  = udf_rw32(context.vds_seq); context.vds_seq++;

	udf_set_regid(&ivd->impl_id, "*UDF LV Info");
	udf_add_udf_regid(&ivd->impl_id);

	/* fill in UDF specific part */
	udf_osta_charset(&lvi->lvi_charset);
	udf_encode_osta_id(lvi->logvol_id, 128, context.logvol_name);

	udf_encode_osta_id(lvi->lvinfo1, 36, field1);
	udf_encode_osta_id(lvi->lvinfo2, 36, field2);
	udf_encode_osta_id(lvi->lvinfo3, 36, field3);

	udf_set_regid(&lvi->impl_id, context.impl_name);
	udf_add_impl_regid(&lvi->impl_id);

	crclen  = sizeof(struct impvol_desc) - UDF_DESC_TAG_LENGTH;
	ivd->tag.desc_crc_len = udf_rw16(crclen);

	context.implementation = ivd;

	return 0;
}


/* XXX might need to be sanitised a bit */
void
udf_update_lvintd(int type)
{
	struct logvol_int_desc *lvid;
	struct udf_logvol_info *lvinfo;
	struct logvol_desc     *logvol;
	uint32_t *pos;
	uint32_t cnt, num_partmappings;
	uint32_t crclen;	/* XXX: should be 16; need to detect overflow */

	lvid   = context.logvol_integrity;
	logvol = context.logical_vol;
	assert(lvid);
	assert(logvol);

	lvid->integrity_type = udf_rw32(type);
	udf_set_timestamp_now(&lvid->time);

	/* initialise lvinfo just in case its not set yet */
	num_partmappings = udf_rw32(logvol->n_pm);
	assert(num_partmappings > 0);

	lvinfo = (struct udf_logvol_info *)
		(lvid->tables + num_partmappings * 2);
	context.logvol_info = lvinfo;

	udf_set_regid(&lvinfo->impl_id, context.impl_name);
	udf_add_impl_regid(&lvinfo->impl_id);

	if (type == UDF_INTEGRITY_CLOSED) {
		lvinfo->num_files          = udf_rw32(context.num_files);
		lvinfo->num_directories    = udf_rw32(context.num_directories);

		lvid->lvint_next_unique_id = udf_rw64(context.unique_id);
	}

	/* sane enough? */
	if (udf_rw16(lvinfo->min_udf_readver) < context.min_udf)
		lvinfo->min_udf_readver  = udf_rw16(context.min_udf);
	if (udf_rw16(lvinfo->min_udf_writever) < context.min_udf)
		lvinfo->min_udf_writever = udf_rw16(context.min_udf);
	if (udf_rw16(lvinfo->max_udf_writever) < context.max_udf)
		lvinfo->max_udf_writever = udf_rw16(context.max_udf);

	lvid->num_part = udf_rw32(num_partmappings);

	pos = &lvid->tables[0];
	for (cnt = 0; cnt < num_partmappings; cnt++) {
		*pos++ = udf_rw32(context.part_free[cnt]);
	}
	for (cnt = 0; cnt < num_partmappings; cnt++) {
		*pos++ = udf_rw32(context.part_size[cnt]);
	}

	crclen  = sizeof(struct logvol_int_desc) -4 -UDF_DESC_TAG_LENGTH +
		udf_rw32(lvid->l_iu);
	crclen += num_partmappings * 2 * 4;

	assert(crclen <= UINT16_MAX);
	if (lvid->tag.desc_crc_len == 0)
		lvid->tag.desc_crc_len = udf_rw16(crclen);

	context.logvol_info = lvinfo;
}


int
udf_create_lvintd(int type)
{
	struct logvol_int_desc *lvid;
	int l_iu;

	lvid = calloc(1, context.sector_size);
	if (lvid == NULL)
		return ENOMEM;

	udf_inittag(&lvid->tag, TAGID_LOGVOL_INTEGRITY, /* loc */ 0);
	context.logvol_integrity = lvid;

	/* only set for standard UDF info, no extra impl. use needed */
	l_iu = sizeof(struct udf_logvol_info);
	lvid->l_iu = udf_rw32(l_iu);

	udf_update_lvintd(type);

	return 0;
}


int
udf_create_fsd(void)
{
	struct fileset_desc *fsd;
	uint16_t crclen;

	fsd = calloc(1, context.sector_size);
	if (fsd == NULL)
		return ENOMEM;

	udf_inittag(&fsd->tag, TAGID_FSD, /* loc */ 0);

	udf_set_timestamp_now(&fsd->time);
	fsd->ichg_lvl     = udf_rw16(3);		/* UDF 2.3.2.1 */
	fsd->max_ichg_lvl = udf_rw16(3);		/* UDF 2.3.2.2 */

	fsd->charset_list     = udf_rw32(1);		/* only CS0 */
	fsd->max_charset_list = udf_rw32(1);		/* only CS0 */

	fsd->fileset_num      = udf_rw32(0);		/* only one fsd */
	fsd->fileset_desc_num = udf_rw32(0);		/* original    */

	udf_osta_charset(&fsd->logvol_id_charset);
	udf_encode_osta_id(fsd->logvol_id, 128, context.logvol_name);

	udf_osta_charset(&fsd->fileset_charset);
	udf_encode_osta_id(fsd->fileset_id, 32, context.fileset_name);

	/* copyright file and abstract file names obmitted */

	fsd->rootdir_icb.len	      = udf_rw32(context.sector_size);
	fsd->rootdir_icb.loc.lb_num   = udf_rw32(layout.rootdir);
	fsd->rootdir_icb.loc.part_num = udf_rw16(context.metadata_part);

	udf_set_regid(&fsd->domain_id, "*OSTA UDF Compliant");
	udf_add_domain_regid(&fsd->domain_id);

	/* next_ex stays zero */
	/* no system streamdirs yet */

	crclen = sizeof(struct fileset_desc) - UDF_DESC_TAG_LENGTH;
	fsd->tag.desc_crc_len = udf_rw16(crclen);

	context.fileset_desc = fsd;

	return 0;
}


int
udf_create_space_bitmap(uint32_t dscr_size, uint32_t part_size_lba,
	struct space_bitmap_desc **sbdp)
{
	struct space_bitmap_desc *sbd;
	uint32_t cnt;
	uint16_t crclen;

	*sbdp = NULL;
	sbd = calloc(context.sector_size, dscr_size);
	if (sbd == NULL)
		return ENOMEM;

	udf_inittag(&sbd->tag, TAGID_SPACE_BITMAP, /* loc */ 0);

	sbd->num_bits  = udf_rw32(part_size_lba);
	sbd->num_bytes = udf_rw32((part_size_lba + 7)/8);

	/* fill space with 0xff to indicate free */
	for (cnt = 0; cnt < udf_rw32(sbd->num_bytes); cnt++)
		sbd->data[cnt] = 0xff;

	/* set crc to only cover the header (UDF 2.3.1.2, 2.3.8.1) */
	crclen = sizeof(struct space_bitmap_desc) -1 - UDF_DESC_TAG_LENGTH;
	sbd->tag.desc_crc_len = udf_rw16(crclen);

	*sbdp = sbd;
	return 0;
}

/* --------------------------------------------------------------------- */

int
udf_register_bad_block(uint32_t location)
{
	struct udf_sparing_table *spt;
	struct spare_map_entry   *sme, *free_sme;
	uint32_t cnt;

	spt = context.sparing_table;
	if (spt == NULL)
		errx(1, "internal error, adding bad block to "
			"non spareable in %s", __func__);

	/* find us a free spare map entry */
	free_sme = NULL;
	for (cnt = 0; cnt < layout.spareable_blocks; cnt++) {
		sme = &spt->entries[cnt];
		/* if we are allready in it, bail out */
		if (udf_rw32(sme->org) == location)
			return 0;
		if (udf_rw32(sme->org) == 0xffffffff) {
			free_sme = sme;
			break;
		}
	}
	if (free_sme == NULL) {
		warnx("disc relocation blocks full; disc too damaged");
		return EINVAL;
	}
	free_sme->org = udf_rw32(location);

	return 0;
}


void
udf_mark_allocated(uint32_t start_lb, int partnr, uint32_t blocks)
{
	union dscrptr *dscr;
	uint8_t *bpos;
	uint32_t cnt, bit;

	/* account for space used on underlying partition */
#ifdef DEBUG
	printf("mark allocated : partnr %d, start_lb %d for %d blocks\n",
		partnr, start_lb, blocks);
#endif

	switch (context.vtop_tp[partnr]) {
	case UDF_VTOP_TYPE_VIRT:
		/* nothing */
		break;
	case UDF_VTOP_TYPE_PHYS:
	case UDF_VTOP_TYPE_SPAREABLE:
	case UDF_VTOP_TYPE_META:
		if (context.part_unalloc_bits[context.vtop[partnr]] == NULL) {
			context.part_free[partnr] = 0;
			break;
		}
#ifdef DEBUG
		printf("marking %d+%d as used\n", start_lb, blocks);
#endif
		dscr = (union dscrptr *) (context.part_unalloc_bits[partnr]);
		for (cnt = start_lb; cnt < start_lb + blocks; cnt++) {
			 bpos  = &dscr->sbd.data[cnt / 8];
			 bit   = cnt % 8;
			 /* only account for bits marked free */
			 if ((*bpos & (1 << bit)))
				context.part_free[partnr] -= 1;
			*bpos &= ~(1<< bit);
		}
		break;
	default:
		errx(1, "internal error: bad mapping type %d in %s",
			context.vtop_tp[partnr], __func__);
	}
}


void
udf_advance_uniqueid(void)
{
	/* Minimum value of 16 : UDF 3.2.1.1, 3.3.3.4. */
	context.unique_id++;
	if (context.unique_id < 0x10)
		context.unique_id = 0x10;
}

/* --------------------------------------------------------------------- */

/* XXX implement the using of the results */
int
udf_surface_check(void)
{
	uint32_t loc, block_bytes;
	uint32_t sector_size, blockingnr, bpos;
	uint8_t *buffer;
	int error, num_errors;

	if (mmc_discinfo.mmc_class == MMC_CLASS_DISC)
		return 0;

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
		error = pwrite(dev_fd, buffer, block_bytes,
				(uint64_t) loc*sector_size);
		printf("   %08d + %d (%02d %%)\r", loc, blockingnr,
			(int)((100.0 * loc)/layout.last_lba));
		fflush(stdout);
		if (error == -1) {
			/* block is bad */
			printf("BAD block at %08d + %d         \n",
				loc, layout.blockingnr);
			if ((error = udf_register_bad_block(loc))) {
				free(buffer);
				return error;
			}
			num_errors ++;
		}
		loc += layout.blockingnr;
	}

	printf("\nChecking disc surface : phase 2 - reading\n");
	num_errors = 0;
	loc = layout.first_lba;
	while (loc <= layout.last_lba) {
		/* read blockingnr sectors */
		error = pread(dev_fd, buffer, block_bytes, loc*sector_size);
		printf("   %08d + %d (%02d %%)\r", loc, blockingnr,
			(int)((100.0 * loc)/layout.last_lba));
		fflush(stdout);
		if (error == -1) {
			/* block is bad */
			printf("BAD block at %08d + %d         \n",
				loc, layout.blockingnr);
			if ((error = udf_register_bad_block(loc))) {
				free(buffer);
				return error;
			}
			num_errors ++;
		}
		loc += layout.blockingnr;
	}
	printf("Scan complete : %d bad blocks found\n", num_errors);
	free(buffer);

	return 0;
}

/* --------------------------------------------------------------------- */

#define UDF_SYMLINKBUFLEN    (64*1024)               /* picked */
int
udf_encode_symlink(uint8_t **pathbufp, uint32_t *pathlenp, char *target)
{
	struct charspec osta_charspec;
	struct pathcomp pathcomp;
	char *pathbuf, *pathpos, *compnamepos;
//	char *mntonname;
//	int   mntonnamelen;
	int pathlen, len, compnamelen;
	int error;

	/* process `target' to an UDF structure */
	pathbuf = malloc(UDF_SYMLINKBUFLEN);
	assert(pathbuf);

	*pathbufp = NULL;
	*pathlenp = 0;

	pathpos = pathbuf;
	pathlen = 0;
	udf_osta_charset(&osta_charspec);

	if (*target == '/') {
		/* symlink starts from the root */
		len = UDF_PATH_COMP_SIZE;
		memset(&pathcomp, 0, len);
		pathcomp.type = UDF_PATH_COMP_ROOT;

#if 0
		/* XXX how to check for in makefs? */
		/* check if its mount-point relative! */
		mntonname    = udf_node->ump->vfs_mountp->mnt_stat.f_mntonname;
		mntonnamelen = strlen(mntonname);
		if (strlen(target) >= mntonnamelen) {
			if (strncmp(target, mntonname, mntonnamelen) == 0) {
				pathcomp.type = UDF_PATH_COMP_MOUNTROOT;
				target += mntonnamelen;
			}
		} else {
			target++;
		}
#else
		target++;
#endif

		memcpy(pathpos, &pathcomp, len);
		pathpos += len;
		pathlen += len;
	}

	error = 0;
	while (*target) {
		/* ignore multiple '/' */
		while (*target == '/') {
			target++;
		}
		if (!*target)
			break;

		/* extract component name */
		compnamelen = 0;
		compnamepos = target;
		while ((*target) && (*target != '/')) {
			target++;
			compnamelen++;
		}

		/* just trunc if too long ?? (security issue) */
		if (compnamelen >= 127) {
			error = ENAMETOOLONG;
			break;
		}

		/* convert unix name to UDF name */
		len = sizeof(struct pathcomp);
		memset(&pathcomp, 0, len);
		pathcomp.type = UDF_PATH_COMP_NAME;
		len = UDF_PATH_COMP_SIZE;

		if ((compnamelen == 2) && (strncmp(compnamepos, "..", 2) == 0))
			pathcomp.type = UDF_PATH_COMP_PARENTDIR;
		if ((compnamelen == 1) && (*compnamepos == '.'))
			pathcomp.type = UDF_PATH_COMP_CURDIR;

		if (pathcomp.type == UDF_PATH_COMP_NAME) {
			unix_to_udf_name(
				(char *) &pathcomp.ident, &pathcomp.l_ci,
				compnamepos, compnamelen,
				&osta_charspec);
			len = UDF_PATH_COMP_SIZE + pathcomp.l_ci;
		}

		if (pathlen + len >= UDF_SYMLINKBUFLEN) {
			error = ENAMETOOLONG;
			break;
		}

		memcpy(pathpos, &pathcomp, len);
		pathpos += len;
		pathlen += len;
	}

	if (error) {
		/* aparently too big */
		free(pathbuf);
		return error;
	}

	/* return status of symlink contents writeout */
	*pathbufp = (uint8_t *) pathbuf;
	*pathlenp = pathlen;

	return 0;

}
#undef UDF_SYMLINKBUFLEN


/*
 * XXX note the different semantics from udfclient: for FIDs it still rounds
 * up to sectors. Use udf_fidsize() for a correct length.
 */
uint32_t
udf_tagsize(union dscrptr *dscr, uint32_t lb_size)
{
	uint32_t size, tag_id, num_lb, elmsz;

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

	if ((size == 0) || (lb_size == 0))
		return 0;

	if (lb_size == 1)
		return size;

	/* round up in sectors */
	num_lb = (size + lb_size -1) / lb_size;
	return num_lb * lb_size;
}


int
udf_fidsize(struct fileid_desc *fid)
{
	uint32_t size;

	if (udf_rw16(fid->tag.id) != TAGID_FID)
		errx(1, "internal error, bad tag in %s", __func__);

	size = UDF_FID_SIZE + fid->l_fi + udf_rw16(fid->l_iu);
	size = (size + 3) & ~3;

	return size;
}


int
udf_create_parentfid(struct fileid_desc *fid, struct long_ad *parent)
{
	/* the size of an empty FID is 38 but needs to be a multiple of 4 */
	int fidsize = 40;

	udf_inittag(&fid->tag, TAGID_FID, udf_rw32(parent->loc.lb_num));
	fid->file_version_num = udf_rw16(1);	/* UDF 2.3.4.1 */
	fid->file_char = UDF_FILE_CHAR_DIR | UDF_FILE_CHAR_PAR;
	fid->icb = *parent;
	fid->icb.longad_uniqueid = parent->longad_uniqueid;
	fid->tag.desc_crc_len = udf_rw16(fidsize - UDF_DESC_TAG_LENGTH);

	/* we have to do the fid here explicitly for simplicity */
	udf_validate_tag_and_crc_sums((union dscrptr *) fid);

	return fidsize;
}


void
udf_create_fid(uint32_t diroff, struct fileid_desc *fid, char *name,
	int file_char, struct long_ad *ref)
{
	struct charspec osta_charspec;
	uint32_t endfid;
	uint32_t fidsize, lb_rest;

	memset(fid, 0, sizeof(*fid));
	udf_inittag(&fid->tag, TAGID_FID, udf_rw32(ref->loc.lb_num));
	fid->file_version_num = udf_rw16(1);	/* UDF 2.3.4.1 */
	fid->file_char = file_char;
	fid->l_iu = udf_rw16(0);
	fid->icb = *ref;
	fid->icb.longad_uniqueid = ref->longad_uniqueid;

	udf_osta_charset(&osta_charspec);
	unix_to_udf_name((char *) fid->data, &fid->l_fi, name, strlen(name),
			&osta_charspec);

	/*
	 * OK, tricky part: we need to pad so the next descriptor header won't
	 * cross the sector boundary
	 */
	endfid = diroff + udf_fidsize(fid);
	lb_rest = context.sector_size - (endfid % context.sector_size);
	if (lb_rest < sizeof(struct desc_tag)) {
		/* add at least 32 */
		fid->l_iu = udf_rw16(32);
		udf_set_regid((struct regid *) fid->data, context.impl_name);
		udf_add_impl_regid((struct regid *) fid->data);

		unix_to_udf_name((char *) fid->data + udf_rw16(fid->l_iu),
			&fid->l_fi, name, strlen(name), &osta_charspec);
	}

	fidsize = udf_fidsize(fid);
	fid->tag.desc_crc_len = udf_rw16(fidsize - UDF_DESC_TAG_LENGTH);

	/* make sure the header sums stays correct */
	udf_validate_tag_and_crc_sums((union dscrptr *)fid);
}


static void
udf_append_parentfid(union dscrptr *dscr, struct long_ad *parent_icb)
{
	struct file_entry      *fe;
	struct extfile_entry   *efe;
	struct fileid_desc     *fid;
	uint32_t l_ea;
	uint32_t fidsize, crclen;
	uint8_t *bpos, *data;

	fe = NULL;
	efe = NULL;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe    = &dscr->fe;
		data  = fe->data;
		l_ea  = udf_rw32(fe->l_ea);
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe   = &dscr->efe;
		data  = efe->data;
		l_ea  = udf_rw32(efe->l_ea);
	} else {
		errx(1, "internal error, bad tag in %s", __func__);
	}

	/* create '..' */
	bpos = data + l_ea;
	fid  = (struct fileid_desc *) bpos;
	fidsize = udf_create_parentfid(fid, parent_icb);

	/* record fidlength information */
	if (fe) {
		fe->inf_len     = udf_rw64(fidsize);
		fe->l_ad        = udf_rw32(fidsize);
		fe->logblks_rec = udf_rw64(0);		/* intern */
		crclen  = sizeof(struct file_entry);
	} else {
		efe->inf_len     = udf_rw64(fidsize);
		efe->obj_size    = udf_rw64(fidsize);
		efe->l_ad        = udf_rw32(fidsize);
		efe->logblks_rec = udf_rw64(0);		/* intern */
		crclen  = sizeof(struct extfile_entry);
	}
	crclen -= 1 + UDF_DESC_TAG_LENGTH;
	crclen += l_ea + fidsize;
	dscr->tag.desc_crc_len = udf_rw16(crclen);

	/* make sure the header sums stays correct */
	udf_validate_tag_and_crc_sums(dscr);
}

/* --------------------------------------------------------------------- */

/*
 * Extended attribute support. UDF knows of 3 places for extended attributes:
 *
 * (a) inside the file's (e)fe in the length of the extended attribute area
 * before the allocation descriptors/filedata
 *
 * (b) in a file referenced by (e)fe->ext_attr_icb and
 *
 * (c) in the e(fe)'s associated stream directory that can hold various
 * sub-files. In the stream directory a few fixed named subfiles are reserved
 * for NT/Unix ACL's and OS/2 attributes.
 *
 * NOTE: Extended attributes are read randomly but always written
 * *atomically*. For ACL's this interface is probably different but not known
 * to me yet.
 *
 * Order of extended attributes in a space:
 *   ECMA 167 EAs
 *   Non block aligned Implementation Use EAs
 *   Block aligned Implementation Use EAs
 *   Application Use EAs
 */

int
udf_impl_extattr_check(struct impl_extattr_entry *implext)
{
	uint16_t   *spos;

	if (strncmp((char *) implext->imp_id.id, "*UDF", 4) == 0) {
		/* checksum valid? */
		spos = (uint16_t *) implext->data;
		if (udf_rw16(*spos) != udf_ea_cksum((uint8_t *) implext))
			return EINVAL;
	}
	return 0;
}

void
udf_calc_impl_extattr_checksum(struct impl_extattr_entry *implext)
{
	uint16_t   *spos;

	if (strncmp((char *) implext->imp_id.id, "*UDF", 4) == 0) {
		/* set checksum */
		spos = (uint16_t *) implext->data;
		*spos = udf_rw16(udf_ea_cksum((uint8_t *) implext));
	}
}


int
udf_extattr_search_intern(union dscrptr *dscr,
	uint32_t sattr, char const *sattrname,
	uint32_t *offsetp, uint32_t *lengthp)
{
	struct extattrhdr_desc    *eahdr;
	struct extattr_entry      *attrhdr;
	struct impl_extattr_entry *implext;
	uint32_t    offset, a_l, sector_size;
	uint32_t    l_ea;
	uint8_t    *pos;
	int         tag_id, error;

	sector_size = context.sector_size;

	/* get information from fe/efe */
	tag_id = udf_rw16(dscr->tag.id);
	if (tag_id == TAGID_FENTRY) {
		l_ea  = udf_rw32(dscr->fe.l_ea);
		eahdr = (struct extattrhdr_desc *) dscr->fe.data;
	} else {
		assert(tag_id == TAGID_EXTFENTRY);
		l_ea  = udf_rw32(dscr->efe.l_ea);
		eahdr = (struct extattrhdr_desc *) dscr->efe.data;
	}

	/* something recorded here? */
	if (l_ea == 0)
		return ENOENT;

	/* check extended attribute tag; what to do if it fails? */
	error = udf_check_tag(eahdr);
	if (error)
		return EINVAL;
	if (udf_rw16(eahdr->tag.id) != TAGID_EXTATTR_HDR)
		return EINVAL;
	error = udf_check_tag_payload(eahdr, sizeof(struct extattrhdr_desc));
	if (error)
		return EINVAL;

	/* looking for Ecma-167 attributes? */
	offset = sizeof(struct extattrhdr_desc);

	/* looking for either implementation use or application use */
	if (sattr == 2048) {				/* [4/48.10.8] */
		offset = udf_rw32(eahdr->impl_attr_loc);
		if (offset == UDF_IMPL_ATTR_LOC_NOT_PRESENT)
			return ENOENT;
	}
	if (sattr == 65536) {				/* [4/48.10.9] */
		offset = udf_rw32(eahdr->appl_attr_loc);
		if (offset == UDF_APPL_ATTR_LOC_NOT_PRESENT)
			return ENOENT;
	}

	/* paranoia check offset and l_ea */
	if (l_ea + offset >= sector_size - sizeof(struct extattr_entry))
		return EINVAL;

	/* find our extended attribute  */
	l_ea -= offset;
	pos = (uint8_t *) eahdr + offset;

	while (l_ea >= sizeof(struct extattr_entry)) {
		attrhdr = (struct extattr_entry *) pos;
		implext = (struct impl_extattr_entry *) pos;

		/* get complete attribute length and check for roque values */
		a_l = udf_rw32(attrhdr->a_l);
		if ((a_l == 0) || (a_l > l_ea))
			return EINVAL;

		if (udf_rw32(attrhdr->type) != sattr)
			goto next_attribute;

		/* we might have found it! */
		if (udf_rw32(attrhdr->type) < 2048) {	/* Ecma-167 attribute */
			*offsetp = offset;
			*lengthp = a_l;
			return 0;		/* success */
		}

		/*
		 * Implementation use and application use extended attributes
		 * have a name to identify. They share the same structure only
		 * UDF implementation use extended attributes have a checksum
		 * we need to check
		 */

		if (strcmp((char *) implext->imp_id.id, sattrname) == 0) {
			/* we have found our appl/implementation attribute */
			*offsetp = offset;
			*lengthp = a_l;
			return 0;		/* success */
		}

next_attribute:
		/* next attribute */
		pos    += a_l;
		l_ea   -= a_l;
		offset += a_l;
	}
	/* not found */
	return ENOENT;
}


static void
udf_extattr_insert_internal(union dscrptr *dscr, struct extattr_entry *extattr)
{
	struct file_entry      *fe;
	struct extfile_entry   *efe;
	struct extattrhdr_desc *extattrhdr;
	struct impl_extattr_entry *implext;
	uint32_t impl_attr_loc, appl_attr_loc, l_ea, l_ad, a_l;
	uint16_t *spos;
	uint8_t *bpos, *data;
	void *l_eap;

	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe    = &dscr->fe;
		data  = fe->data;
		l_eap = &fe->l_ea;
		l_ad  = udf_rw32(fe->l_ad);
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe   = &dscr->efe;
		data  = efe->data;
		l_eap = &efe->l_ea;
		l_ad  = udf_rw32(efe->l_ad);
	} else {
		errx(1, "internal error, bad tag in %s", __func__);
	}

	/* should have a header! */
	extattrhdr = (struct extattrhdr_desc *) data;
	memcpy(&l_ea, l_eap, sizeof(l_ea));
	l_ea = udf_rw32(l_ea);
	if (l_ea == 0) {
		uint32_t exthdr_len;
		assert(l_ad == 0);
		/* create empty extended attribute header */
		l_ea = sizeof(struct extattrhdr_desc);
		exthdr_len = udf_rw32(l_ea);

		udf_inittag(&extattrhdr->tag, TAGID_EXTATTR_HDR, /* loc */ 0);
		extattrhdr->impl_attr_loc = exthdr_len;
		extattrhdr->appl_attr_loc = exthdr_len;
		extattrhdr->tag.desc_crc_len = udf_rw16(8);

		/* record extended attribute header length */
		memcpy(l_eap, &exthdr_len, sizeof(exthdr_len));
	}

	/* extract locations */
	impl_attr_loc = udf_rw32(extattrhdr->impl_attr_loc);
	appl_attr_loc = udf_rw32(extattrhdr->appl_attr_loc);
	if (impl_attr_loc == UDF_IMPL_ATTR_LOC_NOT_PRESENT)
		impl_attr_loc = l_ea;
	if (appl_attr_loc == UDF_IMPL_ATTR_LOC_NOT_PRESENT)
		appl_attr_loc = l_ea;

	/* Ecma 167 EAs */
	if (udf_rw32(extattr->type) < 2048) {
		assert(impl_attr_loc == l_ea);
		assert(appl_attr_loc == l_ea);
	}

	/* implementation use extended attributes */
	if (udf_rw32(extattr->type) == 2048) {
		assert(appl_attr_loc == l_ea);

		/* calculate and write extended attribute header checksum */
		implext = (struct impl_extattr_entry *) extattr;
		assert(udf_rw32(implext->iu_l) == 4);	/* [UDF 3.3.4.5] */
		spos = (uint16_t *) implext->data;
		*spos = udf_rw16(udf_ea_cksum((uint8_t *) implext));
	}

	/* application use extended attributes */
	assert(udf_rw32(extattr->type) != 65536);
	assert(appl_attr_loc == l_ea);

	/* append the attribute at the end of the current space */
	bpos = data + l_ea;
	a_l  = udf_rw32(extattr->a_l);

	/* update impl. attribute locations */
	if (udf_rw32(extattr->type) < 2048) {
		impl_attr_loc = l_ea + a_l;
		appl_attr_loc = l_ea + a_l;
	}
	if (udf_rw32(extattr->type) == 2048) {
		appl_attr_loc = l_ea + a_l;
	}

	/* copy and advance */
	memcpy(bpos, extattr, a_l);
	l_ea += a_l;
	l_ea = udf_rw32(l_ea);
	memcpy(l_eap, &l_ea, sizeof(l_ea));

	/* do the `dance` again backwards */
	if (context.dscrver != 2) {
		if (impl_attr_loc == l_ea)
			impl_attr_loc = UDF_IMPL_ATTR_LOC_NOT_PRESENT;
		if (appl_attr_loc == l_ea)
			appl_attr_loc = UDF_APPL_ATTR_LOC_NOT_PRESENT;
	}

	/* store offsets */
	extattrhdr->impl_attr_loc = udf_rw32(impl_attr_loc);
	extattrhdr->appl_attr_loc = udf_rw32(appl_attr_loc);

	/* make sure the header sums stays correct */
	udf_validate_tag_and_crc_sums((union dscrptr *) extattrhdr);
}

/* --------------------------------------------------------------------- */

int
udf_create_new_fe(struct file_entry **fep, int file_type, struct stat *st)
{
	struct file_entry      *fe;
	struct icb_tag         *icb;
	struct timestamp        birthtime;
	struct filetimes_extattr_entry *ft_extattr;
	uint32_t crclen;	/* XXX: should be 16; need to detect overflow */
	uint16_t icbflags;

	*fep = NULL;
	fe = calloc(1, context.sector_size);
	if (fe == NULL)
		return ENOMEM;

	udf_inittag(&fe->tag, TAGID_FENTRY, /* loc */ 0);
	icb = &fe->icbtag;

	/*
	 * Always use strategy type 4 unless on WORM wich we don't support
	 * (yet). Fill in defaults and set for internal allocation of data.
	 */
	icb->strat_type      = udf_rw16(4);
	icb->max_num_entries = udf_rw16(1);
	icb->file_type       = file_type;	/* 8 bit */
	icb->flags           = udf_rw16(UDF_ICB_INTERN_ALLOC);

	fe->perm     = udf_rw32(0x7fff);	/* all is allowed   */
	fe->link_cnt = udf_rw16(0);		/* explicit setting */

	fe->ckpoint  = udf_rw32(1);		/* user supplied file version */

	udf_set_timestamp_now(&birthtime);
	udf_set_timestamp_now(&fe->atime);
	udf_set_timestamp_now(&fe->attrtime);
	udf_set_timestamp_now(&fe->mtime);

	/* set attributes */
	if (st) {
#if !HAVE_NBTOOL_CONFIG_H
		udf_set_timestamp(&birthtime,    st->st_birthtime);
#else
		udf_set_timestamp(&birthtime,    0);
#endif
		udf_set_timestamp(&fe->atime,    st->st_atime);
		udf_set_timestamp(&fe->attrtime, st->st_ctime);
		udf_set_timestamp(&fe->mtime,    st->st_mtime);
		fe->uid  = udf_rw32(st->st_uid);
		fe->gid  = udf_rw32(st->st_gid);

		fe->perm = udf_rw32(unix_mode_to_udf_perm(st->st_mode));

		icbflags = udf_rw16(fe->icbtag.flags);
		icbflags &= ~UDF_ICB_TAG_FLAGS_SETUID;
		icbflags &= ~UDF_ICB_TAG_FLAGS_SETGID;
		icbflags &= ~UDF_ICB_TAG_FLAGS_STICKY;
		if (st->st_mode & S_ISUID)
			icbflags |= UDF_ICB_TAG_FLAGS_SETUID;
		if (st->st_mode & S_ISGID)
			icbflags |= UDF_ICB_TAG_FLAGS_SETGID;
		if (st->st_mode & S_ISVTX)
			icbflags |= UDF_ICB_TAG_FLAGS_STICKY;
		fe->icbtag.flags  = udf_rw16(icbflags);
	}

	udf_set_regid(&fe->imp_id, context.impl_name);
	udf_add_impl_regid(&fe->imp_id);
	fe->unique_id = udf_rw64(context.unique_id);
	udf_advance_uniqueid();

	fe->l_ea = udf_rw32(0);

	/* create extended attribute to record our creation time */
	ft_extattr = calloc(1, UDF_FILETIMES_ATTR_SIZE(1));
	ft_extattr->hdr.type = udf_rw32(UDF_FILETIMES_ATTR_NO);
	ft_extattr->hdr.subtype = 1;	/* [4/48.10.5] */
	ft_extattr->hdr.a_l = udf_rw32(UDF_FILETIMES_ATTR_SIZE(1));
	ft_extattr->d_l     = udf_rw32(UDF_TIMESTAMP_SIZE); /* one item */
	ft_extattr->existence = UDF_FILETIMES_FILE_CREATION;
	ft_extattr->times[0]  = birthtime;

	udf_extattr_insert_internal((union dscrptr *) fe,
		(struct extattr_entry *) ft_extattr);
	free(ft_extattr);

	/* record fidlength information */
	fe->inf_len = udf_rw64(0);
	fe->l_ad    = udf_rw32(0);
	fe->logblks_rec = udf_rw64(0);		/* intern */

	crclen  = sizeof(struct file_entry) - 1 - UDF_DESC_TAG_LENGTH;
	crclen += udf_rw32(fe->l_ea);

	/* make sure the header sums stays correct */
	fe->tag.desc_crc_len = udf_rw16(crclen);
	udf_validate_tag_and_crc_sums((union dscrptr *) fe);

	*fep = fe;
	return 0;
}


int
udf_create_new_efe(struct extfile_entry **efep, int file_type, struct stat *st)
{
	struct extfile_entry *efe;
	struct icb_tag       *icb;
	uint32_t crclen;	/* XXX: should be 16; need to detect overflow */
	uint16_t icbflags;

	*efep = NULL;
	efe = calloc(1, context.sector_size);
	if (efe == NULL)
		return ENOMEM;

	udf_inittag(&efe->tag, TAGID_EXTFENTRY, /* loc */ 0);
	icb = &efe->icbtag;

	/*
	 * Always use strategy type 4 unless on WORM wich we don't support
	 * (yet). Fill in defaults and set for internal allocation of data.
	 */
	icb->strat_type      = udf_rw16(4);
	icb->max_num_entries = udf_rw16(1);
	icb->file_type       = file_type;	/* 8 bit */
	icb->flags = udf_rw16(UDF_ICB_INTERN_ALLOC);

	efe->perm     = udf_rw32(0x7fff);	/* all is allowed   */
	efe->link_cnt = udf_rw16(0);		/* explicit setting */

	efe->ckpoint  = udf_rw32(1);		/* user supplied file version */

	udf_set_timestamp_now(&efe->ctime);
	udf_set_timestamp_now(&efe->atime);
	udf_set_timestamp_now(&efe->attrtime);
	udf_set_timestamp_now(&efe->mtime);

	/* set attributes */
	if (st) {
#if !HAVE_NBTOOL_CONFIG_H
		udf_set_timestamp(&efe->ctime,    st->st_birthtime);
#else
		udf_set_timestamp(&efe->ctime,    0);
#endif
		udf_set_timestamp(&efe->atime,    st->st_atime);
		udf_set_timestamp(&efe->attrtime, st->st_ctime);
		udf_set_timestamp(&efe->mtime,    st->st_mtime);
		efe->uid = udf_rw32(st->st_uid);
		efe->gid = udf_rw32(st->st_gid);

		efe->perm = udf_rw32(unix_mode_to_udf_perm(st->st_mode));

		icbflags = udf_rw16(efe->icbtag.flags);
		icbflags &= ~UDF_ICB_TAG_FLAGS_SETUID;
		icbflags &= ~UDF_ICB_TAG_FLAGS_SETGID;
		icbflags &= ~UDF_ICB_TAG_FLAGS_STICKY;
		if (st->st_mode & S_ISUID)
			icbflags |= UDF_ICB_TAG_FLAGS_SETUID;
		if (st->st_mode & S_ISGID)
			icbflags |= UDF_ICB_TAG_FLAGS_SETGID;
		if (st->st_mode & S_ISVTX)
			icbflags |= UDF_ICB_TAG_FLAGS_STICKY;
		efe->icbtag.flags = udf_rw16(icbflags);
	}

	udf_set_regid(&efe->imp_id, context.impl_name);
	udf_add_impl_regid(&efe->imp_id);

	efe->unique_id = udf_rw64(context.unique_id);
	udf_advance_uniqueid();

	/* record fidlength information */
	efe->inf_len  = udf_rw64(0);
	efe->obj_size = udf_rw64(0);
	efe->l_ad     = udf_rw32(0);
	efe->logblks_rec = udf_rw64(0);

	crclen  = sizeof(struct extfile_entry) - 1 - UDF_DESC_TAG_LENGTH;

	/* make sure the header sums stays correct */
	efe->tag.desc_crc_len = udf_rw16(crclen);
	udf_validate_tag_and_crc_sums((union dscrptr *) efe);

	*efep = efe;
	return 0;
}

/* --------------------------------------------------------------------- */

/* for METADATA file appending only */
static void
udf_append_meta_mapping_part_to_efe(struct extfile_entry *efe,
		struct short_ad *mapping)
{
	struct icb_tag *icb;
	uint64_t inf_len, obj_size, logblks_rec;
	uint32_t l_ad, l_ea;
	uint16_t crclen;
	uintptr_t bpos;

	inf_len     = udf_rw64(efe->inf_len);
	obj_size    = udf_rw64(efe->obj_size);
	logblks_rec = udf_rw64(efe->logblks_rec);
	l_ad   = udf_rw32(efe->l_ad);
	l_ea   = udf_rw32(efe->l_ea);
	crclen = udf_rw16(efe->tag.desc_crc_len);
	icb    = &efe->icbtag;

	/* set our allocation to shorts if not already done */
	icb->flags = udf_rw16(UDF_ICB_SHORT_ALLOC);

	/* append short_ad */
	bpos = (uintptr_t)efe->data + l_ea + l_ad;
	memcpy((void *)bpos, mapping, sizeof(struct short_ad));

	l_ad   += sizeof(struct short_ad);
	crclen += sizeof(struct short_ad);
	inf_len  += UDF_EXT_LEN(udf_rw32(mapping->len));
	obj_size += UDF_EXT_LEN(udf_rw32(mapping->len));
	logblks_rec = UDF_ROUNDUP(inf_len, context.sector_size) /
				context.sector_size;

	efe->l_ad = udf_rw32(l_ad);
	efe->inf_len     = udf_rw64(inf_len);
	efe->obj_size    = udf_rw64(obj_size);
	efe->logblks_rec = udf_rw64(logblks_rec);
	efe->tag.desc_crc_len = udf_rw16(crclen);
}


/* for METADATA file appending only */
static void
udf_append_meta_mapping_to_efe(struct extfile_entry *efe,
	uint16_t partnr, uint32_t lb_num,
	uint64_t len)
{
	struct short_ad mapping;
	uint64_t max_len, part_len;

	/* calculate max length meta allocation sizes */
	max_len = UDF_EXT_MAXLEN / context.sector_size; /* in sectors */
	max_len = (max_len / layout.meta_blockingnr) * layout.meta_blockingnr;
	max_len = max_len * context.sector_size;

	memset(&mapping, 0, sizeof(mapping));
	while (len) {
		part_len = MIN(len, max_len);
		mapping.lb_num   = udf_rw32(lb_num);
		mapping.len      = udf_rw32(part_len);

		udf_append_meta_mapping_part_to_efe(efe, &mapping);

		lb_num += part_len / context.sector_size;
		len    -= part_len;
	}
}


int
udf_create_meta_files(void)
{
	struct extfile_entry *efe;
	struct long_ad meta_icb;
	uint64_t bytes;
	uint32_t sector_size;
	int filetype, error;

	sector_size = context.sector_size;

	memset(&meta_icb, 0, sizeof(meta_icb));
	meta_icb.len          = udf_rw32(sector_size);
	meta_icb.loc.part_num = udf_rw16(context.data_part);

	/* create metadata file */
	meta_icb.loc.lb_num   = udf_rw32(layout.meta_file);
	filetype = UDF_ICB_FILETYPE_META_MAIN;
	error = udf_create_new_efe(&efe, filetype, NULL);
	if (error)
		return error;
	context.meta_file = efe;
	context.meta_file->unique_id   = udf_rw64(0);

	/* create metadata mirror file */
	meta_icb.loc.lb_num   = udf_rw32(layout.meta_mirror);
	filetype = UDF_ICB_FILETYPE_META_MIRROR;
	error = udf_create_new_efe(&efe, filetype, NULL);
	if (error)
		return error;
	context.meta_mirror = efe;
	context.meta_mirror->unique_id = udf_rw64(0);

	if (!(context.format_flags & FORMAT_READONLY)) {
		/* create metadata bitmap file */
		meta_icb.loc.lb_num   = udf_rw32(layout.meta_bitmap);
		filetype = UDF_ICB_FILETYPE_META_BITMAP;
		error = udf_create_new_efe(&efe, filetype, NULL);
		if (error)
			return error;
		context.meta_bitmap = efe;
		context.meta_bitmap->unique_id = udf_rw64(0);
	}

	/* restart unique id */
	context.unique_id = 0x10;

	/* XXX no support for metadata mirroring yet */
	/* insert extents */
	efe = context.meta_file;
	udf_append_meta_mapping_to_efe(efe, context.data_part,
		layout.meta_part_start_lba,
		(uint64_t) layout.meta_part_size_lba * sector_size);

	efe = context.meta_mirror;
	udf_append_meta_mapping_to_efe(efe, context.data_part,
		layout.meta_part_start_lba,
		(uint64_t) layout.meta_part_size_lba * sector_size);

	if (context.meta_bitmap) {
		efe = context.meta_bitmap;
		bytes = udf_space_bitmap_len(layout.meta_part_size_lba);
		udf_append_meta_mapping_to_efe(efe, context.data_part,
			layout.meta_bitmap_space, bytes);
	}

	return 0;
}


/* --------------------------------------------------------------------- */

int
udf_create_new_rootdir(union dscrptr **dscr)
{
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct long_ad root_icb;
	int filetype, error;

	memset(&root_icb, 0, sizeof(root_icb));
	root_icb.len          = udf_rw32(context.sector_size);
	root_icb.loc.lb_num   = udf_rw32(layout.rootdir);
	root_icb.loc.part_num = udf_rw16(context.metadata_part);

	filetype = UDF_ICB_FILETYPE_DIRECTORY;
	if (context.dscrver == 2) {
		error = udf_create_new_fe(&fe, filetype, NULL);
		*dscr = (union dscrptr *) fe;
	} else {
		error = udf_create_new_efe(&efe, filetype, NULL);
		*dscr = (union dscrptr *) efe;
	}
	if (error)
		return error;

	/* append '..' */
	udf_append_parentfid(*dscr, &root_icb);

	/* rootdir has explicit only one link on creation; '..' is no link */
	if (context.dscrver == 2) {
		fe->link_cnt  = udf_rw16(1);
	} else {
		efe->link_cnt = udf_rw16(1);
	}

	context.num_directories++;
	assert(context.num_directories == 1);

	return 0;
}


void
udf_prepend_VAT_file(void)
{
	/* old style VAT has no prepend */
	if (context.dscrver == 2) {
		context.vat_start = 0;
		context.vat_size  = 0;
		return;
	}

	context.vat_start = offsetof(struct udf_vat, data);
	context.vat_size  = offsetof(struct udf_vat, data);
}


void
udf_vat_update(uint32_t virt, uint32_t phys)
{
	uint32_t *vatpos;
	uint32_t new_size;

	if (context.vtop_tp[context.metadata_part] != UDF_VTOP_TYPE_VIRT)
		return;

	new_size = MAX(context.vat_size,
		(context.vat_start + (virt+1)*sizeof(uint32_t)));

	if (new_size > context.vat_allocated) {
		context.vat_allocated =
			UDF_ROUNDUP(new_size, context.sector_size);
		context.vat_contents = realloc(context.vat_contents,
			context.vat_allocated);
		assert(context.vat_contents);
		/* XXX could also report error */
	}
	vatpos  = (uint32_t *) (context.vat_contents + context.vat_start);
	vatpos[virt] = udf_rw32(phys);

	context.vat_size = MAX(context.vat_size,
		(context.vat_start + (virt+1)*sizeof(uint32_t)));
}


int
udf_append_VAT_file(void)
{
	struct udf_oldvat_tail *oldvat_tail;
	struct udf_vat *vathdr;
	int32_t len_diff;

	/* new style VAT has VAT LVInt analog in front */
	if (context.dscrver == 3) {
		/* set up VATv2 descriptor */
		vathdr = (struct udf_vat *) context.vat_contents;
		vathdr->header_len      = udf_rw16(sizeof(struct udf_vat) - 1);
		vathdr->impl_use_len    = udf_rw16(0);
		memcpy(vathdr->logvol_id, context.logical_vol->logvol_id, 128);
		vathdr->prev_vat        = udf_rw32(UDF_NO_PREV_VAT);
		vathdr->num_files       = udf_rw32(context.num_files);
		vathdr->num_directories = udf_rw32(context.num_directories);

		vathdr->min_udf_readver  = udf_rw16(context.min_udf);
		vathdr->min_udf_writever = udf_rw16(context.min_udf);
		vathdr->max_udf_writever = udf_rw16(context.max_udf);

		return 0;
	}

	/* old style VAT has identifier appended */

	/* append "*UDF Virtual Alloc Tbl" id and prev. VAT location */
	len_diff = context.vat_allocated - context.vat_size;
	assert(len_diff >= 0);
	if (len_diff < (int32_t) sizeof(struct udf_oldvat_tail)) {
		context.vat_allocated += context.sector_size;
		context.vat_contents = realloc(context.vat_contents,
			context.vat_allocated);
		assert(context.vat_contents);
		/* XXX could also report error */
	}

	oldvat_tail = (struct udf_oldvat_tail *) (context.vat_contents +
			context.vat_size);

	udf_set_regid(&oldvat_tail->id, "*UDF Virtual Alloc Tbl");
	udf_add_udf_regid(&oldvat_tail->id);
	oldvat_tail->prev_vat = udf_rw32(UDF_NO_PREV_VAT);

	context.vat_size += sizeof(struct udf_oldvat_tail);

	return 0;
}


int
udf_create_VAT(union dscrptr **vat_dscr, struct long_ad *vatdata_loc)
{
	struct impl_extattr_entry *implext;
	struct vatlvext_extattr_entry *vatlvext;
	struct long_ad *allocpos;
	uint8_t *bpos, *extattr;
	uint32_t ea_len, inf_len, vat_len, blks;
	int filetype;
	int error;

	assert((layout.rootdir < 2) && (layout.fsd < 2));

	if (context.dscrver == 2) {
		struct file_entry *fe;

		/* old style VAT */
		filetype = UDF_ICB_FILETYPE_UNKNOWN;
		error = udf_create_new_fe(&fe, filetype, NULL);
		if (error)
			return error;

		/* append VAT LVExtension attribute */
		ea_len = sizeof(struct impl_extattr_entry) - 2 + 4 +
			 sizeof(struct vatlvext_extattr_entry);

		extattr = calloc(1, ea_len);

		implext  = (struct impl_extattr_entry *) extattr;
		implext->hdr.type = udf_rw32(2048);	/* [4/48.10.8] */
		implext->hdr.subtype = 1;		/* [4/48.10.8.2] */
		implext->hdr.a_l = udf_rw32(ea_len);	/* VAT LVext EA size */
		/* use 4 bytes of imp use for UDF checksum [UDF 3.3.4.5] */
		implext->iu_l = udf_rw32(4);
		udf_set_regid(&implext->imp_id, "*UDF VAT LVExtension");
		udf_add_udf_regid(&implext->imp_id);

		/* VAT LVExtension data follows UDF IU space */
		bpos = ((uint8_t *) implext->data) + 4;
		vatlvext = (struct vatlvext_extattr_entry *) bpos;

		vatlvext->unique_id_chk = fe->unique_id;
		vatlvext->num_files = udf_rw32(context.num_files);
		vatlvext->num_directories = udf_rw32(context.num_directories);
		memcpy(vatlvext->logvol_id, context.logical_vol->logvol_id,128);

		udf_extattr_insert_internal((union dscrptr *) fe,
			(struct extattr_entry *) extattr);

		free(extattr);

		fe->icbtag.flags = udf_rw16(UDF_ICB_LONG_ALLOC);

		allocpos = (struct long_ad *) (fe->data + udf_rw32(fe->l_ea));
		*allocpos = *vatdata_loc;

		/* set length */
		inf_len       = context.vat_size;
		fe->inf_len   = udf_rw64(inf_len);
		allocpos->len = udf_rw32(inf_len);
		fe->l_ad      = udf_rw32(sizeof(struct long_ad));
		blks = UDF_ROUNDUP(inf_len, context.sector_size) /
			context.sector_size;
		fe->logblks_rec = udf_rw64(blks);

		/* update vat descriptor's CRC length */
		vat_len  = sizeof(struct file_entry) - 1 - UDF_DESC_TAG_LENGTH;
		vat_len += udf_rw32(fe->l_ad) + udf_rw32(fe->l_ea);
		fe->tag.desc_crc_len = udf_rw16(vat_len);

		*vat_dscr = (union dscrptr *) fe;
	} else {
		/* the choice is between an EFE or an FE as VAT */
#if 1
		struct extfile_entry *efe;

		/* new style VAT on FE */
		filetype = UDF_ICB_FILETYPE_VAT;
		error = udf_create_new_efe(&efe, filetype, NULL);
		if (error)
			return error;

		efe->icbtag.flags = udf_rw16(UDF_ICB_LONG_ALLOC);

		allocpos = (struct long_ad *) efe->data;
		*allocpos = *vatdata_loc;

		/* set length */
		inf_len = context.vat_size;
		efe->inf_len     = udf_rw64(inf_len);
		allocpos->len    = udf_rw32(inf_len);
		efe->obj_size    = udf_rw64(inf_len);
		efe->l_ad        = udf_rw32(sizeof(struct long_ad));
		blks = UDF_ROUNDUP(inf_len, context.sector_size) /
			context.sector_size;
		efe->logblks_rec = udf_rw64(blks);

		vat_len  = sizeof(struct extfile_entry)-1 - UDF_DESC_TAG_LENGTH;
		vat_len += udf_rw32(efe->l_ad);
		efe->tag.desc_crc_len = udf_rw16(vat_len);

		*vat_dscr = (union dscrptr *) efe;
#else
		struct file_entry *fe;
		uint32_t l_ea;

		/* new style VAT on EFE */
		filetype = UDF_ICB_FILETYPE_VAT;
		error = udf_create_new_fe(&fe, filetype, NULL);
		if (error)
			return error;

		fe->icbtag.flags = udf_rw16(UDF_ICB_LONG_ALLOC);

		l_ea = udf_rw32(fe->l_ea);
		allocpos  = (struct long_ad *) (fe->data + l_ea);
		*allocpos = *vatdata_loc;

		/* set length */
		inf_len         = context.vat_size;
		fe->inf_len     = udf_rw64(inf_len);
		allocpos->len   = udf_rw32(inf_len);
		fe->l_ad        = udf_rw32(sizeof(struct long_ad));
		blks = UDF_ROUNDUP(inf_len, context.sector_size) /
			context.sector_size;
		fe->logblks_rec = udf_rw64(blks);

		vat_len  = sizeof(struct file_entry)-1 - UDF_DESC_TAG_LENGTH;
		vat_len += udf_rw32(fe->l_ad) + udf_rw32(fe->l_ea);
		fe->tag.desc_crc_len = udf_rw16(vat_len);

		*vat_dscr = (union dscrptr *) fe;
#endif
	}

	return 0;
}


int
udf_writeout_VAT(void)
{
	union dscrptr *vat_dscr;
	struct long_ad vatdata;
	uint32_t loc, phys, ext, sects;
	int rel_block, rest_block, error;

	vat_dscr = NULL;
	/* update lvint to reflect the newest values (no writeout) */
	udf_update_lvintd(UDF_INTEGRITY_CLOSED);

	error = udf_append_VAT_file();
	if (error)
		return error;

	/* write out VAT data */
	sects = UDF_ROUNDUP(context.vat_size, context.sector_size) /
		context.sector_size;
	layout.vat = context.alloc_pos[context.data_part];
	udf_data_alloc(sects, &vatdata);
//printf("layout.vat %d\n", layout.vat + udf_rw32(context.partitions[context.data_part]->start_loc));

	loc = udf_rw32(vatdata.loc.lb_num);
	udf_translate_vtop(loc, context.data_part, &phys, &ext);

	error = udf_write_phys(context.vat_contents, phys, sects);
	if (error)
		return error;
	loc += sects;

	/* create new VAT descriptor */
	error = udf_create_VAT(&vat_dscr, &vatdata);
	if (error)
		return error;

//printf("VAT data at %d\n", vatdata.loc.lb_num);
//printf("VAT itself at %d\n", loc + udf_rw32(context.partitions[context.data_part]->start_loc));

	/* at least one */
	error = udf_write_dscr_virt(vat_dscr, loc, context.data_part, 1);
	loc++;

	error = udf_translate_vtop(loc, context.data_part, &phys, &ext);
	assert(!error);

	rel_block  = phys - (UDF_ROUNDDOWN(phys, layout.blockingnr) + wrtrack_skew);
	rest_block = layout.blockingnr - rel_block;

	for (int i = 0; i < rest_block; i++) {
		error = udf_write_dscr_virt(vat_dscr, loc, context.data_part, 1);
		loc++;
	}
	free(vat_dscr);

	return error;
}


/* --------------------------------------------------------------------- */
/*
 * mmc_discinfo and mmc_trackinfo readers modified from origional in udf main
 * code in sys/fs/udf/
 */

void
udf_dump_discinfo(struct mmc_discinfo *di)
{
#ifdef DEBUG
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
#endif
}


void
udf_synchronise_caches(void)
{
#if !HAVE_NBTOOL_CONFIG_H
	struct mmc_op mmc_op;

	bzero(&mmc_op, sizeof(struct mmc_op));
	mmc_op.operation = MMC_OP_SYNCHRONISECACHE;

	/* this device might not know this ioct, so just be ignorant */
	(void) ioctl(dev_fd, MMCOP, &mmc_op);
#endif
}


/*
 * General Idea:
 *
 * stat the dev_fd
 *
 * If a S_ISREG(), we emulate using the emul_* settings.
 *
 * If its a device :
 * 	try the MMCGETDISCINFO ioctl() and be done.
 *
 * If that fails, its a regular disc and set the type to disc media.
 *
 */


int
udf_update_discinfo(void)
{
	off_t size, last_sector, secsize;
	int error;

	memset(&mmc_discinfo, 0, sizeof(struct mmc_discinfo));

#if !HAVE_NBTOOL_CONFIG_H
	/* check if we're on a MMC capable device, i.e. CD/DVD */
	error = ioctl(dev_fd, MMCGETDISCINFO, &mmc_discinfo);
	if (error == 0) {
		if ((emul_mmc_profile != -1) &&
		   (emul_mmc_profile != mmc_discinfo.mmc_profile)) {
			errno = EINVAL;
			perror("media and specified disc type mismatch");
			return errno;
		}
		emul_size = 0;
		return 0;
	}
#endif

	if (S_ISREG(dev_fd_stat.st_mode)) {
		/* file support; we pick the minimum sector size allowed */
		if (emul_mmc_profile < 0)
			emul_mmc_profile = 0x01;
		if (emul_size == 0)
			emul_size = dev_fd_stat.st_size;
		size = emul_size;
		secsize = emul_sectorsize;
		last_sector = (size / secsize) - 1;
		if (ftruncate(dev_fd, size)) {
			perror("can't resize file");
			return EXIT_FAILURE;
		}
	} else {
#if !HAVE_NBTOOL_CONFIG_H
		struct disk_geom	geo;
		struct dkwedge_info	dkw;

		/* sanity */
		if (emul_mmc_profile <= 0)
			emul_mmc_profile = 0x01;
		if (emul_mmc_profile != 0x01) {
			warnx("format incompatible with disc partition");
			return EXIT_FAILURE;
		}

		/* get our disc info */
		error = getdiskinfo(dev_name, dev_fd, NULL, &geo, &dkw);
		if (error) {
			warn("retrieving disc info failed");
			return EXIT_FAILURE;
		}
		secsize = emul_sectorsize;
		last_sector = (dkw.dkw_size - 1) * geo.dg_secsize / secsize;
#else
		warnx("disk partitions only usable outside tools");
		return EIO;
#endif
	}

	/* commons */
	mmc_discinfo.mmc_profile	= emul_mmc_profile;
	mmc_discinfo.disc_state		= MMC_STATE_CLOSED;
	mmc_discinfo.last_session_state	= MMC_STATE_CLOSED;
	mmc_discinfo.bg_format_state	= MMC_BGFSTATE_COMPLETED;
	mmc_discinfo.link_block_penalty	= 0;

	mmc_discinfo.disc_flags = MMC_DFLAGS_UNRESTRICTED;

	mmc_discinfo.last_possible_lba = last_sector;
	mmc_discinfo.sector_size       = secsize;

	mmc_discinfo.num_sessions = 1;
	mmc_discinfo.num_tracks   = 1;

	mmc_discinfo.first_track  = 1;
	mmc_discinfo.first_track_last_session = mmc_discinfo.last_track_last_session = 1;

	mmc_discinfo.mmc_cur = MMC_CAP_RECORDABLE | MMC_CAP_ZEROLINKBLK;
	switch (emul_mmc_profile) {
	case 0x00:	/* unknown, treat as CDROM */
	case 0x08:	/* CDROM */
	case 0x10:	/* DVDROM */
	case 0x40:	/* BDROM */
		/* FALLTHROUGH */
	case 0x01:	/* disc */
		/* set up a disc info profile for partitions/files */
		mmc_discinfo.mmc_class	= MMC_CLASS_DISC;
		mmc_discinfo.mmc_cur    |= MMC_CAP_REWRITABLE | MMC_CAP_HW_DEFECTFREE;
		break;
	case 0x09:	/* CD-R */
		mmc_discinfo.mmc_class	= MMC_CLASS_CD;
		mmc_discinfo.mmc_cur    |= MMC_CAP_SEQUENTIAL;
		mmc_discinfo.disc_state  = MMC_STATE_EMPTY;
		break;
	case 0x0a:	/* CD-RW + CD-MRW (regretably) */
		mmc_discinfo.mmc_class	= MMC_CLASS_CD;
		mmc_discinfo.mmc_cur    |= MMC_CAP_REWRITABLE;
		break;
	case 0x13:	/* DVD-RW */
	case 0x1a:	/* DVD+RW */
		mmc_discinfo.mmc_class	= MMC_CLASS_DVD;
		mmc_discinfo.mmc_cur    |= MMC_CAP_REWRITABLE;
		break;
	case 0x11:	/* DVD-R */
	case 0x14:	/* DVD-RW sequential */
	case 0x1b:	/* DVD+R */
	case 0x2b:	/* DVD+R DL */
	case 0x51:	/* HD DVD-R */
		mmc_discinfo.mmc_class	= MMC_CLASS_DVD;
		mmc_discinfo.mmc_cur    |= MMC_CAP_SEQUENTIAL;
		mmc_discinfo.disc_state  = MMC_STATE_EMPTY;
		break;
	case 0x41:	/* BD-R */
		mmc_discinfo.mmc_class   = MMC_CLASS_BD;
		mmc_discinfo.mmc_cur    |= MMC_CAP_SEQUENTIAL | MMC_CAP_HW_DEFECTFREE;
		mmc_discinfo.disc_state  = MMC_STATE_EMPTY;
		break;
	case 0x43:	/* BD-RE */
		mmc_discinfo.mmc_class   = MMC_CLASS_BD;
		mmc_discinfo.mmc_cur    |= MMC_CAP_REWRITABLE | MMC_CAP_HW_DEFECTFREE;
		break;
	default:
		errno = EINVAL;
		perror("unknown or unimplemented device type");
		return errno;
	}
	mmc_discinfo.mmc_cap    = mmc_discinfo.mmc_cur;

	return 0;
}


int
udf_update_trackinfo(struct mmc_trackinfo *ti)
{
	int error, class;

#if !HAVE_NBTOOL_CONFIG_H
	class = mmc_discinfo.mmc_class;
	if (class != MMC_CLASS_DISC) {
		/* tracknr specified in struct ti */
		error = ioctl(dev_fd, MMCGETTRACKINFO, ti);
		if (!error)
			return 0;
	}
#endif

	/* discs partition support */
	if (ti->tracknr != 1)
		return EIO;

	/* create fake ti (TODO check for resized vnds) */
	ti->sessionnr  = 1;

	ti->track_mode = 0;	/* XXX */
	ti->data_mode  = 0;	/* XXX */
	ti->flags = MMC_TRACKINFO_LRA_VALID | MMC_TRACKINFO_NWA_VALID;

	ti->track_start    = 0;
	ti->packet_size    = emul_packetsize;

	/* TODO support for resizable vnd */
	ti->track_size    = mmc_discinfo.last_possible_lba;
	ti->next_writable = mmc_discinfo.last_possible_lba + 1; //0;
	ti->last_recorded = ti->next_writable;
	ti->free_blocks   = 0;

	return 0;
}


int
udf_opendisc(const char *device, int open_flags)
{
	/* set global variable to the passed name */
	dev_name = strdup(device);

	/* open device */
	if (open_flags & O_RDONLY) {
		dev_fd_rdonly = 1;
		if ((dev_fd = open(dev_name, O_RDONLY, 0)) == -1) {
			warn("device/image not found");
			return EXIT_FAILURE;
		}
	} else {
		dev_fd_rdonly = 0;
		if ((dev_fd = open(dev_name, O_RDWR, 0)) == -1) {
			/* check if we need to create a file */
			dev_fd = open(dev_name, O_RDONLY, 0);
			if (dev_fd > 0) {
				warn("device is there but can't be opened for "
					"read/write");
				return EXIT_FAILURE;
			}
			if ((open_flags & O_CREAT) == 0) {
				warnx("device/image not found");
				return EXIT_FAILURE;
			}
			/* need to create a file */
			dev_fd = open(dev_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
			if (dev_fd == -1) {
				warn("can't create image file");
				return EXIT_FAILURE;
			}
		}
	}

	/* stat the device/image */
	if (fstat(dev_fd, &dev_fd_stat) != 0) {
		warn("can't stat the disc image");
		return EXIT_FAILURE;
	}

	/* sanity check and resizing of file */
	if (S_ISREG(dev_fd_stat.st_mode)) {
		if (emul_size == 0)
			emul_size = dev_fd_stat.st_size;
		/* sanitise arguments */
		emul_sectorsize &= ~511;
		if (emul_size & (emul_sectorsize-1)) {
			warnx("size of file is not a multiple of sector size, "
				"shrinking");
			emul_size -= emul_size & (emul_sectorsize-1);
		}

		/* grow the image */
		if (ftruncate(dev_fd, emul_size)) {
			warn("can't resize file");
			return EXIT_FAILURE;
		}
		/* restat the device/image */
		if (fstat(dev_fd, &dev_fd_stat) != 0) {
			warn("can't re-stat the disc image");
			return EXIT_FAILURE;
		}
	} else {
		if (!S_ISCHR(dev_fd_stat.st_mode)) {
			warnx("%s is not a raw device", dev_name);
			return EXIT_FAILURE;
		}
	}

	/* just in case something went wrong, synchronise the drive's cache */
	udf_synchronise_caches();
	if (udf_update_discinfo()) {
		warnx("update discinfo failed");
		return EXIT_FAILURE;
	}

	/* honour minimum sector size of the device */
	if (mmc_discinfo.sector_size > context.sector_size)
		context.sector_size = mmc_discinfo.sector_size;

	if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL)
		udf_init_writequeue(UDF_WRITE_SEQUENTIAL);
	else {
		udf_init_writequeue(UDF_WRITE_PACKET);
	}
	return 0;
}


void
udf_closedisc(void)
{
	if (!write_queue_suspend) {
		udf_writeout_writequeue(true);
		assert(write_queuelen == 0);
	}

	udf_synchronise_caches();
	if (dev_fd)
		close(dev_fd);
}

/* --------------------------------------------------------------------- */

static int
udf_setup_writeparams(void)
{
#if !HAVE_NBTOOL_CONFIG_H
	struct mmc_writeparams mmc_writeparams;
	int error;

	if (mmc_discinfo.mmc_class == MMC_CLASS_DISC)
		return 0;

	if (S_ISREG(dev_fd_stat.st_mode))
		return 0;

	/*
	 * only CD burning normally needs setting up, but other disc types
	 * might need other settings to be made. The MMC framework will set up
	 * the necessary recording parameters according to the disc
	 * characteristics read in. Modifications can be made in the discinfo
	 * structure passed to change the nature of the disc.
	 */
	memset(&mmc_writeparams, 0, sizeof(struct mmc_writeparams));
	mmc_writeparams.mmc_class  = mmc_discinfo.mmc_class;
	mmc_writeparams.mmc_cur    = mmc_discinfo.mmc_cur;

	/*
	 * UDF dictates first track to determine track mode for the whole
	 * disc. [UDF 1.50/6.10.1.1, UDF 1.50/6.10.2.1]
	 * To prevent problems with a `reserved' track in front we start with
	 * the 2nd track and if that is not valid, go for the 1st.
	 */
	mmc_writeparams.tracknr = 2;
	mmc_writeparams.data_mode  = MMC_DATAMODE_DEFAULT;	/* XA disc */
	mmc_writeparams.track_mode = MMC_TRACKMODE_DEFAULT;	/* data */

	error = ioctl(dev_fd, MMCSETUPWRITEPARAMS, &mmc_writeparams);
	if (error) {
		mmc_writeparams.tracknr = 1;
		error = ioctl(dev_fd, MMCSETUPWRITEPARAMS, &mmc_writeparams);
	}
	return error;
#else
	return 0;
#endif
}


/*
 * On sequential recordable media, we might need to close the last session to
 * be able to write new anchors/new fs.
 */
static int
udf_open_new_session(void)
{
#if !HAVE_NBTOOL_CONFIG_H
	struct mmc_trackinfo ti;
	struct mmc_op        op;
	int tracknr, error;

	/* if the drive is not sequential, we're done */
	if ((mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) == 0)
		return 0;

	/* close the last session if its still open */
	if (mmc_discinfo.last_session_state == MMC_STATE_INCOMPLETE) {
		/*
		 * Leave the disc alone if force format is not set, it will
		 * error out later
		 */
		if (!context.create_new_session)
			return 0;

//		printf("Closing last open session if present\n");
		/* close all associated tracks */
		tracknr = mmc_discinfo.first_track_last_session;
		while (tracknr <= mmc_discinfo.last_track_last_session) {
			ti.tracknr = tracknr;
			error = udf_update_trackinfo(&ti);
			if (error)
				return error;
//			printf("\tClosing open track %d\n", tracknr);
			memset(&op, 0, sizeof(op));
			op.operation   = MMC_OP_CLOSETRACK;
			op.mmc_profile = mmc_discinfo.mmc_profile;
			op.tracknr     = tracknr;
			error = ioctl(dev_fd, MMCOP, &op);
			if (error)
				return error;
			tracknr ++;
		}
//		printf("Closing session\n");
		memset(&op, 0, sizeof(op));
		op.operation   = MMC_OP_CLOSESESSION;
		op.mmc_profile = mmc_discinfo.mmc_profile;
		op.sessionnr   = mmc_discinfo.num_sessions;
		error = ioctl(dev_fd, MMCOP, &op);
		if (error)
			return error;

		/* update discinfo since it changed by the operations */
		error = udf_update_discinfo();
		if (error)
			return error;
	}
#endif
	return 0;
}


/* bit paranoid but tracks may need repair before they can be written to */
static void
udf_repair_tracks(void)
{
#if !HAVE_NBTOOL_CONFIG_H
	struct mmc_trackinfo ti;
	struct mmc_op        op;
	int tracknr, error;

	tracknr = mmc_discinfo.first_track_last_session;
	while (tracknr <= mmc_discinfo.last_track_last_session) {
		ti.tracknr = tracknr;
		error = udf_update_trackinfo(&ti);
		if (error) {
			warnx("error updating track information for track %d",
				tracknr);
			/* resume */
			tracknr++;
			continue;
		}

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
			error = ioctl(dev_fd, MMCOP, &op);

			if (error)
				warnx("drive notifies it can't explicitly repair "
					"damaged track, but it might autorepair\n");
		}
		tracknr++;
	}
	/* tracks (if any) might not be damaged now, operations are ok now */
#endif
}


int
udf_prepare_disc(void)
{
#if !HAVE_NBTOOL_CONFIG_H
	int error;

	/* setup write parameters from discinfo */
	error = udf_setup_writeparams();
	if (error)
		return error;

	udf_repair_tracks();

	/* open new session if needed */
	return udf_open_new_session();
#endif
	return 0;
}


/* --------------------------------------------------------------------- */

/*
 * write queue implementation
 */

void
udf_suspend_writing(void)
{
	write_queue_suspend = 1;
}


void
udf_allow_writing(void)
{
	write_queue_suspend = 0;
}


static void
udf_init_writequeue(int write_strategy)
{
	context.write_strategy = write_strategy;
	write_queue_suspend = 0;

	/* setup sector writeout queue's */
	TAILQ_INIT(&write_queue);
	write_queuelen = 0;
}


int
udf_write_sector(void *sector, uint64_t location)
{
	struct wrpacket *packet, *found_packet;
	uint64_t rel_loc;
	uint64_t blockingnr = layout.blockingnr;
	int error;

	assert(!dev_fd_rdonly);
	assert(blockingnr >= 1);
	assert(blockingnr <= 64);

	/*
	 * We have a write strategy but in practice packet writing is
	 * preferable for all media types.
	 */

again:
	/* search location */
	found_packet = NULL;
	TAILQ_FOREACH_REVERSE(packet, &write_queue, wrpacket_list, next) {
		if (packet->start_sectornr <= location) {
			found_packet = packet;
			break;
		}
	}

	/* are we in a current packet? */
	if (found_packet) {
		uint64_t base = found_packet->start_sectornr;
		if ((location >= base) && (location -base < blockingnr)) {
			/* fill in existing packet */
			rel_loc = location - base;
			memcpy(found_packet->packet_data +
				rel_loc * context.sector_size,
				sector, context.sector_size);
			found_packet->present |= ((uint64_t) 1 << rel_loc);
			return 0;
		}
	}

	if ((write_queuelen > UDF_MAX_QUEUELEN) && !write_queue_suspend) {
		/* we purge the queue and reset found_packet! */
		error = udf_writeout_writequeue(false);
		if (error)
			return error;
		goto again;
	}

	/* create new packet */
	packet = calloc(1, sizeof(struct wrpacket));
	if (packet == NULL)
		return errno;
	packet->packet_data = calloc(1, context.sector_size * blockingnr);
	if (packet->packet_data == NULL) {
		free(packet);
		return errno;
	}
	packet->start_sectornr =
		UDF_ROUNDDOWN(location, blockingnr) + wrtrack_skew;
	rel_loc = location - packet->start_sectornr;

	memcpy(packet->packet_data +
		rel_loc * context.sector_size,
		sector, context.sector_size);
	packet->present = ((uint64_t) 1 << rel_loc);

	if (found_packet) {
		TAILQ_INSERT_AFTER(&write_queue, found_packet, packet, next);
	} else {
		TAILQ_INSERT_HEAD(&write_queue, packet, next);
	}
	write_queuelen++;

	return 0;
}


int
udf_read_sector(void *sector, uint64_t location)
{
	struct wrpacket *packet, *found_packet;
	ssize_t ret;
	uint64_t rpos, rel_loc;
	uint64_t blockingnr = layout.blockingnr;

	rpos = (uint64_t) location * context.sector_size;

	/* search location */
	found_packet = NULL;
	TAILQ_FOREACH_REVERSE(packet, &write_queue, wrpacket_list, next) {
		if (packet->start_sectornr <= location) {
			found_packet = packet;
			break;
		}
	}

	/* are we in a current packet? */
	if (found_packet) {
		uint64_t base = found_packet->start_sectornr;
		if ((location >= base) && (location -base < blockingnr)) {
			/* fill in existing packet */
			rel_loc = location - base;
			if (found_packet->present & ((uint64_t) 1 << rel_loc)) {
				memcpy(sector, found_packet->packet_data +
					rel_loc * context.sector_size,
					context.sector_size);
			} else {
				ret = pread(dev_fd, sector, context.sector_size, rpos);
				if (ret == -1)
					return errno;
				if (ret < (int) context.sector_size)
					return EIO;
				memcpy(found_packet->packet_data +
					rel_loc * context.sector_size,
					sector, context.sector_size);
				found_packet->present |= ((uint64_t) 1 << rel_loc);
				return 0;
			}
		}
	}
	/* don't create a packet just for we read something */
	ret = pread(dev_fd, sector, context.sector_size, rpos);
	if (ret == -1)
		return errno;
	if (ret < (int) context.sector_size)
		return EIO;
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
udf_writeout_writequeue(bool complete)
{
	struct wrpacket *packet, *next_packet;
	int		 blockingnr = layout.blockingnr;
	int		 linesize, offset, ret;
	uint8_t		*linebuf;
	uint64_t	 present, all_present = -1;
	uint64_t	 rpos, wpos;
	static int	 t = 0;

	if (write_queuelen == 0)
		return 0;

	if (blockingnr < 64)
		all_present = ((uint64_t) 1 << blockingnr) -1;
	linesize = blockingnr * context.sector_size;
	linebuf = calloc(1, linesize);
	assert(linebuf);

	/* fill in blanks if needed */
	if (complete && (context.write_strategy != UDF_WRITE_SEQUENTIAL)) {
		TAILQ_FOREACH(packet, &write_queue, next) {
			present = packet->present;
			if (present != all_present) {
				printf("%c", "\\|/-"[t++ % 4]); fflush(stdout);fflush(stderr);
//printf("%16lu : readin %08lx\n", packet->start_sectornr, packet->present ^ all_present);
				rpos = (uint64_t)  packet->start_sectornr * context.sector_size;
				ret = pread(dev_fd, linebuf, linesize, rpos);
				if (ret == -1) {
					printf("\b");
					warn("error reading in blanks, "
						"could indicate bad disc");
					printf(" ");
				}
				for (int i = 0; i < blockingnr; i++) {
//printf("present %08lx, testing bit %08lx, value %08lx\n", present, ((uint64_t) 1 << i), (present & ((uint64_t) 1 << i)));
					if ((present & ((uint64_t) 1 << i)) > 0)
						continue;
//printf("NOT PRESENT\n");
					offset = i * context.sector_size;
					memcpy(packet->packet_data + offset,
						linebuf + offset,
						context.sector_size);
					packet->present |= ((uint64_t) 1<<i);
				}
				printf("\b");
			}
			assert(packet->present == all_present);
		}
	}

	/* writeout */
	TAILQ_FOREACH(packet, &write_queue, next) {
		if (complete || (packet->present == all_present)) {
			printf("%c", "\\|/-"[t++ % 4]); fflush(stdout);fflush(stderr);
//printf("write %lu + %d\n", packet->start_sectornr, linesize / context.sector_size);
			wpos = (uint64_t) packet->start_sectornr * context.sector_size;
			ret = pwrite(dev_fd, packet->packet_data, linesize, wpos);
			printf("\b");
			if (ret == -1)
				warn("error writing packet, "
					"could indicate bad disc");
		}
	}

	/* removing completed packets */
	TAILQ_FOREACH_SAFE(packet, &write_queue, next, next_packet) {
		if (complete || (packet->present == all_present)) {
			TAILQ_REMOVE(&write_queue, packet, next);
			free(packet->packet_data);
			free(packet);
			write_queuelen--;
		}
	}
	if (complete) {
		assert(TAILQ_EMPTY(&write_queue));
		write_queuelen = 0;
	}

	free(linebuf);
	return 0;
}


/* --------------------------------------------------------------------- */

/* simplified version of kernel routine */
int
udf_translate_vtop(uint32_t lb_num, uint16_t vpart,
		   uint32_t *lb_numres, uint32_t *extres)
{
	struct part_desc       *pdesc;
	struct spare_map_entry *sme;
	struct short_ad        *short_ad;
	struct extfile_entry   *efe;
	uint32_t ext, len, lb_rel, lb_packet, vat_off;
	uint32_t start_lb, lb_offset, end_lb_offset;
	uint32_t udf_rw32_lbmap;
	uint32_t flags;
	uint8_t *vat_pos, *data_pos;
	int dscr_size, l_ea, l_ad, icbflags, addr_type;
	int rel, part;

	if (vpart > UDF_VTOP_RAWPART)
		return EINVAL;

	ext = INT_MAX;
translate_again:
	part = context.vtop[vpart];
	pdesc = context.partitions[part];

	switch (context.vtop_tp[vpart]) {
	case UDF_VTOP_TYPE_RAW :
		/* 1:1 to the end of the device */
		*lb_numres = lb_num;
		*extres = MIN(ext, INT_MAX);
		return 0;
	case UDF_VTOP_TYPE_PHYS :
		/* transform into its disc logical block */
		if (lb_num > udf_rw32(pdesc->part_len))
			return EINVAL;
		*lb_numres = lb_num + udf_rw32(pdesc->start_loc);

		/* extent from here to the end of the partition */
		*extres = MIN(ext, udf_rw32(pdesc->part_len) - lb_num);
		if (*extres == 0)
			return EINVAL;
		return 0;
	case UDF_VTOP_TYPE_VIRT :
		/* only maps one logical block, lookup in VAT */
		if (lb_num * 4 >= context.vat_size)
			return EINVAL;
		vat_off = context.vat_start + lb_num * 4;
		vat_pos = context.vat_contents + vat_off;
		udf_rw32_lbmap = *((uint32_t *) vat_pos);

		if (vat_off >= context.vat_size)		/* XXX > or >= ? */
			return EINVAL;
		lb_num = udf_rw32(udf_rw32_lbmap);

		/* transform into its disc logical block */
		if (lb_num > udf_rw32(pdesc->part_len))
			return EINVAL;
		*lb_numres = lb_num + udf_rw32(pdesc->start_loc);

		/* just one logical block */
		*extres = 1;
		return 0;
	case UDF_VTOP_TYPE_SPAREABLE :
		/* check if the packet containing the lb_num is remapped */
		lb_packet = lb_num / layout.spareable_blockingnr;
		lb_rel    = lb_num % layout.spareable_blockingnr;

		for (rel = 0; rel < udf_rw16(context.sparing_table->rt_l); rel++) {
			sme = &context.sparing_table->entries[rel];
			if (lb_packet == udf_rw32(sme->org)) {
				/* NOTE maps to absolute disc logical block! */
				*lb_numres = udf_rw32(sme->map) + lb_rel;
				*extres    = layout.spareable_blockingnr - lb_rel;
				return 0;
			}
		}

		/* transform into its disc logical block */
		if (lb_num > udf_rw32(pdesc->part_len))
			return EINVAL;
		*lb_numres = lb_num + udf_rw32(pdesc->start_loc);

		/* rest of block */
		*extres = MIN(ext, layout.spareable_blockingnr - lb_rel);
		return 0;
	case UDF_VTOP_TYPE_META :
		/* we have to look into the file's allocation descriptors */

		/* get first overlapping extent */
		efe = context.meta_file;
		dscr_size = sizeof(struct extfile_entry) - 1;
		l_ea = udf_rw32(efe->l_ea);
		l_ad = udf_rw32(efe->l_ad);

		icbflags = udf_rw16(efe->icbtag.flags);
		addr_type = icbflags & UDF_ICB_TAG_FLAGS_ALLOC_MASK;
		if (addr_type != UDF_ICB_SHORT_ALLOC) {
			warnx("specification violation: metafile not using"
				"short allocs");
			return EINVAL;
		}

		data_pos = (uint8_t *) context.meta_file + dscr_size + l_ea;
		short_ad = (struct short_ad *) data_pos;
		lb_offset = 0;
		while (l_ad > 0) {
			len      = udf_rw32(short_ad->len);
			start_lb = udf_rw32(short_ad->lb_num);
			flags    = UDF_EXT_FLAGS(len);
			len      = UDF_EXT_LEN(len);
			if (flags == UDF_EXT_REDIRECT) {
				warnx("implementation limit: no support for "
				      "extent redirection in metadata file");
				return EINVAL;
			}
			end_lb_offset = lb_offset + len / context.sector_size;
			/* overlap? */
			if (end_lb_offset > lb_num)
				break;
			short_ad++;
			lb_offset = end_lb_offset;
			l_ad -= sizeof(struct short_ad);
		}
		if (l_ad <= 0) {
			warnx("looking up outside metadata partition!");
			return EINVAL;
		}
		lb_num = start_lb + (lb_num - lb_offset);
		vpart  = part;
		ext = end_lb_offset - lb_num;
		/*
		 * vpart and lb_num are updated, translate again since we
		 * might be mapped on spareable media
		 */
		goto translate_again;
	default:
		printf("UDF vtop translation scheme %d unimplemented yet\n",
			context.vtop_tp[vpart]);
	}

	return EINVAL;
}

/* --------------------------------------------------------------------- */

int
udf_read_phys(void *blob, uint32_t location, uint32_t sects)
{
	uint32_t phys, cnt;
	uint8_t *bpos;
	int error;

	for (cnt = 0; cnt < sects; cnt++) {
		bpos  = (uint8_t *) blob;
		bpos += context.sector_size * cnt;

		phys = location + cnt;
		error = udf_read_sector(bpos, phys);
		if (error)
			return error;
	}
	return 0;
}


int
udf_write_phys(void *blob, uint32_t location, uint32_t sects)
{
	uint32_t phys, cnt;
	uint8_t *bpos;
	int error;

	for (cnt = 0; cnt < sects; cnt++) {
		bpos  = (uint8_t *) blob;
		bpos += context.sector_size * cnt;

		phys = location + cnt;
		error = udf_write_sector(bpos, phys);
		if (error)
			return error;
	}
	return 0;
}


int
udf_read_virt(void *blob, uint32_t location, uint16_t vpart,
	uint32_t sectors)
{
	uint32_t phys, ext;
	uint8_t *data;
	int error;

	/* determine physical location */
	data = (uint8_t *) blob;
	while (sectors) {
		if (udf_translate_vtop(location, vpart, &phys, &ext)) {
			// warnx("internal error: bad translation");
			return EINVAL;
		}
		ext = MIN(sectors, ext);
		error = udf_read_phys(data, phys, ext);
		if (error)
			return error;
		location += ext;
		data     += ext * context.sector_size;
		sectors  -= ext;
	}
	return 0;
}


int
udf_write_virt(void *blob, uint32_t location, uint16_t vpart,
	uint32_t sectors)
{
	uint32_t phys, ext, alloc_pos;
	uint8_t *data;
	int error;

	/* determine physical location */
	if (context.vtop_tp[vpart] == UDF_VTOP_TYPE_VIRT) {
		assert(sectors == 1);
		alloc_pos = context.alloc_pos[context.data_part];
		udf_vat_update(location, alloc_pos);
		udf_translate_vtop(alloc_pos, context.vtop[vpart], &phys, &ext);
		context.alloc_pos[context.data_part]++;
		return udf_write_phys(blob, phys, sectors);
	}

	data = (uint8_t *) blob;
	while (sectors) {
		if (udf_translate_vtop(location, vpart, &phys, &ext)) {
			warnx("internal error: bad translation");
			return EINVAL;
		}
		ext = MIN(sectors, ext);
		error = udf_write_phys(data, phys, ext);
		if (error)
			return error;
		location += ext;
		data     += ext * context.sector_size;
		sectors  -= ext;
	}
	return 0;
}


int
udf_read_dscr_phys(uint32_t sector, union dscrptr **dstp)
{
	union dscrptr *dst, *new_dst;
	uint8_t *pos;
	uint32_t sectors, dscrlen, sector_size;
	int error;

	sector_size = context.sector_size;

	*dstp = dst = NULL;
	dscrlen = sector_size;

	/* read initial piece */
	dst = malloc(sector_size);
	assert(dst);
	error = udf_read_sector(dst, sector);
//	if (error)
//		warn("read error");

	if (!error) {
		/* check if its an empty block */
		if (is_zero(dst, sector_size)) {
			/* return no error but with no dscrptr */
			/* dispose first block */
			free(dst);
			return 0;
		}
		/* check if its a valid tag */
		error = udf_check_tag(dst);
		if (error) {
			free(dst);
			return 0;
		}
		/* calculate descriptor size */
		dscrlen = udf_tagsize(dst, sector_size);
	}

	if (!error && (dscrlen > sector_size)) {
		/* read the rest of descriptor */

		new_dst = realloc(dst, dscrlen);
		if (new_dst == NULL) {
			free(dst);
			return ENOMEM;
		}
		dst = new_dst;

		sectors = dscrlen / sector_size;
		pos = (uint8_t *) dst + sector_size;
		error = udf_read_phys(pos, sector + 1, sectors-1);
		if (error)
			warnx("read error");
	}
	if (!error)
		error = udf_check_tag_payload(dst, dscrlen);
	if (error && dst) {
		free(dst);
		dst = NULL;
	}
	*dstp = dst;

	return error;
}


int
udf_write_dscr_phys(union dscrptr *dscr, uint32_t location,
	uint32_t sectors)
{
	dscr->tag.tag_loc = udf_rw32(location);
	(void) udf_validate_tag_and_crc_sums(dscr);

	assert(sectors == udf_tagsize(dscr, context.sector_size) / context.sector_size);
	return udf_write_phys(dscr, location, sectors);
}


int
udf_read_dscr_virt(uint32_t sector, uint16_t vpart, union dscrptr **dstp)
{
	union dscrptr *dst, *new_dst;
	uint8_t *pos;
	uint32_t sectors, dscrlen, sector_size;
	int error;

	sector_size = context.sector_size;

	*dstp = dst = NULL;
	dscrlen = sector_size;

	/* read initial piece */
	dst = calloc(1, sector_size);
	assert(dst);
	error = udf_read_virt(dst, sector, vpart, 1);
	if (error)
		return error;

	if (!error) {
		/* check if its a valid tag */
		error = udf_check_tag(dst);
		if (error) {
			/* check if its an empty block */
			if (is_zero(dst, sector_size)) {
				/* return no error but with no dscrptr */
				/* dispose first block */
				free(dst);
				return 0;
			}
		}
		/* calculate descriptor size */
		dscrlen = udf_tagsize(dst, sector_size);
	}

	if (!error && (dscrlen > sector_size)) {
		/* read the rest of descriptor */

		new_dst = realloc(dst, dscrlen);
		if (new_dst == NULL) {
			free(dst);
			return ENOMEM;
		}
		dst = new_dst;

		sectors = dscrlen / sector_size;
		pos = (uint8_t *) dst + sector_size;
		error = udf_read_virt(pos, sector + 1, vpart, sectors-1);
		if (error)
			warn("read error");
	}
	if (!error)
		error = udf_check_tag_payload(dst, dscrlen);
	if (error && dst) {
		free(dst);
		dst = NULL;
	}
	*dstp = dst;

	return error;
}


int
udf_write_dscr_virt(union dscrptr *dscr, uint32_t location, uint16_t vpart,
	uint32_t sectors)
{
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct extattrhdr_desc *extattrhdr;

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

	assert(sectors >= (udf_tagsize(dscr, context.sector_size) / context.sector_size));
	return udf_write_virt(dscr, location, vpart, sectors);
}


int
is_zero(void *blob, int size) {
	uint8_t *p = blob;
	for (int i = 0; i < size; i++, p++)
		if (*p)
			return 0;
	return 1;
}

/* --------------------------------------------------------------------- */

static void
udf_partition_alloc(int nblk, int vpart, struct long_ad *pos)
{
	memset(pos, 0, sizeof(*pos));
	pos->len	  = udf_rw32(nblk * context.sector_size);
	pos->loc.lb_num   = udf_rw32(context.alloc_pos[vpart]);
	pos->loc.part_num = udf_rw16(vpart);

	udf_mark_allocated(context.alloc_pos[vpart], vpart, nblk);
	context.alloc_pos[vpart] += nblk;
}


void
udf_metadata_alloc(int nblk, struct long_ad *pos)
{
	udf_partition_alloc(nblk, context.metadata_part, pos);
}


void
udf_data_alloc(int nblk, struct long_ad *pos)
{
	udf_partition_alloc(nblk, context.data_part, pos);
}


void
udf_fids_alloc(int nblk, struct long_ad *pos)
{
	udf_partition_alloc(nblk, context.fids_part, pos);
}


/* --------------------------------------------------------------------- */

/*
 * udf_derive_format derives the format_flags from the disc's mmc_discinfo.
 * The resulting flags uniquely define a disc format. Note there are at least
 * 7 distinct format types defined in UDF.
 */

#define UDF_VERSION(a) \
	(((a) == 0x102) || ((a) == 0x150) || ((a) == 0x200) || \
	 ((a) == 0x201) || ((a) == 0x250) || ((a) == 0x260))

int
udf_derive_format(int req_enable, int req_disable)
{
	int format_flags;
	int media_accesstype;

	/* disc writability, formatted, appendable */
	if ((mmc_discinfo.mmc_cur & MMC_CAP_RECORDABLE) == 0) {
		warnx("can't newfs readonly device");
		return EROFS;
	}
	if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL) {
		/* sequentials need sessions appended */
		if (mmc_discinfo.disc_state == MMC_STATE_CLOSED) {
			warnx("can't append session to a closed disc");
			return EROFS;
		}
		if ((mmc_discinfo.disc_state != MMC_STATE_EMPTY) &&
				!context.create_new_session) {
			warnx("disc not empty! Use -F to force "
			    "initialisation");
			return EROFS;
		}
	} else {
		/* check if disc (being) formatted or has been started on */
		if (mmc_discinfo.disc_state == MMC_STATE_EMPTY) {
			warnx("disc is not formatted");
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
			/* spareables for defect management */
			if (context.min_udf >= 0x150)
				format_flags |= FORMAT_SPAREABLE;
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
		format_flags &= ~(FORMAT_META | FORMAT_LOW);
		req_disable  &= ~FORMAT_META;
	}
	if ((format_flags & FORMAT_VAT) & UDF_512_TRACK)
		format_flags |= FORMAT_TRACK512;

	if (req_enable & FORMAT_READONLY) {
		format_flags |= FORMAT_READONLY;
	}

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

	/* patch up media accesstype */
	if (req_enable & FORMAT_READONLY) {
		/* better now */
		media_accesstype = UDF_ACCESSTYPE_READ_ONLY;
	}

	/* adjust minimum version limits */
	if (format_flags & FORMAT_VAT)
		context.min_udf = MAX(context.min_udf, 0x0150);
	if (format_flags & FORMAT_SPAREABLE)
		context.min_udf = MAX(context.min_udf, 0x0150);
	if (format_flags & FORMAT_META)
		context.min_udf = MAX(context.min_udf, 0x0250);
	if (format_flags & FORMAT_LOW)
		context.min_udf = MAX(context.min_udf, 0x0260);

	/* adjust maximum version limits not to tease or break things */
	if (!(format_flags & (FORMAT_META | FORMAT_LOW | FORMAT_VAT)) &&
	    (context.max_udf > 0x200))
		context.max_udf = 0x201;

	if ((format_flags & (FORMAT_VAT | FORMAT_SPAREABLE)) == 0)
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
		warnx("initialisation prohibited by specified maximum "
		    "UDF version 0x%04x. Minimum version required 0x%04x",
		    context.max_udf, context.min_udf);
		return EPERM;
	}

	if (!UDF_VERSION(context.min_udf) || !UDF_VERSION(context.max_udf)) {
		warnx("internal error, invalid min/max udf versionsi in %s",
			__func__);
		return EPERM;
	}
	context.format_flags = format_flags;
	context.media_accesstype = media_accesstype;

	return 0;
}

#undef UDF_VERSION


/* --------------------------------------------------------------------- */

int
udf_proces_names(void)
{
	struct timeval time_of_day;
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
			(void)gettimeofday(&time_of_day, NULL);
			volset_nr  =  (uint64_t) random();
			volset_nr |= ((uint64_t) time_of_day.tv_sec) << 32;
		}
		context.volset_name = calloc(128,1);
		sprintf(context.volset_name, "%016"PRIx64, volset_nr);
	}
	if (context.fileset_name == NULL)
		context.fileset_name = strdup("anonymous");

	/* check passed/created identifiers */
	if (strlen(context.logvol_name)  > 128) {
		warnx("logical volume name too long");
		return EINVAL;
	}
	if (strlen(context.primary_name) >  32) {
		warnx("primary volume name too long");
		return EINVAL;
	}
	if (strlen(context.volset_name)  > 128) {
		warnx("volume set name too long");
		return EINVAL;
	}
	if (strlen(context.fileset_name) > 32) {
		warnx("fileset name too long");
		return EINVAL;
	}

	/* signal all OK */
	return 0;
}

/* --------------------------------------------------------------------- */

int
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
	if ((context.format_flags & FORMAT_TRACK512) == 0) {
		dpos = (2048 + context.sector_size - 1) / context.sector_size;

		/* wipe at least 6 times 2048 byte `sectors' */
		for (cnt = 0; cnt < 6 *dpos; cnt++) {
			pos = layout.iso9660_vrs + cnt;
			if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
				free(iso9660_vrs_desc);
				return error;
			}
		}

		/* common VRS fields in all written out ISO descriptors */
		iso9660_vrs_desc->struct_type = 0;
		iso9660_vrs_desc->version     = 1;
		pos = layout.iso9660_vrs;

		/* BEA01, NSR[23], TEA01 */
		memcpy(iso9660_vrs_desc->identifier, "BEA01", 5);
		if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
			free(iso9660_vrs_desc);
			return error;
		}
		pos += dpos;

		if (context.dscrver == 2)
			memcpy(iso9660_vrs_desc->identifier, "NSR02", 5);
		else
			memcpy(iso9660_vrs_desc->identifier, "NSR03", 5);
		;
		if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
			free(iso9660_vrs_desc);
			return error;
		}
		pos += dpos;

		memcpy(iso9660_vrs_desc->identifier, "TEA01", 5);
		if ((error = udf_write_sector(iso9660_vrs_desc, pos))) {
			free(iso9660_vrs_desc);
			return error;
		}
	}

	free(iso9660_vrs_desc);
	/* return success */
	return 0;
}


/* --------------------------------------------------------------------- */

int
udf_get_blockingnr(struct mmc_trackinfo *ti)
{
	int blockingnr;

	/* determine blockingnr */
	blockingnr = ti->packet_size;
	if (blockingnr <= 1) {
		/* paranoia on blockingnr */
		switch (mmc_discinfo.mmc_profile) {
		case 0x01 : /* DISC */
			blockingnr = 64;
			break;
		case 0x08 : /* CDROM */
		case 0x09 : /* CD-R    */
		case 0x0a : /* CD-RW   */
			blockingnr = 32;	/* UDF requirement */
			break;
		case 0x10 : /* DVDROM */
		case 0x11 : /* DVD-R (DL) */
		case 0x12 : /* DVD-RAM */
		case 0x1b : /* DVD+R      */
		case 0x2b : /* DVD+R Dual layer */
		case 0x13 : /* DVD-RW restricted overwrite */
		case 0x14 : /* DVD-RW sequential */
		case 0x1a : /* DVD+RW */
			blockingnr = 16;	/* SCSI definition */
			break;
		case 0x40 : /* BDROM */
		case 0x41 : /* BD-R Sequential recording (SRM) */
		case 0x42 : /* BD-R Random recording (RRM) */
		case 0x43 : /* BD-RE */
		case 0x51 : /* HD DVD-R   */
		case 0x52 : /* HD DVD-RW  */
			blockingnr = 32;	/* SCSI definition */
			break;
		default:
			break;
		}
	}
	return blockingnr;
}


int
udf_spareable_blocks(void)
{
	if (mmc_discinfo.mmc_class == MMC_CLASS_CD) {
		/* not too much for CD-RW, still 20MiB */
		return 32;
	} else {
		/* take a value for DVD*RW mainly, BD is `defect free' */
		return 512;
	}
}


int
udf_spareable_blockingnr(void)
{
	struct mmc_trackinfo ti;
	int spareable_blockingnr;
	int error;

	/* determine span/size */
	ti.tracknr = mmc_discinfo.first_track_last_session;
	error = udf_update_trackinfo(&ti);
	spareable_blockingnr = udf_get_blockingnr(&ti);
	if (error)
		spareable_blockingnr = 32;

	/*
	 * Note that for (bug) compatibility with version UDF 2.00
	 * (fixed in 2.01 and higher) the blocking size needs to be 32
	 * sectors otherwise the drive's blockingnr.
	 */
	if (context.min_udf <= 0x200)
		spareable_blockingnr = 32;
	return spareable_blockingnr;
}


/*
 * Main function that creates and writes out disc contents based on the
 * format_flags's that uniquely define the type of disc to create.
 */

int
udf_do_newfs_prefix(void)
{
	union dscrptr *zero_dscr;
	union dscrptr *dscr;
	struct mmc_trackinfo ti;
	uint32_t blockingnr;
	uint32_t cnt, loc, len;
	int sectcopy;
	int error, integrity_type;
	int data_part, metadata_part;
	int format_flags;

	/* init */
	format_flags = context.format_flags;

	/* determine span/size */
	ti.tracknr = mmc_discinfo.first_track_last_session;
	error = udf_update_trackinfo(&ti);
	if (error)
		return error;

	if (mmc_discinfo.sector_size > context.sector_size) {
		warnx("impossible to format: "
			"sector size %d too small for media sector size %d",
			context.sector_size, mmc_discinfo.sector_size);
		return EIO;
	}

	/* determine blockingnr */
	blockingnr = udf_get_blockingnr(&ti);
	if (blockingnr <= 0) {
		warnx("can't fixup blockingnumber for device "
			"type %d", mmc_discinfo.mmc_profile);
		warnx("device is not returning valid blocking"
			" number and media type is unknown");
		return EINVAL;
	}

	wrtrack_skew = 0;
	if (mmc_discinfo.mmc_cur & MMC_CAP_SEQUENTIAL)
		wrtrack_skew = ti.next_writable % blockingnr;

	/* get layout */
	error = udf_calculate_disc_layout(context.min_udf,
		ti.track_start, mmc_discinfo.last_possible_lba,
		context.sector_size, blockingnr);

	/* cache partition for we need it often */
	data_part     = context.data_part;
	metadata_part = context.metadata_part;

	/* Create sparing table descriptor if applicable */
	if (format_flags & FORMAT_SPAREABLE) {
		if ((error = udf_create_sparing_tabled()))
			return error;

		if (context.check_surface) {
			if ((error = udf_surface_check()))
				return error;
		}
	}

	/* Create a generic terminator descriptor (later reused) */
	terminator_dscr = calloc(1, context.sector_size);
	if (terminator_dscr == NULL)
		return ENOMEM;
	udf_create_terminator(terminator_dscr, 0);

	/*
	 * Create the two Volume Descriptor Sets (VDS) each containing the
	 * following descriptors : primary volume, partition space,
	 * unallocated space, logical volume, implementation use and the
	 * terminator
	 */

	/* start of volume recognition sequence building */
	context.vds_seq = 0;

	/* Create primary volume descriptor */
	if ((error = udf_create_primaryd()))
		return error;

	/* Create partition descriptor */
	if ((error = udf_create_partitiond(context.data_part)))
		return error;

	/* Create unallocated space descriptor */
	if ((error = udf_create_unalloc_spaced()))
		return error;

	/* Create logical volume descriptor */
	if ((error = udf_create_logical_dscr()))
		return error;

	/* Create implementation use descriptor */
	/* TODO input of fields 1,2,3 and passing them */
	if ((error = udf_create_impvold(NULL, NULL, NULL)))
		return error;

	/* Create anchors */
	for (cnt = 0; cnt < 3; cnt++) {
		if ((error = udf_create_anchor(cnt))) {
			return error;
		}
	}

	/*
	 * Write out what we've created so far.
	 *
	 * Start with wipeout of VRS1 upto start of partition. This allows
	 * formatting for sequentials with the track reservation and it
	 * cleans old rubbish on rewritables. For sequentials without the
	 * track reservation all is wiped from track start.
	 */
	if ((zero_dscr = calloc(1, context.sector_size)) == NULL)
		return ENOMEM;

	loc = (format_flags & FORMAT_TRACK512) ? layout.vds1 : ti.track_start;
	for (; loc < layout.part_start_lba; loc++) {
		if ((error = udf_write_sector(zero_dscr, loc))) {
			free(zero_dscr);
			return error;
		}
	}
	free(zero_dscr);

	/* writeout iso9660 vrs */
	if ((error = udf_write_iso9660_vrs()))
		return error;

	/* Writeout anchors */
	for (cnt = 0; cnt < 3; cnt++) {
		dscr = (union dscrptr *) context.anchors[cnt];
		loc  = layout.anchors[cnt];
		if ((error = udf_write_dscr_phys(dscr, loc, 1))) {
			err(1, "ERR!");
			return error;
		}

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

	/* writeout the two spareable table descriptors (if needed) */
	if (format_flags & FORMAT_SPAREABLE) {
		for (sectcopy = 1; sectcopy <= 2; sectcopy++) {
			loc  = (sectcopy == 1) ? layout.spt_1 : layout.spt_2;
			dscr = (union dscrptr *) context.sparing_table;
			len  = udf_tagsize(dscr, context.sector_size) /
					context.sector_size;

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
	if ((format_flags & (FORMAT_SEQUENTIAL | FORMAT_READONLY)) == 0) {
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
		error = udf_create_meta_files();
		if (error)
			return error;

		/* mark space allocated for meta partition and its bitmap */
		udf_mark_allocated(layout.meta_file,   data_part, 1);
		udf_mark_allocated(layout.meta_mirror, data_part, 1);
		udf_mark_allocated(layout.meta_part_start_lba, data_part,
			layout.meta_part_size_lba);

		if (context.meta_bitmap) {
			/* metadata bitmap creation and accounting */
			error = udf_create_space_bitmap(
					layout.meta_bitmap_dscr_size,
					layout.meta_part_size_lba,
					&context.part_unalloc_bits[metadata_part]);
			if (error)
				return error;

			udf_mark_allocated(layout.meta_bitmap, data_part, 1);
			/* mark space allocated for the unallocated space bitmap */
			udf_mark_allocated(layout.meta_bitmap_space,
					data_part,
				layout.meta_bitmap_dscr_size);
		}
	}

	/* create logical volume integrity descriptor */
	context.num_files = 0;
	context.num_directories = 0;
	integrity_type = UDF_INTEGRITY_OPEN;
	if ((error = udf_create_lvintd(integrity_type)))
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

	/* create VAT if needed */
	if (format_flags & FORMAT_VAT) {
		context.vat_allocated = context.sector_size;
		context.vat_contents  = malloc(context.vat_allocated);
		assert(context.vat_contents);

		udf_prepend_VAT_file();
	}

	/* create FSD and writeout */
	if ((error = udf_create_fsd()))
		return error;
	udf_mark_allocated(layout.fsd, metadata_part, 1);

	dscr = (union dscrptr *) context.fileset_desc;
	error = udf_write_dscr_virt(dscr, layout.fsd, metadata_part, 1);

	return error;
}


/* specific routine for newfs to create empty rootdirectory */
int
udf_do_rootdir(void)
{
	union dscrptr *root_dscr;
	int error;

	/* create root directory and write out */
	assert(context.unique_id == 0x10);
	context.unique_id = 0;
	if ((error = udf_create_new_rootdir(&root_dscr)))
		return error;
	udf_mark_allocated(layout.rootdir, context.metadata_part, 1);

	error = udf_write_dscr_virt(root_dscr,
		layout.rootdir, context.metadata_part, 1);

	free(root_dscr);

	return error;
}


int
udf_do_newfs_postfix(void)
{
	union dscrptr *dscr;
	uint32_t loc, len;
	int data_part, metadata_part;
	int format_flags = context.format_flags;
	int error;

	/* cache partition for we need it often */
	data_part     = context.data_part;
	metadata_part = context.metadata_part;

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

		/* mark end of integrity descriptor sequence again */
		error = udf_write_dscr_phys(terminator_dscr, loc, 1);
		if (error)
			return error;
	}

	/* write out unallocated space bitmap on non sequential media */
	if ((format_flags & (FORMAT_SEQUENTIAL | FORMAT_READONLY)) == 0) {
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

		if (context.meta_bitmap) {
			loc = layout.meta_bitmap;
			dscr = (union dscrptr *) context.meta_bitmap;
			error = udf_write_dscr_virt(dscr, loc, data_part, 1);
			if (error)
				return error;

			/* writeout unallocated space bitmap */
			loc  = layout.meta_bitmap_space;
			dscr = (union dscrptr *)
				(context.part_unalloc_bits[metadata_part]);
			len  = layout.meta_bitmap_dscr_size;
			error = udf_write_dscr_virt(dscr, loc, data_part, len);
			if (error)
				return error;
		}
	}

	/* create and writeout a VAT */
	if (format_flags & FORMAT_VAT)
		udf_writeout_VAT();

	/* done */
	return 0;
}
