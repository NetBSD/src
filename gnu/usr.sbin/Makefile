#	$NetBSD: Makefile,v 1.3 2000/05/02 02:07:41 perry Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
