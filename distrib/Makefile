#	$NetBSD: Makefile,v 1.11 2002/07/25 00:35:33 matt Exp $

SUBDIR=	notes .WAIT

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

TARGETS+=release

.include <bsd.subdir.mk>
