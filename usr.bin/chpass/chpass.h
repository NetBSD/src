/*	$NetBSD: chpass.h,v 1.11 2003/08/07 11:13:18 agc Exp $	*/

/*
 * Copyright (c) 1988, 1993, 1994
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
 *
 *	@(#)chpass.h	8.4 (Berkeley) 4/2/94
 */

struct passwd;

typedef struct _entry {
	const char *prompt;
	int (*func) __P((const char *, struct passwd *, struct _entry *)), restricted, len;
	const char *except, *save;
} ENTRY;

extern	int use_yp;

/* Field numbers. */
#define	E_BPHONE	8
#define	E_HPHONE	9
#define	E_LOCATE	10
#define	E_NAME		7
#define	E_SHELL		12

extern ENTRY list[];
extern uid_t uid;

int	 atot __P((const char *, time_t *));
void	 display __P((char *, int, struct passwd *));
void	 edit __P((char *, struct passwd *));
const char *
	 ok_shell __P((const char *));
int	 p_change __P((const char *, struct passwd *, ENTRY *));
int	 p_class __P((const char *, struct passwd *, ENTRY *));
int	 p_expire __P((const char *, struct passwd *, ENTRY *));
int	 p_gecos __P((const char *, struct passwd *, ENTRY *));
int	 p_gid __P((const char *, struct passwd *, ENTRY *));
int	 p_hdir __P((const char *, struct passwd *, ENTRY *));
int	 p_login __P((const char *, struct passwd *, ENTRY *));
int	 p_passwd __P((const char *, struct passwd *, ENTRY *));
int	 p_shell __P((const char *, struct passwd *, ENTRY *));
int	 p_uid __P((const char *, struct passwd *, ENTRY *));
char    *ttoa __P((char *, size_t, time_t));
int	 verify __P((char *, struct passwd *));

#ifdef YP
int	check_yppasswdd __P((void));
int	pw_yp __P((struct passwd *, uid_t));
void	yppw_error __P((const char *name, int, int));
void	yppw_prompt __P((void));
struct passwd *ypgetpwnam __P((const char *));
struct passwd *ypgetpwuid __P((uid_t));
#endif

extern	void (*Pw_error) __P((const char *name, int, int));
