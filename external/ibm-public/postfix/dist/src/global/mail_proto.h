/*	$NetBSD: mail_proto.h,v 1.1.1.2.4.1 2013/01/23 00:05:02 yamt Exp $	*/

#ifndef _MAIL_PROTO_H_INCLUDED_
#define _MAIL_PROTO_H_INCLUDED_

/*++
/* NAME
/*	mail_proto 3h
/* SUMMARY
/*	mail internal and external protocol support
/* SYNOPSIS
/*	#include <mail_proto.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <iostuff.h>
#include <attr.h>

 /*
  * External protocols.
  */
#define MAIL_PROTO_SMTP		"SMTP"
#define MAIL_PROTO_ESMTP	"ESMTP"
#define MAIL_PROTO_QMQP		"QMQP"

 /*
  * Names of services: these are the names of the UNIX-domain socket or or
  * FIFO that a service listens on.
  */
#define MAIL_SERVICE_BOUNCE	"bounce"
#define MAIL_SERVICE_CLEANUP	"cleanup"
#define MAIL_SERVICE_DEFER	"defer"
#define MAIL_SERVICE_FORWARD	"forward"
#define MAIL_SERVICE_LOCAL	"local"
#define MAIL_SERVICE_PICKUP	"pickup"
#define MAIL_SERVICE_QUEUE	"qmgr"
#define MAIL_SERVICE_TLSMGR	"tlsmgr"
#define MAIL_SERVICE_RESOLVE	"resolve"
#define MAIL_SERVICE_REWRITE	"rewrite"
#define MAIL_SERVICE_VIRTUAL	"virtual"
#define MAIL_SERVICE_SMTP	"smtp"
#define MAIL_SERVICE_SMTPD	"smtpd"
#define MAIL_SERVICE_SHOWQ	"showq"
#define MAIL_SERVICE_ERROR	"error"
#define MAIL_SERVICE_RETRY	"retry"
#define MAIL_SERVICE_FLUSH	"flush"
#define MAIL_SERVICE_VERIFY	"verify"
#define MAIL_SERVICE_TRACE	"trace"
#define MAIL_SERVICE_RELAY	"relay"
#define MAIL_SERVICE_PROXYMAP	"proxymap"
#define MAIL_SERVICE_PROXYWRITE	"proxywrite"
#define MAIL_SERVICE_SCACHE	"scache"
#define MAIL_SERVICE_DNSBLOG	"dnsblog"
#define MAIL_SERVICE_TLSPROXY	"tlsproxy"

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
extern int mail_trigger(const char *, const char *, const char *, ssize_t);
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
#define MAIL_ATTR_RCPT_COUNT	"recipient_count"
#define MAIL_ATTR_ORCPT		"original_recipient"
#define MAIL_ATTR_RECIP		"recipient"
#define MAIL_ATTR_WHY		"reason"
#define MAIL_ATTR_VERPDL	"verp_delimiters"
#define MAIL_ATTR_SITE		"site"
#define MAIL_ATTR_OFFSET	"offset"
#define MAIL_ATTR_SIZE		"size"
#define MAIL_ATTR_ERRTO		"errors-to"
#define MAIL_ATTR_RRCPT		"return-receipt"
#define MAIL_ATTR_TIME		"time"
#define MAIL_ATTR_LOCALTIME	"localtime"
#define MAIL_ATTR_CREATE_TIME	"create_time"
#define MAIL_ATTR_RULE		"rule"
#define MAIL_ATTR_ADDR		"address"
#define MAIL_ATTR_TRANSPORT	"transport"
#define MAIL_ATTR_NEXTHOP	"nexthop"
#define MAIL_ATTR_TRACE_FLAGS	"trace_flags"
#define MAIL_ATTR_ADDR_STATUS	"recipient_status"
#define MAIL_ATTR_ACTION	"action"
#define MAIL_ATTR_TABLE		"table"
#define MAIL_ATTR_KEY		"key"
#define MAIL_ATTR_VALUE		"value"
#define MAIL_ATTR_INSTANCE	"instance"
#define MAIL_ATTR_SASL_METHOD	"sasl_method"
#define MAIL_ATTR_SASL_USERNAME	"sasl_username"
#define MAIL_ATTR_SASL_SENDER	"sasl_sender"
#define MAIL_ATTR_ETRN_DOMAIN	"etrn_domain"
#define MAIL_ATTR_DUMMY		"dummy"
#define MAIL_ATTR_STRESS	"stress"
#define MAIL_ATTR_LOG_IDENT	"log_ident"
#define MAIL_ATTR_RWR_CONTEXT	"rewrite_context"

#define MAIL_ATTR_RWR_LOCAL	"local"
#define MAIL_ATTR_RWR_REMOTE	"remote"

#define MAIL_ATTR_TTL		"ttl"
#define MAIL_ATTR_LABEL		"label"
#define MAIL_ATTR_PROP		"property"
#define MAIL_ATTR_FUNC		"function"
#define MAIL_ATTR_CCERT_SUBJECT	"ccert_subject"
#define MAIL_ATTR_CCERT_ISSUER	"ccert_issuer"
#define MAIL_ATTR_CCERT_FINGERPRINT "ccert_fingerprint"
#define MAIL_ATTR_CCERT_PKEY_FPRINT "ccert_pubkey_fingerprint"
#define MAIL_ATTR_CRYPTO_PROTOCOL "encryption_protocol"
#define MAIL_ATTR_CRYPTO_CIPHER	"encryption_cipher"
#define MAIL_ATTR_CRYPTO_KEYSIZE "encryption_keysize"

 /*
  * Suffixes for sender_name, sender_domain etc.
  */
#define MAIL_ATTR_S_NAME	"_name"
#define MAIL_ATTR_S_DOMAIN	"_domain"

 /*
  * Special names for RBL results.
  */
#define MAIL_ATTR_RBL_WHAT	"rbl_what"
#define MAIL_ATTR_RBL_DOMAIN	"rbl_domain"
#define MAIL_ATTR_RBL_REASON	"rbl_reason"
#define MAIL_ATTR_RBL_TXT	"rbl_txt"	/* LaMont compatibility */
#define MAIL_ATTR_RBL_CLASS	"rbl_class"
#define MAIL_ATTR_RBL_CODE	"rbl_code"
#define MAIL_ATTR_RBL_ADDR	"rbl_addr"

 /*
  * The following attribute names are stored in queue files. Changing this
  * means lots of work to maintain backwards compatibility with queued mail.
  */
#define MAIL_ATTR_ENCODING	"encoding"	/* internal encoding */
#define MAIL_ATTR_ENC_8BIT	"8bit"	/* 8BITMIME equivalent */
#define MAIL_ATTR_ENC_7BIT	"7bit"	/* 7BIT equivalent */
#define MAIL_ATTR_ENC_NONE	""	/* encoding unknown */

#define MAIL_ATTR_LOG_CLIENT_NAME "log_client_name"	/* client hostname */
#define MAIL_ATTR_LOG_CLIENT_ADDR "log_client_address"	/* client address */
#define MAIL_ATTR_LOG_CLIENT_PORT "log_client_port"	/* client port */
#define MAIL_ATTR_LOG_HELO_NAME	"log_helo_name"	/* SMTP helo name */
#define MAIL_ATTR_LOG_PROTO_NAME "log_protocol_name"	/* SMTP/ESMTP/QMQP */
#define MAIL_ATTR_LOG_ORIGIN	"log_message_origin"	/* name[addr]:port */

#define MAIL_ATTR_ACT_CLIENT	"client"/* client name addr */
#define MAIL_ATTR_ACT_CLIENT_NAME "client_name"	/* client name */
#define MAIL_ATTR_ACT_CLIENT_ADDR "client_address"	/* client address */
#define MAIL_ATTR_ACT_CLIENT_PORT "client_port"	/* client TCP port */
#define MAIL_ATTR_ACT_CLIENT_AF	"client_address_type"	/* AF_INET etc. */
#define MAIL_ATTR_ACT_HELO_NAME	"helo_name"	/* SMTP helo name */
#define MAIL_ATTR_ACT_PROTO_NAME "protocol_name"	/* SMTP/ESMTP/QMQP */
#define MAIL_ATTR_ACT_REVERSE_CLIENT_NAME "reverse_client_name"
#define MAIL_ATTR_ACT_FORWARD_CLIENT_NAME "forward_client_name"

#define MAIL_ATTR_PROTO_STATE	"protocol_state"	/* MAIL/RCPT/... */
#define MAIL_ATTR_ORG_NONE	"unknown"	/* origin unknown */
#define MAIL_ATTR_ORG_LOCAL	"local"	/* local submission */

 /*
  * XCLIENT/XFORWARD in SMTP.
  */
#define XCLIENT_CMD		"XCLIENT"	/* XCLIENT command */
#define XCLIENT_NAME		"NAME"	/* client name */
#define XCLIENT_REVERSE_NAME	"REVERSE_NAME"	/* reverse client name */
#ifdef FORWARD_CLIENT_NAME
#define XCLIENT_FORWARD_NAME	"FORWARD_NAME"	/* forward client name */
#endif
#define XCLIENT_ADDR		"ADDR"	/* client address */
#define XCLIENT_PORT		"PORT"	/* client port */
#define XCLIENT_PROTO		"PROTO"	/* client protocol */
#define XCLIENT_HELO		"HELO"	/* client helo */
#define XCLIENT_LOGIN		"LOGIN"	/* SASL login name */

#define XCLIENT_UNAVAILABLE	"[UNAVAILABLE]"	/* permanently unavailable */
#define XCLIENT_TEMPORARY	"[TEMPUNAVAIL]"	/* temporarily unavailable */

#define XFORWARD_CMD		"XFORWARD"	/* XFORWARD command */
#define XFORWARD_NAME		"NAME"	/* client name */
#define XFORWARD_ADDR		"ADDR"	/* client address */
#define XFORWARD_PORT		"PORT"	/* client port */
#define XFORWARD_PROTO		"PROTO"	/* client protocol */
#define XFORWARD_HELO		"HELO"	/* client helo */
#define XFORWARD_IDENT		"IDENT"	/* message identifier */
#define XFORWARD_DOMAIN		"SOURCE"/* origin type */
#define XFORWARD_DOM_LOCAL	"LOCAL"	/* local origin */
#define XFORWARD_DOM_REMOTE	"REMOTE"/* remote origin */

#define XFORWARD_UNAVAILABLE	"[UNAVAILABLE]"	/* attribute unavailable */

 /*
  * DSN support.
  */
#define MAIL_ATTR_DSN_STATUS	"status"/* XXX Postfix <2.3 compat */
#define MAIL_ATTR_DSN_DTYPE	"diag_type"	/* dsn diagnostic code */
#define MAIL_ATTR_DSN_DTEXT	"diag_text"	/* dsn diagnostic code */
#define MAIL_ATTR_DSN_MTYPE	"mta_type"	/* dsn remote MTA */
#define MAIL_ATTR_DSN_MNAME	"mta_mname"	/* dsn remote MTA */
#define MAIL_ATTR_DSN_ACTION	"action"/* XXX Postfix <2.3 compat */
#define MAIL_ATTR_DSN_ENVID	"envelope_id"	/* dsn envelope id */
#define MAIL_ATTR_DSN_RET	"ret_flags"	/* dsn full/headers */
#define MAIL_ATTR_DSN_NOTIFY	"notify_flags"	/* dsn notify flags */
#define MAIL_ATTR_DSN_ORCPT	"dsn_orig_rcpt"	/* dsn original recipient */

 /*
  * TLSPROXY support.
  */
#define MAIL_ATTR_REMOTE_ENDPT	"remote_endpoint"	/* name[addr]:port */
#define MAIL_ATTR_ROLE		"role"	/* requested role */
#define MAIL_ATTR_ROLE_SERVER	"server"
#define MAIL_ATTR_ROLE_CLIENT	"client"
#define MAIL_ATTR_TIMEOUT	"timeout"
#define MAIL_ATTR_PEER_CN	"peer_CN"
#define MAIL_ATTR_ISSUER_CN	"issuer_CN"
#define MAIL_ATTR_PEER_FPT	"peer_fingerprint"
#define MAIL_ATTR_PEER_PKEY_FPT	"peer_pubkey_fingerprint"
#define MAIL_ATTR_PEER_STATUS	"peer_status"
#define MAIL_ATTR_CIPHER_PROTOCOL "cipher_protocol"
#define MAIL_ATTR_CIPHER_NAME	"cipher_name"
#define MAIL_ATTR_CIPHER_USEBITS "cipher_usebits"
#define MAIL_ATTR_CIPHER_ALGBITS "cipher_algbits"
#define MAIL_ATTR_SERVER_ID	"server_id"

 /*
  * SMTP reply footer support.
  */
#define MAIL_ATTR_SERVER_NAME	"server_name"

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
