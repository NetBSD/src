#	$NetBSD: Makefile,v 1.8 2002/05/02 17:44:32 lukem Exp $

SUBDIR=	notes

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.include <bsd.subdir.mk>
