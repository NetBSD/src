/*
 * Copyright (c) 1997 Erez Zadok
 * Copyright (c) 1989 Jan-Simon Pendry
 * Copyright (c) 1989 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      %W% (Berkeley) %G%
 *
 * $Id: hlfsd.h,v 1.1.1.1 1997/07/24 21:22:42 christos Exp $
 *
 * HLFSD was written at Columbia University Computer Science Department, by
 * Erez Zadok <ezk@cs.columbia.edu> and Alexander Dupuy <dupuy@cs.columbia.edu>
 * It is being distributed under the same terms and conditions as amd does.
 */

#ifndef _HLFSD_HLFS_H
#define _HLFSD_HLFS_H

/*
 * MACROS AND CONSTANTS:
 */

#define HLFSD_VERSION	"hlfsd 1.1 (March 4, 1997)"
#define PERS_SPOOLMODE	0755
#define OPEN_SPOOLMODE	01777
#define ROOTID		-1	/* don't change this from being -1 */
#define SLINKID		-2	/* don't change this from being -2 */
#define DOTSTRING	"."

#define DOTCOOKIE	1
#define DOTDOTCOOKIE	2
#define SLINKCOOKIE	3

#define ALT_SPOOLDIR "/var/hlfs" /* symlink to use if others fail */
#define HOME_SUBDIR ".hlfsdir"	/* dirname in user's home dir */
#define DEFAULT_DIRNAME "/hlfs/home"
#define DEFAULT_INTERVAL 900	/* secs b/t re-reads of the password maps */
#define DEFAULT_CACHE_INTERVAL 300 /* secs during which assume a link is up */
#define DEFAULT_HLFS_GROUP	"hlfs"	/* Group name for special hlfs_gid */

#define PROGNAMESZ	(MAXHOSTNAMELEN - 5)

#ifdef MNT2_NFS_OPT_SYMTTL
# define SYMTTL_ATTR_CACHE_VALUE  0
#else /* MNT2_NFS_OPT_SYMTTL */
# define SYMTTL_ATTR_CACHE_VALUE  1
#endif /* MNT2_NFS_OPT_SYMTTL */

#ifdef HAVE_SYSLOG
# define DEFAULT_LOGFILE "syslog"
#else /* not HAVE)_SYSLOG */
# define DEFAULT_LOGFILE 0
#endif /* not HAVE)_SYSLOG */

#define ERRM ": %m"
#define fatalerror(str) \
  (fatal (strcat (strnsave ((str), strlen ((str)) + sizeof (ERRM) - 1), ERRM)))

/*
 * TYPDEFS:
 */
typedef struct uid2home_t uid2home_t;
typedef struct username2uid_t username2uid_t;


/*
 * STRUCTURES:
 */
struct uid2home_t {
  uid_t uid;
  pid_t child;
  char *home;
  char *uname;
  u_long last_access_time;
  int last_status;		/* 0=used $HOME/.hlfsspool; !0=used alt dir */
};

struct username2uid_t {
  char *username;
  uid_t uid;
  char *home;
};

/*
 * EXTERNALS:
 */
extern RETSIGTYPE cleanup(int);
extern RETSIGTYPE interlock(int);
extern SVCXPRT *nfs_program_2_transp;	/* For quick_reply() */
extern SVCXPRT *nfsxprt;
extern char *alt_spooldir;
extern char *home_subdir;
extern char *homedir(int);
extern char *mailbox(int, char *);
extern char *slinkname;
extern char mboxfile[];
extern int cache_interval;
extern int hlfs_gid;
extern int noverify;
extern int serverpid;
extern int sys_nerr;
extern int untab_index(char *username);
extern am_nfs_fh *root_fhp;
extern am_nfs_fh root;
extern nfstime startup;
extern uid2home_t *plt_search(int);
extern username2uid_t *untab;	/* user name table */
extern void fatal(char *);
extern void init_homedir(void);

#if defined(DEBUG) || defined(DEBUG_PRINT)
extern void plt_dump(uid2home_t *, pid_t);
extern void plt_print(int);
#endif /* defined(DEBUG) || defined(DEBUG_PRINT) */

#endif /* _HLFSD_HLFS_H */
