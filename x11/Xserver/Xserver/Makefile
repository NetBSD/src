#	$NetBSD: Makefile,v 1.7 2004/01/06 12:30:13 mrg Exp $

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

.if ${XSERVER_XFREE86} == "yes" 
SUBDIR+=	XFree86
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
