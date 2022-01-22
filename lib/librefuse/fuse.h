/* $NetBSD: fuse.h,v 1.34 2022/01/22 08:09:39 pho Exp $ */

/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
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
#ifndef FUSE_H_
#define FUSE_H_	20211204

#include <fcntl.h>
#include <fuse_opt.h>
#include <refuse/buf.h>
#include <refuse/chan.h>
#include <refuse/legacy.h>
#include <refuse/poll.h>
#include <refuse/session.h>
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <utime.h>

/* This used to be (maj) * 10 + (min) until FUSE 3.10, and then
 * changed to (maj) * 100 + (min). We can't just use the "newer"
 * definition because filesystems in the wild still use the older one
 * in their FUSE_USE_VERSION request. */
#define FUSE_MAKE_VERSION(maj, min)					\
	(((maj) > 3 || ((maj) == 3 && (min) >= 10))			\
	? (maj) * 100 + (min)						\
	: (maj) *  10 + (min))

/* The latest version of FUSE API currently provided by ReFUSE. This
 * is an implementation detail. User code should not rely on this
 * constant. */
#define _REFUSE_MAJOR_VERSION_	3
#define _REFUSE_MINOR_VERSION_	10

#define _REFUSE_VERSION_	FUSE_MAKE_VERSION(_REFUSE_MAJOR_VERSION_, _REFUSE_MINOR_VERSION_)

/* FUSE_USE_VERSION is expected to be defined by user code to
 * determine the API to be used. Although defining this macro is
 * mandatory in the original FUSE implementation, refuse hasn't
 * required this so we only emit a warning if it's undefined. */
#if defined(FUSE_USE_VERSION)
#	if FUSE_USE_VERSION > _REFUSE_VERSION_
#		warning "The requested API version is higher than the latest one supported by refuse."
#	elif FUSE_USE_VERSION < 11
#		warning "The requested API version is lower than the oldest one supported by refuse."
#	endif
#else
#	if !defined(_REFUSE_IMPLEMENTATION_)
#		warning "User code including <fuse.h> should define FUSE_USE_VERSION before including this header. Defaulting to the latest version."
#		define FUSE_USE_VERSION	_REFUSE_VERSION_
#	endif
#endif

/* FUSE_VERSION is supposed to be the latest version of FUSE API
 * supported by the library. However, due to the way how original FUSE
 * is implemented, some filesystems set FUSE_USE_VERSION to some old
 * one and then expect the actual API version exposed by the library
 * to be something newer if FUSE_VERSION is higher than that. ReFUSE
 * doesn't work that way, so this has to be always identical to
 * FUSE_USE_VERSION.
 */
#if defined(FUSE_USE_VERSION)
#	define FUSE_VERSION		FUSE_USE_VERSION
#	define FUSE_MAJOR_VERSION	(FUSE_VERSION / 10)
#	define FUSE_MINOR_VERSION	(FUSE_VERSION % 10)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct fuse;

struct fuse_file_info {
	int32_t		flags;
	uint32_t	fh_old;			/* Removed as of FUSE 3.0. */
	int32_t		writepage:1;
	uint32_t	direct_io:1;
	uint32_t	keep_cache:1;
	uint32_t	flush:1;
	uint32_t	nonseekable:1;		/* Added on FUSE 2.8. */
	uint32_t	flock_release:1;	/* Added on FUSE 2.9. */
	uint32_t	cache_readdir:1;	/* Added on FUSE 3.5. */
	uint32_t	padding:26;
	uint64_t	fh;
	uint64_t	lock_owner;		/* Added on FUSE 2.6. */
	uint32_t	poll_events;		/* Added on FUSE 3.0. */
};

struct fuse_conn_info {
	uint32_t	proto_major;
	uint32_t	proto_minor;
	uint32_t	async_read;		/* Removed as of FUSE 3.0. */
	uint32_t	max_write;
	uint32_t	max_read;		/* Added on FUSE 3.0. */
	uint32_t	max_readahead;
	uint32_t	capable;		/* Added on FUSE 2.8. */
	uint32_t	want;			/* Added on FUSE 2.8. */
	uint32_t	max_background;		/* Added on FUSE 3.0. */
	uint32_t	congestion_threshold;	/* Added on FUSE 3.0. */
	uint32_t	time_gran;		/* Added on FUSE 3.0. */
	uint32_t	reserved[22];
};

/* equivalent'ish of puffs_cc */
struct fuse_context {
	struct fuse	*fuse;
	uid_t		uid;
	gid_t		gid;
	pid_t		pid;
	void		*private_data;
	mode_t		umask;			/* Added on FUSE 2.8. */
};

/* Capability bits for fuse_conn_info.capable and
 * fuse_conn_info.want */
#define FUSE_CAP_ASYNC_READ		(1 << 0)
#define FUSE_CAP_POSIX_LOCKS		(1 << 1)
#define FUSE_CAP_ATOMIC_O_TRUNC		(1 << 3)
#define FUSE_CAP_EXPORT_SUPPORT		(1 << 4)
#define FUSE_CAP_BIG_WRITES		(1 << 5)	/* Removed as of FUSE 3.0. */
#define FUSE_CAP_DONT_MASK		(1 << 6)
#define FUSE_CAP_SPLICE_WRITE		(1 << 7)	/* Added on FUSE 3.0. */
#define FUSE_CAP_SPLICE_MOVE		(1 << 8)	/* Added on FUSE 3.0. */
#define FUSE_CAP_SPLICE_READ		(1 << 9)	/* Added on FUSE 3.0. */
#define FUSE_CAP_FLOCK_LOCKS		(1 << 10)	/* Added on FUSE 3.0. */
#define FUSE_CAP_IOCTL_DIR		(1 << 11)	/* Added on FUSE 3.0. */
#define FUSE_CAP_AUTO_INVAL_DATA	(1 << 12)	/* Added on FUSE 3.0. */
#define FUSE_CAP_READDIRPLUS		(1 << 13)	/* Added on FUSE 3.0. */
#define FUSE_CAP_READDIRPLUS_AUTO	(1 << 14)	/* Added on FUSE 3.0. */
#define FUSE_CAP_ASYNC_DIO		(1 << 15)	/* Added on FUSE 3.0. */
#define FUSE_CAP_WRITEBACK_CACHE	(1 << 16)	/* Added on FUSE 3.0. */
#define FUSE_CAP_NO_OPEN_SUPPORT	(1 << 17)	/* Added on FUSE 3.0. */
#define FUSE_CAP_PARALLEL_DIROPS	(1 << 18)	/* Added on FUSE 3.0. */
#define FUSE_CAP_POSIX_ACL		(1 << 19)	/* Added on FUSE 3.0. */
#define FUSE_CAP_HANDLE_KILLPRIV	(1 << 20)	/* Added on FUSE 3.0. */
#define FUSE_CAP_CACHE_SYMLINKS		(1 << 23)	/* Added on FUSE 3.10. */
#define FUSE_CAP_NO_OPENDIR_SUPPORT	(1 << 24)	/* Added on FUSE 3.5. */

/* ioctl flags */
#define FUSE_IOCTL_COMPAT	(1 << 0)
#define FUSE_IOCTL_UNRESTRICTED	(1 << 1)
#define FUSE_IOCTL_RETRY	(1 << 2)
#define FUSE_IOCTL_DIR		(1 << 4)	/* Added on FUSE 2.9. */
#define FUSE_IOCTL_MAX_IOV	256

/* readdir() flags, appeared on FUSE 3.0. */
enum fuse_readdir_flags {
	FUSE_READDIR_PLUS	= (1 << 0),
};
enum fuse_fill_dir_flags {
	FUSE_FILL_DIR_PLUS	= (1 << 1),
};

/* Configuration of the high-level API, appeared on FUSE 3.0. */
struct fuse_config {
	int		set_gid;
	unsigned int	gid;
	int		set_uid;
	unsigned int	uid;
	int		set_mode;
	unsigned int	umask;
	double		entry_timeout;
	double		negative_timeout;
	double		attr_timeout;
	int		intr;
	int		intr_signal;
	int		remember;
	int		hard_remove;
	int		use_ino;
	int		readdir_ino;
	int		direct_io;
	int		kernel_cache;
	int		auto_cache;
	int		ac_attr_timeout_set;
	double		ac_attr_timeout;
	int		nullpath_ok;
};

/* Configuration of fuse_loop_mt(), appeared on FUSE 3.2. */
struct fuse_loop_config {
	int		clone_fd;
	unsigned int	max_idle_threads;
};

/**
 * Argument list
 */
struct fuse_args {
	int	argc;
	char	**argv;
	int	allocated;
};

/**
 * Initializer for 'struct fuse_args'
 */
#define FUSE_ARGS_INIT(argc, argv) { argc, argv, 0 }

/* Functions that have existed since the beginning and have never
 * changed between API versions. */
int fuse_loop(struct fuse *);
void fuse_exit(struct fuse *);
struct fuse_context *fuse_get_context(void);

/* Print available library options. Appeared on FUSE 3.1. */
void fuse_lib_help(struct fuse_args *args);

/* Daemonize the calling process. Appeared on FUSE 2.6.
 *
 * NOTE: This function used to have a wrong prototype in librefuse at
 * the time when FUSE_H_ < 20211204. */
int fuse_daemonize(int foreground) __RENAME(fuse_daemonize_rev1);

/* Check if a request has been interrupted. Appeared on FUSE 2.6. */
int fuse_interrupted(void);

/* Invalidate cache for a given path. Appeared on FUSE 3.2. */
int fuse_invalidate_path(struct fuse *fuse, const char *path);

/* Get the version number of the library. Appeared on FUSE 2.7. */
int fuse_version(void);

/* Get the version string of the library. Appeared on FUSE 3.0. */
const char *fuse_pkgversion(void);

/* Get the current supplementary group IDs for the current request, or
 * return -errno on failure. Appeared on FUSE 2.8. */
int fuse_getgroups(int size, gid_t list[]);

/* Start the cleanup thread when using option "-oremember". Appeared
 * on FUSE 2.9. */
int fuse_start_cleanup_thread(struct fuse *fuse);

/* Stop the cleanup thread when using "-oremember". Appeared on FUSE
 * 2.9. */
void fuse_stop_cleanup_thread(struct fuse *fuse);

/* Iterate over cache removing stale entries, used in conjunction with
 * "-oremember". Return the number of seconds until the next
 * cleanup. Appeared on FUSE 2.9. */
int fuse_clean_cache(struct fuse *fuse);

/* Generic implementation of fuse_main(). The exact type of "op" is
 * determined by op_version. This is only an implementation detail:
 * user code should never call this directly. */
int __fuse_main(int argc, char* argv[],
		const void* op, int op_version, void* user_data);

/* NOTE: Compatibility headers are included
 * unconditionally. Declarations in these headers all have a version
 * postfix, and need to be aliased depending on FUSE_USE_VERSION. */
#include <refuse/v11.h>
#include <refuse/v21.h>
#include <refuse/v22.h>
#include <refuse/v23.h>
#include <refuse/v25.h>
#include <refuse/v26.h>
#include <refuse/v28.h>
#include <refuse/v29.h>
#include <refuse/v30.h>
#include <refuse/v32.h>
#include <refuse/v34.h>
#include <refuse/v35.h>
#include <refuse/v38.h>

/* NOTE: refuse/fs.h relies on some typedef's in refuse/v*.h */
#include <refuse/fs.h>

#define _MK_FUSE_OPERATIONS_(VER)	__CONCAT(fuse_operations_v,VER)

/* Version specific types and functions. */
#if defined(FUSE_USE_VERSION)
/* ===== FUSE 1.x ===== */
#	if FUSE_USE_VERSION < 21
		/* Types */
#		define _FUSE_OP_VERSION__	11 /* Implementation detail */
#		define fuse_dirfil_t		fuse_dirfil_t_v11
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_mount		fuse_mount_v11
#		define fuse_unmount		fuse_unmount_v11
static __inline struct fuse *
fuse_new(int fd, int flags, const struct fuse_operations *op) {
    return fuse_new_v11(fd, flags, op, _FUSE_OP_VERSION__);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11

/* ===== FUSE 2.1 ===== */
#	elif FUSE_USE_VERSION == 21
		/* Types */
#		define _FUSE_OP_VERSION__	21
#		define fuse_dirfil_t		fuse_dirfil_t_v11
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_mount		fuse_mount_v21
#		define fuse_unmount		fuse_unmount_v11
static __inline struct fuse *
fuse_new(int fd, const char *opts, const struct fuse_operations *op) {
    return fuse_new_v21(fd, opts, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11

/* ===== FUSE 2.2 ===== */
#	elif FUSE_USE_VERSION == 22
		/* Types */
#		define _FUSE_OP_VERSION__	22
#		define fuse_dirfil_t		fuse_dirfil_t_v22
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_mount		fuse_mount_v21
#		define fuse_unmount		fuse_unmount_v11
static __inline struct fuse *
fuse_new(int fd, const char *opts, const struct fuse_operations *op) {
    return fuse_new_v21(fd, opts, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11
static __inline struct fuse *
fuse_setup(int argc, char *argv[], const struct fuse_operations *op,
	   size_t op_size __attribute__((__unused__)),
	   char **mountpoint, int *multithreaded, int *fd) {
    return fuse_setup_v22(argc, argv, op, _FUSE_OP_VERSION__,
			  mountpoint, multithreaded, fd);
}
#		define fuse_teardown		fuse_teardown_v22

/* ===== FUSE 2.3, 2.4 ===== */
#	elif FUSE_USE_VERSION >= 23 && FUSE_USE_VERSION <= 24
		/* Types */
#		define _FUSE_OP_VERSION__	23
#		define fuse_dirfil_t		fuse_dirfil_t_v22
#		define fuse_fill_dir_t		fuse_fill_dir_t_v23
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_mount		fuse_mount_v21
#		define fuse_unmount		fuse_unmount_v11
static __inline struct fuse *
fuse_new(int fd, const char *opts, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__))) {
    return fuse_new_v21(fd, opts, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11
static __inline struct fuse *
fuse_setup(int argc, char *argv[], const struct fuse_operations *op,
	   size_t op_size __attribute__((__unused__)),
	   char **mountpoint, int *multithreaded, int *fd) {
    return fuse_setup_v22(argc, argv, op, _FUSE_OP_VERSION__,
			  mountpoint, multithreaded, fd);
}
#		define fuse_teardown		fuse_teardown_v22

/* ===== FUSE 2.5 ===== */
#	elif FUSE_USE_VERSION == 25
		/* Types */
#		define _FUSE_OP_VERSION__	25
#		define fuse_dirfil_t		fuse_dirfil_t_v22
#		define fuse_fill_dir_t		fuse_fill_dir_t_v23
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_mount		fuse_mount_v25
#		define fuse_unmount		fuse_unmount_v11
static __inline struct fuse *
fuse_new(int fd, struct fuse_args *args, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__))) {
    return fuse_new_v25(fd, args, op, _FUSE_OP_VERSION__, NULL);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11
static __inline struct fuse *
fuse_setup(int argc, char *argv[], const struct fuse_operations *op,
	   size_t op_size __attribute__((__unused__)),
	   char **mountpoint, int *multithreaded, int *fd) {
    return fuse_setup_v22(argc, argv, op, _FUSE_OP_VERSION__,
			  mountpoint, multithreaded, fd);
}
#		define fuse_teardown		fuse_teardown_v22
#		define fuse_parse_cmdline	fuse_parse_cmdline_v25

/* ===== FUSE 2.6, 2.7 ===== */
#	elif FUSE_USE_VERSION >= 26 && FUSE_USE_VERSION <= 27
		/* Types */
#		define _FUSE_OP_VERSION__	26
#		define fuse_dirfil_t		fuse_dirfil_t_v22
#		define fuse_fill_dir_t		fuse_fill_dir_t_v23
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v27
#		define fuse_fs_rename		fuse_fs_rename_v27
#		define fuse_fs_chmod		fuse_fs_chmod_v27
#		define fuse_fs_chown		fuse_fs_chown_v27
#		define fuse_fs_readdir		fuse_fs_readdir_v27
#		define fuse_fs_truncate		fuse_fs_truncate_v27
#		define fuse_fs_utimens		fuse_fs_utimens_v27
#		define fuse_fs_init		fuse_fs_init_v27
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v26
#		define fuse_unmount		fuse_unmount_v26
static __inline struct fuse *
fuse_new(struct fuse_chan *ch, struct fuse_args *args,
	 const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v26(ch, args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11
static __inline struct fuse *
fuse_setup(int argc, char *argv[], const struct fuse_operations *op,
	   size_t op_size __attribute__((__unused__)),
	   char **mountpoint, int *multithreaded, void *user_data) {
    return fuse_setup_v26(argc, argv, op, _FUSE_OP_VERSION__,
			  mountpoint, multithreaded, user_data);
}
#		define fuse_teardown		fuse_teardown_v26
#		define fuse_parse_cmdline	fuse_parse_cmdline_v25

/* ===== FUSE 2.8 ===== */
#	elif FUSE_USE_VERSION == 28
		/* Types */
#		define _FUSE_OP_VERSION__	28
#		define fuse_dirfil_t		fuse_dirfil_t_v22
#		define fuse_fill_dir_t		fuse_fill_dir_t_v23
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v27
#		define fuse_fs_rename		fuse_fs_rename_v27
#		define fuse_fs_chmod		fuse_fs_chmod_v27
#		define fuse_fs_chown		fuse_fs_chown_v27
#		define fuse_fs_readdir		fuse_fs_readdir_v27
#		define fuse_fs_truncate		fuse_fs_truncate_v27
#		define fuse_fs_utimens		fuse_fs_utimens_v27
#		define fuse_fs_ioctl		fuse_fs_ioctl_v28
#		define fuse_fs_init		fuse_fs_init_v27
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v26
#		define fuse_unmount		fuse_unmount_v26
static __inline struct fuse *
fuse_new(struct fuse_chan *ch, struct fuse_args *args,
	 const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v26(ch, args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11
static __inline struct fuse *
fuse_setup(int argc, char *argv[], const struct fuse_operations *op,
	   size_t op_size __attribute__((__unused__)),
	   char **mountpoint, int *multithreaded, void *user_data) {
    return fuse_setup_v26(argc, argv, op, _FUSE_OP_VERSION__,
			  mountpoint, multithreaded, user_data);
}
#		define fuse_teardown		fuse_teardown_v26
#		define fuse_parse_cmdline	fuse_parse_cmdline_v25

/* ===== FUSE 2.9 ===== */
#	elif FUSE_USE_VERSION == 29
		/* Types */
#		define _FUSE_OP_VERSION__	29
#		define fuse_dirfil_t		fuse_dirfil_t_v22
#		define fuse_fill_dir_t		fuse_fill_dir_t_v23
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v27
#		define fuse_fs_rename		fuse_fs_rename_v27
#		define fuse_fs_chmod		fuse_fs_chmod_v27
#		define fuse_fs_chown		fuse_fs_chown_v27
#		define fuse_fs_readdir		fuse_fs_readdir_v27
#		define fuse_fs_truncate		fuse_fs_truncate_v27
#		define fuse_fs_utimens		fuse_fs_utimens_v27
#		define fuse_fs_ioctl		fuse_fs_ioctl_v28
#		define fuse_fs_init		fuse_fs_init_v27
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v26
#		define fuse_unmount		fuse_unmount_v26
static __inline struct fuse *
fuse_new(struct fuse_chan *ch, struct fuse_args *args,
	 const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v26(ch, args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v11
#		define fuse_loop_mt		fuse_loop_mt_v11
static __inline struct fuse *
fuse_setup(int argc, char *argv[], const struct fuse_operations *op,
	   size_t op_size __attribute__((__unused__)),
	   char **mountpoint, int *multithreaded, void *user_data) {
    return fuse_setup_v26(argc, argv, op, _FUSE_OP_VERSION__,
			  mountpoint, multithreaded, user_data);
}
#		define fuse_teardown		fuse_teardown_v26
#		define fuse_parse_cmdline	fuse_parse_cmdline_v25

/* ===== FUSE 3.0, 3.1 ===== */
#	elif FUSE_USE_VERSION >= 30 && FUSE_USE_VERSION <= 31
		/* Types */
#		define _FUSE_OP_VERSION__	30
#		define fuse_fill_dir_t		fuse_fill_dir_t_v30
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v30
#		define fuse_fs_rename		fuse_fs_rename_v30
#		define fuse_fs_chmod		fuse_fs_chmod_v30
#		define fuse_fs_chown		fuse_fs_chown_v30
#		define fuse_fs_readdir		fuse_fs_readdir_v30
#		define fuse_fs_truncate		fuse_fs_truncate_v30
#		define fuse_fs_utimens		fuse_fs_utimens_v30
#		define fuse_fs_ioctl		fuse_fs_ioctl_v28
#		define fuse_fs_init		fuse_fs_init_v30
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v30
#		define fuse_unmount		fuse_unmount_v30
static __inline struct fuse *
fuse_new(struct fuse_args *args, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v30(args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v30
#		define fuse_loop_mt		fuse_loop_mt_v30
		/* fuse_setup(3) and fuse_teardown(3) have been removed as of FUSE 3.0. */
#		define fuse_parse_cmdline	fuse_parse_cmdline_v30

/* ===== FUSE 3.2, 3.3 ===== */
#	elif FUSE_USE_VERSION >= 32 && FUSE_USE_VERSION <= 33
		/* Types */
#		define _FUSE_OP_VERSION__	30
#		define fuse_fill_dir_t		fuse_fill_dir_t_v30
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v30
#		define fuse_fs_rename		fuse_fs_rename_v30
#		define fuse_fs_chmod		fuse_fs_chmod_v30
#		define fuse_fs_chown		fuse_fs_chown_v30
#		define fuse_fs_readdir		fuse_fs_readdir_v30
#		define fuse_fs_truncate		fuse_fs_truncate_v30
#		define fuse_fs_utimens		fuse_fs_utimens_v30
#		define fuse_fs_ioctl		fuse_fs_ioctl_v28
#		define fuse_fs_init		fuse_fs_init_v30
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v30
#		define fuse_unmount		fuse_unmount_v30
static __inline struct fuse *
fuse_new(struct fuse_args *args, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v30(args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v30
#		define fuse_loop_mt		fuse_loop_mt_v32
#		define fuse_parse_cmdline	fuse_parse_cmdline_v30

/* ===== FUSE 3.4 ===== */
#	elif FUSE_USE_VERSION >= 34
		/* Types */
#		define _FUSE_OP_VERSION__	34
#		define fuse_fill_dir_t		fuse_fill_dir_t_v30
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v30
#		define fuse_fs_rename		fuse_fs_rename_v30
#		define fuse_fs_chmod		fuse_fs_chmod_v30
#		define fuse_fs_chown		fuse_fs_chown_v30
#		define fuse_fs_readdir		fuse_fs_readdir_v30
#		define fuse_fs_truncate		fuse_fs_truncate_v30
#		define fuse_fs_utimens		fuse_fs_utimens_v30
#		define fuse_fs_ioctl		fuse_fs_ioctl_v28
#		define fuse_fs_init		fuse_fs_init_v30
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v30
#		define fuse_unmount		fuse_unmount_v30
static __inline struct fuse *
fuse_new(struct fuse_args *args, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v30(args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v30
#		define fuse_loop_mt		fuse_loop_mt_v32
#		define fuse_parse_cmdline	fuse_parse_cmdline_v30

/* ===== FUSE 3.5, 3.6, 3.7 ===== */
#	elif FUSE_USE_VERSION >= 35 && FUSE_USE_VERSION <= 37
		/* Types */
#		define _FUSE_OP_VERSION__	35
#		define fuse_fill_dir_t		fuse_fill_dir_t_v30
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v30
#		define fuse_fs_rename		fuse_fs_rename_v30
#		define fuse_fs_chmod		fuse_fs_chmod_v30
#		define fuse_fs_chown		fuse_fs_chown_v30
#		define fuse_fs_readdir		fuse_fs_readdir_v30
#		define fuse_fs_truncate		fuse_fs_truncate_v30
#		define fuse_fs_utimens		fuse_fs_utimens_v30
#		define fuse_fs_ioctl		fuse_fs_ioctl_v35
#		define fuse_fs_init		fuse_fs_init_v30
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v30
#		define fuse_unmount		fuse_unmount_v30
static __inline struct fuse *
fuse_new(struct fuse_args *args, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v30(args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v30
#		define fuse_loop_mt		fuse_loop_mt_v32
#		define fuse_parse_cmdline	fuse_parse_cmdline_v30

/* ===== FUSE 3.8, 3.9, 3.10 ===== */
#	elif FUSE_USE_VERSION >= 38 && FUSE_USE_VERSION <= 310
		/* Types */
#		define _FUSE_OP_VERSION__	38
#		define fuse_fill_dir_t		fuse_fill_dir_t_v30
#		define fuse_operations		_MK_FUSE_OPERATIONS_(_FUSE_OP_VERSION__)
		/* Functions */
static __inline struct fuse_fs *
fuse_fs_new(const struct fuse_operations *op,
	    size_t op_size __attribute__((__unused__)), void *user_data) {
    return __fuse_fs_new(op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_fs_getattr		fuse_fs_getattr_v30
#		define fuse_fs_rename		fuse_fs_rename_v30
#		define fuse_fs_chmod		fuse_fs_chmod_v30
#		define fuse_fs_chown		fuse_fs_chown_v30
#		define fuse_fs_readdir		fuse_fs_readdir_v30
#		define fuse_fs_truncate		fuse_fs_truncate_v30
#		define fuse_fs_utimens		fuse_fs_utimens_v30
#		define fuse_fs_ioctl		fuse_fs_ioctl_v35
#		define fuse_fs_init		fuse_fs_init_v30
static __inline int
fuse_main(int argc, char *argv[], const struct fuse_operations *op, void* user_data) {
    return __fuse_main(argc, argv, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_mount		fuse_mount_v30
#		define fuse_unmount		fuse_unmount_v30
static __inline struct fuse *
fuse_new(struct fuse_args *args, const struct fuse_operations *op,
	 size_t op_size __attribute__((__unused__)), void *user_data) {
    return fuse_new_v30(args, op, _FUSE_OP_VERSION__, user_data);
}
#		define fuse_destroy		fuse_destroy_v30
#		define fuse_loop_mt		fuse_loop_mt_v32
#		define fuse_parse_cmdline	fuse_parse_cmdline_v30

#	endif
#endif /* defined(FUSE_USE_VERSION) */

#ifdef __cplusplus
}
#endif

#endif
