/*	$NetBSD: util.h,v 1.31 2003/08/07 09:44:11 agc Exp $	*/

/*-
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _UTIL_H_
#define	_UTIL_H_

#include <sys/cdefs.h>
#include <sys/ttycom.h>
#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <termios.h>
#include <utmp.h>
#include <utmpx.h>

#define	PIDLOCK_NONBLOCK	1
#define	PIDLOCK_USEHOSTNAME	2

#define	HN_DECIMAL		0x01
#define	HN_NOSPACE		0x02
#define	HN_B			0x04
#define	HN_DIVISOR_1000		0x08

#define	HN_GETSCALE		0x10
#define	HN_AUTOSCALE		0x20

__BEGIN_DECLS
struct disklabel;
struct iovec;
struct passwd;
struct termios;
struct utmp;
struct winsize;

pid_t		forkpty(int *, char *, struct termios *, struct winsize *);
const char     *getbootfile(void);
off_t		getlabeloffset(void);
int		getlabelsector(void);
int		getmaxpartitions(void);
int		getrawpartition(void);
int		humanize_number(char *, size_t, int64_t, const char *, int,
		    int);
void		login(const struct utmp *);
void		loginx(const struct utmpx *);
int		login_tty(int);
int		logout(const char *);
int		logoutx(const char *, int, int);
void		logwtmp(const char *, const char *, const char *);
void		logwtmpx(const char *, const char *, const char *, int, int);
int		opendisk(const char *, int, char *, size_t, int);
int		openpty(int *, int *, char *, struct termios *,
		    struct winsize *);
int		pidfile(const char *);
int		pidlock(const char *, int, pid_t *, const char *);
int		pw_abort(void);
void		pw_copy(int, int, struct passwd *, struct passwd *);
void		pw_edit(int, const char *);
void		pw_error(const char *, int, int);
void		pw_getconf(char *, size_t, const char *, const char *);
const char     *pw_getprefix(void);
void		pw_init(void);
int		pw_lock(int);
int		pw_mkdb(const char *, int);
void		pw_prompt(void);
int		pw_setprefix(const char *);
int		secure_path(const char *);
int		snprintb(char *, size_t, const char *, uint64_t);
int		ttyaction(const char *, const char *, const char *);
int		ttylock(const char *, int, pid_t *);
char	       *ttymsg(struct iovec *, int, const char *, int);
int		ttyunlock(const char *);

u_short		disklabel_dkcksum(struct disklabel *);
int		disklabel_scan(struct disklabel *, char *, size_t);
__END_DECLS

#endif /* !_UTIL_H_ */
