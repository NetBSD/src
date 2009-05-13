/*	$NetBSD: unixdev.h,v 1.1.6.2 2009/05/13 17:18:55 jym Exp $	*/
/*	$OpenBSD: unixdev.h,v 1.1 2005/05/24 20:38:20 uwe Exp $	*/

/*
 * Copyright (c) 1996 Michael Shalayeff
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef	_STAND_UNIXDEV_H_
#define	_STAND_UNIXDEV_H_

struct linux_timeval;
struct linux_stat;

/* unixcons.c */
#define	CONSDEV_GLASS	0
#define	CONSDEV_COM0	1

void consinit(int, int);
int awaitkey(int, int);

/* unixdev.c */
int unixopen(struct open_file *, ...);
int unixclose(struct open_file *);
int unixioctl(struct open_file *, u_long, void *);
int unixstrategy(void *, int, daddr_t, size_t, void *, size_t *);
int sleep(int seconds);

/* unixsys.S */
extern int errno;
int uopen(const char *, int, ...);
int uread(int, void *, size_t);
int uwrite(int, void *, size_t);
int uioctl(int, u_long, void *);
int uclose(int);
off_t ulseek(int, off_t, int);
void uexit(int) __attribute__((noreturn));
int uselect(int, fd_set *, fd_set *, fd_set *, struct linux_timeval *);
int ustat(const char *, struct linux_stat *);
int syscall(int, ...);
int __syscall(quad_t, ...);

#endif	/* _STAND_UNIXDEV_H_ */
