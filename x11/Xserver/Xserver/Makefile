#	$NetBSD: Makefile,v 1.8 2004/01/07 13:07:49 minoura Exp $

.include <bsd.own.mk>

.include "../Makefile.common"

SUBDIR=		Xnest Xprt

.if ${XSERVER_XVFB} != "no"
SUBDIR+=	Xvfb
.endif

.if ${XSERVER_XSUN} == "yes"
SUBDIR+=	Xsun Xsun24 XsunMono
.endif

.if ${XSERVER_XHPC} == "yes" 
SUBDIR+=	Xhpc
.endif

.if ${XSERVER_X68K} == "yes" 
SUBDIR+=	X68k
.endif

.if ${XSERVER_XFREE86} == "yes" 
SUBDIR+=	XFree86
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
