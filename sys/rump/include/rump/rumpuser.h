/*	$NetBSD: rumpuser.h,v 1.25 2009/09/02 19:02:51 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RUMP_RUMPUSER_H_
#define _RUMP_RUMPUSER_H_

#include <sys/stdint.h>

struct msghdr;
struct pollfd;
struct sockaddr;

typedef void (*kernel_lockfn)(int);
typedef void (*kernel_unlockfn)(int, int *);

int rumpuser_getfileinfo(const char *, uint64_t *, int *, int *);
#define RUMPUSER_FT_OTHER 0
#define RUMPUSER_FT_DIR 1
#define RUMPUSER_FT_REG 2
#define RUMPUSER_FT_BLK 3
#define RUMPUSER_FT_CHR 4
int rumpuser_nanosleep(uint64_t *, uint64_t *, int *);

#define rumpuser_malloc(a,b) rumpuser__malloc(a,b,__func__,__LINE__);
#define rumpuser_realloc(a,b,c) rumpuser__realloc(a,b,c,__func__,__LINE__);

void *rumpuser__malloc(size_t, int, const char *, int);
void *rumpuser__realloc(void *, size_t, int, const char *, int);
void rumpuser_free(void *);

void *rumpuser_anonmmap(size_t, int, int, int *);
#define RUMPUSER_FILEMMAP_READ		0x01
#define RUMPUSER_FILEMMAP_WRITE		0x02
#define RUMPUSER_FILEMMAP_TRUNCATE	0x04
#define RUMPUSER_FILEMMAP_SHARED	0x08
void *rumpuser_filemmap(int fd, off_t, size_t, int, int *);
void  rumpuser_unmap(void *, size_t);
int   rumpuser_memsync(void *, size_t, int *);

int rumpuser_open(const char *, int, int *);
int rumpuser_ioctl(int, u_long, void *, int *);
int rumpuser_close(int, int *);
int rumpuser_fsync(int, int *);

typedef void (*rump_biodone_fn)(void *, size_t, int);

ssize_t rumpuser_read(int, void *, size_t, int *);
ssize_t rumpuser_pread(int, void *, size_t, off_t, int *);
ssize_t rumpuser_write(int, const void *, size_t, int *);
ssize_t rumpuser_pwrite(int, const void *, size_t, off_t, int *);
void rumpuser_read_bio(int, void *, size_t, off_t, rump_biodone_fn, void *);
void rumpuser_write_bio(int, const void *, size_t, off_t,rump_biodone_fn,void*);

struct rumpuser_iovec {
	void *iov_base;
	uint64_t iov_len;
};
ssize_t rumpuser_readv(int, const struct rumpuser_iovec *, int, int *);
ssize_t rumpuser_writev(int, const struct rumpuser_iovec *, int, int *);

int rumpuser_gettime(uint64_t *, uint64_t *, int *);
int rumpuser_getenv(const char *, char *, size_t, int *);

int rumpuser_gethostname(char *, size_t, int *);

char *rumpuser_realpath(const char *, char *, int *);

int rumpuser_poll(struct pollfd *, int, int, int *);

int rumpuser_putchar(int, int *);

void rumpuser_panic(void);

void rumpuser_seterrno(int);

int rumpuser_writewatchfile_setup(int, int, intptr_t, int *);
int rumpuser_writewatchfile_wait(int, intptr_t *, int *);

int rumpuser_dprintf(const char *, ...);

/* rumpuser_pth */
void rumpuser_thrinit(kernel_lockfn, kernel_unlockfn, int);
int  rumpuser_bioinit(rump_biodone_fn);

int  rumpuser_thread_create(void *(*f)(void *), void *, const char *);
void rumpuser_thread_exit(void);

struct rumpuser_mtx;

void rumpuser_mutex_init(struct rumpuser_mtx **);
void rumpuser_mutex_recursive_init(struct rumpuser_mtx **);
void rumpuser_mutex_enter(struct rumpuser_mtx *);
int  rumpuser_mutex_tryenter(struct rumpuser_mtx *);
void rumpuser_mutex_exit(struct rumpuser_mtx *);
void rumpuser_mutex_destroy(struct rumpuser_mtx *);
int  rumpuser_mutex_held(struct rumpuser_mtx *);

struct rumpuser_rw;

void rumpuser_rw_init(struct rumpuser_rw **);
void rumpuser_rw_enter(struct rumpuser_rw *, int);
int  rumpuser_rw_tryenter(struct rumpuser_rw *, int);
void rumpuser_rw_exit(struct rumpuser_rw *);
void rumpuser_rw_destroy(struct rumpuser_rw *);
int  rumpuser_rw_held(struct rumpuser_rw *);
int  rumpuser_rw_rdheld(struct rumpuser_rw *);
int  rumpuser_rw_wrheld(struct rumpuser_rw *);

struct rumpuser_cv;

void rumpuser_cv_init(struct rumpuser_cv **);
void rumpuser_cv_destroy(struct rumpuser_cv *);
void rumpuser_cv_wait(struct rumpuser_cv *, struct rumpuser_mtx *);
int  rumpuser_cv_timedwait(struct rumpuser_cv *, struct rumpuser_mtx *,
			   struct timespec *);
void rumpuser_cv_signal(struct rumpuser_cv *);
void rumpuser_cv_broadcast(struct rumpuser_cv *);
int  rumpuser_cv_has_waiters(struct rumpuser_cv *);

struct lwp;

void rumpuser_set_curlwp(struct lwp *);
struct lwp *rumpuser_get_curlwp(void);

/* "aio" stuff for being able to fire of a B_ASYNC I/O and continue */
struct rumpuser_aio {
	int	rua_fd;
	uint8_t	*rua_data;
	size_t	rua_dlen;
	off_t	rua_off;
	void	*rua_bp;
	int	rua_op;
};
#define RUA_OP_READ	0x01
#define RUA_OP_WRITE	0x02
#define RUA_OP_SYNC	0x04

#define N_AIOS 1024
extern struct rumpuser_mtx rumpuser_aio_mtx;
extern struct rumpuser_cv rumpuser_aio_cv;
extern struct rumpuser_aio rumpuser_aios[N_AIOS];
extern int rumpuser_aio_head, rumpuser_aio_tail;

/* rumpuser_net */

int  rumpuser_net_socket(int, int, int, int *);
int  rumpuser_net_sendmsg(int, const struct msghdr *, int, int *);
int  rumpuser_net_recvmsg(int, struct msghdr *, int, int *);
int  rumpuser_net_connect(int, const struct sockaddr *, int, int *);
int  rumpuser_net_bind(int, const struct sockaddr *, int, int *);
int  rumpuser_net_accept(int, struct sockaddr *, int *, int *);
int  rumpuser_net_listen(int, int, int *);
enum rumpuser_getnametype { RUMPUSER_SOCKNAME, RUMPUSER_PEERNAME };
int  rumpuser_net_getname(int, struct sockaddr *, int *,
			      enum rumpuser_getnametype, int *);
int  rumpuser_net_setsockopt(int, int, int, const void *, int, int *);

#endif /* _RUMP_RUMPUSER_H_ */
