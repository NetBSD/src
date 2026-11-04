#	$NetBSD: Makefile,v 1.19.54.1 2026/11/04 22:36:50 perseant Exp $

.include <bsd.own.mk>

.if ${MKGROFF} != "no"
SUBDIR=	notes .WAIT
.endif

SUBDIR+= utils
SUBDIR+= lib .WAIT

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
