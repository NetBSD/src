#	$NetBSD: Makefile,v 1.4 2001/10/24 23:58:58 thorpej Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		mdsetimage postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
