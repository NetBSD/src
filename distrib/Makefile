#	$NetBSD: Makefile,v 1.5 2002/03/06 13:32:51 lukem Exp $

SUBDIR=	notes utils

.if exists(${MACHINE}/miniroot/Makefile.inc)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.include <bsd.subdir.mk>
