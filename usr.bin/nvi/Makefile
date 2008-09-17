#	$NetBSD: Makefile,v 1.2.2.2 2008/09/17 05:42:10 wrstuden Exp $

.include "Makefile.inc"

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog
.endif

SUBDIR+= build recover docs

.include <bsd.subdir.mk>
