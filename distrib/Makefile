#	$NetBSD: Makefile,v 1.12 2004/04/25 06:19:49 christos Exp $

SUBDIR=	notes .WAIT

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.if make(obj)
SUBDIR+= utils
.endif

TARGETS+=release

.include <bsd.subdir.mk>
