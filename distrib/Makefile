#	$NetBSD: Makefile,v 1.20 2026/10/08 17:50:26 mrg Exp $

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
