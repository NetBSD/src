#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 15:50:32 yamt Exp $

.include "Makefile.inc"

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog
.endif

SUBDIR+= usr.bin

.if ${MKDOC} != "no"
SUBDIR+= docs
.endif

.include <bsd.subdir.mk>
