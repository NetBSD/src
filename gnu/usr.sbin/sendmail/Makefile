#	$NetBSD: Makefile,v 1.2 2000/05/03 10:50:56 itojun Exp $

.include <bsd.own.mk>

SUBDIR=		libsmdb libsmutil sendmail mailstats makemap praliases smrsh

.if ${MKSHARE} != "no"
SUBDIR+=	cf/cf doc
.if make(install)
SUBDIR+=	cf
.endif
.endif

.include <bsd.subdir.mk>
