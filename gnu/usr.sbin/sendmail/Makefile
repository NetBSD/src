#	$NetBSD: Makefile,v 1.1.1.1 2000/02/08 13:45:34 tron Exp $

.include <bsd.own.mk>

SUBDIR=		sendmail mailstats makemap praliases smrsh

.if ${MKSHARE} != "no"
SUBDIR+=	cf/cf
.if make(install)
SUBDIR+=	doc/intro doc/op cf
.endif
.endif

.include <bsd.subdir.mk>
