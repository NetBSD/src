#	$NetBSD: Makefile,v 1.13.10.1 2007/03/09 06:59:46 riz Exp $

.include <bsd.own.mk>

SUBDIR=	notes .WAIT

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
