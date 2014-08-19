#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:51:50 tls Exp $

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
