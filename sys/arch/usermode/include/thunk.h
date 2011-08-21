/* $NetBSD: thunk.h,v 1.8 2011/08/21 17:11:59 reinoud Exp $ */

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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ucontext.h>
#include <sys/signal.h>
#include <sys/aio.h>
#include <sys/mman.h>

int	thunk_setitimer(int, const struct itimerval *, struct itimerval *);
int	thunk_gettimeofday(struct timeval *, void *);
int	thunk_clock_gettime(clockid_t, struct timespec *);
int	thunk_clock_getres(clockid_t, struct timespec *);
int	thunk_usleep(useconds_t);

void	thunk_exit(int);
void	thunk_abort(void);

int	thunk_getcontext(ucontext_t *);
int	thunk_setcontext(const ucontext_t *);
void	thunk_makecontext(ucontext_t *, void (*)(void), int, void (*)(void *), void *); 
int	thunk_swapcontext(ucontext_t *, ucontext_t *);

int	thunk_getchar(void);
void	thunk_putchar(int);

int	thunk_execv(const char *, char * const []);

int	thunk_open(const char *, int, mode_t);
int	thunk_fstat(int, struct stat *);
ssize_t	thunk_pread(int, void *, size_t, off_t);
ssize_t	thunk_pwrite(int, const void *, size_t, off_t);
int	thunk_fsync(int);
int	thunk_mkstemp(char *);

int	thunk_sigaction(int, const struct sigaction *, struct sigaction *);

int	thunk_aio_read(struct aiocb *);
int	thunk_aio_write(struct aiocb *);
int	thunk_aio_error(const struct aiocb *);
int	thunk_aio_return(struct aiocb *);

void *	thunk_sbrk(intptr_t len);
void *	thunk_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int	thunk_mprotect(void *addr, size_t len, int prot);

#endif /* !_ARCH_USERMODE_INCLUDE_THUNK_H */
