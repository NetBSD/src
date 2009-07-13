/*	$NetBSD: lp.h,v 1.24 2009/07/13 19:05:41 roy Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
 * 	@(#)lp.h	8.2 (Berkeley) 4/28/95
 */


/*
 * Global definitions for the line printer system.
 */

extern const char	*AF;	/* accounting file */
extern long		 BR;	/* baud rate if lp is a tty */
extern const char	*CF;	/* name of cifplot filter (per job) */
extern const char	*DF;	/* name of tex filter (per job) */
extern long		 DU;	/* daemon user-id */
extern long		 FC;	/* flags to clear if lp is a tty */
extern const char	*FF;	/* form feed string */
extern long		 FS;	/* flags to set if lp is a tty */
extern const char	*GF;	/* name of graph(1G) filter (per job) */
extern long		 HL;	/* print header last */
extern const char	*IF;	/* name of input filter (created per job) */
extern const char	*LF;	/* log file for error messages */
extern const char	*LO;	/* lock file name */
extern const char	*LP;	/* line printer device name */
extern long		 MC;	/* maximum number of copies allowed */
extern const char	*MS;	/* stty flags to set if lp is a tty */
extern long		 MX;	/* maximum number of blocks to copy */
extern const char	*NF;	/* name of ditroff(1) filter (per job) */
extern const char	*OF;	/* name of output filter (created once) */
extern const char	*PF;	/* name of postscript filter (per job) */
extern long		 PL;	/* page length */
extern long		 PW;	/* page width */
extern long		 PX;	/* page width in pixels */
extern long		 PY;	/* page length in pixels */
extern const char	*RF;	/* name of fortran text filter (per job) */
extern const char	*RG;	/* restricted group */
extern const char	*RM;	/* remote machine name */
extern const char	*RP;	/* remote printer name */
extern long		 RS;	/* restricted to those with local accounts */
extern long		 RW;	/* open LP for reading and writing */
extern long		 SB;	/* short banner instead of normal header */
extern long		 SC;	/* suppress multiple copies */
extern const char	*SD;	/* spool directory */
extern long		 SF;	/* suppress FF on each print job */
extern long		 SH;	/* suppress header page */
extern const char	*ST;	/* status file name */
extern const char	*TF;	/* name of troff(1) filter (per job) */
extern const char	*TR;	/* trailer string to be output when Q empties */
extern const char	*VF;	/* name of raster filter (per job) */
extern long		 XC;	/* flags to clear for local mode */
extern long		 XS;	/* flags to set for local mode */

extern char	line[BUFSIZ];
extern char	*bp;		/* pointer into printcap buffer */
extern const char *printer;	/* printer name */
				/* host machine name */
extern char	host[MAXHOSTNAMELEN + 1];
extern char	*from;		/* client's machine name */
extern int	remote;		/* true if sending files to a remote host */
extern const char *printcapdb[];/* printcap database array */
extern int	wait_time;	/* time to wait for remote responses */
/*
 * Structure used for building a sorted list of control files.
 */
struct queue {
	time_t	q_time;			/* modification time */
	char	q_name[MAXNAMLEN+1];	/* control file name */
};

#include <sys/cdefs.h>

__BEGIN_DECLS
struct dirent;

void     blankfill(int);
const char *checkremote(void);
int      chk(const char *);
void     displayq(int);
void     dump(const char *, const char *, int);
void	 fatal(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));
size_t	 get_line(FILE *);
const char *gethost(const char *);
int	 getport(const char *);
int	 getq(struct queue *(*[]));
void	 freeq(struct queue **, u_int);
void     header(void);
void     inform(const char *);
int      inlist(const char *, const char *);
int      iscf(const struct dirent *);
int      isowner(const char *, const char *);
void     ldump(const char *, const char *, int);
int      lockchk(const char *);
void     prank(int);
void     process(const char *);
void     rmjob(void);
void     rmremote(void);
void     show(const char *, const char *, int);
int      startdaemon(const char *);
void     nodaemon(void);
void     delay(int);
void	 getprintcap(const char *);
int	 ckqueue(char *);
__END_DECLS
