#	$NetBSD: Makefile,v 1.3 2008/10/13 18:11:54 dyoung Exp $

.include "Makefile.inc"

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog
.endif

SUBDIR+= build recover

.if ${MKDOC} != "no"
SUBDIR+= docs
.endif

.include <bsd.subdir.mk>
