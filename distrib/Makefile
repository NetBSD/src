#	$NetBSD: Makefile,v 1.7 2002/04/02 22:36:59 lukem Exp $

SUBDIR=	notes utils

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.include <bsd.subdir.mk>
