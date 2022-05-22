/*	$NetBSD: tip.h,v 1.37 2022/05/22 11:27:37 andvar Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
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
 *      @(#)tip.h	8.1 (Berkeley) 6/6/93
 */

/*
 * tip - terminal interface program
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <machine/endian.h>

#include <err.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*
 * Remote host attributes
 */
extern char	*DV;			/* UNIX device(s) to open */
extern char	*EL;			/* chars marking an EOL */
extern char	*CM;			/* initial connection message */
extern char	*IE;			/* EOT to expect on input */
extern char	*OE;			/* EOT to send to complete FT */
extern char	*CU;			/* call unit if making a phone call */
extern char	*AT;			/* acu type */
extern char	*PN;			/* phone number(s) */
extern char	*DI;			/* disconnect string */
extern char	*PA;			/* parity to be generated */

extern char	*PH;			/* phone number file */
extern char	*RM;			/* remote file name */
extern char	*HO;			/* host name */

extern long	BR;			/* line speed for conversation */
extern long	FS;			/* frame size for transfers */

extern long	DU;			/* this host is dialed up */
extern long	HW;			/* this device is hardwired, see hunt.c */
extern char	*ES;			/* escape character */
extern char	*EX;			/* exceptions */
extern char	*FO;			/* force (literal next) char*/
extern char	*RC;			/* raise character */
extern char	*RE;			/* script record file */
extern char	*PR;			/* remote prompt */
extern long	DL;			/* line delay for file transfers to remote */
extern long	CL;			/* char delay for file transfers to remote */
extern long	ET;			/* echocheck timeout */
extern long	HD;			/* this host is half duplex - do local echo */
extern char	DC;			/* this host is directly connected. */

/*
 * String value table
 */
typedef
	struct {
		const char *v_name;	/* whose name is it */
		uint8_t v_type;		/* for interpreting set's */
		uint8_t v_access;	/* protection of touchy ones */
		const char *v_abrev;	/* possible abbreviation */
		void *v_value;		/* casted to a union later */
				/*
				 * XXX:	this assumes that the storage space
				 *	of a pointer >= that of a long
				 */
	}
	value_t;

#define STRING	01		/* string valued */
#define BOOL	02		/* true-false value */
#define NUMBER	04		/* numeric value */
#define CHAR	010		/* character value */

#define WRITE	01		/* write access to variable */
#define	READ	02		/* read access */

#define CHANGED	01		/* low bit is used to show modification */
#define PUBLIC	1		/* public access rights */
#define PRIVATE	03		/* private to definer */
#define ROOT	05		/* root defined */

#define	TRUE	1
#define FALSE	0

#define ENVIRON	020		/* initialize out of the environment */
#define IREMOTE	040		/* initialize out of remote structure */
#define INIT	0100		/* static data space used for initialization */
#define TMASK	017

/*
 * Definition of ACU line description
 */
typedef
	struct {
		const char *acu_name;
		int	(*acu_dialer)(char *, char *);
		void	(*acu_disconnect)(void);
		void	(*acu_abort)(void);
	}
	acu_t;

/*
 * variable manipulation stuff --
 *   if we defined the value entry in value_t, then we couldn't
 *   initialize it in vars.c, so we cast it as needed to keep lint
 *   happy.
 */

#define value(v)	vtable[v].v_value

#define	number(v)	((int)(intptr_t)(v))
#define	boolean(v)	((short)(intptr_t)(v))
#define	character(v)	((char)(intptr_t)(v))
#define	address(v)	((long *)(intptr_t)(v))

#define	setnumber(v,n)		do { (v) = (char *)(intptr_t)(n); } while (0)
#define	setboolean(v,n)		do { (v) = (char *)(intptr_t)(n); } while (0)
#define	setcharacter(v,n)	do { (v) = (char *)(intptr_t)(n); } while (0)
#define	setaddress(v,n)		do { (v) = (char *)(intptr_t)(n); } while (0)

/*
 * Escape command table definitions --
 *   lookup in this table is performed when ``escapec'' is recognized
 *   at the beginning of a line (as defined by the eolmarks variable).
*/

typedef
	struct {
		char	e_char;			/* char to match on */
		char	e_flags;		/* experimental, privileged */
		const char *e_help;		/* help string */
		void 	(*e_func)(char);	/* command */
	}
	esctable_t;

#define NORM	00		/* normal protection, execute anyone */
#define EXP	01		/* experimental, mark it with a `*' on help */
#define PRIV	02		/* privileged, root execute only */

extern int	vflag;		/* verbose during reading of .tiprc file */
extern value_t	vtable[];	/* variable table */

/*
 * Definition of indices into variable table so
 *  value(DEFINE) turns into a static address.
 *
 * XXX: keep in sync with vtable[] in vars.c
 */

#define BEAUTIFY	0
#define BAUDRATE	1
#define DIALTIMEOUT	2
#define EOFREAD		3
#define EOFWRITE	4
#define EOL		5
#define ESCAPE		6
#define EXCEPTIONS	7
#define FORCE		8
#define FRAMESIZE	9
#define HOST		10
#define PHONES		11
#define PROMPT		12
#define RAISE		13
#define RAISECHAR	14
#define RECORD		15
#define REMOTE		16
#define SCRIPT		17
#define TABEXPAND	18
#define VERBOSE		19
#define SHELL		20
#define HOME		21
#define ECHOCHECK	22
#define DISCONNECT	23
#define TAND		24
#define LDELAY		25
#define CDELAY		26
#define ETIMEOUT	27
#define RAWFTP		28
#define HALFDUPLEX	29
#define	LECHO		30
#define	PARITY		31
#define	HARDWAREFLOW	32

extern struct termios	term;		/* current mode of terminal */
extern struct termios	defterm;	/* initial mode of terminal */
extern struct termios	defchars;	/* current mode with initial chars */

extern FILE	*fscript;		/* FILE for scripting */

extern int	attndes[2];		/* coprocess wakeup channel */
extern int	fildes[2];		/* file transfer synchronization channel */
extern int	repdes[2];		/* read process synchronization channel */
extern int	FD;			/* open file descriptor to remote host */
#ifndef __lint__  /* not used by hayes.c, but used by some other dialers */
extern int	AC;			/* open file descriptor to dialer (v831 only) */
#endif /*__lint__*/
extern int	sfd;			/* for ~< operation */
extern int	pid;			/* pid of tipout */
extern uid_t	uid, euid;		/* real and effective user id's */
extern gid_t	gid, egid;		/* real and effective group id's */
extern int	stop;			/* stop transfer session flag */
extern int	quit;			/* same; but on other end */
extern int	stoprompt;		/* for interrupting a prompt session */
extern int	timedout;		/* ~> transfer timedout */
extern int	cumode;			/* simulating the "cu" program */
extern int	bits8;			/* terminal is in 8-bit mode */
#define STRIP_PAR	(bits8 ? 0377 : 0177)

extern char	fname[80];		/* file name buffer for ~< */
extern char	copyname[80];		/* file name buffer for ~> */
extern char	ccc;			/* synchronization character */

extern int	odisc;			/* initial tty line discipline */

extern acu_t		acutable[];
extern esctable_t	etable[];
extern unsigned char	evenpartab[];

void	alrmtimeout(int);
int	any(char, const char *);
void	chdirectory(char);
void	cleanup(int) __dead;
const char   *tip_connect(void);
void	consh(char);
char   *ctrl(char);
void	cumain(int, char **);
void	cu_put(char);
void	cu_take(char);
void	disconnect(const char *);
char   *expand(char *);
void	finish(char) __dead;
void	genbrk(char);
void	getfl(char);
char   *getremote(char *);
void	hardwareflow(const char *);
void	help(char);
int	hunt(char *);
char   *interp(const char *);
void	pipefile(char);
void	pipeout(char);
int	prompt(const char *, char *, size_t);
void	xpwrite(int, char *, size_t);
void	raw(void);
void	sendchar(char);
void	sendfile(char);
void	setparity(const char *);
void	setscript(void);
void	shell(char);
void	suspend(char);
void	tandem(const char *);
void	tipabort(const char *) __dead;
void	tipout(void) __dead;
int	ttysetup(speed_t);
void	unraw(void);
void	variable(char);
void	vinit(void);
char   *vinterp(char *, char);
void	vlex(char *);
int	vstring(const char *, char *);

void	biz22_abort(void);
void	biz22_disconnect(void);
int	biz22f_dialer(char *, char *);
int	biz22w_dialer(char *, char *);
void	biz31_abort(void);
void	biz31_disconnect(void);
int	biz31f_dialer(char *, char *);
int	biz31w_dialer(char *, char *);
void	cour_abort(void);
int	cour_dialer(char *, char *);
void	cour_disconnect(void);
int	df02_dialer(char *, char *);
int	df03_dialer(char *, char *);
void	df_abort(void);
void	df_disconnect(void);
void	dn_abort(void);
int	dn_dialer(char *, char *);
void	dn_disconnect(void);
void	hay_abort(void);
int	hay_dialer(char *, char *);
void	hay_disconnect(void);
void	t3000_abort(void);
int	t3000_dialer(char *, char *);
void	t3000_disconnect(void);
void	v3451_abort(void);
int	v3451_dialer(char *, char *);
void	v3451_disconnect(void);
void	v831_abort(void);
int	v831_dialer(char *, char *);
void	v831_disconnect(void);
void	ven_abort(void);
int	ven_dialer(char *, char *);
void	ven_disconnect(void);
