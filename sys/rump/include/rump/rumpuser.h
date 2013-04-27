/*	$NetBSD: rumpuser.h,v 1.80 2013/04/27 15:32:36 pooka Exp $	*/

/*
 * Copyright (c) 2007-2011 Antti Kantee.  All Rights Reserved.
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

/*
 * Do not include any headers here!  Implementation must take care of
 * having stdint or equivalent included before including this header.
 */

#if !defined(_KERNEL) && !defined(LIBRUMPUSER)
#error The rump/rumpuser.h interface is not for non-kernel consumers
#endif

#define RUMPUSER_VERSION 16

int rumpuser_daemonize_begin(void);
int rumpuser_daemonize_done(int);

typedef void (*rump_reschedulefn)(int, void *);
typedef void (*rump_unschedulefn)(int, int *, void *);
int rumpuser_init(int, rump_reschedulefn, rump_unschedulefn);

int rumpuser_getfileinfo(const char *, uint64_t *, int *, int *);
#define RUMPUSER_FT_OTHER 0
#define RUMPUSER_FT_DIR 1
#define RUMPUSER_FT_REG 2
#define RUMPUSER_FT_BLK 3
#define RUMPUSER_FT_CHR 4
int rumpuser_nanosleep(uint64_t *, uint64_t *, int *);

void *rumpuser_malloc(size_t, int);
void *rumpuser_realloc(void *, size_t);
void rumpuser_free(void *);

void *rumpuser_anonmmap(void *, size_t, int, int, int *);
#define RUMPUSER_FILEMMAP_READ		0x01
#define RUMPUSER_FILEMMAP_WRITE		0x02
#define RUMPUSER_FILEMMAP_TRUNCATE	0x04
#define RUMPUSER_FILEMMAP_SHARED	0x08
void *rumpuser_filemmap(int fd, off_t, size_t, int, int *);
void  rumpuser_unmap(void *, size_t);
int   rumpuser_memsync(void *, size_t, int *);

/*
 * Open modes.  these "accidentally" happen to match the respective
 * NetBSD bit representations to preserve NetBSD binary compat for
 * current users.  If you need to add new ones, just pick any numbers.
 */
#define RUMPUSER_OPEN_RDONLY	0x000000
#define RUMPUSER_OPEN_WRONLY	0x000001
#define RUMPUSER_OPEN_RDWR	0x000002
#define RUMPUSER_OPEN_ACCMODE	0x000003 /* "yay" */
#define RUMPUSER_OPEN_CREATE	0x000200
#define RUMPUSER_OPEN_EXCL	0x000800
#define RUMPUSER_OPEN_DIRECT	0x080000
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

int rumpuser_putchar(int, int *);

#define RUMPUSER_PID_SELF ((int64_t)-1)
int rumpuser_kill(int64_t, int, int *);

#define RUMPUSER_PANIC (-1)
void rumpuser_exit(int) __dead;
void rumpuser_seterrno(int);

int rumpuser_writewatchfile_setup(int, int, intptr_t, int *);
int rumpuser_writewatchfile_wait(int, intptr_t *, int *);

int rumpuser_dprintf(const char *, ...) __printflike(1, 2);

int rumpuser_getnhostcpu(void);

uint32_t rumpuser_arc4random(void);

__dead void rumpuser_biothread(void *);

int  rumpuser_thread_create(void *(*f)(void *), void *, const char *, int,
			    void **);
void rumpuser_thread_exit(void) __dead;
int  rumpuser_thread_join(void *);

struct rumpuser_mtx;

void rumpuser_mutex_init(struct rumpuser_mtx **);
void rumpuser_mutex_init_kmutex(struct rumpuser_mtx **, int);
void rumpuser_mutex_enter(struct rumpuser_mtx *);
void rumpuser_mutex_enter_nowrap(struct rumpuser_mtx *);
int  rumpuser_mutex_tryenter(struct rumpuser_mtx *);
void rumpuser_mutex_exit(struct rumpuser_mtx *);
void rumpuser_mutex_destroy(struct rumpuser_mtx *);
struct lwp *rumpuser_mutex_owner(struct rumpuser_mtx *);

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
void rumpuser_cv_wait_nowrap(struct rumpuser_cv *, struct rumpuser_mtx *);
int  rumpuser_cv_timedwait(struct rumpuser_cv *, struct rumpuser_mtx *,
			   int64_t, int64_t);
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

/* rumpuser dynloader */

struct modinfo;
struct rump_component;
typedef void (*rump_modinit_fn)(const struct modinfo *const *, size_t);
typedef int (*rump_symload_fn)(void *, uint64_t, char *, uint64_t);
typedef void (*rump_compload_fn)(const struct rump_component *);
void rumpuser_dl_bootstrap(rump_modinit_fn, rump_symload_fn, rump_compload_fn);
void *rumpuser_dl_globalsym(const char *);

/* syscall proxy routines */

struct rumpuser_sp_ops {
	void (*spop_schedule)(void);
	void (*spop_unschedule)(void);

	void (*spop_lwproc_switch)(struct lwp *);
	void (*spop_lwproc_release)(void);
	int (*spop_lwproc_rfork)(void *, int, const char *);
	int (*spop_lwproc_newlwp)(pid_t);
	struct lwp * (*spop_lwproc_curlwp)(void);
	int (*spop_syscall)(int, void *, register_t *);
	void (*spop_lwpexit)(void);
	void (*spop_execnotify)(const char *);
	pid_t (*spop_getpid)(void);
};

int	rumpuser_sp_init(const char *, const struct rumpuser_sp_ops *,
			 const char *, const char *, const char *);
int	rumpuser_sp_copyin(void *, const void *, void *, size_t);
int	rumpuser_sp_copyinstr(void *, const void *, void *, size_t *);
int	rumpuser_sp_copyout(void *, const void *, void *, size_t);
int	rumpuser_sp_copyoutstr(void *, const void *, void *, size_t *);
int	rumpuser_sp_anonmmap(void *, size_t, void **);
int	rumpuser_sp_raise(void *, int);
void	rumpuser_sp_fini(void *);

#endif /* _RUMP_RUMPUSER_H_ */
