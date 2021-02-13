/*	$NetBSD: slave.h,v 1.7 2021/02/13 08:17:15 rillig Exp $	*/

/*-
 * Copyright 2009 Brett Lymn <blymn@NetBSD.org>
 * Copyright 2021 Roland Illig <rillig@NetBSD.org>
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CTF_SLAVE_H
#define CTF_SLAVE_H

#include <curses.h>

extern int from_director;
extern int to_director;

void command_execute(char *, int, char **);
void report_count(int);
void report_error(const char *);
void report_int(int);
void report_byte(chtype);
void report_return(int);
void report_nstr(chtype *);
void report_cchar(cchar_t);
void report_wchar(wchar_t);
void report_wstr(wchar_t *);
void report_status(const char *);
void report_ptr(void *);
int  check_arg_count(int, int);

#endif
