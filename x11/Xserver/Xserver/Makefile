#	$NetBSD: Makefile,v 1.4 2003/12/20 01:38:00 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	Xnest Xprt Xvfb
XTODO=	XFree86 

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SUBDIR+=	Xsun Xsun24 XsunMono
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
