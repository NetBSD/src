#	$NetBSD: Makefile,v 1.4 2020/07/23 09:13:18 tsutsui Exp $

.include "../Makefile.common"

SUBDIR= xnest vfb
.if ${XSERVER_XFREE86} != "no"
SUBDIR+= xfree86
.endif

.if ${XSERVER_X68K} != "no"
SUBDIR+=netbsd/x68k
.endif

.if ${XSERVER_XSUN} != "no" || \
    ${XSERVER_XSUNMONO} != "no" || \
    ${XSERVER_XSUN24} != "no"
SUBDIR+=sun
.endif

.if 0
.if ${XSERVER_XALPHANETBSD} != "no"
SUBDIR+=netbsd/alpha
.endif

.if ${XSERVER_XAMIGA} != "no"
SUBDIR+=netbsd/amiga
.endif

.if ${XSERVER_XARM32VIDC} != "no"
SUBDIR+=netbsd/arm32vidc
.endif

.if ${XSERVER_XDECNETBSD} != "no"
SUBDIR+=netbsd/dec
.endif

.if ${XSERVER_XMAC68K} != "no"
SUBDIR+=netbsd/mac68k
.endif

.if ${XSERVER_XVFB} != "no"
SUBDIR+=vfb
.endif
.endif

.include <bsd.subdir.mk>
