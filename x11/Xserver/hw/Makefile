#	$NetBSD: Makefile,v 1.4 2003/12/23 10:26:37 rtr Exp $

SUBDIR=	vfb xnest

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SUBDIR+=sun
.else
SUBDIR+=xfree86
.endif

.include <bsd.subdir.mk>
