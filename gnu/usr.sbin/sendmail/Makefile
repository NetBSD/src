#	$NetBSD: Makefile,v 1.2.2.2 2001/03/09 18:01:23 he Exp $

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
