/* $NetBSD: usermgmt.h,v 1.1 1999/12/06 21:31:49 agc Exp $ */

/*
 * Copyright © 1999 Alistair G. Crooks.  All rights reserved.
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
 *	This product includes software developed by Alistair G. Crooks.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef USERMGMT_H_
#define USERMGMT_H_

#define CONFFILE	"/etc/usermgmt.conf"

#define DEF_GROUP	"other"
#define DEF_BASEDIR	"/home"
#define DEF_SKELDIR	"/etc/skel"
#define DEF_SHELL	"/bin/csh"
#define DEF_COMMENT	""
#define DEF_LOWUID	1000
#define DEF_HIGHUID	60000
#define DEF_INACTIVE	0
#define DEF_EXPIRE	(char *) NULL

#ifndef MASTER
#define MASTER		"/etc/master.passwd"
#endif

#ifndef ETCGROUP
#define ETCGROUP	"/etc/group"
#endif

#ifndef WAITSECS
#define WAITSECS	10
#endif

#ifndef NOBODY_UID
#define NOBODY_UID	32767
#endif

/* some useful constants */
enum {
	MaxShellNameLen = 256,
	MaxFileNameLen = MAXPATHLEN,
	MaxUserNameLen = 32,
	MaxFieldNameLen = 32,
	MaxCommandLen = 2048,
	MaxEntryLen = 2048,
	PasswordLength = 13,

	LowGid = 1000,
	HighGid = 60000
};

/* Full paths of programs used here */
#define CHOWN		"/usr/sbin/chown"
#define CP		"/bin/cp"
#define FALSE_PROG	"/usr/bin/false"
#define MKDIR		"/bin/mkdir -p"
#define MV		"/bin/mv"
#define RM		"/bin/rm"

#define UNSET_EXPIRY	"Null (unset)"

/* this struct describes a uid range */
typedef struct range_t {
	int	r_from;		/* low uid */
	int	r_to;		/* high uid */
} range_t;

/* this struct encapsulates the user information */
typedef struct user_t {
	int		u_uid;			/* uid of user */
	char		*u_password;		/* encrypted password */
	char		*u_comment;		/* comment field */
	int		u_homeset;		/* home dir has been set */
	char		*u_home;		/* home directory */
	char		*u_primgrp;		/* primary group */
	int		u_groupc;		/* # of secondary groups */
	char		*u_groupv[NGROUPS_MAX];	/* secondary groups */
	char		*u_shell;		/* user's shell */
	char		*u_basedir;		/* base directory for home */
	char		*u_expire;		/* when password will expire */
	int		u_inactive;		/* inactive */
	int		u_mkdir;		/* make the home directory */
	int		u_dupuid;		/* duplicate uids are allowed */
	char		*u_skeldir;		/* directory for startup files */
	unsigned	u_rsize;		/* size of range array */
	unsigned	u_rc;			/* # of ranges */
	range_t		*u_rv;			/* the ranges */
	int		u_preserve;		/* preserve uids on deletion */
} user_t;

#endif
