#	$NetBSD: Makefile,v 1.9 2002/05/09 11:26:50 lukem Exp $

SUBDIR=	notes

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot .WAIT
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.include <bsd.subdir.mk>
