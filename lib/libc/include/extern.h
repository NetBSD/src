/*	$NetBSD: extern.h,v 1.10 2006/01/24 19:33:10 christos Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

__BEGIN_DECLS
extern char *__minbrk;
int __getcwd __P((char *, size_t));
int __getlogin __P((char *, size_t));
int __setlogin __P((const char *));
void _resumecontext __P((void));
const char *__strerror __P((int , char *, size_t));
const char *__strsignal __P((int , char *, size_t));
char *__dtoa __P((double, int, int, int *, int *, char **));
int __sysctl __P((int *, unsigned int, void *, size_t *, void *, size_t));

struct sigaction;
int __sigaction_sigtramp __P((int, const struct sigaction *,
    struct sigaction *, const void *, int));

struct _dirdesc;
void _seekdir_unlocked(struct _dirdesc *, long);
long _telldir_unlocked(const struct _dirdesc *);
#ifndef __LIBC12_SOURCE__
struct dirent;
struct dirent *_readdir_unlocked(struct _dirdesc *) __RENAME(___readdir_unlocked30);
#endif
__END_DECLS
