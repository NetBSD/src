/* $NetBSD: fuse.h,v 1.33 2022/01/22 08:06:21 pho Exp $ */

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
#define _REFUSE_MAJOR_VERSION_	2
#define _REFUSE_MINOR_VERSION_	6

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


typedef int (*fuse_fill_dir_t)(void *, const char *, const struct stat *, off_t);
typedef int (*fuse_dirfil_t)(fuse_dirh_t, const char *, int, ino_t);

/*
 * These operations shadow those in puffs_usermount, and are used
 * as a table of callbacks to make when file system requests come
 * in.
 *
 * NOTE: keep same order as fuse
 */
struct fuse_operations {
	int	(*getattr)(const char *, struct stat *);
	int	(*readlink)(const char *, char *, size_t);
	int	(*getdir)(const char *, fuse_dirh_t, fuse_dirfil_t);
	int	(*mknod)(const char *, mode_t, dev_t);
	int	(*mkdir)(const char *, mode_t);
	int	(*unlink)(const char *);
	int	(*rmdir)(const char *);
	int	(*symlink)(const char *, const char *);
	int	(*rename)(const char *, const char *);
	int	(*link)(const char *, const char *);
	int	(*chmod)(const char *, mode_t);
	int	(*chown)(const char *, uid_t, gid_t);
	int	(*truncate)(const char *, off_t);
	int	(*utime)(const char *, struct utimbuf *);
	int	(*open)(const char *, struct fuse_file_info *);
	int	(*read)(const char *, char *, size_t, off_t, struct fuse_file_info *);
	int	(*write)(const char *, const char *, size_t, off_t, struct fuse_file_info *);
	int	(*statfs)(const char *, struct statvfs *);
	int	(*flush)(const char *, struct fuse_file_info *);
	int	(*release)(const char *, struct fuse_file_info *);
	int	(*fsync)(const char *, int, struct fuse_file_info *);
	int	(*setxattr)(const char *, const char *, const char *, size_t, int);
	int	(*getxattr)(const char *, const char *, char *, size_t);
	int	(*listxattr)(const char *, char *, size_t);
	int	(*removexattr)(const char *, const char *);
	int	(*opendir)(const char *, struct fuse_file_info *);
	int	(*readdir)(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
	int	(*releasedir)(const char *, struct fuse_file_info *);
	int	(*fsyncdir)(const char *, int, struct fuse_file_info *);
	void	*(*init)(struct fuse_conn_info *);
	void	(*destroy)(void *);
	int	(*access)(const char *, int);
	int	(*create)(const char *, mode_t, struct fuse_file_info *);
	int	(*ftruncate)(const char *, off_t, struct fuse_file_info *);
	int	(*fgetattr)(const char *, struct stat *, struct fuse_file_info *);
	int	(*lock)(const char *, struct fuse_file_info *, int, struct flock *);
	int	(*utimens)(const char *, const struct timespec *);
	int	(*bmap)(const char *, size_t , uint64_t *);
};


struct fuse *fuse_new(struct fuse_args *,
	const struct fuse_operations *, size_t, void *);

int fuse_mount(struct fuse *, const char *);
void fuse_unmount(struct fuse *);

int fuse_main_real(int, char **, const struct fuse_operations *, size_t, void *);
/* Functions that have existed since the beginning and have never
 * changed between API versions. */
int fuse_loop(struct fuse *);
void fuse_exit(struct fuse *);
void fuse_destroy(struct fuse *);
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

#if FUSE_USE_VERSION == 22
#define fuse_unmount fuse_unmount_compat22
#endif

void fuse_unmount_compat22(const char *);

#if FUSE_USE_VERSION >= 26
#define fuse_main(argc, argv, op, user_data) \
            fuse_main_real(argc, argv, op, sizeof(*(op)), user_data)
#else
#define fuse_main(argc, argv, op) \
            fuse_main_real(argc, argv, op, sizeof(*(op)), NULL)
#endif
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

#ifdef __cplusplus
}
#endif

#endif
