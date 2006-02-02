/* $NetBSD $ */

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


#ifndef _UDF_SUBR_H_
#define _UDF_SUBR_H_

/* device information updating */
int udf_update_trackinfo(struct udf_mount *ump, struct mmc_trackinfo *trackinfo);
int udf_update_discinfo(struct udf_mount *ump);

/* tags and read/write descriptors */
int udf_check_tag(void *blob);
int udf_check_tag_payload(void *blob, uint32_t max_length);
int udf_validate_tag_sum(void *blob);
int udf_validate_tag_and_crc_sums(void *blob);
int udf_tagsize(union dscrptr *dscr, uint32_t udf_sector_size);

int udf_read_descriptor(
		struct udf_mount *ump,
		uint32_t sector,
		struct malloc_type *mtype,		/* where to allocate */
		union dscrptr **dstp);			/* out */


/* volume descriptors readers and checkers */
int udf_read_anchors(struct udf_mount *ump, struct udf_args *args);

int udf_read_vds_space(struct udf_mount *ump);
int udf_process_vds(struct udf_mount *ump, struct udf_args *args);
int udf_read_vds_tables(struct udf_mount *ump, struct udf_args *args);
int udf_read_rootdirs(struct udf_mount *ump, struct udf_args *args);

/* translation services */
int udf_translate_vtop(struct udf_mount *ump, struct long_ad *icb_loc, uint32_t *lb_numres, uint32_t *extres);
int udf_translate_file_extent(struct udf_node *node, uint32_t from, uint32_t pages, uint64_t *map);

/* node readers and writers */
int udf_get_node(struct udf_mount *ump, struct long_ad *icbloc, struct udf_node **noderes);
int udf_dispose_node(struct udf_node *node);
int udf_dispose_locked_node(struct udf_node *node);
void udf_read_filebuf(struct udf_node *node, struct buf *buf);
int udf_read_file_extent(struct udf_node *node, uint32_t from, uint32_t sectors, uint8_t *blob);

/* directory read and parse utils */
void udf_to_unix_name(char *result, char *id, int len, struct charspec *chsp);
void unix_to_udf_name(char *result, char *name, uint8_t *result_len, struct charspec *chsp);
int udf_lookup_name_in_dir(struct vnode *vp, const char *name, int namelen, struct long_ad *icb_loc);
int udf_read_fid_stream(struct vnode *vp, uint64_t *offset, struct fileid_desc *fid, struct dirent *dirent);

/* vnode operations */
int udf_inactive(void *v);
int udf_reclaim(void *v);
int udf_readdir(void *v);
int udf_getattr(void *v);
int udf_setattr(void *v);
int udf_pathconf(void *v);
int udf_open(void *v);
int udf_close(void *v);
int udf_access(void *v);
int udf_read(void *v);
int udf_write(void *v);
int udf_trivial_bmap(void *v);
int udf_strategy(void *v);
int udf_lookup(void *v);
int udf_create(void *v);
int udf_mknod(void *v);
int udf_link(void *);
int udf_symlink(void *v);
int udf_readlink(void *v);
int udf_rename(void *v);
int udf_remove(void *v);
int udf_mkdir(void *v);
int udf_rmdir(void *v);
int udf_fsync(void *v);
int udf_advlock(void *v);


/* helpers and converters */
long udf_calchash(struct long_ad *icbptr);    /* for `inode' numbering */
uint32_t udf_getaccessmode(struct udf_node *node);
void udf_timestamp_to_timespec(struct udf_mount *ump, struct timestamp *timestamp, struct timespec *timespec);

#endif	/* !_UDF_SUBR_H_ */

