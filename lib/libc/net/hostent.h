/*	$NetBSD: hostent.h,v 1.1 2013/08/16 15:27:12 christos Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#include <stdio.h>
#include <netdb.h>
#include <stdarg.h>

/*
 * These are not being advertised because the interfaces are non-standard.
 * There are versions by linux, aix, qnx, sun, etc. Our versions are used
 * internally to provide thread safety; they mostly resemble qnx.
 */
void sethostent_r(FILE **);
struct hostent	*gethostent_r(FILE *, struct hostent *, char *, size_t, int *);
void endhostent_r(FILE **);

struct hostent *gethostbyname_r(const char *, struct hostent *, char *, size_t,
    int *);
struct hostent *gethostbyname2_r(const char *, int, struct hostent *, char *,
    size_t, int *);
struct hostent *gethostbyaddr_r(const void *, socklen_t, int, struct hostent *,
    char *, size_t, int *);

extern FILE *_h_file;

/*
 * The following are internal API's and are used only for testing.
 */
struct getnamaddr {
	struct hostent *hp;
	char *buf;
	size_t buflen;
	int *he;
};

/* /etc/hosts lookup */
void _hf_sethostsfile(const char *);
int _hf_gethtbyaddr(void *, void *, va_list);
int _hf_gethtbyname(void *, void *, va_list);

/* DNS lookup */
int _dns_gethtbyaddr(void *, void *, va_list);
int _dns_gethtbyname(void *, void *, va_list);

#ifdef YP
/* NIS lookup */
int _yp_gethtbyaddr(void *, void *, va_list);
int _yp_gethtbyname(void *, void *, va_list);
#endif
