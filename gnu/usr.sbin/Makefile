#	$NetBSD: Makefile,v 1.3.2.1 2002/10/01 04:14:16 itojun Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		map-mbone mrinfo mrouted mtrace postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
