#	$NetBSD: Makefile,v 1.9 2003/03/05 22:09:08 wiz Exp $

# XXX Temporary for NO_SENDMAIL
.include <bsd.own.mk>

SUBDIR=		dbsym mdsetimage postfix

.if !defined(NO_SENDMAIL) || ${NO_SENDMAIL} == "no"
SUBDIR+=	sendmail
.endif

.include <bsd.subdir.mk>
