#	$NetBSD: Makefile,v 1.3 2001/06/21 13:33:02 hubertf Exp $

SUBDIR=	notes utils

.if exists(${MACHINE}/miniroot)
SUBDIR+= miniroot
.endif

.if exists(${MACHINE}/Makefile)
SUBDIR+= ${MACHINE}
.endif


.include <bsd.subdir.mk>
