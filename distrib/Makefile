#	$NetBSD: Makefile,v 1.18 2012/01/22 03:53:32 tsutsui Exp $

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
TARGETS+=live_image
TARGETS+=install_image

.include <bsd.subdir.mk>
