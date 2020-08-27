#	$NetBSD: Makefile,v 1.1.2.2 2020/08/27 09:05:37 martin Exp $

.include "../../Makefile.common"

SUBDIR= Xsun

.if ${XSERVER_XSUNMONO} != "no"
SUBDIR+= XsunMono
.endif

.if ${XSERVER_XSUN24} != "no"
SUBDIR+= Xsun24
.endif

.include <bsd.subdir.mk>
