#	$NetBSD: Makefile,v 1.6 2002/03/12 12:51:57 lukem Exp $

SUBDIR=	notes

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.include <bsd.subdir.mk>
