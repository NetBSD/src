/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef COMMON_H
#define COMMON_H

/* string.h pulls in features.h so the below define checks work */
#include <sys/types.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if __GNUC__ > 2 || defined(__INTEL_COMPILER)
# define _unused __attribute__((__unused__))
#else
# define _unused
#endif

#ifndef HAVE_ARC4RANDOM
# ifdef __GLIBC__
uint32_t arc4random(void);
#else
# define HAVE_ARC4RANDOM
# endif
#endif

#ifndef HAVE_STRLCPY
#  define HAVE_STRLCPY 1
#endif
/* Only GLIBC doesn't support strlcpy */
#ifdef __GLIBC__
#  if !defined(__UCLIBC__) && !defined (__dietlibc__)
#    undef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#  endif
#endif

#ifndef HAVE_CLOSEFROM
#define HAVE_CLOSEFROM 1
#endif
#if defined(__linux__) || defined(__FreeBSD__)
#  undef HAVE_CLOSEFROM
int closefrom(int);
#endif

int close_fds(void);
int close_on_exec(int);
ssize_t get_line(char **, size_t *, FILE *);
int get_time(struct timeval *);
time_t uptime(void);
int writepid(int, pid_t);
void *xrealloc(void *, size_t);
void *xmalloc(size_t);
void *xzalloc(size_t);
char *xstrdup(const char *);

#endif
