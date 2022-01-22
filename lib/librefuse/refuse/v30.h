/* $NetBSD: v30.h,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
#if !defined(_FUSE_V30_H_)
#define _FUSE_V30_H_

/* Compatibility header with FUSE 3.0 API. Only declarations that have
 * differences between versions should be listed here. */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Defined in <fuse_lowlevel.h> */
struct fuse_cmdline_opts;

/* Function to add an entry in a readdir() operation. Introduced on
 * FUSE 2.3 and changed on FUSE 3.0. */
typedef int (*fuse_fill_dir_t_v30)(
	void *, const char *, const struct stat *, off_t, enum fuse_fill_dir_flags flags);

/* The table of file system operations in FUSE 3.0. */
struct fuse_operations_v30 {
	int	(*getattr)		(const char *, struct stat *, struct fuse_file_info *fi);
	int	(*readlink)		(const char *, char *, size_t);
	/* getdir has been removed as of FUSE 3.0. */
	int	(*mknod)		(const char *, mode_t, dev_t);
	int	(*mkdir)		(const char *, mode_t);
	int	(*unlink)		(const char *);
	int	(*rmdir)		(const char *);
	int	(*symlink)		(const char *, const char *);
	int	(*rename)		(const char *, const char *, unsigned int);
	int	(*link)			(const char *, const char *);
	int	(*chmod)		(const char *, mode_t, struct fuse_file_info *fi);
	int	(*chown)		(const char *, uid_t, gid_t, struct fuse_file_info *fi);
	int	(*truncate)		(const char *, off_t, struct fuse_file_info *fi);
	/* utime has been removed as of FUSE 3.0. */
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
	int	(*readdir)		(const char *, void *, fuse_fill_dir_t_v30, off_t, struct fuse_file_info *, enum fuse_readdir_flags);
	int	(*releasedir)	(const char *, struct fuse_file_info *);
	int	(*fsyncdir)		(const char *, int, struct fuse_file_info *);
	void	*(*init)	(struct fuse_conn_info *, struct fuse_config *);
	void	(*destroy)	(void *);
	int	(*access)		(const char *, int);
	int	(*create)		(const char *, mode_t, struct fuse_file_info *);
	/* ftruncate has been removed as of FUSE 3.0. */
	/* fgetattr has been removed as of FUSE 3.0. */
	int	(*lock)			(const char *, struct fuse_file_info *, int, struct flock *);
	int	(*utimens)		(const char *, const struct timespec[2], struct fuse_file_info *);
	int	(*bmap)			(const char *, size_t , uint64_t *);
	/* flag_* have been removed as of FUSE 3.0. */
	int	(*ioctl)		(const char *, int, void *, struct fuse_file_info *, unsigned int, void *);
	int	(*poll)			(const char *, struct fuse_file_info *, struct fuse_pollhandle *, unsigned *);
	int	(*write_buf)	(const char *, struct fuse_bufvec *, off_t, struct fuse_file_info *);
	int	(*read_buf)		(const char *, struct fuse_bufvec **, size_t, off_t, struct fuse_file_info *);
	int	(*flock)		(const char *, struct fuse_file_info *, int);
	int	(*fallocate)	(const char *, int, off_t, off_t, struct fuse_file_info *);
};

/* Mount a FUSE 3.0 filesystem. */
int fuse_mount_v30(struct fuse *fuse, const char *mountpoint);

/* Unmount a FUSE 3.0 filesystem. */
void fuse_unmount_v30(struct fuse *);

/* Create a FUSE 3.0 filesystem. */
struct fuse *fuse_new_v30(struct fuse_args *args,
						  const void *op, int op_version, void *user_data);

/* Destroy a FUSE 3.0 filesystem. */
void fuse_destroy_v30(struct fuse *fuse);

/* Multithreaded event loop for FUSE >= 3.0 < 3.2. Of course, every
 * time they change a function prototype to take a single extra
 * option, they change it again so it takes a struct instead. Every
 * single time. */
int fuse_loop_mt_v30(struct fuse *fuse, int clone_fd);

/* Parse common options for FUSE 3.0 */
int fuse_parse_cmdline_v30(struct fuse_args *args,
						   struct fuse_cmdline_opts *opts);

#ifdef __cplusplus
}
#endif

#endif
