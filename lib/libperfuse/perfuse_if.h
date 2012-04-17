/*  $NetBSD: perfuse_if.h,v 1.17.2.1 2012/04/17 00:05:30 yamt Exp $ */

/*-
 *  Copyright (c) 2010-2011 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _PERFUSE_IF_H
#define _PERFUSE_IF_H

#ifndef _PATH_PERFUSED
#define _PATH_PERFUSED "/usr/sbin/perfused"
#endif /* _PATH_PERFUSED */
#define _PATH_FUSE "/dev/fuse"
#define FUSE_COMMFD_ENV "_FUSE_COMMFD" 
#define PERFUSE_MOUNT_MAGIC "noFuseRq"
#define PERFUSE_UNKNOWN_INO 0xffffffff
#define PERFUSE_UNKNOWN_NODEID 0xffffffff

/* 
 * Diagnostic flags. This global is used only for DPRINTF/DERR/DWARN
 */
extern int perfuse_diagflags;
#define PDF_FOREGROUND	0x0001	/* we run in foreground */
#define PDF_FUSE	0x0002	/* Display FUSE reqeusts and reply */
#define PDF_DUMP	0x0004	/* Dump FUSE frames */
#define PDF_PUFFS	0x0008	/* Display PUFFS requets and reply */
#define PDF_FH		0x0010	/* File handles */
#define PDF_RECLAIM	0x0020	/* Reclaimed files */
#define PDF_READDIR	0x0040	/* readdir operations */
#define PDF_REQUEUE	0x0080	/* reueued messages */
#define PDF_SYNC	0x0100	/* fsync and dirty flags */
#define PDF_MISC	0x0200	/* Miscelaneous messages */
#define PDF_SYSLOG	0x0400	/* use syslog */
#define PDF_FILENAME	0x0800	/* File names */
#define PDF_RESIZE	0x1000	/* Resize operations */
#define PDF_TRACE	0x2000	/* Trace FUSE calls */

/*
 * Diagnostic functions
 */
#define DPRINTF(fmt, ...) do {						\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_INFO, fmt, ## __VA_ARGS__);			\
									\
	if (perfuse_diagflags & PDF_FOREGROUND)				\
		(void)printf(fmt, ## __VA_ARGS__);			\
} while (0 /* CONSTCOND */)

#define DERRX(status, fmt, ...) do {					\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_ERR, fmt, ## __VA_ARGS__);			\
									\
	if (perfuse_diagflags & PDF_FOREGROUND) {			\
		(void)fprintf(stderr,  fmt, ## __VA_ARGS__);		\
		abort();						\
	} else {							\
		errx(status, fmt, ## __VA_ARGS__);			\
	}								\
} while (0 /* CONSTCOND */)

#define DERR(status, fmt, ...) do {					\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_ERR, fmt ": %m", ## __VA_ARGS__);		\
									\
	if (perfuse_diagflags & PDF_FOREGROUND) {			\
		char strerrbuf[BUFSIZ];					\
									\
		(void)strerror_r(errno, strerrbuf, sizeof(strerrbuf));	\
		(void)fprintf(stderr,  fmt ": %s", ## __VA_ARGS__,	\
		    strerrbuf);						\
		abort();						\
	} else {							\
		err(status, fmt, ## __VA_ARGS__);			\
	}								\
} while (0 /* CONSTCOND */)

#define DWARNX(fmt, ...) do {						\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_WARNING, fmt, ## __VA_ARGS__);		\
									\
	warnx(fmt, ## __VA_ARGS__);					\
} while (0 /* CONSTCOND */)

#define DWARN(fmt, ...) do {						\
									\
	if (perfuse_diagflags & PDF_SYSLOG) 				\
		syslog(LOG_WARNING, fmt ": %m", ## __VA_ARGS__);	\
									\
	warn(fmt, ## __VA_ARGS__);					\
} while (0 /* CONSTCOND */)

/*
 * frame handling callbacks
 */
#ifndef PEFUSE_MSG_T
#define PEFUSE_MSG_T struct perfuse_framebuf
#endif
typedef PEFUSE_MSG_T perfuse_msg_t;

#define PERFUSE_UNSPEC_REPLY_LEN (size_t)-1

enum perfuse_xchg_pb_reply { wait_reply, no_reply };
typedef perfuse_msg_t *(*perfuse_new_msg_fn)(struct puffs_usermount *,
    puffs_cookie_t, int, size_t, const struct puffs_cred *);
typedef int (*perfuse_xchg_msg_fn)(struct puffs_usermount *, 
    perfuse_msg_t *, size_t, enum perfuse_xchg_pb_reply);
typedef void (*perfuse_destroy_msg_fn)(perfuse_msg_t *);
typedef struct fuse_out_header *(*perfuse_get_outhdr_fn)(perfuse_msg_t *);
typedef struct fuse_in_header *(*perfuse_get_inhdr_fn)(perfuse_msg_t *);
typedef char *(*perfuse_get_inpayload_fn)(perfuse_msg_t *);
typedef char *(*perfuse_get_outpayload_fn)(perfuse_msg_t *);
typedef void (*perfuse_umount_fn)(struct puffs_usermount *);

struct perfuse_callbacks {
	perfuse_new_msg_fn pc_new_msg;
	perfuse_xchg_msg_fn pc_xchg_msg;
	perfuse_destroy_msg_fn pc_destroy_msg;
	perfuse_get_inhdr_fn pc_get_inhdr;
	perfuse_get_inpayload_fn pc_get_inpayload;
	perfuse_get_outhdr_fn pc_get_outhdr;
	perfuse_get_outpayload_fn pc_get_outpayload;
	perfuse_umount_fn pc_umount;
};

/* 
 * mount request
 */
struct perfuse_mount_out {
        uint32_t pmo_len;
        int32_t pmo_error;  
        uint64_t pmo_unique;
	char pmo_magic[sizeof(PERFUSE_MOUNT_MAGIC)];
	uint32_t pmo_source_len;
	uint32_t pmo_target_len;
	uint32_t pmo_filesystemtype_len;
	uint32_t pmo_mountflags;
	uint32_t pmo_data_len;
	uint32_t pmo_sock_len;
};

struct perfuse_mount_info {
	const char *pmi_source;
	const char *pmi_target;
	const char *pmi_filesystemtype;
	int pmi_mountflags;
	void *pmi_data;
	uid_t pmi_uid;
};

/*
 * Duplicated from fuse.h to avoid making it public
 */
#ifndef FUSE_BUFSIZE
#define FUSE_MIN_BUFSIZE 0x21000
#define FUSE_PREF_BUFSIZE (sysconf(_SC_PAGESIZE) + 0x1000)
#define FUSE_BUFSIZE MAX(FUSE_PREF_BUFSIZE /* CONSTCOND */, FUSE_MIN_BUFSIZE)
#endif /* FUSE_BUFSIZE */

struct fuse_in_header {
	uint32_t	len;
	uint32_t	opcode;
	uint64_t	unique;
	uint64_t	nodeid;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	pid;
	uint32_t	padding;
};

struct fuse_out_header {
	uint32_t	len;
	int32_t	error;
	uint64_t	unique;
};

__BEGIN_DECLS

struct puffs_usermount *perfuse_init(struct perfuse_callbacks *,
    struct perfuse_mount_info *);
void perfuse_setspecific(struct puffs_usermount *, void *);
void *perfuse_getspecific(struct puffs_usermount *);
uint64_t perfuse_next_unique(struct puffs_usermount *);
uint64_t perfuse_get_nodeid(struct puffs_usermount *, puffs_cookie_t);
int perfuse_inloop(struct puffs_usermount *);
const char *perfuse_opname(int);
void perfuse_fs_init(struct puffs_usermount *);
int perfuse_mainloop(struct puffs_usermount *);
int perfuse_unmount(struct puffs_usermount *);
void perfuse_trace_dump(struct puffs_usermount *, FILE *);

#endif /* _PERFUSE_IF_H */
