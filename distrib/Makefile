#	$NetBSD: Makefile,v 1.10 2002/07/23 10:43:50 lukem Exp $

SUBDIR=	notes

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

TARGETS+=release

.include <bsd.subdir.mk>
