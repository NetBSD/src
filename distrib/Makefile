#	$NetBSD: Makefile,v 1.13 2004/05/12 16:56:41 cl Exp $

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

.include <bsd.subdir.mk>
