#	$NetBSD: Makefile,v 1.2.2.1 2000/12/26 15:51:54 jhawk Exp $

.include <bsd.own.mk>

SUBDIR=		libsmdb libsmutil sendmail mailstats makemap praliases smrsh

.if ${MKDOC} != "no"
SUBDIR+=	doc
.endif

.if ${MKSHARE} != "no"
SUBDIR+=	cf/cf
.if make(install)
SUBDIR+=	cf
.endif
.endif

.include <bsd.subdir.mk>
