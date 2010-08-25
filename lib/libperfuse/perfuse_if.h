/*  $NetBSD: perfuse_if.h,v 1.1 2010/08/25 07:16:00 manu Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
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

#ifndef _REFUSE_PERFUSE_H
#define _REFUSE_PERFUSE_H

#define _PATH_FUSE "/dev/fuse"
#define FUSE_COMMFD_ENV "_FUSE_COMMFD" 
#define PERFUSE_MOUNT_MAGIC "noFuseRq"
#define PERFUSE_UNKNOWN_INO 0xffffffff

/* 
 * Diagnostic flags. This global is used only for DPRINTF/DERR/DWARN
 */
extern int perfuse_diagflags;
#define PDF_FOREGROUND	0x001	/* we run in foreground */
#define PDF_FUSE	0x002	/* Display FUSE reqeusts and reply */
#define PDF_DUMP	0x004	/* Dump FUSE frames */
#define PDF_PUFFS	0x008	/* Display PUFFS requets and reply */
#define PDF_FH		0x010	/* File handles */
#define PDF_RECLAIM	0x020	/* Reclaimed files */
#define PDF_READDIR	0x040	/* readdir operations */
#define PDF_REQUEUE	0x080	/* reueued messages */
#define PDF_MISC	0x100	/* Miscelaneous messages */
#define PDF_SYSLOG	0x200	/* use syslog */

/*
 * Diagnostic functions
 */
#define DPRINTF(fmt, ...) do {						\
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
	char fmterr[BUFSIZ];						\
	char strerrbuf[BUFSIZ];						\
									\
	(void)strerror_r(errno, strerrbuf, sizeof(strerrbuf));		\
	(void)sprintf(fmterr, "%s: %s\n", fmt, strerrbuf);		\
									\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_ERR, fmterr, ## __VA_ARGS__);		\
									\
	if (perfuse_diagflags & PDF_FOREGROUND) {			\
		(void)fprintf(stderr,  fmterr, ## __VA_ARGS__);		\
		abort();						\
	} else {							\
		errx(status, fmt, ## __VA_ARGS__);			\
	}								\
} while (0 /* CONSTCOND */)

#define DWARNX(fmt, ...) do {						\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_WARNING, fmt, ## __VA_ARGS__);		\
									\
	warnx(fmt, ## __VA_ARGS__);					\
} while (0 /* CONSTCOND */)

#define DWARN(fmt, ...) do {						\
	char fmterr[BUFSIZ];						\
	char strerrbuf[BUFSIZ];						\
									\
	(void)strerror_r(errno, strerrbuf, sizeof(strerrbuf));		\
	(void)sprintf(fmterr, "%s: %s\n", fmt, strerrbuf);		\
									\
	if (perfuse_diagflags & PDF_SYSLOG)				\
		syslog(LOG_WARNING, fmterr, ## __VA_ARGS__);		\
									\
	warn(fmterr, ## __VA_ARGS__);					\
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

struct perfuse_callbacks {
	perfuse_new_msg_fn pc_new_msg;
	perfuse_xchg_msg_fn pc_xchg_msg;
	perfuse_destroy_msg_fn pc_destroy_msg;
	perfuse_get_inhdr_fn pc_get_inhdr;
	perfuse_get_inpayload_fn pc_get_inpayload;
	perfuse_get_outhdr_fn pc_get_outhdr;
	perfuse_get_outpayload_fn pc_get_outpayload;
};

/* 
 * mount request
 */
struct perfuse_mount_out {
        uint32_t pmo_len;
        int32_t pmo_error;  
        uint64_t pmo_unique;
	char pmo_magic[sizeof(PERFUSE_MOUNT_MAGIC)];
	size_t pmo_source_len;
	size_t pmo_target_len;
	size_t pmo_filesystemtype_len;
	int pmo_mountflags;
	size_t pmo_data_len;
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
 * Duplicated fro fuse.h to avoid making it public
 */
#define FUSE_MIN_BUFSIZE 0x21000
#define FUSE_PREF_BUFSIZE (PAGE_SIZE + 0x1000)
#define FUSE_BUFSIZE MAX(FUSE_PREF_BUFSIZE, FUSE_MIN_BUFSIZE)

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
uint64_t perfuse_get_ino(struct puffs_usermount *, puffs_cookie_t);
int perfuse_inloop(struct puffs_usermount *);
const char *perfuse_opname(int);
void perfuse_fs_init(struct puffs_usermount *);
int perfuse_mainloop(struct puffs_usermount *);

#endif /* _REFUSE_PERFUSE_H */
