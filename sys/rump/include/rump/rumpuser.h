/*	$NetBSD: rumpuser.h,v 1.6 2009/01/05 01:51:31 pooka Exp $	*/

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

struct stat;
struct msghdr;
struct pollfd;
struct iovec;
struct timespec;
struct timeval;
struct sockaddr;

int rumpuser_stat(const char *, struct stat *, int *);
int rumpuser_lstat(const char *, struct stat *, int *);
int rumpuser_nanosleep(const struct timespec *, struct timespec *, int *);

#define rumpuser_malloc(a,b) _rumpuser_malloc(a,b,__func__,__LINE__);
#define rumpuser_realloc(a,b,c) _rumpuser_realloc(a,b,c,__func__,__LINE__);

void *_rumpuser_malloc(size_t, int, const char *, int);
void *_rumpuser_realloc(void *, size_t, int, const char *, int);
void rumpuser_free(void *);

int rumpuser_open(const char *, int, int *);
int rumpuser_ioctl(int, u_long, void *, int *);
int rumpuser_close(int, int *);
int rumpuser_fsync(int, int *);

typedef void (*rump_biodone_fn)(void *, size_t, int);

ssize_t rumpuser_read(int, void *, size_t, int *);
ssize_t rumpuser_pread(int, void *, size_t, off_t, int *);
ssize_t rumpuser_readv(int, const struct iovec *, int, int *);
ssize_t rumpuser_write(int, const void *, size_t, int *);
ssize_t rumpuser_pwrite(int, const void *, size_t, off_t, int *);
ssize_t rumpuser_writev(int, const struct iovec *, int, int *);
void rumpuser_read_bio(int, void *, size_t, off_t, rump_biodone_fn, void *);
void rumpuser_write_bio(int, const void *, size_t, off_t,rump_biodone_fn,void*);

int rumpuser_gettimeofday(struct timeval *, int *);
int rumpuser_getenv(const char *, char *, size_t, int *);

int rumpuser_gethostname(char *, size_t, int *);

char *rumpuser_realpath(const char *, char *, int *);

int rumpuser_poll(struct pollfd *, int, int, int *);

int rumpuser_putchar(int, int *);

void rumpuser_panic(void);

/* rumpuser_pth */

int  rumpuser_thrinit(void);
int  rumpuser_bioinit(rump_biodone_fn);
void rumpuser_thrdestroy(void);

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
int  rumpuser_cv_timedwait(struct rumpuser_cv *, struct rumpuser_mtx *, int);
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

#define N_AIOS 128
extern struct rumpuser_mtx rua_mtx;
extern struct rumpuser_cv rua_cv;
extern struct rumpuser_aio *rua_aios[N_AIOS];
extern int rua_head, rua_tail;

extern struct rumpuser_rw rumpspl;

#define RUMPUSER_IPL_SPLFOO 1
#define RUMPUSER_IPL_INTR (-1)

void rumpuser_set_ipl(int);
int  rumpuser_whatis_ipl(void);
void rumpuser_clear_ipl(int);

/* rumpuser_net */

int  rumpuser_net_socket(int, int, int, int *);
int  rumpuser_net_sendmsg(int, const struct msghdr *, int, int *);
int  rumpuser_net_recvmsg(int, struct msghdr *, int, int *);
int  rumpuser_net_connect(int, const struct sockaddr *, int, int *);
int  rumpuser_net_bind(int, const struct sockaddr *, int, int *);
int  rumpuser_net_accept(int, struct sockaddr *, int *, int *);
int  rumpuser_net_listen(int, int, int *);

#endif /* _RUMP_RUMPUSER_H_ */
