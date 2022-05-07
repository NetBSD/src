/* $NetBSD: udf.c,v 1.30 2022/05/07 08:54:02 reinoud Exp $ */

/*
 * Copyright (c) 2006, 2008, 2013, 2021, 2022 Reinoud Zandijk
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
__RCSID("$NetBSD: udf.c,v 1.30 2022/05/07 08:54:02 reinoud Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <util.h>

#if !HAVE_NBTOOL_CONFIG_H
#define _EXPOSE_MMC
#include <sys/cdio.h>
#else
#include "udf/cdio_mmc_structs.h"
#endif

#if !HAVE_NBTOOL_CONFIG_H
#define HAVE_STRUCT_TM_TM_GMTOFF
#endif

#include "makefs.h"
#include "udf_core.h"
#include "newfs_udf.h"

/* identification */
#define IMPL_NAME		"*NetBSD makefs 10.0"
#define APP_VERSION_MAIN	0
#define APP_VERSION_SUB		5

/*
 * Note: due to the setup of the newfs code, the current state of the program
 * and its options are held in a few global variables. The FS specific parts
 * are in global `context' and 'layout' structures.
 */

/* global variables describing disc and format requests */
int	 req_enable, req_disable;


/* --------------------------------------------------------------------- */

static int
udf_readonly_format(void)
{
	/*
	 * we choose the emulated profile to determine this since the media
	 * might be different from the format we create. Say creating a CDROM
	 * on a CD-R media.
	 */
	switch (emul_mmc_profile) {
	case 0x00:	/* unknown, treat as CDROM */
	case 0x08:	/* CDROM */
	case 0x10:	/* DVDROM */
	case 0x40:	/* BDROM */
		return true;
	}
	return false;
}


#define OPT_STR(letter, name, desc)  \
	{ letter, name, NULL, OPT_STRBUF, 0, 0, desc }

#define OPT_NUM(letter, name, field, min, max, desc) \
	{ letter, name, &context.field, \
	  sizeof(context.field) == 8 ? OPT_INT64 : \
	  (sizeof(context.field) == 4 ? OPT_INT32 : \
	  (sizeof(context.field) == 2 ? OPT_INT16 : OPT_INT8)), \
	  min, max, desc }

#define OPT_BOOL(letter, name, field, desc) \
	OPT_NUM(letter, name, field, 0, 1, desc)

void
udf_prep_opts(fsinfo_t *fsopts)
{
	const option_t udf_options[] = {
		OPT_STR('T', "disctype", "disc type (cdrom,dvdrom,bdrom,"
			"dvdram,bdre,disk,cdr,dvdr,bdr,cdrw,dvdrw)"),
		OPT_STR('L', "loglabel", "\"logical volume name\""),
		OPT_STR('P', "discid",   "\"[volset name ':']"
			"physical volume name\""),
		OPT_NUM('t', "tz", gmtoff, -24, 24, "timezone"),
		OPT_STR('v', "minver", "minimum UDF version in either "
			"``0x201'' or ``2.01'' format"),
		OPT_STR('V', "maxver", "maximum UDF version in either "
			"``0x201'' or ``2.01'' format"),
		OPT_NUM('p', "metaperc", meta_perc, 1, 99,
			"minimum free metadata percentage"),
		OPT_BOOL('c', "checksurface", check_surface,
			"perform crude surface check on rewritable media"),
		OPT_BOOL('F', "forceformat", create_new_session,
			"force file system construction on non-empty recordable media"),
		{ .name = NULL }
	};

	/* initialise */
	req_enable = req_disable = 0;
	fsopts->sectorsize = 512;	/* minimum allowed sector size */

	srandom((unsigned long) time(NULL));

	udf_init_create_context();
	context.app_name         = "*NetBSD UDF";
	context.app_version_main = APP_VERSION_MAIN;
	context.app_version_sub  = APP_VERSION_SUB;
	context.impl_name        = IMPL_NAME;

	/* minimum and maximum UDF versions we advise */
	context.min_udf = 0x102;
	context.max_udf = 0x250;	/* 0x260 is not ready */

	/* defaults for disc/files */
	emul_mmc_profile  =  -1;	/* invalid->no emulation	*/
	emul_packetsize   =   1;	/* reasonable default		*/
	emul_sectorsize   = 512;	/* minimum allowed sector size	*/
	emul_size	  =   0;	/* empty			*/

	/* use user's time zone as default */
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	if (!stampst.st_ino)  {
		struct tm tm;
		time_t now;
		(void)time(&now);
		(void)localtime_r(&now, &tm);
		context.gmtoff = tm.tm_gmtoff;
	} else
#endif
		context.gmtoff = 0;

	/* return info */
	fsopts->fs_specific = NULL;
	fsopts->fs_options = copy_opts(udf_options);
}


void
udf_cleanup_opts(fsinfo_t *fsopts)
{
	free(fsopts->fs_options);
}


/* ----- included from newfs_udf.c ------ */

#define CDRSIZE    ((uint64_t)   700*1024*1024)	/* small approx */
#define CDRWSIZE   ((uint64_t)   576*1024*1024)	/* small approx */
#define DVDRSIZE   ((uint64_t)  4488*1024*1024)	/* small approx */
#define DVDRAMSIZE ((uint64_t)  4330*1024*1024)	/* small approx with spare */
#define DVDRWSIZE  ((uint64_t)  4482*1024*1024)	/* small approx */
#define BDRSIZE    ((uint64_t) 23866*1024*1024)	/* small approx */
#define BDRESIZE   ((uint64_t) 23098*1024*1024)	/* small approx */
int
udf_parse_opts(const char *option, fsinfo_t *fsopts)
{
	option_t *udf_options = fsopts->fs_options;
	uint64_t stdsize, maxsize;
	uint32_t set_sectorsize;
	char buffer[1024], *buf, *colon;
	int i;

	assert(option != NULL);

	if (debug & DEBUG_FS_PARSE_OPTS)
		printf("udf_parse_opts: got `%s'\n", option);

	i = set_option(udf_options, option, buffer, sizeof(buffer));
	if (i == -1)
		return 0;

	if (udf_options[i].name == NULL)
		abort();

	set_sectorsize = 0;
	stdsize = 0;
	maxsize = 0;

	buf = buffer;
	switch (udf_options[i].letter) {
	case 'T':
		if (strcmp(buf, "cdrom") == 0) {
			emul_mmc_profile = 0x00;
			maxsize = CDRSIZE;
		} else if (strcmp(buf, "dvdrom") == 0) {
			emul_mmc_profile = 0x10;
			maxsize = DVDRSIZE;
		} else if (strcmp(buf, "bdrom") == 0) {
			emul_mmc_profile = 0x40;
			maxsize = BDRSIZE;
		} else if (strcmp(buf, "dvdram") == 0) {
			emul_mmc_profile = 0x12;
			stdsize = DVDRAMSIZE;
		} else if (strcmp(buf, "bdre") == 0) {
			emul_mmc_profile = 0x43;
			stdsize = BDRESIZE;
		} else if (strcmp(buf, "disk") == 0) {
			emul_mmc_profile = 0x01;
		} else if (strcmp(buf, "cdr") == 0) {
			emul_mmc_profile = 0x09;
			stdsize = CDRSIZE;
		} else if (strcmp(buf, "dvdr") == 0) {
			emul_mmc_profile = 0x1b;
			stdsize = DVDRSIZE;
		} else if (strcmp(buf, "bdr") == 0) {
			emul_mmc_profile = 0x41;
			stdsize = BDRSIZE;
		} else if (strcmp(buf, "cdrw") == 0) {
			emul_mmc_profile = 0x0a;
			stdsize = CDRWSIZE;
		} else if (strcmp(buf, "dvdrw") == 0) {
			emul_mmc_profile = 0x1a;
			stdsize = DVDRWSIZE;
		} else {
			errx(1, "unknown or unimplemented disc format");
		}
		if (emul_mmc_profile != 0x01)
			set_sectorsize = 2048;
		break;
	case 'L':
		if (context.logvol_name) free(context.logvol_name);
		context.logvol_name = strdup(buf);
		break;
	case 'P':
		if ((colon = strstr(buf, ":"))) {
			if (context.volset_name)
				free(context.volset_name);
			*colon = 0;
			context.volset_name = strdup(buf);
			buf = colon+1;
		}
		if (context.primary_name)
			free(context.primary_name);
		if ((strstr(buf, ":")))
			errx(1, "primary name can't have ':' in its name");
		context.primary_name = strdup(buf);
		break;
	case 'v':
		context.min_udf = a_udf_version(buf, "min_udf");
		if (context.min_udf > 0x250)
			errx(1, "maximum supported version is UDF 2.50");
		if (context.min_udf > context.max_udf)
			context.max_udf = context.min_udf;
		break;
	case 'V':
		context.max_udf = a_udf_version(buf, "min_udf");
		if (context.max_udf > 0x250)
			errx(1, "maximum supported version is UDF 2.50");
		if (context.min_udf > context.max_udf)
			context.min_udf = context.max_udf;
		break;
	}
	if (set_sectorsize)
		fsopts->sectorsize = set_sectorsize;
	if (stdsize) {
		if (fsopts->maxsize > 0)
			stdsize = MIN(stdsize, (uint64_t) fsopts->maxsize);
		if (fsopts->minsize > 0)
			stdsize = MAX(stdsize, (uint64_t) fsopts->minsize);
		fsopts->size = fsopts->minsize = fsopts->maxsize = stdsize;
	}
	if (maxsize) {
		if (fsopts->maxsize > 0)
			maxsize = MIN(maxsize, (uint64_t) fsopts->maxsize);
		if (fsopts->minsize > 0)
			maxsize = MAX(maxsize, (uint64_t) fsopts->minsize);
		fsopts->maxsize = maxsize;
	}
	return 1;
}

/* -
 * -------------------------------------------------------------------- */

struct udf_stats {
	uint32_t nfiles;
	uint32_t ndirs;
	uint32_t ndescr;
	uint32_t nmetadatablocks;
	uint32_t ndatablocks;
};


/* node reference administration */
static void
udf_inc_link(union dscrptr *dscr)
{
	struct file_entry *fe;
	struct extfile_entry *efe;

	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe        = &dscr->fe;
		fe->link_cnt = udf_rw16(udf_rw16(fe->link_cnt) + 1);
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe       = &dscr->efe;
		efe->link_cnt = udf_rw16(udf_rw16(efe->link_cnt) + 1);
	} else {
		errx(1, "bad tag passed to udf_inc_link");
	}
}


static void
udf_set_link_cnt(union dscrptr *dscr, int num)
{
	struct file_entry *fe;
	struct extfile_entry *efe;

	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe        = &dscr->fe;
		fe->link_cnt = udf_rw16(num);
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe       = &dscr->efe;
		efe->link_cnt = udf_rw16(num);
	} else {
		errx(1, "bad tag passed to udf_set_link_cnt");
	}
}


static uint32_t
udf_datablocks(off_t sz)
{
	/* predictor if it can be written inside the node */
	/* XXX the predictor assumes NO extended attributes in the node */
	if (sz < context.sector_size - UDF_EXTFENTRY_SIZE - 16)
		return 0;

	return UDF_ROUNDUP(sz, context.sector_size) / context.sector_size;
}


static void
udf_prepare_fids(struct long_ad *dir_icb, struct long_ad *dirdata_icb,
		uint8_t *dirdata, uint32_t dirdata_size)
{
	struct fileid_desc *fid;
	struct long_ad     *icb;
	uint32_t fidsize, offset;
	uint32_t location;

	if (udf_datablocks(dirdata_size) == 0) {
		/* going internal */
		icb = dir_icb;
	} else {
		/* external blocks to write to */
		icb = dirdata_icb;
	}

	for (offset = 0; offset < dirdata_size; offset += fidsize) {
		/* for each FID: */
		fid = (struct fileid_desc *) (dirdata + offset);
		assert(udf_rw16(fid->tag.id) == TAGID_FID);

		location  = udf_rw32(icb->loc.lb_num);
		location += offset / context.sector_size;

		fid->tag.tag_loc = udf_rw32(location);
		udf_validate_tag_and_crc_sums((union dscrptr *) fid);

		fidsize = udf_fidsize(fid);
	}
}


static int
udf_file_inject_blob(union dscrptr *dscr,  uint8_t *blob, off_t size)
{
	struct icb_tag *icb;
	struct file_entry *fe;
	struct extfile_entry *efe;
	uint64_t inf_len, obj_size;
	uint32_t l_ea, l_ad;
	uint16_t crclen;
	uint8_t *data, *pos;

	fe = NULL;
	efe = NULL;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe        = &dscr->fe;
		data      = fe->data;
		l_ea      = udf_rw32(fe->l_ea);
		l_ad      = udf_rw32(fe->l_ad);
		icb       = &fe->icbtag;
		inf_len   = udf_rw64(fe->inf_len);
		obj_size  = 0;
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe       = &dscr->efe;
		data      = efe->data;
		l_ea      = udf_rw32(efe->l_ea);
		l_ad      = udf_rw32(efe->l_ad);
		icb       = &efe->icbtag;
		inf_len   = udf_rw64(efe->inf_len);
		obj_size  = udf_rw64(efe->obj_size);
	} else {
		errx(1, "bad tag passed to udf_file_inject_blob");
	}
	crclen = udf_rw16(dscr->tag.desc_crc_len);

	/* check if we can go internal */
	if ((udf_rw16(icb->flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK) !=
			UDF_ICB_INTERN_ALLOC)
		return 1;

	/* check if it will fit internally */
	if (udf_datablocks(size)) {
		/* the predictor tells it won't fit internally */
		return 1;
	}

	/* going internal */
	assert((udf_rw16(icb->flags) & UDF_ICB_TAG_FLAGS_ALLOC_MASK) ==
			UDF_ICB_INTERN_ALLOC);
	assert(l_ad == 0);

	pos = data + l_ea + l_ad;
	memcpy(pos, blob, size);
	l_ad   += size;
	crclen += size;

	inf_len  += size;
	obj_size += size;

	if (fe) {
		fe->l_ad = udf_rw32(l_ad);
		fe->inf_len = udf_rw64(inf_len);
	} else if (efe) {
		efe->l_ad = udf_rw32(l_ad);
		efe->inf_len  = udf_rw64(inf_len);
		efe->obj_size = udf_rw64(inf_len);
	}

	/* make sure the header sums stays correct */
	dscr->tag.desc_crc_len = udf_rw16(crclen);
	udf_validate_tag_and_crc_sums(dscr);

	(void) obj_size;
	return 0;
}


/* XXX no sparse file support */
static void
udf_append_file_mapping(union dscrptr *dscr, struct long_ad *piece)
{
	struct icb_tag *icb;
	struct file_entry *fe;
	struct extfile_entry *efe;
	struct long_ad *last_long, last_piece;
	struct short_ad *last_short, new_short;
	uint64_t inf_len, obj_size, logblks_rec;
	uint32_t l_ea, l_ad, size;
	uint32_t last_lb_num, piece_lb_num;
	uint64_t last_len, piece_len, last_flags;
	uint64_t rest_len, merge_len, last_end;
	uint16_t last_part_num, piece_part_num;
	uint16_t crclen, cur_alloc;
	uint8_t *data, *pos;
	const int short_len = sizeof(struct short_ad);
	const int long_len  = sizeof(struct long_ad);
	const int sector_size = context.sector_size;
	uint64_t max_len = UDF_ROUNDDOWN(UDF_EXT_MAXLEN, sector_size);
	int use_shorts;

	fe  = NULL;
	efe = NULL;
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe          = &dscr->fe;
		data        = fe->data;
		l_ea        = udf_rw32(fe->l_ea);
		l_ad        = udf_rw32(fe->l_ad);
		icb         = &fe->icbtag;
		inf_len     = udf_rw64(fe->inf_len);
		logblks_rec = udf_rw64(fe->logblks_rec);
		obj_size = 0;
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe         = &dscr->efe;
		data        = efe->data;
		l_ea        = udf_rw32(efe->l_ea);
		l_ad        = udf_rw32(efe->l_ad);
		icb         = &efe->icbtag;
		inf_len     = udf_rw64(efe->inf_len);
		obj_size    = udf_rw64(efe->obj_size);
		logblks_rec = udf_rw64(efe->logblks_rec);
	} else {
		errx(1, "bad tag passed to udf_file_append_blob");
	}
	crclen = udf_rw16(dscr->tag.desc_crc_len);

	/* we use shorts if referring inside the metadata partition */
	use_shorts = (udf_rw16(piece->loc.part_num) == context.metadata_part);

	pos = data + l_ea;
	cur_alloc = udf_rw16(icb->flags);
	size = UDF_EXT_LEN(udf_rw32(piece->len));

	/* extract last entry as a long_ad */
	memset(&last_piece, 0, sizeof(last_piece));
	last_len      = 0;
	last_lb_num   = 0;
	last_part_num = 0;
	last_flags    = 0;
	last_short    = NULL;
	last_long     = NULL;
	if (l_ad != 0) {
		if (use_shorts) {
			assert(cur_alloc == UDF_ICB_SHORT_ALLOC);
			pos += l_ad - short_len;
			last_short   = (struct short_ad *) pos;
			last_lb_num  = udf_rw32(last_short->lb_num);
			last_part_num = udf_rw16(piece->loc.part_num);
			last_len     = UDF_EXT_LEN(udf_rw32(last_short->len));
			last_flags   = UDF_EXT_FLAGS(udf_rw32(last_short->len));
		} else {
			assert(cur_alloc == UDF_ICB_LONG_ALLOC);
			pos += l_ad - long_len;
			last_long    = (struct long_ad *) pos;
			last_lb_num  = udf_rw32(last_long->loc.lb_num);
			last_part_num = udf_rw16(last_long->loc.part_num);
			last_len     = UDF_EXT_LEN(udf_rw32(last_long->len));
			last_flags   = UDF_EXT_FLAGS(udf_rw32(last_long->len));
		}
	}

	piece_len      = UDF_EXT_LEN(udf_rw32(piece->len));
	piece_lb_num   = udf_rw32(piece->loc.lb_num);
	piece_part_num = udf_rw16(piece->loc.part_num);

	/* try merging */
	rest_len  = max_len - last_len;

	merge_len = MIN(piece_len, rest_len);
	last_end  = last_lb_num + (last_len / sector_size);
	if ((piece_lb_num == last_end) && (last_part_num == piece_part_num)) {
		/* we can merge */
		last_len  += merge_len;
		piece_len -= merge_len;

		/* write back merge result */
		if (use_shorts) {
			last_short->len = udf_rw32(last_len | last_flags);
		} else {
			last_long->len  = udf_rw32(last_len | last_flags);
		}
		piece_lb_num += merge_len / sector_size;
	}

	if (piece_len) {
		/* append new entry */
		pos = data + l_ea + l_ad;
		if (use_shorts) {
			icb->flags = udf_rw16(UDF_ICB_SHORT_ALLOC);
			memset(&new_short, 0, short_len);
			new_short.len    = udf_rw32(piece_len);
			new_short.lb_num = udf_rw32(piece_lb_num);
			memcpy(pos, &new_short, short_len);
			l_ad += short_len;
			crclen += short_len;
		} else {
			icb->flags = udf_rw16(UDF_ICB_LONG_ALLOC);
			piece->len        = udf_rw32(piece_len);
			piece->loc.lb_num = udf_rw32(piece_lb_num);
			memcpy(pos, piece, long_len);
			l_ad += long_len;
			crclen += long_len;
		}
	}
	piece->len = udf_rw32(0);

	inf_len  += size;
	obj_size += size;
	logblks_rec += UDF_ROUNDUP(size, sector_size) / sector_size;

	dscr->tag.desc_crc_len = udf_rw16(crclen);
	if (udf_rw16(dscr->tag.id) == TAGID_FENTRY) {
		fe->l_ad = udf_rw32(l_ad);
		fe->inf_len = udf_rw64(inf_len);
		fe->logblks_rec = udf_rw64(logblks_rec);
	} else if (udf_rw16(dscr->tag.id) == TAGID_EXTFENTRY) {
		efe->l_ad = udf_rw32(l_ad);
		efe->inf_len  = udf_rw64(inf_len);
		efe->obj_size = udf_rw64(obj_size);
		efe->logblks_rec = udf_rw64(logblks_rec);
	}
}


static int
udf_append_file_contents(union dscrptr *dscr, struct long_ad *data_icb,
		uint8_t *fdata, off_t flen)
{
	struct long_ad icb;
	uint32_t location;
	uint16_t vpart;
	int sectors;

	if (udf_file_inject_blob(dscr, fdata, flen) == 0)
		return 0;

	/* has to be appended in mappings */
	icb = *data_icb;
	icb.len = udf_rw32(flen);
	while (udf_rw32(icb.len) > 0)
		udf_append_file_mapping(dscr, &icb);
	udf_validate_tag_and_crc_sums(dscr);

	/* write out data piece */
	vpart    = udf_rw16(data_icb->loc.part_num);
	location = udf_rw32(data_icb->loc.lb_num);
	sectors  = udf_datablocks(flen);

	return udf_write_virt(fdata, location, vpart, sectors);
}


static int
udf_create_new_file(struct stat *st, union dscrptr **dscr,
	int filetype, struct long_ad *icb)
{
	struct file_entry *fe;
	struct extfile_entry *efe;
	int error;

	fe = NULL;
	efe = NULL;
	if (context.dscrver == 2) {
		error = udf_create_new_fe(&fe, filetype, st);
		if (error)
			errx(error, "can't create fe");
		*dscr = (union dscrptr *) fe;
		icb->longad_uniqueid = udf_rw32(udf_rw64(fe->unique_id));
	} else {
		error = udf_create_new_efe(&efe, filetype, st);
		if (error)
			errx(error, "can't create fe");
		*dscr = (union dscrptr *) efe;
		icb->longad_uniqueid = udf_rw32(udf_rw64(efe->unique_id));
	}

	return 0;
}


static void
udf_estimate_walk(fsinfo_t *fsopts,
		fsnode *root, char *dir, struct udf_stats *stats)
{
	struct fileid_desc *fid;
	struct long_ad dummy_ref;
	fsnode *cur;
	fsinode *fnode;
	size_t pathlen = strlen(dir);
	char *mydir = dir + pathlen;
	off_t sz;
	uint32_t nblk, ddoff;
	uint32_t softlink_len;
	uint8_t *softlink_buf;
	int nentries;
	int error;

	stats->ndirs++;

	/*
	 * Count number of directory entries and count directory size; needed
	 * for the reservation of enough space for the directory. Pity we
	 * don't keep the FIDs we created. If it turns out to be a issue we
	 * can cache it later.
	 */
	fid = (struct fileid_desc *) malloc(context.sector_size);
	assert(fid);

	ddoff = 40;	/* '..' entry */
	for (cur = root, nentries = 0; cur != NULL; cur = cur->next) {
		switch (cur->type & S_IFMT) {
		default:
			/* what kind of nodes? */
			break;
		case S_IFCHR:
		case S_IFBLK:
			/* not supported yet */
			break;
		case S_IFDIR:
			if (strcmp(cur->name, ".") == 0)
				continue;
			/* FALLTHROUGH */
		case S_IFLNK:
		case S_IFREG:
			/* create dummy FID to see how long name will become */
			memset(&dummy_ref, 0, sizeof(dummy_ref));
			udf_create_fid(ddoff, fid, cur->name, 0, &dummy_ref);
			nentries++;
			ddoff += udf_fidsize(fid);
			break;
		}
	}
	sz = ddoff;

	root->inode->st.st_size = sz;	/* max now */
	root->inode->flags |= FI_SIZED;

	nblk = udf_datablocks(sz);
	stats->nmetadatablocks += nblk;

	/* for each entry in the directory, there needs to be a (E)FE */
	stats->nmetadatablocks += nentries + 1;

	/* recurse */
	for (cur = root; cur != NULL; cur = cur->next) {
		switch (cur->type & S_IFMT) {
		default:
			/* what kind of nodes? */
			break;
		case S_IFDIR:
			if (strcmp(cur->name, ".") == 0)
				continue;
			/* empty dir? */
			if (!cur->child)
				break;
			mydir[0] = '/';
			strncpy(&mydir[1], cur->name, MAXPATHLEN - pathlen);
			udf_estimate_walk(fsopts, cur->child, dir, stats);
			mydir[0] = '\0';
			break;
		case S_IFCHR:
		case S_IFBLK:
			/* not supported yet */
			// stats->nfiles++;
			break;
		case S_IFREG:
			fnode = cur->inode;
			/* don't double-count hard-links */
			if (!(fnode->flags & FI_SIZED)) {
				sz = fnode->st.st_size;
				nblk = udf_datablocks(sz);
				stats->ndatablocks += nblk;
				/* ... */
				fnode->flags |= FI_SIZED;
			}
			stats->nfiles++;
			break;
		case S_IFLNK:
			/* softlink */
			fnode = cur->inode;
			/* don't double-count hard-links */
			if (!(fnode->flags & FI_SIZED)) {
				error = udf_encode_symlink(&softlink_buf,
						&softlink_len, cur->symlink);
				if (error) {
					printf("SOFTLINK error %d\n", error);
					break;
				}
				nblk = udf_datablocks(softlink_len);
				stats->ndatablocks += nblk;
				fnode->flags |= FI_SIZED;

				free(softlink_buf);
			}
			stats->nfiles++;
			break;
		}
	}
}


#define UDF_MAX_CHUNK_SIZE (4*1024*1024)
static int
udf_copy_file(struct stat *st, char *path, fsnode *cur, struct fileid_desc *fid,
	struct long_ad *icb)
{
	union dscrptr *dscr;
	struct long_ad data_icb;
	fsinode *fnode;
	off_t sz, chunk, rd;
	uint8_t *data;
	bool intern;
	int nblk;
	int f;
	int error;

	fnode = cur->inode;

	f = open(path, O_RDONLY);
	if (f < 0) {
		warn("Can't open file %s for reading", cur->name);
		return errno;
	}

	/* claim disc space for the (e)fe descriptor for this file */
	udf_metadata_alloc(1, icb);
	udf_create_new_file(st, &dscr, UDF_ICB_FILETYPE_RANDOMACCESS, icb);

	sz = fnode->st.st_size;

	chunk = MIN(sz, UDF_MAX_CHUNK_SIZE);
	data = malloc(MAX(chunk, context.sector_size));
	assert(data);

	intern = (udf_datablocks(chunk) == 0);
	error = 0;
	while (chunk) {
		rd = read(f, data, chunk);
		if (rd != chunk) {
			warn("Short read of file %s", cur->name);
			error = errno;
			break;
		}

		nblk = UDF_ROUNDUP(chunk, context.sector_size) / context.sector_size;
		if (chunk && !intern)
			udf_data_alloc(nblk, &data_icb);
		udf_append_file_contents(dscr, &data_icb, data, chunk);

		sz -= chunk;
		chunk = MIN(sz, UDF_MAX_CHUNK_SIZE);
	}
	close(f);
	free(data);

	/* write out dscr (e)fe */
	udf_set_link_cnt(dscr, fnode->nlink);
	udf_write_dscr_virt(dscr, udf_rw32(icb->loc.lb_num),
		udf_rw16(icb->loc.part_num), 1);
	free(dscr);

	/* remember our location for hardlinks */
	cur->inode->fsuse = malloc(sizeof(struct long_ad));
	memcpy(cur->inode->fsuse, icb, sizeof(struct long_ad));

	return error;
}


static int
udf_populate_walk(fsinfo_t *fsopts, fsnode *root, char *dir,
		struct long_ad *parent_icb, struct long_ad *dir_icb)
{
	union dscrptr *dir_dscr, *dscr;
	struct fileid_desc *fid;
	struct long_ad icb, data_icb, dirdata_icb;
	fsnode *cur;
	fsinode *fnode;
	size_t pathlen = strlen(dir);
	size_t dirlen;
	char *mydir = dir + pathlen;
	uint32_t nblk, ddoff;
	uint32_t softlink_len;
	uint8_t *softlink_buf;
	uint8_t *dirdata;
	int error, ret, retval;

	/* claim disc space for the (e)fe descriptor for this dir */
	udf_metadata_alloc(1, dir_icb);

	/* create new e(fe) */
	udf_create_new_file(&root->inode->st, &dir_dscr,
		UDF_ICB_FILETYPE_DIRECTORY, dir_icb);

	/* allocate memory for the directory contents */
	dirlen = root->inode->st.st_size;
	nblk = UDF_ROUNDUP(dirlen, context.sector_size) / context.sector_size;
	dirdata = malloc(nblk * context.sector_size);
	assert(dirdata);
	memset(dirdata, 0, nblk * context.sector_size);

	/* create and append '..' */
	fid = (struct fileid_desc *) dirdata;
	ddoff = udf_create_parentfid(fid, parent_icb);
	assert(ddoff == 40);

	/* for '..' */
	udf_inc_link(dir_dscr);

	/* recurse */
	retval = 0;
	for (cur = root; cur != NULL; cur = cur->next) {
		mydir[0] = '/';
		strncpy(&mydir[1], cur->name, MAXPATHLEN - pathlen);

		fid = (struct fileid_desc *) (dirdata + ddoff);
		switch (cur->type & S_IFMT) {
		default:
			/* what kind of nodes? */
			retval = 2;
			break;
		case S_IFCHR:
		case S_IFBLK:
			/* not supported */
			retval = 2;
			warnx("device node %s not supported", dir);
			break;
		case S_IFDIR:
			/* not an empty dir? */
			if (strcmp(cur->name, ".") == 0)
				break;
			assert(cur->child);
			if (cur->child) {
				ret = udf_populate_walk(fsopts, cur->child,
					dir, dir_icb, &icb);
				if (ret)
					retval = 2;
			}
			udf_create_fid(ddoff, fid, cur->name,
					UDF_FILE_CHAR_DIR, &icb);
			udf_inc_link(dir_dscr);
			ddoff += udf_fidsize(fid);
			break;
		case S_IFREG:
			fnode = cur->inode;
			/* don't re-copy hard-links */
			if (!(fnode->flags & FI_WRITTEN)) {
				printf("%s\n", dir);
				error = udf_copy_file(&fnode->st, dir, cur,
					fid, &icb);
				if (!error) {
					fnode->flags |= FI_WRITTEN;
					udf_create_fid(ddoff, fid, cur->name,
						0, &icb);
					ddoff += udf_fidsize(fid);
				} else {
					retval = 2;
				}
			} else {
				/* hardlink! */
				printf("%s (hardlink)\n", dir);
				udf_create_fid(ddoff, fid, cur->name,
					0, (struct long_ad *) (fnode->fsuse));
				ddoff += udf_fidsize(fid);
			}
			fnode->nlink--;
			if (fnode->nlink == 0)
				free(fnode->fsuse);
			break;
		case S_IFLNK:
			/* softlink */
			fnode = cur->inode;
			printf("%s -> %s\n", dir, cur->symlink);
			error = udf_encode_symlink(&softlink_buf,
					&softlink_len, cur->symlink);
			if (error) {
				printf("SOFTLINK error %d\n", error);
				retval = 2;
				break;
			}

			udf_metadata_alloc(1, &icb);
			udf_create_new_file(&fnode->st, &dscr,
				UDF_ICB_FILETYPE_SYMLINK, &icb);

			nblk = udf_datablocks(softlink_len);
			if (nblk > 0)
				udf_data_alloc(nblk, &data_icb);
			udf_append_file_contents(dscr, &data_icb,
					softlink_buf, softlink_len);

			/* write out dscr (e)fe */
			udf_inc_link(dscr);
			udf_write_dscr_virt(dscr, udf_rw32(icb.loc.lb_num),
				udf_rw16(icb.loc.part_num), 1);

			free(dscr);
			free(softlink_buf);

			udf_create_fid(ddoff, fid, cur->name, 0, &icb);
			ddoff += udf_fidsize(fid);
			break;
		}
		mydir[0] = '\0';
	}
	assert(dirlen == ddoff);

	/* pre allocate space for the directory contents */
	memset(&dirdata_icb, 0, sizeof(dirdata_icb));
	nblk = udf_datablocks(dirlen);

	/* claim disc space for the dir contents if needed */
	if (nblk > 0)
		udf_fids_alloc(nblk, &dirdata_icb);

	udf_prepare_fids(dir_icb, &dirdata_icb, dirdata, dirlen);
	udf_append_file_contents(dir_dscr, &dirdata_icb, dirdata, dirlen);

	/* write out dir_dscr (e)fe */
	udf_write_dscr_virt(dir_dscr, udf_rw32(dir_icb->loc.lb_num),
			udf_rw16(dir_icb->loc.part_num), 1);

	free(dirdata);
	free(dir_dscr);
	return retval;
}


static int
udf_populate(const char *dir, fsnode *root, fsinfo_t *fsopts,
		struct udf_stats *stats)
{
	struct long_ad rooticb;
	static char path[MAXPATHLEN+1];
	int error;

	strncpy(path, dir, sizeof(path));
	error = udf_populate_walk(fsopts, root, path, &rooticb, &rooticb);

	return error;
}


static void
udf_enumerate_and_estimate(const char *dir, fsnode *root, fsinfo_t *fsopts,
		struct udf_stats *stats)
{
	char path[MAXPATHLEN + 1];
	off_t proposed_size;
	uint32_t n, nblk, nmetablk, nbytes;
	uint32_t spareable_blocks, spareable_blockingnr;

	strncpy(path, dir, sizeof(path));

	/* calculate strict minimal size */
	udf_estimate_walk(fsopts, root, path, stats);
#if 0
	printf("ndirs            %d\n", stats->ndirs);
	printf("nfiles           %d\n", stats->nfiles);
	printf("ndata_blocks     %d\n", stats->ndatablocks);
	printf("nmetadata_blocks %d\n", stats->nmetadatablocks);
	printf("\n");
#endif

	/* adjust for options : free file nodes */
	if (fsopts->freefiles) {
		/* be mercifull and reserve more for the FID */
		stats->nmetadatablocks += fsopts->freefiles * 1.5;
	} else if ((n = fsopts->freefilepc)) {
		stats->nmetadatablocks += (stats->nmetadatablocks*n) / (100-n);
	}

	/* adjust for options : free data blocks */
	if (fsopts->freeblocks) {
		stats->ndatablocks += fsopts->freeblocks;
	} else if ((n = fsopts->freeblockpc)) {
		stats->ndatablocks += (stats->ndatablocks * n) / (100-n);
	}

	/* rough predictor of minimum disc size */
	nblk  = stats->ndatablocks + stats->nmetadatablocks;
	if (context.format_flags & FORMAT_META) {
		float meta_p;
		double factor;

		meta_p = (float) context.meta_perc/100.0;
		factor = meta_p / (1.0 - meta_p);

		/* add space for metadata partition including some slack */
		nmetablk = factor * nblk + 32;
		nblk =  stats->ndatablocks + nmetablk;

		/* free space maps */
		nbytes = ceil((double) nblk * (1.0/8.0));
		nblk += 1 + (nbytes + context.sector_size-1)/context.sector_size;
		if (!(context.format_flags & FORMAT_READONLY)) {
			nbytes = ceil((double) nmetablk * (1.0/8.0));
			nblk += 1 + (nbytes + context.sector_size-1)/context.sector_size;
		}
	} else if (context.format_flags & FORMAT_SEQUENTIAL) {
		/* nothing */
	} else {
		if (!(context.format_flags & FORMAT_READONLY)) {
			nbytes = ceil((double) nblk * (1.0/8.0));
			nblk += 1 + (nbytes + context.sector_size-1)/
				context.sector_size;
		}
	}

	/*
	 * Make extra room for spareable table if requested
	 */
	if (context.format_flags & FORMAT_SPAREABLE) {
		spareable_blockingnr = udf_spareable_blockingnr();
		spareable_blocks     = udf_spareable_blocks();

		nblk += spareable_blocks * spareable_blockingnr;
		nblk += spareable_blockingnr;		/* slack */
	}

	nblk += 256;					/* pre-volume space  */
	nblk += 256;					/* post-volume space */
	nblk += 1024;					/* safeguard	     */

	/* try to honour minimum size */
	n = fsopts->minsize / fsopts->sectorsize;
	if (nblk < n) {
		stats->ndatablocks += (n - nblk);
		nblk += n - nblk;
	}

	/* keep proposed size a multiple of blockingnr for image creation */
	if (S_ISREG(dev_fd_stat.st_mode)) {
		struct mmc_trackinfo ti;
		int blockingnr;
		int error;

		/* adjust proposed size to be a multiple of the blockingnr */
		udf_update_discinfo();
		ti.tracknr = mmc_discinfo.first_track_last_session;
		error = udf_update_trackinfo(&ti);
		assert(!error);
		blockingnr = udf_get_blockingnr(&ti);
		nblk = UDF_ROUNDUP(nblk, blockingnr);
	}

	proposed_size = (off_t) nblk * fsopts->sectorsize;

	/* sanity size */
	if (proposed_size < 512*1024)
		proposed_size = 512*1024;

	if (fsopts->size) {
		if (fsopts->size < proposed_size)
			errx(1, "makefs_udf: won't fit on disc!");
	} else {
		fsopts->size = proposed_size;
	}

	fsopts->inodes = stats->nfiles + stats->ndirs;
}


void
udf_makefs(const char *image, const char *dir, fsnode *root, fsinfo_t *fsopts)
{
	struct udf_stats stats;
	uint64_t truncate_len;
	uint32_t last_sector, ext;
	char scrap[255];
	int error;

	/* setup */
	emul_sectorsize = fsopts->sectorsize;
	emul_size = 0;
	context.sector_size = fsopts->sectorsize;

	/* names */
	error = udf_proces_names();
	if (error)
		errx(1, "bad names given");

	/* open disc device or emulated file */
	if (udf_opendisc(image, O_CREAT)) {
		udf_closedisc();
		errx(1, "can't open %s", image);
	}
	fsopts->fd = dev_fd;

	/* determine format */
	if (udf_readonly_format())
		req_enable |= FORMAT_READONLY;
	// printf("req_enable %d, req_disable %d\n", req_enable, req_disable);
	error = udf_derive_format(req_enable, req_disable);
	if (error) {
		udf_closedisc();
		errx(1, "can't derive format from media/settings");
	}

	/* estimate the amount of space needed */
	memset(&stats, 0, sizeof(stats));
	udf_enumerate_and_estimate(dir, root, fsopts, &stats);

	printf("Calculated size of `%s' is "
		"%"PRIu64" KiB, %"PRIu64" MiB, %"PRIu64" GiB with %ld inodes\n",
		image,
		(uint64_t) fsopts->size/1024,
		(uint64_t) fsopts->size/1024/1024,
		(uint64_t) fsopts->size/1024/1024/1024,
		(long)fsopts->inodes);
	emul_size = MAX(emul_size, fsopts->size);
	if ((fsopts->maxsize > 0) && (emul_size > fsopts->maxsize))
		errx(1, "won't fit due to set maximum disk size");

	/* prepare disc if necessary (recordables mainly) */
	error = udf_prepare_disc();
	if (error) {
		udf_closedisc();
		errx(1, "preparing disc failed");
	}

	/* update mmc info but now with correct size */
	udf_update_discinfo();
	udf_dump_discinfo(&mmc_discinfo);

	printf("Building disc compatible with UDF version %x to %x\n\n",
		context.min_udf, context.max_udf);
	(void)snprintb(scrap, sizeof(scrap), FORMAT_FLAGBITS,
	    (uint64_t) context.format_flags);
	printf("UDF properties       %s\n", scrap);
	printf("Volume set          `%s'\n", context.volset_name);
	printf("Primary volume      `%s`\n", context.primary_name);
	printf("Logical volume      `%s`\n", context.logvol_name);
	if (context.format_flags & FORMAT_META)
		printf("Metadata percentage  %d%% (%d%% used)\n",
			context.meta_perc,
			(int) ceil(100.0*stats.nmetadatablocks/stats.ndatablocks));
	printf("\n");

	/* prefix */
	udf_allow_writing();
	if (udf_do_newfs_prefix()) {
		udf_closedisc();
		errx(1, "basic setup failed");
	}

	/* update context */
	context.unique_id = 0;

	/* add all directories */
	error = udf_populate(dir, root, fsopts, &stats);

	if (!error) {
		/* update values for integrity sequence */
		context.num_files = stats.nfiles;
		context.num_directories = stats.ndirs;

		udf_do_newfs_postfix();

		if (S_ISREG(dev_fd_stat.st_mode) &&
				(context.format_flags & FORMAT_VAT)) {
			udf_translate_vtop(context.alloc_pos[context.data_part],
				context.data_part,
				&last_sector, &ext);
			truncate_len = (uint64_t) last_sector * context.sector_size;

			printf("\nTruncing the disc-image to allow for VAT\n");
			printf("Free space left on this volume approx. "
				"%"PRIu64" KiB, %"PRIu64" MiB\n",
				(fsopts->size - truncate_len)/1024,
				(fsopts->size - truncate_len)/1024/1024);
			ftruncate(dev_fd, truncate_len);
		}
	}
	udf_closedisc();

	if (error == 2)
		errx(error, "not all files could be added");
	if (error == 1)
		errx(error, "creation of %s failed", image);
	return;
}
