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
  * External interface.
  */
extern void smtpd_chat_reset(SMTPD_STATE *);
extern void smtpd_chat_query(SMTPD_STATE *);
extern void PRINTFLIKE(2, 3) smtpd_chat_reply(SMTPD_STATE *, char *, ...);
extern void smtpd_chat_notify(SMTPD_STATE *);

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

