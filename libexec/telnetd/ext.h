/*	$NetBSD: ext.h,v 1.18 2003/08/07 09:46:51 agc Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	from: @(#)ext.h	8.2 (Berkeley) 12/15/93
 */

/*
 * Telnet server variable declarations
 */
extern char	options[256];
extern char	do_dont_resp[256];
extern char	will_wont_resp[256];
extern int	linemode;	/* linemode on/off */
#ifdef	LINEMODE
extern int	uselinemode;	/* what linemode to use (on/off) */
extern int	editmode;	/* edit modes in use */
extern int	useeditmode;	/* edit modes to use */
extern int	alwayslinemode;	/* command line option */
# ifdef	KLUDGELINEMODE
extern int	lmodetype;	/* Client support for linemode */
# endif	/* KLUDGELINEMODE */
#endif	/* LINEMODE */
extern int	flowmode;	/* current flow control state */
extern int	restartany;	/* restart output on any character state */
#ifdef DIAGNOSTICS
extern int	diagnostic;	/* telnet diagnostic capabilities */
#endif /* DIAGNOSTICS */
#ifdef SECURELOGIN
extern int	require_secure_login;
#endif
#ifdef AUTHENTICATION
extern int	auth_level;
#endif

extern slcfun	slctab[NSLC + 1];	/* slc mapping table */
extern char	*terminaltype;

/*
 * I/O data buffers, pointers, and counters.
 */
extern char	ptyobuf[BUFSIZ+NETSLOP], *pfrontp, *pbackp;
extern char	netibuf[BUFSIZ], *netip;
extern char	netobuf[BUFSIZ+NETSLOP], *nfrontp, *nbackp;
extern char	*neturg;		/* one past last bye of urgent data */
extern int	pcc, ncc;
extern int	pty, net;
extern char	*line;
extern int	SYNCHing;		/* we are in TELNET SYNCH mode */

#include <sys/cdefs.h>

extern void
	_termstat __P((void)),
	add_slc __P((int, int, int)),
	check_slc __P((void)),
	change_slc __P((int, int, int)),
	cleanup __P((int)),
	clientstat __P((int, int, int)),
	copy_termbuf __P((char *, int)),
	deferslc __P((void)),
	defer_terminit __P((void)),
	do_opt_slc __P((unsigned char *, int)),
	doeof __P((void)),
	dooption __P((int)),
	dontoption __P((int)),
	edithost __P((char *, char *)),
	fatal __P((int, const char *)),
	fatalperror __P((int, const char *)),
	get_slc_defaults __P((void)),
	init_env __P((void)),
	init_termbuf __P((void)),
	interrupt __P((void)),
	localstat __P((void)),
	flowstat __P((void)),
	netclear __P((void)),
	netflush __P((void)),
#ifdef DIAGNOSTICS
	printoption __P((const char *, int)),
	printdata __P((char *, char *, int)),
#ifndef ENCRYPTION
	printsub __P((int, unsigned char *, int)),
#endif
#endif
	ptyflush __P((void)),
	putchr __P((int)),
	recv_ayt __P((void)),
	send_do __P((int, int)),
	send_dont __P((int, int)),
	send_slc __P((void)),
	send_status __P((void)),
	send_will __P((int, int)),
	send_wont __P((int, int)),
	sendbrk __P((void)),
	sendsusp __P((void)),
	set_termbuf __P((void)),
	start_login __P((char *, int, char *)),
	start_slc __P((int)),
	startslave __P((char *, int, char *)),
	suboption __P((void)),
	telrcv __P((void)),
	ttloop __P((void)),
	tty_binaryin __P((int)),
	tty_binaryout __P((int));

extern char *
	putf __P((char *, char *));

extern int
	end_slc __P((unsigned char **)),
	getnpty __P((void)),
	getpty __P((int *)),
	spcset __P((int, cc_t *, cc_t **)),
	stilloob __P((int)),
	terminit __P((void)),
	termstat __P((void)),
	tty_flowmode __P((void)),
	tty_restartany __P((void)),
	tty_isbinaryin __P((void)),
	tty_isbinaryout __P((void)),
	tty_iscrnl __P((void)),
	tty_isecho __P((void)),
	tty_isediting __P((void)),
	tty_islitecho __P((void)),
	tty_isnewmap __P((void)),
	tty_israw __P((void)),
	tty_issofttab __P((void)),
	tty_istrapsig __P((void)),
	tty_linemode __P((void));

extern void
	tty_rspeed __P((int)),
	tty_setecho __P((int)),
	tty_setedit __P((int)),
	tty_setlinemode __P((int)),
	tty_setlitecho __P((int)),
	tty_setsig __P((int)),
	tty_setsofttab __P((int)),
	tty_tspeed __P((int)),
	willoption __P((int)),
	wontoption __P((int)),
	writenet __P((unsigned char *, int));

extern int output_data __P((const char *, ...))
	__attribute__((__format__(__printf__, 1, 2)));
extern int output_datalen __P((const char *, size_t));

#ifdef	ENCRYPTION
extern char	*nclearto;
#endif	/* ENCRYPTION */

/*
 * The following are some clocks used to decide how to interpret
 * the relationship between various variables.
 */
extern struct {
    int
	system,			/* what the current time is */
	echotoggle,		/* last time user entered echo character */
	modenegotiated,		/* last time operating mode negotiated */
	didnetreceive,		/* last time we read data from network */
	ttypesubopt,		/* ttype subopt is received */
	tspeedsubopt,		/* tspeed subopt is received */
	environsubopt,		/* environ subopt is received */
	oenvironsubopt,		/* old environ subopt is received */
	xdisplocsubopt,		/* xdisploc subopt is received */
	baseline,		/* time started to do timed action */
	gotDM;			/* when did we last see a data mark */
} clocks;

#ifndef	DEFAULT_IM
# define DEFAULT_IM	"\r\n\r\n4.4 BSD UNIX (%h) (%t)\r\n\r\r\n\r"
#endif

#ifdef AUTHENTICATION
#include <libtelnet/auth.h>
#endif

#ifdef ENCRYPTION
#include <libtelnet/encrypt.h>
#endif
