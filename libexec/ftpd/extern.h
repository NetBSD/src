/*	$NetBSD: extern.h,v 1.27 2000/06/14 13:44:22 lukem Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)extern.h	8.2 (Berkeley) 4/4/94
 */

/*-
 * Copyright (c) 1997-2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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

/*
 * Copyright (C) 1997 and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

void	blkfree(char **);
char   *conffilename(const char *);
char  **copyblk(char **);
void	count_users(void);
void	cwd(const char *);
FILE   *dataconn(const char *, off_t, const char *);
void	delete(const char *);
char  **do_conversion(const char *);
void	dologout(int);
void	fatal(const char *);
void	feat(void);
int	format_file(const char *, int);
int	ftpd_pclose(FILE *);
FILE   *ftpd_popen(char *[], const char *, int);
char   *getline(char *, int, FILE *);
void	init_curclass(void);
void	logcmd(const char *, off_t, const char *, const char *,
	    const struct timeval *, const char *);
void	logwtmp(const char *, const char *, const char *);
struct tab *lookup(struct tab *, const char *);
void	lreply(int, const char *, ...);
void	makedir(const char *);
void	mlsd(const char *);
void	mlst(const char *);
void	opts(const char *);
void	parse_conf(const char *);
void	pass(const char *);
void	passive(void);
void	long_passive(char *, int);
void	perror_reply(int, const char *);
void	pwd(void);
void	removedir(const char *);
void	renamecmd(const char *, const char *);
char   *renamefrom(const char *);
void	reply(int, const char *, ...);
void	retrieve(char *[], const char *);
void	send_file_list(const char *);
void	show_chdir_messages(int);
void	sizecmd(const char *);
void	statcmd(void);
void	statfilecmd(const char *);
void	store(const char *, const char *, int);
int	strsuftoi(const char *);
void	user(const char *);
char   *xstrdup(const char *);
void	yyerror(char *);

typedef long long qdfmt_t;
typedef unsigned long long qufmt_t;

typedef enum {
	CLASS_GUEST,
	CLASS_CHROOT,
	CLASS_REAL
} class_ft;

struct tab {
	char	*name;
	short	 token;
	short	 state;
	short	 flags;	/* 1 if command implemented, 2 if has options */
	char	*help;
	char	*options;
};

struct ftpconv {
	struct ftpconv	*next;
	char		*suffix;	/* Suffix of requested name */
	char		*types;		/* Valid file types */
	char		*disable;	/* File to disable conversions */
	char		*command;	/* Command to do the conversion */
};

struct ftpclass {
	int		 checkportcmd;	/* Check PORT commands are valid */
	char		*classname;	/* Current class */
	struct ftpconv	*conversions;	/* List of conversions */
	char		*display;	/* Files to display upon chdir */
	int	 	 limit;		/* Max connections (-1 = unlimited) */
	char		*limitfile;	/* File to display if limit reached */
	int		 maxrateget;	/* Maximum get transfer rate throttle */
	int		 maxrateput;	/* Maximum put transfer rate throttle */
	unsigned int	 maxtimeout;	/* Maximum permitted timeout */
	int		 modify;	/* Allow CHMOD, DELE, MKD, RMD, RNFR,
					   UMASK */
	char		*motd;		/* MotD file to display after login */
	char		*notify;	/* Files to notify about upon chdir */
	int		 passive;	/* Allow PASV mode */
	int		 portmin;	/* Minumum port for passive mode */
	int		 portmax;	/* Maximum port for passive mode */
	int		 rateget;	/* Get (RETR) transfer rate throttle */
	int		 rateput;	/* Put (STOR) transfer rate throttle */
	unsigned int	 timeout;	/* Default timeout */
	class_ft	 type;		/* Class type */
	mode_t		 umask;		/* Umask to use */
	int		 upload;	/* As per modify, but also allow
					   APPE, STOR, STOU */
};

#include <netinet/in.h>

union sockunion {
	struct sockinet {
		u_char si_len;
		u_char si_family;
		u_short si_port;
	} su_si;
	struct sockaddr_in  su_sin;
	struct sockaddr_in6 su_sin6;
};
#define su_len		su_si.si_len
#define su_family	su_si.si_family
#define su_port		su_si.si_port

extern  int		yyparse(void);

#ifndef	GLOBAL
#define	GLOBAL	extern
#endif


GLOBAL	union sockunion	ctrl_addr;
GLOBAL	union sockunion	data_dest;
GLOBAL	union sockunion	data_source;
GLOBAL	union sockunion	his_addr;
GLOBAL	union sockunion	pasv_addr;
GLOBAL	int		connections;
GLOBAL	struct ftpclass	curclass;
GLOBAL	int		debug;
GLOBAL	jmp_buf		errcatch;
GLOBAL	int		form;
GLOBAL	int		hasyyerrored;
GLOBAL	char		hostname[MAXHOSTNAMELEN+1];
#ifdef KERBEROS5
GLOBAL	krb5_context	kcontext;
#endif
GLOBAL	int		logged_in;
GLOBAL	int		logging;
GLOBAL	int		pdata;			/* for passive mode */
#ifdef HASSETPROCTITLE
GLOBAL	char		proctitle[BUFSIZ];	/* initial part of title */
#endif
GLOBAL	struct passwd  *pw;
GLOBAL	int		quietmessages;
GLOBAL	char		remotehost[MAXHOSTNAMELEN+1];
GLOBAL	off_t		restart_point;
GLOBAL	char		tmpline[7];
GLOBAL	sig_atomic_t	transflag;
GLOBAL	int		type;
GLOBAL	int		usedefault;		/* for data transfers */

						/* total file data bytes */
GLOBAL	off_t		total_data_in,  total_data_out,  total_data;
						/* total number of data files */
GLOBAL	off_t		total_files_in, total_files_out, total_files;
						/* total bytes */
GLOBAL	off_t		total_bytes_in, total_bytes_out, total_bytes;
						/* total number of xfers */
GLOBAL	off_t		total_xfers_in, total_xfers_out, total_xfers;

extern	struct tab	cmdtab[];

#define	INTERNAL_LS	"/bin/ls"


#define CMD_IMPLEMENTED(x)	((x)->flags != 0)
#define CMD_HAS_OPTIONS(x)	((x)->flags == 2)

#define CURCLASSTYPE	curclass.type == CLASS_GUEST  ? "GUEST"  : \
		    	curclass.type == CLASS_CHROOT ? "CHROOT" : \
		    	curclass.type == CLASS_REAL   ? "REAL"   : \
			"<unknown>"

#define ISDOTDIR(x)	(x[0] == '.' && x[1] == '\0')
#define ISDOTDOTDIR(x)	(x[0] == '.' && x[1] == '.' && x[2] == '\0')

#define EMPTYSTR(p)	((p) == NULL || *(p) == '\0')
#define NEXTWORD(P, W)	do { \
				(W) = strsep(&(P), " \t"); \
			} while ((W) != NULL && *(W) == '\0')
#define PLURAL(s)	((s) == 1 ? "" : "s")
#define REASSIGN(X,Y)	do { if (X) free(X); (X)=(Y); } while (/*CONSTCOND*/0)
