#	$NetBSD: Makefile,v 1.3 2003/12/20 01:32:06 rtr Exp $

SUBDIR=	vfb xnest

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SUBDIR+=sun
.endif

.include <bsd.subdir.mk>
