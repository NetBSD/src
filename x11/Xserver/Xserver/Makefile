#	$NetBSD: Makefile,v 1.6 2004/01/03 02:05:49 takemura Exp $

.include <bsd.own.mk>

SUBDIR=	Xnest Xprt Xvfb

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SUBDIR+=	Xsun Xsun24 XsunMono
.elif ${MACHINE} == "hpcmips" 
SUBDIR+=	Xhpc
.else
SUBDIR+=	XFree86
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
