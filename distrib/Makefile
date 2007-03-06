#	$NetBSD: Makefile,v 1.14 2007/03/06 21:56:47 bouyer Exp $

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
