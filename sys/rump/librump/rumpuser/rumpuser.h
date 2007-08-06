/*	$NetBSD: rumpuser.h,v 1.2 2007/08/06 22:20:58 pooka Exp $	*/

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

#ifndef _SYS_RUMPUSER_H_
#define _SYS_RUMPUSER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vnode.h>

int rumpuser_stat(const char *, struct stat *, int *);
int rumpuser_lstat(const char *, struct stat *, int *);

#define rumpuser_malloc(a,b) _rumpuser_malloc(a,b,__func__,__LINE__);
#define rumpuser_realloc(a,b,c) _rumpuser_realloc(a,b,c,__func__,__LINE__);

void *_rumpuser_malloc(size_t, int, const char *, int);
void *_rumpuser_realloc(void *, size_t, int, const char *, int);
void rumpuser_free(void *);

int rumpuser_open(const char *, int, int *);
int rumpuser_ioctl(int, u_long, void *, int *);
void rumpuser_close(int);

ssize_t rumpuser_pread(int, void *, size_t, off_t, int *);
ssize_t rumpuser_pwrite(int, const void *, size_t, off_t, int *);

int rumpuser_gettimeofday(struct timeval *);

uint16_t rumpuser_bswap16(uint16_t);
uint32_t rumpuser_bswap32(uint32_t);
uint64_t rumpuser_bswap64(uint64_t);

int rumpuser_gethostname(char *, size_t, int *);

char *rumpuser_realpath(const char *, char *);

#endif /* _SYS_RUMPUSER_H_ */
