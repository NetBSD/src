/* $NetBSD: fs.c,v 1.1 2022/01/22 08:09:40 pho Exp $ */

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

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: fs.c,v 1.1 2022/01/22 08:09:40 pho Exp $");
#endif /* !lint */

/*
 * Filesystem Stacking API, appeared on FUSE 2.7.
 *
 * So many callback functions in struct fuse_operations have different
 * prototypes between versions. We use the stacking API to abstract
 * that away to implement puffs operations in a manageable way.
 */

#include <err.h>
#include <fuse_internal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/errno.h>

struct fuse_fs {
    void* op;
    int   op_version;
    void* user_data;
};

#define UNKNOWN_VERSION(op_version)                                     \
    errc(EXIT_FAILURE, ENOSYS, "%s: unknown fuse_operations version: %d", \
         __func__, op_version)

static void*
clone_op(const void* op, int op_version) {
    void* cloned;

    switch (op_version) {
#define CLONE_OP(VER)                                                   \
    case VER:                                                           \
        cloned = malloc(sizeof(struct __CONCAT(fuse_operations_v,VER)));      \
        if (!cloned)                                                    \
            return NULL;                                                \
        memcpy(cloned, op, sizeof(struct __CONCAT(fuse_operations_v,VER)));   \
        return cloned

        CLONE_OP(11);
        CLONE_OP(21);
        CLONE_OP(22);
        CLONE_OP(23);
        CLONE_OP(25);
        CLONE_OP(26);
        CLONE_OP(28);
        CLONE_OP(29);
        CLONE_OP(30);
        CLONE_OP(34);
        CLONE_OP(35);
        CLONE_OP(38);
#undef CLONE_OP
    default:
        UNKNOWN_VERSION(op_version);
    }
}

struct fuse_fs*
__fuse_fs_new(const void* op, int op_version, void* user_data) {
    struct fuse_fs* fs;

    fs = malloc(sizeof(struct fuse_fs));
    if (!fs)
        err(EXIT_FAILURE, __func__);

    /* Callers aren't obliged to keep "op" valid during the lifetime
     * of struct fuse_fs*. We must clone it now, even though it's
     * non-trivial. */
    fs->op = clone_op(op, op_version);
    if (!fs->op)
        err(EXIT_FAILURE, __func__);

    fs->op_version = op_version;
    fs->user_data  = user_data;

    return fs;
}

/* Clobber the context private_data with that of this filesystem
 * layer. This function needs to be called before invoking any of
 * operation callbacks. */
static void
clobber_context_user_data(struct fuse_fs* fs) {
    fuse_get_context()->private_data = fs->user_data;
}

/* Ugly... These are like hand-written vtables... */
int
fuse_fs_getattr_v27(struct fuse_fs *fs, const char *path, struct stat *buf) {
    return fuse_fs_getattr_v30(fs, path, buf, NULL);
}

int
fuse_fs_getattr_v30(struct fuse_fs* fs, const char* path,
                    struct stat* buf, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_GETATTR(VER)                                           \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getattr(path, buf); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_GETATTR(11);
        CALL_OLD_GETATTR(21);
        CALL_OLD_GETATTR(22);
        CALL_OLD_GETATTR(23);
        CALL_OLD_GETATTR(25);
        CALL_OLD_GETATTR(26);
        CALL_OLD_GETATTR(28);
        CALL_OLD_GETATTR(29);
#undef CALL_OLD_GETATTR

#define CALL_GETATTR(VER)                                               \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getattr(path, buf, fi); \
        else                                                            \
            return -ENOSYS
        CALL_GETATTR(30);
        CALL_GETATTR(34);
        CALL_GETATTR(38);
#undef CALL_GETATTR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_fgetattr(struct fuse_fs* fs, const char* path, struct stat* buf,
                 struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    /* fgetattr() was introduced on FUSE 2.5 then disappeared on FUSE
     * 3.0. Fall back to getattr() if it's missing. */
    switch (fs->op_version) {
    case 11:
    case 21:
    case 22:
    case 23:
        return fuse_fs_getattr_v30(fs, path, buf, fi);

#define CALL_FGETATTR_OR_OLD_GETATTR(VER)       \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fgetattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fgetattr(path, buf, fi); \
        else if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getattr(path, buf); \
        else                                                            \
            return -ENOSYS
        CALL_FGETATTR_OR_OLD_GETATTR(25);
        CALL_FGETATTR_OR_OLD_GETATTR(26);
        CALL_FGETATTR_OR_OLD_GETATTR(28);
        CALL_FGETATTR_OR_OLD_GETATTR(29);
#undef CALL_FGETATTR_OR_OLD_GETATTR

    case 30:
    case 34:
    case 38:
        return fuse_fs_getattr_v30(fs, path, buf, fi);
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_rename_v27(struct fuse_fs* fs, const char* oldpath, const char* newpath) {
    return fuse_fs_rename_v30(fs, oldpath, newpath, 0);
}

int
fuse_fs_rename_v30(struct fuse_fs* fs, const char* oldpath,
                   const char* newpath, unsigned int flags) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_RENAME(VER)                                            \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->rename) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->rename(oldpath, newpath); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_RENAME(11);
        CALL_OLD_RENAME(21);
        CALL_OLD_RENAME(22);
        CALL_OLD_RENAME(23);
        CALL_OLD_RENAME(25);
        CALL_OLD_RENAME(26);
        CALL_OLD_RENAME(28);
        CALL_OLD_RENAME(29);
#undef CALL_OLD_RENAME

#define CALL_RENAME(VER)                                                \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->rename) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->rename(oldpath, newpath, flags); \
        else                                                            \
            return -ENOSYS
        CALL_RENAME(30);
        CALL_RENAME(34);
        CALL_RENAME(38);
#undef CALL_RENAME
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_unlink(struct fuse_fs* fs, const char* path) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_UNLINK(VER)                                                \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->unlink) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->unlink(path); \
        else                                                            \
            return -ENOSYS
        CALL_UNLINK(11);
        CALL_UNLINK(21);
        CALL_UNLINK(22);
        CALL_UNLINK(23);
        CALL_UNLINK(25);
        CALL_UNLINK(26);
        CALL_UNLINK(28);
        CALL_UNLINK(29);
        CALL_UNLINK(30);
        CALL_UNLINK(34);
        CALL_UNLINK(38);
#undef CALL_UNLINK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_rmdir(struct fuse_fs* fs, const char* path) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_RMDIR(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->rmdir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->rmdir(path); \
        else                                                            \
            return -ENOSYS
        CALL_RMDIR(11);
        CALL_RMDIR(21);
        CALL_RMDIR(22);
        CALL_RMDIR(23);
        CALL_RMDIR(25);
        CALL_RMDIR(26);
        CALL_RMDIR(28);
        CALL_RMDIR(29);
        CALL_RMDIR(30);
        CALL_RMDIR(34);
        CALL_RMDIR(38);
#undef CALL_RMDIR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_symlink(struct fuse_fs* fs, const char* linkname, const char* path) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_SYMLINK(VER)                                               \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->symlink) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->symlink(linkname, path); \
        else                                                            \
            return -ENOSYS
        CALL_SYMLINK(11);
        CALL_SYMLINK(21);
        CALL_SYMLINK(22);
        CALL_SYMLINK(23);
        CALL_SYMLINK(25);
        CALL_SYMLINK(26);
        CALL_SYMLINK(28);
        CALL_SYMLINK(29);
        CALL_SYMLINK(30);
        CALL_SYMLINK(34);
        CALL_SYMLINK(38);
#undef CALL_SYMLINK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_link(struct fuse_fs* fs, const char* oldpath, const char* newpath) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_LINK(VER)                                               \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->link) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->link(oldpath, newpath); \
        else                                                            \
            return -ENOSYS
        CALL_LINK(11);
        CALL_LINK(21);
        CALL_LINK(22);
        CALL_LINK(23);
        CALL_LINK(25);
        CALL_LINK(26);
        CALL_LINK(28);
        CALL_LINK(29);
        CALL_LINK(30);
        CALL_LINK(34);
        CALL_LINK(38);
#undef CALL_LINK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_release(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_RELEASE(VER)                                           \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->release) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->release(path, fi->flags); \
        else                                                            \
            return 0 /* Special case */
        CALL_OLD_RELEASE(11);
        CALL_OLD_RELEASE(21);
#undef CALL_OLD_RELEASE

#define CALL_RELEASE(VER)                                               \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->release) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->release(path, fi); \
        else                                                            \
            return 0 /* Special case */
        CALL_RELEASE(22);
        CALL_RELEASE(23);
        CALL_RELEASE(25);
        CALL_RELEASE(26);
        CALL_RELEASE(28);
        CALL_RELEASE(29);
        CALL_RELEASE(30);
        CALL_RELEASE(34);
        CALL_RELEASE(38);
#undef CALL_RELEASE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_open(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_OPEN(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->open) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->open(path, fi->flags); \
        else                                                            \
            return 0 /* Special case */
        CALL_OLD_OPEN(11);
        CALL_OLD_OPEN(21);
#undef CALL_OLD_OPEN

#define CALL_OPEN(VER)                                                  \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->open) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->open(path, fi); \
        else                                                            \
            return 0 /* Special case */
        CALL_OPEN(22);
        CALL_OPEN(23);
        CALL_OPEN(25);
        CALL_OPEN(26);
        CALL_OPEN(28);
        CALL_OPEN(29);
        CALL_OPEN(30);
        CALL_OPEN(34);
        CALL_OPEN(38);
#undef CALL_OPEN
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_read(struct fuse_fs* fs, const char* path, char* buf,
             size_t size, off_t off, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_READ(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->read) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->read(path, buf, size, off); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_READ(11);
        CALL_OLD_READ(21);
#undef CALL_OLD_READ

#define CALL_READ(VER)                                                  \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->read) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->read(path, buf, size, off, fi); \
        else                                                            \
            return -ENOSYS
        CALL_READ(22);
        CALL_READ(23);
        CALL_READ(25);
        CALL_READ(26);
        CALL_READ(28);
        CALL_READ(29);
        CALL_READ(30);
        CALL_READ(34);
        CALL_READ(38);
#undef CALL_READ
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_read_buf(struct fuse_fs* fs, const char* path,
                 struct fuse_bufvec** bufp, size_t size, off_t off,
                 struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* FUSE < 2.9 didn't have read_buf(). */
    case 11:
    case 21:
    case 22:
    case 23:
    case 25:
    case 26:
    case 28:
        return -ENOSYS;
#define CALL_READ_BUF(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->read_buf) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->read_buf(path, bufp, size, off, fi); \
        else                                                            \
            return -ENOSYS
        CALL_READ_BUF(29);
        CALL_READ_BUF(30);
        CALL_READ_BUF(34);
        CALL_READ_BUF(38);
#undef CALL_READ_BUF
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_write(struct fuse_fs* fs, const char* path, const char* buf,
              size_t size, off_t off, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_WRITE(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->write) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->write(path, buf, size, off); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_WRITE(11);
        CALL_OLD_WRITE(21);
#undef CALL_OLD_WRITE

#define CALL_WRITE(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->write) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->write(path, buf, size, off, fi); \
        else                                                            \
            return -ENOSYS
        CALL_WRITE(22);
        CALL_WRITE(23);
        CALL_WRITE(25);
        CALL_WRITE(26);
        CALL_WRITE(28);
        CALL_WRITE(29);
        CALL_WRITE(30);
        CALL_WRITE(34);
        CALL_WRITE(38);
#undef CALL_WRITE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_write_buf(struct fuse_fs* fs, const char* path,
                  struct fuse_bufvec* bufp, off_t off,
                  struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* FUSE < 2.9 didn't have write_buf(). */
    case 11:
    case 21:
    case 22:
    case 23:
    case 25:
    case 26:
    case 28:
        return -ENOSYS;
#define CALL_WRITE_BUF(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->write_buf) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->write_buf(path, bufp, off, fi); \
        else                                                            \
            return -ENOSYS
        CALL_WRITE_BUF(29);
        CALL_WRITE_BUF(30);
        CALL_WRITE_BUF(34);
        CALL_WRITE_BUF(38);
#undef CALL_WRITE_BUF
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_fsync(struct fuse_fs* fs, const char* path, int datasync, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_FSYNC(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fsync) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fsync(path, datasync); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_FSYNC(11);
        CALL_OLD_FSYNC(21);
#undef CALL_OLD_FSYNC

#define CALL_FSYNC(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fsync) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fsync(path, datasync, fi); \
        else                                                            \
            return -ENOSYS
        CALL_FSYNC(22);
        CALL_FSYNC(23);
        CALL_FSYNC(25);
        CALL_FSYNC(26);
        CALL_FSYNC(28);
        CALL_FSYNC(29);
        CALL_FSYNC(30);
        CALL_FSYNC(34);
        CALL_FSYNC(38);
#undef CALL_FSYNC
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_flush(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    /* flush() appeared on FUSE 2.1 and its prototype was changed on
     * 2.2. */
    switch (fs->op_version) {
    case 11:
        return -ENOSYS;
    case 21:
        if (((const struct fuse_operations_v21 *)fs->op)->flush)
            return ((const struct fuse_operations_v21 *)fs->op)->flush(path);
        else
            return -ENOSYS;

#define CALL_FLUSH(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->flush) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->flush(path, fi); \
        else                                                            \
            return -ENOSYS
        CALL_FLUSH(22);
        CALL_FLUSH(23);
        CALL_FLUSH(25);
        CALL_FLUSH(26);
        CALL_FLUSH(28);
        CALL_FLUSH(29);
        CALL_FLUSH(30);
        CALL_FLUSH(34);
        CALL_FLUSH(38);
#undef CALL_FLUSH
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

static void
zero_statvfs(struct statvfs* dst) {
    dst->f_bsize   = 0;
    dst->f_frsize  = 0;
    dst->f_blocks  = 0;
    dst->f_bfree   = 0;
    dst->f_bavail  = 0;
    dst->f_files   = 0;
    dst->f_ffree   = 0;
    dst->f_fresvd  = 0;
}
static void
fuse_statfs_to_statvfs(struct statvfs* dst, const struct fuse_statfs* src) {
    dst->f_bsize   = (unsigned long)src->block_size;
    dst->f_frsize  = (unsigned long)src->block_size; /* Dunno if this is correct. */
    dst->f_blocks  = (fsblkcnt_t)src->blocks;
    dst->f_bfree   = (fsblkcnt_t)src->blocks_free;
    dst->f_bavail  = (fsblkcnt_t)src->blocks_free;
    dst->f_files   = (fsfilcnt_t)src->files;
    dst->f_ffree   = (fsfilcnt_t)src->files_free;
}
static void
linux_statfs_to_statvfs(struct statvfs* dst, const struct statfs* src) {
    dst->f_bsize   = (unsigned long)src->f_bsize;
    dst->f_frsize  = (unsigned long)src->f_bsize; /* Dunno if this is correct. */
    dst->f_blocks  = src->f_blocks;
    dst->f_bfree   = src->f_bfree;
    dst->f_bavail  = src->f_bavail;
    dst->f_files   = src->f_files;
    dst->f_ffree   = src->f_ffree;
}
int
fuse_fs_statfs(struct fuse_fs* fs, const char* path, struct statvfs* buf) {
    clobber_context_user_data(fs);

    zero_statvfs(buf);

    switch (fs->op_version) {
        /* FUSE < 2.1 used "struct fuse_statfs". */
    case 11:
        if (((const struct fuse_operations_v11*)fs->op)->statfs) {
            struct fuse_statfs statfs_v11;
            int ret;

            ret = ((const struct fuse_operations_v11*)fs->op)->statfs(path, &statfs_v11);
            if (ret == 0)
                fuse_statfs_to_statvfs(buf, &statfs_v11);

            return ret;
        }
        else
            return 0; /* Special case */

        /* FUSE >= 2.2 && < 2.5 used Linux-specific "struct
         * statfs". */
#define CALL_LINUX_STATFS(VER)                                          \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->statfs) { \
            struct statfs statfs_v22;                                   \
            int ret;                                                    \
                                                                        \
            ret = ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->statfs(path, &statfs_v22); \
            if (ret == 0)                                               \
                linux_statfs_to_statvfs(buf, &statfs_v22);              \
                                                                        \
            return ret;                                                 \
        }                                                               \
        else                                                            \
            return 0; /* Special case */
        CALL_LINUX_STATFS(22);
        CALL_LINUX_STATFS(23);
#undef CALL_STATFS

        /* FUSE >= 2.5 use struct statvfs. */
#define CALL_STATFS(VER)                                                \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->statfs) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->statfs(path, buf); \
        else                                                            \
            return 0; /* Special case */
        CALL_STATFS(25);
        CALL_STATFS(26);
        CALL_STATFS(28);
        CALL_STATFS(29);
        CALL_STATFS(30);
        CALL_STATFS(34);
        CALL_STATFS(38);
#undef CALL_STATFS
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_opendir(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* FUSE < 2.3 didn't have opendir() and used to read
         * directories without opening them. */
    case 11:
    case 21:
    case 22:
        return 0; /* Special case */

#define CALL_OPENDIR(VER)                                               \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->opendir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->opendir(path, fi); \
        else                                                            \
            return 0 /* Special case */
        CALL_OPENDIR(23);
        CALL_OPENDIR(25);
        CALL_OPENDIR(26);
        CALL_OPENDIR(28);
        CALL_OPENDIR(29);
        CALL_OPENDIR(30);
        CALL_OPENDIR(34);
        CALL_OPENDIR(38);
#undef CALL_OPENDIR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

/* ===================================
 *     -=- The readdir Madness -=-
 *     Juggling with Nested Shims
 * =================================== */

struct fuse_fill_dir_v23_shim {
    void*               dirh;
    fuse_fill_dir_t_v23 fill_dir_v23;
};

/* Translate dirent DT_* to mode_t. Needed by shim functions. */
static mode_t
dt_to_mode(int dt) {
    switch (dt) {
    case DT_UNKNOWN: return 0;
    case DT_FIFO:    return S_IFIFO;
    case DT_CHR:     return S_IFCHR;
    case DT_DIR:     return S_IFCHR;
    case DT_BLK:     return S_IFBLK;
    case DT_REG:     return S_IFREG;
    case DT_LNK:     return S_IFLNK;
    case DT_SOCK:    return S_IFSOCK;
    case DT_WHT:     return S_IFWHT;
    default:
        errx(EXIT_FAILURE, "%s: unknown dirent type: %d",
             __func__, dt);
    }
}

/* This is a shim function that satisfies the type of
 * fuse_dirfil_t_v11 but calls fuse_fill_dir_v23. */
static int
fuse_dirfil_v11_to_fill_dir_v23(fuse_dirh_t handle, const char* name, int type) {
    struct fuse_fill_dir_v23_shim* shim = handle;
    struct stat stbuf;
    int res; /* 1 or 0 */

    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_mode = dt_to_mode(type);

    res = shim->fill_dir_v23(shim->dirh, name, &stbuf, 0);
    return res ? -ENOMEM : 0;
}

/* This is a shim function that satisfies the type of
 * fuse_dirfil_t_v22 but calls fuse_fill_dir_v23. */
static int
fuse_dirfil_v22_to_fill_dir_v23(fuse_dirh_t handle, const char* name, int type, ino_t ino) {
    struct fuse_fill_dir_v23_shim* shim = handle;
    struct stat stbuf;
    int res; /* 1 or 0 */

    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_mode = dt_to_mode(type);
    stbuf.st_ino  = ino;

    res = shim->fill_dir_v23(shim->dirh, name, &stbuf, 0);
    return res ? -ENOMEM : 0;
}

struct fuse_fill_dir_v30_shim {
    void*               dirh;
    fuse_fill_dir_t_v30 fill_dir_v30;
};

/* This is a shim function that satisfies the type of
 * fuse_fill_dir_v23 but calls fuse_fill_dir_v30. */
static int
fuse_fill_dir_v23_to_v30(void* buf, const char* name,
                         const struct stat* stat, off_t off) {

    struct fuse_fill_dir_v30_shim* shim = buf;

    return shim->fill_dir_v30(shim->dirh, name, stat, off, (enum fuse_fill_dir_flags)0);
}

int
fuse_fs_readdir_v27(struct fuse_fs* fs, const char* path, void* buf,
                    fuse_fill_dir_t_v23 filler, off_t off,
                    struct fuse_file_info* fi) {

    struct fuse_fill_dir_v23_shim v23_shim;

    v23_shim.dirh         = buf;
    v23_shim.fill_dir_v23 = filler;

    clobber_context_user_data(fs);

    switch (fs->op_version) {
        /* FUSE < 2.2 had getdir() that used fuse_dirfil_t_v11. */
#define CALL_GETDIR_V11(VER)                                        \
    case VER:                                                       \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getdir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getdir(path, &v23_shim, fuse_dirfil_v11_to_fill_dir_v23); \
        else                                                            \
            return -ENOSYS
        CALL_GETDIR_V11(11);
        CALL_GETDIR_V11(21);
#undef CALL_GETDIR_V11

        /* FUSE 2.2 had getdir() that used fuse_dirfil_t_v22 but
         * didn't have readdir(). */
    case 22:
        if (((const struct fuse_operations_v22*)fs->op)->getdir)
            return ((const struct fuse_operations_v22*)fs->op)->getdir(path, &v23_shim, fuse_dirfil_v22_to_fill_dir_v23);
        else
            return -ENOSYS;

        /* FUSE 2.3 introduced readdir() but still had getdir() as
         * a deprecated operation. It had been this way until FUSE 3.0
         * finally removed getdir() and also changed the prototype of
         * readdir(). */
#define CALL_READDIR_OR_GETDIR(VER)                                     \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->readdir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->readdir(path, buf, filler, off, fi); \
        else if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getdir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getdir(path, &v23_shim, fuse_dirfil_v22_to_fill_dir_v23); \
        else                                                            \
            return -ENOSYS
        CALL_READDIR_OR_GETDIR(23);
        CALL_READDIR_OR_GETDIR(25);
        CALL_READDIR_OR_GETDIR(26);
        CALL_READDIR_OR_GETDIR(28);
        CALL_READDIR_OR_GETDIR(29);
#undef CALL_READDIR_OR_GETDIR

    default:
        /* FUSE >= 3.0 filesystems will never call this function. We
         * can safely ignore them here. */
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_readdir_v30(struct fuse_fs* fs, const char* path, void* buf,
                    fuse_fill_dir_t_v30 filler, off_t off,
                    struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    clobber_context_user_data(fs);

    if (fs->op_version < 30) {
        struct fuse_fill_dir_v30_shim v30_shim;

        v30_shim.dirh         = buf;
        v30_shim.fill_dir_v30 = filler;

        return fuse_fs_readdir_v27(fs, path, &v30_shim, fuse_fill_dir_v23_to_v30, off, fi);
    }
    else {
        switch (fs->op_version) {
#define CALL_READDIR(VER)                                               \
            case VER:                                                   \
                if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->readdir) \
                    return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->readdir(path, buf, filler, off, fi, flags); \
                else                                                    \
                    return -ENOSYS
            CALL_READDIR(30);
            CALL_READDIR(34);
            CALL_READDIR(38);
#undef CALL_READDIR
        default:
            UNKNOWN_VERSION(fs->op_version);
        }
    }
}

/* ==============================
 *   The End of readdir Madness
 * ============================== */

int
fuse_fs_fsyncdir(struct fuse_fs* fs, const char* path, int datasync, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    /* fsyncdir() appeared on FUSE 2.3. */
    switch (fs->op_version) {
    case 11:
    case 21:
    case 22:
        return -ENOSYS;

#define CALL_FSYNCDIR(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fsyncdir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fsyncdir(path, datasync, fi); \
        else                                                            \
            return -ENOSYS
        CALL_FSYNCDIR(23);
        CALL_FSYNCDIR(25);
        CALL_FSYNCDIR(26);
        CALL_FSYNCDIR(28);
        CALL_FSYNCDIR(29);
        CALL_FSYNCDIR(30);
        CALL_FSYNCDIR(34);
        CALL_FSYNCDIR(38);
#undef CALL_FSYNCDIR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_releasedir(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* FUSE < 2.3 didn't have releasedir() and was reading
         * directories without opening them. */
    case 11:
    case 21:
    case 22:
        return 0; /* Special case */

#define CALL_RELEASEDIR(VER)                                            \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->releasedir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->releasedir(path, fi); \
        else                                                            \
            return 0 /* Special case */
        CALL_RELEASEDIR(23);
        CALL_RELEASEDIR(25);
        CALL_RELEASEDIR(26);
        CALL_RELEASEDIR(28);
        CALL_RELEASEDIR(29);
        CALL_RELEASEDIR(30);
        CALL_RELEASEDIR(34);
        CALL_RELEASEDIR(38);
#undef CALL_RELEASEDIR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_create(struct fuse_fs* fs, const char* path, mode_t mode, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* FUSE < 2.5 didn't have create(). */
    case 11:
    case 21:
    case 22:
    case 23:
        return -ENOSYS;

#define CALL_CREATE(VER)                                                \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->create) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->create(path, mode, fi); \
        else                                                            \
            return -ENOSYS
        CALL_CREATE(25);
        CALL_CREATE(26);
        CALL_CREATE(28);
        CALL_CREATE(29);
        CALL_CREATE(30);
        CALL_CREATE(34);
        CALL_CREATE(38);
#undef CALL_CREATE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_lock(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi,
             int cmd, struct flock* lock) {
    clobber_context_user_data(fs);
    /* locK() appeared on FUSE 2.6. */
    switch (fs->op_version) {
    case 11:
    case 21:
    case 22:
    case 23:
    case 25:
        return -ENOSYS;

#define CALL_LOCK(VER)                                                  \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->lock) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->lock(path, fi, cmd, lock); \
        else                                                            \
            return -ENOSYS
        CALL_LOCK(26);
        CALL_LOCK(28);
        CALL_LOCK(29);
        CALL_LOCK(30);
        CALL_LOCK(34);
        CALL_LOCK(38);
#undef CALL_LOCK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_flock(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi, int op) {
    clobber_context_user_data(fs);
    /* flocK() appeared on FUSE 2.9. */
    switch (fs->op_version) {
    case 11:
    case 21:
    case 22:
    case 23:
    case 25:
    case 26:
    case 28:
        return -ENOSYS;

#define CALL_FLOCK(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->flock) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->flock(path, fi, op); \
        else                                                            \
            return -ENOSYS
        CALL_FLOCK(29);
        CALL_FLOCK(30);
        CALL_FLOCK(34);
        CALL_FLOCK(38);
#undef CALL_FLOCK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_chmod_v27(struct fuse_fs *fs, const char *path, mode_t mode) {
    return fuse_fs_chmod_v30(fs, path, mode, NULL);
}

int
fuse_fs_chmod_v30(struct fuse_fs* fs, const char* path,
                  mode_t mode, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_CHMOD(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chmod) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chmod(path, mode); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_CHMOD(11);
        CALL_OLD_CHMOD(21);
        CALL_OLD_CHMOD(22);
        CALL_OLD_CHMOD(23);
        CALL_OLD_CHMOD(25);
        CALL_OLD_CHMOD(26);
        CALL_OLD_CHMOD(28);
        CALL_OLD_CHMOD(29);
#undef CALL_OLD_CHMOD

#define CALL_CHMOD(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chmod) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chmod(path, mode, fi); \
        else                                                            \
            return -ENOSYS
        CALL_CHMOD(30);
        CALL_CHMOD(34);
        CALL_CHMOD(38);
#undef CALL_CHMOD
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int fuse_fs_chown_v27(struct fuse_fs *fs, const char *path, uid_t uid, gid_t gid) {
    return fuse_fs_chown_v30(fs, path, uid, gid, NULL);
}

int
fuse_fs_chown_v30(struct fuse_fs* fs, const char* path,
                  uid_t uid, gid_t gid, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_CHOWN(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chown) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chown(path, uid, gid); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_CHOWN(11);
        CALL_OLD_CHOWN(21);
        CALL_OLD_CHOWN(22);
        CALL_OLD_CHOWN(23);
        CALL_OLD_CHOWN(25);
        CALL_OLD_CHOWN(26);
        CALL_OLD_CHOWN(28);
        CALL_OLD_CHOWN(29);
#undef CALL_OLD_CHOWN

#define CALL_CHOWN(VER)                         \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chown) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->chown(path, uid, gid, fi); \
        else                                                            \
            return -ENOSYS
        CALL_CHOWN(30);
        CALL_CHOWN(34);
        CALL_CHOWN(38);
#undef CALL_CHOWN
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int fuse_fs_truncate_v27(struct fuse_fs *fs, const char *path, off_t size) {
    return fuse_fs_truncate_v30(fs, path, size, NULL);
}

int
fuse_fs_truncate_v30(struct fuse_fs* fs, const char* path, off_t size, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_OLD_TRUNCATE(VER)                                          \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate(path, size); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_TRUNCATE(11);
        CALL_OLD_TRUNCATE(21);
        CALL_OLD_TRUNCATE(22);
        CALL_OLD_TRUNCATE(23);
        CALL_OLD_TRUNCATE(25);
        CALL_OLD_TRUNCATE(26);
        CALL_OLD_TRUNCATE(28);
        CALL_OLD_TRUNCATE(29);
#undef CALL_OLD_TRUNCATE

#define CALL_TRUNCATE(VER)                      \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate(path, size, fi); \
        else                                                            \
            return -ENOSYS
        CALL_TRUNCATE(30);
        CALL_TRUNCATE(34);
        CALL_TRUNCATE(38);
#undef CALL_TRUNCATE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_ftruncate(struct fuse_fs* fs, const char* path, off_t size, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* FUSE < 2.5 didn't have ftruncate(). Always fall back to
         * truncate(). */
#define CALL_OLD_TRUNCATE(VER)                                          \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate(path, size); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_TRUNCATE(11);
        CALL_OLD_TRUNCATE(21);
        CALL_OLD_TRUNCATE(22);
        CALL_OLD_TRUNCATE(23);
#undef CALL_OLD_TRUNCATE

        /* ftruncate() appeared on FUSE 2.5 and then disappeared on
         * FUSE 3.0. Call it if it exists, or fall back to truncate()
         * otherwise. */
#define CALL_FTRUNCATE_OR_TRUNCATE(VER)                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->ftruncate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->ftruncate(path, size, fi); \
        else if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate(path, size); \
        else                                                            \
            return -ENOSYS
        CALL_FTRUNCATE_OR_TRUNCATE(25);
        CALL_FTRUNCATE_OR_TRUNCATE(26);
        CALL_FTRUNCATE_OR_TRUNCATE(28);
        CALL_FTRUNCATE_OR_TRUNCATE(29);
#undef CALL_FTRUNCATE_OR_TRUNCATE

        /* FUSE >= 3.0 have truncate() but with a different function
         * type. */
#define CALL_TRUNCATE(VER)                      \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->truncate(path, size, fi); \
        else                                                            \
            return -ENOSYS
        CALL_TRUNCATE(30);
        CALL_TRUNCATE(34);
        CALL_TRUNCATE(38);
#undef CALL_TRUNCATE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_utimens_v27(struct fuse_fs *fs, const char *path, const struct timespec tv[2]) {
    return fuse_fs_utimens_v30(fs, path, tv, NULL);
}

int
fuse_fs_utimens_v30(struct fuse_fs* fs, const char* path,
                    const struct timespec tv[2], struct fuse_file_info* fi) {
    struct utimbuf timbuf;

    timbuf.actime  = tv[0].tv_sec;
    timbuf.modtime = tv[1].tv_sec;

    clobber_context_user_data(fs);

    switch (fs->op_version) {
        /* FUSE < 2.6 didn't have utimens() but had utime()
         * instead. */
#define CALL_UTIME(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utime) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utime(path, &timbuf); \
        else                                                            \
            return -ENOSYS
        CALL_UTIME(11);
        CALL_UTIME(21);
        CALL_UTIME(22);
        CALL_UTIME(23);
        CALL_UTIME(25);
#undef CALL_UTIME

        /* utimens() appeared on FUSE 2.6. Call it if it exists, or fall back to
         * utime() otherwise. */
#define CALL_UTIMENS_OR_UTIME(VER)                                      \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utimens) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utimens(path, tv); \
        else if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utime) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utime(path, &timbuf); \
        else                                                            \
            return -ENOSYS
        CALL_UTIMENS_OR_UTIME(26);
        CALL_UTIMENS_OR_UTIME(28);
        CALL_UTIMENS_OR_UTIME(29);
#undef CALL_UTIMENS_OR_UTIME

        /* utime() disappeared on FUSE 3.0. */
#define CALL_UTIMENS(VER)                       \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utimens) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->utimens(path, tv, fi); \
        else                                                            \
            return -ENOSYS
        CALL_UTIMENS(30);
        CALL_UTIMENS(34);
        CALL_UTIMENS(38);
#undef CALL_UTIMENS
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_access(struct fuse_fs* fs, const char* path, int mask) {
    clobber_context_user_data(fs);
    /* access() appeared on FUSE 2.5. */
    switch (fs->op_version) {
    case 11:
    case 21:
    case 22:
    case 23:
        return -ENOSYS;
#define CALL_ACCESS(VER)                                                \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->access) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->access(path, mask); \
        else                                                            \
            return -ENOSYS
        CALL_ACCESS(25);
        CALL_ACCESS(26);
        CALL_ACCESS(28);
        CALL_ACCESS(29);
        CALL_ACCESS(30);
        CALL_ACCESS(34);
        CALL_ACCESS(38);
#undef CALL_ACCESS
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_readlink(struct fuse_fs* fs, const char* path, char* buf, size_t len) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_READLINK(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->readlink) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->readlink(path, buf, len); \
        else                                                            \
            return -ENOSYS
        CALL_READLINK(11);
        CALL_READLINK(21);
        CALL_READLINK(22);
        CALL_READLINK(23);
        CALL_READLINK(25);
        CALL_READLINK(26);
        CALL_READLINK(28);
        CALL_READLINK(29);
        CALL_READLINK(30);
        CALL_READLINK(34);
        CALL_READLINK(38);
#undef CALL_READLINK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_mknod(struct fuse_fs* fs, const char* path, mode_t mode, dev_t rdev) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_MKNOD(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->mknod) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->mknod(path, mode, rdev); \
        else                                                            \
            return -ENOSYS
        CALL_MKNOD(11);
        CALL_MKNOD(21);
        CALL_MKNOD(22);
        CALL_MKNOD(23);
        CALL_MKNOD(25);
        CALL_MKNOD(26);
        CALL_MKNOD(28);
        CALL_MKNOD(29);
        CALL_MKNOD(30);
        CALL_MKNOD(34);
        CALL_MKNOD(38);
#undef CALL_MKNOD
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_mkdir(struct fuse_fs* fs, const char* path, mode_t mode) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
#define CALL_MKDIR(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->mkdir) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->mkdir(path, mode); \
        else                                                            \
            return -ENOSYS
        CALL_MKDIR(11);
        CALL_MKDIR(21);
        CALL_MKDIR(22);
        CALL_MKDIR(23);
        CALL_MKDIR(25);
        CALL_MKDIR(26);
        CALL_MKDIR(28);
        CALL_MKDIR(29);
        CALL_MKDIR(30);
        CALL_MKDIR(34);
        CALL_MKDIR(38);
#undef CALL_MKDIR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int fuse_fs_setxattr(struct fuse_fs* fs, const char* path, const char* name,
                     const char* value, size_t size, int flags) {
    clobber_context_user_data(fs);
    /* setxattr() appeared on FUSE 2.1. */
    switch (fs->op_version) {
    case 11:
        return -ENOSYS;
#define CALL_SETXATTR(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->setxattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->setxattr(path, name, value, size, flags); \
        else                                                            \
            return -ENOSYS
        CALL_SETXATTR(21);
        CALL_SETXATTR(22);
        CALL_SETXATTR(23);
        CALL_SETXATTR(25);
        CALL_SETXATTR(26);
        CALL_SETXATTR(28);
        CALL_SETXATTR(29);
        CALL_SETXATTR(30);
        CALL_SETXATTR(34);
        CALL_SETXATTR(38);
#undef CALL_SETXATTR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_getxattr(struct fuse_fs* fs, const char* path, const char* name,
                 char* value, size_t size) {
    clobber_context_user_data(fs);
    /* getxattr() appeared on FUSE 2.1. */
    switch (fs->op_version) {
    case 11:
        return -ENOSYS;
#define CALL_GETXATTR(VER)                                              \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getxattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->getxattr(path, name, value, size); \
        else                                                            \
            return -ENOSYS
        CALL_GETXATTR(21);
        CALL_GETXATTR(22);
        CALL_GETXATTR(23);
        CALL_GETXATTR(25);
        CALL_GETXATTR(26);
        CALL_GETXATTR(28);
        CALL_GETXATTR(29);
        CALL_GETXATTR(30);
        CALL_GETXATTR(34);
        CALL_GETXATTR(38);
#undef CALL_GETXATTR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int fuse_fs_listxattr(struct fuse_fs* fs, const char* path, char* list, size_t size) {
    clobber_context_user_data(fs);
    /* listxattr() appeared on FUSE 2.1. */
    switch (fs->op_version) {
    case 11:
        return -ENOSYS;
#define CALL_LISTXATTR(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->listxattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->listxattr(path, list, size); \
        else                                                            \
            return -ENOSYS
        CALL_LISTXATTR(21);
        CALL_LISTXATTR(22);
        CALL_LISTXATTR(23);
        CALL_LISTXATTR(25);
        CALL_LISTXATTR(26);
        CALL_LISTXATTR(28);
        CALL_LISTXATTR(29);
        CALL_LISTXATTR(30);
        CALL_LISTXATTR(34);
        CALL_LISTXATTR(38);
#undef CALL_LISTXATTR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_removexattr(struct fuse_fs* fs, const char* path, const char* name) {
    clobber_context_user_data(fs);
    /* removexattr() appeared on FUSE 2.1. */
    switch (fs->op_version) {
    case 11:
        return -ENOSYS;
#define CALL_REMOVEXATTR(VER)                                           \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->removexattr) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->removexattr(path, name); \
        else                                                            \
            return -ENOSYS
        CALL_REMOVEXATTR(21);
        CALL_REMOVEXATTR(22);
        CALL_REMOVEXATTR(23);
        CALL_REMOVEXATTR(25);
        CALL_REMOVEXATTR(26);
        CALL_REMOVEXATTR(28);
        CALL_REMOVEXATTR(29);
        CALL_REMOVEXATTR(30);
        CALL_REMOVEXATTR(34);
        CALL_REMOVEXATTR(38);
#undef CALL_REMOVEXATTR
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_bmap(struct fuse_fs* fs, const char* path, size_t blocksize, uint64_t *idx) {
    clobber_context_user_data(fs);
    /* bmap() appeared on FUSE 2.6. */
    switch (fs->op_version) {
    case 11:
    case 22:
    case 23:
    case 25:
        return -ENOSYS;
#define CALL_BMAP(VER)                                                  \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->bmap) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->bmap(path, blocksize, idx); \
        else                                                            \
            return -ENOSYS
        CALL_BMAP(26);
        CALL_BMAP(28);
        CALL_BMAP(29);
        CALL_BMAP(30);
        CALL_BMAP(34);
        CALL_BMAP(38);
#undef CALL_BMAP
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int fuse_fs_ioctl_v28(struct fuse_fs* fs, const char* path, int cmd, void* arg,
                      struct fuse_file_info* fi, unsigned int flags, void* data) {
    return fuse_fs_ioctl_v35(fs, path, (unsigned int)cmd, arg, fi, flags, data);
}

int fuse_fs_ioctl_v35(struct fuse_fs* fs, const char* path, unsigned int cmd, void* arg,
                      struct fuse_file_info* fi, unsigned int flags, void* data) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* ioctl() appeared on FUSE 2.8 but with (int)cmd. */
    case 11:
    case 22:
    case 23:
    case 25:
    case 26:
        return -ENOSYS;
#define CALL_OLD_IOCTL(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->ioctl) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->ioctl(path, (int)cmd, arg, fi, flags, data); \
        else                                                            \
            return -ENOSYS
        CALL_OLD_IOCTL(28);
        CALL_OLD_IOCTL(29);
        CALL_OLD_IOCTL(30);
        CALL_OLD_IOCTL(34);
#undef CALL_OLD_IOCTL

        /* It was then changed to (unsigned int)cmd on FUSE 3.5. */
#define CALL_IOCTL(VER)                                                 \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->ioctl) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->ioctl(path, cmd, arg, fi, flags, data); \
        else                                                            \
            return -ENOSYS
        CALL_IOCTL(35);
        CALL_IOCTL(38);
#undef CALL_IOCTL
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_poll(struct fuse_fs* fs, const char* path, struct fuse_file_info* fi,
             struct fuse_pollhandle* ph, unsigned* reventsp) {
    clobber_context_user_data(fs);
    /* poll() appeared on FUSE 2.8. */
    switch (fs->op_version) {
    case 11:
    case 22:
    case 23:
    case 25:
    case 26:
        return -ENOSYS;
#define CALL_POLL(VER)                                                  \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->poll) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->poll(path, fi, ph, reventsp); \
        else                                                            \
            return -ENOSYS
        CALL_POLL(28);
        CALL_POLL(29);
        CALL_POLL(30);
        CALL_POLL(34);
        CALL_POLL(38);
#undef CALL_POLL
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

int
fuse_fs_fallocate(struct fuse_fs* fs, const char* path, int mode, off_t offset,
                  off_t length, struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    /* fallocate() appeared on FUSE 2.9. */
    switch (fs->op_version) {
    case 11:
    case 22:
    case 23:
    case 25:
    case 26:
    case 28:
        return -ENOSYS;
#define CALL_FALLOCATE(VER)                                             \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fallocate) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->fallocate(path, mode, offset, length, fi); \
        else                                                            \
            return -ENOSYS
        CALL_FALLOCATE(29);
        CALL_FALLOCATE(30);
        CALL_FALLOCATE(34);
        CALL_FALLOCATE(38);
#undef CALL_FALLOCATE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

ssize_t
fuse_fs_copy_file_range(struct fuse_fs *fs,
                        const char *path_in, struct fuse_file_info *fi_in, off_t off_in,
                        const char *path_out, struct fuse_file_info *fi_out, off_t off_out,
                        size_t len, int flags) {
    clobber_context_user_data(fs);
    /* copy_file_range() appeared on FUSE 3.4. */
    switch (fs->op_version) {
    case 11:
    case 22:
    case 23:
    case 25:
    case 26:
    case 28:
    case 29:
    case 30:
        return -ENOSYS;
#define CALL_COPY_FILE_RANGE(VER)                                       \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->copy_file_range) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->copy_file_range(path_in, fi_in, off_in, path_out, fi_out, off_out, len, flags); \
        else                                                            \
            return -ENOSYS
        CALL_COPY_FILE_RANGE(34);
        CALL_COPY_FILE_RANGE(38);
#undef CALL_COPY_FILE_RANGE
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

off_t
fuse_fs_lseek(struct fuse_fs* fs, const char* path, off_t off, int whence,
              struct fuse_file_info* fi) {
    clobber_context_user_data(fs);
    /* lseek() appeared on FUSE 3.8. */
    switch (fs->op_version) {
    case 11:
    case 22:
    case 23:
    case 25:
    case 26:
    case 28:
    case 29:
    case 30:
    case 34:
        return -ENOSYS;
#define CALL_LSEEK(VER)                         \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->lseek) \
            return ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->lseek(path, off, whence, fi); \
        else                                                            \
            return -ENOSYS
        CALL_LSEEK(38);
#undef CALL_LSEEK
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

void
fuse_fs_init_v27(struct fuse_fs *fs, struct fuse_conn_info *conn) {
    fuse_fs_init_v30(fs, conn, NULL);
}

void
fuse_fs_init_v30(struct fuse_fs* fs, struct fuse_conn_info* conn,
                 struct fuse_config* cfg) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
    case 11:
    case 21:
    case 22:
        break;

        /* init() appeared on FUSE 2.3 as init(void). */
#define CALL_NULLARY_INIT(VER)                                          \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->init) \
            fs->user_data = ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->init(); \
        break
        CALL_NULLARY_INIT(23);
        CALL_NULLARY_INIT(25);
#undef CALL_NULLARY_INIT

        /* It was changed to init(struct fuse_conn_info*) on FUSE
         * 2.6. */
#define CALL_UNARY_INIT(VER)                                            \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->init) \
            fs->user_data = ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->init(conn); \
        break
        CALL_UNARY_INIT(26);
        CALL_UNARY_INIT(28);
        CALL_UNARY_INIT(29);
#undef CALL_INIT

        /* It was again changed to init(struct fuse_conn_info*, struct
         * fuse_config*) on FUSE 3.0. */
#define CALL_BINARY_INIT(VER)                   \
    case VER:                                   \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->init) \
            fs->user_data = ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->init(conn, cfg); \
        break
        CALL_BINARY_INIT(30);
        CALL_BINARY_INIT(34);
        CALL_BINARY_INIT(38);
#undef CALL_BINARY_INIT
    default:
        UNKNOWN_VERSION(fs->op_version);
    }
}

void
fuse_fs_destroy(struct fuse_fs *fs) {
    clobber_context_user_data(fs);
    switch (fs->op_version) {
        /* destroy() appeared on FUSE 2.3. */
    case 11:
    case 21:
    case 22:
        break;

#define CALL_DESTROY(VER)                                               \
    case VER:                                                           \
        if (((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->destroy) \
            ((const struct __CONCAT(fuse_operations_v,VER)*)fs->op)->destroy(fs->user_data); \
        break
        CALL_DESTROY(23);
        CALL_DESTROY(25);
        CALL_DESTROY(26);
        CALL_DESTROY(28);
        CALL_DESTROY(29);
        CALL_DESTROY(30);
        CALL_DESTROY(34);
        CALL_DESTROY(38);
#undef CALL_DESTROY
    default:
        UNKNOWN_VERSION(fs->op_version);
    }

    /* fuse_fs_destroy(3) also deallocates struct fuse_fs itself. */
    free(fs->op);
    free(fs);
}
