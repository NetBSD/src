#	$NetBSD: Makefile,v 1.5 2003/12/23 10:20:57 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	Xnest Xprt Xvfb

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SUBDIR+=	Xsun Xsun24 XsunMono
.else
SUBDIR+=	XFree86
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
