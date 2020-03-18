/*	$NetBSD: smtpd_chat.h,v 1.3 2020/03/18 19:05:20 christos Exp $	*/

/*++
/* NAME
/*	smtpd_chat 3h
/* SUMMARY
/*	SMTP server request/response support
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_chat.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <mail_params.h>

 /*
  * External interface.
  */
extern void smtpd_chat_pre_jail_init(void);
extern void smtpd_chat_reset(SMTPD_STATE *);
extern int smtpd_chat_query_limit(SMTPD_STATE *, int);
extern void smtpd_chat_query(SMTPD_STATE *);
extern void PRINTFLIKE(2, 3) smtpd_chat_reply(SMTPD_STATE *, const char *,...);
extern void vsmtpd_chat_reply(SMTPD_STATE *, const char *, va_list);
extern void smtpd_chat_notify(SMTPD_STATE *);

#define smtpd_chat_query(state) \
	((void) smtpd_chat_query_limit((state), var_line_limit))

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/
