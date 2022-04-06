/* $NetBSD: udf_core.h,v 1.1 2022/04/06 13:29:15 reinoud Exp $ */

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

#ifndef _FS_UDF_CORE_H_
#define _FS_UDF_CORE_H_


#if 0
# ifndef DEBUG
#   define DEBUG
#  endif
#endif


#include <sys/types.h>
#include <sys/stat.h>
#include "udf_bswap.h"
#include "udf_osta.h"

#if !HAVE_NBTOOL_CONFIG_H
#define _EXPOSE_MMC
#include <sys/cdio.h>
#include <fs/udf/ecma167-udf.h>
#else
#include "udf/cdio_mmc_structs.h"
#include "../../sys/fs/udf/ecma167-udf.h"
#endif


/* format flags indicating properties of disc to create */
#define FORMAT_WRITEONCE	0x00001
#define FORMAT_SEQUENTIAL	0x00002
#define FORMAT_REWRITABLE	0x00004
#define FORMAT_SPAREABLE	0x00008
#define FORMAT_META		0x00010
#define FORMAT_LOW		0x00020
#define FORMAT_VAT		0x00040
#define FORMAT_WORM		0x00080
#define FORMAT_TRACK512		0x00100
#define FORMAT_INVALID		0x00200
#define FORMAT_READONLY		0x00400
#define FORMAT_FLAGBITS \
    "\10\1WRITEONCE\2SEQUENTIAL\3REWRITABLE\4SPAREABLE\5META\6LOW" \
    "\7VAT\10WORM\11TRACK512\12INVALID\13READONLY"

/* writing strategy */
#define UDF_WRITE_SEQUENTIAL	1
#define UDF_WRITE_PACKET	2	/* with fill-in if needed */
#define UDF_MAX_QUEUELEN	400	/* must hold all pre-partition space */

/* structure space */
#define UDF_ANCHORS		4	/* 256, 512, N-256, N */
#define UDF_PARTITIONS		4	/* overkill */
#define UDF_PMAPS		4	/* overkill */

/* misc constants */
#define UDF_MAX_NAMELEN		255	/* as per SPEC */
#define UDF_LVDINT_SEGMENTS	10	/* big overkill */
#define UDF_LVINT_LOSSAGE	4	/* lose 2 openings */
#define UDF_MAX_ALLOC_EXTENTS	5	/* overkill */

/* translation constants */
#define UDF_VTOP_RAWPART UDF_PMAPS	/* [0..UDF_PMAPS> are normal     */

/* virtual to physical mapping types */
#define UDF_VTOP_TYPE_RAW            0
#define UDF_VTOP_TYPE_UNKNOWN        0
#define UDF_VTOP_TYPE_PHYS           1
#define UDF_VTOP_TYPE_VIRT           2
#define UDF_VTOP_TYPE_SPAREABLE      3
#define UDF_VTOP_TYPE_META           4

#define UDF_TRANS_ZERO		((uint64_t) -1)
#define UDF_TRANS_UNMAPPED	((uint64_t) -2)
#define UDF_TRANS_INTERN	((uint64_t) -3)
#define UDF_MAX_SECTOR		((uint64_t) -10)	/* high water mark */

/* handys */
#define UDF_ROUNDUP(val, gran) \
	((uint64_t) (gran) * (((uint64_t)(val) + (gran)-1) / (gran)))

#define UDF_ROUNDDOWN(val, gran) \
	((uint64_t) (gran) * (((uint64_t)(val)) / (gran)))

/* default */
#define UDF_META_PERC  20	/* picked */


/* disc offsets for various structures and their sizes */
struct udf_disclayout {
	uint32_t wrtrack_skew;

	uint32_t iso9660_vrs;
	uint32_t anchors[UDF_ANCHORS];
	uint32_t vds1_size, vds2_size, vds1, vds2;
	uint32_t lvis_size, lvis;

	uint32_t first_lba, last_lba;
	uint32_t blockingnr, align_blockingnr, spareable_blockingnr;
	uint32_t meta_blockingnr, meta_alignment;

	/* spareables */
	uint32_t spareable_blocks;
	uint32_t spareable_area, spareable_area_size;
	uint32_t sparing_table_dscr_lbas;
	uint32_t spt_1, spt_2;

	/* metadata partition */
	uint32_t meta_file, meta_mirror, meta_bitmap;
	uint32_t meta_part_start_lba, meta_part_size_lba;
	uint32_t meta_bitmap_dscr_size;
	uint32_t meta_bitmap_space;

	/* main partition */
	uint32_t part_start_lba, part_size_lba;
	uint32_t alloc_bitmap_dscr_size;
	uint32_t unalloc_space, freed_space;

	/* main structures */
	uint32_t fsd, rootdir, vat;

};


struct udf_lvintq {
	uint32_t		start;
	uint32_t		end;
	uint32_t		pos;
	uint32_t		wpos;
};


/* all info about discs and descriptors building */
struct udf_create_context {
	/* descriptors */
	int	 dscrver;		/* 2 or 3          */
	int	 min_udf;		/* hex             */
	int	 max_udf;		/* hex             */
	int	 serialnum;		/* format serialno */

	int	 gmtoff;		/* in minutes	             */
	int	 meta_perc;		/* format paramter           */
	int	 check_surface;		/* for spareables            */
	int	 create_new_session;	/* for non empty recordables */

	uint32_t sector_size;
	int	 media_accesstype;
	int	 format_flags;
	int	 write_strategy;

	/* identification */
	char	*logvol_name;
	char	*primary_name;
	char	*volset_name;
	char	*fileset_name;

	char const *app_name;
	char const *impl_name;
	int	 app_version_main;
	int	 app_version_sub;

	/* building */
	int	 vds_seq;	/* for building functions  */

	/* constructed structures */
	struct anchor_vdp	*anchors[UDF_ANCHORS];	/* anchors to VDS    */
	struct pri_vol_desc	*primary_vol;		/* identification    */
	struct logvol_desc	*logical_vol;		/* main mapping v->p */
	struct unalloc_sp_desc	*unallocated;		/* free UDF space    */
	struct impvol_desc	*implementation;	/* likely reduntant  */
	struct logvol_int_desc	*logvol_integrity;	/* current integrity */
	struct part_desc	*partitions[UDF_PARTITIONS]; /* partitions   */

	struct space_bitmap_desc*part_unalloc_bits[UDF_PARTITIONS];
	struct space_bitmap_desc*part_freed_bits  [UDF_PARTITIONS];

	/* track information */
	struct mmc_trackinfo	 first_ti_partition;
	struct mmc_trackinfo	 first_ti;
	struct mmc_trackinfo	 last_ti;

	/* current partitions for allocation */
	int	data_part;
	int	metadata_part;
	int	fids_part;

	/* current highest file unique_id */
	uint64_t unique_id;

	/* block numbers as offset in partition, building ONLY! */
	uint32_t alloc_pos[UDF_PARTITIONS];

	/* derived; points *into* other structures */
	struct udf_logvol_info	*logvol_info;		/* inside integrity  */

	/* fileset and root directories */
	struct fileset_desc	*fileset_desc;		/* normally one      */

	/* logical to physical translations */
	int 			 vtop[UDF_PMAPS+1];	/* vpartnr trans     */
	int			 vtop_tp[UDF_PMAPS+1];	/* type of trans     */

	/* spareable */
	struct udf_sparing_table*sparing_table;		/* replacements      */

	/* VAT file */
	uint32_t		 vat_size;		/* length */
	uint32_t		 vat_allocated;		/* allocated length */
	uint32_t		 vat_start;		/* offset 1st entry */
	uint8_t			*vat_contents;		/* the VAT */

	/* meta data partition */
	struct extfile_entry	*meta_file;
	struct extfile_entry	*meta_mirror;
	struct extfile_entry	*meta_bitmap;

	/* lvint */
	uint32_t	 	 num_files;
	uint32_t		 num_directories;
	uint32_t		 part_size[UDF_PARTITIONS];
	uint32_t		 part_free[UDF_PARTITIONS];

	/* fsck */
	union dscrptr		*vds_buf;
	int			 vds_size;
	struct udf_lvintq	 lvint_trace[UDF_LVDINT_SEGMENTS]; /* fsck   */
	uint8_t			*lvint_history;			   /* fsck   */
	int			 lvint_history_len;		   /* fsck   */
	int			 lvint_history_wpos;		   /* fsck   */
	int			 lvint_history_ondisc_len;	   /* fsck   */
};


/* global variables describing disc and format */
extern struct udf_create_context context;
extern struct udf_disclayout     layout;
extern struct mmc_discinfo mmc_discinfo;  /* device: disc info		   */

extern int		dev_fd_rdonly;	  /* device: open readonly!	   */
extern int	 	dev_fd;		  /* device: file descriptor	   */
extern struct stat	dev_fd_stat;	  /* device: last stat info	   */
extern char	       *dev_name;	  /* device: name		   */
extern int	 	emul_mmc_profile; /* for files			   */
extern int		emul_packetsize;  /* for discs and files	   */
extern int		emul_sectorsize;  /* for files		    	   */
extern off_t		emul_size;	  /* for files			   */
extern uint32_t		wrtrack_skew;	  /* offset for write sector0	   */


/* prototypes */
extern void udf_init_create_context(void);
extern int a_udf_version(const char *s, const char *id_type);
extern int is_zero(void *blob, int size);
extern uint32_t udf_bytes_to_sectors(uint64_t bytes);

extern int udf_calculate_disc_layout(int min_udf,
	uint32_t first_lba, uint32_t last_lba,
	uint32_t sector_size, uint32_t blockingnr);
extern void udf_dump_layout(void);
extern int udf_spareable_blocks(void);
extern int udf_spareable_blockingnr(void);

extern void udf_osta_charset(struct charspec *charspec);
extern void udf_encode_osta_id(char *osta_id, uint16_t len, char *text);
extern void udf_to_unix_name(char *result, int result_len, char *id, int len,
	struct charspec *chsp);
extern void unix_to_udf_name(char *result, uint8_t *result_len,
	char const *name, int name_len, struct charspec *chsp);

extern void udf_set_regid(struct regid *regid, char const *name);
extern void udf_add_domain_regid(struct regid *regid);
extern void udf_add_udf_regid(struct regid *regid);
extern void udf_add_impl_regid(struct regid *regid);
extern void udf_add_app_regid(struct regid *regid);

extern int udf_check_tag(void *blob);
extern int udf_check_tag_payload(void *blob, uint32_t max_length);
extern int udf_check_tag_and_location(void *blob, uint32_t location);
extern int udf_validate_tag_sum(union dscrptr *dscr);
extern int udf_validate_tag_and_crc_sums(union dscrptr *dscr);

extern void udf_set_timestamp_now(struct timestamp *timestamp);
extern void udf_timestamp_to_timespec(struct timestamp *timestamp,
	struct timespec *timespec);
extern void udf_timespec_to_timestamp(struct timespec *timespec,
	struct timestamp *timestamp);

extern void udf_inittag(struct desc_tag *tag, int tagid, uint32_t loc);
extern int udf_create_anchor(int num);

extern void udf_create_terminator(union dscrptr *dscr, uint32_t loc);
extern int udf_create_primaryd(void);
extern int udf_create_partitiond(int part_num);
extern int udf_create_unalloc_spaced(void);
extern int udf_create_sparing_tabled(void);
extern int udf_create_space_bitmap(uint32_t dscr_size, uint32_t part_size_lba,
	struct space_bitmap_desc **sbdp);
extern int udf_create_logical_dscr(void);
extern int udf_create_impvold(char *field1, char *field2, char *field3);
extern int udf_create_fsd(void);
extern int udf_create_lvintd(int type);
extern void udf_update_lvintd(int type);
extern uint16_t udf_find_raw_phys(uint16_t raw_phys_part);

extern int udf_register_bad_block(uint32_t location);
extern void udf_mark_allocated(uint32_t start_lb, int partnr, uint32_t blocks);

extern int udf_impl_extattr_check(struct impl_extattr_entry *implext);
extern void udf_calc_impl_extattr_checksum(struct impl_extattr_entry *implext);
extern int udf_extattr_search_intern(union dscrptr *dscr,
	uint32_t sattr, char const *sattrname,
	uint32_t *offsetp, uint32_t *lengthp);

extern int udf_create_new_fe(struct file_entry **fep, int file_type,
	struct stat *st);
extern int udf_create_new_efe(struct extfile_entry **efep, int file_type,
	struct stat *st);

extern int udf_encode_symlink(uint8_t **pathbufp, uint32_t *pathlenp, char *target);

extern void udf_advance_uniqueid(void);
extern uint32_t udf_tagsize(union dscrptr *dscr, uint32_t lb_size);
extern int udf_fidsize(struct fileid_desc *fid);
extern void udf_create_fid(uint32_t diroff, struct fileid_desc *fid,
	char *name, int namelen, struct long_ad *ref);
extern int udf_create_parentfid(struct fileid_desc *fid, struct long_ad *parent);

extern int udf_create_meta_files(void);
extern int udf_create_new_rootdir(union dscrptr **dscr);

extern int udf_create_VAT(union dscrptr **vat_dscr, struct long_ad *vatdata_loc);
extern void udf_prepend_VAT_file(void);
extern void udf_vat_update(uint32_t virt, uint32_t phys);
extern int udf_append_VAT_file(void);
extern int udf_writeout_VAT(void);

extern int udf_opendisc(const char *device, int open_flags);
extern void udf_closedisc(void);
extern int udf_prepare_disc(void);
extern int udf_update_discinfo(void);
extern int udf_update_trackinfo(struct mmc_trackinfo *ti);
extern int udf_get_blockingnr(struct mmc_trackinfo *ti);
extern void udf_synchronise_caches(void);
extern void udf_suspend_writing(void);
extern void udf_allow_writing(void);

extern int udf_write_iso9660_vrs(void);

/* address translation */
extern int udf_translate_vtop(uint32_t lb_num, uint16_t vpart,
	uint32_t *lb_numres, uint32_t *extres);

/* basic sector read/write with caching */
extern int udf_read_sector(void *sector, uint64_t location);
extern int udf_write_sector(void *sector, uint64_t location);

/* extent reading and writing */
extern int udf_read_phys(void *blob, uint32_t location, uint32_t sects);
extern int udf_write_phys(void *blob, uint32_t location, uint32_t sects);

extern int udf_read_virt(void *blob, uint32_t location, uint16_t vpart,
	uint32_t sectors);
extern int udf_write_virt(void *blob, uint32_t location, uint16_t vpart,
	uint32_t sectors);

extern int udf_read_dscr_phys(uint32_t sector, union dscrptr **dstp);
extern int udf_write_dscr_phys(union dscrptr *dscr, uint32_t location,
	uint32_t sects);

extern int udf_read_dscr_virt(uint32_t sector, uint16_t vpart,
	union dscrptr **dstp);
extern int udf_write_dscr_virt(union dscrptr *dscr,
	uint32_t location, uint16_t vpart, uint32_t sects);

extern void udf_metadata_alloc(int nblk, struct long_ad *pos);
extern void udf_data_alloc(int nblk, struct long_ad *pos);
extern void udf_fids_alloc(int nblk, struct long_ad *pos);

extern int udf_derive_format(int req_enable, int req_disable);
extern int udf_proces_names(void);
extern int udf_surface_check(void);

extern int udf_do_newfs_prefix(void);
extern int udf_do_rootdir(void);
extern int udf_do_newfs_postfix(void);

extern void udf_dump_discinfo(struct mmc_discinfo *di);

#endif /* _UDF_CORE_H_ */
