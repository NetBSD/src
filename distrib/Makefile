#	$NetBSD: Makefile,v 1.1 1998/05/02 07:40:23 ross Exp $

SUBDIR=	utils

.if exists(${MACHINE}/miniroot)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

SUBDIR+= notes

.include <bsd.subdir.mk>
