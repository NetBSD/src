/*	$NetBSD: externs.h,v 1.21 2001/06/18 11:23:01 wiz Exp $	*/

/*
 * Copyright (c) 1988, 1990, 1993
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
 *	from: @(#)externs.h	8.3 (Berkeley) 5/30/95
 */

#ifndef	BSD
# define BSD 43
#endif

/*
 * ucb stdio.h defines BSD as something wierd
 */
#if defined(sun) && defined(__svr4__) && !defined(BSD)
#define BSD 43
#endif

#ifndef	USE_TERMIO
# if BSD > 43 || defined(SYSV_TERMIO)
#  define USE_TERMIO
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#if defined(CRAY) && !defined(NO_BSD_SETJMP)
#include <bsdsetjmp.h>
#endif
#ifndef	FILIO_H
#include <sys/ioctl.h>
#else
#include <sys/filio.h>
#endif
#include <errno.h>
#ifdef	USE_TERMIO
# ifndef	VINTR
#  ifdef SYSV_TERMIO
#   include <sys/termio.h>
#  else
#   include <sys/termios.h>
#   define termio termios
#  endif
# else
#  if defined(TCSANOW)
#   define termio termios
#  endif
# endif
#endif
#if defined(NO_CC_T) || !defined(USE_TERMIO)
# if !defined(USE_TERMIO)
typedef char cc_t;
# else
typedef unsigned char cc_t;
# endif
#endif

#ifndef	NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#if defined(IPSEC)
#include <netinet6/ipsec.h>
#if defined(IPSEC_POLICY_IPSEC)
extern char *ipsec_policy_in;
extern char *ipsec_policy_out;
#endif
#endif

#ifndef	_POSIX_VDISABLE
# ifdef sun
#  include <sys/param.h>	/* pick up VDISABLE definition, mayby */
# endif
# ifdef VDISABLE
#  define _POSIX_VDISABLE VDISABLE
# else
#  define _POSIX_VDISABLE ((cc_t)'\377')
# endif
#endif

#define	SUBBUFSIZE	256

#include <sys/cdefs.h>
#define P __P

extern int
    autologin,		/* Autologin enabled */
    skiprc,		/* Don't process the ~/.telnetrc file */
    eight,		/* use eight bit mode (binary in and/or out */
    flushout,		/* flush output */
    connected,		/* Are we connected to the other side? */
    globalmode,		/* Mode tty should be in */
    In3270,		/* Are we in 3270 mode? */
    telnetport,		/* Are we connected to the telnet port? */
    localflow,		/* Flow control handled locally */
    restartany,		/* If flow control, restart output on any character */
    localchars,		/* we recognize interrupt/quit */
    donelclchars,	/* the user has set "localchars" */
    showoptions,
    net,		/* Network file descriptor */
    tin,		/* Terminal input file descriptor */
    tout,		/* Terminal output file descriptor */
    crlf,		/* Should '\r' be mapped to <CR><LF> (or <CR><NUL>)? */
    autoflush,		/* flush output when interrupting? */
    autosynch,		/* send interrupt characters with SYNCH? */
    SYNCHing,		/* Is the stream in telnet SYNCH mode? */
    donebinarytoggle,	/* the user has put us in binary */
    dontlecho,		/* do we suppress local echoing right now? */
    crmod,
    netdata,		/* Print out network data flow */
    prettydump,		/* Print "netdata" output in user readable format */
#if	defined(unix)
#if	defined(TN3270)
    cursesdata,		/* Print out curses data flow */
    apitrace,		/* Trace API transactions */
#endif	/* defined(TN3270) */
    termdata,		/* Print out terminal data flow */
#endif	/* defined(unix) */
    debug,		/* Debug level */
    doaddrlookup,	/* do a reverse address lookup? */
    clienteof;		/* Client received EOF */

extern cc_t escape;	/* Escape to command mode */
extern cc_t rlogin;	/* Rlogin mode escape character */
#ifdef	KLUDGELINEMODE
extern cc_t echoc;	/* Toggle local echoing */
#endif

extern char
    *prompt;		/* Prompt for command. */

extern char
    doopt[],
    dont[],
    will[],
    wont[],
    options[],		/* All the little options */
    *hostname;		/* Who are we connected to? */

#ifdef	ENCRYPTION
extern void (*encrypt_output) P((unsigned char *, int));
extern int (*decrypt_input) P((int));
#endif	/* ENCRYPTION */

/*
 * We keep track of each side of the option negotiation.
 */

#define	MY_STATE_WILL		0x01
#define	MY_WANT_STATE_WILL	0x02
#define	MY_STATE_DO		0x04
#define	MY_WANT_STATE_DO	0x08

/*
 * Macros to check the current state of things
 */

#define	my_state_is_do(opt)		(options[opt]&MY_STATE_DO)
#define	my_state_is_will(opt)		(options[opt]&MY_STATE_WILL)
#define my_want_state_is_do(opt)	(options[opt]&MY_WANT_STATE_DO)
#define my_want_state_is_will(opt)	(options[opt]&MY_WANT_STATE_WILL)

#define	my_state_is_dont(opt)		(!my_state_is_do(opt))
#define	my_state_is_wont(opt)		(!my_state_is_will(opt))
#define my_want_state_is_dont(opt)	(!my_want_state_is_do(opt))
#define my_want_state_is_wont(opt)	(!my_want_state_is_will(opt))

#define	set_my_state_do(opt)		{options[opt] |= MY_STATE_DO;}
#define	set_my_state_will(opt)		{options[opt] |= MY_STATE_WILL;}
#define	set_my_want_state_do(opt)	{options[opt] |= MY_WANT_STATE_DO;}
#define	set_my_want_state_will(opt)	{options[opt] |= MY_WANT_STATE_WILL;}

#define	set_my_state_dont(opt)		{options[opt] &= ~MY_STATE_DO;}
#define	set_my_state_wont(opt)		{options[opt] &= ~MY_STATE_WILL;}
#define	set_my_want_state_dont(opt)	{options[opt] &= ~MY_WANT_STATE_DO;}
#define	set_my_want_state_wont(opt)	{options[opt] &= ~MY_WANT_STATE_WILL;}

/*
 * Make everything symmetrical
 */

#define	HIS_STATE_WILL			MY_STATE_DO
#define	HIS_WANT_STATE_WILL		MY_WANT_STATE_DO
#define HIS_STATE_DO			MY_STATE_WILL
#define HIS_WANT_STATE_DO		MY_WANT_STATE_WILL

#define	his_state_is_do			my_state_is_will
#define	his_state_is_will		my_state_is_do
#define his_want_state_is_do		my_want_state_is_will
#define his_want_state_is_will		my_want_state_is_do

#define	his_state_is_dont		my_state_is_wont
#define	his_state_is_wont		my_state_is_dont
#define his_want_state_is_dont		my_want_state_is_wont
#define his_want_state_is_wont		my_want_state_is_dont

#define	set_his_state_do		set_my_state_will
#define	set_his_state_will		set_my_state_do
#define	set_his_want_state_do		set_my_want_state_will
#define	set_his_want_state_will		set_my_want_state_do

#define	set_his_state_dont		set_my_state_wont
#define	set_his_state_wont		set_my_state_dont
#define	set_his_want_state_dont		set_my_want_state_wont
#define	set_his_want_state_wont		set_my_want_state_dont


extern FILE
    *NetTrace;		/* Where debugging output goes */
extern unsigned char
    NetTraceFile[];	/* Name of file where debugging output goes */

extern jmp_buf
    peerdied,
    toplevel;		/* For error conditions. */


/* authenc.c */
int telnet_net_write P((unsigned char *, int));
void net_encrypt P((void));
int telnet_spin P((void));
char *telnet_getenv P((char *));
char *telnet_gets P((char *, char *, int, int));

/* commands.c */
int send_tncmd P((void (*)(int, int), char *, char *));
void _setlist_init P((void));
void set_escape_char P((char *));
int set_mode P((int));
int clear_mode P((int));
int modehelp P((int));
int suspend P((int, char *[]));
int shell P((int, char *[]));
int quit P((int, char *[]));
int logout P((int, char *[]));
int env_cmd P((int, char *[]));
struct env_lst *env_find P((unsigned char *));
void env_init P((void));
struct env_lst *env_define P((unsigned char *, unsigned char *));
struct env_lst *env_undefine P((unsigned char *, unsigned char *));
struct env_lst *env_export P((unsigned char *, unsigned char *));
struct env_lst *env_unexport P((unsigned char *, unsigned char *));
struct env_lst *env_send P((unsigned char *, unsigned char *));
struct env_lst *env_list P((unsigned char *, unsigned char *));
unsigned char *env_default P((int, int ));
unsigned char *env_getvalue P((unsigned char *));
void env_varval P((unsigned char *));
int auth_cmd P((int, char *[]));
int ayt_status P((void));
int encrypt_cmd P((int, char *[]));
int tn P((int, char *[]));
void command P((int, char *, int));
void cmdrc P((const char *, const char *));
struct addrinfo;
int sourceroute P((struct addrinfo *, char *, char **, int *, int*));

/* main.c */
void tninit P((void));
void usage P((void));

/* network.c */
void init_network P((void));
int stilloob P((void));
void setneturg P((void));
int netflush P((void));

/* sys_bsd.c */
void init_sys P((void));
int TerminalWrite P((char *, int));
int TerminalRead P((unsigned char *, int));
int TerminalAutoFlush P((void));
int TerminalSpecialChars P((int));
void TerminalFlushOutput P((void));
void TerminalSaveState P((void));
cc_t *tcval P((int));
void TerminalDefaultChars P((void));
void TerminalRestoreState P((void));
void TerminalNewMode P((int));
void TerminalSpeeds P((long *, long *));
int TerminalWindowSize P((long *, long *));
int NetClose P((int));
void NetNonblockingIO P((int, int));
void NetSigIO P((int, int));
void NetSetPgrp P((int));
void sys_telnet_init P((void));
int process_rings P((int , int , int , int , int , int));

/* telnet.c */
void init_telnet P((void));
void send_do P((int, int ));
void send_dont P((int, int ));
void send_will P((int, int ));
void send_wont P((int, int ));
void willoption P((int));
void wontoption P((int));
char **mklist P((char *, char *));
int is_unique P((char *, char **, char **));
int setup_term P((char *, int, int *));
char *gettermname P((void));
void lm_will P((unsigned char *, int));
void lm_wont P((unsigned char *, int));
void lm_do P((unsigned char *, int));
void lm_dont P((unsigned char *, int));
void lm_mode P((unsigned char *, int, int ));
void slc_init P((void));
void slcstate P((void));
void slc_mode_export P((int));
void slc_mode_import P((int));
void slc_import P((int));
void slc_export P((void));
void slc P((unsigned char *, int));
void slc_check P((void));
void slc_start_reply P((void));
void slc_add_reply P((unsigned int, unsigned int, cc_t));
void slc_end_reply P((void));
int slc_update P((void));
void env_opt P((unsigned char *, int));
void env_opt_start P((void));
void env_opt_start_info P((void));
void env_opt_add P((unsigned char *));
int opt_welldefined P((char *));
void env_opt_end P((int));
int telrcv P((void));
int rlogin_susp P((void));
int Scheduler P((int));
void telnet P((const char *));
void xmitAO P((void));
void xmitEL P((void));
void xmitEC P((void));
int dosynch P((char *));
int get_status P((char *));
void intp P((void));
void sendbrk P((void));
void sendabort P((void));
void sendsusp P((void));
void sendeof P((void));
void sendayt P((void));
void sendnaws P((void));
void tel_enter_binary P((int));
void tel_leave_binary P((int));

/* terminal.c */
void init_terminal P((void));
int ttyflush P((int));
int getconnmode P((void));
void setconnmode P((int));
void setcommandmode P((void));

/* utilities.c */
void upcase P((char *));
int SetSockOpt P((int, int , int , int ));
void SetNetTrace P((char *));
void Dump P((int, unsigned char *, int));
void printoption P((char *, int, int ));
void optionstatus P((void));
void printsub P((int, unsigned char *, int));
void EmptyTerminal P((void));
void SetForExit P((void));
void Exit P((int)) __attribute__((__noreturn__));
void ExitString P((char *, int)) __attribute__((__noreturn__));

#ifndef	USE_TERMIO

extern struct	tchars ntc;
extern struct	ltchars nltc;
extern struct	sgttyb nttyb;

# define termEofChar		ntc.t_eofc
# define termEraseChar		nttyb.sg_erase
# define termFlushChar		nltc.t_flushc
# define termIntChar		ntc.t_intrc
# define termKillChar		nttyb.sg_kill
# define termLiteralNextChar	nltc.t_lnextc
# define termQuitChar		ntc.t_quitc
# define termSuspChar		nltc.t_suspc
# define termRprntChar		nltc.t_rprntc
# define termWerasChar		nltc.t_werasc
# define termStartChar		ntc.t_startc
# define termStopChar		ntc.t_stopc
# define termForw1Char		ntc.t_brkc
extern cc_t termForw2Char;
extern cc_t termAytChar;

# define termEofCharp		(cc_t *)&ntc.t_eofc
# define termEraseCharp		(cc_t *)&nttyb.sg_erase
# define termFlushCharp		(cc_t *)&nltc.t_flushc
# define termIntCharp		(cc_t *)&ntc.t_intrc
# define termKillCharp		(cc_t *)&nttyb.sg_kill
# define termLiteralNextCharp	(cc_t *)&nltc.t_lnextc
# define termQuitCharp		(cc_t *)&ntc.t_quitc
# define termSuspCharp		(cc_t *)&nltc.t_suspc
# define termRprntCharp		(cc_t *)&nltc.t_rprntc
# define termWerasCharp		(cc_t *)&nltc.t_werasc
# define termStartCharp		(cc_t *)&ntc.t_startc
# define termStopCharp		(cc_t *)&ntc.t_stopc
# define termForw1Charp		(cc_t *)&ntc.t_brkc
# define termForw2Charp		(cc_t *)&termForw2Char
# define termAytCharp		(cc_t *)&termAytChar

# else

extern struct	termio new_tc;

# define termEofChar		new_tc.c_cc[VEOF]
# define termEraseChar		new_tc.c_cc[VERASE]
# define termIntChar		new_tc.c_cc[VINTR]
# define termKillChar		new_tc.c_cc[VKILL]
# define termQuitChar		new_tc.c_cc[VQUIT]

# ifndef	VSUSP
extern cc_t termSuspChar;
# else
#  define termSuspChar		new_tc.c_cc[VSUSP]
# endif
# if	defined(VFLUSHO) && !defined(VDISCARD)
#  define VDISCARD VFLUSHO
# endif
# ifndef	VDISCARD
extern cc_t termFlushChar;
# else
#  define termFlushChar		new_tc.c_cc[VDISCARD]
# endif
# ifndef VWERASE
extern cc_t termWerasChar;
# else
#  define termWerasChar		new_tc.c_cc[VWERASE]
# endif
# ifndef	VREPRINT
extern cc_t termRprntChar;
# else
#  define termRprntChar		new_tc.c_cc[VREPRINT]
# endif
# ifndef	VLNEXT
extern cc_t termLiteralNextChar;
# else
#  define termLiteralNextChar	new_tc.c_cc[VLNEXT]
# endif
# ifndef	VSTART
extern cc_t termStartChar;
# else
#  define termStartChar		new_tc.c_cc[VSTART]
# endif
# ifndef	VSTOP
extern cc_t termStopChar;
# else
#  define termStopChar		new_tc.c_cc[VSTOP]
# endif
# ifndef	VEOL
extern cc_t termForw1Char;
# else
#  define termForw1Char		new_tc.c_cc[VEOL]
# endif
# ifndef	VEOL2
extern cc_t termForw2Char;
# else
#  define termForw2Char		new_tc.c_cc[VEOL]
# endif
# ifndef	VSTATUS
extern cc_t termAytChar;
#else
#  define termAytChar		new_tc.c_cc[VSTATUS]
#endif

# if !defined(CRAY) || defined(__STDC__)
#  define termEofCharp		&termEofChar
#  define termEraseCharp	&termEraseChar
#  define termIntCharp		&termIntChar
#  define termKillCharp		&termKillChar
#  define termQuitCharp		&termQuitChar
#  define termSuspCharp		&termSuspChar
#  define termFlushCharp	&termFlushChar
#  define termWerasCharp	&termWerasChar
#  define termRprntCharp	&termRprntChar
#  define termLiteralNextCharp	&termLiteralNextChar
#  define termStartCharp	&termStartChar
#  define termStopCharp		&termStopChar
#  define termForw1Charp	&termForw1Char
#  define termForw2Charp	&termForw2Char
#  define termAytCharp		&termAytChar
# else
	/* Work around a compiler bug */
#  define termEofCharp		0
#  define termEraseCharp	0
#  define termIntCharp		0
#  define termKillCharp		0
#  define termQuitCharp		0
#  define termSuspCharp		0
#  define termFlushCharp	0
#  define termWerasCharp	0
#  define termRprntCharp	0
#  define termLiteralNextCharp	0
#  define termStartCharp	0
#  define termStopCharp		0
#  define termForw1Charp	0
#  define termForw2Charp	0
#  define termAytCharp		0
# endif
#endif


/* Tn3270 section */
#if	defined(TN3270)

extern int
    HaveInput,		/* Whether an asynchronous I/O indication came in */
    noasynchtty,	/* Don't do signals on I/O (SIGURG, SIGIO) */
    noasynchnet,	/* Don't do signals on I/O (SIGURG, SIGIO) */
    sigiocount,		/* Count of SIGIO receptions */
    shell_active;	/* Subshell is active */

extern char
    *Ibackp,		/* Oldest byte of 3270 data */
    Ibuf[],		/* 3270 buffer */
    *Ifrontp,		/* Where next 3270 byte goes */
    tline[200],
    *transcom;		/* Transparent command */

/* tn3270.c */
void init_3270 P((void));
int DataToNetwork P((char *, int, int));
void inputAvailable P((int));
void outputPurge P((void));
int DataToTerminal P((char *, int));
int Push3270 P((void));
void Finish3270 P((void));
void StringToTerminal P((char *));
void _putchar P((int));
void SetIn3270 P((void));
int tn3270_ttype P((void));
int settranscom P((int, char *[]));
int shell_continue P((void));
int DataFromTerminal __P((char *, int));
int DataFromNetwork __P((char *, int, int));
void ConnectScreen __P((void));
int DoTerminalOutput __P((void));

#endif	/* defined(TN3270) */
