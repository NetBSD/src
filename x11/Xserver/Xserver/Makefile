#	$NetBSD: Makefile,v 1.12 2004/02/07 08:30:34 rtr Exp $

.include <bsd.own.mk>

.include "../Makefile.common"

SUBDIR=		Xnest Xprt

.if ${XSERVER_X68K} != "no" 
SUBDIR+=	X68k
.endif

.if ${XSERVER_XALPHANETBSD} != "no"
SUBDIR+=	XalphaNetBSD
.endif

.if ${XSERVER_XAMIGA} != "no"
SUBDIR+=	Xamiga
.endif

.if ${XSERVER_XARM32VIDC} != "no"
SUBDIR+=	Xarm32VIDC
.endif

.if ${XSERVER_XDECNETBSD} != "no"
SUBDIR+=	XdecNetBSD
.endif

.if ${XSERVER_XDREAMCAST} != "no"
SUBDIR+=	Xdreamcast
.endif

.if ${XSERVER_XFREE86} != "no" 
SUBDIR+=	XFree86
.endif

.if ${XSERVER_XHPC} != "no" 
SUBDIR+=	Xhpc
.endif

.if ${XSERVER_XMAC68K} != "no"
SUBDIR+=	Xmac68k
.endif

.if ${XSERVER_XMACPPC} != "no"
SUBDIR+=	Xmacppc
.endif

.if ${XSERVER_XPMAX} != "no"
SUBDIR+=	Xpmax
.endif

.if ${XSERVER_XSUN} != "no"
SUBDIR+=	Xsun Xsun24 XsunMono
.endif

.if ${XSERVER_XVFB} != "no"
SUBDIR+=	Xvfb
.endif

.include "Makefile.Xserver"

.include <bsd.subdir.mk>
