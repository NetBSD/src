#	$NetBSD: Makefile,v 1.3 2000/10/23 04:38:38 simonb Exp $

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
