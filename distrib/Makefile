#	$NetBSD: Makefile,v 1.14.12.1 2008/09/28 11:09:15 mjf Exp $

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

.if make(obj)
SUBDIR+= utils
.endif

TARGETS+=release
TARGETS+=iso_image

.include <bsd.subdir.mk>
