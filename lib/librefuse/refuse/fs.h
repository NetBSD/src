/* $NetBSD: fs.h,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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
#if !defined(_FUSE_FS_H_)
#define _FUSE_FS_H_

/*
 * Filesystem Stacking API, appeared on FUSE 2.7
 */

#if !defined(FUSE_H_)
#  error Do not include this header directly. Include <fuse.h> instead.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* An opaque object representing a filesystem layer. */
struct fuse_fs;

/* Create a filesystem layer. api_version has to be the version of
 * struct fuse_operations which is not necessarily the same as API
 * version. Note that user code should always use fuse_fs_new() and
 * should never call this internal function directly. */
struct fuse_fs *__fuse_fs_new(const void* op, int op_version, void* user_data);

/* These functions call the relevant filesystem operation, and return
 * the result. If the operation is not defined, they return -ENOSYS,
 * with the exception of fuse_fs_open, fuse_fs_release,
 * fuse_fs_opendir, fuse_fs_releasedir, and fuse_fs_statfs, which return 0. */
int fuse_fs_getattr_v27(struct fuse_fs *fs, const char *path, struct stat *buf);
int fuse_fs_getattr_v30(struct fuse_fs* fs, const char* path, struct stat* buf, struct fuse_file_info* fi);
int fuse_fs_fgetattr(struct fuse_fs* fs, const char* path, struct stat* buf, struct fuse_file_info* fi);
int fuse_fs_rename_v27(struct fuse_fs* fs, const char* oldpath, const char* newpath);
int fuse_fs_rename_v30(struct fuse_fs* fs, const char* oldpath, const char* newpath, unsigned int flags);
int fuse_fs_unlink(struct fuse_fs* fs, const char* path);
int fuse_fs_rmdir(struct fuse_fs* fs, const char* path);
int fuse_fs_symlink(struct fuse_fs* fs, const char* linkname, const char* path);
int fuse_fs_link(struct fuse_fs* fs, const char* oldpath, const char* newpath);
int fuse_fs_release(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi);
int fuse_fs_open(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi);
int fuse_fs_read(struct fuse_fs* fs, const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi);
int fuse_fs_read_buf(struct fuse_fs* fs, const char* path, struct fuse_bufvec** bufp, size_t size, off_t off, struct fuse_file_info* fi);
int fuse_fs_write(struct fuse_fs* fs, const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi);
int fuse_fs_write_buf(struct fuse_fs* fs, const char* path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info* fi);
int fuse_fs_fsync(struct fuse_fs* fs, const char* path, int datasync, struct fuse_file_info* fi);
int fuse_fs_flush(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi);
int fuse_fs_statfs(struct fuse_fs* fs, const char* path, struct statvfs* buf);
int fuse_fs_opendir(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi);
int fuse_fs_readdir_v27(struct fuse_fs* fs, const char* path, void* buf, fuse_fill_dir_t_v23 filler, off_t off, struct fuse_file_info* fi);
int fuse_fs_readdir_v30(struct fuse_fs* fs, const char* path, void* buf, fuse_fill_dir_t_v30 filler, off_t off, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
int fuse_fs_fsyncdir(struct fuse_fs* fs, const char* path, int datasync, struct fuse_file_info* fi);
int fuse_fs_releasedir(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi);
int fuse_fs_create(struct fuse_fs* fs, const char* path, mode_t mode, struct fuse_file_info* fi);
int fuse_fs_lock(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi, int cmd, struct flock* lock);
int fuse_fs_flock(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi, int op);
int fuse_fs_chmod_v27(struct fuse_fs *fs, const char *path, mode_t mode);
int fuse_fs_chmod_v30(struct fuse_fs* fs, const char* path, mode_t mode, struct fuse_file_info* fi);
int fuse_fs_chown_v27(struct fuse_fs *fs, const char *path, uid_t uid, gid_t gid);
int fuse_fs_chown_v30(struct fuse_fs* fs, const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi);
int fuse_fs_truncate_v27(struct fuse_fs *fs, const char *path, off_t size);
int fuse_fs_truncate_v30(struct fuse_fs* fs, const char* path, off_t size, struct fuse_file_info* fi);
int fuse_fs_ftruncate(struct fuse_fs* fs, const char* path, off_t size, struct fuse_file_info* fi);
int fuse_fs_utimens_v27(struct fuse_fs *fs, const char *path, const struct timespec tv[2]);
int fuse_fs_utimens_v30(struct fuse_fs* fs, const char* path, const struct timespec tv[2], struct fuse_file_info* fi);
int fuse_fs_access(struct fuse_fs* fs, const char* path, int mask);
int fuse_fs_readlink(struct fuse_fs* fs, const char* path, char* buf, size_t len);
int fuse_fs_mknod(struct fuse_fs* fs, const char* path, mode_t mode, dev_t rdev);
int fuse_fs_mkdir(struct fuse_fs* fs, const char* path, mode_t mode);
int fuse_fs_setxattr(struct fuse_fs* fs, const char* path, const char* name, const char* value, size_t size, int flags);
int fuse_fs_getxattr(struct fuse_fs* fs, const char* path, const char* name, char* value, size_t size);
int fuse_fs_listxattr(struct fuse_fs* fs, const char* path, char* list, size_t size);
int fuse_fs_removexattr(struct fuse_fs* fs, const char* path, const char* name);
int fuse_fs_bmap(struct fuse_fs* fs, const char* path, size_t blocksize, uint64_t *idx);
int fuse_fs_ioctl_v28(struct fuse_fs* fs, const char* path, int cmd, void* arg, struct fuse_file_info* fi, unsigned int flags, void* data);
int fuse_fs_ioctl_v35(struct fuse_fs* fs, const char* path, unsigned int cmd, void* arg, struct fuse_file_info* fi, unsigned int flags, void* data);
int fuse_fs_poll(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi, struct fuse_pollhandle* ph, unsigned* reventsp);
int fuse_fs_fallocate(struct fuse_fs* fs, const char* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi);
ssize_t fuse_fs_copy_file_range(struct fuse_fs* fs,
                                const char* path_in, struct fuse_file_info* fi_in, off_t off_in,
                                const char* path_out, struct fuse_file_info* fi_out, off_t off_out,
                                size_t len, int flags);
off_t fuse_fs_lseek(struct fuse_fs* fs, const char* path, off_t off, int whence, struct fuse_file_info* fi);
void fuse_fs_init_v27(struct fuse_fs* fs, struct fuse_conn_info* conn);
void fuse_fs_init_v30(struct fuse_fs* fs, struct fuse_conn_info* conn, struct fuse_config* cfg);
void fuse_fs_destroy(struct fuse_fs* fs);

#ifdef __cplusplus
}
#endif

#endif
