#	$NetBSD: Makefile,v 1.5 2001/12/15 03:23:39 thorpej Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
