#	$NetBSD: Makefile,v 1.9.2.1 2002/07/24 06:53:57 lukem Exp $

SUBDIR=	notes

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

TARGETS+=release

.include <bsd.subdir.mk>
