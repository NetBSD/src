#	$NetBSD: Makefile,v 1.9.2.2 2002/07/29 15:55:56 lukem Exp $

SUBDIR=	notes .WAIT

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

TARGETS+=release

.include <bsd.subdir.mk>
