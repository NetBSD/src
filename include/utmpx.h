/*	$NetBSD: utmpx.h,v 1.3 2002/02/25 13:57:24 christos Exp $	 */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#ifndef	_UTMPX_H_
#define	_UTMPX_H_

#include <sys/socket.h>

#define	_PATH_UTMPX		"/var/run/utmpx"
#define	_PATH_WTMPX		"/var/log/wtmpx"
#define	_PATH_LASTLOGX		"/var/log/lastlogx"
#define	_PATH_UTMP_UPDATE	"/usr/libexec/utmp_update"

#define _UTX_USERSIZE	32
#define _UTX_LINESIZE	32
#define	_UTX_IDSIZE	4
#define _UTX_HOSTSIZE	256

#ifndef _XOPEN_SOURCE
#define UTX_USERSIZE	_UTX_USERSIZE
#define UTX_LINESIZE	_UTX_LINESIZE
#define	UTX_IDSIZE	_UTX_IDSIZE
#define UTX_HOSTSIZE	_UTX_HOSTSIZE
#endif

#define EMPTY		0
#define RUN_LVL		1
#define BOOT_TIME	2
#define OLD_TIME	3
#define NEW_TIME	4
#define INIT_PROCESS	5
#define LOGIN_PROCESS	6
#define USER_PROCESS	7
#define DEAD_PROCESS	8

#ifndef _XOPEN_SOURCE
#define ACCOUNTING	9
#define SIGNATURE	10
#endif

/*
 * The following structure describes the fields of the utmpx entries
 * stored in _PATH_UTMPX or _PATH_WTMPX. This is not the format the
 * entries are stored in the files, and application should only access
 * entries using routines described in getutxent(3).
 */
struct utmpx {
	char ut_user[_UTX_USERSIZE];	/* login name */
	char ut_id[_UTX_IDSIZE];	/* inittab id */
	char ut_line[_UTX_LINESIZE];	/* tty name */
	char ut_host[_UTX_HOSTSIZE];	/* host name */
	uint16_t ut_session;		/* session id used for windowing */
	uint16_t ut_type;		/* type of this entry */
	pid_t ut_pid;			/* process id creating the entry */
	struct {
		uint16_t e_termination;	/* process termination signal */
		uint16_t e_exit;	/* process exit status */
	} ut_exit;
	struct sockaddr_storage ut_ss;	/* address where entry was made from */
	struct timeval ut_tv;		/* time entry was created */
	uint32_t ut_pad[10];		/* reserved for future use */
};

#ifndef _XOPEN_SOURCE
struct lastlogx {
	struct timespec ll_time;	/* time entry was created */
	char ll_line[_UTX_LINESIZE];	/* tty name */
	char ll_host[_UTX_HOSTSIZE];	/* host name */
	struct sockaddr_storage ll_ss;	/* address where entry was made from */
};
#endif	/* !_XOPEN_SOURCE */

__BEGIN_DECLS

void setutxent(void);
void endutxent(void);
struct utmpx *getutxent(void);
struct utmpx *getutxid(const struct utmpx *);
struct utmpx *getutxline(const struct utmpx *);
struct utmpx *pututxline(const struct utmpx *);

#ifndef _XOPEN_SOURCE

int lastlogxname(const char *);
struct lastlogx *getlastlogxuid(uid_t);
void updlastlogx(const char *, struct lastlogx *);

int utmpxname(const char *);

#endif /* !_XOPEN_SOURCE */

__END_DECLS

#endif /* !_UTMPX_H_ */
