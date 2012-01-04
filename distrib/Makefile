#	$NetBSD: Makefile,v 1.17 2012/01/04 03:43:34 riz Exp $

.include <bsd.own.mk>

.if ${MKMAN} != "no"
SUBDIR=	notes .WAIT
.endif

SUBDIR+= utils

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${RELEASEMACHINE}/Makefile)
SUBDIR+= ${RELEASEMACHINE}
.endif

TARGETS+=release
TARGETS+=iso_image

.include <bsd.subdir.mk>
