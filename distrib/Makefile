#	$NetBSD: Makefile,v 1.16 2008/11/21 15:39:51 ad Exp $

.include <bsd.own.mk>

.if ${MKMAN} != "no"
SUBDIR=	notes .WAIT
.endif

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${RELEASEMACHINE}/Makefile)
SUBDIR+= ${RELEASEMACHINE}
.endif

SUBDIR+= utils

TARGETS+=release
TARGETS+=iso_image

.include <bsd.subdir.mk>
