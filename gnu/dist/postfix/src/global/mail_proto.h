#ifndef _MAIL_PROTO_H_INCLUDED_
#define _MAIL_PROTO_H_INCLUDED_

/*++
/* NAME
/*	mail_proto 3h
/* SUMMARY
/*	mail internal IPC support
/* SYNOPSIS
/*	#include <mail_proto.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <iostuff.h>
#include <attr.h>

 /*
  * Names of services: these are the names if INET ports, UNIX-domain sockets
  * or FIFOs that a service listens on.
  */
#define MAIL_SERVICE_BOUNCE	"bounce"
#define MAIL_SERVICE_CLEANUP	"cleanup"
#define MAIL_SERVICE_DEFER	"defer"
#define MAIL_SERVICE_FORWARD	"forward"
#define MAIL_SERVICE_LOCAL	"local"
#define MAIL_SERVICE_PICKUP	"pickup"
#define MAIL_SERVICE_QUEUE	"qmgr"
#define MAIL_SERVICE_RESOLVE	"resolve"
#define MAIL_SERVICE_REWRITE	"rewrite"
#define MAIL_SERVICE_VIRTUAL	"virtual"
#define MAIL_SERVICE_SMTP	"smtp"
#define MAIL_SERVICE_SMTPD	"smtpd"
#define MAIL_SERVICE_SHOWQ	"showq"
#define MAIL_SERVICE_ERROR	"error"
#define MAIL_SERVICE_FLUSH	"flush"

 /*
  * Well-known socket or FIFO directories. The main difference is in file
  * access permissions.
  */
#define MAIL_CLASS_PUBLIC	"public"
#define MAIL_CLASS_PRIVATE	"private"

 /*
  * Generic triggers.
  */
#define TRIGGER_REQ_WAKEUP	'W'	/* wakeup */

 /*
  * Queue manager requests.
  */
#define QMGR_REQ_SCAN_DEFERRED	'D'	/* scan deferred queue */
#define QMGR_REQ_SCAN_INCOMING	'I'	/* scan incoming queue */
#define QMGR_REQ_FLUSH_DEAD	'F'	/* flush dead xport/site */
#define QMGR_REQ_SCAN_ALL	'A'	/* ignore time stamps */

 /*
  * Functional interface.
  */
extern VSTREAM *mail_connect(const char *, const char *, int);
extern VSTREAM *mail_connect_wait(const char *, const char *);
extern int mail_command_client(const char *, const char *,...);
extern int mail_command_server(VSTREAM *,...);
extern int mail_trigger(const char *, const char *, const char *, int);
extern char *mail_pathname(const char *, const char *);

 /*
  * Attribute names.
  */
#define MAIL_ATTR_REQ		"request"
#define MAIL_ATTR_NREQ		"nrequest"
#define MAIL_ATTR_STATUS	"status"

#define MAIL_ATTR_FLAGS		"flags"
#define MAIL_ATTR_QUEUE		"queue_name"
#define MAIL_ATTR_QUEUEID	"queue_id"
#define MAIL_ATTR_SENDER	"sender"
#define MAIL_ATTR_RECIP		"recipient"
#define MAIL_ATTR_WHY		"reason"
#define MAIL_ATTR_VERPDL	"verp_delimiters"
#define MAIL_ATTR_SITE		"site"
#define MAIL_ATTR_OFFSET	"offset"
#define MAIL_ATTR_SIZE		"size"
#define MAIL_ATTR_ERRTO		"errors-to"
#define MAIL_ATTR_RRCPT		"return-receipt"
#define MAIL_ATTR_TIME		"time"
#define MAIL_ATTR_RULE		"rule"
#define MAIL_ATTR_ADDR		"address"
#define MAIL_ATTR_TRANSPORT	"transport"
#define MAIL_ATTR_NEXTHOP	"nexthop"

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

#endif
