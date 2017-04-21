/*	$NetBSD: smtputf8.h,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _SMTPUTF8_H_INCLUDED_
#define _SMTPUTF8_H_INCLUDED_

/*++
/* NAME
/*	smtputf8 3h
/* SUMMARY
/*	SMTPUTF8 support
/* SYNOPSIS
/*	#include <smtputf8.h>
/* DESCRIPTION
/* .nf

 /*
  * Avoiding chicken-and-egg problems during the initial SMTPUTF8 roll-out in
  * environments with pre-existing mail flows that contain UTF8.
  * 
  * Prior to SMTPUTF8, mail flows that contain UTF8 worked because the vast
  * majority of MTAs is perfectly capable of handling UTF8 in addres
  * localparts (and in headers), even if pre-SMTPUTF8 standards do not
  * support this practice.
  * 
  * When turning on Postfix SMTPUTF8 support for the first time, we don't want
  * to suddenly break pre-existing mail flows that contain UTF8 because 1) a
  * client does not request SMTPUTF8 support, and because 2) a down-stream
  * MTA does not announce SMTPUTF8 support.
  * 
  * While 1) is easy enough to avoid (keep accepting UTF8 in addres localparts
  * just like Postfix has always done), 2) presents a thornier problem. The
  * root cause of that problem is the need for SMTPUTF8 autodetection.
  * 
  * What is SMTPUTF8 autodetection? Postfix cannot rely solely on the sender's
  * declaration that a message requires SMTPUTF8 support, because UTF8 may be
  * introduced during local processing (for example, the client hostname in
  * Postfix's Received: header, adding @$myorigin or .$mydomain to an
  * incomplete address, address rewriting, alias expansion, automatic BCC
  * recipients, local forwarding, and changes made by header checks or Milter
  * applications).
  * 
  * In summary, after local processing has happened, Postfix may decide that a
  * message requires SMTPUTF8 support, even when that message initially did
  * not require SMTPUTF8 support. This could make the message undeliverable
  * to destinations that do not support SMTPUTF8. In an environment with
  * pre-existing mail flows that contain UTF8, we want to avoid disrupting
  * those mail flows when rolling out SMTPUTF8 support.
  * 
  * For the vast majority of sites, the simplest solution is to autodetect
  * SMTPUTF8 support only for Postfix sendmail command-line submissions, at
  * least as long as SMTPUTF8 support has not yet achieved wold domination.
  * 
  * However, sites that add UTF8 content via local processing (see above) should
  * autodetect SMTPUTF8 support for all email.
  * 
  * smtputf8_autodetect() uses the setting of the smtputf8_autodetect_classes
  * parameter, and the mail source classes defined in mail_params.h.
  */
extern int smtputf8_autodetect(int);

 /*
  * The flag SMTPUTF8_FLAG_REQUESTED is raised on request by the sender, or
  * when a queue file contains at least one UTF8 envelope recipient. One this
  * flag is raised it is preserved when mail is forwarded or bounced.
  * 
  * The flag SMTPUTF8_FLAG_HEADER is raised when a queue file contains at least
  * one UTF8 message header.
  * 
  * The flag SMTPUTF8_FLAG_SENDER is raised when a queue file contains an UTF8
  * envelope sender.
  * 
  * The three flags SMTPUTF8_FLAG_REQUESTED/HEADER/SENDER are stored in the
  * queue file, are sent with delivery requests to Postfix delivery agents,
  * and are sent with "flush" requests to the bounce daemon to ensure that
  * the resulting notification message will have a content-transfer-encoding
  * of 8bit.
  * 
  * In the future, mailing lists will have a mix of UTF8 and non-UTF8
  * subscribers. With the following flag, Postfix can avoid requiring
  * SMTPUTF8 delivery when it isn't really needed.
  * 
  * The flag SMTPUTF8_FLAG_RECIPIENT is raised when a delivery request (NOT:
  * message) contains at least one UTF8 envelope recipient. The flag is NOT
  * stored in the queue file. The flag sent in requests to the bounce daemon
  * ONLY when bouncing a single recipient. The flag is used ONLY in requests
  * to Postfix delivery agents, to give Postfix flexibility when delivering
  * messages to non-SMTPUTF8 servers.
  * 
  * If a delivery request has none of the flags SMTPUTF8_FLAG_RECIPIENT,
  * SMTPUTF8_FLAG_SENDER, or SMTPUTF8_FLAG_HEADER, then the message can
  * safely be delivered to a non-SMTPUTF8 server (DSN original recipients
  * will be encoded appropriately per RFC 6533).
  * 
  * To allow even more SMTPUTF8 mail to be sent to non-SMTPUTF8 servers,
  * implement RFC 2047 header encoding in the Postfix SMTP client, and update
  * the SMTP client protocol engine.
  */
#define SMTPUTF8_FLAG_NONE	(0)
#define SMTPUTF8_FLAG_REQUESTED	(1<<0)	/* queue file/delivery/bounce request */
#define SMTPUTF8_FLAG_HEADER	(1<<1)	/* queue file/delivery/bounce request */
#define SMTPUTF8_FLAG_SENDER	(1<<2)	/* queue file/delivery/bounce request */
#define SMTPUTF8_FLAG_RECIPIENT	(1<<3)	/* delivery request only */

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
