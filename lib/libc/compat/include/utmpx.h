/*	$NetBSD: utmpx.h,v 1.2.8.2 2008/11/08 21:45:38 christos Exp $	 */

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
#ifndef	_COMPAT_UTMPX_H_
#define	_COMPAT_UTMPX_H_

struct utmpx50 {
	char ut_name[_UTX_USERSIZE];	/* login name */
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
	struct timeval50 ut_tv;		/* time entry was created */
	uint32_t ut_pad[10];		/* reserved for future use */
};

struct lastlogx50 {
	struct timeval50 ll_tv;		/* time entry was created */
	char ll_line[_UTX_LINESIZE];	/* tty name */
	char ll_host[_UTX_HOSTSIZE];	/* host name */
	struct sockaddr_storage ll_ss;	/* address where entry was made from */
};

__BEGIN_DECLS

struct utmpx50 *getutxent(void);
struct utmpx *__getutxent50(void);
struct utmpx50 *getutxid(const struct utmpx50 *);
struct utmpx *__getutxid50(const struct utmpx *);
struct utmpx50 *getutxline(const struct utmpx50 *);
struct utmpx *__getutxline50(const struct utmpx *);
struct utmpx50 *pututxline(const struct utmpx50 *);
struct utmpx *__pututxline50(const struct utmpx *);
int updwtmpx(const char *, const struct utmpx50 *);
int __updwtmpx50(const char *, const struct utmpx *);
int updlastlogx(const char *, uid_t, struct lastlogx50 *);
int __updlastlogx50(const char *, uid_t, struct lastlogx *);
struct utmp;
void getutmp(const struct utmpx50 *, struct utmp *);
void __getutmp50(const struct utmpx *, struct utmp *);
void getutmpx(const struct utmp *, struct utmpx50 *);
void __getutmpx50(const struct utmp *, struct utmpx *);

int lastlogxname(const char *);
struct lastlogx50 *getlastlogx(uid_t, struct lastlogx50 *);
struct lastlogx50 *__getlastlogx13(const char *, uid_t, struct lastlogx50 *);
struct lastlogx *__getlastlogx50(const char *, uid_t, struct lastlogx *);

__END_DECLS

#endif /* !_COMPAT_UTMPX_H_ */
