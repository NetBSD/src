/*	$NetBSD: ssl.h,v 1.4.2.1 2021/06/14 11:57:39 martin Exp $	*/

/*-
 * Copyright (c) 2012-2021 The NetBSD Foundation, Inc.
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

#define FETCH struct fetch_connect
struct fetch_connect;

int fetch_printf(struct fetch_connect *, const char *fmt, ...)
    __printflike(2, 3);
int fetch_fileno(struct fetch_connect *);
int fetch_error(struct fetch_connect *);
int fetch_flush(struct fetch_connect *);
struct fetch_connect *fetch_open(const char *, const char *);
struct fetch_connect *fetch_fdopen(int, const char *);
int fetch_close(struct fetch_connect *);
size_t fetch_read(void *, size_t, size_t, struct fetch_connect *);
char *fetch_getln(char *, int, struct fetch_connect *);
int fetch_getline(struct fetch_connect *, char *, size_t, const char **);
void fetch_set_ssl(struct fetch_connect *, void *);
void *fetch_start_ssl(int, const char *);
