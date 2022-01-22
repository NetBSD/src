/* $NetBSD: v25.h,v 1.1 2022/01/22 08:09:40 pho Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(_FUSE_V25_H_)
#define _FUSE_V25_H_

/* Compatibility header with FUSE 2.5 API. Only declarations that have
 * differences between versions should be listed here. */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The table of file system operations in FUSE 2.5. */
struct fuse_operations_v25 {
	int	(*getattr)		(const char *, struct stat *);
	int	(*readlink)		(const char *, char *, size_t);
	int	(*getdir)		(const char *, fuse_dirh_t, fuse_dirfil_t_v22);
	int	(*mknod)		(const char *, mode_t, dev_t);
	int	(*mkdir)		(const char *, mode_t);
	int	(*unlink)		(const char *);
	int	(*rmdir)		(const char *);
	int	(*symlink)		(const char *, const char *);
	int	(*rename)		(const char *, const char *);
	int	(*link)			(const char *, const char *);
	int	(*chmod)		(const char *, mode_t);
	int	(*chown)		(const char *, uid_t, gid_t);
	int	(*truncate)		(const char *, off_t);
	int	(*utime)		(const char *, struct utimbuf *);
	int	(*open)			(const char *, struct fuse_file_info *);
	int	(*read)			(const char *, char *, size_t, off_t, struct fuse_file_info *);
	int	(*write)		(const char *, const char *, size_t, off_t, struct fuse_file_info *);
	int	(*statfs)		(const char *, struct statvfs *);
	int	(*flush)		(const char *, struct fuse_file_info *);
	int	(*release)		(const char *, struct fuse_file_info *);
	int	(*fsync)		(const char *, int, struct fuse_file_info *);
	int	(*setxattr)		(const char *, const char *, const char *, size_t, int);
	int	(*getxattr)		(const char *, const char *, char *, size_t);
	int	(*listxattr)	(const char *, char *, size_t);
	int	(*removexattr)	(const char *, const char *);
	int	(*opendir)		(const char *, struct fuse_file_info *);
	int	(*readdir)		(const char *, void *, fuse_fill_dir_t_v23, off_t, struct fuse_file_info *);
	int	(*releasedir)	(const char *, struct fuse_file_info *);
	int	(*fsyncdir)		(const char *, int, struct fuse_file_info *);
	void	*(*init)	(void);
	void	(*destroy)	(void *);
	int	(*access)		(const char *, int);
	int	(*create)		(const char *, mode_t, struct fuse_file_info *);
	int	(*ftruncate)	(const char *, off_t, struct fuse_file_info *);
	int	(*fgetattr)		(const char *, struct stat *, struct fuse_file_info *);
};

/* Mount a FUSE 2.5 filesystem. */
int fuse_mount_v25(const char *mountpoint, struct fuse_args *args);

/* Parse common options for FUSE >= 2.5 && < 3.0 */
int fuse_parse_cmdline_v25(struct fuse_args *args, char **mountpoint,
						   int *multithreaded, int *foreground);

/* Create a FUSE 2.5 filesystem. */
struct fuse *fuse_new_v25(int fd, struct fuse_args *args, const void *op,
			  int op_version, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
