/*	$NetBSD: ftp_var.h,v 1.34 1999/07/02 08:07:42 itojun Exp $	*/

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

/*
 * Copyright (c) 1985, 1989, 1993, 1994
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
 *	@(#)ftp_var.h	8.4 (Berkeley) 10/9/94
 */

/*
 * FTP global variables.
 */

#ifdef SMALL
#undef	NO_ABOUT
#define	NO_ABOUT
#undef	NO_EDITCOMPLETE
#define	NO_EDITCOMPLETE
#undef	NO_PROGRESS
#define	NO_PROGRESS
#endif

#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <setjmp.h>
#include <stringlist.h>

#ifndef NO_EDITCOMPLETE
#include <histedit.h>
#endif /* !NO_EDITCOMPLETE */

#include "extern.h"

#define FTPBUFLEN	MAXPATHLEN + 200
#define MAX_IN_PORT_T	0xffffU

#define HASHBYTES	1024	/* default mark for `hash' command */
#define	DEFAULTINCR	1024	/* default increment for `rate' command */
#define STALLTIME	5	/* # of seconds of no xfer before "stalling" */

#define	FTP_PORT	21	/* default if ! getservbyname("ftp/tcp") */
#define	HTTP_PORT	80	/* default if ! getservbyname("http/tcp") */
#ifndef	GATE_PORT
#define	GATE_PORT	21	/* default if ! getservbyname("ftpgate/tcp") */
#endif
#ifndef	GATE_SERVER
#define	GATE_SERVER	""	/* default server */
#endif

#define PAGER		"more"	/* default pager if $PAGER isn't set */

/*
 * Options and other state info.
 */
int	trace;			/* trace packets exchanged */
int	hash;			/* print # for each buffer transferred */
int	mark;			/* number of bytes between hashes */
int	sendport;		/* use PORT/LPRT cmd for each data connection */
int	verbose;		/* print messages coming back from server */
int	connected;		/* 1 = connected to server, -1 = logged in */
int	fromatty;		/* input is from a terminal */
int	interactive;		/* interactively prompt on m* cmds */
int	confirmrest;		/* confirm rest of current m* cmd */
int	debug;			/* debugging level */
int	bell;			/* ring bell on cmd completion */
int	doglob;			/* glob local file names */
int	autologin;		/* establish user account on connection */
int	proxy;			/* proxy server connection active */
int	proxflag;		/* proxy connection exists */
int	gatemode;		/* use gate-ftp */
char   *gateserver;		/* server to use for gate-ftp */
int	sunique;		/* store files on server with unique name */
int	runique;		/* store local files with unique name */
int	mcase;			/* map upper to lower case for mget names */
int	ntflag;			/* use ntin ntout tables for name translation */
int	mapflag;		/* use mapin mapout templates on file names */
int	preserve;		/* preserve modification time on files */
int	progress;		/* display transfer progress bar */
int	code;			/* return/reply code for ftp command */
int	crflag;			/* if 1, strip car. rets. on ascii gets */
char	pasv[BUFSIZ];		/* passive port for proxy data connection */
int	passivemode;		/* passive mode enabled */
int	activefallback;		/* fall back to active mode if passive fails */
char   *altarg;			/* argv[1] with no shell-like preprocessing  */
char	ntin[17];		/* input translation table */
char	ntout[17];		/* output translation table */
char	mapin[MAXPATHLEN];	/* input map template */
char	mapout[MAXPATHLEN];	/* output map template */
char	typename[32];		/* name of file transfer type */
int	type;			/* requested file transfer type */
int	curtype;		/* current file transfer type */
char	structname[32];		/* name of file transfer structure */
int	stru;			/* file transfer structure */
char	formname[32];		/* name of file transfer format */
int	form;			/* file transfer format */
char	modename[32];		/* name of file transfer mode */
int	mode;			/* file transfer mode */
char	bytename[32];		/* local byte size in ascii */
int	bytesize;		/* local byte size in binary */
int	anonftp;		/* automatic anonymous login */
int	dirchange;		/* remote directory changed by cd command */
int	flushcache;		/* set HTTP cache flush headers with request */
int	rate_get;		/* maximum get xfer rate */
int	rate_get_incr;		/* increment for get xfer rate */
int	rate_put;		/* maximum put xfer rate */
int	rate_put_incr;		/* increment for put xfer rate */
int	retry_connect;		/* seconds between retrying connection */
int	ttywidth;		/* width of tty */
char   *tmpdir;			/* temporary directory */
FILE   *ttyout;			/* stdout, or stderr if retrieving to stdout */

#ifndef NO_EDITCOMPLETE
int	  editing;		/* command line editing enabled */
EditLine *el;			/* editline(3) status structure */
History  *hist;			/* editline(3) history structure */
char	 *cursor_pos;		/* cursor position we're looking for */
size_t	  cursor_argc;		/* location of cursor in margv */
size_t	  cursor_argo;		/* offset of cursor in margv[cursor_argc] */
#endif /* !NO_EDITCOMPLETE */

off_t	bytes;			/* current # of bytes read */
off_t	filesize;		/* size of file being transferred */
char   *direction;		/* direction transfer is occurring */
off_t	restart_point;		/* offset to restart transfer */

char   *hostname;		/* name of host connected to */
int	unix_server;		/* server is unix, can use binary for ascii */
int	unix_proxy;		/* proxy is unix, can use binary for ascii */

#if 0
in_port_t	ftpport;	/* port number to use for FTP connections */
in_port_t	httpport;	/* port number to use for HTTP connections */
in_port_t	gateport;	/* port number to use for gateftp connections */
#else
char 	*ftpport;		/* port number to use for FTP connections */
char	*httpport;		/* port number to use for HTTP connections */
char	*gateport;		/* port number to use for gateftp connections */
#endif

const char	*ftpproxy;	/* ftp:// proxy server */
const char	*httpproxy;	/* http:// proxy server */
const char	*no_proxy;	/* list of domains not to proxy */

char   *outfile;		/* filename to output URLs to */
int	restartautofetch;	/* restart auto-fetch */

jmp_buf	toplevel;		/* non-local goto stuff for cmd scanner */

char	line[FTPBUFLEN];	/* input line buffer */
char	*stringbase;		/* current scan point in line buffer */
char	argbuf[FTPBUFLEN];	/* argument storage buffer */
char	*argbase;		/* current storage point in arg buffer */
StringList *marg_sl;		/* stringlist containing margv */
int	margc;			/* count of arguments on input line */
#define margv (marg_sl->sl_str)	/* args parsed from input line */
int     cpend;                  /* flag: if != 0, then pending server reply */
int	mflag;			/* flag: if != 0, then active multi command */

int	options;		/* used during socket creation */

int	sndbuf_size;		/* socket send buffer size */
int	sndbuf_manual;		/* sndbuf_size was set manually; override
				   conf file */
int	rcvbuf_size;		/* socket receive buffer size */
int	rcvbuf_manual;		/* rcvbuf_size was set manually; override
				   conf file */

/*
 * Format of command table.
 */
struct cmd {
	char	*c_name;	/* name of command */
	char	*c_help;	/* help string */
	char	 c_bell;	/* give bell when command completes */
	char	 c_conn;	/* must be connected to use command */
	char	 c_proxy;	/* proxy server may execute */
#ifndef NO_EDITCOMPLETE
	char	*c_complete;	/* context sensitive completion list */
#endif /* !NO_EDITCOMPLETE */
	void	(*c_handler) __P((int, char **)); /* function to call */
};

struct macel {
	char mac_name[9];	/* macro name */
	char *mac_start;	/* start of macro in macbuf */
	char *mac_end;		/* end of macro in macbuf */
};

int	macnum;			/* number of defined macros */
struct macel macros[16];
char	macbuf[4096];
