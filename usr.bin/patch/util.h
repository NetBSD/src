/*	$NetBSD: util.h,v 1.10 2003/07/12 13:47:44 itojun Exp $	*/

/*
 * Copyright (c) 1988, Larry Wall
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition
 * is met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this condition and the following disclaimer.
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

int move_file(char *, char *);
void copy_file(char *, char *);
void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *);
void say(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void fatal(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void pfatal(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void ask(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
void set_signals(int);
void ignore_signals(void);
void makedirs(char *, bool);
char *fetchname(char *, int, int);
