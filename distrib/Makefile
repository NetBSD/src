#	$NetBSD: Makefile,v 1.2 1999/04/25 12:05:36 veego Exp $

SUBDIR=	notes utils

.if exists(${MACHINE}/miniroot)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif

.include <bsd.subdir.mk>
