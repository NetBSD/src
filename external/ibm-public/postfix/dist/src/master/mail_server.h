/*	$NetBSD: mail_server.h,v 1.1.1.4 2013/01/02 18:59:01 tron Exp $	*/

/*++
/* NAME
/*	mail_server 3h
/* SUMMARY
/*	skeleton servers
/* SYNOPSIS
/*	#include <mail_server.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface. Tables are defined in mail_conf.h.
  */
#define MAIL_SERVER_INT_TABLE	1
#define MAIL_SERVER_STR_TABLE	2
#define MAIL_SERVER_BOOL_TABLE	3
#define MAIL_SERVER_TIME_TABLE	4
#define MAIL_SERVER_RAW_TABLE	5
#define MAIL_SERVER_NINT_TABLE	6
#define MAIL_SERVER_NBOOL_TABLE	7
#define MAIL_SERVER_LONG_TABLE	8

#define	MAIL_SERVER_PRE_INIT	10
#define MAIL_SERVER_POST_INIT	11
#define MAIL_SERVER_LOOP	12
#define MAIL_SERVER_EXIT	13
#define MAIL_SERVER_PRE_ACCEPT	14
#define MAIL_SERVER_SOLITARY	15
#define MAIL_SERVER_UNLIMITED	16
#define MAIL_SERVER_PRE_DISCONN	17
#define MAIL_SERVER_PRIVILEGED	18
#define MAIL_SERVER_WATCHDOG	19

#define MAIL_SERVER_IN_FLOW_DELAY	20
#define MAIL_SERVER_SLOW_EXIT	21

typedef void (*MAIL_SERVER_INIT_FN) (char *, char **);
typedef int (*MAIL_SERVER_LOOP_FN) (char *, char **);
typedef void (*MAIL_SERVER_EXIT_FN) (char *, char **);
typedef void (*MAIL_SERVER_ACCEPT_FN) (char *, char **);
typedef void (*MAIL_SERVER_DISCONN_FN) (VSTREAM *, char *, char **);
typedef void (*MAIL_SERVER_SLOW_EXIT_FN) (char *, char **);

 /*
  * single_server.c
  */
typedef void (*SINGLE_SERVER_FN) (VSTREAM *, char *, char **);
extern NORETURN single_server_main(int, char **, SINGLE_SERVER_FN,...);

 /*
  * multi_server.c
  */
typedef void (*MULTI_SERVER_FN) (VSTREAM *, char *, char **);
extern NORETURN multi_server_main(int, char **, MULTI_SERVER_FN,...);
extern void multi_server_disconnect(VSTREAM *);
extern int multi_server_drain(void);

 /*
  * event_server.c
  */
typedef void (*EVENT_SERVER_FN) (VSTREAM *, char *, char **);
extern NORETURN event_server_main(int, char **, EVENT_SERVER_FN,...);
extern void event_server_disconnect(VSTREAM *);
extern int event_server_drain(void);

 /*
  * trigger_server.c
  */
typedef void (*TRIGGER_SERVER_FN) (char *, int, char *, char **);
extern NORETURN trigger_server_main(int, char **, TRIGGER_SERVER_FN,...);

#define TRIGGER_BUF_SIZE	1024

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/
