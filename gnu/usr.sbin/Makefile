#	$NetBSD: Makefile,v 1.5.2.1 2002/12/07 00:35:12 he Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage mrouted postfix

.if !defined(NO_SENDMAIL)
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
