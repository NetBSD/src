#	$NetBSD: Makefile,v 1.2 2008/09/02 09:25:39 christos Exp $

.include "Makefile.inc"

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR+= catalog
.endif

SUBDIR+= build recover docs

.include <bsd.subdir.mk>
