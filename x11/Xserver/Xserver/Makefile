#	$NetBSD: Makefile,v 1.11 2004/01/18 05:35:03 rtr Exp $

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

.if ${XSERVER_XMAC68K} == "yes"
SUBDIR+=	Xmac68k
.endif

.if ${XSERVER_XAMIGA} == "yes"
SUBDIR+=	Xamiga
.endif

.if ${XSERVER_XDECNETBSD} == "yes"
SUBDIR+=	XdecNetBSD
.endif

.if ${XSERVER_XALPHANETBSD} == "yes"
SUBDIR+=	XalphaNetBSD
.endif

.if ${XSERVER_XARM32VIDC} == "yes"
SUBDIR+=	Xarm32VIDC
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
