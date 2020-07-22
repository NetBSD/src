#	$NetBSD: Makefile,v 1.1 2020/07/22 19:26:37 tsutsui Exp $

.include "../../Makefile.common"

SUBDIR= Xsun

.if ${XSERVER_XSUNMONO} != "no"
SUBDIR+= XsunMono
.endif

.if ${XSERVER_XSUN24} != "no"
SUBDIR+= Xsun24
.endif

.include <bsd.subdir.mk>
