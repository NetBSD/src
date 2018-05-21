/* $NetBSD: thunk.h,v 1.62.16.1 2018/05/21 04:36:02 pgoyette Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARCH_USERMODE_INCLUDE_THUNK_H
#define _ARCH_USERMODE_INCLUDE_THUNK_H

#include "types.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ucontext.h>
#include <sys/signal.h>

struct thunk_timeval {
	int64_t tv_sec;
	int32_t tv_usec;
};

struct thunk_itimerval {
	struct thunk_timeval it_interval;
	struct thunk_timeval it_value;
};

struct thunk_termios {
	uint32_t c_iflag;
	uint32_t c_oflag;
	uint32_t c_cflag;
	uint32_t c_lflag;
	uint8_t c_cc[20];
	int32_t c_ispeed;
	int32_t c_ospeed;
};

#define THUNK_MAP_ANON		0x0001
#define THUNK_MAP_FIXED		0x0002
#define THUNK_MAP_FILE		0x0004
#define THUNK_MAP_SHARED	0x0010
#define THUNK_MAP_PRIVATE	0x0020

#define THUNK_PROT_NONE		0x00
#define THUNK_PROT_READ		0x01
#define THUNK_PROT_WRITE	0x02
#define THUNK_PROT_EXEC		0x04

#define THUNK_MADV_NORMAL	0x01
#define THUNK_MADV_RANDOM	0x02
#define THUNK_MADV_SEQUENTIAL	0x04
#define THUNK_MADV_WILLNEED	0x08
#define THUNK_MADV_DONTNEED	0x10
#define THUNK_MADV_FREE		0x20


struct aiocb;

void	thunk_printf_debug(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));
void	thunk_printf(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));

int	thunk_syscallemu_init(void *, void *);

int	thunk_setitimer(int, const struct thunk_itimerval *, struct thunk_itimerval *);
int	thunk_gettimeofday(struct thunk_timeval *, void *);
unsigned int thunk_getcounter(void);
long	thunk_clock_getres_monotonic(void);
int	thunk_usleep(useconds_t);

timer_t	thunk_timer_attach(void);
int	thunk_timer_start(timer_t, int);
int	thunk_timer_getoverrun(timer_t);

void	thunk_exit(int);
void	thunk_abort(void);

int	thunk_geterrno(void);
void	thunk_seterrno(int err);

int	thunk_getcontext(ucontext_t *);
int	thunk_setcontext(const ucontext_t *);
void	thunk_makecontext(ucontext_t *ucp, void (*func)(void), 
		int nargs, void *arg1, void *arg2, void *arg3, void *arg4);
int	thunk_swapcontext(ucontext_t *, ucontext_t *);

int	thunk_tcgetattr(int, struct thunk_termios *);
int	thunk_tcsetattr(int, int, const struct thunk_termios *);

int	thunk_set_stdin_sigio(int);
int	thunk_pollchar(void);
int	thunk_getchar(void);
void	thunk_putchar(int);

int	thunk_execv(const char *, char * const []);

int	thunk_open(const char *, int, mode_t);
int	thunk_close(int);
int	thunk_fstat_getsize(int, off_t *, ssize_t *);
ssize_t	thunk_pread(int, void *, size_t, off_t);
ssize_t	thunk_pwrite(int, const void *, size_t, off_t);
ssize_t	thunk_read(int, void *, size_t);
ssize_t	thunk_write(int, const void *, size_t);
int	thunk_fsync(int);
int	thunk_mkstemp(char *);
int	thunk_unlink(const char *);
pid_t	thunk_getpid(void);

int	thunk_sigaction(int, const struct sigaction *, struct sigaction *);
int	thunk_sigaltstack(const stack_t *, stack_t *);
void	thunk_signal(int, void (*)(int));
int	thunk_sigblock(int);
int	thunk_sigunblock(int);
int	thunk_sigemptyset(sigset_t *sa_mask);
int	thunk_sigfillset(sigset_t *sa_mask);
void	thunk_sigaddset(sigset_t *sa_mask, int sig);
int	thunk_sigprocmask(int how, const sigset_t * set, sigset_t *oset);
int	thunk_atexit(void (*function)(void));

int	thunk_aio_read(struct aiocb *);
int	thunk_aio_write(struct aiocb *);
int	thunk_aio_error(const struct aiocb *);
int	thunk_aio_return(struct aiocb *);

void *	thunk_malloc(size_t len);
void 	thunk_free(void *addr);
void *	thunk_sbrk(intptr_t len);
void *	thunk_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int	thunk_munmap(void *addr, size_t len);
int	thunk_mprotect(void *addr, size_t len, int prot);
int	thunk_madvise(void *addr, size_t len, int behav);
int	thunk_posix_memalign(void **, size_t, size_t);

int	thunk_idle(void);

char *	thunk_getenv(const char *);
vaddr_t	thunk_get_vm_min_address(void);

int	thunk_getcpuinfo(char *, size_t *);

int	thunk_getmachine(char *, size_t, char *, size_t);

int	thunk_setown(int);

int	thunk_open_tap(const char *);
int	thunk_pollin_tap(int, int);
int	thunk_pollout_tap(int, int);

typedef struct {
	unsigned int		sample_rate;
	unsigned int		precision;
	unsigned int		validbits;
	unsigned int		channels;
} thunk_audio_config_t;

int	thunk_audio_open(const char *);
int	thunk_audio_close(int);
int	thunk_audio_drain(int);
int	thunk_audio_config(int, const thunk_audio_config_t *,
			   const thunk_audio_config_t *);
int	thunk_audio_pollout(int);
int	thunk_audio_pollin(int);
ssize_t	thunk_audio_write(int, const void *, size_t);
ssize_t	thunk_audio_read(int, void *, size_t);

typedef enum {
	/* client -> server */
	THUNK_RFB_SET_PIXEL_FORMAT = 0,
	THUNK_RFB_SET_ENCODINGS = 2,
	THUNK_RFB_FRAMEBUFFER_UPDATE_REQUEST = 3,
	THUNK_RFB_KEY_EVENT = 4,
	THUNK_RFB_POINTER_EVENT = 5,
	THUNK_RFB_CLIENT_CUT_TEXT = 6,
} thunk_rfb_message_t;

typedef struct {
	thunk_rfb_message_t	message_type;
	union {
		struct {
			uint8_t		down_flag;
			uint32_t	keysym;
		} key_event;
		struct {
			uint8_t		button_mask;
			uint16_t	absx;
			uint16_t	absy;
		} pointer_event;
	} data;
} thunk_rfb_event_t;


typedef struct {
	uint8_t			enc;
	uint16_t		x, y, w, h;
	uint16_t		srcx, srcy;
	uint8_t			pixel[4];
} thunk_rfb_update_t;
#define THUNK_RFB_TYPE_RAW	0
#define THUNK_RFB_TYPE_COPYRECT	1
#define THUNK_RFB_TYPE_RRE	2		/* rectangle fill */

#define THUNK_RFB_QUEUELEN	128

typedef struct {
	int			sockfd;
	int			clientfd;
	thunk_rfb_event_t	event;

	bool			connected;

	uint16_t		width;
	uint16_t		height;
	uint8_t			depth;
	char			name[64];
	uint8_t			*framebuf;

	bool			schedule_bell;
	unsigned int		nupdates;
	unsigned int		first_mergable;
	thunk_rfb_update_t	update[THUNK_RFB_QUEUELEN];
} thunk_rfb_t;

int	thunk_rfb_open(thunk_rfb_t *, uint16_t);
int	thunk_rfb_poll(thunk_rfb_t *, thunk_rfb_event_t *);
void	thunk_rfb_bell(thunk_rfb_t *);
void	thunk_rfb_update(thunk_rfb_t *, int, int, int, int);
void	thunk_rfb_copyrect(thunk_rfb_t *, int, int, int, int, int, int);
void	thunk_rfb_fillrect(thunk_rfb_t *, int, int, int, int, uint8_t *);

#endif /* !_ARCH_USERMODE_INCLUDE_THUNK_H */
