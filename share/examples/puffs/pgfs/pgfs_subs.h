/*	$NetBSD: pgfs_subs.h,v 1.2 2012/04/11 14:26:19 yamt Exp $	*/

/*-
 * Copyright (c)2010,2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct fileid_lock_handle;

struct fileid_lock_handle *fileid_lock(fileid_t, struct puffs_cc *);
void fileid_unlock(struct fileid_lock_handle *);

struct Xconn;
struct vattr;
enum vtype;

int my_lo_truncate(struct Xconn *, int32_t, int32_t);
int my_lo_lseek(struct Xconn *, int32_t, int32_t, int32_t, int32_t *);
int my_lo_read(struct Xconn *, int32_t, void *, size_t, size_t *);
int my_lo_write(struct Xconn *, int32_t, const void *, size_t, size_t *);
int my_lo_open(struct Xconn *, Oid, int32_t, int32_t *);
int my_lo_close(struct Xconn *, int32_t);
int lo_open_by_fileid(struct Xconn *, fileid_t, int, int *);

#define	GETATTR_TYPE	0x00000001
#define	GETATTR_NLINK	0x00000002
#define	GETATTR_SIZE	0x00000004
#define	GETATTR_MODE	0x00000008
#define	GETATTR_UID	0x00000010
#define	GETATTR_GID	0x00000020
#define	GETATTR_TIME	0x00000040
#define	GETATTR_ALL	\
	(GETATTR_TYPE|GETATTR_NLINK|GETATTR_SIZE|GETATTR_MODE| \
	GETATTR_UID|GETATTR_GID|GETATTR_TIME)

int getattr(struct Xconn *, fileid_t, struct vattr *, unsigned int);
int update_mctime(struct Xconn *, fileid_t);
int update_atime(struct Xconn *, fileid_t);
int update_mtime(struct Xconn *, fileid_t);
int update_ctime(struct Xconn *, fileid_t);
int update_nlink(struct Xconn *, fileid_t, int);
int lookupp(struct Xconn *, fileid_t, fileid_t *);
int mkfile(struct Xconn *, enum vtype, mode_t, uid_t, gid_t, fileid_t *);
int linkfile(struct Xconn *, fileid_t, const char *, fileid_t);
int unlinkfile(struct Xconn *, fileid_t, const char *, fileid_t);
int mklinkfile(struct Xconn *, fileid_t, const char *, enum vtype, mode_t,
    uid_t, gid_t, fileid_t *);
int mklinkfile_lo(struct Xconn *, fileid_t, const char *, enum vtype, mode_t,
    uid_t, gid_t, fileid_t *, int *);
int cleanupfile(struct Xconn *, fileid_t);
int check_path(struct Xconn *, fileid_t, fileid_t);
int isempty(struct Xconn *, fileid_t, bool *);
